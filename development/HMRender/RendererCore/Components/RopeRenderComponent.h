#pragma once

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Meshes/SkinnedMeshComponent.h>
#include <memory>

struct wdMsgExtractRenderData;
struct wdMsgSetColor;
struct wdMsgSetMeshMaterial;
struct wdMsgRopePoseUpdated;
class wdShaderTransform;

using wdRopeRenderComponentManager = wdComponentManager<class wdRopeRenderComponent, wdBlockStorageType::Compact>;

class WD_RENDERERCORE_DLL wdRopeRenderComponent : public wdRenderComponent
{
  WD_DECLARE_COMPONENT_TYPE(wdRopeRenderComponent, wdRenderComponent, wdRopeRenderComponentManager);

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

protected:
  virtual wdResult GetLocalBounds(wdBoundingBoxSphere& bounds, bool& bAlwaysVisible, wdMsgUpdateLocalBounds& msg) override;
  void OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const; // [ msg handler ]

  //////////////////////////////////////////////////////////////////////////
  // wdRopeRenderComponent

public:
  wdRopeRenderComponent();
  ~wdRopeRenderComponent();

  wdColor m_Color = wdColor::White; // [ property ]

  void SetMaterialFile(const char* szFile); // [ property ]
  const char* GetMaterialFile() const;      // [ property ]

  void SetMaterial(const wdMaterialResourceHandle& hMaterial) { m_hMaterial = hMaterial; }
  wdMaterialResourceHandle GetMaterial() const { return m_hMaterial; }

  void SetThickness(float fThickness);                // [ property ]
  float GetThickness() const { return m_fThickness; } // [ property ]

  void SetDetail(wdUInt32 uiDetail);                // [ property ]
  wdUInt32 GetDetail() const { return m_uiDetail; } // [ property ]

  void SetSubdivide(bool bSubdivide);                // [ property ]
  bool GetSubdivide() const { return m_bSubdivide; } // [ property ]

  void SetUScale(float fUScale);                // [ property ]
  float GetUScale() const { return m_fUScale; } // [ property ]

  void OnMsgSetColor(wdMsgSetColor& ref_msg);               // [ msg handler ]
  void OnMsgSetMeshMaterial(wdMsgSetMeshMaterial& ref_msg); // [ msg handler ]

private:
  void OnRopePoseUpdated(wdMsgRopePoseUpdated& msg); // [ msg handler ]

  void GenerateRenderMesh(wdUInt32 uiNumRopePieces);

  void UpdateSkinningTransformBuffer(wdArrayPtr<const wdTransform> skinningTransforms);

  wdBoundingBoxSphere m_LocalBounds;

  wdSkinningState m_SkinningState;

  wdMeshResourceHandle m_hMesh;
  wdMaterialResourceHandle m_hMaterial;

  float m_fThickness = 0.05f;
  wdUInt32 m_uiDetail = 6;
  bool m_bSubdivide = false;

  float m_fUScale = 1.0f;
};
