#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Meshes/SkinnedMeshComponent.h>
#include <RendererCore/Meshes/SkinnedMeshRenderer.h>
#include <RendererCore/RenderContext/RenderContext.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdSkinnedMeshRenderer, 1, wdRTTIDefaultAllocator<wdSkinnedMeshRenderer>)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdUInt32 wdSkinnedMeshRenderer::s_uiSkinningBufferUpdates = 0;

wdSkinnedMeshRenderer::wdSkinnedMeshRenderer() = default;
wdSkinnedMeshRenderer::~wdSkinnedMeshRenderer() = default;

void wdSkinnedMeshRenderer::GetSupportedRenderDataTypes(wdHybridArray<const wdRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(wdGetStaticRTTI<wdSkinnedMeshRenderData>());
}

void wdSkinnedMeshRenderer::SetAdditionalData(const wdRenderViewContext& renderViewContext, const wdMeshRenderData* pRenderData) const
{
  // Don't call base class implementation here since the state will be overwritten in this method anyways.

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();
  wdRenderContext* pContext = renderViewContext.m_pRenderContext;

  auto pSkinnedRenderData = static_cast<const wdSkinnedMeshRenderData*>(pRenderData);

  if (pSkinnedRenderData->m_hSkinningTransforms.IsInvalidated())
  {
    pContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");
  }
  else
  {
    pContext->SetShaderPermutationVariable("VERTEX_SKINNING", "TRUE");

    if (pSkinnedRenderData->m_bTransformsUpdated != nullptr && *pSkinnedRenderData->m_bTransformsUpdated == false)
    {
      // if this is the first renderer that is supposed to actually render the skinned mesh, upload the skinning matrices
      *pSkinnedRenderData->m_bTransformsUpdated = true;
      pContext->GetCommandEncoder()->UpdateBuffer(pSkinnedRenderData->m_hSkinningTransforms, 0, pSkinnedRenderData->m_pNewSkinningTransformData);

      // TODO: could expose this somewhere (wdStats?)
      s_uiSkinningBufferUpdates++;
    }

    pContext->BindBuffer("skinningTransforms", pDevice->GetDefaultResourceView(pSkinnedRenderData->m_hSkinningTransforms));
  }
}


WD_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_SkinnedMeshRenderer);
