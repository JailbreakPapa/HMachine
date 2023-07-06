#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/RendererCoreDLL.h>

using wdSurfaceResourceHandle = wdTypedResourceHandle<class wdSurfaceResource>;

struct wdSkeletonResourceGeometry
{
  // scale is used to resize a unit sphere / box / capsule
  wdTransform m_Transform;
  wdUInt16 m_uiAttachedToJoint = 0;
  wdEnum<wdSkeletonJointGeometryType> m_Type;
  wdHashedString m_sName;
  wdSurfaceResourceHandle m_hSurface;
  wdUInt8 m_uiCollisionLayer = 0;
};

struct WD_RENDERERCORE_DLL wdSkeletonResourceDescriptor
{
  wdSkeletonResourceDescriptor();
  ~wdSkeletonResourceDescriptor();
  wdSkeletonResourceDescriptor(const wdSkeletonResourceDescriptor& rhs) = delete;
  wdSkeletonResourceDescriptor(wdSkeletonResourceDescriptor&& rhs);
  void operator=(wdSkeletonResourceDescriptor&& rhs);
  void operator=(const wdSkeletonResourceDescriptor& rhs) = delete;

  wdResult Serialize(wdStreamWriter& inout_stream) const;
  wdResult Deserialize(wdStreamReader& inout_stream);

  wdUInt64 GetHeapMemoryUsage() const;

  wdTransform m_RootTransform = wdTransform::IdentityTransform();
  wdSkeleton m_Skeleton;

  wdDynamicArray<wdSkeletonResourceGeometry> m_Geometry;

  float m_fMaxImpluse = wdMath::HighValue<float>();
};

using wdSkeletonResourceHandle = wdTypedResourceHandle<class wdSkeletonResource>;

class WD_RENDERERCORE_DLL wdSkeletonResource : public wdResource
{
  WD_ADD_DYNAMIC_REFLECTION(wdSkeletonResource, wdResource);
  WD_RESOURCE_DECLARE_COMMON_CODE(wdSkeletonResource);
  WD_RESOURCE_DECLARE_CREATEABLE(wdSkeletonResource, wdSkeletonResourceDescriptor);

public:
  wdSkeletonResource();
  ~wdSkeletonResource();

  const wdSkeletonResourceDescriptor& GetDescriptor() const { return *m_pDescriptor; }

private:
  virtual wdResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual wdResourceLoadDesc UpdateContent(wdStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  wdUniquePtr<wdSkeletonResourceDescriptor> m_pDescriptor;
};
