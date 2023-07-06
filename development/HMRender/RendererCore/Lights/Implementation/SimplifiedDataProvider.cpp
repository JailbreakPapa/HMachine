#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/SimplifiedDataExtractor.h>
#include <RendererCore/Lights/SimplifiedDataProvider.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightDataSimplified.h>
WD_DEFINE_AS_POD_TYPE(wdSimplifiedDataConstants);

wdSimplifiedDataGPU::wdSimplifiedDataGPU()
{
  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();
  m_hConstantBuffer = wdRenderContext::CreateConstantBufferStorage<wdSimplifiedDataConstants>();
}

wdSimplifiedDataGPU::~wdSimplifiedDataGPU()
{
  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();
  wdRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void wdSimplifiedDataGPU::BindResources(wdRenderContext* pRenderContext)
{
  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  auto hReflectionSpecularTextureView = pDevice->GetDefaultResourceView(wdReflectionPool::GetReflectionSpecularTexture(m_uiSkyIrradianceIndex, m_cameraUsageHint));
  auto hSkyIrradianceTextureView = pDevice->GetDefaultResourceView(wdReflectionPool::GetSkyIrradianceTexture());

  pRenderContext->BindTextureCube("ReflectionSpecularTexture", hReflectionSpecularTextureView);
  pRenderContext->BindTexture2D("SkyIrradianceTexture", hSkyIrradianceTextureView);

  pRenderContext->BindConstantBuffer("wdSimplifiedDataConstants", m_hConstantBuffer);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdSimplifiedDataProvider, 1, wdRTTIDefaultAllocator<wdSimplifiedDataProvider>)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdSimplifiedDataProvider::wdSimplifiedDataProvider() = default;

wdSimplifiedDataProvider::~wdSimplifiedDataProvider() = default;

void* wdSimplifiedDataProvider::UpdateData(const wdRenderViewContext& renderViewContext, const wdExtractedRenderData& extractedData)
{
  wdGALCommandEncoder* pGALCommandEncoder = renderViewContext.m_pRenderContext->GetRenderCommandEncoder();

  WD_PROFILE_AND_MARKER(pGALCommandEncoder, "Update Clustered Data");

  if (auto pData = extractedData.GetFrameData<wdSimplifiedDataCPU>())
  {
    m_Data.m_uiSkyIrradianceIndex = pData->m_uiSkyIrradianceIndex;
    m_Data.m_cameraUsageHint = pData->m_cameraUsageHint;

    // Update Constants
    const wdRectFloat& viewport = renderViewContext.m_pViewData->m_ViewPortRect;

    wdSimplifiedDataConstants* pConstants =
      renderViewContext.m_pRenderContext->GetConstantBufferData<wdSimplifiedDataConstants>(m_Data.m_hConstantBuffer);

    pConstants->SkyIrradianceIndex = pData->m_uiSkyIrradianceIndex;
  }

  return &m_Data;
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SimplifiedDataProvider);
