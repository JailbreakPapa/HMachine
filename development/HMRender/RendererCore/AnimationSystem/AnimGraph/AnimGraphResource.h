#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/DynamicArray.h>

class wdAnimGraph;

//////////////////////////////////////////////////////////////////////////

using wdAnimGraphResourceHandle = wdTypedResourceHandle<class wdAnimGraphResource>;

class WD_RENDERERCORE_DLL wdAnimGraphResource : public wdResource
{
  WD_ADD_DYNAMIC_REFLECTION(wdAnimGraphResource, wdResource);
  WD_RESOURCE_DECLARE_COMMON_CODE(wdAnimGraphResource);

public:
  wdAnimGraphResource();
  ~wdAnimGraphResource();

  void DeserializeAnimGraphState(wdAnimGraph& ref_out);

private:
  virtual wdResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual wdResourceLoadDesc UpdateContent(wdStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  wdDataBuffer m_Storage;
};
