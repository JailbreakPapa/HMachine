#pragma once

#include <Foundation/IO/Stream.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>

class WD_RENDERERCORE_DLL wdMeshResourceDescriptor
{
public:
  struct SubMesh
  {
    WD_DECLARE_POD_TYPE();

    wdUInt32 m_uiPrimitiveCount;
    wdUInt32 m_uiFirstPrimitive;
    wdUInt32 m_uiMaterialIndex;

    wdBoundingBoxSphere m_Bounds;
  };

  struct Material
  {
    wdString m_sPath;
  };

  wdMeshResourceDescriptor();

  void Clear();

  wdMeshBufferResourceDescriptor& MeshBufferDesc();

  const wdMeshBufferResourceDescriptor& MeshBufferDesc() const;

  void UseExistingMeshBuffer(const wdMeshBufferResourceHandle& hBuffer);

  void AddSubMesh(wdUInt32 uiPrimitiveCount, wdUInt32 uiFirstPrimitive, wdUInt32 uiMaterialIndex);

  void SetMaterial(wdUInt32 uiMaterialIndex, const char* szPathToMaterial);

  void Save(wdStreamWriter& inout_stream);
  wdResult Save(const char* szFile);

  wdResult Load(wdStreamReader& inout_stream);
  wdResult Load(const char* szFile);

  const wdMeshBufferResourceHandle& GetExistingMeshBuffer() const;

  wdArrayPtr<const Material> GetMaterials() const;

  wdArrayPtr<const SubMesh> GetSubMeshes() const;

  /// \brief Merges all submeshes into just one.
  void CollapseSubMeshes();

  void ComputeBounds();
  const wdBoundingBoxSphere& GetBounds() const;
  void SetBounds(const wdBoundingBoxSphere& bounds) { m_Bounds = bounds; }

  struct BoneData
  {
    wdMat4 m_GlobalInverseBindPoseMatrix;
    wdUInt16 m_uiBoneIndex = wdInvalidJointIndex;

    wdResult Serialize(wdStreamWriter& inout_stream) const;
    wdResult Deserialize(wdStreamReader& inout_stream);
  };

  wdSkeletonResourceHandle m_hDefaultSkeleton;
  wdHashTable<wdHashedString, BoneData> m_Bones;
  float m_fMaxBoneVertexOffset = 0.0f; // the maximum distance between any vertex and its influencing bones, can be used for adjusting the bounding box of a pose

private:
  wdHybridArray<Material, 8> m_Materials;
  wdHybridArray<SubMesh, 8> m_SubMeshes;
  wdMeshBufferResourceDescriptor m_MeshBufferDescriptor;
  wdMeshBufferResourceHandle m_hMeshBuffer;
  wdBoundingBoxSphere m_Bounds;
};
