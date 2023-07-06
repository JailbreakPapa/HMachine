#pragma once

#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

using wdMaterialResourceHandle = wdTypedResourceHandle<class wdMaterialResource>;

class WD_RENDERERCORE_DLL wdMeshResource : public wdResource
{
  WD_ADD_DYNAMIC_REFLECTION(wdMeshResource, wdResource);
  WD_RESOURCE_DECLARE_COMMON_CODE(wdMeshResource);
  WD_RESOURCE_DECLARE_CREATEABLE(wdMeshResource, wdMeshResourceDescriptor);

public:
  wdMeshResource();

  /// \brief Returns the array of sub-meshes in this mesh.
  wdArrayPtr<const wdMeshResourceDescriptor::SubMesh> GetSubMeshes() const { return m_SubMeshes; }

  /// \brief Returns the mesh buffer that is used by this resource.
  const wdMeshBufferResourceHandle& GetMeshBuffer() const { return m_hMeshBuffer; }

  /// \brief Returns the default materials for this mesh.
  wdArrayPtr<const wdMaterialResourceHandle> GetMaterials() const { return m_Materials; }

  /// \brief Returns the bounds of this mesh.
  const wdBoundingBoxSphere& GetBounds() const { return m_Bounds; }

  // TODO: clean up
  wdSkeletonResourceHandle m_hDefaultSkeleton;
  wdHashTable<wdHashedString, wdMeshResourceDescriptor::BoneData> m_Bones;
  float m_fMaxBoneVertexOffset = 0.0f; // the maximum distance between any vertex and its influencing bones, can be used for adjusting the bounding box of a pose

private:
  virtual wdResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual wdResourceLoadDesc UpdateContent(wdStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  wdDynamicArray<wdMeshResourceDescriptor::SubMesh> m_SubMeshes;
  wdMeshBufferResourceHandle m_hMeshBuffer;
  wdDynamicArray<wdMaterialResourceHandle> m_Materials;

  wdBoundingBoxSphere m_Bounds;

  static wdUInt32 s_uiMeshBufferNameSuffix;
};

using wdMeshResourceHandle = wdTypedResourceHandle<class wdMeshResource>;
