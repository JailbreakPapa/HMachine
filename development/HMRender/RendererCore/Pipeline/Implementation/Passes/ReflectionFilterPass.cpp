#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Pipeline/Passes/ReflectionFilterPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/ReflectionFilteredSpecularConstants.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/ReflectionIrradianceConstants.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdReflectionFilterPass, 1, wdRTTIDefaultAllocator<wdReflectionFilterPass>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("FilteredSpecular", m_PinFilteredSpecular),
    WD_MEMBER_PROPERTY("AvgLuminance", m_PinAvgLuminance),
    WD_MEMBER_PROPERTY("IrradianceData", m_PinIrradianceData),
    WD_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new wdDefaultValueAttribute(1.0f)),
    WD_MEMBER_PROPERTY("Saturation", m_fSaturation)->AddAttributes(new wdDefaultValueAttribute(1.0f)),
    WD_MEMBER_PROPERTY("SpecularOutputIndex", m_uiSpecularOutputIndex),
    WD_MEMBER_PROPERTY("IrradianceOutputIndex", m_uiIrradianceOutputIndex),
    WD_ACCESSOR_PROPERTY("InputCubemap", GetInputCubemap, SetInputCubemap)
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdReflectionFilterPass::wdReflectionFilterPass()
  : wdRenderPipelinePass("ReflectionFilterPass")

{
  {
    m_hFilteredSpecularConstantBuffer = wdRenderContext::CreateConstantBufferStorage<wdReflectionFilteredSpecularConstants>();
    m_hFilteredSpecularShader = wdResourceManager::LoadResource<wdShaderResource>("Shaders/Pipeline/ReflectionFilteredSpecular.wdShader");
    WD_ASSERT_DEV(m_hFilteredSpecularShader.IsValid(), "Could not load ReflectionFilteredSpecular shader!");

    m_hIrradianceConstantBuffer = wdRenderContext::CreateConstantBufferStorage<wdReflectionIrradianceConstants>();
    m_hIrradianceShader = wdResourceManager::LoadResource<wdShaderResource>("Shaders/Pipeline/ReflectionIrradiance.wdShader");
    WD_ASSERT_DEV(m_hIrradianceShader.IsValid(), "Could not load ReflectionIrradiance shader!");
  }
}

wdReflectionFilterPass::~wdReflectionFilterPass()
{
  wdRenderContext::DeleteConstantBufferStorage(m_hIrradianceConstantBuffer);
  m_hIrradianceConstantBuffer.Invalidate();
}

bool wdReflectionFilterPass::GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs)
{
  {
    wdGALTextureCreationDescription desc;
    desc.m_uiWidth = wdReflectionPool::GetReflectionCubeMapSize();
    desc.m_uiHeight = desc.m_uiWidth;
    desc.m_Format = wdGALResourceFormat::RGBAHalf;
    desc.m_Type = wdGALTextureType::TextureCube;
    desc.m_bAllowUAV = true;
    desc.m_uiMipLevelCount = wdMath::Log2i(desc.m_uiWidth) - 1;
    outputs[m_PinFilteredSpecular.m_uiOutputIndex] = desc;
  }

  return true;
}

void wdReflectionFilterPass::Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs)
{
  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  auto pInputCubemap = pDevice->GetTexture(m_hInputCubemap);
  if (pInputCubemap == nullptr)
  {
    return;
  }

  // We cannot allow the filter to work on fallback resources as the step will not be repeated for static cube maps. Thus, we force loading the shaders and disable async shader loading in this scope.
  wdResourceManager::ForceLoadResourceNow(m_hFilteredSpecularShader);
  wdResourceManager::ForceLoadResourceNow(m_hIrradianceShader);
  bool bAllowAsyncShaderLoading = renderViewContext.m_pRenderContext->GetAllowAsyncShaderLoading();
  renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(false);

  wdGALPass* pGALPass = pDevice->BeginPass(GetName());
  WD_SCOPE_EXIT(
    pDevice->EndPass(pGALPass);
    renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading));

  if (pInputCubemap->GetDescription().m_bAllowDynamicMipGeneration)
  {
    auto pCommandEncoder = wdRenderContext::BeginRenderingScope(pGALPass, renderViewContext, wdGALRenderingSetup(), "MipMaps");
    pCommandEncoder->GenerateMipMaps(pDevice->GetDefaultResourceView(m_hInputCubemap));
  }

  {
    auto pFilteredSpecularOutput = outputs[m_PinFilteredSpecular.m_uiOutputIndex];
    if (pFilteredSpecularOutput != nullptr && !pFilteredSpecularOutput->m_TextureHandle.IsInvalidated())
    {
      wdUInt32 uiNumMipMaps = pFilteredSpecularOutput->m_Desc.m_uiMipLevelCount;

      wdUInt32 uiWidth = pFilteredSpecularOutput->m_Desc.m_uiWidth;
      wdUInt32 uiHeight = pFilteredSpecularOutput->m_Desc.m_uiHeight;

      auto pCommandEncoder = wdRenderContext::BeginComputeScope(pGALPass, renderViewContext, "ReflectionFilter");
      renderViewContext.m_pRenderContext->BindTextureCube("InputCubemap", pDevice->GetDefaultResourceView(m_hInputCubemap));
      renderViewContext.m_pRenderContext->BindConstantBuffer("wdReflectionFilteredSpecularConstants", m_hFilteredSpecularConstantBuffer);
      renderViewContext.m_pRenderContext->BindShader(m_hFilteredSpecularShader);

      for (wdUInt32 uiMipMapIndex = 0; uiMipMapIndex < uiNumMipMaps; ++uiMipMapIndex)
      {
        wdGALUnorderedAccessViewHandle hFilterOutput;
        {
          wdGALUnorderedAccessViewCreationDescription desc;
          desc.m_hTexture = pFilteredSpecularOutput->m_TextureHandle;
          desc.m_uiMipLevelToUse = uiMipMapIndex;
          desc.m_uiFirstArraySlice = m_uiSpecularOutputIndex * 6;
          desc.m_uiArraySize = 6;
          hFilterOutput = pDevice->CreateUnorderedAccessView(desc);
        }
        renderViewContext.m_pRenderContext->BindUAV("ReflectionOutput", hFilterOutput);
        UpdateFilteredSpecularConstantBuffer(uiMipMapIndex, uiNumMipMaps);

        constexpr wdUInt32 uiThreadsX = 8;
        constexpr wdUInt32 uiThreadsY = 8;
        const wdUInt32 uiDispatchX = (uiWidth + uiThreadsX - 1) / uiThreadsX;
        const wdUInt32 uiDispatchY = (uiHeight + uiThreadsY - 1) / uiThreadsY;

        renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 6).IgnoreResult();

        uiWidth >>= 1;
        uiHeight >>= 1;
      }
    }
  }

  auto pIrradianceOutput = outputs[m_PinIrradianceData.m_uiOutputIndex];
  if (pIrradianceOutput != nullptr && !pIrradianceOutput->m_TextureHandle.IsInvalidated())
  {
    auto pCommandEncoder = wdRenderContext::BeginComputeScope(pGALPass, renderViewContext, "Irradiance");

    wdGALUnorderedAccessViewHandle hIrradianceOutput;
    {
      wdGALUnorderedAccessViewCreationDescription desc;
      desc.m_hTexture = pIrradianceOutput->m_TextureHandle;

      hIrradianceOutput = pDevice->CreateUnorderedAccessView(desc);
    }
    renderViewContext.m_pRenderContext->BindUAV("IrradianceOutput", hIrradianceOutput);

    renderViewContext.m_pRenderContext->BindTextureCube("InputCubemap", pDevice->GetDefaultResourceView(m_hInputCubemap));

    UpdateIrradianceConstantBuffer();

    renderViewContext.m_pRenderContext->BindConstantBuffer("wdReflectionIrradianceConstants", m_hIrradianceConstantBuffer);
    renderViewContext.m_pRenderContext->BindShader(m_hIrradianceShader);

    renderViewContext.m_pRenderContext->Dispatch(1).IgnoreResult();
  }
}

wdUInt32 wdReflectionFilterPass::GetInputCubemap() const
{
  return m_hInputCubemap.GetInternalID().m_Data;
}

void wdReflectionFilterPass::SetInputCubemap(wdUInt32 uiCubemapHandle)
{
  m_hInputCubemap = wdGALTextureHandle(wdGAL::wd18_14Id(uiCubemapHandle));
}

void wdReflectionFilterPass::UpdateFilteredSpecularConstantBuffer(wdUInt32 uiMipMapIndex, wdUInt32 uiNumMipMaps)
{
  auto constants = wdRenderContext::GetConstantBufferData<wdReflectionFilteredSpecularConstants>(m_hFilteredSpecularConstantBuffer);
  constants->MipLevel = uiMipMapIndex;
  constants->Intensity = m_fIntensity;
  constants->Saturation = m_fSaturation;
}

void wdReflectionFilterPass::UpdateIrradianceConstantBuffer()
{
  auto constants = wdRenderContext::GetConstantBufferData<wdReflectionIrradianceConstants>(m_hIrradianceConstantBuffer);
  constants->LodLevel = 6; // TODO: calculate from cubemap size and number of samples
  constants->Intensity = m_fIntensity;
  constants->Saturation = m_fSaturation;
  constants->OutputIndex = m_uiIrradianceOutputIndex;
}


WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_ReflectionFilterPass);
