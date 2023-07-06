#pragma once

#include <RendererCore/Declarations.h>

class wdDirectionalLightComponent;
class wdPointLightComponent;
class wdSpotLightComponent;
class wdGALTextureHandle;
class wdGALBufferHandle;
class wdView;
struct wdRenderWorldExtractionEvent;
struct wdRenderWorldRenderEvent;

class WD_RENDERERCORE_DLL wdShadowPool
{
public:
  static wdUInt32 AddDirectionalLight(const wdDirectionalLightComponent* pDirLight, const wdView* pReferenceView);
  static wdUInt32 AddPointLight(const wdPointLightComponent* pPointLight, float fScreenSpaceSize, const wdView* pReferenceView);
  static wdUInt32 AddSpotLight(const wdSpotLightComponent* pSpotLight, float fScreenSpaceSize, const wdView* pReferenceView);

  static wdGALTextureHandle GetShadowAtlasTexture();
  static wdGALBufferHandle GetShadowDataBuffer();

  /// \brief All exclude tags on this white list are copied from the reference views to the shadow views.
  static void AddExcludeTagToWhiteList(const wdTag& tag);

private:
  WD_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererCore, ShadowPool);

  static void OnEngineStartup();
  static void OnEngineShutdown();

  static void OnExtractionEvent(const wdRenderWorldExtractionEvent& e);
  static void OnRenderEvent(const wdRenderWorldRenderEvent& e);

  struct Data;
  static Data* s_pData;
};
