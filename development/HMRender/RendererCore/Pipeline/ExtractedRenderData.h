#pragma once

#include <Core/Graphics/Camera.h>
#include <RendererCore/Debug/DebugRendererContext.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/ViewData.h>

class WD_RENDERERCORE_DLL wdExtractedRenderData
{
public:
  wdExtractedRenderData();

  WD_ALWAYS_INLINE void SetCamera(const wdCamera& camera) { m_Camera = camera; }
  WD_ALWAYS_INLINE const wdCamera& GetCamera() const { return m_Camera; }

  WD_ALWAYS_INLINE void SetLodCamera(const wdCamera& camera) { m_LodCamera = camera; }
  WD_ALWAYS_INLINE const wdCamera& GetLodCamera() const { return m_LodCamera; }

  WD_ALWAYS_INLINE void SetViewData(const wdViewData& viewData) { m_ViewData = viewData; }
  WD_ALWAYS_INLINE const wdViewData& GetViewData() const { return m_ViewData; }

  WD_ALWAYS_INLINE void SetWorldTime(wdTime time) { m_WorldTime = time; }
  WD_ALWAYS_INLINE wdTime GetWorldTime() const { return m_WorldTime; }

  WD_ALWAYS_INLINE void SetWorldDebugContext(const wdDebugRendererContext& debugContext) { m_WorldDebugContext = debugContext; }
  WD_ALWAYS_INLINE const wdDebugRendererContext& GetWorldDebugContext() const { return m_WorldDebugContext; }

  WD_ALWAYS_INLINE void SetViewDebugContext(const wdDebugRendererContext& debugContext) { m_ViewDebugContext = debugContext; }
  WD_ALWAYS_INLINE const wdDebugRendererContext& GetViewDebugContext() const { return m_ViewDebugContext; }

  void AddRenderData(const wdRenderData* pRenderData, wdRenderData::Category category);
  void AddFrameData(const wdRenderData* pFrameData);

  void SortAndBatch();

  void Clear();

  wdRenderDataBatchList GetRenderDataBatchesWithCategory(
    wdRenderData::Category category, wdRenderDataBatch::Filter filter = wdRenderDataBatch::Filter()) const;

  template <typename T>
  WD_ALWAYS_INLINE const T* GetFrameData() const
  {
    return static_cast<const T*>(GetFrameData(wdGetStaticRTTI<T>()));
  }

private:
  const wdRenderData* GetFrameData(const wdRTTI* pRtti) const;

  struct DataPerCategory
  {
    wdDynamicArray<wdRenderDataBatch> m_Batches;
    wdDynamicArray<wdRenderDataBatch::SortableRenderData> m_SortableRenderData;
  };

  wdCamera m_Camera;
  wdCamera m_LodCamera; // Temporary until we have a real LOD system
  wdViewData m_ViewData;
  wdTime m_WorldTime;

  wdDebugRendererContext m_WorldDebugContext;
  wdDebugRendererContext m_ViewDebugContext;

  wdHybridArray<DataPerCategory, 16> m_DataPerCategory;
  wdHybridArray<const wdRenderData*, 16> m_FrameData;
};
