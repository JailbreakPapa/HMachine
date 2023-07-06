#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/MemoryStream.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdAnimGraphResource, 1, wdRTTIDefaultAllocator<wdAnimGraphResource>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_RESOURCE_IMPLEMENT_COMMON_CODE(wdAnimGraphResource);
// clang-format on

wdAnimGraphResource::wdAnimGraphResource()
  : wdResource(wdResource::DoUpdate::OnAnyThread, 0)
{
}

wdAnimGraphResource::~wdAnimGraphResource() = default;

void wdAnimGraphResource::DeserializeAnimGraphState(wdAnimGraph& ref_out)
{
  wdMemoryStreamContainerWrapperStorage<wdDataBuffer> wrapper(&m_Storage);
  wdMemoryStreamReader reader(&wrapper);
  ref_out.Deserialize(reader).IgnoreResult();
}

wdResourceLoadDesc wdAnimGraphResource::UnloadData(Unload WhatToUnload)
{
  m_Storage.Clear();
  m_Storage.Compact();

  wdResourceLoadDesc d;
  d.m_State = wdResourceState::Unloaded;
  d.m_uiQualityLevelsDiscardable = 0;
  d.m_uiQualityLevelsLoadable = 0;
  return d;
}

wdResourceLoadDesc wdAnimGraphResource::UpdateContent(wdStreamReader* Stream)
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
    wdStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  wdAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  wdUInt32 uiDateSize = 0;
  *Stream >> uiDateSize;
  m_Storage.SetCountUninitialized(uiDateSize);
  Stream->ReadBytes(m_Storage.GetData(), uiDateSize);

  res.m_State = wdResourceState::Loaded;
  return res;
}

void wdAnimGraphResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = m_Storage.GetHeapMemoryUsage();
}


WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Implementation_AnimGraphResource);
