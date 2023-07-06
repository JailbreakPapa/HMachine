#pragma once

#include <RendererCore/Pipeline/Extractor.h>

class wdSimplifiedDataCPU : public wdRenderData
{
  WD_ADD_DYNAMIC_REFLECTION(wdSimplifiedDataCPU, wdRenderData);

public:
  wdSimplifiedDataCPU();
  ~wdSimplifiedDataCPU();

  wdUInt32 m_uiSkyIrradianceIndex = 0;
  wdEnum<wdCameraUsageHint> m_cameraUsageHint = wdCameraUsageHint::Default;
};

class WD_RENDERERCORE_DLL wdSimplifiedDataExtractor : public wdExtractor
{
  WD_ADD_DYNAMIC_REFLECTION(wdSimplifiedDataExtractor, wdExtractor);

public:
  wdSimplifiedDataExtractor(const char* szName = "SimplifiedDataExtractor");
  ~wdSimplifiedDataExtractor();

  virtual void PostSortAndBatch(
    const wdView& view, const wdDynamicArray<const wdGameObject*>& visibleObjects, wdExtractedRenderData& ref_extractedRenderData) override;
};
