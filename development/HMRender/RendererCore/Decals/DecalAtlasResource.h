#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/RendererCoreDLL.h>
#include <Texture/Utils/TextureAtlasDesc.h>

using wdDecalAtlasResourceHandle = wdTypedResourceHandle<class wdDecalAtlasResource>;
using wdTexture2DResourceHandle = wdTypedResourceHandle<class wdTexture2DResource>;

class wdImage;

struct wdDecalAtlasResourceDescriptor
{
};

class WD_RENDERERCORE_DLL wdDecalAtlasResource : public wdResource
{
  WD_ADD_DYNAMIC_REFLECTION(wdDecalAtlasResource, wdResource);
  WD_RESOURCE_DECLARE_COMMON_CODE(wdDecalAtlasResource);
  WD_RESOURCE_DECLARE_CREATEABLE(wdDecalAtlasResource, wdDecalAtlasResourceDescriptor);

public:
  wdDecalAtlasResource();

  /// \brief Returns the one global decal atlas resource
  static wdDecalAtlasResourceHandle GetDecalAtlasResource();

  const wdTexture2DResourceHandle& GetBaseColorTexture() const { return m_hBaseColor; }
  const wdTexture2DResourceHandle& GetNormalTexture() const { return m_hNormal; }
  const wdTexture2DResourceHandle& GetORMTexture() const { return m_hORM; }
  const wdVec2U32& GetBaseColorTextureSize() const { return m_vBaseColorSize; }
  const wdVec2U32& GetNormalTextureSize() const { return m_vNormalSize; }
  const wdVec2U32& GetORMTextureSize() const { return m_vORMSize; }
  const wdTextureAtlasRuntimeDesc& GetAtlas() const { return m_Atlas; }

private:
  virtual wdResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual wdResourceLoadDesc UpdateContent(wdStreamReader* Stream) override;
  virtual void ReportResourceIsMissing() override;

  void ReadDecalInfo(wdStreamReader* Stream);

  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  void CreateLayerTexture(const wdImage& img, bool bSRGB, wdTexture2DResourceHandle& out_hTexture);

  wdTextureAtlasRuntimeDesc m_Atlas;
  static wdUInt32 s_uiDecalAtlasResources;
  wdTexture2DResourceHandle m_hBaseColor;
  wdTexture2DResourceHandle m_hNormal;
  wdTexture2DResourceHandle m_hORM;
  wdVec2U32 m_vBaseColorSize;
  wdVec2U32 m_vNormalSize;
  wdVec2U32 m_vORMSize;
};
