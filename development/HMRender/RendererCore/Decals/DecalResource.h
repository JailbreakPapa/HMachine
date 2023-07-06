#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <RendererCore/RendererCoreDLL.h>

using wdDecalResourceHandle = wdTypedResourceHandle<class wdDecalResource>;

struct wdDecalResourceDescriptor
{
};

class WD_RENDERERCORE_DLL wdDecalResource : public wdResource
{
  WD_ADD_DYNAMIC_REFLECTION(wdDecalResource, wdResource);
  WD_RESOURCE_DECLARE_COMMON_CODE(wdDecalResource);
  WD_RESOURCE_DECLARE_CREATEABLE(wdDecalResource, wdDecalResourceDescriptor);

public:
  wdDecalResource();

private:
  virtual wdResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual wdResourceLoadDesc UpdateContent(wdStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
};

class WD_RENDERERCORE_DLL wdDecalResourceLoader : public wdResourceTypeLoader
{
public:
  struct LoadedData
  {
    LoadedData()
      : m_Reader(&m_Storage)
    {
    }

    wdContiguousMemoryStreamStorage m_Storage;
    wdMemoryStreamReader m_Reader;
  };

  virtual wdResourceLoadData OpenDataStream(const wdResource* pResource) override;
  virtual void CloseDataStream(const wdResource* pResource, const wdResourceLoadData& loaderData) override;
  virtual bool IsResourceOutdated(const wdResource* pResource) const override;
};
