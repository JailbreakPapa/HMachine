#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/SelectionHighlightPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/SelectionHighlightConstants.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdSelectionHighlightPass, 1, wdRTTIDefaultAllocator<wdSelectionHighlightPass>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("Color", m_PinColor),
    WD_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),

    WD_MEMBER_PROPERTY("HighlightColor", m_HighlightColor)->AddAttributes(new wdDefaultValueAttribute(wdColorScheme::LightUI(wdColorScheme::Yellow))),
    WD_MEMBER_PROPERTY("OverlayOpacity", m_fOverlayOpacity)->AddAttributes(new wdDefaultValueAttribute(0.1f))
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdSelectionHighlightPass::wdSelectionHighlightPass(const char* szName)
  : wdRenderPipelinePass(szName, true)
{
  // Load shader.
  m_hShader = wdResourceManager::LoadResource<wdShaderResource>("Shaders/Pipeline/SelectionHighlight.wdShader");
  WD_ASSERT_DEV(m_hShader.IsValid(), "Could not load selection highlight shader!");

  m_hConstantBuffer = wdRenderContext::CreateConstantBufferStorage<wdSelectionHighlightConstants>();
}

wdSelectionHighlightPass::~wdSelectionHighlightPass()
{
  wdRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool wdSelectionHighlightPass::GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    outputs[m_PinColor.m_uiOutputIndex] = *inputs[m_PinColor.m_uiInputIndex];
    return true;
  }

  return false;
}

void wdSelectionHighlightPass::Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs)
{
  auto pColorOutput = outputs[m_PinColor.m_uiOutputIndex];
  if (pColorOutput == nullptr)
  {
    return;
  }

  auto pDepthInput = inputs[m_PinDepthStencil.m_uiInputIndex];
  if (pDepthInput == nullptr)
  {
    return;
  }

  wdRenderDataBatchList renderDataBatchList = GetPipeline()->GetRenderDataBatchesWithCategory(wdDefaultRenderDataCategories::Selection);
  if (renderDataBatchList.GetBatchCount() == 0)
  {
    return;
  }

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  wdGALTextureHandle hDepthTexture;

  // render all selection objects to depth target only
  {
    wdUInt32 uiWidth = pColorOutput->m_Desc.m_uiWidth;
    wdUInt32 uiHeight = pColorOutput->m_Desc.m_uiHeight;
    wdGALMSAASampleCount::Enum sampleCount = pColorOutput->m_Desc.m_SampleCount;
    wdUInt32 uiSliceCount = pColorOutput->m_Desc.m_uiArraySize;

    hDepthTexture = wdGPUResourcePool::GetDefaultInstance()->GetRenderTarget(uiWidth, uiHeight, wdGALResourceFormat::D24S8, sampleCount, uiSliceCount);

    wdGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(hDepthTexture));
    renderingSetup.m_bClearDepth = true;
    renderingSetup.m_bClearStencil = true;

    auto pCommandEncoder = wdRenderContext::BeginPassAndRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_DEPTH_ONLY");

    RenderDataWithCategory(renderViewContext, wdDefaultRenderDataCategories::Selection);
  }

  // reconstruct selection overlay from depth target
  {
    auto constants = wdRenderContext::GetConstantBufferData<wdSelectionHighlightConstants>(m_hConstantBuffer);
    constants->HighlightColor = m_HighlightColor;
    constants->OverlayOpacity = m_fOverlayOpacity;

    wdGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pColorOutput->m_TextureHandle));

    auto pCommandEncoder = wdRenderContext::BeginPassAndRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindConstantBuffer("wdSelectionHighlightConstants", m_hConstantBuffer);
    renderViewContext.m_pRenderContext->BindMeshBuffer(wdGALBufferHandle(), wdGALBufferHandle(), nullptr, wdGALPrimitiveTopology::Triangles, 1);
    renderViewContext.m_pRenderContext->BindTexture2D("SelectionDepthTexture", pDevice->GetDefaultResourceView(hDepthTexture));
    renderViewContext.m_pRenderContext->BindTexture2D("SceneDepthTexture", pDevice->GetDefaultResourceView(pDepthInput->m_TextureHandle));

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

    wdGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hDepthTexture);
  }
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SelectionHighlightPass);
