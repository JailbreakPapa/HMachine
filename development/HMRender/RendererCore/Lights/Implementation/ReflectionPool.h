#pragma once

#include <Core/Graphics/AmbientCubeBasis.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/Declarations.h>

class wdGALTextureHandle;
class wdGALBufferHandle;
class wdView;
class wdWorld;
class wdComponent;
struct wdRenderWorldExtractionEvent;
struct wdRenderWorldRenderEvent;
struct wdMsgExtractRenderData;
struct wdReflectionProbeDesc;
class wdReflectionProbeRenderData;
using wdReflectionProbeId = wdGenericId<24, 8>;
class wdReflectionProbeComponentBase;
class wdSkyLightComponent;

class WD_RENDERERCORE_DLL wdReflectionPool
{
public:
  //Probes
  static wdReflectionProbeId RegisterReflectionProbe(const wdWorld* pWorld, const wdReflectionProbeDesc& desc, const wdReflectionProbeComponentBase* pComponent);
  static void DeregisterReflectionProbe(const wdWorld* pWorld, wdReflectionProbeId id);
  static void UpdateReflectionProbe(const wdWorld* pWorld, wdReflectionProbeId id, const wdReflectionProbeDesc& desc, const wdReflectionProbeComponentBase* pComponent);
  static void ExtractReflectionProbe(const wdComponent* pComponent, wdMsgExtractRenderData& ref_msg, wdReflectionProbeRenderData* pRenderData, const wdWorld* pWorld, wdReflectionProbeId id, float fPriority);

  // SkyLight
  static wdReflectionProbeId RegisterSkyLight(const wdWorld* pWorld, wdReflectionProbeDesc& ref_desc, const wdSkyLightComponent* pComponent);
  static void DeregisterSkyLight(const wdWorld* pWorld, wdReflectionProbeId id);
  static void UpdateSkyLight(const wdWorld* pWorld, wdReflectionProbeId id, const wdReflectionProbeDesc& desc, const wdSkyLightComponent* pComponent);


  static void SetConstantSkyIrradiance(const wdWorld* pWorld, const wdAmbientCube<wdColor>& skyIrradiance);
  static void ResetConstantSkyIrradiance(const wdWorld* pWorld);

  static wdUInt32 GetReflectionCubeMapSize();
  static wdGALTextureHandle GetReflectionSpecularTexture(wdUInt32 uiWorldIndex, wdEnum<wdCameraUsageHint> cameraUsageHint);
  static wdGALTextureHandle GetSkyIrradianceTexture();

private:
  WD_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, ReflectionPool);

  static void OnEngineStartup();
  static void OnEngineShutdown();

  static void OnExtractionEvent(const wdRenderWorldExtractionEvent& e);
  static void OnRenderEvent(const wdRenderWorldRenderEvent& e);

  struct Data;
  static Data* s_pData;
};
