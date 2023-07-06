#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Decals/DecalAtlasResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Image.h>

// clang-format off
WD_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, DecalAtlasResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation",
  "Core",
  "TextureResource"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    wdDecalAtlasResourceDescriptor desc;
    wdDecalAtlasResourceHandle hFallback = wdResourceManager::CreateResource<wdDecalAtlasResource>("Fallback Decal Atlas", std::move(desc), "Empty Decal Atlas for loading and missing decals");

    wdResourceManager::SetResourceTypeLoadingFallback<wdDecalAtlasResource>(hFallback);
    wdResourceManager::SetResourceTypeMissingFallback<wdDecalAtlasResource>(hFallback);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    wdResourceManager::SetResourceTypeLoadingFallback<wdDecalAtlasResource>(wdDecalAtlasResourceHandle());
    wdResourceManager::SetResourceTypeMissingFallback<wdDecalAtlasResource>(wdDecalAtlasResourceHandle());
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

WD_END_SUBSYSTEM_DECLARATION;
// clang-format on


//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdDecalAtlasResource, 1, wdRTTIDefaultAllocator<wdDecalAtlasResource>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_RESOURCE_IMPLEMENT_COMMON_CODE(wdDecalAtlasResource);
// clang-format on

wdUInt32 wdDecalAtlasResource::s_uiDecalAtlasResources = 0;

wdDecalAtlasResource::wdDecalAtlasResource()
  : wdResource(DoUpdate::OnAnyThread, 1)
  , m_vBaseColorSize(wdVec2U32::ZeroVector())
  , m_vNormalSize(wdVec2U32::ZeroVector())
{
}

wdDecalAtlasResourceHandle wdDecalAtlasResource::GetDecalAtlasResource()
{
  return wdResourceManager::LoadResource<wdDecalAtlasResource>("{ ProjectDecalAtlas }");
}

wdResourceLoadDesc wdDecalAtlasResource::UnloadData(Unload WhatToUnload)
{
  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::Unloaded;

  return res;
}

wdResourceLoadDesc wdDecalAtlasResource::UpdateContent(wdStreamReader* Stream)
{
  WD_LOG_BLOCK("wdDecalAtlasResource::UpdateContent", GetResourceDescription().GetData());

  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::LoadedResourceMissing;

  if (Stream == nullptr)
    return res;

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    wdStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  // skip the asset header
  {
    wdAssetFileHeader header;
    header.Read(*Stream).IgnoreResult();
  }

  {
    wdUInt8 uiVersion = 0;
    *Stream >> uiVersion;
    WD_ASSERT_DEV(uiVersion <= 3, "Invalid decal atlas version {0}", uiVersion);

    // this version is now incompatible
    if (uiVersion < 3)
      return res;
  }

  // read the textures
  {
    wdDdsFileFormat dds;
    wdImage baseColor, normal, orm;

    if (dds.ReadImage(*Stream, baseColor, "dds").Failed())
    {
      wdLog::Error("Failed to load baseColor image for decal atlas");
      return res;
    }

    if (dds.ReadImage(*Stream, normal, "dds").Failed())
    {
      wdLog::Error("Failed to load normal image for decal atlas");
      return res;
    }

    if (dds.ReadImage(*Stream, orm, "dds").Failed())
    {
      wdLog::Error("Failed to load normal image for decal atlas");
      return res;
    }

    CreateLayerTexture(baseColor, true, m_hBaseColor);
    CreateLayerTexture(normal, false, m_hNormal);
    CreateLayerTexture(orm, false, m_hORM);

    m_vBaseColorSize = wdVec2U32(baseColor.GetWidth(), baseColor.GetHeight());
    m_vNormalSize = wdVec2U32(normal.GetWidth(), normal.GetHeight());
    m_vORMSize = wdVec2U32(orm.GetWidth(), orm.GetHeight());
  }

  ReadDecalInfo(Stream);

  res.m_State = wdResourceState::Loaded;
  return res;
}

void wdDecalAtlasResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(wdDecalAtlasResource) + (wdUInt32)m_Atlas.m_Items.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

WD_RESOURCE_IMPLEMENT_CREATEABLE(wdDecalAtlasResource, wdDecalAtlasResourceDescriptor)
{
  wdResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable = 0;
  ret.m_State = wdResourceState::Loaded;

  m_Atlas.Clear();

  return ret;
}

void wdDecalAtlasResource::CreateLayerTexture(const wdImage& img, bool bSRGB, wdTexture2DResourceHandle& out_hTexture)
{
  wdTexture2DResourceDescriptor td;
  td.m_SamplerDesc.m_AddressU = wdImageAddressMode::Clamp;
  td.m_SamplerDesc.m_AddressV = wdImageAddressMode::Clamp;
  td.m_SamplerDesc.m_AddressW = wdImageAddressMode::Clamp;

  wdUInt32 uiMemory;
  wdHybridArray<wdGALSystemMemoryDescription, 32> initData;
  wdTexture2DResource::FillOutDescriptor(td, &img, bSRGB, img.GetNumMipLevels(), uiMemory, initData);
  wdTextureUtils::ConfigureSampler(wdTextureFilterSetting::HighQuality, td.m_SamplerDesc);

  wdStringBuilder sTexId;
  sTexId.Format("{0}_Tex{1}", GetResourceID(), s_uiDecalAtlasResources);
  ++s_uiDecalAtlasResources;

  out_hTexture = wdResourceManager::CreateResource<wdTexture2DResource>(sTexId, std::move(td));
}

void wdDecalAtlasResource::ReadDecalInfo(wdStreamReader* Stream)
{
  m_Atlas.Deserialize(*Stream).IgnoreResult();
}

void wdDecalAtlasResource::ReportResourceIsMissing()
{
#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  // normal during development, don't care much
  wdLog::Debug("Decal Atlas Resource is missing: '{0}' ('{1}')", GetResourceID(), GetResourceDescription());
#else
  // should probably exist for shipped applications, report this
  wdLog::Warning("Decal Atlas Resource is missing: '{0}' ('{1}')", GetResourceID(), GetResourceDescription());
#endif
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Decals_Implementation_DecalAtlasResource);
