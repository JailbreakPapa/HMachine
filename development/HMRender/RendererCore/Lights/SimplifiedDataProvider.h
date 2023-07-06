#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

struct WD_RENDERERCORE_DLL wdSimplifiedDataGPU
{
  WD_DISALLOW_COPY_AND_ASSIGN(wdSimplifiedDataGPU);

public:
  wdSimplifiedDataGPU();
  ~wdSimplifiedDataGPU();

  wdUInt32 m_uiSkyIrradianceIndex = 0;
  wdEnum<wdCameraUsageHint> m_cameraUsageHint = wdCameraUsageHint::Default;
  wdConstantBufferStorageHandle m_hConstantBuffer;

  void BindResources(wdRenderContext* pRenderContext);
};

class WD_RENDERERCORE_DLL wdSimplifiedDataProvider : public wdFrameDataProvider<wdSimplifiedDataGPU>
{
  WD_ADD_DYNAMIC_REFLECTION(wdSimplifiedDataProvider, wdFrameDataProviderBase);

public:
  wdSimplifiedDataProvider();
  ~wdSimplifiedDataProvider();

private:
  virtual void* UpdateData(const wdRenderViewContext& renderViewContext, const wdExtractedRenderData& extractedData) override;

  wdSimplifiedDataGPU m_Data;
};
