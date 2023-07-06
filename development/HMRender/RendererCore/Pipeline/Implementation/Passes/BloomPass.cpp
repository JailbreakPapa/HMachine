#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/BloomPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/BloomConstants.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdBloomPass, 1, wdRTTIDefaultAllocator<wdBloomPass>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("Input", m_PinInput),
    WD_MEMBER_PROPERTY("Output", m_PinOutput),
    WD_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new wdDefaultValueAttribute(0.2f), new wdClampValueAttribute(0.01f, 1.0f)),
    WD_MEMBER_PROPERTY("Threshold", m_fThreshold)->AddAttributes(new wdDefaultValueAttribute(1.0f)),
    WD_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new wdDefaultValueAttribute(0.3f)),
    WD_MEMBER_PROPERTY("InnerTintColor", m_InnerTintColor),
    WD_MEMBER_PROPERTY("MidTintColor", m_MidTintColor),
    WD_MEMBER_PROPERTY("OuterTintColor", m_OuterTintColor),
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdBloomPass::wdBloomPass()
  : wdRenderPipelinePass("BloomPass", true)
{
  {
    // Load shader.
    m_hShader = wdResourceManager::LoadResource<wdShaderResource>("Shaders/Pipeline/Bloom.wdShader");
    WD_ASSERT_DEV(m_hShader.IsValid(), "Could not load bloom shader!");
  }

  {
    m_hConstantBuffer = wdRenderContext::CreateConstantBufferStorage<wdBloomConstants>();
  }
}

wdBloomPass::~wdBloomPass()
{
  wdRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool wdBloomPass::GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinInput.m_uiInputIndex])
  {
    if (!inputs[m_PinInput.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      wdLog::Error("'{0}' input must allow shader resource view.", GetName());
      return false;
    }

    // Output is half-res
    wdGALTextureCreationDescription desc = *inputs[m_PinInput.m_uiInputIndex];
    desc.m_uiWidth = desc.m_uiWidth / 2;
    desc.m_uiHeight = desc.m_uiHeight / 2;
    desc.m_Format = wdGALResourceFormat::RG11B10Float;

    outputs[m_PinOutput.m_uiOutputIndex] = desc;
  }
  else
  {
    wdLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void wdBloomPass::Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs)
{
  auto pColorInput = inputs[m_PinInput.m_uiInputIndex];
  auto pColorOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pColorInput == nullptr || pColorOutput == nullptr)
  {
    return;
  }

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();
  wdGALPass* pGALPass = pDevice->BeginPass(GetName());
  WD_SCOPE_EXIT(pDevice->EndPass(pGALPass));

  wdUInt32 uiWidth = pColorInput->m_Desc.m_uiWidth;
  wdUInt32 uiHeight = pColorInput->m_Desc.m_uiHeight;
  bool bFastDownscale = wdMath::IsEven(uiWidth) && wdMath::IsEven(uiHeight);

  const float fMaxRes = (float)wdMath::Max(uiWidth, uiHeight);
  const float fRadius = wdMath::Clamp(m_fRadius, 0.01f, 1.0f);
  const float fDownscaledSize = 4.0f / fRadius;
  const float fNumBlurPasses = wdMath::Log2(fMaxRes / fDownscaledSize);
  const wdUInt32 uiNumBlurPasses = (wdUInt32)wdMath::Ceil(fNumBlurPasses);

  // Find temp targets
  wdHybridArray<wdVec2, 8> targetSizes;
  wdHybridArray<wdGALTextureHandle, 8> tempDownscaleTextures;
  wdHybridArray<wdGALTextureHandle, 8> tempUpscaleTextures;

  for (wdUInt32 i = 0; i < uiNumBlurPasses; ++i)
  {
    uiWidth = wdMath::Max(uiWidth / 2, 1u);
    uiHeight = wdMath::Max(uiHeight / 2, 1u);
    targetSizes.PushBack(wdVec2((float)uiWidth, (float)uiHeight));
    auto uiSliceCount = pColorOutput->m_Desc.m_uiArraySize;

    tempDownscaleTextures.PushBack(wdGPUResourcePool::GetDefaultInstance()->GetRenderTarget(uiWidth, uiHeight, wdGALResourceFormat::RG11B10Float, wdGALMSAASampleCount::None, uiSliceCount));

    // biggest upscale target is the output and lowest is not needed
    if (i > 0 && i < uiNumBlurPasses - 1)
    {
      tempUpscaleTextures.PushBack(wdGPUResourcePool::GetDefaultInstance()->GetRenderTarget(uiWidth, uiHeight, wdGALResourceFormat::RG11B10Float, wdGALMSAASampleCount::None, uiSliceCount));
    }
    else
    {
      tempUpscaleTextures.PushBack(wdGALTextureHandle());
    }
  }

  renderViewContext.m_pRenderContext->BindConstantBuffer("wdBloomConstants", m_hConstantBuffer);
  renderViewContext.m_pRenderContext->BindShader(m_hShader);

  renderViewContext.m_pRenderContext->BindMeshBuffer(wdGALBufferHandle(), wdGALBufferHandle(), nullptr, wdGALPrimitiveTopology::Triangles, 1);

  // Downscale passes
  {
    wdTempHashedString sInitialDownscale = "BLOOM_PASS_MODE_INITIAL_DOWNSCALE";
    wdTempHashedString sInitialDownscaleFast = "BLOOM_PASS_MODE_INITIAL_DOWNSCALE_FAST";
    wdTempHashedString sDownscale = "BLOOM_PASS_MODE_DOWNSCALE";
    wdTempHashedString sDownscaleFast = "BLOOM_PASS_MODE_DOWNSCALE_FAST";

    for (wdUInt32 i = 0; i < uiNumBlurPasses; ++i)
    {
      wdGALTextureHandle hInput;
      if (i == 0)
      {
        hInput = pColorInput->m_TextureHandle;
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLOOM_PASS_MODE", bFastDownscale ? sInitialDownscaleFast : sInitialDownscale);
      }
      else
      {
        hInput = tempDownscaleTextures[i - 1];
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLOOM_PASS_MODE", bFastDownscale ? sDownscaleFast : sDownscale);
      }

      wdGALTextureHandle hOutput = tempDownscaleTextures[i];
      wdVec2 targetSize = targetSizes[i];

      wdGALRenderingSetup renderingSetup;
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(hOutput));
      renderViewContext.m_pRenderContext->BeginRendering(pGALPass, renderingSetup, wdRectFloat(targetSize.x, targetSize.y), "Downscale", renderViewContext.m_pCamera->IsStereoscopic());

      wdColor tintColor = (i == uiNumBlurPasses - 1) ? wdColor(m_OuterTintColor) : wdColor::White;
      UpdateConstantBuffer(wdVec2(1.0f).CompDiv(targetSize), tintColor);

      renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", pDevice->GetDefaultResourceView(hInput));
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

      renderViewContext.m_pRenderContext->EndRendering();

      bFastDownscale = wdMath::IsEven((wdInt32)targetSize.x) && wdMath::IsEven((wdInt32)targetSize.y);
    }
  }

  // Upscale passes
  {
    const float fBlurRadius = 2.0f * fNumBlurPasses / uiNumBlurPasses;
    const float fMidPass = (uiNumBlurPasses - 1.0f) / 2.0f;

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLOOM_PASS_MODE", "BLOOM_PASS_MODE_UPSCALE");

    for (wdUInt32 i = uiNumBlurPasses - 1; i-- > 0;)
    {
      wdGALTextureHandle hNextInput = tempDownscaleTextures[i];
      wdGALTextureHandle hInput;
      if (i == uiNumBlurPasses - 2)
      {
        hInput = tempDownscaleTextures[i + 1];
      }
      else
      {
        hInput = tempUpscaleTextures[i + 1];
      }

      wdGALTextureHandle hOutput;
      if (i == 0)
      {
        hOutput = pColorOutput->m_TextureHandle;
      }
      else
      {
        hOutput = tempUpscaleTextures[i];
      }

      wdVec2 targetSize = targetSizes[i];

      wdGALRenderingSetup renderingSetup;
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(hOutput));
      renderViewContext.m_pRenderContext->BeginRendering(pGALPass, renderingSetup, wdRectFloat(targetSize.x, targetSize.y), "Upscale", renderViewContext.m_pCamera->IsStereoscopic());

      wdColor tintColor;
      float fPass = (float)i;
      if (fPass < fMidPass)
      {
        tintColor = wdMath::Lerp<wdColor>(m_InnerTintColor, m_MidTintColor, fPass / fMidPass);
      }
      else
      {
        tintColor = wdMath::Lerp<wdColor>(m_MidTintColor, m_OuterTintColor, (fPass - fMidPass) / fMidPass);
      }

      UpdateConstantBuffer(wdVec2(fBlurRadius).CompDiv(targetSize), tintColor);

      renderViewContext.m_pRenderContext->BindTexture2D("NextColorTexture", pDevice->GetDefaultResourceView(hNextInput));
      renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", pDevice->GetDefaultResourceView(hInput));
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

      renderViewContext.m_pRenderContext->EndRendering();
    }
  }

  // Return temp targets
  for (auto hTexture : tempDownscaleTextures)
  {
    if (!hTexture.IsInvalidated())
    {
      wdGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hTexture);
    }
  }

  for (auto hTexture : tempUpscaleTextures)
  {
    if (!hTexture.IsInvalidated())
    {
      wdGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hTexture);
    }
  }
}

void wdBloomPass::ExecuteInactive(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs)
{
  auto pColorOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pColorOutput == nullptr)
  {
    return;
  }

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  wdGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pColorOutput->m_TextureHandle));
  renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
  renderingSetup.m_ClearColor = wdColor::Black;

  auto pCommandEncoder = wdRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, "Clear");
}

void wdBloomPass::UpdateConstantBuffer(wdVec2 pixelSize, const wdColor& tintColor)
{
  wdBloomConstants* constants = wdRenderContext::GetConstantBufferData<wdBloomConstants>(m_hConstantBuffer);
  constants->PixelSize = pixelSize;
  constants->BloomThreshold = m_fThreshold;
  constants->BloomIntensity = m_fIntensity;

  constants->TintColor = tintColor;
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_BloomPass);
