#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/SimpleRenderPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdSimpleRenderPass, 1, wdRTTIDefaultAllocator<wdSimpleRenderPass>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("Color", m_PinColor),
    WD_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    WD_MEMBER_PROPERTY("Message", m_sMessage),
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdSimpleRenderPass::wdSimpleRenderPass(const char* szName)
  : wdRenderPipelinePass(szName, true)
{
}

wdSimpleRenderPass::~wdSimpleRenderPass() = default;

bool wdSimpleRenderPass::GetRenderTargetDescriptions(
  const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs)
{
  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();
  const wdGALRenderTargets& renderTargets = view.GetActiveRenderTargets();

  // Color
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    outputs[m_PinColor.m_uiOutputIndex] = *inputs[m_PinColor.m_uiInputIndex];
  }
  else
  {
    // If no input is available, we use the render target setup instead.
    const wdGALTexture* pTexture = pDevice->GetTexture(renderTargets.m_hRTs[0]);
    if (pTexture)
    {
      outputs[m_PinColor.m_uiOutputIndex] = pTexture->GetDescription();
      outputs[m_PinColor.m_uiOutputIndex].m_bCreateRenderTarget = true;
      outputs[m_PinColor.m_uiOutputIndex].m_bAllowShaderResourceView = true;
      outputs[m_PinColor.m_uiOutputIndex].m_ResourceAccess.m_bReadBack = false;
      outputs[m_PinColor.m_uiOutputIndex].m_ResourceAccess.m_bImmutable = true;
      outputs[m_PinColor.m_uiOutputIndex].m_pExisitingNativeObject = nullptr;
    }
  }

  // DepthStencil
  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    outputs[m_PinDepthStencil.m_uiOutputIndex] = *inputs[m_PinDepthStencil.m_uiInputIndex];
  }
  else
  {
    // If no input is available, we use the render target setup instead.
    const wdGALTexture* pTexture = pDevice->GetTexture(renderTargets.m_hDSTarget);
    if (pTexture)
    {
      outputs[m_PinDepthStencil.m_uiOutputIndex] = pTexture->GetDescription();
    }
  }

  return true;
}

void wdSimpleRenderPass::Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs,
  const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs)
{
  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  // Setup render target
  wdGALRenderingSetup renderingSetup;
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(inputs[m_PinColor.m_uiInputIndex]->m_TextureHandle));
  }

  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(inputs[m_PinDepthStencil.m_uiInputIndex]->m_TextureHandle));
  }

  auto pCommandEncoder = wdRenderContext::BeginPassAndRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

  // Setup Permutation Vars
  wdTempHashedString sRenderPass("RENDER_PASS_FORWARD");
  if (renderViewContext.m_pViewData->m_ViewRenderMode != wdViewRenderMode::None)
  {
    sRenderPass = wdViewRenderMode::GetPermutationValue(renderViewContext.m_pViewData->m_ViewRenderMode);
  }

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", sRenderPass);

  // Execute render functions
  RenderDataWithCategory(renderViewContext, wdDefaultRenderDataCategories::SimpleOpaque);
  RenderDataWithCategory(renderViewContext, wdDefaultRenderDataCategories::SimpleTransparent);

  if (!m_sMessage.IsEmpty())
  {
    wdDebugRenderer::Draw2DText(*renderViewContext.m_pViewDebugContext, m_sMessage.GetData(), wdVec2I32(20, 20), wdColor::OrangeRed);
  }

  wdDebugRenderer::Render(renderViewContext);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
  RenderDataWithCategory(renderViewContext, wdDefaultRenderDataCategories::SimpleForeground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
  RenderDataWithCategory(renderViewContext, wdDefaultRenderDataCategories::SimpleForeground);

  RenderDataWithCategory(renderViewContext, wdDefaultRenderDataCategories::GUI);
}

void wdSimpleRenderPass::SetMessage(const char* szMessage)
{
  m_sMessage = szMessage;
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SimpleRenderPass);
