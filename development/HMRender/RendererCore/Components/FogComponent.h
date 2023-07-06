#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <RendererCore/Pipeline/RenderData.h>

struct wdMsgUpdateLocalBounds;

using wdFogComponentManager = wdSettingsComponentManager<class wdFogComponent>;

/// \brief The render data object for ambient light.
class WD_RENDERERCORE_DLL wdFogRenderData : public wdRenderData
{
  WD_ADD_DYNAMIC_REFLECTION(wdFogRenderData, wdRenderData);

public:
  wdColor m_Color;
  float m_fDensity;
  float m_fHeightFalloff;
  float m_fInvSkyDistance;
};

class WD_RENDERERCORE_DLL wdFogComponent : public wdSettingsComponent
{
  WD_DECLARE_COMPONENT_TYPE(wdFogComponent, wdSettingsComponent, wdFogComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // wdComponent

public:
  virtual void SerializeComponent(wdWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(wdWorldReader& inout_stream) override;

protected:
  virtual void Deinitialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // wdFogComponent

public:
  wdFogComponent();
  ~wdFogComponent();

  void SetColor(wdColor color); // [ property ]
  wdColor GetColor() const;     // [ property ]

  void SetDensity(float fDensity); // [ property ]
  float GetDensity() const;        // [ property ]

  void SetHeightFalloff(float fHeightFalloff); // [ property ]
  float GetHeightFalloff() const;              // [ property ]

  void SetModulateWithSkyColor(bool bModulate); // [ property ]
  bool GetModulateWithSkyColor() const;         // [ property ]

  void SetSkyDistance(float fDistance); // [ property ]
  float GetSkyDistance() const;         // [ property ]

protected:
  void OnUpdateLocalBounds(wdMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const;

  wdColor m_Color = wdColor(0.2f, 0.2f, 0.3f);
  float m_fDensity = 1.0f;
  float m_fHeightFalloff = 10.0f;
  float m_fSkyDistance = 1000.0f;
  bool m_bModulateWithSkyColor = false;
};
