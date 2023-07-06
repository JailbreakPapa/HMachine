#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/AntialiasingPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdAntialiasingPass, 1, wdRTTIDefaultAllocator<wdAntialiasingPass>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("Input", m_PinInput),
    WD_MEMBER_PROPERTY("Output", m_PinOutput)
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdAntialiasingPass::wdAntialiasingPass()
  : wdRenderPipelinePass("AntialiasingPass", true)
{
  {
    // Load shader.
    m_hShader = wdResourceManager::LoadResource<wdShaderResource>("Shaders/Pipeline/Antialiasing.wdShader");
    WD_ASSERT_DEV(m_hShader.IsValid(), "Could not load antialiasing shader!");
  }
}

wdAntialiasingPass::~wdAntialiasingPass() = default;

bool wdAntialiasingPass::GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs)
{
  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  if (pInput != nullptr)
  {
    if (pInput->m_SampleCount == wdGALMSAASampleCount::TwoSamples)
    {
      m_sMsaaSampleCount.Assign("MSAA_SAMPLES_TWO");
    }
    else if (pInput->m_SampleCount == wdGALMSAASampleCount::FourSamples)
    {
      m_sMsaaSampleCount.Assign("MSAA_SAMPLES_FOUR");
    }
    else if (pInput->m_SampleCount == wdGALMSAASampleCount::EightSamples)
    {
      m_sMsaaSampleCount.Assign("MSAA_SAMPLES_EIGHT");
    }
    else
    {
      wdLog::Error("Input is not a valid msaa target");
      return false;
    }

    wdGALTextureCreationDescription desc = *pInput;
    desc.m_SampleCount = wdGALMSAASampleCount::None;

    outputs[m_PinOutput.m_uiOutputIndex] = desc;
  }
  else
  {
    wdLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void wdAntialiasingPass::Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs)
{
  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  // Setup render target
  wdGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));

  // Bind render target and viewport
  auto pCommandEncoder = wdRenderContext::BeginPassAndRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("MSAA_SAMPLES", m_sMsaaSampleCount);

  renderViewContext.m_pRenderContext->BindShader(m_hShader);

  renderViewContext.m_pRenderContext->BindMeshBuffer(wdGALBufferHandle(), wdGALBufferHandle(), nullptr, wdGALPrimitiveTopology::Triangles, 1);
  renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", pDevice->GetDefaultResourceView(pInput->m_TextureHandle));

  renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_AntialiasingPass);
