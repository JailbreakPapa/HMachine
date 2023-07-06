#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/BlurPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <Core/Graphics/Geometry.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/BlurConstants.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdBlurPass, 1, wdRTTIDefaultAllocator<wdBlurPass>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("Input", m_PinInput),
    WD_MEMBER_PROPERTY("Output", m_PinOutput),
    WD_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new wdDefaultValueAttribute(15)),
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdBlurPass::wdBlurPass()
  : wdRenderPipelinePass("BlurPass")

{
  {
    // Load shader.
    m_hShader = wdResourceManager::LoadResource<wdShaderResource>("Shaders/Pipeline/Blur.wdShader");
    WD_ASSERT_DEV(m_hShader.IsValid(), "Could not load blur shader!");
  }

  {
    m_hBlurCB = wdRenderContext::CreateConstantBufferStorage<wdBlurConstants>();
  }
}

wdBlurPass::~wdBlurPass()
{
  wdRenderContext::DeleteConstantBufferStorage(m_hBlurCB);
  m_hBlurCB.Invalidate();
}

bool wdBlurPass::GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinInput.m_uiInputIndex])
  {
    if (!inputs[m_PinInput.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      wdLog::Error("Blur pass input must allow shader resoure view.");
      return false;
    }

    outputs[m_PinOutput.m_uiOutputIndex] = *inputs[m_PinInput.m_uiInputIndex];
  }
  else
  {
    wdLog::Error("No input connected to blur pass!");
    return false;
  }

  return true;
}

void wdBlurPass::Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs)
{
  if (outputs[m_PinOutput.m_uiOutputIndex])
  {
    wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

    // Setup render target
    wdGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(outputs[m_PinOutput.m_uiOutputIndex]->m_TextureHandle));
    renderingSetup.m_uiRenderTargetClearMask = wdInvalidIndex;
    renderingSetup.m_ClearColor = wdColor(1.0f, 0.0f, 0.0f);

    // Bind render target and viewport
    auto pCommandEncoder = wdRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, GetName(), renderViewContext.m_pCamera->IsStereoscopic());

    // Setup input view and sampler
    wdGALResourceViewCreationDescription rvcd;
    rvcd.m_hTexture = inputs[m_PinInput.m_uiInputIndex]->m_TextureHandle;
    wdGALResourceViewHandle hResourceView = wdGALDevice::GetDefaultDevice()->CreateResourceView(rvcd);

    // Bind shader and inputs
    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindMeshBuffer(wdGALBufferHandle(), wdGALBufferHandle(), nullptr, wdGALPrimitiveTopology::Triangles, 1);
    renderViewContext.m_pRenderContext->BindTexture2D("Input", hResourceView);
    renderViewContext.m_pRenderContext->BindConstantBuffer("wdBlurConstants", m_hBlurCB);

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }
}

void wdBlurPass::SetRadius(wdInt32 iRadius)
{
  m_iRadius = iRadius;

  wdBlurConstants* cb = wdRenderContext::GetConstantBufferData<wdBlurConstants>(m_hBlurCB);
  cb->BlurRadius = m_iRadius;
}

wdInt32 wdBlurPass::GetRadius() const
{
  return m_iRadius;
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_BlurPass);
