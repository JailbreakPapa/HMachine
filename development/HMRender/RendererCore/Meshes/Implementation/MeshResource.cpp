#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdMeshResource, 1, wdRTTIDefaultAllocator<wdMeshResource>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_RESOURCE_IMPLEMENT_COMMON_CODE(wdMeshResource);
// clang-format on

wdUInt32 wdMeshResource::s_uiMeshBufferNameSuffix = 0;

wdMeshResource::wdMeshResource()
  : wdResource(DoUpdate::OnAnyThread, 1)
{
  m_Bounds.SetInvalid();
}

wdResourceLoadDesc wdMeshResource::UnloadData(Unload WhatToUnload)
{
  wdResourceLoadDesc res;
  res.m_State = GetLoadingState();
  res.m_uiQualityLevelsDiscardable = GetNumQualityLevelsDiscardable();
  res.m_uiQualityLevelsLoadable = GetNumQualityLevelsLoadable();

  // we currently can only unload the entire mesh
  // if (WhatToUnload == Unload::AllQualityLevels)
  {
    m_SubMeshes.Clear();
    m_SubMeshes.Compact();
    m_Materials.Clear();
    m_Materials.Compact();
    m_Bones.Clear();
    m_Bones.Compact();

    m_hMeshBuffer.Invalidate();
    m_hDefaultSkeleton.Invalidate();

    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = wdResourceState::Unloaded;
  }

  return res;
}

wdResourceLoadDesc wdMeshResource::UpdateContent(wdStreamReader* Stream)
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

  if (desc.Load(*Stream).Failed())
  {
    res.m_State = wdResourceState::LoadedResourceMissing;
    return res;
  }

  return CreateResource(std::move(desc));
}

void wdMeshResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(wdMeshResource) + (wdUInt32)m_SubMeshes.GetHeapMemoryUsage() + (wdUInt32)m_Materials.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

WD_RESOURCE_IMPLEMENT_CREATEABLE(wdMeshResource, wdMeshResourceDescriptor)
{
  // if there is an existing mesh buffer to use, take that
  m_hMeshBuffer = descriptor.GetExistingMeshBuffer();

  m_hDefaultSkeleton = descriptor.m_hDefaultSkeleton;
  m_Bones = descriptor.m_Bones;
  m_fMaxBoneVertexOffset = descriptor.m_fMaxBoneVertexOffset;

  // otherwise create a new mesh buffer from the descriptor
  if (!m_hMeshBuffer.IsValid())
  {
    s_uiMeshBufferNameSuffix++;
    wdStringBuilder sMbName;
    sMbName.Format("{0}  [MeshBuffer {1}]", GetResourceID(), wdArgU(s_uiMeshBufferNameSuffix, 4, true, 16, true));

    // note: this gets move'd, might be invalid afterwards
    wdMeshBufferResourceDescriptor& mb = descriptor.MeshBufferDesc();

    m_hMeshBuffer = wdResourceManager::CreateResource<wdMeshBufferResource>(sMbName, std::move(mb), GetResourceDescription());
  }

  m_SubMeshes = descriptor.GetSubMeshes();

  m_Materials.Clear();
  m_Materials.Reserve(descriptor.GetMaterials().GetCount());

  // copy all the material assignments and load the materials
  for (const auto& mat : descriptor.GetMaterials())
  {
    wdMaterialResourceHandle hMat;

    if (!mat.m_sPath.IsEmpty())
      hMat = wdResourceManager::LoadResource<wdMaterialResource>(mat.m_sPath);

    m_Materials.PushBack(hMat); // may be an invalid handle
  }

  m_Bounds = descriptor.GetBounds();
  WD_ASSERT_DEV(m_Bounds.IsValid(), "The mesh bounds are invalid. Make sure to call wdMeshResourceDescriptor::ComputeBounds()");

  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::Loaded;

  return res;
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshResource);
