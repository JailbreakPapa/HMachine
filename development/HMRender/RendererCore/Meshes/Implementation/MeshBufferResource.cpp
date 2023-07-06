#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdMeshBufferResource, 1, wdRTTIDefaultAllocator<wdMeshBufferResource>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_RESOURCE_IMPLEMENT_COMMON_CODE(wdMeshBufferResource);
// clang-format on

wdMeshBufferResourceDescriptor::wdMeshBufferResourceDescriptor()
{
  m_Topology = wdGALPrimitiveTopology::Triangles;
  m_uiVertexSize = 0;
  m_uiVertexCount = 0;
}

wdMeshBufferResourceDescriptor::~wdMeshBufferResourceDescriptor() = default;

void wdMeshBufferResourceDescriptor::Clear()
{
  m_Topology = wdGALPrimitiveTopology::Triangles;
  m_uiVertexSize = 0;
  m_uiVertexCount = 0;
  m_VertexDeclaration.m_uiHash = 0;
  m_VertexDeclaration.m_VertexStreams.Clear();
  m_VertexStreamData.Clear();
  m_IndexBufferData.Clear();
}

wdArrayPtr<const wdUInt8> wdMeshBufferResourceDescriptor::GetVertexBufferData() const
{
  return m_VertexStreamData.GetArrayPtr();
}

wdArrayPtr<const wdUInt8> wdMeshBufferResourceDescriptor::GetIndexBufferData() const
{
  return m_IndexBufferData.GetArrayPtr();
}

wdDynamicArray<wdUInt8, wdAlignedAllocatorWrapper>& wdMeshBufferResourceDescriptor::GetVertexBufferData()
{
  WD_ASSERT_DEV(!m_VertexStreamData.IsEmpty(), "The vertex data must be allocated first");
  return m_VertexStreamData;
}

wdDynamicArray<wdUInt8, wdAlignedAllocatorWrapper>& wdMeshBufferResourceDescriptor::GetIndexBufferData()
{
  WD_ASSERT_DEV(!m_IndexBufferData.IsEmpty(), "The index data must be allocated first");
  return m_IndexBufferData;
}

wdUInt32 wdMeshBufferResourceDescriptor::AddStream(wdGALVertexAttributeSemantic::Enum semantic, wdGALResourceFormat::Enum format)
{
  WD_ASSERT_DEV(m_VertexStreamData.IsEmpty(), "This function can only be called before 'AllocateStreams' is called");

  for (wdUInt32 i = 0; i < m_VertexDeclaration.m_VertexStreams.GetCount(); ++i)
  {
    WD_ASSERT_DEV(m_VertexDeclaration.m_VertexStreams[i].m_Semantic != semantic, "The given semantic {0} is already used by a previous stream", semantic);
  }

  wdVertexStreamInfo si;

  si.m_Semantic = semantic;
  si.m_Format = format;
  si.m_uiOffset = 0;
  si.m_uiElementSize = static_cast<wdUInt16>(wdGALResourceFormat::GetBitsPerElement(format) / 8);
  m_uiVertexSize += si.m_uiElementSize;

  WD_ASSERT_DEV(si.m_uiElementSize > 0, "Invalid Element Size. Format not supported?");

  if (!m_VertexDeclaration.m_VertexStreams.IsEmpty())
    si.m_uiOffset = m_VertexDeclaration.m_VertexStreams.PeekBack().m_uiOffset + m_VertexDeclaration.m_VertexStreams.PeekBack().m_uiElementSize;

  m_VertexDeclaration.m_VertexStreams.PushBack(si);

  return m_VertexDeclaration.m_VertexStreams.GetCount() - 1;
}

void wdMeshBufferResourceDescriptor::AddCommonStreams()
{
  AddStream(wdGALVertexAttributeSemantic::Position, wdGALResourceFormat::XYZFloat);
  AddStream(wdGALVertexAttributeSemantic::TexCoord0, wdMeshTexCoordPrecision::ToResourceFormat(wdMeshTexCoordPrecision::Default));
  AddStream(wdGALVertexAttributeSemantic::Normal, wdMeshNormalPrecision::ToResourceFormatNormal(wdMeshNormalPrecision::Default));
  AddStream(wdGALVertexAttributeSemantic::Tangent, wdMeshNormalPrecision::ToResourceFormatTangent(wdMeshNormalPrecision::Default));
}

void wdMeshBufferResourceDescriptor::AllocateStreams(wdUInt32 uiNumVertices, wdGALPrimitiveTopology::Enum topology, wdUInt32 uiNumPrimitives, bool bZeroFill /*= false*/)
{
  WD_ASSERT_DEV(!m_VertexDeclaration.m_VertexStreams.IsEmpty(), "You have to add streams via 'AddStream' before calling this function");

  m_Topology = topology;
  m_uiVertexCount = uiNumVertices;
  const wdUInt32 uiVertexStreamSize = m_uiVertexSize * uiNumVertices;

  if (bZeroFill)
  {
    m_VertexStreamData.SetCount(uiVertexStreamSize);
  }
  else
  {
    m_VertexStreamData.SetCountUninitialized(uiVertexStreamSize);
  }

  if (uiNumPrimitives > 0)
  {
    // use an index buffer at all
    wdUInt32 uiIndexBufferSize = uiNumPrimitives * wdGALPrimitiveTopology::VerticesPerPrimitive(topology);

    if (Uses32BitIndices())
    {
      uiIndexBufferSize *= sizeof(wdUInt32);
    }
    else
    {
      uiIndexBufferSize *= sizeof(wdUInt16);
    }

    m_IndexBufferData.SetCountUninitialized(uiIndexBufferSize);
  }
}

void wdMeshBufferResourceDescriptor::AllocateStreamsFromGeometry(const wdGeometry& geom, wdGALPrimitiveTopology::Enum topology)
{
  wdLogBlock _("Allocate Streams From Geometry");

  // Index Buffer Generation
  wdDynamicArray<wdUInt32> Indices;

  if (topology == wdGALPrimitiveTopology::Points)
  {
    // Leaving indices empty disables indexed rendering.
  }
  else if (topology == wdGALPrimitiveTopology::Lines)
  {
    Indices.Reserve(geom.GetLines().GetCount() * 2);

    for (wdUInt32 p = 0; p < geom.GetLines().GetCount(); ++p)
    {
      Indices.PushBack(geom.GetLines()[p].m_uiStartVertex);
      Indices.PushBack(geom.GetLines()[p].m_uiEndVertex);
    }
  }
  else if (topology == wdGALPrimitiveTopology::Triangles)
  {
    Indices.Reserve(geom.GetPolygons().GetCount() * 6);

    for (wdUInt32 p = 0; p < geom.GetPolygons().GetCount(); ++p)
    {
      for (wdUInt32 v = 0; v < geom.GetPolygons()[p].m_Vertices.GetCount() - 2; ++v)
      {
        Indices.PushBack(geom.GetPolygons()[p].m_Vertices[0]);
        Indices.PushBack(geom.GetPolygons()[p].m_Vertices[v + 1]);
        Indices.PushBack(geom.GetPolygons()[p].m_Vertices[v + 2]);
      }
    }
  }
  AllocateStreams(geom.GetVertices().GetCount(), topology, Indices.GetCount() / (topology + 1));

  // Fill vertex buffer.
  for (wdUInt32 s = 0; s < m_VertexDeclaration.m_VertexStreams.GetCount(); ++s)
  {
    const wdVertexStreamInfo& si = m_VertexDeclaration.m_VertexStreams[s];
    switch (si.m_Semantic)
    {
      case wdGALVertexAttributeSemantic::Position:
      {
        if (si.m_Format == wdGALResourceFormat::XYZFloat)
        {
          for (wdUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            SetVertexData<wdVec3>(s, v, geom.GetVertices()[v].m_vPosition);
          }
        }
        else
        {
          wdLog::Error("Position stream with format '{0}' is not supported.", (int)si.m_Format);
        }
      }
      break;

      case wdGALVertexAttributeSemantic::Normal:
      {
        for (wdUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
        {
          if (wdMeshBufferUtils::EncodeNormal(geom.GetVertices()[v].m_vNormal, GetVertexData(s, v), si.m_Format).Failed())
          {
            wdLog::Error("Normal stream with format '{0}' is not supported.", (int)si.m_Format);
            break;
          }
        }
      }
      break;

      case wdGALVertexAttributeSemantic::Tangent:
      {
        for (wdUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
        {
          if (wdMeshBufferUtils::EncodeTangent(geom.GetVertices()[v].m_vTangent, geom.GetVertices()[v].m_fBiTangentSign, GetVertexData(s, v), si.m_Format).Failed())
          {
            wdLog::Error("Tangent stream with format '{0}' is not supported.", (int)si.m_Format);
            break;
          }
        }
      }
      break;

      case wdGALVertexAttributeSemantic::Color0:
      case wdGALVertexAttributeSemantic::Color1:
      {
        if (si.m_Format == wdGALResourceFormat::RGBAUByteNormalized)
        {
          for (wdUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            SetVertexData<wdColorLinearUB>(s, v, geom.GetVertices()[v].m_Color);
          }
        }
        else
        {
          wdLog::Error("Color stream with format '{0}' is not supported.", (int)si.m_Format);
        }
      }
      break;

      case wdGALVertexAttributeSemantic::TexCoord0:
      case wdGALVertexAttributeSemantic::TexCoord1:
      {
        for (wdUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
        {
          if (wdMeshBufferUtils::EncodeTexCoord(geom.GetVertices()[v].m_vTexCoord, GetVertexData(s, v), si.m_Format).Failed())
          {
            wdLog::Error("UV stream with format '{0}' is not supported.", (int)si.m_Format);
            break;
          }
        }
      }
      break;

      case wdGALVertexAttributeSemantic::BoneIndices0:
      {
        // if a bone index array is available, move the custom index into it

        if (si.m_Format == wdGALResourceFormat::RGBAUByte)
        {
          for (wdUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            wdVec4U16 boneIndices = geom.GetVertices()[v].m_BoneIndices;
            wdVec4U8 storage(static_cast<wdUInt8>(boneIndices.x), static_cast<wdUInt8>(boneIndices.y), static_cast<wdUInt8>(boneIndices.z), static_cast<wdUInt8>(boneIndices.w));
            SetVertexData<wdVec4U8>(s, v, storage);
          }
        }
        else if (si.m_Format == wdGALResourceFormat::RGBAUShort)
        {
          for (wdUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            SetVertexData<wdVec4U16>(s, v, geom.GetVertices()[v].m_BoneIndices);
          }
        }
      }
      break;

      case wdGALVertexAttributeSemantic::BoneWeights0:
      {
        // if a bone weight array is available, set it to fully use the first bone

        if (si.m_Format == wdGALResourceFormat::RGBAUByteNormalized)
        {
          for (wdUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            SetVertexData<wdColorLinearUB>(s, v, geom.GetVertices()[v].m_BoneWeights);
          }
        }

        if (si.m_Format == wdGALResourceFormat::XYZWFloat)
        {
          for (wdUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            SetVertexData<wdVec4>(s, v, wdColor(geom.GetVertices()[v].m_BoneWeights).GetAsVec4());
          }
        }
      }
      break;

      case wdGALVertexAttributeSemantic::BoneIndices1:
      case wdGALVertexAttributeSemantic::BoneWeights1:
        // Don't error out for these semantics as they may be used by the user (e.g. breakable mesh construction)
        break;

      default:
      {
        wdLog::Error("Streams semantic '{0}' is not supported.", (int)si.m_Semantic);
      }
      break;
    }
  }

  // Fill index buffer.
  if (topology == wdGALPrimitiveTopology::Points)
  {
    for (wdUInt32 t = 0; t < Indices.GetCount(); t += 1)
    {
      SetPointIndices(t, Indices[t]);
    }
  }
  else if (topology == wdGALPrimitiveTopology::Triangles)
  {
    for (wdUInt32 t = 0; t < Indices.GetCount(); t += 3)
    {
      SetTriangleIndices(t / 3, Indices[t], Indices[t + 1], Indices[t + 2]);
    }
  }
  else if (topology == wdGALPrimitiveTopology::Lines)
  {
    for (wdUInt32 t = 0; t < Indices.GetCount(); t += 2)
    {
      SetLineIndices(t / 2, Indices[t], Indices[t + 1]);
    }
  }
}

void wdMeshBufferResourceDescriptor::SetPointIndices(wdUInt32 uiPoint, wdUInt32 uiVertex0)
{
  WD_ASSERT_DEBUG(m_Topology == wdGALPrimitiveTopology::Points, "Wrong topology");

  if (Uses32BitIndices())
  {
    wdUInt32* pIndices = reinterpret_cast<wdUInt32*>(&m_IndexBufferData[uiPoint * sizeof(wdUInt32) * 1]);
    pIndices[0] = uiVertex0;
  }
  else
  {
    wdUInt16* pIndices = reinterpret_cast<wdUInt16*>(&m_IndexBufferData[uiPoint * sizeof(wdUInt16) * 1]);
    pIndices[0] = static_cast<wdUInt16>(uiVertex0);
  }
}

void wdMeshBufferResourceDescriptor::SetLineIndices(wdUInt32 uiLine, wdUInt32 uiVertex0, wdUInt32 uiVertex1)
{
  WD_ASSERT_DEBUG(m_Topology == wdGALPrimitiveTopology::Lines, "Wrong topology");

  if (Uses32BitIndices())
  {
    wdUInt32* pIndices = reinterpret_cast<wdUInt32*>(&m_IndexBufferData[uiLine * sizeof(wdUInt32) * 2]);
    pIndices[0] = uiVertex0;
    pIndices[1] = uiVertex1;
  }
  else
  {
    wdUInt16* pIndices = reinterpret_cast<wdUInt16*>(&m_IndexBufferData[uiLine * sizeof(wdUInt16) * 2]);
    pIndices[0] = static_cast<wdUInt16>(uiVertex0);
    pIndices[1] = static_cast<wdUInt16>(uiVertex1);
  }
}

void wdMeshBufferResourceDescriptor::SetTriangleIndices(wdUInt32 uiTriangle, wdUInt32 uiVertex0, wdUInt32 uiVertex1, wdUInt32 uiVertex2)
{
  WD_ASSERT_DEBUG(m_Topology == wdGALPrimitiveTopology::Triangles, "Wrong topology");

  if (Uses32BitIndices())
  {
    wdUInt32* pIndices = reinterpret_cast<wdUInt32*>(&m_IndexBufferData[uiTriangle * sizeof(wdUInt32) * 3]);
    pIndices[0] = uiVertex0;
    pIndices[1] = uiVertex1;
    pIndices[2] = uiVertex2;
  }
  else
  {
    wdUInt16* pIndices = reinterpret_cast<wdUInt16*>(&m_IndexBufferData[uiTriangle * sizeof(wdUInt16) * 3]);
    pIndices[0] = static_cast<wdUInt16>(uiVertex0);
    pIndices[1] = static_cast<wdUInt16>(uiVertex1);
    pIndices[2] = static_cast<wdUInt16>(uiVertex2);
  }
}

wdUInt32 wdMeshBufferResourceDescriptor::GetPrimitiveCount() const
{
  const wdUInt32 divider = m_Topology + 1;

  if (!m_IndexBufferData.IsEmpty())
  {
    if (Uses32BitIndices())
      return (m_IndexBufferData.GetCount() / sizeof(wdUInt32)) / divider;
    else
      return (m_IndexBufferData.GetCount() / sizeof(wdUInt16)) / divider;
  }
  else
  {
    return m_uiVertexCount / divider;
  }
}

wdBoundingBoxSphere wdMeshBufferResourceDescriptor::ComputeBounds() const
{
  wdBoundingBoxSphere bounds;
  bounds.SetInvalid();

  for (wdUInt32 i = 0; i < m_VertexDeclaration.m_VertexStreams.GetCount(); ++i)
  {
    if (m_VertexDeclaration.m_VertexStreams[i].m_Semantic == wdGALVertexAttributeSemantic::Position)
    {
      WD_ASSERT_DEBUG(m_VertexDeclaration.m_VertexStreams[i].m_Format == wdGALResourceFormat::XYZFloat, "Position format is not usable");

      const wdUInt32 offset = m_VertexDeclaration.m_VertexStreams[i].m_uiOffset;

      if (!m_VertexStreamData.IsEmpty() && m_uiVertexCount > 0)
      {
        bounds.SetFromPoints(reinterpret_cast<const wdVec3*>(&m_VertexStreamData[offset]), m_uiVertexCount, m_uiVertexSize);
      }

      return bounds;
    }
  }

  return bounds;
}

wdResult wdMeshBufferResourceDescriptor::RecomputeNormals()
{
  if (m_Topology != wdGALPrimitiveTopology::Triangles)
    return WD_FAILURE; // normals not needed

  const wdUInt32 uiVertexSize = m_uiVertexSize;
  const wdUInt8* pPositions = nullptr;
  wdUInt8* pNormals = nullptr;
  wdGALResourceFormat::Enum normalsFormat = wdGALResourceFormat::XYZFloat;

  for (wdUInt32 i = 0; i < m_VertexDeclaration.m_VertexStreams.GetCount(); ++i)
  {
    if (m_VertexDeclaration.m_VertexStreams[i].m_Semantic == wdGALVertexAttributeSemantic::Position && m_VertexDeclaration.m_VertexStreams[i].m_Format == wdGALResourceFormat::XYZFloat)
    {
      pPositions = GetVertexData(i, 0).GetPtr();
    }

    if (m_VertexDeclaration.m_VertexStreams[i].m_Semantic == wdGALVertexAttributeSemantic::Normal)
    {
      normalsFormat = m_VertexDeclaration.m_VertexStreams[i].m_Format;
      pNormals = GetVertexData(i, 0).GetPtr();
    }
  }

  if (pPositions == nullptr || pNormals == nullptr)
    return WD_FAILURE; // there are no normals that could be recomputed

  wdDynamicArray<wdVec3> newNormals;
  newNormals.SetCountUninitialized(m_uiVertexCount);

  for (auto& n : newNormals)
  {
    n.SetZero();
  }

  wdResult res = WD_SUCCESS;

  const wdUInt16* pIndices16 = reinterpret_cast<const wdUInt16*>(m_IndexBufferData.GetData());
  const wdUInt32* pIndices32 = reinterpret_cast<const wdUInt32*>(m_IndexBufferData.GetData());
  const bool bUseIndices32 = Uses32BitIndices();

  // Compute unnormalized triangle normals and add them to all vertices.
  // This way large triangles have an higher influence on the vertex normal.
  for (wdUInt32 triIdx = 0; triIdx < GetPrimitiveCount(); ++triIdx)
  {
    const wdUInt32 v0 = bUseIndices32 ? pIndices32[triIdx * 3 + 0] : pIndices16[triIdx * 3 + 0];
    const wdUInt32 v1 = bUseIndices32 ? pIndices32[triIdx * 3 + 1] : pIndices16[triIdx * 3 + 1];
    const wdUInt32 v2 = bUseIndices32 ? pIndices32[triIdx * 3 + 2] : pIndices16[triIdx * 3 + 2];

    const wdVec3 p0 = *reinterpret_cast<const wdVec3*>(pPositions + wdMath::SafeMultiply64(uiVertexSize, v0));
    const wdVec3 p1 = *reinterpret_cast<const wdVec3*>(pPositions + wdMath::SafeMultiply64(uiVertexSize, v1));
    const wdVec3 p2 = *reinterpret_cast<const wdVec3*>(pPositions + wdMath::SafeMultiply64(uiVertexSize, v2));

    const wdVec3 d01 = p1 - p0;
    const wdVec3 d02 = p2 - p0;

    const wdVec3 triNormal = d01.CrossRH(d02);

    if (triNormal.IsValid())
    {
      newNormals[v0] += triNormal;
      newNormals[v1] += triNormal;
      newNormals[v2] += triNormal;
    }
  }

  for (wdUInt32 i = 0; i < newNormals.GetCount(); ++i)
  {
    // normalize the new normal
    if (newNormals[i].NormalizeIfNotZero(wdVec3::UnitXAxis()).Failed())
      res = WD_FAILURE;

    // then encode it in the target format precision and write it back to the buffer
    WD_SUCCEED_OR_RETURN(wdMeshBufferUtils::EncodeNormal(newNormals[i], wdByteArrayPtr(pNormals + wdMath::SafeMultiply64(uiVertexSize, i), sizeof(wdVec3)), normalsFormat));
  }

  return res;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

wdMeshBufferResource::~wdMeshBufferResource()
{
  WD_ASSERT_DEBUG(m_hVertexBuffer.IsInvalidated(), "Implementation error");
  WD_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");
}

wdResourceLoadDesc wdMeshBufferResource::UnloadData(Unload WhatToUnload)
{
  if (!m_hVertexBuffer.IsInvalidated())
  {
    wdGALDevice::GetDefaultDevice()->DestroyBuffer(m_hVertexBuffer);
    m_hVertexBuffer.Invalidate();
  }

  if (!m_hIndexBuffer.IsInvalidated())
  {
    wdGALDevice::GetDefaultDevice()->DestroyBuffer(m_hIndexBuffer);
    m_hIndexBuffer.Invalidate();
  }

  m_uiPrimitiveCount = 0;

  // we cannot compute this in UpdateMemoryUsage(), so we only read the data there, therefore we need to update this information here
  ModifyMemoryUsage().m_uiMemoryGPU = 0;

  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::Unloaded;

  return res;
}

wdResourceLoadDesc wdMeshBufferResource::UpdateContent(wdStreamReader* Stream)
{
  WD_REPORT_FAILURE("This resource type does not support loading data from file.");

  return wdResourceLoadDesc();
}

void wdMeshBufferResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  // we cannot compute this data here, so we update it wherever we know the memory usage

  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(wdMeshBufferResource);
  out_NewMemoryUsage.m_uiMemoryGPU = ModifyMemoryUsage().m_uiMemoryGPU;
}

WD_RESOURCE_IMPLEMENT_CREATEABLE(wdMeshBufferResource, wdMeshBufferResourceDescriptor)
{
  WD_ASSERT_DEBUG(m_hVertexBuffer.IsInvalidated(), "Implementation error");
  WD_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");

  m_VertexDeclaration = descriptor.GetVertexDeclaration();
  m_VertexDeclaration.ComputeHash();

  m_uiPrimitiveCount = descriptor.GetPrimitiveCount();
  m_Topology = descriptor.GetTopology();

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  m_hVertexBuffer = pDevice->CreateVertexBuffer(descriptor.GetVertexDataSize(), descriptor.GetVertexCount(), descriptor.GetVertexBufferData().GetArrayPtr());

  wdStringBuilder sName;
  sName.Format("{0} Vertex Buffer", GetResourceDescription());
  pDevice->GetBuffer(m_hVertexBuffer)->SetDebugName(sName);

  if (descriptor.HasIndexBuffer())
  {
    m_hIndexBuffer = pDevice->CreateIndexBuffer(descriptor.Uses32BitIndices() ? wdGALIndexType::UInt : wdGALIndexType::UShort, m_uiPrimitiveCount * wdGALPrimitiveTopology::VerticesPerPrimitive(m_Topology), descriptor.GetIndexBufferData());

    sName.Format("{0} Index Buffer", GetResourceDescription());
    pDevice->GetBuffer(m_hIndexBuffer)->SetDebugName(sName);

    // we only know the memory usage here, so we write it back to the internal variable directly and then read it in UpdateMemoryUsage() again
    ModifyMemoryUsage().m_uiMemoryGPU = descriptor.GetVertexBufferData().GetCount() + descriptor.GetIndexBufferData().GetCount();
  }
  else
  {
    // we only know the memory usage here, so we write it back to the internal variable directly and then read it in UpdateMemoryUsage() again
    ModifyMemoryUsage().m_uiMemoryGPU = descriptor.GetVertexBufferData().GetCount();
  }


  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::Loaded;

  m_Bounds = descriptor.ComputeBounds();

  return res;
}

void wdVertexDeclarationInfo::ComputeHash()
{
  m_uiHash = 0;

  for (const auto& vs : m_VertexStreams)
  {
    m_uiHash += vs.CalculateHash();

    WD_ASSERT_DEBUG(m_uiHash != 0, "Invalid Hash Value");
  }
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshBufferResource);
