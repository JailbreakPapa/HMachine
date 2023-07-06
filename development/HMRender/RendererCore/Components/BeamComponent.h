#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/Messages/EventMessage.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

using wdBeamComponentManager = wdComponentManagerSimple<class wdBeamComponent, wdComponentUpdateType::Always>;

struct wdMsgExtractRenderData;
class wdGeometry;
class wdMeshResourceDescriptor;

/// \brief A beam component
class WD_RENDERERCORE_DLL wdBeamComponent : public wdRenderComponent
{
  WD_DECLARE_COMPONENT_TYPE(wdBeamComponent, wdRenderComponent, wdBeamComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // wdComponent

public:
  virtual void SerializeComponent(wdWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(wdWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // wdRenderComponent

public:
  virtual wdResult GetLocalBounds(wdBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, wdMsgUpdateLocalBounds& ref_msg) override;


  //////////////////////////////////////////////////////////////////////////
  // wdBeamComponent

public:
  wdBeamComponent();
  ~wdBeamComponent();

  void SetTargetObject(const char* szReference); // [ property ]

  void SetWidth(float fWidth); // [ property ]
  float GetWidth() const;      // [ property ]

  void SetUVUnitsPerWorldUnit(float fUVUnitsPerWorldUnit); // [ property ]
  float GetUVUnitsPerWorldUnit() const;                    // [ property ]

  void SetMaterialFile(const char* szFile); // [ property ]
  const char* GetMaterialFile() const;      // [ property ]

  wdMaterialResourceHandle GetMaterial() const;

  wdGameObjectHandle m_hTargetObject; // [ property ]

  wdColor m_Color; // [ property ]

protected:
  void Update();

  void OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const;

  float m_fWidth = 0.1f;               // [ property ]
  float m_fUVUnitsPerWorldUnit = 1.0f; // [ property ]

  wdMaterialResourceHandle m_hMaterial; // [ property ]

  const float m_fDistanceUpdateEpsilon = 0.02f;

  // State
  wdMeshResourceHandle m_hMesh;

  wdVec3 m_vLastOwnerPosition = wdVec3::ZeroVector();
  wdVec3 m_vLastTargetPosition = wdVec3::ZeroVector();

  void CreateMeshes();
  void BuildMeshResourceFromGeometry(wdGeometry& Geometry, wdMeshResourceDescriptor& MeshDesc) const;
  void ReinitMeshes();
  void Cleanup();

  const char* DummyGetter() const { return nullptr; }
};
