#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/SeparatedBilateralBlur.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <Core/Graphics/Geometry.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/BilateralBlurConstants.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdSeparatedBilateralBlurPass, 2, wdRTTIDefaultAllocator<wdSeparatedBilateralBlurPass>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("BlurSource", m_PinBlurSourceInput),
    WD_MEMBER_PROPERTY("Depth", m_PinDepthInput),
    WD_MEMBER_PROPERTY("Output", m_PinOutput),
    WD_ACCESSOR_PROPERTY("BlurRadius", GetRadius, SetRadius)->AddAttributes(new wdDefaultValueAttribute(7)),
      // Should we really expose that? This gives the user control over the error compared to a perfect gaussian.
      // In theory we could also compute this for a given error from the blur radius. See http://dev.theomader.com/gaussian-kernel-calculator/ for visualization.
    WD_ACCESSOR_PROPERTY("GaussianSigma", GetGaussianSigma, SetGaussianSigma)->AddAttributes(new wdDefaultValueAttribute(4.0f)),
    WD_ACCESSOR_PROPERTY("Sharpness", GetSharpness, SetSharpness)->AddAttributes(new wdDefaultValueAttribute(120.0f)),
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdSeparatedBilateralBlurPass::wdSeparatedBilateralBlurPass()
  : wdRenderPipelinePass("SeparatedBilateral")

{
  {
    // Load shader.
    m_hShader = wdResourceManager::LoadResource<wdShaderResource>("Shaders/Pipeline/SeparatedBilateralBlur.wdShader");
    WD_ASSERT_DEV(m_hShader.IsValid(), "Could not load blur shader!");
  }

  {
    m_hBilateralBlurCB = wdRenderContext::CreateConstantBufferStorage<wdBilateralBlurConstants>();
  }
}

wdSeparatedBilateralBlurPass::~wdSeparatedBilateralBlurPass()
{
  wdRenderContext::DeleteConstantBufferStorage(m_hBilateralBlurCB);
  m_hBilateralBlurCB.Invalidate();
}

bool wdSeparatedBilateralBlurPass::GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs)
{
  WD_ASSERT_DEBUG(inputs.GetCount() == 2, "Unexpected number of inputs for wdSeparatedBilateralBlurPass.");

  // Color
  if (!inputs[m_PinBlurSourceInput.m_uiInputIndex])
  {
    wdLog::Error("No blur target connected to bilateral blur pass!");
    return false;
  }
  if (!inputs[m_PinBlurSourceInput.m_uiInputIndex]->m_bAllowShaderResourceView)
  {
    wdLog::Error("All bilateral blur pass inputs must allow shader resoure view.");
    return false;
  }

  // Depth
  if (!inputs[m_PinDepthInput.m_uiInputIndex])
  {
    wdLog::Error("No depth connected to bilateral blur pass!");
    return false;
  }
  if (!inputs[m_PinDepthInput.m_uiInputIndex]->m_bAllowShaderResourceView)
  {
    wdLog::Error("All bilateral blur pass inputs must allow shader resoure view.");
    return false;
  }
  if (inputs[m_PinBlurSourceInput.m_uiInputIndex]->m_uiWidth != inputs[m_PinDepthInput.m_uiInputIndex]->m_uiWidth || inputs[m_PinBlurSourceInput.m_uiInputIndex]->m_uiHeight != inputs[m_PinDepthInput.m_uiInputIndex]->m_uiHeight)
  {
    wdLog::Error("Blur target and depth buffer for bilateral blur pass need to have the same dimensions.");
    return false;
  }


  // Output format maches input format.
  outputs[m_PinOutput.m_uiOutputIndex] = *inputs[m_PinBlurSourceInput.m_uiInputIndex];

  return true;
}

void wdSeparatedBilateralBlurPass::Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs)
{
  if (outputs[m_PinOutput.m_uiOutputIndex])
  {
    wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();
    wdGALPass* pGALPass = pDevice->BeginPass(GetName());
    WD_SCOPE_EXIT(pDevice->EndPass(pGALPass));

    // Setup input view and sampler
    wdGALResourceViewCreationDescription rvcd;
    rvcd.m_hTexture = inputs[m_PinBlurSourceInput.m_uiInputIndex]->m_TextureHandle;
    wdGALResourceViewHandle hBlurSourceInputView = wdGALDevice::GetDefaultDevice()->CreateResourceView(rvcd);
    rvcd.m_hTexture = inputs[m_PinDepthInput.m_uiInputIndex]->m_TextureHandle;
    wdGALResourceViewHandle hDepthInputView = wdGALDevice::GetDefaultDevice()->CreateResourceView(rvcd);

    // Get temp texture for horizontal target / vertical source.
    wdGALTextureCreationDescription tempTextureDesc = outputs[m_PinBlurSourceInput.m_uiInputIndex]->m_Desc;
    tempTextureDesc.m_bAllowShaderResourceView = true;
    tempTextureDesc.m_bCreateRenderTarget = true;
    wdGALTextureHandle tempTexture = wdGPUResourcePool::GetDefaultInstance()->GetRenderTarget(tempTextureDesc);
    rvcd.m_hTexture = tempTexture;
    wdGALResourceViewHandle hTempTextureRView = wdGALDevice::GetDefaultDevice()->CreateResourceView(rvcd);

    wdGALRenderingSetup renderingSetup;

    // Bind shader and inputs
    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindMeshBuffer(wdGALBufferHandle(), wdGALBufferHandle(), nullptr, wdGALPrimitiveTopology::Triangles, 1);
    renderViewContext.m_pRenderContext->BindTexture2D("DepthBuffer", hDepthInputView);
    renderViewContext.m_pRenderContext->BindConstantBuffer("wdBilateralBlurConstants", m_hBilateralBlurCB);

    // Horizontal
    {
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(tempTexture));
      auto pCommandEncoder = wdRenderContext::BeginRenderingScope(pGALPass, renderViewContext, renderingSetup, "", renderViewContext.m_pCamera->IsStereoscopic());

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLUR_DIRECTION", "BLUR_DIRECTION_HORIZONTAL");
      renderViewContext.m_pRenderContext->BindTexture2D("BlurSource", hBlurSourceInputView);
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
    }

    // Vertical
    {
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(outputs[m_PinOutput.m_uiOutputIndex]->m_TextureHandle));
      auto pCommandEncoder = wdRenderContext::BeginRenderingScope(pGALPass, renderViewContext, renderingSetup, "", renderViewContext.m_pCamera->IsStereoscopic());

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLUR_DIRECTION", "BLUR_DIRECTION_VERTICAL");
      renderViewContext.m_pRenderContext->BindTexture2D("BlurSource", hTempTextureRView);
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
    }

    // Give back temp texture.
    wdGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(tempTexture);
  }
}

void wdSeparatedBilateralBlurPass::SetRadius(wdUInt32 uiRadius)
{
  m_uiRadius = uiRadius;

  wdBilateralBlurConstants* cb = wdRenderContext::GetConstantBufferData<wdBilateralBlurConstants>(m_hBilateralBlurCB);
  cb->BlurRadius = m_uiRadius;
}

wdUInt32 wdSeparatedBilateralBlurPass::GetRadius() const
{
  return m_uiRadius;
}

void wdSeparatedBilateralBlurPass::SetGaussianSigma(const float fSigma)
{
  m_fGaussianSigma = fSigma;

  wdBilateralBlurConstants* cb = wdRenderContext::GetConstantBufferData<wdBilateralBlurConstants>(m_hBilateralBlurCB);
  cb->GaussianFalloff = 1.0f / (2.0f * m_fGaussianSigma * m_fGaussianSigma);
}

float wdSeparatedBilateralBlurPass::GetGaussianSigma() const
{
  return m_fGaussianSigma;
}

void wdSeparatedBilateralBlurPass::SetSharpness(const float fSharpness)
{
  m_fSharpness = fSharpness;

  wdBilateralBlurConstants* cb = wdRenderContext::GetConstantBufferData<wdBilateralBlurConstants>(m_hBilateralBlurCB);
  cb->Sharpness = m_fSharpness;
}

float wdSeparatedBilateralBlurPass::GetSharpness() const
{
  return m_fSharpness;
}



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class wdSeparatedBilateralBlurPassPatch_1_2 : public wdGraphPatch
{
public:
  wdSeparatedBilateralBlurPassPatch_1_2()
    : wdGraphPatch("wdSeparatedBilateralBlurPass", 2)
  {
  }

  virtual void Patch(wdGraphPatchContext& ref_context, wdAbstractObjectGraph* pGraph, wdAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Blur Radius", "BlurRadius");
    pNode->RenameProperty("Gaussian Standard Deviation", "GaussianSigma");
    pNode->RenameProperty("Bilateral Sharpness", "Sharpness");
  }
};

wdSeparatedBilateralBlurPassPatch_1_2 g_wdSeparatedBilateralBlurPassPatch_1_2;



WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SeparatedBilateralBlur);
