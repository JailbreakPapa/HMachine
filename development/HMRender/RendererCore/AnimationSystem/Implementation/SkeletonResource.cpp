#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <RendererCore/AnimationSystem/Implementation/OzzUtils.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/io/archive.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdSkeletonResource, 1, wdRTTIDefaultAllocator<wdSkeletonResource>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_RESOURCE_IMPLEMENT_COMMON_CODE(wdSkeletonResource);
// clang-format on

wdSkeletonResource::wdSkeletonResource()
  : wdResource(DoUpdate::OnAnyThread, 1)
{
}

wdSkeletonResource::~wdSkeletonResource() = default;

WD_RESOURCE_IMPLEMENT_CREATEABLE(wdSkeletonResource, wdSkeletonResourceDescriptor)
{
  m_pDescriptor = WD_DEFAULT_NEW(wdSkeletonResourceDescriptor);
  *m_pDescriptor = std::move(descriptor);

  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::Loaded;

  return res;
}

wdResourceLoadDesc wdSkeletonResource::UnloadData(Unload WhatToUnload)
{
  m_pDescriptor.Clear();

  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::Unloaded;

  return res;
}

wdResourceLoadDesc wdSkeletonResource::UpdateContent(wdStreamReader* Stream)
{
  WD_LOG_BLOCK("wdSkeletonResource::UpdateContent", GetResourceDescription().GetData());

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

  // skip the asset file header at the start of the file
  wdAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_pDescriptor = WD_DEFAULT_NEW(wdSkeletonResourceDescriptor);
  m_pDescriptor->Deserialize(*Stream).IgnoreResult();

  res.m_State = wdResourceState::Loaded;
  return res;
}

void wdSkeletonResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(wdSkeletonResource); // TODO
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

wdSkeletonResourceDescriptor::wdSkeletonResourceDescriptor() = default;
wdSkeletonResourceDescriptor::~wdSkeletonResourceDescriptor() = default;
wdSkeletonResourceDescriptor::wdSkeletonResourceDescriptor(wdSkeletonResourceDescriptor&& rhs)
{
  *this = std::move(rhs);
}

void wdSkeletonResourceDescriptor::operator=(wdSkeletonResourceDescriptor&& rhs)
{
  m_Skeleton = std::move(rhs.m_Skeleton);
  m_Geometry = std::move(rhs.m_Geometry);
}

wdUInt64 wdSkeletonResourceDescriptor::GetHeapMemoryUsage() const
{
  return m_Geometry.GetHeapMemoryUsage() + m_Skeleton.GetHeapMemoryUsage();
}

wdResult wdSkeletonResourceDescriptor::Serialize(wdStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(6);

  m_Skeleton.Save(inout_stream);
  inout_stream << m_RootTransform;

  const wdUInt16 uiNumGeom = static_cast<wdUInt16>(m_Geometry.GetCount());
  inout_stream << uiNumGeom;

  for (wdUInt32 i = 0; i < uiNumGeom; ++i)
  {
    const auto& geo = m_Geometry[i];

    inout_stream << geo.m_uiAttachedToJoint;
    inout_stream << geo.m_Type;
    inout_stream << geo.m_Transform;
    inout_stream << geo.m_sName;
    inout_stream << geo.m_hSurface;
    inout_stream << geo.m_uiCollisionLayer;
  }

  return WD_SUCCESS;
}

wdResult wdSkeletonResourceDescriptor::Deserialize(wdStreamReader& inout_stream)
{
  const wdTypeVersion version = inout_stream.ReadVersion(6);

  if (version != 6)
    return WD_FAILURE;

  m_Skeleton.Load(inout_stream);

  inout_stream >> m_RootTransform;

  m_Geometry.Clear();

  wdUInt16 uiNumGeom = 0;
  inout_stream >> uiNumGeom;
  m_Geometry.Reserve(uiNumGeom);

  for (wdUInt32 i = 0; i < uiNumGeom; ++i)
  {
    auto& geo = m_Geometry.ExpandAndGetRef();

    inout_stream >> geo.m_uiAttachedToJoint;
    inout_stream >> geo.m_Type;
    inout_stream >> geo.m_Transform;
    inout_stream >> geo.m_sName;
    inout_stream >> geo.m_hSurface;
    inout_stream >> geo.m_uiCollisionLayer;
  }

  return WD_SUCCESS;
}


WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_SkeletonResource);
