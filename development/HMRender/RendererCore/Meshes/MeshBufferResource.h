#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

using wdMeshBufferResourceHandle = wdTypedResourceHandle<class wdMeshBufferResource>;
class wdGeometry;

struct WD_RENDERERCORE_DLL wdVertexStreamInfo : public wdHashableStruct<wdVertexStreamInfo>
{
  WD_DECLARE_POD_TYPE();

  wdGALVertexAttributeSemantic::Enum m_Semantic;
  wdUInt8 m_uiVertexBufferSlot = 0;
  wdGALResourceFormat::Enum m_Format;
  wdUInt16 m_uiOffset;      ///< at which byte offset the first element starts
  wdUInt16 m_uiElementSize; ///< the number of bytes for this element type (depends on the format); this is not the stride between elements!
};

struct WD_RENDERERCORE_DLL wdVertexDeclarationInfo
{
  void ComputeHash();

  wdHybridArray<wdVertexStreamInfo, 8> m_VertexStreams;
  wdUInt32 m_uiHash;
};


struct WD_RENDERERCORE_DLL wdMeshBufferResourceDescriptor
{
public:
  wdMeshBufferResourceDescriptor();
  ~wdMeshBufferResourceDescriptor();

  void Clear();

  /// \brief Use this function to add vertex streams to the mesh buffer. The return value is the index of the just added stream.
  wdUInt32 AddStream(wdGALVertexAttributeSemantic::Enum semantic, wdGALResourceFormat::Enum format);

  /// \brief Adds common vertex streams to the mesh buffer.
  ///
  /// The streams are added in this order (with the corresponding stream indices):
  /// * Position (index 0)
  /// * TexCoord0 (index 1)
  /// * Normal (index 2)
  /// * Tangent (index 3)
  void AddCommonStreams();

  /// \brief After all streams are added, call this to allocate the data for the streams. If uiNumPrimitives is 0, the mesh buffer will not
  /// use indexed rendering.
  void AllocateStreams(wdUInt32 uiNumVertices, wdGALPrimitiveTopology::Enum topology = wdGALPrimitiveTopology::Triangles, wdUInt32 uiNumPrimitives = 0, bool bZeroFill = false);

  /// \brief Creates streams and fills them with data from the wdGeometry. Only the geometry matching the given topology is used.
  ///  Streams that do not match any of the data inside the wdGeometry directly are skipped.
  void AllocateStreamsFromGeometry(const wdGeometry& geom, wdGALPrimitiveTopology::Enum topology = wdGALPrimitiveTopology::Triangles);

  /// \brief Gives read access to the allocated vertex data
  wdArrayPtr<const wdUInt8> GetVertexBufferData() const;

  /// \brief Gives read access to the allocated index data
  wdArrayPtr<const wdUInt8> GetIndexBufferData() const;

  /// \brief Allows write access to the allocated vertex data. This can be used for copying data fast into the array.
  wdDynamicArray<wdUInt8, wdAlignedAllocatorWrapper>& GetVertexBufferData();

  /// \brief Allows write access to the allocated index data. This can be used for copying data fast into the array.
  wdDynamicArray<wdUInt8, wdAlignedAllocatorWrapper>& GetIndexBufferData();

  /// \brief Slow, but convenient method to write one piece of vertex data at a time into the stream buffer.
  ///
  /// uiStream is the index of the data stream to write to.
  /// uiVertexIndex is the index of the vertex for which to write the data.
  /// data is the piece of data to write to the stream.
  template <typename TYPE>
  void SetVertexData(wdUInt32 uiStream, wdUInt32 uiVertexIndex, const TYPE& data)
  {
    reinterpret_cast<TYPE&>(m_VertexStreamData[m_uiVertexSize * uiVertexIndex + m_VertexDeclaration.m_VertexStreams[uiStream].m_uiOffset]) = data;
  }

  /// \brief Slow, but convenient method to access one piece of vertex data at a time into the stream buffer.
  ///
  /// uiStream is the index of the data stream to write to.
  /// uiVertexIndex is the index of the vertex for which to write the data.
  wdArrayPtr<wdUInt8> GetVertexData(wdUInt32 uiStream, wdUInt32 uiVertexIndex) { return m_VertexStreamData.GetArrayPtr().GetSubArray(m_uiVertexSize * uiVertexIndex + m_VertexDeclaration.m_VertexStreams[uiStream].m_uiOffset); }

  /// \brief Writes the vertex index for the given point into the index buffer.
  void SetPointIndices(wdUInt32 uiPoint, wdUInt32 uiVertex0);

  /// \brief Writes the two vertex indices for the given line into the index buffer.
  void SetLineIndices(wdUInt32 uiLine, wdUInt32 uiVertex0, wdUInt32 uiVertex1);

  /// \brief Writes the three vertex indices for the given triangle into the index buffer.
  void SetTriangleIndices(wdUInt32 uiTriangle, wdUInt32 uiVertex0, wdUInt32 uiVertex1, wdUInt32 uiVertex2);

  /// \brief Allows to read the stream info of the descriptor, which is filled out by AddStream()
  const wdVertexDeclarationInfo& GetVertexDeclaration() const { return m_VertexDeclaration; }

  /// \brief Returns the byte size of all the data for one vertex.
  wdUInt32 GetVertexDataSize() const { return m_uiVertexSize; }

  /// \brief Return the number of vertices, with which AllocateStreams() was called.
  wdUInt32 GetVertexCount() const { return m_uiVertexCount; }

  /// \brief Returns the number of primitives that the array holds.
  wdUInt32 GetPrimitiveCount() const;

  /// \brief Returns whether 16 or 32 Bit indices are to be used.
  bool Uses32BitIndices() const { return m_uiVertexCount > 0xFFFF; }

  /// \brief Returns whether an index buffer is available.
  bool HasIndexBuffer() const { return !m_IndexBufferData.IsEmpty(); }

  /// \brief Calculates the bounds using the data from the position stream
  wdBoundingBoxSphere ComputeBounds() const;

  /// \brief Returns the primitive topology
  wdGALPrimitiveTopology::Enum GetTopology() const { return m_Topology; }

  wdResult RecomputeNormals();

private:
  wdGALPrimitiveTopology::Enum m_Topology;
  wdUInt32 m_uiVertexSize;
  wdUInt32 m_uiVertexCount;
  wdVertexDeclarationInfo m_VertexDeclaration;
  wdDynamicArray<wdUInt8, wdAlignedAllocatorWrapper> m_VertexStreamData;
  wdDynamicArray<wdUInt8, wdAlignedAllocatorWrapper> m_IndexBufferData;
};

class WD_RENDERERCORE_DLL wdMeshBufferResource : public wdResource
{
  WD_ADD_DYNAMIC_REFLECTION(wdMeshBufferResource, wdResource);
  WD_RESOURCE_DECLARE_COMMON_CODE(wdMeshBufferResource);
  WD_RESOURCE_DECLARE_CREATEABLE(wdMeshBufferResource, wdMeshBufferResourceDescriptor);

public:
  wdMeshBufferResource()
    : wdResource(DoUpdate::OnAnyThread, 1)

  {
  }
  ~wdMeshBufferResource();

  WD_ALWAYS_INLINE wdUInt32 GetPrimitiveCount() const { return m_uiPrimitiveCount; }

  WD_ALWAYS_INLINE wdGALBufferHandle GetVertexBuffer() const { return m_hVertexBuffer; }

  WD_ALWAYS_INLINE wdGALBufferHandle GetIndexBuffer() const { return m_hIndexBuffer; }

  WD_ALWAYS_INLINE wdGALPrimitiveTopology::Enum GetTopology() const { return m_Topology; }

  /// \brief Returns the vertex declaration used by this mesh buffer.
  const wdVertexDeclarationInfo& GetVertexDeclaration() const { return m_VertexDeclaration; }

  /// \brief Returns the bounds of the mesh
  const wdBoundingBoxSphere& GetBounds() const { return m_Bounds; }

private:
  virtual wdResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual wdResourceLoadDesc UpdateContent(wdStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  wdBoundingBoxSphere m_Bounds;
  wdVertexDeclarationInfo m_VertexDeclaration;
  wdUInt32 m_uiPrimitiveCount = 0;
  wdGALBufferHandle m_hVertexBuffer;
  wdGALBufferHandle m_hIndexBuffer;
  wdGALPrimitiveTopology::Enum m_Topology = wdGALPrimitiveTopology::Enum::Default;
};
