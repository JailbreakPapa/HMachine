#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/IO/MemoryStream.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>

class wdImage;

using wdTexture3DResourceHandle = wdTypedResourceHandle<class wdTexture3DResource>;

/// \brief Use this descriptor in calls to wdResourceManager::CreateResource<wdTexture3DResource> to create textures from data in memory.
struct WD_RENDERERCORE_DLL wdTexture3DResourceDescriptor
{
  /// Describes the texture format, etc.
  wdGALTextureCreationDescription m_DescGAL;
  wdGALSamplerStateCreationDescription m_SamplerDesc;

  /// How many quality levels can be discarded and reloaded. For created textures this can currently only be 0 or 1.
  wdUInt8 m_uiQualityLevelsDiscardable = 0;

  /// How many additional quality levels can be loaded (typically from file).
  wdUInt8 m_uiQualityLevelsLoadable = 0;

  /// One memory desc per (array * faces * mipmap) (in that order) (array is outer loop, mipmap is inner loop). Can be empty to not
  /// initialize data.
  wdArrayPtr<wdGALSystemMemoryDescription> m_InitialContent;
};

class WD_RENDERERCORE_DLL wdTexture3DResource : public wdResource
{
  WD_ADD_DYNAMIC_REFLECTION(wdTexture3DResource, wdResource);

  WD_RESOURCE_DECLARE_COMMON_CODE(wdTexture3DResource);
  WD_RESOURCE_DECLARE_CREATEABLE(wdTexture3DResource, wdTexture3DResourceDescriptor);

public:
  wdTexture3DResource();

  WD_ALWAYS_INLINE wdGALResourceFormat::Enum GetFormat() const { return m_Format; }
  WD_ALWAYS_INLINE wdUInt32 GetWidth() const { return m_uiWidth; }
  WD_ALWAYS_INLINE wdUInt32 GetHeight() const { return m_uiHeight; }
  WD_ALWAYS_INLINE wdUInt32 GetDepth() const { return m_uiDepth; }
  WD_ALWAYS_INLINE wdGALTextureType::Enum GetType() const { return m_Type; }

  static void FillOutDescriptor(wdTexture3DResourceDescriptor& ref_td, const wdImage* pImage, bool bSRGB, wdUInt32 uiNumMipLevels,
    wdUInt32& out_uiMemoryUsed, wdHybridArray<wdGALSystemMemoryDescription, 32>& ref_initData);

  const wdGALTextureHandle& GetGALTexture() const { return m_hGALTexture[m_uiLoadedTextures - 1]; }
  const wdGALSamplerStateHandle& GetGALSamplerState() const { return m_hSamplerState; }

protected:
  virtual wdResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual wdResourceLoadDesc UpdateContent(wdStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  wdTexture3DResource(DoUpdate ResourceUpdateThread);

  wdUInt8 m_uiLoadedTextures = 0;
  wdGALTextureHandle m_hGALTexture[2];
  wdUInt32 m_uiMemoryGPU[2] = {0, 0};

  wdGALTextureType::Enum m_Type = wdGALTextureType::Invalid;
  wdGALResourceFormat::Enum m_Format = wdGALResourceFormat::Invalid;
  wdUInt32 m_uiWidth = 0;
  wdUInt32 m_uiHeight = 0;
  wdUInt32 m_uiDepth = 0;

  wdGALSamplerStateHandle m_hSamplerState;
};
