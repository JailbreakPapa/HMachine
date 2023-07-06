#pragma once

#include <RendererCore/Lights/ReflectionProbeComponentBase.h>

class WD_RENDERERCORE_DLL wdBoxReflectionProbeComponentManager final : public wdComponentManager<class wdBoxReflectionProbeComponent, wdBlockStorageType::Compact>
{
public:
  wdBoxReflectionProbeComponentManager(wdWorld* pWorld);
};

/// \brief Box reflection probe component.
///
/// The generated reflection cube map is projected on a box defined by this component's extents. The influence volume can be smaller than the projection which is defined by a scale and shift parameter. Each side of the influence volume has a separate falloff parameter to smoothly blend the probe into others.
class WD_RENDERERCORE_DLL wdBoxReflectionProbeComponent : public wdReflectionProbeComponentBase
{
  WD_DECLARE_COMPONENT_TYPE(wdBoxReflectionProbeComponent, wdReflectionProbeComponentBase, wdBoxReflectionProbeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // wdComponent

public:
  virtual void SerializeComponent(wdWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(wdWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // wdBoxReflectionProbeComponent

public:
  wdBoxReflectionProbeComponent();
  ~wdBoxReflectionProbeComponent();

  const wdVec3& GetExtents() const;       // [ property ]
  void SetExtents(const wdVec3& vExtents); // [ property ]

  const wdVec3& GetInfluenceScale() const;               // [ property ]
  void SetInfluenceScale(const wdVec3& vInfluenceScale); // [ property ]
  const wdVec3& GetInfluenceShift() const;               // [ property ]
  void SetInfluenceShift(const wdVec3& vInfluenceShift); // [ property ]

  void SetPositiveFalloff(const wdVec3& vFalloff);                        // [ property ]
  const wdVec3& GetPositiveFalloff() const { return m_vPositiveFalloff; } // [ property ]
  void SetNegativeFalloff(const wdVec3& vFalloff);                        // [ property ]
  const wdVec3& GetNegativeFalloff() const { return m_vNegativeFalloff; } // [ property ]

  void SetBoxProjection(bool bBoxProjection);                // [ property ]
  bool GetBoxProjection() const { return m_bBoxProjection; } // [ property ]

protected:
  //////////////////////////////////////////////////////////////////////////
  // Editor
  void OnObjectCreated(const wdAbstractObjectNode& node);

protected:
  void OnUpdateLocalBounds(wdMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const;
  void OnTransformChanged(wdMsgTransformChanged& msg);

protected:
  wdVec3 m_vExtents = wdVec3(5.0f);
  wdVec3 m_vInfluenceScale = wdVec3(1.0f);
  wdVec3 m_vInfluenceShift = wdVec3(0.0f);
  wdVec3 m_vPositiveFalloff = wdVec3(0.1f, 0.1f, 0.0f);
  wdVec3 m_vNegativeFalloff = wdVec3(0.1f, 0.1f, 0.0f);
  bool m_bBoxProjection = true;
};

/// \brief A special visualizer attribute for box reflection probes
class WD_RENDERERCORE_DLL wdBoxReflectionProbeVisualizerAttribute : public wdVisualizerAttribute
{
  WD_ADD_DYNAMIC_REFLECTION(wdBoxReflectionProbeVisualizerAttribute, wdVisualizerAttribute);

public:
  wdBoxReflectionProbeVisualizerAttribute();

  wdBoxReflectionProbeVisualizerAttribute(const char* szExtentsProperty, const char* szInfluenceScaleProperty, const char* szInfluenceShiftProperty);

  const wdUntrackedString& GetExtentsProperty() const { return m_sProperty1; }
  const wdUntrackedString& GetInfluenceScaleProperty() const { return m_sProperty2; }
  const wdUntrackedString& GetInfluenceShiftProperty() const { return m_sProperty3; }
};
