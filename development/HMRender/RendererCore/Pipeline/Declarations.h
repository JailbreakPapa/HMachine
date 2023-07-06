#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Reflection/Reflection.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class wdCamera;
class wdExtractedRenderData;
class wdExtractor;
class wdView;
class wdRenderer;
class wdRenderData;
class wdRenderDataBatch;
class wdRenderPipeline;
class wdRenderPipelinePass;
class wdRenderContext;
class wdDebugRendererContext;

struct wdRenderPipelineNodePin;
struct wdRenderPipelinePassConnection;
struct wdViewData;

namespace wdInternal
{
  struct RenderDataCache;

  struct RenderDataCacheEntry
  {
    WD_DECLARE_POD_TYPE();

    const wdRenderData* m_pRenderData = nullptr;
    wdUInt16 m_uiCategory = 0;
    wdUInt16 m_uiComponentIndex = 0;
    wdUInt16 m_uiPartIndex = 0;

    WD_ALWAYS_INLINE bool operator==(const RenderDataCacheEntry& other) const { return m_pRenderData == other.m_pRenderData && m_uiCategory == other.m_uiCategory && m_uiComponentIndex == other.m_uiComponentIndex && m_uiPartIndex == other.m_uiPartIndex; }

    // Cache entries need to be sorted by component index and then by part index
    WD_ALWAYS_INLINE bool operator<(const RenderDataCacheEntry& other) const
    {
      if (m_uiComponentIndex == other.m_uiComponentIndex)
        return m_uiPartIndex < other.m_uiPartIndex;

      return m_uiComponentIndex < other.m_uiComponentIndex;
    }
  };
} // namespace wdInternal

struct wdRenderViewContext
{
  const wdCamera* m_pCamera;
  const wdCamera* m_pLodCamera;
  const wdViewData* m_pViewData;
  wdRenderContext* m_pRenderContext;

  const wdDebugRendererContext* m_pWorldDebugContext;
  const wdDebugRendererContext* m_pViewDebugContext;
};

using wdViewId = wdGenericId<24, 8>;

class wdViewHandle
{
  WD_DECLARE_HANDLE_TYPE(wdViewHandle, wdViewId);

  friend class wdRenderWorld;
};

/// \brief HashHelper implementation so view handles can be used as key in a hashtable.
template <>
struct wdHashHelper<wdViewHandle>
{
  WD_ALWAYS_INLINE static wdUInt32 Hash(wdViewHandle value) { return value.GetInternalID().m_Data * 2654435761U; }

  WD_ALWAYS_INLINE static bool Equal(wdViewHandle a, wdViewHandle b) { return a == b; }
};

/// \brief Usage hint of a camera/view.
struct WD_RENDERERCORE_DLL wdCameraUsageHint
{
  using StorageType = wdUInt8;

  enum Enum
  {
    None,
    MainView,
    EditorView,
    RenderTarget,
    Culling,
    Shadow,
    Reflection,
    Thumbnail,

    ENUM_COUNT,

    Default = None,
  };
};

WD_DECLARE_REFLECTABLE_TYPE(WD_RENDERERCORE_DLL, wdCameraUsageHint);
