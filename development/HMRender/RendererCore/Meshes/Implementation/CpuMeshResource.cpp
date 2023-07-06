#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <RendererCore/Meshes/CpuMeshResource.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdCpuMeshResource, 1, wdRTTIDefaultAllocator<wdCpuMeshResource>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_RESOURCE_IMPLEMENT_COMMON_CODE(wdCpuMeshResource);
// clang-format on

wdCpuMeshResource::wdCpuMeshResource()
  : wdResource(DoUpdate::OnAnyThread, 1)
{
}

wdResourceLoadDesc wdCpuMeshResource::UnloadData(Unload WhatToUnload)
{
  wdResourceLoadDesc res;
  res.m_State = GetLoadingState();
  res.m_uiQualityLevelsDiscardable = GetNumQualityLevelsDiscardable();
  res.m_uiQualityLevelsLoadable = GetNumQualityLevelsLoadable();

  // we currently can only unload the entire mesh
  // if (WhatToUnload == Unload::AllQualityLevels)
  {
    m_Descriptor.Clear();

    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = wdResourceState::Unloaded;
  }

  return res;
}

wdResourceLoadDesc wdCpuMeshResource::UpdateContent(wdStreamReader* Stream)
{
  wdMeshResourceDescriptor desc;
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
    wdStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  wdAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  if (m_Descriptor.Load(*Stream).Failed())
  {
    res.m_State = wdResourceState::LoadedResourceMissing;
    return res;
  }

  res.m_State = wdResourceState::Loaded;
  return res;
}

void wdCpuMeshResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(wdCpuMeshResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

WD_RESOURCE_IMPLEMENT_CREATEABLE(wdCpuMeshResource, wdMeshResourceDescriptor)
{
  m_Descriptor = descriptor;

  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::Loaded;

  return res;
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_CpuMeshResource);
