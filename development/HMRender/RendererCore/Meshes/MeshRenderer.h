#pragma once

#include <RendererCore/Pipeline/Renderer.h>

class wdMeshRenderData;
struct wdPerInstanceData;

/// \brief Implements rendering of static meshes
class WD_RENDERERCORE_DLL wdMeshRenderer : public wdRenderer
{
  WD_ADD_DYNAMIC_REFLECTION(wdMeshRenderer, wdRenderer);
  WD_DISALLOW_COPY_AND_ASSIGN(wdMeshRenderer);

public:
  wdMeshRenderer();
  ~wdMeshRenderer();

  // wdRenderer implementation
  virtual void GetSupportedRenderDataTypes(wdHybridArray<const wdRTTI*, 8>& ref_types) const override;
  virtual void GetSupportedRenderDataCategories(wdHybridArray<wdRenderData::Category, 8>& ref_categories) const override;
  virtual void RenderBatch(
    const wdRenderViewContext& renderContext, const wdRenderPipelinePass* pPass, const wdRenderDataBatch& batch) const override;

protected:
  virtual void SetAdditionalData(const wdRenderViewContext& renderViewContext, const wdMeshRenderData* pRenderData) const;
  virtual void FillPerInstanceData(
    wdArrayPtr<wdPerInstanceData> instanceData, const wdRenderDataBatch& batch, wdUInt32 uiStartIndex, wdUInt32& out_uiFilteredCount) const;
};
