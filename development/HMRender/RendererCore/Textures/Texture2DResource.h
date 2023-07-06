#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/IO/MemoryStream.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class wdImage;

using wdTexture2DResourceHandle = wdTypedResourceHandle<class wdTexture2DResource>;

/// \brief Use this descriptor in calls to wdResourceManager::CreateResource<wdTexture2DResource> to create textures from data in memory.
struct WD_RENDERERCORE_DLL wdTexture2DResourceDescriptor
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

class WD_RENDERERCORE_DLL wdTexture2DResource : public wdResource
{
  WD_ADD_DYNAMIC_REFLECTION(wdTexture2DResource, wdResource);

  WD_RESOURCE_DECLARE_COMMON_CODE(wdTexture2DResource);
  WD_RESOURCE_DECLARE_CREATEABLE(wdTexture2DResource, wdTexture2DResourceDescriptor);

public:
  wdTexture2DResource();

  WD_ALWAYS_INLINE wdGALResourceFormat::Enum GetFormat() const { return m_Format; }
  WD_ALWAYS_INLINE wdUInt32 GetWidth() const { return m_uiWidth; }
  WD_ALWAYS_INLINE wdUInt32 GetHeight() const { return m_uiHeight; }
  WD_ALWAYS_INLINE wdGALTextureType::Enum GetType() const { return m_Type; }

  static void FillOutDescriptor(wdTexture2DResourceDescriptor& ref_td, const wdImage* pImage, bool bSRGB, wdUInt32 uiNumMipLevels,
    wdUInt32& out_uiMemoryUsed, wdHybridArray<wdGALSystemMemoryDescription, 32>& ref_initData);

  const wdGALTextureHandle& GetGALTexture() const { return m_hGALTexture[m_uiLoadedTextures - 1]; }
  const wdGALSamplerStateHandle& GetGALSamplerState() const { return m_hSamplerState; }

protected:
  virtual wdResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual wdResourceLoadDesc UpdateContent(wdStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  wdTexture2DResource(DoUpdate ResourceUpdateThread);

  wdUInt8 m_uiLoadedTextures = 0;
  wdGALTextureHandle m_hGALTexture[2];
  wdUInt32 m_uiMemoryGPU[2] = {0, 0};

  wdGALTextureType::Enum m_Type = wdGALTextureType::Invalid;
  wdGALResourceFormat::Enum m_Format = wdGALResourceFormat::Invalid;
  wdUInt32 m_uiWidth = 0;
  wdUInt32 m_uiHeight = 0;

  wdGALSamplerStateHandle m_hSamplerState;
};

//////////////////////////////////////////////////////////////////////////

using wdRenderToTexture2DResourceHandle = wdTypedResourceHandle<class wdRenderToTexture2DResource>;

struct WD_RENDERERCORE_DLL wdRenderToTexture2DResourceDescriptor
{
  wdUInt32 m_uiWidth = 0;
  wdUInt32 m_uiHeight = 0;
  wdEnum<wdGALMSAASampleCount> m_SampleCount;
  wdEnum<wdGALResourceFormat> m_Format;
  wdGALSamplerStateCreationDescription m_SamplerDesc;
  wdArrayPtr<wdGALSystemMemoryDescription> m_InitialContent;
};

class WD_RENDERERCORE_DLL wdRenderToTexture2DResource : public wdTexture2DResource
{
  WD_ADD_DYNAMIC_REFLECTION(wdRenderToTexture2DResource, wdTexture2DResource);

  WD_RESOURCE_DECLARE_COMMON_CODE(wdRenderToTexture2DResource);
  WD_RESOURCE_DECLARE_CREATEABLE(wdRenderToTexture2DResource, wdRenderToTexture2DResourceDescriptor);

public:
  wdGALRenderTargetViewHandle GetRenderTargetView() const;
  void AddRenderView(wdViewHandle hView);
  void RemoveRenderView(wdViewHandle hView);
  const wdDynamicArray<wdViewHandle>& GetAllRenderViews() const;

private:
  virtual wdResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual wdResourceLoadDesc UpdateContent(wdStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

protected:
  // other views that use this texture as their target
  wdDynamicArray<wdViewHandle> m_RenderViews;
};
