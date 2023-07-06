#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/ClusteredDataProvider.h>
#include <RendererCore/Lights/SimplifiedDataProvider.h>
#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdForwardRenderPass, 1, wdRTTINoAllocator)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("Color", m_PinColor),
    WD_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    WD_ENUM_MEMBER_PROPERTY("ShadingQuality", wdForwardRenderShadingQuality, m_ShadingQuality)->AddAttributes(new wdDefaultValueAttribute((int)wdForwardRenderShadingQuality::Normal)),
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_BEGIN_STATIC_REFLECTED_ENUM(wdForwardRenderShadingQuality, 1)
  WD_ENUM_CONSTANTS(wdForwardRenderShadingQuality::Normal, wdForwardRenderShadingQuality::Simplified)
WD_END_STATIC_REFLECTED_ENUM;
// clang-format on

wdForwardRenderPass::wdForwardRenderPass(const char* szName)
  : wdRenderPipelinePass(szName, true)
  , m_ShadingQuality(wdForwardRenderShadingQuality::Normal)
{
}

wdForwardRenderPass::~wdForwardRenderPass() = default;

bool wdForwardRenderPass::GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    outputs[m_PinColor.m_uiOutputIndex] = *inputs[m_PinColor.m_uiInputIndex];
  }
  else
  {
    wdLog::Error("No color input connected to pass '{0}'!", GetName());
    return false;
  }

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

void wdForwardRenderPass::Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs)
{
  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  wdGALPass* pGALPass = pDevice->BeginPass(GetName());

  SetupResources(pGALPass, renderViewContext, inputs, outputs);
  SetupPermutationVars(renderViewContext);
  SetupLighting(renderViewContext);

  RenderObjects(renderViewContext);

  renderViewContext.m_pRenderContext->EndRendering();
  pDevice->EndPass(pGALPass);
}

void wdForwardRenderPass::SetupResources(wdGALPass* pGALPass, const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs)
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

  renderViewContext.m_pRenderContext->BeginRendering(pGALPass, std::move(renderingSetup), renderViewContext.m_pViewData->m_ViewPortRect, "", renderViewContext.m_pCamera->IsStereoscopic());
}

void wdForwardRenderPass::SetupPermutationVars(const wdRenderViewContext& renderViewContext)
{
  wdTempHashedString sRenderPass("RENDER_PASS_FORWARD");
  if (renderViewContext.m_pViewData->m_ViewRenderMode != wdViewRenderMode::None)
  {
    sRenderPass = wdViewRenderMode::GetPermutationValue(renderViewContext.m_pViewData->m_ViewRenderMode);
  }

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", sRenderPass);

  wdStringBuilder sDebugText;
  wdViewRenderMode::GetDebugText(renderViewContext.m_pViewData->m_ViewRenderMode, sDebugText);
  if (!sDebugText.IsEmpty())
  {
    wdDebugRenderer::Draw2DText(*renderViewContext.m_pViewDebugContext, sDebugText, wdVec2I32(10, 10), wdColor::White);
  }

  // Set permutation for shading quality
  if (m_ShadingQuality == wdForwardRenderShadingQuality::Normal)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SHADING_QUALITY", "SHADING_QUALITY_NORMAL");
  }
  else if (m_ShadingQuality == wdForwardRenderShadingQuality::Simplified)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SHADING_QUALITY", "SHADING_QUALITY_SIMPLIFIED");
  }
  else
  {
    WD_REPORT_FAILURE("Unknown shading quality setting.");
  }
}

void wdForwardRenderPass::SetupLighting(const wdRenderViewContext& renderViewContext)
{
  // Setup clustered data
  if (m_ShadingQuality == wdForwardRenderShadingQuality::Normal)
  {
    auto pClusteredData = GetPipeline()->GetFrameDataProvider<wdClusteredDataProvider>()->GetData(renderViewContext);
    pClusteredData->BindResources(renderViewContext.m_pRenderContext);
  }
  // Or other light properties.
  else
  {
    auto pSimplifiedData = GetPipeline()->GetFrameDataProvider<wdSimplifiedDataProvider>()->GetData(renderViewContext);
    pSimplifiedData->BindResources(renderViewContext.m_pRenderContext);
    // todo
  }
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_ForwardRenderPass);
