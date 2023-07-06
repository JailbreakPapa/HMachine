#pragma once

#include <Core/Graphics/AmbientCubeBasis.h>
#include <Core/ResourceManager/Resource.h>
#include <RendererCore/BakedProbes/BakingUtils.h>

using wdProbeTreeSectorResourceHandle = wdTypedResourceHandle<class wdProbeTreeSectorResource>;

struct WD_RENDERERCORE_DLL wdProbeTreeSectorResourceDescriptor
{
  WD_DISALLOW_COPY_AND_ASSIGN(wdProbeTreeSectorResourceDescriptor);

  wdProbeTreeSectorResourceDescriptor();
  ~wdProbeTreeSectorResourceDescriptor();
  wdProbeTreeSectorResourceDescriptor& operator=(wdProbeTreeSectorResourceDescriptor&& other);

  wdVec3 m_vGridOrigin;
  wdVec3 m_vProbeSpacing;
  wdVec3U32 m_vProbeCount;

  wdDynamicArray<wdVec3> m_ProbePositions;
  wdDynamicArray<wdCompressedSkyVisibility> m_SkyVisibility;

  void Clear();
  wdUInt64 GetHeapMemoryUsage() const;

  wdResult Serialize(wdStreamWriter& inout_stream) const;
  wdResult Deserialize(wdStreamReader& inout_stream);
};

class WD_RENDERERCORE_DLL wdProbeTreeSectorResource : public wdResource
{
  WD_ADD_DYNAMIC_REFLECTION(wdProbeTreeSectorResource, wdResource);
  WD_RESOURCE_DECLARE_COMMON_CODE(wdProbeTreeSectorResource);
  WD_RESOURCE_DECLARE_CREATEABLE(wdProbeTreeSectorResource, wdProbeTreeSectorResourceDescriptor);

public:
  wdProbeTreeSectorResource();
  ~wdProbeTreeSectorResource();

  const wdVec3& GetGridOrigin() const { return m_Desc.m_vGridOrigin; }
  const wdVec3& GetProbeSpacing() const { return m_Desc.m_vProbeSpacing; }
  const wdVec3U32& GetProbeCount() const { return m_Desc.m_vProbeCount; }

  wdArrayPtr<const wdVec3> GetProbePositions() const { return m_Desc.m_ProbePositions; }
  wdArrayPtr<const wdCompressedSkyVisibility> GetSkyVisibility() const { return m_Desc.m_SkyVisibility; }

private:
  virtual wdResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual wdResourceLoadDesc UpdateContent(wdStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  wdProbeTreeSectorResourceDescriptor m_Desc;
};
