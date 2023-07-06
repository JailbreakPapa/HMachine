#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

wdMeshResourceDescriptor::wdMeshResourceDescriptor()
{
  m_Bounds.SetInvalid();
}

void wdMeshResourceDescriptor::Clear()
{
  m_Bounds.SetInvalid();
  m_hMeshBuffer.Invalidate();
  m_Materials.Clear();
  m_MeshBufferDescriptor.Clear();
  m_SubMeshes.Clear();
}

wdMeshBufferResourceDescriptor& wdMeshResourceDescriptor::MeshBufferDesc()
{
  return m_MeshBufferDescriptor;
}

const wdMeshBufferResourceDescriptor& wdMeshResourceDescriptor::MeshBufferDesc() const
{
  return m_MeshBufferDescriptor;
}

void wdMeshResourceDescriptor::UseExistingMeshBuffer(const wdMeshBufferResourceHandle& hBuffer)
{
  m_hMeshBuffer = hBuffer;
}

const wdMeshBufferResourceHandle& wdMeshResourceDescriptor::GetExistingMeshBuffer() const
{
  return m_hMeshBuffer;
}

wdArrayPtr<const wdMeshResourceDescriptor::Material> wdMeshResourceDescriptor::GetMaterials() const
{
  return m_Materials;
}

wdArrayPtr<const wdMeshResourceDescriptor::SubMesh> wdMeshResourceDescriptor::GetSubMeshes() const
{
  return m_SubMeshes;
}

void wdMeshResourceDescriptor::CollapseSubMeshes()
{
  for (wdUInt32 idx = 1; idx < m_SubMeshes.GetCount(); ++idx)
  {
    m_SubMeshes[0].m_uiFirstPrimitive = wdMath::Min(m_SubMeshes[0].m_uiFirstPrimitive, m_SubMeshes[idx].m_uiFirstPrimitive);
    m_SubMeshes[0].m_uiPrimitiveCount += m_SubMeshes[idx].m_uiPrimitiveCount;

    if (m_SubMeshes[0].m_Bounds.IsValid() && m_SubMeshes[idx].m_Bounds.IsValid())
    {
      m_SubMeshes[0].m_Bounds.ExpandToInclude(m_SubMeshes[idx].m_Bounds);
    }
  }

  m_SubMeshes.SetCount(1);
  m_SubMeshes[0].m_uiMaterialIndex = 0;

  m_Materials.SetCount(1);
}

const wdBoundingBoxSphere& wdMeshResourceDescriptor::GetBounds() const
{
  return m_Bounds;
}

void wdMeshResourceDescriptor::AddSubMesh(wdUInt32 uiPrimitiveCount, wdUInt32 uiFirstPrimitive, wdUInt32 uiMaterialIndex)
{
  SubMesh p;
  p.m_uiFirstPrimitive = uiFirstPrimitive;
  p.m_uiPrimitiveCount = uiPrimitiveCount;
  p.m_uiMaterialIndex = uiMaterialIndex;
  p.m_Bounds.SetInvalid();

  m_SubMeshes.PushBack(p);
}

void wdMeshResourceDescriptor::SetMaterial(wdUInt32 uiMaterialIndex, const char* szPathToMaterial)
{
  m_Materials.EnsureCount(uiMaterialIndex + 1);

  m_Materials[uiMaterialIndex].m_sPath = szPathToMaterial;
}

wdResult wdMeshResourceDescriptor::Save(const char* szFile)
{
  WD_LOG_BLOCK("wdMeshResourceDescriptor::Save", szFile);

  wdFileWriter file;
  if (file.Open(szFile, 1024 * 1024).Failed())
  {
    wdLog::Error("Failed to open file '{0}'", szFile);
    return WD_FAILURE;
  }

  Save(file);
  return WD_SUCCESS;
}

void wdMeshResourceDescriptor::Save(wdStreamWriter& inout_stream)
{
  wdUInt8 uiVersion = 7;
  inout_stream << uiVersion;

  wdUInt8 uiCompressionMode = 0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  uiCompressionMode = 1;
  wdCompressedStreamWriterZstd compressor(&inout_stream, wdCompressedStreamWriterZstd::Compression::Average);
  wdChunkStreamWriter chunk(compressor);
#else
  wdChunkStreamWriter chunk(stream);
#endif

  inout_stream << uiCompressionMode;

  chunk.BeginStream(1);

  {
    chunk.BeginChunk("Materials", 1);

    // number of materials
    chunk << m_Materials.GetCount();

    // each material
    for (wdUInt32 idx = 0; idx < m_Materials.GetCount(); ++idx)
    {
      chunk << idx;                      // Material Index
      chunk << m_Materials[idx].m_sPath; // Material Path (data directory relative)
      /// \todo Material Path (relative to mesh file)
    }

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("SubMeshes", 1);

    // number of sub-meshes
    chunk << m_SubMeshes.GetCount();

    for (wdUInt32 idx = 0; idx < m_SubMeshes.GetCount(); ++idx)
    {
      chunk << idx;                                // Sub-Mesh index
      chunk << m_SubMeshes[idx].m_uiMaterialIndex; // The material to use
      chunk << m_SubMeshes[idx].m_uiFirstPrimitive;
      chunk << m_SubMeshes[idx].m_uiPrimitiveCount;
    }

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("MeshInfo", 4);

    // Number of vertices
    chunk << m_MeshBufferDescriptor.GetVertexCount();

    // Number of triangles
    chunk << m_MeshBufferDescriptor.GetPrimitiveCount();

    // Whether any index buffer is used
    chunk << m_MeshBufferDescriptor.HasIndexBuffer();

    // Whether the indices are 16 or 32 Bit, always false, if no index buffer is used
    chunk << (m_MeshBufferDescriptor.HasIndexBuffer() && m_MeshBufferDescriptor.Uses32BitIndices());

    // Number of vertex streams
    chunk << m_MeshBufferDescriptor.GetVertexDeclaration().m_VertexStreams.GetCount();

    // Version 3: Topology
    chunk << (wdUInt8)m_MeshBufferDescriptor.GetTopology();

    for (wdUInt32 idx = 0; idx < m_MeshBufferDescriptor.GetVertexDeclaration().m_VertexStreams.GetCount(); ++idx)
    {
      const auto& vs = m_MeshBufferDescriptor.GetVertexDeclaration().m_VertexStreams[idx];

      chunk << idx; // Vertex stream index
      chunk << (wdInt32)vs.m_Format;
      chunk << (wdInt32)vs.m_Semantic;
      chunk << vs.m_uiElementSize; // not needed, but can be used to check that memory layout has not changed
      chunk << vs.m_uiOffset;      // not needed, but can be used to check that memory layout has not changed
    }

    // Version 2
    if (!m_Bounds.IsValid())
    {
      ComputeBounds();
    }

    chunk << m_Bounds.m_vCenter;
    chunk << m_Bounds.m_vBoxHalfExtends;
    chunk << m_Bounds.m_fSphereRadius;
    // Version 4
    chunk << m_fMaxBoneVertexOffset;

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("VertexBuffer", 1);

    // size in bytes
    chunk << m_MeshBufferDescriptor.GetVertexBufferData().GetCount();

    if (!m_MeshBufferDescriptor.GetVertexBufferData().IsEmpty())
    {
      chunk.WriteBytes(m_MeshBufferDescriptor.GetVertexBufferData().GetData(), m_MeshBufferDescriptor.GetVertexBufferData().GetCount()).IgnoreResult();
    }

    chunk.EndChunk();
  }

  // always write the index buffer chunk, even if it is empty
  {
    chunk.BeginChunk("IndexBuffer", 1);

    // size in bytes
    chunk << m_MeshBufferDescriptor.GetIndexBufferData().GetCount();

    if (!m_MeshBufferDescriptor.GetIndexBufferData().IsEmpty())
    {
      chunk.WriteBytes(m_MeshBufferDescriptor.GetIndexBufferData().GetData(), m_MeshBufferDescriptor.GetIndexBufferData().GetCount()).IgnoreResult();
    }

    chunk.EndChunk();
  }

  if (!m_Bones.IsEmpty())
  {
    chunk.BeginChunk("BindPose", 1);

    chunk.WriteHashTable(m_Bones).IgnoreResult();

    chunk.EndChunk();
  }

  if (m_hDefaultSkeleton.IsValid())
  {
    chunk.BeginChunk("Skeleton", 1);

    chunk << m_hDefaultSkeleton;

    chunk.EndChunk();
  }

  chunk.EndStream();

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  compressor.FinishCompressedStream().IgnoreResult();

  wdLog::Dev("Compressed mesh data from {0} KB to {1} KB ({2}%%)", wdArgF((float)compressor.GetUncompressedSize() / 1024.0f, 1), wdArgF((float)compressor.GetCompressedSize() / 1024.0f, 1), wdArgF(100.0f * compressor.GetCompressedSize() / compressor.GetUncompressedSize(), 1));
#endif
}

wdResult wdMeshResourceDescriptor::Load(const char* szFile)
{
  WD_LOG_BLOCK("wdMeshResourceDescriptor::Load", szFile);

  wdFileReader file;
  if (file.Open(szFile, 1024 * 1024).Failed())
  {
    wdLog::Error("Failed to open file '{0}'", szFile);
    return WD_FAILURE;
  }

  // skip asset header
  wdAssetFileHeader assetHeader;
  WD_SUCCEED_OR_RETURN(assetHeader.Read(file));

  return Load(file);
}

wdResult wdMeshResourceDescriptor::Load(wdStreamReader& inout_stream)
{
  wdUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  // version 4 and below is broken
  if (uiVersion <= 4)
    return WD_FAILURE;

  wdUInt8 uiCompressionMode = 0;
  if (uiVersion >= 6)
  {
    inout_stream >> uiCompressionMode;
  }

  wdStreamReader* pCompressor = &inout_stream;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  wdCompressedStreamReaderZstd decompressorZstd;
#endif

  switch (uiCompressionMode)
  {
    case 0:
      break;

    case 1:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
      decompressorZstd.SetInputStream(&inout_stream);
      pCompressor = &decompressorZstd;
      break;
#else
      wdLog::Error("Mesh is compressed with zstandard, but support for this compressor is not compiled in.");
      return WD_FAILURE;
#endif

    default:
      wdLog::Error("Mesh is compressed with an unknown algorithm.");
      return WD_FAILURE;
  }

  wdChunkStreamReader chunk(*pCompressor);
  chunk.BeginStream();

  wdUInt32 count;
  bool bHasIndexBuffer = false;
  bool b32BitIndices = false;
  bool bCalculateBounds = true;

  while (chunk.GetCurrentChunk().m_bValid)
  {
    const auto& ci = chunk.GetCurrentChunk();

    if (ci.m_sChunkName == "Materials")
    {
      if (ci.m_uiChunkVersion != 1)
      {
        wdLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return WD_FAILURE;
      }

      // number of materials
      chunk >> count;
      m_Materials.SetCount(count);

      // each material
      for (wdUInt32 i = 0; i < m_Materials.GetCount(); ++i)
      {
        wdUInt32 idx;
        chunk >> idx;                      // Material Index
        chunk >> m_Materials[idx].m_sPath; // Material Path (data directory relative)
        /// \todo Material Path (relative to mesh file)
      }
    }

    if (chunk.GetCurrentChunk().m_sChunkName == "SubMeshes")
    {
      if (ci.m_uiChunkVersion != 1)
      {
        wdLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return WD_FAILURE;
      }

      // number of sub-meshes
      chunk >> count;
      m_SubMeshes.SetCount(count);

      for (wdUInt32 i = 0; i < m_SubMeshes.GetCount(); ++i)
      {
        wdUInt32 idx;
        chunk >> idx;                                // Sub-Mesh index
        chunk >> m_SubMeshes[idx].m_uiMaterialIndex; // The material to use
        chunk >> m_SubMeshes[idx].m_uiFirstPrimitive;
        chunk >> m_SubMeshes[idx].m_uiPrimitiveCount;

        /// \todo load from file
        m_SubMeshes[idx].m_Bounds.SetInvalid();
      }
    }

    if (ci.m_sChunkName == "MeshInfo")
    {
      if (ci.m_uiChunkVersion > 4)
      {
        wdLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return WD_FAILURE;
      }

      // Number of vertices
      wdUInt32 uiVertexCount = 0;
      chunk >> uiVertexCount;

      // Number of primitives
      wdUInt32 uiPrimitiveCount = 0;
      chunk >> uiPrimitiveCount;

      // Whether any index buffer is used
      chunk >> bHasIndexBuffer;

      // Whether the indices are 16 or 32 Bit, always false, if no index buffer is used
      chunk >> b32BitIndices;

      // Number of vertex streams
      wdUInt32 uiStreamCount = 0;
      chunk >> uiStreamCount;

      wdUInt8 uiTopology = wdGALPrimitiveTopology::Triangles;
      if (ci.m_uiChunkVersion >= 3)
      {
        chunk >> uiTopology;
      }

      for (wdUInt32 i = 0; i < uiStreamCount; ++i)
      {
        wdUInt32 idx;
        chunk >> idx; // Vertex stream index
        WD_ASSERT_DEV(idx == i, "Invalid stream index ({0}) in file (should be {1})", idx, i);

        wdInt32 iFormat, iSemantic;
        wdUInt16 uiElementSize, uiOffset;

        chunk >> iFormat;
        chunk >> iSemantic;
        chunk >> uiElementSize; // not needed, but can be used to check that memory layout has not changed
        chunk >> uiOffset;      // not needed, but can be used to check that memory layout has not changed

        if (uiVersion < 7)
        {
          // wdGALVertexAttributeSemantic got new elements inserted
          // need to adjust old file formats accordingly

          if (iSemantic >= wdGALVertexAttributeSemantic::Color2) // should be wdGALVertexAttributeSemantic::TexCoord0 instead
          {
            iSemantic += 6;
          }
        }

        m_MeshBufferDescriptor.AddStream((wdGALVertexAttributeSemantic::Enum)iSemantic, (wdGALResourceFormat::Enum)iFormat);
      }

      m_MeshBufferDescriptor.AllocateStreams(uiVertexCount, (wdGALPrimitiveTopology::Enum)uiTopology, uiPrimitiveCount);

      // Version 2
      if (ci.m_uiChunkVersion >= 2)
      {
        bCalculateBounds = false;
        chunk >> m_Bounds.m_vCenter;
        chunk >> m_Bounds.m_vBoxHalfExtends;
        chunk >> m_Bounds.m_fSphereRadius;
      }
      if (ci.m_uiChunkVersion >= 4)
      {
        chunk >> m_fMaxBoneVertexOffset;
      }
    }

    if (ci.m_sChunkName == "VertexBuffer")
    {
      if (ci.m_uiChunkVersion != 1)
      {
        wdLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return WD_FAILURE;
      }

      // size in bytes
      chunk >> count;
      m_MeshBufferDescriptor.GetVertexBufferData().SetCountUninitialized(count);

      if (!m_MeshBufferDescriptor.GetVertexBufferData().IsEmpty())
        chunk.ReadBytes(m_MeshBufferDescriptor.GetVertexBufferData().GetData(), m_MeshBufferDescriptor.GetVertexBufferData().GetCount());
    }

    if (ci.m_sChunkName == "IndexBuffer")
    {
      if (ci.m_uiChunkVersion != 1)
      {
        wdLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return WD_FAILURE;
      }

      // size in bytes
      chunk >> count;
      m_MeshBufferDescriptor.GetIndexBufferData().SetCountUninitialized(count);

      if (!m_MeshBufferDescriptor.GetIndexBufferData().IsEmpty())
        chunk.ReadBytes(m_MeshBufferDescriptor.GetIndexBufferData().GetData(), m_MeshBufferDescriptor.GetIndexBufferData().GetCount());
    }

    if (ci.m_sChunkName == "BindPose")
    {
      WD_SUCCEED_OR_RETURN(chunk.ReadHashTable(m_Bones));
    }

    if (ci.m_sChunkName == "Skeleton")
    {
      chunk >> m_hDefaultSkeleton;
    }

    chunk.NextChunk();
  }

  chunk.EndStream();

  if (bCalculateBounds)
  {
    ComputeBounds();

    auto b = m_Bounds;
    wdLog::Info("Calculated Bounds: {0} | {1} | {2} - {3} | {4} | {5}", wdArgF(b.m_vCenter.x, 2), wdArgF(b.m_vCenter.y, 2), wdArgF(b.m_vCenter.z, 2), wdArgF(b.m_vBoxHalfExtends.x, 2), wdArgF(b.m_vBoxHalfExtends.y, 2), wdArgF(b.m_vBoxHalfExtends.z, 2));
  }

  return WD_SUCCESS;
}

void wdMeshResourceDescriptor::ComputeBounds()
{
  if (m_hMeshBuffer.IsValid())
  {
    wdResourceLock<wdMeshBufferResource> pMeshBuffer(m_hMeshBuffer, wdResourceAcquireMode::AllowLoadingFallback);
    m_Bounds = pMeshBuffer->GetBounds();
  }
  else
  {
    m_Bounds = m_MeshBufferDescriptor.ComputeBounds();
  }
}

wdResult wdMeshResourceDescriptor::BoneData::Serialize(wdStreamWriter& inout_stream) const
{
  inout_stream << m_GlobalInverseBindPoseMatrix;
  inout_stream << m_uiBoneIndex;

  return WD_SUCCESS;
}

wdResult wdMeshResourceDescriptor::BoneData::Deserialize(wdStreamReader& inout_stream)
{
  inout_stream >> m_GlobalInverseBindPoseMatrix;
  inout_stream >> m_uiBoneIndex;

  return WD_SUCCESS;
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshResourceDescriptor);
