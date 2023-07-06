#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/TonemapPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/TonemapConstants.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdTonemapPass, 1, wdRTTIDefaultAllocator<wdTonemapPass>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("Color", m_PinColorInput),
    WD_MEMBER_PROPERTY("Bloom", m_PinBloomInput),
    WD_MEMBER_PROPERTY("Output", m_PinOutput),
    WD_ACCESSOR_PROPERTY("VignettingTexture", GetVignettingTextureFile, SetVignettingTextureFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    WD_MEMBER_PROPERTY("MoodColor", m_MoodColor)->AddAttributes(new wdDefaultValueAttribute(wdColor::Orange)),
    WD_MEMBER_PROPERTY("MoodStrength", m_fMoodStrength)->AddAttributes(new wdClampValueAttribute(0.0f, wdVariant())),
    WD_MEMBER_PROPERTY("Saturation", m_fSaturation)->AddAttributes(new wdClampValueAttribute(0.0f, 2.0f), new wdDefaultValueAttribute(1.0f)),
    WD_MEMBER_PROPERTY("Contrast", m_fContrast)->AddAttributes(new wdClampValueAttribute(0.0f, 1.0f)),
    WD_MEMBER_PROPERTY("LUT1Strength", m_fLut1Strength)->AddAttributes(new wdClampValueAttribute(0.0f, 1.0f)),
    WD_MEMBER_PROPERTY("LUT2Strength", m_fLut2Strength)->AddAttributes(new wdClampValueAttribute(0.0f, 1.0f)),
    WD_ACCESSOR_PROPERTY("LUT1", GetLUT1TextureFile, SetLUT1TextureFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Texture_3D")),
    WD_ACCESSOR_PROPERTY("LUT2", GetLUT2TextureFile, SetLUT2TextureFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Texture_3D")),
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdTonemapPass::wdTonemapPass()
  : wdRenderPipelinePass("TonemapPass", true)
{
  m_hVignettingTexture = wdResourceManager::LoadResource<wdTexture2DResource>("White.color");
  m_hNoiseTexture = wdResourceManager::LoadResource<wdTexture2DResource>("Textures/BlueNoise.dds");

  m_MoodColor = wdColor::Orange;
  m_fMoodStrength = 0.0f;
  m_fSaturation = 1.0f;
  m_fContrast = 1.0f;
  m_fLut1Strength = 1.0f;
  m_fLut2Strength = 0.0f;

  m_hShader = wdResourceManager::LoadResource<wdShaderResource>("Shaders/Pipeline/Tonemap.wdShader");
  WD_ASSERT_DEV(m_hShader.IsValid(), "Could not load tonemap shader!");

  m_hConstantBuffer = wdRenderContext::CreateConstantBufferStorage<wdTonemapConstants>();
}

wdTonemapPass::~wdTonemapPass()
{
  wdRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
  m_hConstantBuffer.Invalidate();
}

bool wdTonemapPass::GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs)
{
  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();
  const wdGALRenderTargets& renderTargets = view.GetActiveRenderTargets();

  // Color
  auto pColorInput = inputs[m_PinColorInput.m_uiInputIndex];
  if (pColorInput != nullptr)
  {
    if (const wdGALTexture* pTexture = pDevice->GetTexture(renderTargets.m_hRTs[0]))
    {
      const wdGALTextureCreationDescription& desc = pTexture->GetDescription();
      // if (desc.m_uiWidth != pColorInput->m_uiWidth || desc.m_uiHeight != pColorInput->m_uiHeight)
      //{
      //  wdLog::Error("Render target sizes don't match");
      //  return false;
      //}

      outputs[m_PinOutput.m_uiOutputIndex].SetAsRenderTarget(pColorInput->m_uiWidth, pColorInput->m_uiHeight, desc.m_Format);
      outputs[m_PinOutput.m_uiOutputIndex].m_uiArraySize = pColorInput->m_uiArraySize;
    }
    else
    {
      wdLog::Error("View '{0}' does not have a valid color target", view.GetName());
      return false;
    }
  }
  else
  {
    wdLog::Error("No input connected to tone map pass!");
    return false;
  }

  return true;
}

void wdTonemapPass::Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs)
{
  auto pColorInput = inputs[m_PinColorInput.m_uiInputIndex];
  auto pColorOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pColorInput == nullptr || pColorOutput == nullptr)
  {
    return;
  }

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  // Setup render target
  wdGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pColorOutput->m_TextureHandle));

  // Bind render target and viewport
  auto pCommandEncoder = wdRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, GetName(), renderViewContext.m_pCamera->IsStereoscopic());

  // Determine how many LUTs are active
  wdUInt32 numLUTs = 0;
  wdTexture3DResourceHandle luts[2] = {};
  float lutStrengths[2] = {};

  if (m_hLUT1.IsValid())
  {
    luts[numLUTs] = m_hLUT1;
    lutStrengths[numLUTs] = m_fLut1Strength;
    numLUTs++;
  }

  if (m_hLUT2.IsValid())
  {
    luts[numLUTs] = m_hLUT2;
    lutStrengths[numLUTs] = m_fLut2Strength;
    numLUTs++;
  }

  {
    wdTonemapConstants* constants = wdRenderContext::GetConstantBufferData<wdTonemapConstants>(m_hConstantBuffer);
    constants->AutoExposureParams.SetZero();
    constants->MoodColor = m_MoodColor;
    constants->MoodStrength = m_fMoodStrength;
    constants->Saturation = m_fSaturation;
    constants->Lut1Strength = lutStrengths[0];
    constants->Lut2Strength = lutStrengths[1];

    // Pre-calculate factors of a s-shaped polynomial-function
    const float m = (0.5f - 0.5f * m_fContrast) / (0.5f + 0.5f * m_fContrast);
    const float a = 2.0f * m - 2.0f;
    const float b = -3.0f * m + 3.0f;

    constants->ContrastParams = wdVec4(a, b, m, 0.0f);
  }

  wdGALResourceViewHandle hBloomTextureView;
  auto pBloomInput = inputs[m_PinBloomInput.m_uiInputIndex];
  if (pBloomInput != nullptr)
  {
    hBloomTextureView = pDevice->GetDefaultResourceView(pBloomInput->m_TextureHandle);
  }

  renderViewContext.m_pRenderContext->BindShader(m_hShader);
  renderViewContext.m_pRenderContext->BindConstantBuffer("wdTonemapConstants", m_hConstantBuffer);
  renderViewContext.m_pRenderContext->BindMeshBuffer(wdGALBufferHandle(), wdGALBufferHandle(), nullptr, wdGALPrimitiveTopology::Triangles, 1);
  renderViewContext.m_pRenderContext->BindTexture2D("VignettingTexture", m_hVignettingTexture, wdResourceAcquireMode::BlockTillLoaded);
  renderViewContext.m_pRenderContext->BindTexture2D("NoiseTexture", m_hNoiseTexture, wdResourceAcquireMode::BlockTillLoaded);
  renderViewContext.m_pRenderContext->BindTexture2D("SceneColorTexture", pDevice->GetDefaultResourceView(pColorInput->m_TextureHandle));
  renderViewContext.m_pRenderContext->BindTexture2D("BloomTexture", hBloomTextureView);
  renderViewContext.m_pRenderContext->BindTexture3D("Lut1Texture", luts[0]);
  renderViewContext.m_pRenderContext->BindTexture3D("Lut2Texture", luts[1]);

  wdTempHashedString sLUTModeValues[3] = {"LUT_MODE_NONE", "LUT_MODE_ONE", "LUT_MODE_TWO"};
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LUT_MODE", sLUTModeValues[numLUTs]);

  renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
}

void wdTonemapPass::SetVignettingTextureFile(const char* szFile)
{
  if (!wdStringUtils::IsNullOrEmpty(szFile))
  {
    m_hVignettingTexture = wdResourceManager::LoadResource<wdTexture2DResource>(szFile);
  }
}

const char* wdTonemapPass::GetVignettingTextureFile() const
{
  if (!m_hVignettingTexture.IsValid())
    return "";

  return m_hVignettingTexture.GetResourceID();
}


void wdTonemapPass::SetLUT1TextureFile(const char* szFile)
{
  if (!wdStringUtils::IsNullOrEmpty(szFile))
  {
    m_hLUT1 = wdResourceManager::LoadResource<wdTexture3DResource>(szFile);
  }
}

const char* wdTonemapPass::GetLUT1TextureFile() const
{
  if (!m_hLUT1.IsValid())
    return "";

  return m_hLUT1.GetResourceID();
}

void wdTonemapPass::SetLUT2TextureFile(const char* szFile)
{
  if (!wdStringUtils::IsNullOrEmpty(szFile))
  {
    m_hLUT2 = wdResourceManager::LoadResource<wdTexture3DResource>(szFile);
  }
}

const char* wdTonemapPass::GetLUT2TextureFile() const
{
  if (!m_hLUT2.IsValid())
    return "";

  return m_hLUT2.GetResourceID();
}


WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_TonemapPass);
