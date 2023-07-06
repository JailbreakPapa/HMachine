#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/IO/MemoryStream.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <Texture/Image/Image.h>

using wdTextureCubeResourceHandle = wdTypedResourceHandle<class wdTextureCubeResource>;

/// \brief Use this descriptor in calls to wdResourceManager::CreateResource<wdTextureCubeResource> to create textures from data in memory.
struct wdTextureCubeResourceDescriptor
{
  wdTextureCubeResourceDescriptor()
  {
    m_uiQualityLevelsDiscardable = 0;
    m_uiQualityLevelsLoadable = 0;
  }

  /// Describes the texture format, etc.
  wdGALTextureCreationDescription m_DescGAL;
  wdGALSamplerStateCreationDescription m_SamplerDesc;

  /// How many quality levels can be discarded and reloaded. For created textures this can currently only be 0 or 1.
  wdUInt8 m_uiQualityLevelsDiscardable;

  /// How many additional quality levels can be loaded (typically from file).
  wdUInt8 m_uiQualityLevelsLoadable;

  /// One memory desc per (array * faces * mipmap) (in that order) (array is outer loop, mipmap is inner loop). Can be empty to not
  /// initialize data.
  wdArrayPtr<wdGALSystemMemoryDescription> m_InitialContent;
};

class WD_RENDERERCORE_DLL wdTextureCubeResource : public wdResource
{
  WD_ADD_DYNAMIC_REFLECTION(wdTextureCubeResource, wdResource);
  WD_RESOURCE_DECLARE_COMMON_CODE(wdTextureCubeResource);
  WD_RESOURCE_DECLARE_CREATEABLE(wdTextureCubeResource, wdTextureCubeResourceDescriptor);

public:
  wdTextureCubeResource();

  WD_ALWAYS_INLINE wdGALResourceFormat::Enum GetFormat() const { return m_Format; }
  WD_ALWAYS_INLINE wdUInt32 GetWidthAndHeight() const { return m_uiWidthAndHeight; }

  const wdGALTextureHandle& GetGALTexture() const { return m_hGALTexture[m_uiLoadedTextures - 1]; }
  const wdGALSamplerStateHandle& GetGALSamplerState() const { return m_hSamplerState; }

protected:
  virtual wdResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual wdResourceLoadDesc UpdateContent(wdStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  wdUInt8 m_uiLoadedTextures;
  wdGALTextureHandle m_hGALTexture[2];
  wdUInt32 m_uiMemoryGPU[2];

  wdGALResourceFormat::Enum m_Format;
  wdUInt32 m_uiWidthAndHeight;

  wdGALSamplerStateHandle m_hSamplerState;
};
