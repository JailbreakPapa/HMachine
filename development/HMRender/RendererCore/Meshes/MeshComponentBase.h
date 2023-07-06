#pragma once

#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>

struct wdMsgSetColor;
struct wdInstanceData;

class WD_RENDERERCORE_DLL wdMeshRenderData : public wdRenderData
{
  WD_ADD_DYNAMIC_REFLECTION(wdMeshRenderData, wdRenderData);

public:
  virtual void FillBatchIdAndSortingKey();

  wdMeshResourceHandle m_hMesh;
  wdMaterialResourceHandle m_hMaterial;
  wdColor m_Color = wdColor::White;

  wdUInt32 m_uiSubMeshIndex : 30;
  wdUInt32 m_uiFlipWinding : 1;
  wdUInt32 m_uiUniformScale : 1;

  wdUInt32 m_uiUniqueID = 0;

protected:
  WD_FORCE_INLINE void FillBatchIdAndSortingKeyInternal(wdUInt32 uiAdditionalBatchData)
  {
    m_uiFlipWinding = m_GlobalTransform.ContainsNegativeScale() ? 1 : 0;
    m_uiUniformScale = m_GlobalTransform.ContainsUniformScale() ? 1 : 0;

    const wdUInt32 uiMeshIDHash = wdHashingUtils::StringHashTo32(m_hMesh.GetResourceIDHash());
    const wdUInt32 uiMaterialIDHash = m_hMaterial.IsValid() ? wdHashingUtils::StringHashTo32(m_hMaterial.GetResourceIDHash()) : 0;

    // Generate batch id from mesh, material and part index.
    wdUInt32 data[] = {uiMeshIDHash, uiMaterialIDHash, m_uiSubMeshIndex, m_uiFlipWinding, uiAdditionalBatchData};
    m_uiBatchId = wdHashingUtils::xxHash32(data, sizeof(data));

    // Sort by material and then by mesh
    m_uiSortingKey = (uiMaterialIDHash << 16) | ((uiMeshIDHash + m_uiSubMeshIndex) & 0xFFFE) | m_uiFlipWinding;
  }
};

struct WD_RENDERERCORE_DLL wdMsgSetMeshMaterial : public wdMessage
{
  WD_DECLARE_MESSAGE_TYPE(wdMsgSetMeshMaterial, wdMessage);

  void SetMaterialFile(const char* szFile);
  const char* GetMaterialFile() const;

  wdMaterialResourceHandle m_hMaterial;
  wdUInt32 m_uiMaterialSlot = 0xFFFFFFFFu;

  virtual void Serialize(wdStreamWriter& inout_stream) const override;
  virtual void Deserialize(wdStreamReader& inout_stream, wdUInt8 uiTypeVersion) override;
};

class WD_RENDERERCORE_DLL wdMeshComponentBase : public wdRenderComponent
{
  WD_DECLARE_ABSTRACT_COMPONENT_TYPE(wdMeshComponentBase, wdRenderComponent);

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
  // wdRenderMeshComponent

public:
  wdMeshComponentBase();
  ~wdMeshComponentBase();

  void SetMesh(const wdMeshResourceHandle& hMesh);
  WD_ALWAYS_INLINE const wdMeshResourceHandle& GetMesh() const { return m_hMesh; }

  void SetMaterial(wdUInt32 uiIndex, const wdMaterialResourceHandle& hMaterial);
  wdMaterialResourceHandle GetMaterial(wdUInt32 uiIndex) const;

  WD_ALWAYS_INLINE void SetRenderDataCategory(wdRenderData::Category category) { m_RenderDataCategory = category; }

  void SetMeshFile(const char* szFile); // [ property ]
  const char* GetMeshFile() const;      // [ property ]

  void SetColor(const wdColor& color); // [ property ]
  const wdColor& GetColor() const;     // [ property ]

  void OnMsgSetMeshMaterial(wdMsgSetMeshMaterial& ref_msg); // [ msg handler ]
  void OnMsgSetColor(wdMsgSetColor& ref_msg);               // [ msg handler ]

protected:
  virtual wdMeshRenderData* CreateRenderData() const;

  wdUInt32 Materials_GetCount() const;                          // [ property ]
  const char* Materials_GetValue(wdUInt32 uiIndex) const;       // [ property ]
  void Materials_SetValue(wdUInt32 uiIndex, const char* value); // [ property ]
  void Materials_Insert(wdUInt32 uiIndex, const char* value);   // [ property ]
  void Materials_Remove(wdUInt32 uiIndex);                      // [ property ]

  void OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const;

  wdRenderData::Category m_RenderDataCategory = wdInvalidRenderDataCategory;
  wdMeshResourceHandle m_hMesh;
  wdDynamicArray<wdMaterialResourceHandle> m_Materials;
  wdColor m_Color = wdColor::White;
};
