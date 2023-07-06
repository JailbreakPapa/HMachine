#pragma once

#include <RendererCore/Meshes/MeshResourceDescriptor.h>

class WD_RENDERERCORE_DLL wdCpuMeshResource : public wdResource
{
  WD_ADD_DYNAMIC_REFLECTION(wdCpuMeshResource, wdResource);
  WD_RESOURCE_DECLARE_COMMON_CODE(wdCpuMeshResource);
  WD_RESOURCE_DECLARE_CREATEABLE(wdCpuMeshResource, wdMeshResourceDescriptor);

public:
  wdCpuMeshResource();

  const wdMeshResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual wdResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual wdResourceLoadDesc UpdateContent(wdStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  wdMeshResourceDescriptor m_Descriptor;
};

using wdCpuMeshResourceHandle = wdTypedResourceHandle<class wdCpuMeshResource>;
