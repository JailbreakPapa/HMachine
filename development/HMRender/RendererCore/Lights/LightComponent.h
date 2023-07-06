#pragma once

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

struct wdMsgSetColor;

/// \brief Base class for light render data objects.
class WD_RENDERERCORE_DLL wdLightRenderData : public wdRenderData
{
  WD_ADD_DYNAMIC_REFLECTION(wdLightRenderData, wdRenderData);

public:
  void FillBatchIdAndSortingKey(float fScreenSpaceSize);

  wdColor m_LightColor;
  float m_fIntensity;
  wdUInt32 m_uiShadowDataOffset;
};

/// \brief Base class for all wd light components containing shared properties
class WD_RENDERERCORE_DLL wdLightComponent : public wdRenderComponent
{
  WD_DECLARE_ABSTRACT_COMPONENT_TYPE(wdLightComponent, wdRenderComponent);

  //////////////////////////////////////////////////////////////////////////
  // wdComponent

public:
  virtual void SerializeComponent(wdWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(wdWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // wdLightComponent

public:
  wdLightComponent();
  ~wdLightComponent();

  void SetLightColor(wdColorGammaUB lightColor); // [ property ]
  wdColorGammaUB GetLightColor() const;          // [ property ]

  void SetIntensity(float fIntensity); // [ property ]
  float GetIntensity() const;          // [ property ]

  void SetCastShadows(bool bCastShadows); // [ property ]
  bool GetCastShadows() const;            // [ property ]

  void SetPenumbraSize(float fPenumbraSize); // [ property ]
  float GetPenumbraSize() const;             // [ property ]

  void SetSlopeBias(float fShadowBias); // [ property ]
  float GetSlopeBias() const;           // [ property ]

  void SetConstantBias(float fShadowBias); // [ property ]
  float GetConstantBias() const;           // [ property ]

  void OnMsgSetColor(wdMsgSetColor& ref_msg); // [ msg handler ]

  static float CalculateEffectiveRange(float fRange, float fIntensity);
  static float CalculateScreenSpaceSize(const wdBoundingSphere& sphere, const wdCamera& camera);

protected:
  wdColorGammaUB m_LightColor = wdColor::White;
  float m_fIntensity = 10.0f;
  float m_fPenumbraSize = 0.1f;
  float m_fSlopeBias = 0.25f;
  float m_fConstantBias = 0.1f;
  bool m_bCastShadows = false;
};
