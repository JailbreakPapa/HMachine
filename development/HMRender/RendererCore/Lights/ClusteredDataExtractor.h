#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/Extractor.h>

struct wdPerLightData;
struct wdPerDecalData;
struct wdPerReflectionProbeData;
struct wdPerClusterData;

class wdClusteredDataCPU : public wdRenderData
{
  WD_ADD_DYNAMIC_REFLECTION(wdClusteredDataCPU, wdRenderData);

public:
  wdClusteredDataCPU();
  ~wdClusteredDataCPU();

  enum
  {
    MAX_LIGHT_DATA = 1024,
    MAX_DECAL_DATA = 1024,
    MAX_REFLECTION_PROBE_DATA = 1024,
    MAX_ITEMS_PER_CLUSTER = 256
  };

  wdArrayPtr<wdPerLightData> m_LightData;
  wdArrayPtr<wdPerDecalData> m_DecalData;
  wdArrayPtr<wdPerReflectionProbeData> m_ReflectionProbeData;
  wdArrayPtr<wdPerClusterData> m_ClusterData;
  wdArrayPtr<wdUInt32> m_ClusterItemList;

  wdUInt32 m_uiSkyIrradianceIndex = 0;
  wdEnum<wdCameraUsageHint> m_cameraUsageHint = wdCameraUsageHint::Default;

  float m_fFogHeight = 0.0f;
  float m_fFogHeightFalloff = 0.0f;
  float m_fFogDensityAtCameraPos = 0.0f;
  float m_fFogDensity = 0.0f;
  float m_fFogInvSkyDistance = 0.0f;
  wdColor m_FogColor = wdColor::Black;
};

class WD_RENDERERCORE_DLL wdClusteredDataExtractor : public wdExtractor
{
  WD_ADD_DYNAMIC_REFLECTION(wdClusteredDataExtractor, wdExtractor);

public:
  wdClusteredDataExtractor(const char* szName = "ClusteredDataExtractor");
  ~wdClusteredDataExtractor();

  virtual void PostSortAndBatch(
    const wdView& view, const wdDynamicArray<const wdGameObject*>& visibleObjects, wdExtractedRenderData& ref_extractedRenderData) override;

private:
  void FillItemListAndClusterData(wdClusteredDataCPU* pData);

  template <wdUInt32 MaxData>
  struct TempCluster
  {
    WD_DECLARE_POD_TYPE();

    wdUInt32 m_BitMask[MaxData / 32];
  };

  wdDynamicArray<wdPerLightData, wdAlignedAllocatorWrapper> m_TempLightData;
  wdDynamicArray<wdPerDecalData, wdAlignedAllocatorWrapper> m_TempDecalData;
  wdDynamicArray<wdPerReflectionProbeData, wdAlignedAllocatorWrapper> m_TempReflectionProbeData;
  wdDynamicArray<TempCluster<wdClusteredDataCPU::MAX_LIGHT_DATA>> m_TempLightsClusters;
  wdDynamicArray<TempCluster<wdClusteredDataCPU::MAX_DECAL_DATA>> m_TempDecalsClusters;
  wdDynamicArray<TempCluster<wdClusteredDataCPU::MAX_REFLECTION_PROBE_DATA>> m_TempReflectionProbeClusters;
  wdDynamicArray<wdUInt32> m_TempClusterItemList;

  wdDynamicArray<wdSimdBSphere, wdAlignedAllocatorWrapper> m_ClusterBoundingSpheres;
};
