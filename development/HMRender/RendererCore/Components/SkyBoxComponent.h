#pragma once

#include <Core/World/World.h>
#include <RendererCore/Meshes/MeshComponent.h>

using wdSkyBoxComponentManager = wdComponentManager<class wdSkyBoxComponent, wdBlockStorageType::Compact>;
using wdTextureCubeResourceHandle = wdTypedResourceHandle<class wdTextureCubeResource>;

class WD_RENDERERCORE_DLL wdSkyBoxComponent : public wdRenderComponent
{
  WD_DECLARE_COMPONENT_TYPE(wdSkyBoxComponent, wdRenderComponent, wdSkyBoxComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // wdComponent

public:
  virtual void SerializeComponent(wdWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(wdWorldReader& inout_stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnActivated() override;


  //////////////////////////////////////////////////////////////////////////
  // wdRenderComponent

public:
  virtual wdResult GetLocalBounds(wdBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, wdMsgUpdateLocalBounds& ref_msg) override;


  //////////////////////////////////////////////////////////////////////////
  // wdSkyBoxComponent

public:
  wdSkyBoxComponent();
  ~wdSkyBoxComponent();

  void SetExposureBias(float fExposureBias);                // [ property ]
  float GetExposureBias() const { return m_fExposureBias; } // [ property ]

  void SetInverseTonemap(bool bInverseTonemap);                // [ property ]
  bool GetInverseTonemap() const { return m_bInverseTonemap; } // [ property ]

  void SetUseFog(bool bUseFog);                // [ property ]
  bool GetUseFog() const { return m_bUseFog; } // [ property ]

  void SetVirtualDistance(float fVirtualDistance);                // [ property ]
  float GetVirtualDistance() const { return m_fVirtualDistance; } // [ property ]

  void SetCubeMapFile(const char* szFile); // [ property ]
  const char* GetCubeMapFile() const;      // [ property ]

  void SetCubeMap(const wdTextureCubeResourceHandle& hCubeMap);
  const wdTextureCubeResourceHandle& GetCubeMap() const;

private:
  void OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const;
  void UpdateMaterials();

  float m_fExposureBias = 0.0f;
  float m_fVirtualDistance = 1000.0f;
  bool m_bInverseTonemap = false;
  bool m_bUseFog = true;

  wdTextureCubeResourceHandle m_hCubeMap;

  wdMeshResourceHandle m_hMesh;
  wdMaterialResourceHandle m_hCubeMapMaterial;
};
