#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Containers/HashTable.h>
#include <RendererCore/RendererCoreDLL.h>

using wdRenderPipelineResourceHandle = wdTypedResourceHandle<class wdRenderPipelineResource>;
class wdRenderPipeline;

struct wdRenderPipelineResourceDescriptor
{
  void Clear() {}

  void CreateFromRenderPipeline(const wdRenderPipeline* pPipeline);

  wdDynamicArray<wdUInt8> m_SerializedPipeline;
  wdString m_sPath;
};

class WD_RENDERERCORE_DLL wdRenderPipelineResource : public wdResource
{
  WD_ADD_DYNAMIC_REFLECTION(wdRenderPipelineResource, wdResource);
  WD_RESOURCE_DECLARE_COMMON_CODE(wdRenderPipelineResource);
  WD_RESOURCE_DECLARE_CREATEABLE(wdRenderPipelineResource, wdRenderPipelineResourceDescriptor);

public:
  wdRenderPipelineResource();

  WD_ALWAYS_INLINE const wdRenderPipelineResourceDescriptor& GetDescriptor() { return m_Desc; }

  wdInternal::NewInstance<wdRenderPipeline> CreateRenderPipeline() const;

public:
  static wdRenderPipelineResourceHandle CreateMissingPipeline();

private:
  virtual wdResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual wdResourceLoadDesc UpdateContent(wdStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  wdRenderPipelineResourceDescriptor m_Desc;
};
