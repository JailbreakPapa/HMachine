#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/OpaqueForwardRenderPass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdOpaqueForwardRenderPass, 1, wdRTTIDefaultAllocator<wdOpaqueForwardRenderPass>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("SSAO", m_PinSSAO),
    WD_MEMBER_PROPERTY("WriteDepth", m_bWriteDepth)->AddAttributes(new wdDefaultValueAttribute(true)),
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdOpaqueForwardRenderPass::wdOpaqueForwardRenderPass(const char* szName)
  : wdForwardRenderPass(szName)

{
  m_hWhiteTexture = wdResourceManager::LoadResource<wdTexture2DResource>("White.color");
}

wdOpaqueForwardRenderPass::~wdOpaqueForwardRenderPass() = default;

bool wdOpaqueForwardRenderPass::GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs)
{
  if (!SUPER::GetRenderTargetDescriptions(view, inputs, outputs))
  {
    return false;
  }

  if (inputs[m_PinSSAO.m_uiInputIndex])
  {
    if (inputs[m_PinSSAO.m_uiInputIndex]->m_uiWidth != inputs[m_PinColor.m_uiInputIndex]->m_uiWidth ||
        inputs[m_PinSSAO.m_uiInputIndex]->m_uiHeight != inputs[m_PinColor.m_uiInputIndex]->m_uiHeight)
    {
      wdLog::Warning("Expected same resolution for SSAO and color input to pass '{0}'!", GetName());
    }

    if (m_ShadingQuality == wdForwardRenderShadingQuality::Simplified)
    {
      wdLog::Warning("SSAO input will be ignored for pass '{0}' since simplified shading is activated.", GetName());
    }
  }

  return true;
}

void wdOpaqueForwardRenderPass::SetupResources(wdGALPass* pGALPass, const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs)
{
  SUPER::SetupResources(pGALPass, renderViewContext, inputs, outputs);

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  // SSAO texture
  if (m_ShadingQuality == wdForwardRenderShadingQuality::Normal)
  {
    if (inputs[m_PinSSAO.m_uiInputIndex])
    {
      wdGALResourceViewHandle ssaoResourceViewHandle = pDevice->GetDefaultResourceView(inputs[m_PinSSAO.m_uiInputIndex]->m_TextureHandle);
      renderViewContext.m_pRenderContext->BindTexture2D("SSAOTexture", ssaoResourceViewHandle);
    }
    else
    {
      renderViewContext.m_pRenderContext->BindTexture2D("SSAOTexture", m_hWhiteTexture, wdResourceAcquireMode::BlockTillLoaded);
    }
  }
}

void wdOpaqueForwardRenderPass::SetupPermutationVars(const wdRenderViewContext& renderViewContext)
{
  SUPER::SetupPermutationVars(renderViewContext);

  if (m_bWriteDepth)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("FORWARD_PASS_WRITE_DEPTH", "TRUE");
  }
  else
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("FORWARD_PASS_WRITE_DEPTH", "FALSE");
  }
}

void wdOpaqueForwardRenderPass::RenderObjects(const wdRenderViewContext& renderViewContext)
{
  RenderDataWithCategory(renderViewContext, wdDefaultRenderDataCategories::LitOpaque);
  RenderDataWithCategory(renderViewContext, wdDefaultRenderDataCategories::LitMasked);
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_OpaqueForwardRenderPass);
