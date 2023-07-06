#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/MsaaResolvePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdMsaaResolvePass, 1, wdRTTIDefaultAllocator<wdMsaaResolvePass>)
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

wdMsaaResolvePass::wdMsaaResolvePass()
  : wdRenderPipelinePass("MsaaResolvePass", true)

{
  {
    // Load shader.
    m_hDepthResolveShader = wdResourceManager::LoadResource<wdShaderResource>("Shaders/Pipeline/MsaaDepthResolve.wdShader");
    WD_ASSERT_DEV(m_hDepthResolveShader.IsValid(), "Could not load depth resolve shader!");
  }
}

wdMsaaResolvePass::~wdMsaaResolvePass() = default;

bool wdMsaaResolvePass::GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs)
{
  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  if (pInput != nullptr)
  {
    if (pInput->m_SampleCount == wdGALMSAASampleCount::None)
    {
      wdLog::Error("Input is not a valid msaa target");
      return false;
    }

    m_bIsDepth = wdGALResourceFormat::IsDepthFormat(pInput->m_Format);
    m_MsaaSampleCount = pInput->m_SampleCount;

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

void wdMsaaResolvePass::Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs)
{
  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  if (m_bIsDepth)
  {
    // Setup render target
    wdGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));

    // Bind render target and viewport
    auto pCommandEncoder = wdRenderContext::BeginPassAndRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

    auto& globals = renderViewContext.m_pRenderContext->WriteGlobalConstants();
    globals.NumMsaaSamples = m_MsaaSampleCount;

    renderViewContext.m_pRenderContext->BindShader(m_hDepthResolveShader);
    renderViewContext.m_pRenderContext->BindMeshBuffer(wdGALBufferHandle(), wdGALBufferHandle(), nullptr, wdGALPrimitiveTopology::Triangles, 1);
    renderViewContext.m_pRenderContext->BindTexture2D("DepthTexture", pDevice->GetDefaultResourceView(pInput->m_TextureHandle));

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }
  else
  {
    auto pCommandEncoder = wdRenderContext::BeginPassAndRenderingScope(renderViewContext, wdGALRenderingSetup(), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

    wdGALTextureSubresource subresource;
    subresource.m_uiMipLevel = 0;
    subresource.m_uiArraySlice = 0;

    pCommandEncoder->ResolveTexture(pOutput->m_TextureHandle, subresource, pInput->m_TextureHandle, subresource);

    if (renderViewContext.m_pCamera->IsStereoscopic())
    {
      subresource.m_uiArraySlice = 1;
      pCommandEncoder->ResolveTexture(pOutput->m_TextureHandle, subresource, pInput->m_TextureHandle, subresource);
    }
  }
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_MsaaResolvePass);
