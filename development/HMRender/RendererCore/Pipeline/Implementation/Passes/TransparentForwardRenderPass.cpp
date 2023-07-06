#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/TransparentForwardRenderPass.h>
#include <RendererCore/RenderContext/RenderContext.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdTransparentForwardRenderPass, 1, wdRTTIDefaultAllocator<wdTransparentForwardRenderPass>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("ResolvedDepth", m_PinResolvedDepth),
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdTransparentForwardRenderPass::wdTransparentForwardRenderPass(const char* szName)
  : wdForwardRenderPass(szName)
{
}

wdTransparentForwardRenderPass::~wdTransparentForwardRenderPass()
{
  if (!m_hSceneColorSamplerState.IsInvalidated())
  {
    wdGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSceneColorSamplerState);
    m_hSceneColorSamplerState.Invalidate();
  }
}

void wdTransparentForwardRenderPass::Execute(const wdRenderViewContext& renderViewContext,
  const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs)
{
  auto pColorInput = inputs[m_PinColor.m_uiInputIndex];
  if (pColorInput == nullptr)
  {
    return;
  }

  CreateSamplerState();

  wdUInt32 uiWidth = pColorInput->m_Desc.m_uiWidth;
  wdUInt32 uiHeight = pColorInput->m_Desc.m_uiHeight;

  wdGALTextureCreationDescription desc;
  desc.SetAsRenderTarget(uiWidth, uiHeight, pColorInput->m_Desc.m_Format);
  desc.m_uiArraySize = pColorInput->m_Desc.m_uiArraySize;
  desc.m_uiMipLevelCount = 1;

  wdGALTextureHandle hSceneColor = wdGPUResourcePool::GetDefaultInstance()->GetRenderTarget(desc);

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();
  wdGALPass* pGALPass = pDevice->BeginPass(GetName());

  SetupResources(pGALPass, renderViewContext, inputs, outputs);
  SetupPermutationVars(renderViewContext);
  SetupLighting(renderViewContext);

  UpdateSceneColorTexture(renderViewContext, hSceneColor, pColorInput->m_TextureHandle);

  wdGALResourceViewHandle colorResourceViewHandle = pDevice->GetDefaultResourceView(hSceneColor);
  renderViewContext.m_pRenderContext->BindTexture2D("SceneColor", colorResourceViewHandle);
  renderViewContext.m_pRenderContext->BindSamplerState("SceneColorSampler", m_hSceneColorSamplerState);

  RenderObjects(renderViewContext);

  renderViewContext.m_pRenderContext->EndRendering();
  pDevice->EndPass(pGALPass);

  wdGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hSceneColor);
}

void wdTransparentForwardRenderPass::SetupResources(wdGALPass* pGALPass, const wdRenderViewContext& renderViewContext,
  const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs)
{
  SUPER::SetupResources(pGALPass, renderViewContext, inputs, outputs);

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  if (inputs[m_PinResolvedDepth.m_uiInputIndex])
  {
    wdGALResourceViewHandle depthResourceViewHandle = pDevice->GetDefaultResourceView(inputs[m_PinResolvedDepth.m_uiInputIndex]->m_TextureHandle);
    renderViewContext.m_pRenderContext->BindTexture2D("SceneDepth", depthResourceViewHandle);
  }
}

void wdTransparentForwardRenderPass::RenderObjects(const wdRenderViewContext& renderViewContext)
{
  RenderDataWithCategory(renderViewContext, wdDefaultRenderDataCategories::LitTransparent);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
  RenderDataWithCategory(renderViewContext, wdDefaultRenderDataCategories::LitForeground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
  RenderDataWithCategory(renderViewContext, wdDefaultRenderDataCategories::LitForeground);
}

void wdTransparentForwardRenderPass::UpdateSceneColorTexture(
  const wdRenderViewContext& renderViewContext, wdGALTextureHandle hSceneColorTexture, wdGALTextureHandle hCurrentColorTexture)
{
  wdGALTextureSubresource subresource;
  subresource.m_uiMipLevel = 0;
  subresource.m_uiArraySlice = 0;

  renderViewContext.m_pRenderContext->GetCommandEncoder()->ResolveTexture(hSceneColorTexture, subresource, hCurrentColorTexture, subresource);
}

void wdTransparentForwardRenderPass::CreateSamplerState()
{
  if (m_hSceneColorSamplerState.IsInvalidated())
  {
    wdGALSamplerStateCreationDescription desc;
    desc.m_MinFilter = wdGALTextureFilterMode::Linear;
    desc.m_MagFilter = wdGALTextureFilterMode::Linear;
    desc.m_MipFilter = wdGALTextureFilterMode::Linear;
    desc.m_AddressU = wdImageAddressMode::Clamp;
    desc.m_AddressV = wdImageAddressMode::Mirror;
    desc.m_AddressW = wdImageAddressMode::Mirror;

    m_hSceneColorSamplerState = wdGALDevice::GetDefaultDevice()->CreateSamplerState(desc);
  }
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_TransparentForwardRenderPass);
