#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Pipeline/Renderer.h>

struct SpriteData;
class wdRenderDataBatch;
using wdShaderResourceHandle = wdTypedResourceHandle<class wdShaderResource>;

/// \brief Implements rendering of sprites
class WD_RENDERERCORE_DLL wdSpriteRenderer : public wdRenderer
{
  WD_ADD_DYNAMIC_REFLECTION(wdSpriteRenderer, wdRenderer);
  WD_DISALLOW_COPY_AND_ASSIGN(wdSpriteRenderer);

public:
  wdSpriteRenderer();
  ~wdSpriteRenderer();

  // wdRenderer implementation
  virtual void GetSupportedRenderDataTypes(wdHybridArray<const wdRTTI*, 8>& ref_types) const override;
  virtual void GetSupportedRenderDataCategories(wdHybridArray<wdRenderData::Category, 8>& ref_categories) const override;
  virtual void RenderBatch(
    const wdRenderViewContext& renderContext, const wdRenderPipelinePass* pPass, const wdRenderDataBatch& batch) const override;

protected:
  wdGALBufferHandle CreateSpriteDataBuffer() const;
  void DeleteSpriteDataBuffer(wdGALBufferHandle hBuffer) const;
  virtual void FillSpriteData(const wdRenderDataBatch& batch, wdUInt32 uiStartIndex, wdUInt32 uiCount) const;

  wdShaderResourceHandle m_hShader;
  mutable wdDynamicArray<SpriteData, wdAlignedAllocatorWrapper> m_SpriteData;
};
