#include <RendererCore/RendererCorePCH.h>

#include <Core/Console/ConsoleFunction.h>
#include <Core/World/World.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Utilities/DGMLWriter.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Profiling/Profiling.h>

wdCVarBool cvar_RenderingMultithreading("Rendering.Multithreading", true, wdCVarFlags::Default, "Enables multi-threaded update and rendering");
wdCVarBool cvar_RenderingCachingStaticObjects("Rendering.Caching.StaticObjects", true, wdCVarFlags::Default, "Enables render data caching of static objects");

wdEvent<wdView*, wdMutex> wdRenderWorld::s_ViewCreatedEvent;
wdEvent<wdView*, wdMutex> wdRenderWorld::s_ViewDeletedEvent;

wdEvent<void*> wdRenderWorld::s_CameraConfigsModifiedEvent;
bool wdRenderWorld::s_bModifyingCameraConfigs = false;
wdMap<wdString, wdRenderWorld::CameraConfig> wdRenderWorld::s_CameraConfigs;

wdEvent<const wdRenderWorldExtractionEvent&, wdMutex> wdRenderWorld::s_ExtractionEvent;
wdEvent<const wdRenderWorldRenderEvent&, wdMutex> wdRenderWorld::s_RenderEvent;
wdUInt64 wdRenderWorld::s_uiFrameCounter;

namespace
{
  static bool s_bInExtract;
  static wdThreadID s_RenderingThreadID;

  static wdMutex s_ExtractTasksMutex;
  static wdDynamicArray<wdTaskGroupID> s_ExtractTasks;

  static wdMutex s_ViewsMutex;
  static wdIdTable<wdViewId, wdView*> s_Views;

  static wdDynamicArray<wdViewHandle> s_MainViews;

  static wdMutex s_ViewsToRenderMutex;
  static wdDynamicArray<wdView*> s_ViewsToRender;

  static wdDynamicArray<wdSharedPtr<wdRenderPipeline>> s_FilteredRenderPipelines[2];

  struct PipelineToRebuild
  {
    WD_DECLARE_POD_TYPE();

    wdRenderPipeline* m_pPipeline;
    wdViewHandle m_hView;
  };

  static wdMutex s_PipelinesToRebuildMutex;
  static wdDynamicArray<PipelineToRebuild> s_PipelinesToRebuild;

  static wdProxyAllocator* s_pCacheAllocator;

  static wdMutex s_CachedRenderDataMutex;
  using CachedRenderDataPerComponent = wdHybridArray<const wdRenderData*, 4>;
  static wdHashTable<wdComponentHandle, CachedRenderDataPerComponent> s_CachedRenderData;
  static wdDynamicArray<const wdRenderData*> s_DeletedRenderData;

  enum
  {
    MaxNumNewCacheEntries = 32
  };

  static bool s_bWriteRenderPipelineDgml = false;
  static wdConsoleFunction<void()> s_ConFunc_WriteRenderPipelineDgml("WriteRenderPipelineDgml", "()", []() { s_bWriteRenderPipelineDgml = true; });
} // namespace

namespace wdInternal
{
  struct RenderDataCache
  {
    RenderDataCache(wdAllocatorBase* pAllocator)
      : m_PerObjectCaches(pAllocator)
    {
      for (wdUInt32 i = 0; i < MaxNumNewCacheEntries; ++i)
      {
        m_NewEntriesPerComponent.PushBack(NewEntryPerComponent(pAllocator));
      }
    }

    struct PerObjectCache
    {
      PerObjectCache() = default;

      PerObjectCache(wdAllocatorBase* pAllocator)
        : m_Entries(pAllocator)
      {
      }

      wdHybridArray<RenderDataCacheEntry, 4> m_Entries;
      wdUInt16 m_uiVersion = 0;
    };

    wdDynamicArray<PerObjectCache> m_PerObjectCaches;

    struct NewEntryPerComponent
    {
      NewEntryPerComponent(wdAllocatorBase* pAllocator)
        : m_Cache(pAllocator)
      {
      }

      wdGameObjectHandle m_hOwnerObject;
      wdComponentHandle m_hOwnerComponent;
      PerObjectCache m_Cache;
    };

    wdStaticArray<NewEntryPerComponent, MaxNumNewCacheEntries> m_NewEntriesPerComponent;
    wdAtomicInteger32 m_NewEntriesCount;
  };

#if WD_ENABLED(WD_PLATFORM_64BIT)
  WD_CHECK_AT_COMPILETIME(sizeof(RenderDataCacheEntry) == 16);
#endif
} // namespace wdInternal

// clang-format off
WD_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, RenderWorld)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    wdRenderWorld::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    wdRenderWorld::OnEngineShutdown();
  }

WD_END_SUBSYSTEM_DECLARATION;
// clang-format on

wdViewHandle wdRenderWorld::CreateView(const char* szName, wdView*& out_pView)
{
  wdView* pView = WD_DEFAULT_NEW(wdView);

  {
    WD_LOCK(s_ViewsMutex);
    pView->m_InternalId = s_Views.Insert(pView);
  }

  pView->SetName(szName);
  pView->InitializePins();

  pView->m_pRenderDataCache = WD_NEW(s_pCacheAllocator, wdInternal::RenderDataCache, s_pCacheAllocator);

  s_ViewCreatedEvent.Broadcast(pView);

  out_pView = pView;
  return pView->GetHandle();
}

void wdRenderWorld::DeleteView(const wdViewHandle& hView)
{
  wdView* pView = nullptr;

  {
    WD_LOCK(s_ViewsMutex);
    if (!s_Views.Remove(hView, &pView))
      return;
  }

  s_ViewDeletedEvent.Broadcast(pView);

  WD_DELETE(s_pCacheAllocator, pView->m_pRenderDataCache);

  {
    WD_LOCK(s_PipelinesToRebuildMutex);

    for (wdUInt32 i = s_PipelinesToRebuild.GetCount(); i-- > 0;)
    {
      if (s_PipelinesToRebuild[i].m_hView == hView)
      {
        s_PipelinesToRebuild.RemoveAtAndCopy(i);
      }
    }
  }

  RemoveMainView(hView);

  WD_DEFAULT_DELETE(pView);
}

bool wdRenderWorld::TryGetView(const wdViewHandle& hView, wdView*& out_pView)
{
  WD_LOCK(s_ViewsMutex);
  return s_Views.TryGetValue(hView, out_pView);
}

wdView* wdRenderWorld::GetViewByUsageHint(wdCameraUsageHint::Enum usageHint, wdCameraUsageHint::Enum alternativeUsageHint /*= wdCameraUsageHint::None*/, const wdWorld* pWorld /*= nullptr*/)
{
  WD_LOCK(s_ViewsMutex);

  wdView* pAlternativeView = nullptr;

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    wdView* pView = it.Value();
    if (pWorld != nullptr && pView->GetWorld() != pWorld)
      continue;

    if (pView->GetCameraUsageHint() == usageHint)
    {
      return pView;
    }
    else if (alternativeUsageHint != wdCameraUsageHint::None && pView->GetCameraUsageHint() == alternativeUsageHint)
    {
      pAlternativeView = pView;
    }
  }

  return pAlternativeView;
}

void wdRenderWorld::AddMainView(const wdViewHandle& hView)
{
  WD_ASSERT_DEV(!s_bInExtract, "Cannot add main view during extraction");

  if (!s_MainViews.Contains(hView))
    s_MainViews.PushBack(hView);
}

void wdRenderWorld::RemoveMainView(const wdViewHandle& hView)
{
  wdUInt32 uiIndex = s_MainViews.IndexOf(hView);
  if (uiIndex != wdInvalidIndex)
  {
    WD_ASSERT_DEV(!s_bInExtract, "Cannot remove main view during extraction");
    s_MainViews.RemoveAtAndCopy(uiIndex);
  }
}

void wdRenderWorld::ClearMainViews()
{
  WD_ASSERT_DEV(!s_bInExtract, "Cannot clear main views during extraction");

  s_MainViews.Clear();
}

wdArrayPtr<wdViewHandle> wdRenderWorld::GetMainViews()
{
  return s_MainViews;
}

void wdRenderWorld::CacheRenderData(const wdView& view, const wdGameObjectHandle& hOwnerObject, const wdComponentHandle& hOwnerComponent, wdUInt16 uiComponentVersion, wdArrayPtr<wdInternal::RenderDataCacheEntry> cacheEntries)
{
  if (cvar_RenderingCachingStaticObjects)
  {
    wdUInt32 uiNewEntriesCount = view.m_pRenderDataCache->m_NewEntriesCount;
    if (uiNewEntriesCount >= MaxNumNewCacheEntries)
    {
      return;
    }

    uiNewEntriesCount = view.m_pRenderDataCache->m_NewEntriesCount.Increment();
    if (uiNewEntriesCount <= MaxNumNewCacheEntries)
    {
      auto& newEntry = view.m_pRenderDataCache->m_NewEntriesPerComponent[uiNewEntriesCount - 1];
      newEntry.m_hOwnerObject = hOwnerObject;
      newEntry.m_hOwnerComponent = hOwnerComponent;
      newEntry.m_Cache.m_Entries = cacheEntries;
      newEntry.m_Cache.m_uiVersion = uiComponentVersion;
    }
  }
}

void wdRenderWorld::DeleteAllCachedRenderData()
{
  WD_PROFILE_SCOPE("DeleteAllCachedRenderData");

  WD_ASSERT_DEV(!s_bInExtract, "Cannot delete cached render data during extraction");

  {
    WD_LOCK(s_ViewsMutex);

    for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
    {
      wdView* pView = it.Value();
      pView->m_pRenderDataCache->m_PerObjectCaches.Clear();
    }
  }

  {
    WD_LOCK(s_CachedRenderDataMutex);

    for (auto it = s_CachedRenderData.GetIterator(); it.IsValid(); ++it)
    {
      auto& cachedRenderDataPerComponent = it.Value();

      for (auto pCachedRenderData : cachedRenderDataPerComponent)
      {
        s_DeletedRenderData.PushBack(pCachedRenderData);
      }

      cachedRenderDataPerComponent.Clear();
    }
  }
}

void wdRenderWorld::DeleteCachedRenderData(const wdGameObjectHandle& hOwnerObject, const wdComponentHandle& hOwnerComponent)
{
  WD_ASSERT_DEV(!s_bInExtract, "Cannot delete cached render data during extraction");

  DeleteCachedRenderDataInternal(hOwnerObject);

  WD_LOCK(s_CachedRenderDataMutex);

  CachedRenderDataPerComponent* pCachedRenderDataPerComponent = nullptr;
  if (s_CachedRenderData.TryGetValue(hOwnerComponent, pCachedRenderDataPerComponent))
  {
    for (auto pCachedRenderData : *pCachedRenderDataPerComponent)
    {
      s_DeletedRenderData.PushBack(pCachedRenderData);
    }

    s_CachedRenderData.Remove(hOwnerComponent);
  }
}

void wdRenderWorld::ResetRenderDataCache(wdView& ref_view)
{
  ref_view.m_pRenderDataCache->m_PerObjectCaches.Clear();
  ref_view.m_pRenderDataCache->m_NewEntriesCount = 0;

  if (ref_view.GetWorld() != nullptr)
  {
    if (ref_view.GetWorld()->GetObjectDeletionEvent().HasEventHandler(&wdRenderWorld::DeleteCachedRenderDataForObject) == false)
    {
      ref_view.GetWorld()->GetObjectDeletionEvent().AddEventHandler(&wdRenderWorld::DeleteCachedRenderDataForObject);
    }
  }
}

void wdRenderWorld::DeleteCachedRenderDataForObject(const wdGameObject* pOwnerObject)
{
  WD_ASSERT_DEV(!s_bInExtract, "Cannot delete cached render data during extraction");

  DeleteCachedRenderDataInternal(pOwnerObject->GetHandle());

  WD_LOCK(s_CachedRenderDataMutex);

  auto components = pOwnerObject->GetComponents();
  for (auto pComponent : components)
  {
    wdComponentHandle hComponent = pComponent->GetHandle();

    CachedRenderDataPerComponent* pCachedRenderDataPerComponent = nullptr;
    if (s_CachedRenderData.TryGetValue(hComponent, pCachedRenderDataPerComponent))
    {
      for (auto pCachedRenderData : *pCachedRenderDataPerComponent)
      {
        s_DeletedRenderData.PushBack(pCachedRenderData);
      }

      s_CachedRenderData.Remove(hComponent);
    }
  }
}

void wdRenderWorld::DeleteCachedRenderDataForObjectRecursive(const wdGameObject* pOwnerObject)
{
  DeleteCachedRenderDataForObject(pOwnerObject);

  for (auto it = pOwnerObject->GetChildren(); it.IsValid(); ++it)
  {
    DeleteCachedRenderDataForObjectRecursive(it);
  }
}

wdArrayPtr<const wdInternal::RenderDataCacheEntry> wdRenderWorld::GetCachedRenderData(const wdView& view, const wdGameObjectHandle& hOwner, wdUInt16 uiComponentVersion)
{
  if (cvar_RenderingCachingStaticObjects)
  {
    const auto& perObjectCaches = view.m_pRenderDataCache->m_PerObjectCaches;
    wdUInt32 uiCacheIndex = hOwner.GetInternalID().m_InstanceIndex;
    if (uiCacheIndex < perObjectCaches.GetCount())
    {
      auto& perObjectCache = perObjectCaches[uiCacheIndex];
      if (perObjectCache.m_uiVersion == uiComponentVersion)
      {
        return perObjectCache.m_Entries;
      }
    }
  }

  return wdArrayPtr<const wdInternal::RenderDataCacheEntry>();
}

void wdRenderWorld::AddViewToRender(const wdViewHandle& hView)
{
  wdView* pView = nullptr;
  if (!TryGetView(hView, pView))
    return;

  if (!pView->IsValid())
    return;

  {
    WD_LOCK(s_ViewsToRenderMutex);
    WD_ASSERT_DEV(s_bInExtract, "Render views need to be collected during extraction");

    // make sure the view is put at the end of the array, if it is already there, reorder it
    // this ensures that the views that have been referenced by the last other view, get rendered first
    wdUInt32 uiIndex = s_ViewsToRender.IndexOf(pView);
    if (uiIndex != wdInvalidIndex)
    {
      s_ViewsToRender.RemoveAtAndCopy(uiIndex);
      s_ViewsToRender.PushBack(pView);
      return;
    }

    s_ViewsToRender.PushBack(pView);
  }

  if (cvar_RenderingMultithreading)
  {
    wdTaskGroupID extractTaskID = wdTaskSystem::StartSingleTask(pView->GetExtractTask(), wdTaskPriority::EarlyThisFrame);

    {
      WD_LOCK(s_ExtractTasksMutex);
      s_ExtractTasks.PushBack(extractTaskID);
    }
  }
  else
  {
    pView->ExtractData();
  }
}

void wdRenderWorld::ExtractMainViews()
{
  WD_ASSERT_DEV(!s_bInExtract, "ExtractMainViews must not be called from multiple threads.");

  s_bInExtract = true;

  wdRenderWorldExtractionEvent extractionEvent;
  extractionEvent.m_Type = wdRenderWorldExtractionEvent::Type::BeginExtraction;
  extractionEvent.m_uiFrameCounter = s_uiFrameCounter;
  s_ExtractionEvent.Broadcast(extractionEvent);

  if (cvar_RenderingMultithreading)
  {
    s_ExtractTasks.Clear();

    wdTaskGroupID extractTaskID = wdTaskSystem::CreateTaskGroup(wdTaskPriority::EarlyThisFrame);
    s_ExtractTasks.PushBack(extractTaskID);

    {
      WD_LOCK(s_ViewsMutex);

      for (wdUInt32 i = 0; i < s_MainViews.GetCount(); ++i)
      {
        wdView* pView = nullptr;
        if (s_Views.TryGetValue(s_MainViews[i], pView) && pView->IsValid())
        {
          s_ViewsToRender.PushBack(pView);
          wdTaskSystem::AddTaskToGroup(extractTaskID, pView->GetExtractTask());
        }
      }
    }

    wdTaskSystem::StartTaskGroup(extractTaskID);

    {
      WD_PROFILE_SCOPE("Wait for Extraction");

      while (true)
      {
        wdTaskGroupID taskID;

        {
          WD_LOCK(s_ExtractTasksMutex);
          if (s_ExtractTasks.IsEmpty())
            break;

          taskID = s_ExtractTasks.PeekBack();
          s_ExtractTasks.PopBack();
        }

        wdTaskSystem::WaitForGroup(taskID);
      }
    }
  }
  else
  {
    for (wdUInt32 i = 0; i < s_MainViews.GetCount(); ++i)
    {
      wdView* pView = nullptr;
      if (s_Views.TryGetValue(s_MainViews[i], pView) && pView->IsValid())
      {
        s_ViewsToRender.PushBack(pView);
        pView->ExtractData();
      }
    }
  }

  // filter out duplicates and reverse order so that dependent views are rendered first
  {
    auto& filteredRenderPipelines = s_FilteredRenderPipelines[GetDataIndexForExtraction()];
    filteredRenderPipelines.Clear();

    for (wdUInt32 i = s_ViewsToRender.GetCount(); i-- > 0;)
    {
      auto& pRenderPipeline = s_ViewsToRender[i]->m_pRenderPipeline;
      if (!filteredRenderPipelines.Contains(pRenderPipeline))
      {
        filteredRenderPipelines.PushBack(pRenderPipeline);
      }
    }

    s_ViewsToRender.Clear();
  }

  extractionEvent.m_Type = wdRenderWorldExtractionEvent::Type::EndExtraction;
  s_ExtractionEvent.Broadcast(extractionEvent);

  s_bInExtract = false;
}

void wdRenderWorld::Render(wdRenderContext* pRenderContext)
{
  WD_PROFILE_SCOPE("wdRenderWorld::Render");

  wdRenderWorldRenderEvent renderEvent;
  renderEvent.m_Type = wdRenderWorldRenderEvent::Type::BeginRender;
  renderEvent.m_uiFrameCounter = s_uiFrameCounter;
  {
    WD_PROFILE_SCOPE("BeginRender");
    s_RenderEvent.Broadcast(renderEvent);
  }

  if (!cvar_RenderingMultithreading)
  {
    RebuildPipelines();
  }

  auto& filteredRenderPipelines = s_FilteredRenderPipelines[GetDataIndexForRendering()];

  if (s_bWriteRenderPipelineDgml)
  {
    // Executed via WriteRenderPipelineDgml console command.
    s_bWriteRenderPipelineDgml = false;
    const wdDateTime dt = wdTimestamp::CurrentTimestamp();
    for (wdUInt32 i = 0; i < filteredRenderPipelines.GetCount(); ++i)
    {
      auto& pRenderPipeline = filteredRenderPipelines[i];
      wdStringBuilder sPath(":appdata/Profiling/", wdApplication::GetApplicationInstance()->GetApplicationName());
      sPath.AppendFormat("_{0}-{1}-{2}_{3}-{4}-{5}_Pipeline{}_{}.dgml", dt.GetYear(), wdArgU(dt.GetMonth(), 2, true), wdArgU(dt.GetDay(), 2, true), wdArgU(dt.GetHour(), 2, true), wdArgU(dt.GetMinute(), 2, true), wdArgU(dt.GetSecond(), 2, true), i, pRenderPipeline->GetViewName().GetData());

      wdDGMLGraph graph(wdDGMLGraph::Direction::TopToBottom);
      pRenderPipeline->CreateDgmlGraph(graph);
      if (wdDGMLGraphWriter::WriteGraphToFile(sPath, graph).Failed())
      {
        wdLog::Error("Failed to write render pipeline dgml: {}", sPath);
      }
    }
  }

  for (auto& pRenderPipeline : filteredRenderPipelines)
  {
    // If we are the only one holding a reference to the pipeline skip rendering. The pipeline is not needed anymore and will be deleted soon.
    if (pRenderPipeline->GetRefCount() > 1)
    {
      pRenderPipeline->Render(pRenderContext);
    }
    pRenderPipeline = nullptr;
  }

  filteredRenderPipelines.Clear();

  renderEvent.m_Type = wdRenderWorldRenderEvent::Type::EndRender;
  WD_PROFILE_SCOPE("EndRender");
  s_RenderEvent.Broadcast(renderEvent);
}

void wdRenderWorld::BeginFrame()
{
  WD_PROFILE_SCOPE("BeginFrame");

  s_RenderingThreadID = wdThreadUtils::GetCurrentThreadID();

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    wdView* pView = it.Value();
    pView->EnsureUpToDate();
  }

  RebuildPipelines();
}

void wdRenderWorld::EndFrame()
{
  WD_PROFILE_SCOPE("EndFrame");

  ++s_uiFrameCounter;

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    wdView* pView = it.Value();
    if (pView->IsValid())
    {
      pView->ReadBackPassProperties();
    }
  }

  ClearRenderDataCache();
  UpdateRenderDataCache();

  s_RenderingThreadID = (wdThreadID)0;
}

bool wdRenderWorld::GetUseMultithreadedRendering()
{
  return cvar_RenderingMultithreading;
}


bool wdRenderWorld::IsRenderingThread()
{
  return s_RenderingThreadID == wdThreadUtils::GetCurrentThreadID();
}

void wdRenderWorld::DeleteCachedRenderDataInternal(const wdGameObjectHandle& hOwnerObject)
{
  wdUInt32 uiCacheIndex = hOwnerObject.GetInternalID().m_InstanceIndex;
  wdWorld* pWorld = wdWorld::GetWorld(hOwnerObject);

  WD_LOCK(s_ViewsMutex);

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    wdView* pView = it.Value();
    if (pView->GetWorld() != nullptr && pView->GetWorld() == pWorld)
    {
      auto& perObjectCaches = pView->m_pRenderDataCache->m_PerObjectCaches;

      if (uiCacheIndex < perObjectCaches.GetCount())
      {
        perObjectCaches[uiCacheIndex].m_Entries.Clear();
        perObjectCaches[uiCacheIndex].m_uiVersion = 0;
      }
    }
  }
}

void wdRenderWorld::ClearRenderDataCache()
{
  WD_PROFILE_SCOPE("Clear Render Data Cache");

  for (auto pRenderData : s_DeletedRenderData)
  {
    wdRenderData* ptr = const_cast<wdRenderData*>(pRenderData);
    WD_DELETE(s_pCacheAllocator, ptr);
  }

  s_DeletedRenderData.Clear();
}

void wdRenderWorld::UpdateRenderDataCache()
{
  WD_PROFILE_SCOPE("Update Render Data Cache");

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    wdView* pView = it.Value();
    wdUInt32 uiNumNewEntries = wdMath::Min<wdInt32>(pView->m_pRenderDataCache->m_NewEntriesCount, MaxNumNewCacheEntries);
    pView->m_pRenderDataCache->m_NewEntriesCount = 0;

    auto& perObjectCaches = pView->m_pRenderDataCache->m_PerObjectCaches;

    for (wdUInt32 uiNewEntryIndex = 0; uiNewEntryIndex < uiNumNewEntries; ++uiNewEntryIndex)
    {
      auto& newEntries = pView->m_pRenderDataCache->m_NewEntriesPerComponent[uiNewEntryIndex];
      WD_ASSERT_DEV(!newEntries.m_hOwnerObject.IsInvalidated(), "Implementation error");

      // find or create cached render data
      auto& cachedRenderDataPerComponent = s_CachedRenderData[newEntries.m_hOwnerComponent];

      const wdUInt32 uiNumCachedRenderData = cachedRenderDataPerComponent.GetCount();
      if (uiNumCachedRenderData == 0) // Nothing cached yet
      {
        cachedRenderDataPerComponent = CachedRenderDataPerComponent(s_pCacheAllocator);
      }

      wdUInt32 uiCachedRenderDataIndex = 0;
      for (auto& newEntry : newEntries.m_Cache.m_Entries)
      {
        if (newEntry.m_pRenderData != nullptr)
        {
          if (uiCachedRenderDataIndex >= cachedRenderDataPerComponent.GetCount())
          {
            const wdRTTI* pRtti = newEntry.m_pRenderData->GetDynamicRTTI();
            newEntry.m_pRenderData = pRtti->GetAllocator()->Clone<wdRenderData>(newEntry.m_pRenderData, s_pCacheAllocator);

            cachedRenderDataPerComponent.PushBack(newEntry.m_pRenderData);
          }
          else
          {
            // replace with cached render data
            newEntry.m_pRenderData = cachedRenderDataPerComponent[uiCachedRenderDataIndex];
          }

          ++uiCachedRenderDataIndex;
        }
      }

      // add entry for this view
      const wdUInt32 uiCacheIndex = newEntries.m_hOwnerObject.GetInternalID().m_InstanceIndex;
      perObjectCaches.EnsureCount(uiCacheIndex + 1);

      auto& perObjectCache = perObjectCaches[uiCacheIndex];
      if (perObjectCache.m_uiVersion != newEntries.m_Cache.m_uiVersion)
      {
        perObjectCache.m_Entries.Clear();
        perObjectCache.m_uiVersion = newEntries.m_Cache.m_uiVersion;
      }

      for (auto& newEntry : newEntries.m_Cache.m_Entries)
      {
        if (!perObjectCache.m_Entries.Contains(newEntry))
        {
          perObjectCache.m_Entries.PushBack(newEntry);
        }
      }

      // keep entries sorted, otherwise the logic wdExtractor::ExtractRenderData doesn't work
      perObjectCache.m_Entries.Sort();
    }
  }
}

// static
void wdRenderWorld::AddRenderPipelineToRebuild(wdRenderPipeline* pRenderPipeline, const wdViewHandle& hView)
{
  WD_LOCK(s_PipelinesToRebuildMutex);

  for (auto& pipelineToRebuild : s_PipelinesToRebuild)
  {
    if (pipelineToRebuild.m_hView == hView)
    {
      pipelineToRebuild.m_pPipeline = pRenderPipeline;
      return;
    }
  }

  auto& pipelineToRebuild = s_PipelinesToRebuild.ExpandAndGetRef();
  pipelineToRebuild.m_pPipeline = pRenderPipeline;
  pipelineToRebuild.m_hView = hView;
}

// static
void wdRenderWorld::RebuildPipelines()
{
  WD_PROFILE_SCOPE("RebuildPipelines");

  for (auto& pipelineToRebuild : s_PipelinesToRebuild)
  {
    wdView* pView = nullptr;
    if (s_Views.TryGetValue(pipelineToRebuild.m_hView, pView))
    {
      if (pipelineToRebuild.m_pPipeline->Rebuild(*pView) == wdRenderPipeline::PipelineState::RebuildError)
      {
        wdLog::Error("Failed to rebuild pipeline '{}' for view '{}'", pipelineToRebuild.m_pPipeline->m_sName, pView->GetName());
      }
    }
  }

  s_PipelinesToRebuild.Clear();
}

void wdRenderWorld::OnEngineStartup()
{
  s_pCacheAllocator = WD_DEFAULT_NEW(wdProxyAllocator, "Cached Render Data", wdFoundation::GetDefaultAllocator());

  s_CachedRenderData = wdHashTable<wdComponentHandle, CachedRenderDataPerComponent>(s_pCacheAllocator);
}

void wdRenderWorld::OnEngineShutdown()
{
#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  for (auto it : s_CachedRenderData)
  {
    auto& cachedRenderDataPerComponent = it.Value();
    if (cachedRenderDataPerComponent.IsEmpty() == false)
    {
      WD_REPORT_FAILURE("Leaked cached render data of type '{}'", cachedRenderDataPerComponent[0]->GetDynamicRTTI()->GetTypeName());
    }
  }
#endif

  ClearRenderDataCache();

  WD_DEFAULT_DELETE(s_pCacheAllocator);

  s_FilteredRenderPipelines[0].Clear();
  s_FilteredRenderPipelines[1].Clear();

  ClearMainViews();

  for (auto it = s_Views.GetIterator(); it.IsValid(); ++it)
  {
    wdView* pView = it.Value();
    WD_DEFAULT_DELETE(pView);
  }

  s_Views.Clear();
}

void wdRenderWorld::BeginModifyCameraConfigs()
{
  WD_ASSERT_DEBUG(!s_bModifyingCameraConfigs, "Recursive call not allowed.");
  s_bModifyingCameraConfigs = true;
}

void wdRenderWorld::EndModifyCameraConfigs()
{
  WD_ASSERT_DEBUG(s_bModifyingCameraConfigs, "You have to call wdRenderWorld::BeginModifyCameraConfigs first");
  s_bModifyingCameraConfigs = false;
  s_CameraConfigsModifiedEvent.Broadcast(nullptr);
}

void wdRenderWorld::ClearCameraConfigs()
{
  WD_ASSERT_DEBUG(s_bModifyingCameraConfigs, "You have to call wdRenderWorld::BeginModifyCameraConfigs first");
  s_CameraConfigs.Clear();
}

void wdRenderWorld::SetCameraConfig(const char* szName, const CameraConfig& config)
{
  WD_ASSERT_DEBUG(s_bModifyingCameraConfigs, "You have to call wdRenderWorld::BeginModifyCameraConfigs first");
  s_CameraConfigs[szName] = config;
}

const wdRenderWorld::CameraConfig* wdRenderWorld::FindCameraConfig(const char* szName)
{
  auto it = s_CameraConfigs.Find(szName);

  if (!it.IsValid())
    return nullptr;

  return &it.Value();
}

WD_STATICLINK_FILE(RendererCore, RendererCore_RenderWorld_Implementation_RenderWorld);
