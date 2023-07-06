#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Pipeline/Declarations.h>

using wdRenderPipelineResourceHandle = wdTypedResourceHandle<class wdRenderPipelineResource>;

struct wdRenderWorldExtractionEvent
{
  enum class Type
  {
    BeginExtraction,
    BeforeViewExtraction,
    AfterViewExtraction,
    EndExtraction
  };

  Type m_Type;
  wdView* m_pView = nullptr;
  wdUInt64 m_uiFrameCounter = 0;
};

struct wdRenderWorldRenderEvent
{
  enum class Type
  {
    BeginRender,
    BeforePipelineExecution,
    AfterPipelineExecution,
    EndRender,
  };

  Type m_Type;
  wdRenderPipeline* m_pPipeline = nullptr;
  const wdRenderViewContext* m_pRenderViewContext = nullptr;
  wdUInt64 m_uiFrameCounter = 0;
};

class WD_RENDERERCORE_DLL wdRenderWorld
{
public:
  static wdViewHandle CreateView(const char* szName, wdView*& out_pView);
  static void DeleteView(const wdViewHandle& hView);

  static bool TryGetView(const wdViewHandle& hView, wdView*& out_pView);
  static wdView* GetViewByUsageHint(wdCameraUsageHint::Enum usageHint, wdCameraUsageHint::Enum alternativeUsageHint = wdCameraUsageHint::None, const wdWorld* pWorld = nullptr);

  static void AddMainView(const wdViewHandle& hView);
  static void RemoveMainView(const wdViewHandle& hView);
  static void ClearMainViews();
  static wdArrayPtr<wdViewHandle> GetMainViews();

  static void CacheRenderData(const wdView& view, const wdGameObjectHandle& hOwnerObject, const wdComponentHandle& hOwnerComponent, wdUInt16 uiComponentVersion, wdArrayPtr<wdInternal::RenderDataCacheEntry> cacheEntries);

  static void DeleteAllCachedRenderData();
  static void DeleteCachedRenderData(const wdGameObjectHandle& hOwnerObject, const wdComponentHandle& hOwnerComponent);
  static void DeleteCachedRenderDataForObject(const wdGameObject* pOwnerObject);
  static void DeleteCachedRenderDataForObjectRecursive(const wdGameObject* pOwnerObject);
  static void ResetRenderDataCache(wdView& ref_view);
  static wdArrayPtr<const wdInternal::RenderDataCacheEntry> GetCachedRenderData(const wdView& view, const wdGameObjectHandle& hOwner, wdUInt16 uiComponentVersion);

  static void AddViewToRender(const wdViewHandle& hView);

  static void ExtractMainViews();

  static void Render(wdRenderContext* pRenderContext);

  static void BeginFrame();
  static void EndFrame();

  static wdEvent<wdView*, wdMutex> s_ViewCreatedEvent;
  static wdEvent<wdView*, wdMutex> s_ViewDeletedEvent;

  static const wdEvent<const wdRenderWorldExtractionEvent&, wdMutex>& GetExtractionEvent() { return s_ExtractionEvent; }
  static const wdEvent<const wdRenderWorldRenderEvent&, wdMutex>& GetRenderEvent() { return s_RenderEvent; }

  static bool GetUseMultithreadedRendering();

  /// \brief Resets the frame counter to zero. Only for test purposes !
  WD_ALWAYS_INLINE static void ResetFrameCounter() { s_uiFrameCounter = 0; }

  WD_ALWAYS_INLINE static wdUInt64 GetFrameCounter() { return s_uiFrameCounter; }

  WD_FORCE_INLINE static wdUInt32 GetDataIndexForExtraction() { return GetUseMultithreadedRendering() ? (s_uiFrameCounter & 1) : 0; }

  WD_FORCE_INLINE static wdUInt32 GetDataIndexForRendering() { return GetUseMultithreadedRendering() ? ((s_uiFrameCounter + 1) & 1) : 0; }

  static bool IsRenderingThread();

  /// \name Render To Texture
  /// @{
public:
  struct CameraConfig
  {
    wdRenderPipelineResourceHandle m_hRenderPipeline;
  };

  static void BeginModifyCameraConfigs();
  static void EndModifyCameraConfigs();
  static void ClearCameraConfigs();
  static void SetCameraConfig(const char* szName, const CameraConfig& config);
  static const CameraConfig* FindCameraConfig(const char* szName);

  static wdEvent<void*> s_CameraConfigsModifiedEvent;

private:
  static bool s_bModifyingCameraConfigs;
  static wdMap<wdString, CameraConfig> s_CameraConfigs;

  /// @}

private:
  WD_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, RenderWorld);
  friend class wdView;
  friend class wdRenderPipeline;

  static void DeleteCachedRenderDataInternal(const wdGameObjectHandle& hOwnerObject);
  static void ClearRenderDataCache();
  static void UpdateRenderDataCache();

  static void AddRenderPipelineToRebuild(wdRenderPipeline* pRenderPipeline, const wdViewHandle& hView);
  static void RebuildPipelines();

  static void OnEngineStartup();
  static void OnEngineShutdown();

  static wdEvent<const wdRenderWorldExtractionEvent&, wdMutex> s_ExtractionEvent;
  static wdEvent<const wdRenderWorldRenderEvent&, wdMutex> s_RenderEvent;
  static wdUInt64 s_uiFrameCounter;
};
