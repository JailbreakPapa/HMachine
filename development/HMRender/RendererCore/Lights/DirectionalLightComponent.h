#pragma once

#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Textures/Texture2DResource.h>

using wdDirectionalLightComponentManager = wdComponentManager<class wdDirectionalLightComponent, wdBlockStorageType::Compact>;

/// \brief The render data object for directional lights.
class WD_RENDERERCORE_DLL wdDirectionalLightRenderData : public wdLightRenderData
{
  WD_ADD_DYNAMIC_REFLECTION(wdDirectionalLightRenderData, wdLightRenderData);

public:
};

/// \brief The standard directional light component.
/// This component represents directional lights.
class WD_RENDERERCORE_DLL wdDirectionalLightComponent : public wdLightComponent
{
  WD_DECLARE_COMPONENT_TYPE(wdDirectionalLightComponent, wdLightComponent, wdDirectionalLightComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // wdComponent

public:
  virtual void SerializeComponent(wdWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(wdWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // wdRenderComponent

public:
  virtual wdResult GetLocalBounds(wdBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, wdMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // wdDirectionalLightComponent

public:
  wdDirectionalLightComponent();
  ~wdDirectionalLightComponent();

  void SetNumCascades(wdUInt32 uiNumCascades); // [ property ]
  wdUInt32 GetNumCascades() const;             // [ property ]

  void SetMinShadowRange(float fMinShadowRange); // [ property ]
  float GetMinShadowRange() const;               // [ property ]

  void SetFadeOutStart(float fFadeOutStart); // [ property ]
  float GetFadeOutStart() const;             // [ property ]

  void SetSplitModeWeight(float fSplitModeWeight); // [ property ]
  float GetSplitModeWeight() const;                // [ property ]

  void SetNearPlaneOffset(float fNearPlaneOffset); // [ property ]
  float GetNearPlaneOffset() const;                // [ property ]

protected:
  void OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const;

  wdUInt32 m_uiNumCascades = 3;
  float m_fMinShadowRange = 50.0f;
  float m_fFadeOutStart = 0.8f;
  float m_fSplitModeWeight = 0.7f;
  float m_fNearPlaneOffset = 100.0f;
};
