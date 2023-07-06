#pragma once

#include <Core/World/World.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Types/Bitflags.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeMapping.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeUpdater.h>
#include <RendererCore/Pipeline/View.h>

class wdSkyLightComponent;
class wdSphereReflectionProbeComponent;
class wdBoxReflectionProbeComponent;

static const wdUInt32 s_uiReflectionCubeMapSize = 128;
static const wdUInt32 s_uiNumReflectionProbeCubeMaps = 32;
static const float s_fDebugSphereRadius = 0.3f;

inline wdUInt32 GetMipLevels()
{
  return wdMath::Log2i(s_uiReflectionCubeMapSize) - 1; // only down to 4x4
}

//////////////////////////////////////////////////////////////////////////
/// wdReflectionPool::Data

struct wdReflectionPool::Data
{
  Data();
  ~Data();

  struct ProbeData
  {
    wdReflectionProbeDesc m_desc;
    wdTransform m_GlobalTransform;
    wdBitflags<wdProbeFlags> m_Flags;
    wdTextureCubeResourceHandle m_hCubeMap; // static data or empty for dynamic.
  };

  struct WorldReflectionData
  {
    WorldReflectionData()
      : m_mapping(s_uiNumReflectionProbeCubeMaps)
    {
    }
    WD_DISALLOW_COPY_AND_ASSIGN(WorldReflectionData);

    wdIdTable<wdReflectionProbeId, ProbeData> m_Probes;
    wdReflectionProbeId m_SkyLight; // SkyLight is always fixed at reflectionIndex 0.
    wdEventSubscriptionID m_mappingSubscriptionId = 0;
    wdReflectionProbeMapping m_mapping;
  };

  // WorldReflectionData management
  wdReflectionProbeId AddProbe(const wdWorld* pWorld, ProbeData&& probeData);
  wdReflectionPool::Data::WorldReflectionData& GetWorldData(const wdWorld* pWorld);
  void RemoveProbe(const wdWorld* pWorld, wdReflectionProbeId id);
  void UpdateProbeData(ProbeData& ref_probeData, const wdReflectionProbeDesc& desc, const wdReflectionProbeComponentBase* pComponent);
  bool UpdateSkyLightData(ProbeData& ref_probeData, const wdReflectionProbeDesc& desc, const wdSkyLightComponent* pComponent);
  void OnReflectionProbeMappingEvent(const wdUInt32 uiWorldIndex, const wdReflectionProbeMappingEvent& e);

  void PreExtraction();
  void PostExtraction();

  // Dynamic Update Queue (all worlds combined)
  wdHashSet<wdReflectionProbeRef> m_PendingDynamicUpdate;
  wdDeque<wdReflectionProbeRef> m_DynamicUpdateQueue;

  wdHashSet<wdReflectionProbeRef> m_ActiveDynamicUpdate;
  wdReflectionProbeUpdater m_ReflectionProbeUpdater;

  void CreateReflectionViewsAndResources();
  void CreateSkyIrradianceTexture();

  wdMutex m_Mutex;
  wdUInt64 m_uiWorldHasSkyLight = 0;
  wdUInt64 m_uiSkyIrradianceChanged = 0;
  wdHybridArray<wdUniquePtr<WorldReflectionData>, 2> m_WorldReflectionData;

  // GPU storage
  wdGALTextureHandle m_hFallbackReflectionSpecularTexture;
  wdGALTextureHandle m_hSkyIrradianceTexture;
  wdHybridArray<wdAmbientCube<wdColorLinear16f>, 64> m_SkyIrradianceStorage;

  // Debug data
  wdMeshResourceHandle m_hDebugSphere;
  wdHybridArray<wdMaterialResourceHandle, 6 * s_uiNumReflectionProbeCubeMaps> m_hDebugMaterial;
};
