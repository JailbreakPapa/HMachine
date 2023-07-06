#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/AOPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/DownscaleDepthConstants.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/SSAOConstants.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdAOPass, 1, wdRTTIDefaultAllocator<wdAOPass>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("DepthInput", m_PinDepthInput),
    WD_MEMBER_PROPERTY("Output", m_PinOutput),
    WD_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new wdDefaultValueAttribute(1.0f), new wdClampValueAttribute(0.01f, 10.0f)),
    WD_MEMBER_PROPERTY("MaxScreenSpaceRadius", m_fMaxScreenSpaceRadius)->AddAttributes(new wdDefaultValueAttribute(1.0f), new wdClampValueAttribute(0.01f, 2.0f)),
    WD_MEMBER_PROPERTY("Contrast", m_fContrast)->AddAttributes(new wdDefaultValueAttribute(2.0f)),
    WD_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new wdDefaultValueAttribute(0.7f)),
    WD_ACCESSOR_PROPERTY("FadeOutStart", GetFadeOutStart, SetFadeOutStart)->AddAttributes(new wdDefaultValueAttribute(80.0f), new wdClampValueAttribute(0.0f, wdVariant())),
    WD_ACCESSOR_PROPERTY("FadeOutEnd", GetFadeOutEnd, SetFadeOutEnd)->AddAttributes(new wdDefaultValueAttribute(100.0f), new wdClampValueAttribute(0.0f, wdVariant())),
    WD_MEMBER_PROPERTY("PositionBias", m_fPositionBias)->AddAttributes(new wdDefaultValueAttribute(5.0f), new wdClampValueAttribute(0.0f, 1000.0f)),
    WD_MEMBER_PROPERTY("MipLevelScale", m_fMipLevelScale)->AddAttributes(new wdDefaultValueAttribute(10.0f), new wdClampValueAttribute(0.0f, wdVariant())),
    WD_MEMBER_PROPERTY("DepthBlurThreshold", m_fDepthBlurThreshold)->AddAttributes(new wdDefaultValueAttribute(2.0f), new wdClampValueAttribute(0.01f, wdVariant())),
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdAOPass::wdAOPass()
  : wdRenderPipelinePass("AOPass", true)

{
  m_hNoiseTexture = wdResourceManager::LoadResource<wdTexture2DResource>("Textures/SSAONoise.dds");

  m_hDownscaleShader = wdResourceManager::LoadResource<wdShaderResource>("Shaders/Pipeline/DownscaleDepth.wdShader");
  WD_ASSERT_DEV(m_hDownscaleShader.IsValid(), "Could not load downsample shader!");

  m_hSSAOShader = wdResourceManager::LoadResource<wdShaderResource>("Shaders/Pipeline/SSAO.wdShader");
  WD_ASSERT_DEV(m_hSSAOShader.IsValid(), "Could not load SSAO shader!");

  m_hBlurShader = wdResourceManager::LoadResource<wdShaderResource>("Shaders/Pipeline/SSAOBlur.wdShader");
  WD_ASSERT_DEV(m_hBlurShader.IsValid(), "Could not load SSAO shader!");

  m_hDownscaleConstantBuffer = wdRenderContext::CreateConstantBufferStorage<wdDownscaleDepthConstants>();
  m_hSSAOConstantBuffer = wdRenderContext::CreateConstantBufferStorage<wdSSAOConstants>();
}

wdAOPass::~wdAOPass()
{
  if (!m_hSSAOSamplerState.IsInvalidated())
  {
    wdGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSSAOSamplerState);
    m_hSSAOSamplerState.Invalidate();
  }

  wdRenderContext::DeleteConstantBufferStorage(m_hDownscaleConstantBuffer);
  m_hDownscaleConstantBuffer.Invalidate();

  wdRenderContext::DeleteConstantBufferStorage(m_hSSAOConstantBuffer);
  m_hSSAOConstantBuffer.Invalidate();
}

bool wdAOPass::GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs)
{
  if (auto pDepthInput = inputs[m_PinDepthInput.m_uiInputIndex])
  {
    if (!pDepthInput->m_bAllowShaderResourceView)
    {
      wdLog::Error("'{0}' input must allow shader resource view.", GetName());
      return false;
    }

    if (pDepthInput->m_SampleCount != wdGALMSAASampleCount::None)
    {
      wdLog::Error("'{0}' input must be resolved", GetName());
      return false;
    }

    wdGALTextureCreationDescription desc = *pDepthInput;
    desc.m_Format = wdGALResourceFormat::RGHalf;

    outputs[m_PinOutput.m_uiOutputIndex] = desc;
  }
  else
  {
    wdLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void wdAOPass::Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs)
{
  auto pDepthInput = inputs[m_PinDepthInput.m_uiInputIndex];
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pDepthInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();
  wdGALPass* pGALPass = pDevice->BeginPass(GetName());
  WD_SCOPE_EXIT(pDevice->EndPass(pGALPass));

  wdUInt32 uiWidth = pDepthInput->m_Desc.m_uiWidth;
  wdUInt32 uiHeight = pDepthInput->m_Desc.m_uiHeight;

  wdUInt32 uiNumMips = 3;
  wdUInt32 uiHzbWidth = wdMath::RoundUp(uiWidth, 1u << uiNumMips);
  wdUInt32 uiHzbHeight = wdMath::RoundUp(uiHeight, 1u << uiNumMips);

  float fHzbScaleX = (float)uiWidth / uiHzbWidth;
  float fHzbScaleY = (float)uiHeight / uiHzbHeight;

  // Find temp targets
  wdGALTextureHandle hzbTexture;
  wdHybridArray<wdVec2, 8> hzbSizes;
  wdHybridArray<wdGALResourceViewHandle, 8> hzbResourceViews;
  wdHybridArray<wdGALRenderTargetViewHandle, 8> hzbRenderTargetViews;

  wdGALTextureHandle tempSSAOTexture;

  {
    {
      wdGALTextureCreationDescription desc;
      desc.m_uiWidth = uiHzbWidth / 2;
      desc.m_uiHeight = uiHzbHeight / 2;
      desc.m_uiMipLevelCount = 3;
      desc.m_Type = wdGALTextureType::Texture2D;
      desc.m_Format = wdGALResourceFormat::RHalf;
      desc.m_bCreateRenderTarget = true;
      desc.m_bAllowShaderResourceView = true;
      desc.m_uiArraySize = pOutput->m_Desc.m_uiArraySize;

      hzbTexture = wdGPUResourcePool::GetDefaultInstance()->GetRenderTarget(desc);
    }

    for (wdUInt32 i = 0; i < uiNumMips; ++i)
    {
      uiHzbWidth = uiHzbWidth / 2;
      uiHzbHeight = uiHzbHeight / 2;

      hzbSizes.PushBack(wdVec2((float)uiHzbWidth, (float)uiHzbHeight));

      {
        wdGALResourceViewCreationDescription desc;
        desc.m_hTexture = hzbTexture;
        desc.m_uiMostDetailedMipLevel = i;
        desc.m_uiMipLevelsToUse = 1;
        desc.m_uiArraySize = pOutput->m_Desc.m_uiArraySize;

        hzbResourceViews.PushBack(pDevice->CreateResourceView(desc));
      }

      {
        wdGALRenderTargetViewCreationDescription desc;
        desc.m_hTexture = hzbTexture;
        desc.m_uiMipLevel = i;
        desc.m_uiSliceCount = pOutput->m_Desc.m_uiArraySize;

        hzbRenderTargetViews.PushBack(pDevice->CreateRenderTargetView(desc));
      }
    }

    tempSSAOTexture = wdGPUResourcePool::GetDefaultInstance()->GetRenderTarget(uiWidth, uiHeight, wdGALResourceFormat::RGHalf, wdGALMSAASampleCount::None, pOutput->m_Desc.m_uiArraySize);
  }

  // Mip map passes
  {
    CreateSamplerState();

    for (wdUInt32 i = 0; i < uiNumMips; ++i)
    {
      wdGALResourceViewHandle hInputView;
      wdVec2 pixelSize;

      if (i == 0)
      {
        hInputView = pDevice->GetDefaultResourceView(pDepthInput->m_TextureHandle);
        pixelSize = wdVec2(1.0f / uiWidth, 1.0f / uiHeight);
      }
      else
      {
        hInputView = hzbResourceViews[i - 1];
        pixelSize = wdVec2(1.0f).CompDiv(hzbSizes[i - 1]);
      }

      wdGALRenderTargetViewHandle hOutputView = hzbRenderTargetViews[i];
      wdVec2 targetSize = hzbSizes[i];

      wdGALRenderingSetup renderingSetup;
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, hOutputView);
      renderViewContext.m_pRenderContext->BeginRendering(pGALPass, renderingSetup, wdRectFloat(targetSize.x, targetSize.y), "SSAOMipMaps", renderViewContext.m_pCamera->IsStereoscopic());

      wdDownscaleDepthConstants* constants = wdRenderContext::GetConstantBufferData<wdDownscaleDepthConstants>(m_hDownscaleConstantBuffer);
      constants->PixelSize = pixelSize;
      constants->LinearizeDepth = (i == 0);

      renderViewContext.m_pRenderContext->BindConstantBuffer("wdDownscaleDepthConstants", m_hDownscaleConstantBuffer);
      renderViewContext.m_pRenderContext->BindShader(m_hDownscaleShader);

      renderViewContext.m_pRenderContext->BindTexture2D("DepthTexture", hInputView);
      renderViewContext.m_pRenderContext->BindSamplerState("DepthSampler", m_hSSAOSamplerState);

      renderViewContext.m_pRenderContext->BindNullMeshBuffer(wdGALPrimitiveTopology::Triangles, 1);

      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

      renderViewContext.m_pRenderContext->EndRendering();
    }
  }

  // Update constants
  {
    float fadeOutScale = -1.0f / wdMath::Max(0.001f, (m_fFadeOutEnd - m_fFadeOutStart));
    float fadeOutOffset = -fadeOutScale * m_fFadeOutStart + 1.0f;

    wdSSAOConstants* constants = wdRenderContext::GetConstantBufferData<wdSSAOConstants>(m_hSSAOConstantBuffer);
    constants->TexCoordsScale = wdVec2(fHzbScaleX, fHzbScaleY);
    constants->FadeOutParams = wdVec2(fadeOutScale, fadeOutOffset);
    constants->WorldRadius = m_fRadius;
    constants->MaxScreenSpaceRadius = m_fMaxScreenSpaceRadius;
    constants->Contrast = m_fContrast;
    constants->Intensity = m_fIntensity;
    constants->PositionBias = m_fPositionBias / 1000.0f;
    constants->MipLevelScale = m_fMipLevelScale;
    constants->DepthBlurScale = 1.0f / m_fDepthBlurThreshold;
  }

  // SSAO pass
  {
    wdGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(tempSSAOTexture));
    auto pCommandEncoder = renderViewContext.m_pRenderContext->BeginRenderingScope(pGALPass, renderViewContext, renderingSetup, "SSAO", renderViewContext.m_pCamera->IsStereoscopic());

    renderViewContext.m_pRenderContext->BindConstantBuffer("wdSSAOConstants", m_hSSAOConstantBuffer);
    renderViewContext.m_pRenderContext->BindShader(m_hSSAOShader);

    renderViewContext.m_pRenderContext->BindTexture2D("DepthTexture", pDevice->GetDefaultResourceView(pDepthInput->m_TextureHandle));
    renderViewContext.m_pRenderContext->BindTexture2D("LowResDepthTexture", pDevice->GetDefaultResourceView(hzbTexture));
    renderViewContext.m_pRenderContext->BindSamplerState("DepthSampler", m_hSSAOSamplerState);

    renderViewContext.m_pRenderContext->BindTexture2D("NoiseTexture", m_hNoiseTexture, wdResourceAcquireMode::BlockTillLoaded);

    renderViewContext.m_pRenderContext->BindNullMeshBuffer(wdGALPrimitiveTopology::Triangles, 1);

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }

  // Blur pass
  {
    wdGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
    auto pCommandEncoder = renderViewContext.m_pRenderContext->BeginRenderingScope(pGALPass, renderViewContext, renderingSetup, "Blur", renderViewContext.m_pCamera->IsStereoscopic());

    renderViewContext.m_pRenderContext->BindConstantBuffer("wdSSAOConstants", m_hSSAOConstantBuffer);
    renderViewContext.m_pRenderContext->BindShader(m_hBlurShader);

    renderViewContext.m_pRenderContext->BindTexture2D("SSAOTexture", pDevice->GetDefaultResourceView(tempSSAOTexture));

    renderViewContext.m_pRenderContext->BindNullMeshBuffer(wdGALPrimitiveTopology::Triangles, 1);

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }

  // Return temp targets
  if (!hzbTexture.IsInvalidated())
  {
    wdGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hzbTexture);
  }

  if (!tempSSAOTexture.IsInvalidated())
  {
    wdGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(tempSSAOTexture);
  }
}

void wdAOPass::ExecuteInactive(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs)
{
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr)
  {
    return;
  }

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  wdGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
  renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
  renderingSetup.m_ClearColor = wdColor::White;

  auto pCommandEncoder = wdRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, GetName());
}

void wdAOPass::SetFadeOutStart(float fStart)
{
  m_fFadeOutStart = wdMath::Clamp(fStart, 0.0f, m_fFadeOutEnd);
}

float wdAOPass::GetFadeOutStart() const
{
  return m_fFadeOutStart;
}

void wdAOPass::SetFadeOutEnd(float fEnd)
{
  if (m_fFadeOutEnd == fEnd)
    return;

  m_fFadeOutEnd = wdMath::Max(fEnd, m_fFadeOutStart);

  if (!m_hSSAOSamplerState.IsInvalidated())
  {
    wdGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSSAOSamplerState);
    m_hSSAOSamplerState.Invalidate();
  }
}

float wdAOPass::GetFadeOutEnd() const
{
  return m_fFadeOutEnd;
}

void wdAOPass::CreateSamplerState()
{
  if (m_hSSAOSamplerState.IsInvalidated())
  {
    wdGALSamplerStateCreationDescription desc;
    desc.m_MinFilter = wdGALTextureFilterMode::Point;
    desc.m_MagFilter = wdGALTextureFilterMode::Point;
    desc.m_MipFilter = wdGALTextureFilterMode::Point;
    desc.m_AddressU = wdImageAddressMode::ClampBorder;
    desc.m_AddressV = wdImageAddressMode::ClampBorder;
    desc.m_AddressW = wdImageAddressMode::ClampBorder;
    desc.m_BorderColor = wdColor::White * m_fFadeOutEnd;

    m_hSSAOSamplerState = wdGALDevice::GetDefaultDevice()->CreateSamplerState(desc);
  }
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_AOPass);
