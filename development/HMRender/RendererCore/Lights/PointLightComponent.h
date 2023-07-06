#pragma once

#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Textures/TextureCubeResource.h>

using wdPointLightComponentManager = wdComponentManager<class wdPointLightComponent, wdBlockStorageType::Compact>;

/// \brief The render data object for point lights.
class WD_RENDERERCORE_DLL wdPointLightRenderData : public wdLightRenderData
{
  WD_ADD_DYNAMIC_REFLECTION(wdPointLightRenderData, wdLightRenderData);

public:
  float m_fRange;
  wdTextureCubeResourceHandle m_hProjectedTexture;
};

/// \brief The standard point light component.
/// This component represents point lights with various properties (e.g. a projected cube map, range, etc.)
class WD_RENDERERCORE_DLL wdPointLightComponent : public wdLightComponent
{
  WD_DECLARE_COMPONENT_TYPE(wdPointLightComponent, wdLightComponent, wdPointLightComponentManager);

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
  // wdPointLightComponent

public:
  wdPointLightComponent();
  ~wdPointLightComponent();

  void SetRange(float fRange); // [ property ]
  float GetRange() const;      // [ property ]

  float GetEffectiveRange() const;

  void SetProjectedTextureFile(const char* szFile); // [ property ]
  const char* GetProjectedTextureFile() const;      // [ property ]

  void SetProjectedTexture(const wdTextureCubeResourceHandle& hProjectedTexture);
  const wdTextureCubeResourceHandle& GetProjectedTexture() const;

protected:
  void OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const;

  float m_fRange = 0.0f;
  float m_fEffectiveRange = 0.0f;

  wdTextureCubeResourceHandle m_hProjectedTexture;
};

/// \brief A special visualizer attribute for point lights
class WD_RENDERERCORE_DLL wdPointLightVisualizerAttribute : public wdVisualizerAttribute
{
  WD_ADD_DYNAMIC_REFLECTION(wdPointLightVisualizerAttribute, wdVisualizerAttribute);

public:
  wdPointLightVisualizerAttribute();
  wdPointLightVisualizerAttribute(const char* szRangeProperty, const char* szIntensityProperty, const char* szColorProperty);

  const wdUntrackedString& GetRangeProperty() const { return m_sProperty1; }
  const wdUntrackedString& GetIntensityProperty() const { return m_sProperty2; }
  const wdUntrackedString& GetColorProperty() const { return m_sProperty3; }
};
