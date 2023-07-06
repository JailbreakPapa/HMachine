#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <Texture/Image/Image.h>
#include <Texture/ezTexFormat/ezTexFormat.h>

class WD_RENDERERCORE_DLL wdTextureResourceLoader : public wdResourceTypeLoader
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
    wdImage m_Image;

    bool m_bIsFallback = false;
    wdTexFormat m_TexFormat;
  };

  virtual wdResourceLoadData OpenDataStream(const wdResource* pResource) override;
  virtual void CloseDataStream(const wdResource* pResource, const wdResourceLoadData& loaderData) override;
  virtual bool IsResourceOutdated(const wdResource* pResource) const override;

  static wdResult LoadTexFile(wdStreamReader& inout_stream, LoadedData& ref_data);
  static void WriteTextureLoadStream(wdStreamWriter& inout_stream, const LoadedData& data);
};
