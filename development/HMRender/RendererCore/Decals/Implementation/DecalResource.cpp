#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <RendererCore/Decals/DecalResource.h>

static wdDecalResourceLoader s_DecalResourceLoader;

// clang-format off
WD_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, DecalResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation",
  "Core",
  "TextureResource"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    wdResourceManager::SetResourceTypeLoader<wdDecalResource>(&s_DecalResourceLoader);

    wdDecalResourceDescriptor desc;
    wdDecalResourceHandle hFallback = wdResourceManager::CreateResource<wdDecalResource>("Fallback Decal", std::move(desc), "Empty Decal for loading and missing decals");

    wdResourceManager::SetResourceTypeLoadingFallback<wdDecalResource>(hFallback);
    wdResourceManager::SetResourceTypeMissingFallback<wdDecalResource>(hFallback);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    wdResourceManager::SetResourceTypeLoader<wdDecalResource>(nullptr);

    wdResourceManager::SetResourceTypeLoadingFallback<wdDecalResource>(wdDecalResourceHandle());
    wdResourceManager::SetResourceTypeMissingFallback<wdDecalResource>(wdDecalResourceHandle());
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
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdDecalResource, 1, wdRTTIDefaultAllocator<wdDecalResource>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_RESOURCE_IMPLEMENT_COMMON_CODE(wdDecalResource);
// clang-format on

wdDecalResource::wdDecalResource()
  : wdResource(DoUpdate::OnAnyThread, 1)
{
}

wdResourceLoadDesc wdDecalResource::UnloadData(Unload WhatToUnload)
{
  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::Unloaded;

  return res;
}

wdResourceLoadDesc wdDecalResource::UpdateContent(wdStreamReader* Stream)
{
  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::Loaded;

  return res;
}

void wdDecalResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(wdDecalResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

WD_RESOURCE_IMPLEMENT_CREATEABLE(wdDecalResource, wdDecalResourceDescriptor)
{
  wdResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable = 0;
  ret.m_State = wdResourceState::Loaded;

  return ret;
}

//////////////////////////////////////////////////////////////////////////

wdResourceLoadData wdDecalResourceLoader::OpenDataStream(const wdResource* pResource)
{
  // nothing to load, decals are solely identified by their id (name)
  // the rest of the information is in the decal atlas resource

  wdResourceLoadData res;
  return res;
}

void wdDecalResourceLoader::CloseDataStream(const wdResource* pResource, const wdResourceLoadData& loaderData)
{
  // nothing to do
}

bool wdDecalResourceLoader::IsResourceOutdated(const wdResource* pResource) const
{
  // decals are never outdated
  return false;
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Decals_Implementation_DecalResource);
