#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <RendererCore/RendererCoreDLL.h>

struct wdMsgUpdateLocalBounds;

using wdAmbientLightComponentManager = wdSettingsComponentManager<class wdAmbientLightComponent>;

class WD_RENDERERCORE_DLL wdAmbientLightComponent : public wdSettingsComponent
{
  WD_DECLARE_COMPONENT_TYPE(wdAmbientLightComponent, wdSettingsComponent, wdAmbientLightComponentManager);

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
  // wdAmbientLightComponent

public:
  wdAmbientLightComponent();
  ~wdAmbientLightComponent();

  void SetTopColor(wdColorGammaUB color); // [ property ]
  wdColorGammaUB GetTopColor() const;     // [ property ]

  void SetBottomColor(wdColorGammaUB color); // [ property ]
  wdColorGammaUB GetBottomColor() const;     // [ property ]

  void SetIntensity(float fIntensity); // [ property ]
  float GetIntensity() const;          // [ property ]

private:
  void OnUpdateLocalBounds(wdMsgUpdateLocalBounds& msg);
  void UpdateSkyIrradiance();

  wdColorGammaUB m_TopColor = wdColor(0.2f, 0.2f, 0.3f);
  wdColorGammaUB m_BottomColor = wdColor(0.1f, 0.1f, 0.15f);
  float m_fIntensity = 1.0f;
};
