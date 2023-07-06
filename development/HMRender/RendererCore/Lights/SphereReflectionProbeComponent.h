#pragma once

#include <RendererCore/Lights/ReflectionProbeComponentBase.h>

class WD_RENDERERCORE_DLL wdSphereReflectionProbeComponentManager final : public wdComponentManager<class wdSphereReflectionProbeComponent, wdBlockStorageType::Compact>
{
public:
  wdSphereReflectionProbeComponentManager(wdWorld* pWorld);
};

//////////////////////////////////////////////////////////////////////////
// wdSphereReflectionProbeComponent

/// \brief Sphere reflection probe component.
///
/// The generated reflection cube map is is projected to infinity. So parallax correction takes place.
class WD_RENDERERCORE_DLL wdSphereReflectionProbeComponent : public wdReflectionProbeComponentBase
{
  WD_DECLARE_COMPONENT_TYPE(wdSphereReflectionProbeComponent, wdReflectionProbeComponentBase, wdSphereReflectionProbeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // wdComponent

public:
  virtual void SerializeComponent(wdWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(wdWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // wdSphereReflectionProbeComponent

public:
  wdSphereReflectionProbeComponent();
  ~wdSphereReflectionProbeComponent();

  void SetRadius(float fRadius); // [ property ]
  float GetRadius() const;       // [ property ]

  void SetFalloff(float fFalloff);                // [ property ]
  float GetFalloff() const { return m_fFalloff; } // [ property ]

  void SetSphereProjection(bool bSphereProjection);                // [ property ]
  bool GetSphereProjection() const { return m_bSphereProjection; } // [ property ]

protected:
  //////////////////////////////////////////////////////////////////////////
  // Editor
  void OnObjectCreated(const wdAbstractObjectNode& node);

protected:
  void OnUpdateLocalBounds(wdMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const;
  void OnTransformChanged(wdMsgTransformChanged& msg);
  float m_fRadius = 5.0f;
  float m_fFalloff = 0.1f;
  bool m_bSphereProjection = true;
};
