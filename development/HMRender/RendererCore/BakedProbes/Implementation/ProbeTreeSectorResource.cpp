#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/ChunkStream.h>
#include <RendererCore/BakedProbes/ProbeTreeSectorResource.h>

wdProbeTreeSectorResourceDescriptor::wdProbeTreeSectorResourceDescriptor() = default;
wdProbeTreeSectorResourceDescriptor::~wdProbeTreeSectorResourceDescriptor() = default;
wdProbeTreeSectorResourceDescriptor& wdProbeTreeSectorResourceDescriptor::operator=(wdProbeTreeSectorResourceDescriptor&& other) = default;

void wdProbeTreeSectorResourceDescriptor::Clear()
{
  m_ProbePositions.Clear();
  m_SkyVisibility.Clear();
}

wdUInt64 wdProbeTreeSectorResourceDescriptor::GetHeapMemoryUsage() const
{
  wdUInt64 uiMemUsage = 0;
  uiMemUsage += m_ProbePositions.GetHeapMemoryUsage();
  uiMemUsage += m_SkyVisibility.GetHeapMemoryUsage();
  return uiMemUsage;
}

static wdTypeVersion s_ProbeTreeResourceDescriptorVersion = 1;
wdResult wdProbeTreeSectorResourceDescriptor::Serialize(wdStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_ProbeTreeResourceDescriptorVersion);

  inout_stream << m_vGridOrigin;
  inout_stream << m_vProbeSpacing;
  inout_stream << m_vProbeCount;

  WD_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_ProbePositions));
  WD_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_SkyVisibility));

  return WD_SUCCESS;
}

wdResult wdProbeTreeSectorResourceDescriptor::Deserialize(wdStreamReader& inout_stream)
{
  Clear();

  const wdTypeVersion version = inout_stream.ReadVersion(s_ProbeTreeResourceDescriptorVersion);

  inout_stream >> m_vGridOrigin;
  inout_stream >> m_vProbeSpacing;
  inout_stream >> m_vProbeCount;

  WD_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_ProbePositions));
  WD_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_SkyVisibility));

  return WD_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdProbeTreeSectorResource, 1, wdRTTIDefaultAllocator<wdProbeTreeSectorResource>);
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_RESOURCE_IMPLEMENT_COMMON_CODE(wdProbeTreeSectorResource);
// clang-format on

wdProbeTreeSectorResource::wdProbeTreeSectorResource()
  : wdResource(DoUpdate::OnAnyThread, 1)
{
}

wdProbeTreeSectorResource::~wdProbeTreeSectorResource() = default;

wdResourceLoadDesc wdProbeTreeSectorResource::UnloadData(Unload WhatToUnload)
{
  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::Unloaded;

  m_Desc.Clear();

  return res;
}

wdResourceLoadDesc wdProbeTreeSectorResource::UpdateContent(wdStreamReader* Stream)
{
  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = wdResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    wdString sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  wdAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  wdProbeTreeSectorResourceDescriptor descriptor;
  if (descriptor.Deserialize(*Stream).Failed())
  {
    res.m_State = wdResourceState::LoadedResourceMissing;
    return res;
  }

  return CreateResource(std::move(descriptor));
}

void wdProbeTreeSectorResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(wdProbeTreeSectorResource);
  out_NewMemoryUsage.m_uiMemoryCPU += m_Desc.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

wdResourceLoadDesc wdProbeTreeSectorResource::CreateResource(wdProbeTreeSectorResourceDescriptor&& descriptor)
{
  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::Loaded;

  m_Desc = std::move(descriptor);

  return res;
}


WD_STATICLINK_FILE(RendererCore, RendererCore_BakedProbes_Implementation_ProbeTreeSectorResource);
