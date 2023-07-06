#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdDynamicMeshBufferResource, 1, wdRTTIDefaultAllocator<wdDynamicMeshBufferResource>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_RESOURCE_IMPLEMENT_COMMON_CODE(wdDynamicMeshBufferResource);
// clang-format on

wdDynamicMeshBufferResource::wdDynamicMeshBufferResource()
  : wdResource(DoUpdate::OnAnyThread, 1)
{
}

wdDynamicMeshBufferResource::~wdDynamicMeshBufferResource()
{
  WD_ASSERT_DEBUG(m_hVertexBuffer.IsInvalidated(), "Implementation error");
  WD_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");
  WD_ASSERT_DEBUG(m_hColorBuffer.IsInvalidated(), "Implementation error");
}

wdResourceLoadDesc wdDynamicMeshBufferResource::UnloadData(Unload WhatToUnload)
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

  if (!m_hColorBuffer.IsInvalidated())
  {
    wdGALDevice::GetDefaultDevice()->DestroyBuffer(m_hColorBuffer);
    m_hColorBuffer.Invalidate();
  }

  // we cannot compute this in UpdateMemoryUsage(), so we only read the data there, therefore we need to update this information here
  ModifyMemoryUsage().m_uiMemoryGPU = 0;

  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::Unloaded;

  return res;
}

wdResourceLoadDesc wdDynamicMeshBufferResource::UpdateContent(wdStreamReader* Stream)
{
  WD_REPORT_FAILURE("This resource type does not support loading data from file.");

  return wdResourceLoadDesc();
}

void wdDynamicMeshBufferResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  // we cannot compute this data here, so we update it wherever we know the memory usage

  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(wdDynamicMeshBufferResource) + m_VertexData.GetHeapMemoryUsage() + m_Index16Data.GetHeapMemoryUsage() + m_Index32Data.GetHeapMemoryUsage() + m_ColorData.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = ModifyMemoryUsage().m_uiMemoryGPU;
}

WD_RESOURCE_IMPLEMENT_CREATEABLE(wdDynamicMeshBufferResource, wdDynamicMeshBufferResourceDescriptor)
{
  WD_ASSERT_DEBUG(m_hVertexBuffer.IsInvalidated(), "Implementation error");
  WD_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");
  WD_ASSERT_DEBUG(m_hColorBuffer.IsInvalidated(), "Implementation error");

  m_Descriptor = descriptor;

  m_VertexData.SetCountUninitialized(m_Descriptor.m_uiMaxVertices);

  {
    wdVertexStreamInfo si;
    si.m_uiOffset = 0;
    si.m_Format = wdGALResourceFormat::XYZFloat;
    si.m_Semantic = wdGALVertexAttributeSemantic::Position;
    si.m_uiElementSize = sizeof(wdVec3);
    m_VertexDeclaration.m_VertexStreams.PushBack(si);

    si.m_uiOffset += si.m_uiElementSize;
    si.m_Format = wdGALResourceFormat::XYFloat;
    si.m_Semantic = wdGALVertexAttributeSemantic::TexCoord0;
    si.m_uiElementSize = sizeof(wdVec2);
    m_VertexDeclaration.m_VertexStreams.PushBack(si);

    si.m_uiOffset += si.m_uiElementSize;
    si.m_Format = wdGALResourceFormat::XYZFloat;
    si.m_Semantic = wdGALVertexAttributeSemantic::Normal;
    si.m_uiElementSize = sizeof(wdVec3);
    m_VertexDeclaration.m_VertexStreams.PushBack(si);

    si.m_uiOffset += si.m_uiElementSize;
    si.m_Format = wdGALResourceFormat::XYZWFloat;
    si.m_Semantic = wdGALVertexAttributeSemantic::Tangent;
    si.m_uiElementSize = sizeof(wdVec4);
    m_VertexDeclaration.m_VertexStreams.PushBack(si);

    if (m_Descriptor.m_bColorStream)
    {
      si.m_uiVertexBufferSlot = 1; // separate buffer
      si.m_uiOffset = 0;
      si.m_Format = wdGALResourceFormat::RGBAUByteNormalized;
      si.m_Semantic = wdGALVertexAttributeSemantic::Color0;
      si.m_uiElementSize = sizeof(wdColorLinearUB);
      m_VertexDeclaration.m_VertexStreams.PushBack(si);
    }

    m_VertexDeclaration.ComputeHash();
  }

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  m_hVertexBuffer = pDevice->CreateVertexBuffer(sizeof(wdDynamicMeshVertex), m_Descriptor.m_uiMaxVertices /* no initial data -> mutable */);

  wdStringBuilder sName;
  sName.Format("{0} - Dynamic Vertex Buffer", GetResourceDescription());
  pDevice->GetBuffer(m_hVertexBuffer)->SetDebugName(sName);

  const wdUInt32 uiMaxIndices = wdGALPrimitiveTopology::VerticesPerPrimitive(m_Descriptor.m_Topology) * m_Descriptor.m_uiMaxPrimitives;

  if (m_Descriptor.m_bColorStream)
  {
    m_ColorData.SetCountUninitialized(uiMaxIndices);
    m_hColorBuffer = pDevice->CreateVertexBuffer(sizeof(wdColorLinearUB), m_Descriptor.m_uiMaxVertices /* no initial data -> mutable */);

    sName.Format("{0} - Dynamic Color Buffer", GetResourceDescription());
    pDevice->GetBuffer(m_hColorBuffer)->SetDebugName(sName);
  }

  if (m_Descriptor.m_IndexType == wdGALIndexType::UInt)
  {
    m_Index32Data.SetCountUninitialized(uiMaxIndices);

    m_hIndexBuffer = pDevice->CreateIndexBuffer(wdGALIndexType::UInt, uiMaxIndices /* no initial data -> mutable */);

    sName.Format("{0} - Dynamic Index32 Buffer", GetResourceDescription());
    pDevice->GetBuffer(m_hIndexBuffer)->SetDebugName(sName);
  }
  else if (m_Descriptor.m_IndexType == wdGALIndexType::UShort)
  {
    m_Index16Data.SetCountUninitialized(uiMaxIndices);

    m_hIndexBuffer = pDevice->CreateIndexBuffer(wdGALIndexType::UShort, uiMaxIndices /* no initial data -> mutable */);

    sName.Format("{0} - Dynamic Index16 Buffer", GetResourceDescription());
    pDevice->GetBuffer(m_hIndexBuffer)->SetDebugName(sName);
  }

  // we only know the memory usage here, so we write it back to the internal variable directly and then read it in UpdateMemoryUsage() again
  ModifyMemoryUsage().m_uiMemoryGPU = m_VertexData.GetHeapMemoryUsage() + m_Index32Data.GetHeapMemoryUsage() + m_Index16Data.GetHeapMemoryUsage() + m_ColorData.GetHeapMemoryUsage();

  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::Loaded;

  return res;
}

void wdDynamicMeshBufferResource::UpdateGpuBuffer(wdGALCommandEncoder* pGALCommandEncoder, wdUInt32 uiFirstVertex, wdUInt32 uiNumVertices, wdUInt32 uiFirstIndex, wdUInt32 uiNumIndices, wdGALUpdateMode::Enum mode /*= wdGALUpdateMode::Discard*/)
{
  if (m_bAccessedVB && uiNumVertices > 0)
  {
    if (uiNumVertices == wdMath::MaxValue<wdUInt32>())
      uiNumVertices = m_VertexData.GetCount() - uiFirstVertex;

    WD_ASSERT_DEV(uiNumVertices <= m_VertexData.GetCount(), "Can't upload {} vertices, the buffer was allocated to hold a maximum of {} vertices.", uiNumVertices, m_VertexData.GetCount());

    m_bAccessedVB = false;

    pGALCommandEncoder->UpdateBuffer(m_hVertexBuffer, sizeof(wdDynamicMeshVertex) * uiFirstVertex, m_VertexData.GetArrayPtr().GetSubArray(uiFirstVertex, uiNumVertices).ToByteArray(), mode);
  }

  if (m_bAccessedCB && uiNumVertices > 0)
  {
    if (uiNumVertices == wdMath::MaxValue<wdUInt32>())
      uiNumVertices = m_ColorData.GetCount() - uiFirstVertex;

    WD_ASSERT_DEV(uiNumVertices <= m_ColorData.GetCount(), "Can't upload {} vertices, the buffer was allocated to hold a maximum of {} vertices.", uiNumVertices, m_ColorData.GetCount());

    m_bAccessedCB = false;

    pGALCommandEncoder->UpdateBuffer(m_hColorBuffer, sizeof(wdColorLinearUB) * uiFirstVertex, m_ColorData.GetArrayPtr().GetSubArray(uiFirstVertex, uiNumVertices).ToByteArray(), mode);
  }

  if (m_bAccessedIB && uiNumIndices > 0 && !m_hIndexBuffer.IsInvalidated())
  {
    m_bAccessedIB = false;

    if (!m_Index16Data.IsEmpty())
    {
      WD_ASSERT_DEV(uiFirstIndex < m_Index16Data.GetCount(), "Invalid first index value {}", uiFirstIndex);

      if (uiNumIndices == wdMath::MaxValue<wdUInt32>())
        uiNumIndices = m_Index16Data.GetCount() - uiFirstIndex;

      WD_ASSERT_DEV(uiNumIndices <= m_Index16Data.GetCount(), "Can't upload {} indices, the buffer was allocated to hold a maximum of {} indices.", uiNumIndices, m_Index16Data.GetCount());

      pGALCommandEncoder->UpdateBuffer(m_hIndexBuffer, sizeof(wdUInt16) * uiFirstIndex, m_Index16Data.GetArrayPtr().GetSubArray(uiFirstIndex, uiNumIndices).ToByteArray(), mode);
    }
    else if (!m_Index32Data.IsEmpty())
    {
      WD_ASSERT_DEV(uiFirstIndex < m_Index32Data.GetCount(), "Invalid first index value {}", uiFirstIndex);

      if (uiNumIndices == wdMath::MaxValue<wdUInt32>())
        uiNumIndices = m_Index32Data.GetCount() - uiFirstIndex;

      WD_ASSERT_DEV(uiNumIndices <= m_Index32Data.GetCount(), "Can't upload {} indices, the buffer was allocated to hold a maximum of {} indices.", uiNumIndices, m_Index32Data.GetCount());

      pGALCommandEncoder->UpdateBuffer(m_hIndexBuffer, sizeof(wdUInt32) * uiFirstIndex, m_Index32Data.GetArrayPtr().GetSubArray(uiFirstIndex, uiNumIndices).ToByteArray(), mode);
    }
  }
}


WD_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_DynamicMeshBufferResource);
