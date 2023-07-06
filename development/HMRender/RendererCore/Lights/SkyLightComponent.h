#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Textures/TextureCubeResource.h>

struct wdMsgUpdateLocalBounds;
struct wdMsgExtractRenderData;
struct wdMsgTransformChanged;

using wdSkyLightComponentManager = wdSettingsComponentManager<class wdSkyLightComponent>;

class WD_RENDERERCORE_DLL wdSkyLightComponent : public wdSettingsComponent
{
  WD_DECLARE_COMPONENT_TYPE(wdSkyLightComponent, wdSettingsComponent, wdSkyLightComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // wdComponent

public:
  virtual void SerializeComponent(wdWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(wdWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // wdSkyLightComponent

public:
  wdSkyLightComponent();
  ~wdSkyLightComponent();

  void SetReflectionProbeMode(wdEnum<wdReflectionProbeMode> mode); // [ property ]
  wdEnum<wdReflectionProbeMode> GetReflectionProbeMode() const;    // [ property ]

  void SetIntensity(float fIntensity); // [ property ]
  float GetIntensity() const;          // [ property ]

  void SetSaturation(float fSaturation); // [ property ]
  float GetSaturation() const;           // [ property ]

  const wdTagSet& GetIncludeTags() const;   // [ property ]
  void InsertIncludeTag(const char* szTag); // [ property ]
  void RemoveIncludeTag(const char* szTag); // [ property ]

  const wdTagSet& GetExcludeTags() const;   // [ property ]
  void InsertExcludeTag(const char* szTag); // [ property ]
  void RemoveExcludeTag(const char* szTag); // [ property ]

  void SetShowDebugInfo(bool bShowDebugInfo); // [ property ]
  bool GetShowDebugInfo() const;              // [ property ]

  void SetShowMipMaps(bool bShowMipMaps); // [ property ]
  bool GetShowMipMaps() const;            // [ property ]

  void SetCubeMapFile(const char* szFile); // [ property ]
  const char* GetCubeMapFile() const;      // [ property ]
  wdTextureCubeResourceHandle GetCubeMap() const
  {
    return m_hCubeMap;
  }

  float GetNearPlane() const { return m_Desc.m_fNearPlane; } // [ property ]
  void SetNearPlane(float fNearPlane);                       // [ property ]

  float GetFarPlane() const { return m_Desc.m_fFarPlane; } // [ property ]
  void SetFarPlane(float fFarPlane);                       // [ property ]

protected:
  void OnUpdateLocalBounds(wdMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const;
  void OnTransformChanged(wdMsgTransformChanged& msg);

  wdReflectionProbeDesc m_Desc;
  wdTextureCubeResourceHandle m_hCubeMap;

  wdReflectionProbeId m_Id;

  mutable bool m_bStatesDirty = true;
};
