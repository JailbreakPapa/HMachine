#pragma once

#include <Core/ResourceManager/Resource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using wdDynamicMeshBufferResourceHandle = wdTypedResourceHandle<class wdDynamicMeshBufferResource>;

struct wdDynamicMeshBufferResourceDescriptor
{
  wdGALPrimitiveTopology::Enum m_Topology = wdGALPrimitiveTopology::Triangles;
  wdGALIndexType::Enum m_IndexType = wdGALIndexType::UInt;
  wdUInt32 m_uiMaxPrimitives = 0;
  wdUInt32 m_uiMaxVertices = 0;
  bool m_bColorStream = false;
};

struct WD_RENDERERCORE_DLL wdDynamicMeshVertex
{
  WD_DECLARE_POD_TYPE();

  wdVec3 m_vPosition;
  wdVec2 m_vTexCoord;
  wdVec3 m_vEncodedNormal;
  wdVec4 m_vEncodedTangent;
  //wdColorLinearUB m_Color;

  WD_ALWAYS_INLINE void EncodeNormal(const wdVec3& vNormal)
  {
    // store in [0; 1] range
    m_vEncodedNormal = vNormal * 0.5f + wdVec3(0.5f);

    // this is the same
    //wdMeshBufferUtils::EncodeNormal(normal, wdByteArrayPtr(reinterpret_cast<wdUInt8*>(&m_vEncodedNormal), sizeof(wdVec3)), wdMeshNormalPrecision::_32Bit).IgnoreResult();
  }

  WD_ALWAYS_INLINE void EncodeTangent(const wdVec3& vTangent, float fBitangentSign)
  {
    // store in [0; 1] range
    m_vEncodedTangent.x = vTangent.x * 0.5f + 0.5f;
    m_vEncodedTangent.y = vTangent.y * 0.5f + 0.5f;
    m_vEncodedTangent.z = vTangent.z * 0.5f + 0.5f;
    m_vEncodedTangent.w = fBitangentSign < 0.0f ? 0.0f : 1.0f;

    // this is the same
    //wdMeshBufferUtils::EncodeTangent(tangent, bitangentSign, wdByteArrayPtr(reinterpret_cast<wdUInt8*>(&m_vEncodedTangent), sizeof(wdVec4)), wdMeshNormalPrecision::_32Bit).IgnoreResult();
  }
};

class WD_RENDERERCORE_DLL wdDynamicMeshBufferResource : public wdResource
{
  WD_ADD_DYNAMIC_REFLECTION(wdDynamicMeshBufferResource, wdResource);
  WD_RESOURCE_DECLARE_COMMON_CODE(wdDynamicMeshBufferResource);
  WD_RESOURCE_DECLARE_CREATEABLE(wdDynamicMeshBufferResource, wdDynamicMeshBufferResourceDescriptor);

public:
  wdDynamicMeshBufferResource();
  ~wdDynamicMeshBufferResource();

  WD_ALWAYS_INLINE const wdDynamicMeshBufferResourceDescriptor& GetDescriptor() const { return m_Descriptor; }
  WD_ALWAYS_INLINE wdGALBufferHandle GetVertexBuffer() const { return m_hVertexBuffer; }
  WD_ALWAYS_INLINE wdGALBufferHandle GetIndexBuffer() const { return m_hIndexBuffer; }
  WD_ALWAYS_INLINE wdGALBufferHandle GetColorBuffer() const { return m_hColorBuffer; }

  /// \brief Grants write access to the vertex data, and flags the data as 'dirty'.
  wdArrayPtr<wdDynamicMeshVertex> AccessVertexData()
  {
    m_bAccessedVB = true;
    return m_VertexData;
  }

  /// \brief Grants write access to the 16 bit index data, and flags the data as 'dirty'.
  ///
  /// Accessing this data is only valid, if the buffer was created with 16 bit indices.
  wdArrayPtr<wdUInt16> AccessIndex16Data()
  {
    m_bAccessedIB = true;
    return m_Index16Data;
  }

  /// \brief Grants write access to the 32 bit index data, and flags the data as 'dirty'.
  ///
  /// Accessing this data is only valid, if the buffer was created with 32 bit indices.
  wdArrayPtr<wdUInt32> AccessIndex32Data()
  {
    m_bAccessedIB = true;
    return m_Index32Data;
  }

  /// \brief Grants write access to the color data, and flags the data as 'dirty'.
  ///
  /// Accessing this data is only valid, if creation of the color buffer was enabled.
  wdArrayPtr<wdColorLinearUB> AccessColorData()
  {
    m_bAccessedCB = true;
    return m_ColorData;
  }

  const wdVertexDeclarationInfo& GetVertexDeclaration() const { return m_VertexDeclaration; }

  /// \brief Uploads the current vertex and index data to the GPU.
  ///
  /// If all values are set to default, the entire data is uploaded.
  /// If \a uiNumVertices or \a uiNumIndices is set to the max value, all vertices or indices (after their start offset) are uploaded.
  ///
  /// In all other cases, the number of elements to upload must be within valid bounds.
  ///
  /// This function can be used to only upload a subset of the modified data.
  ///
  /// Note that this function doesn't do anything, if the vertex or index data wasn't recently accessed through AccessVertexData(), AccessIndex16Data() or AccessIndex32Data(). So if you want to upload multiple pieces of the data to the GPU, you have to call these functions in between to flag the uploaded data as out-of-date.
  void UpdateGpuBuffer(wdGALCommandEncoder* pGALCommandEncoder, wdUInt32 uiFirstVertex = 0, wdUInt32 uiNumVertices = wdMath::MaxValue<wdUInt32>(), wdUInt32 uiFirstIndex = 0, wdUInt32 uiNumIndices = wdMath::MaxValue<wdUInt32>(), wdGALUpdateMode::Enum mode = wdGALUpdateMode::Discard);

private:
  virtual wdResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual wdResourceLoadDesc UpdateContent(wdStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  bool m_bAccessedVB = false;
  bool m_bAccessedIB = false;
  bool m_bAccessedCB = false;

  wdGALBufferHandle m_hVertexBuffer;
  wdGALBufferHandle m_hIndexBuffer;
  wdGALBufferHandle m_hColorBuffer;
  wdDynamicMeshBufferResourceDescriptor m_Descriptor;

  wdVertexDeclarationInfo m_VertexDeclaration;
  wdDynamicArray<wdDynamicMeshVertex, wdAlignedAllocatorWrapper> m_VertexData;
  wdDynamicArray<wdUInt16, wdAlignedAllocatorWrapper> m_Index16Data;
  wdDynamicArray<wdUInt32, wdAlignedAllocatorWrapper> m_Index32Data;
  wdDynamicArray<wdColorLinearUB, wdAlignedAllocatorWrapper> m_ColorData;
};
