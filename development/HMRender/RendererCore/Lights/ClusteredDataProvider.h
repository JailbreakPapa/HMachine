#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

struct WD_RENDERERCORE_DLL wdClusteredDataGPU
{
  WD_DISALLOW_COPY_AND_ASSIGN(wdClusteredDataGPU);

public:
  wdClusteredDataGPU();
  ~wdClusteredDataGPU();

  wdUInt32 m_uiSkyIrradianceIndex = 0;
  wdEnum<wdCameraUsageHint> m_cameraUsageHint = wdCameraUsageHint::Default;

  wdGALBufferHandle m_hLightDataBuffer;
  wdGALBufferHandle m_hDecalDataBuffer;
  wdGALBufferHandle m_hReflectionProbeDataBuffer;
  wdGALBufferHandle m_hClusterDataBuffer;
  wdGALBufferHandle m_hClusterItemBuffer;

  wdConstantBufferStorageHandle m_hConstantBuffer;

  wdGALSamplerStateHandle m_hShadowSampler;

  wdDecalAtlasResourceHandle m_hDecalAtlas;
  wdGALSamplerStateHandle m_hDecalAtlasSampler;

  void BindResources(wdRenderContext* pRenderContext);
};

class WD_RENDERERCORE_DLL wdClusteredDataProvider : public wdFrameDataProvider<wdClusteredDataGPU>
{
  WD_ADD_DYNAMIC_REFLECTION(wdClusteredDataProvider, wdFrameDataProviderBase);

public:
  wdClusteredDataProvider();
  ~wdClusteredDataProvider();

private:
  virtual void* UpdateData(const wdRenderViewContext& renderViewContext, const wdExtractedRenderData& extractedData) override;

  wdClusteredDataGPU m_Data;
};
