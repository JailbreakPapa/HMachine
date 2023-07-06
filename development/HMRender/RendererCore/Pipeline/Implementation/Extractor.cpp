#include <RendererCore/RendererCorePCH.h>

#include <Core/World/SpatialSystem_RegularGrid.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
wdCVarBool cvar_SpatialVisBounds("Spatial.VisBounds", false, wdCVarFlags::Default, "Enables debug visualization of object bounds");
wdCVarBool cvar_SpatialVisLocalBBox("Spatial.VisLocalBBox", false, wdCVarFlags::Default, "Enables debug visualization of object local bounding box");
wdCVarBool cvar_SpatialVisData("Spatial.VisData", false, wdCVarFlags::Default, "Enables debug visualization of the spatial data structure");
wdCVarString cvar_SpatialVisDataOnlyCategory("Spatial.VisData.OnlyCategory", "", wdCVarFlags::Default, "When set the debug visualization is only shown for the given spatial data category");
wdCVarBool cvar_SpatialVisDataOnlySelected("Spatial.VisData.OnlySelected", false, wdCVarFlags::Default, "When set the debug visualization is only shown for selected objects");
wdCVarString cvar_SpatialVisDataOnlyObject("Spatial.VisData.OnlyObject", "", wdCVarFlags::Default, "When set the debug visualization is only shown for objects with the given name");

wdCVarBool cvar_SpatialExtractionShowStats("Spatial.Extraction.ShowStats", false, wdCVarFlags::Default, "Display some stats of the render data extraction");
#endif

namespace
{
#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  void VisualizeSpatialData(const wdView& view)
  {
    if (cvar_SpatialVisData && cvar_SpatialVisDataOnlyObject.GetValue().IsEmpty() && !cvar_SpatialVisDataOnlySelected)
    {
      const wdSpatialSystem& spatialSystem = *view.GetWorld()->GetSpatialSystem();
      if (auto pSpatialSystemGrid = wdDynamicCast<const wdSpatialSystem_RegularGrid*>(&spatialSystem))
      {
        wdSpatialData::Category filterCategory = wdSpatialData::FindCategory(cvar_SpatialVisDataOnlyCategory.GetValue());

        wdHybridArray<wdBoundingBox, 16> boxes;
        pSpatialSystemGrid->GetAllCellBoxes(boxes, filterCategory);

        for (auto& box : boxes)
        {
          wdDebugRenderer::DrawLineBox(view.GetHandle(), box, wdColor::Cyan);
        }
      }
    }
  }

  void VisualizeObject(const wdView& view, const wdGameObject* pObject)
  {
    if (!cvar_SpatialVisBounds && !cvar_SpatialVisLocalBBox && !cvar_SpatialVisData)
      return;

    if (cvar_SpatialVisLocalBBox)
    {
      const wdBoundingBoxSphere& localBounds = pObject->GetLocalBounds();
      if (localBounds.IsValid())
      {
        wdDebugRenderer::DrawLineBox(view.GetHandle(), localBounds.GetBox(), wdColor::Yellow, pObject->GetGlobalTransform());
      }
    }

    if (cvar_SpatialVisBounds)
    {
      const wdBoundingBoxSphere& globalBounds = pObject->GetGlobalBounds();
      if (globalBounds.IsValid())
      {
        wdDebugRenderer::DrawLineBox(view.GetHandle(), globalBounds.GetBox(), wdColor::Lime);
        wdDebugRenderer::DrawLineSphere(view.GetHandle(), globalBounds.GetSphere(), wdColor::Magenta);
      }
    }

    if (cvar_SpatialVisData && cvar_SpatialVisDataOnlyCategory.GetValue().IsEmpty())
    {
      const wdSpatialSystem& spatialSystem = *view.GetWorld()->GetSpatialSystem();
      if (auto pSpatialSystemGrid = wdDynamicCast<const wdSpatialSystem_RegularGrid*>(&spatialSystem))
      {
        wdBoundingBox box;
        if (pSpatialSystemGrid->GetCellBoxForSpatialData(pObject->GetSpatialData(), box).Succeeded())
        {
          wdDebugRenderer::DrawLineBox(view.GetHandle(), box, wdColor::Cyan);
        }
      }
    }
  }
#endif
} // namespace

//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdExtractor, 1, wdRTTINoAllocator)
  {
    WD_BEGIN_PROPERTIES
    {
      WD_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new wdDefaultValueAttribute(true)),
      WD_ACCESSOR_PROPERTY("Name", GetName, SetName),
    }
    WD_END_PROPERTIES;
    WD_BEGIN_ATTRIBUTES
    {
      new wdColorAttribute(wdColorScheme::DarkUI(wdColorScheme::Red)),
    }
    WD_END_ATTRIBUTES;
  }
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format om

wdExtractor::wdExtractor(const char* szName)
{
  m_bActive = true;
  m_sName.Assign(szName);

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  m_uiNumCachedRenderData = 0;
  m_uiNumUncachedRenderData = 0;
#endif
}

wdExtractor::~wdExtractor() = default;

void wdExtractor::SetName(const char* szName)
{
  if (!wdStringUtils::IsNullOrEmpty(szName))
  {
    m_sName.Assign(szName);
  }
}

const char* wdExtractor::GetName() const
{
  return m_sName.GetData();
}

bool wdExtractor::FilterByViewTags(const wdView& view, const wdGameObject* pObject) const
{
  if (!view.m_ExcludeTags.IsEmpty() && view.m_ExcludeTags.IsAnySet(pObject->GetTags()))
    return true;

  if (!view.m_IncludeTags.IsEmpty() && !view.m_IncludeTags.IsAnySet(pObject->GetTags()))
    return true;

  return false;
}

void wdExtractor::ExtractRenderData(const wdView& view, const wdGameObject* pObject, wdMsgExtractRenderData& msg, wdExtractedRenderData& extractedRenderData) const
{
  auto AddRenderDataFromMessage = [&](const wdMsgExtractRenderData& msg) {
    if (msg.m_OverrideCategory != wdInvalidRenderDataCategory)
    {
      for (auto& data : msg.m_ExtractedRenderData)
      {
        extractedRenderData.AddRenderData(data.m_pRenderData, msg.m_OverrideCategory);
      }
    }
    else
    {
      for (auto& data : msg.m_ExtractedRenderData)
      {
        extractedRenderData.AddRenderData(data.m_pRenderData, wdRenderData::Category(data.m_uiCategory));
      }
    }

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
    m_uiNumUncachedRenderData += msg.m_ExtractedRenderData.GetCount();
#endif
  };

  if (pObject->IsStatic())
  {
    wdUInt16 uiComponentVersion = pObject->GetComponentVersion();

    auto cachedRenderData = wdRenderWorld::GetCachedRenderData(view, pObject->GetHandle(), uiComponentVersion);

#if WD_ENABLED(WD_COMPILE_FOR_DEBUG)
    for (wdUInt32 i = 1; i < cachedRenderData.GetCount(); ++i)
    {
      WD_ASSERT_DEBUG(cachedRenderData[i - 1].m_uiComponentIndex <= cachedRenderData[i].m_uiComponentIndex, "Cached render data needs to be sorted");
      if (cachedRenderData[i - 1].m_uiComponentIndex == cachedRenderData[i].m_uiComponentIndex)
      {
        WD_ASSERT_DEBUG(cachedRenderData[i - 1].m_uiPartIndex < cachedRenderData[i].m_uiPartIndex, "Cached render data needs to be sorted");
      }
    }
#endif

    wdUInt32 uiCacheIndex = 0;

    auto components = pObject->GetComponents();
    const wdUInt32 uiNumComponents = components.GetCount();
    for (wdUInt32 uiComponentIndex = 0; uiComponentIndex < uiNumComponents; ++uiComponentIndex)
    {
      bool bCacheFound = false;
      while (uiCacheIndex < cachedRenderData.GetCount() && cachedRenderData[uiCacheIndex].m_uiComponentIndex == uiComponentIndex)
      {
        const wdInternal::RenderDataCacheEntry& cacheEntry = cachedRenderData[uiCacheIndex];
        if (cacheEntry.m_pRenderData != nullptr)
        {
          extractedRenderData.AddRenderData(cacheEntry.m_pRenderData, msg.m_OverrideCategory != wdInvalidRenderDataCategory ? msg.m_OverrideCategory : wdRenderData::Category(cacheEntry.m_uiCategory));

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
          ++m_uiNumCachedRenderData;
#endif
        }
        ++uiCacheIndex;

        bCacheFound = true;
      }

      if (bCacheFound)
      {
        continue;
      }

      const wdComponent* pComponent = components[uiComponentIndex];

      msg.m_ExtractedRenderData.Clear();
      msg.m_uiNumCacheIfStatic = 0;

      if (pComponent->SendMessage(msg))
      {
        // Only cache render data if all parts should be cached otherwise the cache is incomplete and we won't call SendMessage again
        if (msg.m_uiNumCacheIfStatic > 0 && msg.m_ExtractedRenderData.GetCount() == msg.m_uiNumCacheIfStatic)
        {
          wdHybridArray<wdInternal::RenderDataCacheEntry, 16> newCacheEntries(wdFrameAllocator::GetCurrentAllocator());

          for (wdUInt32 uiPartIndex = 0; uiPartIndex < msg.m_ExtractedRenderData.GetCount(); ++uiPartIndex)
          {
            auto& newCacheEntry = newCacheEntries.ExpandAndGetRef();
            newCacheEntry.m_pRenderData = msg.m_ExtractedRenderData[uiPartIndex].m_pRenderData;
            newCacheEntry.m_uiCategory = msg.m_ExtractedRenderData[uiPartIndex].m_uiCategory;
            newCacheEntry.m_uiComponentIndex = static_cast<wdUInt16>(uiComponentIndex);
            newCacheEntry.m_uiPartIndex = static_cast<wdUInt16>(uiPartIndex);
          }

          wdRenderWorld::CacheRenderData(view, pObject->GetHandle(), pComponent->GetHandle(), uiComponentVersion, newCacheEntries);
        }

        AddRenderDataFromMessage(msg);
      }
      else if (pComponent->IsActiveAndInitialized()) // component does not handle extract message at all
      {
        WD_ASSERT_DEV(pComponent->GetDynamicRTTI()->CanHandleMessage<wdMsgExtractRenderData>() == false, "");

        // Create a dummy cache entry so we don't call send message next time
        wdInternal::RenderDataCacheEntry dummyEntry;
        dummyEntry.m_pRenderData = nullptr;
        dummyEntry.m_uiCategory = wdInvalidRenderDataCategory.m_uiValue;
        dummyEntry.m_uiComponentIndex = static_cast<wdUInt16>(uiComponentIndex);

        wdRenderWorld::CacheRenderData(view, pObject->GetHandle(), pComponent->GetHandle(), uiComponentVersion, wdMakeArrayPtr(&dummyEntry, 1));
      }
    }
  }
  else
  {
    msg.m_ExtractedRenderData.Clear();
    pObject->SendMessage(msg);

    AddRenderDataFromMessage(msg);
  }
}

void wdExtractor::Extract(const wdView& view, const wdDynamicArray<const wdGameObject*>& visibleObjects, wdExtractedRenderData& ref_extractedRenderData)
{
}

void wdExtractor::PostSortAndBatch(
  const wdView& view, const wdDynamicArray<const wdGameObject*>& visibleObjects, wdExtractedRenderData& ref_extractedRenderData)
{
}

//////////////////////////////////////////////////////////////////////////

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdVisibleObjectsExtractor, 1, wdRTTIDefaultAllocator<wdVisibleObjectsExtractor>)
WD_END_DYNAMIC_REFLECTED_TYPE;

wdVisibleObjectsExtractor::wdVisibleObjectsExtractor(const char* szName)
  : wdExtractor(szName)
{
}

wdVisibleObjectsExtractor::~wdVisibleObjectsExtractor() = default;

void wdVisibleObjectsExtractor::Extract(
  const wdView& view, const wdDynamicArray<const wdGameObject*>& visibleObjects, wdExtractedRenderData& ref_extractedRenderData)
{
  wdMsgExtractRenderData msg;
  msg.m_pView = &view;

  WD_LOCK(view.GetWorld()->GetReadMarker());

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  VisualizeSpatialData(view);

  m_uiNumCachedRenderData = 0;
  m_uiNumUncachedRenderData = 0;
#endif

  for (auto pObject : visibleObjects)
  {
    ExtractRenderData(view, pObject, msg, ref_extractedRenderData);

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
    if (cvar_SpatialVisBounds || cvar_SpatialVisLocalBBox || cvar_SpatialVisData)
    {
      if ((cvar_SpatialVisDataOnlyObject.GetValue().IsEmpty() ||
            pObject->GetName().FindSubString_NoCase(cvar_SpatialVisDataOnlyObject.GetValue()) != nullptr) &&
          !cvar_SpatialVisDataOnlySelected)
      {
        VisualizeObject(view, pObject);
      }
    }
#endif
  }

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  const bool bIsMainView = (view.GetCameraUsageHint() == wdCameraUsageHint::MainView || view.GetCameraUsageHint() == wdCameraUsageHint::EditorView);

  if (cvar_SpatialExtractionShowStats && bIsMainView)
  {
    wdViewHandle hView = view.GetHandle();

    wdStringBuilder sb;

    wdDebugRenderer::DrawInfoText(hView, wdDebugRenderer::ScreenPlacement::TopLeft, "ExtractionStats", "Extraction Stats:");

    sb.Format("Num Cached Render Data: {0}", m_uiNumCachedRenderData);
    wdDebugRenderer::DrawInfoText(hView, wdDebugRenderer::ScreenPlacement::TopLeft, "ExtractionStats", sb);

    sb.Format("Num Uncached Render Data: {0}", m_uiNumUncachedRenderData);
    wdDebugRenderer::DrawInfoText(hView, wdDebugRenderer::ScreenPlacement::TopLeft, "ExtractionStats", sb);
  }
#endif
}

//////////////////////////////////////////////////////////////////////////

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdSelectedObjectsExtractorBase, 1, wdRTTINoAllocator)
WD_END_DYNAMIC_REFLECTED_TYPE;

wdSelectedObjectsExtractorBase::wdSelectedObjectsExtractorBase(const char* szName)
  : wdExtractor(szName)
  , m_OverrideCategory(wdDefaultRenderDataCategories::Selection)
{
}

wdSelectedObjectsExtractorBase::~wdSelectedObjectsExtractorBase() = default;

void wdSelectedObjectsExtractorBase::Extract(
  const wdView& view, const wdDynamicArray<const wdGameObject*>& visibleObjects, wdExtractedRenderData& ref_extractedRenderData)
{
  const wdDeque<wdGameObjectHandle>* pSelection = GetSelection();
  if (pSelection == nullptr)
    return;

  wdMsgExtractRenderData msg;
  msg.m_pView = &view;
  msg.m_OverrideCategory = m_OverrideCategory;

  WD_LOCK(view.GetWorld()->GetReadMarker());

  for (const auto& hObj : *pSelection)
  {
    const wdGameObject* pObject = nullptr;
    if (!view.GetWorld()->TryGetObject(hObj, pObject))
      continue;

    ExtractRenderData(view, pObject, msg, ref_extractedRenderData);

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
    if (cvar_SpatialVisBounds || cvar_SpatialVisLocalBBox || cvar_SpatialVisData)
    {
      if (cvar_SpatialVisDataOnlySelected)
      {
        VisualizeObject(view, pObject);
      }
    }
#endif
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdSelectedObjectsContext, 1, wdRTTINoAllocator)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdSelectedObjectsExtractor, 1, wdRTTIDefaultAllocator<wdSelectedObjectsExtractor>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("SelectionContext", GetSelectionContext, SetSelectionContext),
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdSelectedObjectsContext::wdSelectedObjectsContext() = default;
wdSelectedObjectsContext::~wdSelectedObjectsContext() = default;

void wdSelectedObjectsContext::RemoveDeadObjects(const wdWorld& world)
{
  for (wdUInt32 i = 0; i < m_Objects.GetCount();)
  {
    const wdGameObject* pObj;
    if (world.TryGetObject(m_Objects[i], pObj) == false)
    {
      m_Objects.RemoveAtAndSwap(i);
    }
    else
      ++i;
  }
}

void wdSelectedObjectsContext::AddObjectAndChildren(const wdWorld& world, const wdGameObjectHandle& hObject)
{
  const wdGameObject* pObj;
  if (world.TryGetObject(hObject, pObj))
  {
    m_Objects.PushBack(hObject);

    for (auto it = pObj->GetChildren(); it.IsValid(); ++it)
    {
      AddObjectAndChildren(world, it);
    }
  }
}

void wdSelectedObjectsContext::AddObjectAndChildren(const wdWorld& world, const wdGameObject* pObject)
{
  m_Objects.PushBack(pObject->GetHandle());

  for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
  {
    AddObjectAndChildren(world, it);
  }
}

wdSelectedObjectsExtractor::wdSelectedObjectsExtractor(const char* szName /*= "ExplicitlySelectedObjectsExtractor"*/)
  : wdSelectedObjectsExtractorBase(szName)
{
}

wdSelectedObjectsExtractor::~wdSelectedObjectsExtractor() = default;

const wdDeque<wdGameObjectHandle>* wdSelectedObjectsExtractor::GetSelection()
{
  if (m_pSelectionContext)
    return &m_pSelectionContext->m_Objects;

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Extractor);
