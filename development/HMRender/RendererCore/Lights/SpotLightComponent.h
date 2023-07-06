#pragma once

#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Textures/Texture2DResource.h>

using wdSpotLightComponentManager = wdComponentManager<class wdSpotLightComponent, wdBlockStorageType::Compact>;

/// \brief The render data object for spot lights.
class WD_RENDERERCORE_DLL wdSpotLightRenderData : public wdLightRenderData
{
  WD_ADD_DYNAMIC_REFLECTION(wdSpotLightRenderData, wdLightRenderData);

public:
  float m_fRange;
  wdAngle m_InnerSpotAngle;
  wdAngle m_OuterSpotAngle;
  wdTexture2DResourceHandle m_hProjectedTexture;
};

/// \brief The standard spot light component.
/// This component represents spot lights with various properties (e.g. a projected texture, range, spot angle, etc.)
class WD_RENDERERCORE_DLL wdSpotLightComponent : public wdLightComponent
{
  WD_DECLARE_COMPONENT_TYPE(wdSpotLightComponent, wdLightComponent, wdSpotLightComponentManager);

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
  // wdSpotLightComponent

public:
  wdSpotLightComponent();
  ~wdSpotLightComponent();

  void SetRange(float fRange); // [ property ]
  float GetRange() const;      // [ property ]

  float GetEffectiveRange() const;

  void SetInnerSpotAngle(wdAngle spotAngle);  // [ property ]
  wdAngle GetInnerSpotAngle() const;          // [ property ]

  void SetOuterSpotAngle(wdAngle spotAngle);  // [ property ]
  wdAngle GetOuterSpotAngle() const;          // [ property ]

  void SetProjectedTextureFile(const char* szFile); // [ property ]
  const char* GetProjectedTextureFile() const;      // [ property ]

  void SetProjectedTexture(const wdTexture2DResourceHandle& hProjectedTexture);
  const wdTexture2DResourceHandle& GetProjectedTexture() const;

protected:
  void OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const;
  wdBoundingSphere CalculateBoundingSphere(const wdTransform& t, float fRange) const;

  float m_fRange = 0.0f;
  float m_fEffectiveRange = 0.0f;

  wdAngle m_InnerSpotAngle = wdAngle::Degree(15.0f);
  wdAngle m_OuterSpotAngle = wdAngle::Degree(30.0f);

  wdTexture2DResourceHandle m_hProjectedTexture;
};

/// \brief A special visualizer attribute for spot lights
class WD_RENDERERCORE_DLL wdSpotLightVisualizerAttribute : public wdVisualizerAttribute
{
  WD_ADD_DYNAMIC_REFLECTION(wdSpotLightVisualizerAttribute, wdVisualizerAttribute);

public:
  wdSpotLightVisualizerAttribute();
  wdSpotLightVisualizerAttribute(
    const char* szAngleProperty, const char* szRangeProperty, const char* szIntensityProperty, const char* szColorProperty);

  const wdUntrackedString& GetAngleProperty() const { return m_sProperty1; }
  const wdUntrackedString& GetRangeProperty() const { return m_sProperty2; }
  const wdUntrackedString& GetIntensityProperty() const { return m_sProperty3; }
  const wdUntrackedString& GetColorProperty() const { return m_sProperty4; }
};
