#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererCore/Textures/TextureLoader.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/ezTexFormat/ezTexFormat.h>

static wdTextureResourceLoader s_TextureResourceLoader;

wdCVarFloat cvar_StreamingTextureLoadDelay("Streaming.TextureLoadDelay", 0.0f, wdCVarFlags::Save, "Artificial texture loading slowdown");

// clang-format off
WD_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, TextureResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    wdResourceManager::SetResourceTypeLoader<wdTexture2DResource>(&s_TextureResourceLoader);
    wdResourceManager::SetResourceTypeLoader<wdTexture3DResource>(&s_TextureResourceLoader);
    wdResourceManager::SetResourceTypeLoader<wdTextureCubeResource>(&s_TextureResourceLoader);
    wdResourceManager::SetResourceTypeLoader<wdRenderToTexture2DResource>(&s_TextureResourceLoader);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    wdResourceManager::SetResourceTypeLoader<wdTexture2DResource>(nullptr);
    wdResourceManager::SetResourceTypeLoader<wdTexture3DResource>(nullptr);
    wdResourceManager::SetResourceTypeLoader<wdTextureCubeResource>(nullptr);
    wdResourceManager::SetResourceTypeLoader<wdRenderToTexture2DResource>(nullptr);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

WD_END_SUBSYSTEM_DECLARATION;
// clang-format on

wdResourceLoadData wdTextureResourceLoader::OpenDataStream(const wdResource* pResource)
{
  LoadedData* pData = WD_DEFAULT_NEW(LoadedData);

  wdResourceLoadData res;

  // Solid Color Textures
  if (wdPathUtils::HasExtension(pResource->GetResourceID(), "color"))
  {
    wdStringBuilder sName = pResource->GetResourceID();
    sName.RemoveFileExtension();

    bool bValidColor = false;
    const wdColorGammaUB color = wdConversionUtils::GetColorByName(sName, &bValidColor);

    if (!bValidColor)
    {
      wdLog::Error("'{0}' is not a valid color name. Using 'RebeccaPurple' as fallback.", sName);
    }

    pData->m_TexFormat.m_bSRGB = true;

    wdImageHeader header;
    header.SetWidth(4);
    header.SetHeight(4);
    header.SetDepth(1);
    header.SetImageFormat(wdImageFormat::R8G8B8A8_UNORM_SRGB);
    header.SetNumMipLevels(1);
    header.SetNumFaces(1);
    pData->m_Image.ResetAndAlloc(header);
    wdUInt8* pPixels = pData->m_Image.GetPixelPointer<wdUInt8>();

    for (wdUInt32 px = 0; px < 4 * 4 * 4; px += 4)
    {
      pPixels[px + 0] = color.r;
      pPixels[px + 1] = color.g;
      pPixels[px + 2] = color.b;
      pPixels[px + 3] = color.a;
    }
  }
  else
  {
    wdFileReader File;
    if (File.Open(pResource->GetResourceID()).Failed())
      return res;

    const wdStringBuilder sAbsolutePath = File.GetFilePathAbsolute();
    res.m_sResourceDescription = File.GetFilePathRelative().GetView();

#if WD_ENABLED(WD_SUPPORTS_FILE_STATS)
    {
      wdFileStats stat;
      if (wdFileSystem::GetFileStats(pResource->GetResourceID(), stat).Succeeded())
      {
        res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
      }
    }
#endif

    /// In case this is not a proper asset (wdTextureXX format), this is a hack to get the SRGB information for the texture
    const wdStringBuilder sName = wdPathUtils::GetFileName(sAbsolutePath);
    pData->m_TexFormat.m_bSRGB = (sName.EndsWith_NoCase("_D") || sName.EndsWith_NoCase("_SRGB") || sName.EndsWith_NoCase("_diff"));

    if (sAbsolutePath.HasExtension("wdTexture2D") || sAbsolutePath.HasExtension("wdTexture3D") || sAbsolutePath.HasExtension("wdTextureCube") || sAbsolutePath.HasExtension("wdRenderTarget") || sAbsolutePath.HasExtension("wdLUT"))
    {
      if (LoadTexFile(File, *pData).Failed())
        return res;
    }
    else
    {
      // read whatever format, as long as wdImage supports it
      File.Close();

      if (pData->m_Image.LoadFrom(pResource->GetResourceID()).Failed())
        return res;

      if (pData->m_Image.GetImageFormat() == wdImageFormat::B8G8R8_UNORM)
      {
        /// \todo A conversion to B8G8R8X8_UNORM currently fails

        wdLog::Warning("Texture resource uses inefficient BGR format, converting to BGRX: '{0}'", sAbsolutePath);
        if (wdImageConversion::Convert(pData->m_Image, pData->m_Image, wdImageFormat::B8G8R8A8_UNORM).Failed())
          return res;
      }
    }
  }

  wdMemoryStreamWriter w(&pData->m_Storage);

  WriteTextureLoadStream(w, *pData);

  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  if (cvar_StreamingTextureLoadDelay > 0)
  {
    wdThreadUtils::Sleep(wdTime::Seconds(cvar_StreamingTextureLoadDelay));
  }

  return res;
}

void wdTextureResourceLoader::CloseDataStream(const wdResource* pResource, const wdResourceLoadData& loaderData)
{
  LoadedData* pData = (LoadedData*)loaderData.m_pCustomLoaderData;

  WD_DEFAULT_DELETE(pData);
}

bool wdTextureResourceLoader::IsResourceOutdated(const wdResource* pResource) const
{
  // solid color textures are never outdated
  if (wdPathUtils::HasExtension(pResource->GetResourceID(), "color"))
    return false;

  // don't try to reload a file that cannot be found
  wdStringBuilder sAbs;
  if (wdFileSystem::ResolvePath(pResource->GetResourceID(), &sAbs, nullptr).Failed())
    return false;

#if WD_ENABLED(WD_SUPPORTS_FILE_STATS)

  if (pResource->GetLoadedFileModificationTime().IsValid())
  {
    wdFileStats stat;
    if (wdFileSystem::GetFileStats(pResource->GetResourceID(), stat).Failed())
      return false;

    return !stat.m_LastModificationTime.Compare(pResource->GetLoadedFileModificationTime(), wdTimestamp::CompareMode::FileTimeEqual);
  }

#endif

  return true;
}

wdResult wdTextureResourceLoader::LoadTexFile(wdStreamReader& inout_stream, LoadedData& ref_data)
{
  // read the hash, ignore it
  wdAssetFileHeader AssetHash;
  WD_SUCCEED_OR_RETURN(AssetHash.Read(inout_stream));

  ref_data.m_TexFormat.ReadHeader(inout_stream);

  if (ref_data.m_TexFormat.m_iRenderTargetResolutionX == 0)
  {
    wdDdsFileFormat fmt;
    return fmt.ReadImage(inout_stream, ref_data.m_Image, "dds");
  }
  else
  {
    return WD_SUCCESS;
  }
}

void wdTextureResourceLoader::WriteTextureLoadStream(wdStreamWriter& w, const LoadedData& data)
{
  const wdImage* pImage = &data.m_Image;
  w.WriteBytes(&pImage, sizeof(wdImage*)).IgnoreResult();

  w << data.m_bIsFallback;
  data.m_TexFormat.WriteRenderTargetHeader(w);
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Textures_TextureLoader);
