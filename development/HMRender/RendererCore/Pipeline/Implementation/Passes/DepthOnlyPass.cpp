#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/DepthOnlyPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdDepthOnlyPass, 1, wdRTTIDefaultAllocator<wdDepthOnlyPass>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdDepthOnlyPass::wdDepthOnlyPass(const char* szName)
  : wdRenderPipelinePass(szName, true)
{
}

wdDepthOnlyPass::~wdDepthOnlyPass() = default;

bool wdDepthOnlyPass::GetRenderTargetDescriptions(
  const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs)
{
  // DepthStencil
  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    outputs[m_PinDepthStencil.m_uiOutputIndex] = *inputs[m_PinDepthStencil.m_uiInputIndex];
  }
  else
  {
    wdLog::Error("No depth stencil input connected to pass '{0}'!", GetName());
    return false;
  }

  return true;
}

void wdDepthOnlyPass::Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs,
  const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs)
{
  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  // Setup render target
  wdGALRenderingSetup renderingSetup;
  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(inputs[m_PinDepthStencil.m_uiInputIndex]->m_TextureHandle));
  }

  auto pCommandEncoder = wdRenderContext::BeginPassAndRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_DEPTH_ONLY");
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SHADING_QUALITY", "SHADING_QUALITY_NORMAL");

  // Render
  RenderDataWithCategory(renderViewContext, wdDefaultRenderDataCategories::LitOpaque);
  RenderDataWithCategory(renderViewContext, wdDefaultRenderDataCategories::LitMasked);
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_DepthOnlyPass);
