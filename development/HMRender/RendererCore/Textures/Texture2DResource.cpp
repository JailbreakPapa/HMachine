#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/ezTexFormat/ezTexFormat.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdTexture2DResource, 1, wdRTTIDefaultAllocator<wdTexture2DResource>)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdCVarInt cvar_RenderingOffscreenTargetResolution1("Rendering.Offscreen.TargetResolution1", 256, wdCVarFlags::Default, "Configurable render target resolution");
wdCVarInt cvar_RenderingOffscreenTargetResolution2("Rendering.Offscreen.TargetResolution2", 512, wdCVarFlags::Default, "Configurable render target resolution");

WD_RESOURCE_IMPLEMENT_COMMON_CODE(wdTexture2DResource);

wdTexture2DResource::wdTexture2DResource()
  : wdResource(DoUpdate::OnAnyThread, wdTextureUtils::s_bForceFullQualityAlways ? 1 : 2)
{
}

wdTexture2DResource::wdTexture2DResource(wdResource::DoUpdate ResourceUpdateThread)
  : wdResource(ResourceUpdateThread, wdTextureUtils::s_bForceFullQualityAlways ? 1 : 2)
{
}

wdResourceLoadDesc wdTexture2DResource::UnloadData(Unload WhatToUnload)
{
  if (m_uiLoadedTextures > 0)
  {
    for (wdInt32 r = 0; r < 2; ++r)
    {
      --m_uiLoadedTextures;

      if (!m_hGALTexture[m_uiLoadedTextures].IsInvalidated())
      {
        wdGALDevice::GetDefaultDevice()->DestroyTexture(m_hGALTexture[m_uiLoadedTextures]);
        m_hGALTexture[m_uiLoadedTextures].Invalidate();
      }

      m_uiMemoryGPU[m_uiLoadedTextures] = 0;

      if (WhatToUnload == Unload::OneQualityLevel || m_uiLoadedTextures == 0)
        break;
    }
  }

  if (WhatToUnload == Unload::AllQualityLevels)
  {
    if (!m_hSamplerState.IsInvalidated())
    {
      wdGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSamplerState);
      m_hSamplerState.Invalidate();
    }
  }

  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = m_uiLoadedTextures;
  res.m_uiQualityLevelsLoadable = 2 - m_uiLoadedTextures;
  res.m_State = m_uiLoadedTextures == 0 ? wdResourceState::Unloaded : wdResourceState::Loaded;
  return res;
}

void wdTexture2DResource::FillOutDescriptor(wdTexture2DResourceDescriptor& ref_td, const wdImage* pImage, bool bSRGB, wdUInt32 uiNumMipLevels,
  wdUInt32& out_uiMemoryUsed, wdHybridArray<wdGALSystemMemoryDescription, 32>& ref_initData)
{
  const wdUInt32 uiHighestMipLevel = pImage->GetNumMipLevels() - uiNumMipLevels;

  const wdGALResourceFormat::Enum format = wdTextureUtils::ImageFormatToGalFormat(pImage->GetImageFormat(), bSRGB);

  ref_td.m_DescGAL.m_Format = format;
  ref_td.m_DescGAL.m_uiWidth = pImage->GetWidth(uiHighestMipLevel);
  ref_td.m_DescGAL.m_uiHeight = pImage->GetHeight(uiHighestMipLevel);
  ref_td.m_DescGAL.m_uiDepth = pImage->GetDepth(uiHighestMipLevel);
  ref_td.m_DescGAL.m_uiMipLevelCount = uiNumMipLevels;
  ref_td.m_DescGAL.m_uiArraySize = pImage->GetNumArrayIndices();

  if (wdImageFormat::GetType(pImage->GetImageFormat()) == wdImageFormatType::BLOCK_COMPRESSED)
  {
    ref_td.m_DescGAL.m_uiWidth = wdMath::RoundUp(ref_td.m_DescGAL.m_uiWidth, 4);
    ref_td.m_DescGAL.m_uiHeight = wdMath::RoundUp(ref_td.m_DescGAL.m_uiHeight, 4);
  }

  if (ref_td.m_DescGAL.m_uiDepth > 1)
    ref_td.m_DescGAL.m_Type = wdGALTextureType::Texture3D;

  if (pImage->GetNumFaces() == 6)
    ref_td.m_DescGAL.m_Type = wdGALTextureType::TextureCube;

  WD_ASSERT_DEV(pImage->GetNumFaces() == 1 || pImage->GetNumFaces() == 6, "Invalid number of image faces");

  out_uiMemoryUsed = 0;

  ref_initData.Clear();

  for (wdUInt32 array_index = 0; array_index < pImage->GetNumArrayIndices(); ++array_index)
  {
    for (wdUInt32 face = 0; face < pImage->GetNumFaces(); ++face)
    {
      for (wdUInt32 mip = uiHighestMipLevel; mip < pImage->GetNumMipLevels(); ++mip)
      {
        wdGALSystemMemoryDescription& id = ref_initData.ExpandAndGetRef();

        id.m_pData = const_cast<wdUInt8*>(pImage->GetPixelPointer<wdUInt8>(mip, face, array_index));

        if (wdImageFormat::GetType(pImage->GetImageFormat()) == wdImageFormatType::BLOCK_COMPRESSED)
        {
          const wdUInt32 uiMemPitchFactor = wdGALResourceFormat::GetBitsPerElement(format) * 4 / 8;

          id.m_uiRowPitch = wdMath::RoundUp(pImage->GetWidth(mip), 4) * uiMemPitchFactor;
        }
        else
        {
          id.m_uiRowPitch = static_cast<wdUInt32>(pImage->GetRowPitch(mip));
        }

        WD_ASSERT_DEV(pImage->GetDepthPitch(mip) < wdMath::MaxValue<wdUInt32>(), "Depth pitch exceeds wdGAL limits.");
        id.m_uiSlicePitch = static_cast<wdUInt32>(pImage->GetDepthPitch(mip));

        out_uiMemoryUsed += id.m_uiSlicePitch;
      }
    }
  }

  const wdArrayPtr<wdGALSystemMemoryDescription> InitDataPtr(ref_initData);

  ref_td.m_InitialContent = InitDataPtr;
}


wdResourceLoadDesc wdTexture2DResource::UpdateContent(wdStreamReader* Stream)
{
  if (Stream == nullptr)
  {
    wdResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = wdResourceState::LoadedResourceMissing;

    return res;
  }

  wdTexture2DResourceDescriptor td;
  wdImage* pImage = nullptr;
  bool bIsFallback = false;
  wdTexFormat texFormat;

  // load image data
  {
    Stream->ReadBytes(&pImage, sizeof(wdImage*));
    *Stream >> bIsFallback;
    texFormat.ReadHeader(*Stream);

    td.m_SamplerDesc.m_AddressU = texFormat.m_AddressModeU;
    td.m_SamplerDesc.m_AddressV = texFormat.m_AddressModeV;
    td.m_SamplerDesc.m_AddressW = texFormat.m_AddressModeW;
  }

  const bool bIsRenderTarget = texFormat.m_iRenderTargetResolutionX != 0;
  WD_ASSERT_DEV(!bIsRenderTarget, "Render targets are not supported by regular 2D texture resources");

  {

    const wdUInt32 uiNumMipmapsLowRes = wdTextureUtils::s_bForceFullQualityAlways ? pImage->GetNumMipLevels() : wdMath::Min(pImage->GetNumMipLevels(), 6U);
    wdUInt32 uiUploadNumMipLevels = 0;
    bool bCouldLoadMore = false;

    if (bIsFallback)
    {
      if (m_uiLoadedTextures == 0)
      {
        // only upload fallback textures, if we don't have any texture data at all, yet
        bCouldLoadMore = true;
        uiUploadNumMipLevels = uiNumMipmapsLowRes;
      }
      else if (m_uiLoadedTextures == 1)
      {
        // ignore this texture entirely, if we already have low res data
        // but assume we could load a higher resolution version
        bCouldLoadMore = true;
        wdLog::Debug("Ignoring fallback texture data, low-res resource data is already loaded.");
      }
      else
      {
        wdLog::Debug("Ignoring fallback texture data, resource is already fully loaded.");
      }
    }
    else
    {
      if (m_uiLoadedTextures == 0)
      {
        bCouldLoadMore = uiNumMipmapsLowRes < pImage->GetNumMipLevels();
        uiUploadNumMipLevels = uiNumMipmapsLowRes;
      }
      else if (m_uiLoadedTextures == 1)
      {
        uiUploadNumMipLevels = pImage->GetNumMipLevels();
      }
      else
      {
        // ignore the texture, if we already have fully loaded data
        wdLog::Debug("Ignoring texture data, resource is already fully loaded.");
      }
    }

    if (uiUploadNumMipLevels > 0)
    {
      WD_ASSERT_DEBUG(m_uiLoadedTextures < 2, "Invalid texture upload");

      wdHybridArray<wdGALSystemMemoryDescription, 32> initData;
      FillOutDescriptor(td, pImage, texFormat.m_bSRGB, uiUploadNumMipLevels, m_uiMemoryGPU[m_uiLoadedTextures], initData);

      wdTextureUtils::ConfigureSampler(static_cast<wdTextureFilterSetting::Enum>(texFormat.m_TextureFilter.GetValue()), td.m_SamplerDesc);

      // ignore its return value here, we build our own
      CreateResource(std::move(td));
    }

    {
      wdResourceLoadDesc res;
      res.m_uiQualityLevelsDiscardable = m_uiLoadedTextures;
      res.m_uiQualityLevelsLoadable = bCouldLoadMore ? 1 : 0;
      res.m_State = wdResourceState::Loaded;

      return res;
    }
  }
}

void wdTexture2DResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(wdTexture2DResource);
  out_NewMemoryUsage.m_uiMemoryGPU = m_uiMemoryGPU[0] + m_uiMemoryGPU[1];
}

WD_RESOURCE_IMPLEMENT_CREATEABLE(wdTexture2DResource, wdTexture2DResourceDescriptor)
{
  wdResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = descriptor.m_uiQualityLevelsDiscardable;
  ret.m_uiQualityLevelsLoadable = descriptor.m_uiQualityLevelsLoadable;
  ret.m_State = wdResourceState::Loaded;

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  m_Type = descriptor.m_DescGAL.m_Type;
  m_Format = descriptor.m_DescGAL.m_Format;
  m_uiWidth = descriptor.m_DescGAL.m_uiWidth;
  m_uiHeight = descriptor.m_DescGAL.m_uiHeight;

  m_hGALTexture[m_uiLoadedTextures] = pDevice->CreateTexture(descriptor.m_DescGAL, descriptor.m_InitialContent);
  WD_ASSERT_DEV(!m_hGALTexture[m_uiLoadedTextures].IsInvalidated(), "Texture Data could not be uploaded to the GPU");

  pDevice->GetTexture(m_hGALTexture[m_uiLoadedTextures])->SetDebugName(GetResourceDescription());

  if (!m_hSamplerState.IsInvalidated())
  {
    pDevice->DestroySamplerState(m_hSamplerState);
  }

  m_hSamplerState = pDevice->CreateSamplerState(descriptor.m_SamplerDesc);
  WD_ASSERT_DEV(!m_hSamplerState.IsInvalidated(), "Sampler state error");

  ++m_uiLoadedTextures;

  return ret;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// TODO (resources): move into separate file

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdRenderToTexture2DResource, 1, wdRTTIDefaultAllocator<wdRenderToTexture2DResource>);
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, Texture2D)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ResourceManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    wdResourceManager::RegisterResourceOverrideType(wdGetStaticRTTI<wdRenderToTexture2DResource>(), [](const wdStringBuilder& sResourceID) -> bool  {
        return sResourceID.HasExtension(".wdRenderTarget");
      });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    wdResourceManager::UnregisterResourceOverrideType(wdGetStaticRTTI<wdRenderToTexture2DResource>());
  }

WD_END_SUBSYSTEM_DECLARATION;
// clang-format on

WD_RESOURCE_IMPLEMENT_COMMON_CODE(wdRenderToTexture2DResource);

WD_RESOURCE_IMPLEMENT_CREATEABLE(wdRenderToTexture2DResource, wdRenderToTexture2DResourceDescriptor)
{
  wdResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable = 0;
  ret.m_State = wdResourceState::Loaded;

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  m_Type = wdGALTextureType::Texture2D;
  m_Format = descriptor.m_Format;
  m_uiWidth = descriptor.m_uiWidth;
  m_uiHeight = descriptor.m_uiHeight;

  wdGALTextureCreationDescription descGAL;
  descGAL.SetAsRenderTarget(m_uiWidth, m_uiHeight, m_Format, descriptor.m_SampleCount);

  m_hGALTexture[m_uiLoadedTextures] = pDevice->CreateTexture(descGAL, descriptor.m_InitialContent);
  WD_ASSERT_DEV(!m_hGALTexture[m_uiLoadedTextures].IsInvalidated(), "Texture data could not be uploaded to the GPU");

  pDevice->GetTexture(m_hGALTexture[m_uiLoadedTextures])->SetDebugName(GetResourceDescription());

  if (!m_hSamplerState.IsInvalidated())
  {
    pDevice->DestroySamplerState(m_hSamplerState);
  }

  m_hSamplerState = pDevice->CreateSamplerState(descriptor.m_SamplerDesc);
  WD_ASSERT_DEV(!m_hSamplerState.IsInvalidated(), "Sampler state error");

  ++m_uiLoadedTextures;

  return ret;
}

wdResourceLoadDesc wdRenderToTexture2DResource::UnloadData(Unload WhatToUnload)
{
  for (wdInt32 r = 0; r < 2; ++r)
  {
    if (!m_hGALTexture[r].IsInvalidated())
    {
      wdGALDevice::GetDefaultDevice()->DestroyTexture(m_hGALTexture[r]);
      m_hGALTexture[r].Invalidate();
    }

    m_uiMemoryGPU[r] = 0;
  }

  m_uiLoadedTextures = 0;

  if (!m_hSamplerState.IsInvalidated())
  {
    wdGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSamplerState);
    m_hSamplerState.Invalidate();
  }

  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = m_uiLoadedTextures;
  res.m_uiQualityLevelsLoadable = 2 - m_uiLoadedTextures;
  res.m_State = wdResourceState::Unloaded;
  return res;
}

wdGALRenderTargetViewHandle wdRenderToTexture2DResource::GetRenderTargetView() const
{
  return wdGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(m_hGALTexture[0]);
}

void wdRenderToTexture2DResource::AddRenderView(wdViewHandle hView)
{
  m_RenderViews.PushBack(hView);
}

void wdRenderToTexture2DResource::RemoveRenderView(wdViewHandle hView)
{
  m_RenderViews.RemoveAndSwap(hView);
}

const wdDynamicArray<wdViewHandle>& wdRenderToTexture2DResource::GetAllRenderViews() const
{
  return m_RenderViews;
}

static wdUInt16 GetNextBestResolution(float fRes)
{
  fRes = wdMath::Clamp(fRes, 8.0f, 4096.0f);

  int mulEight = (int)wdMath::Floor((fRes + 7.9f) / 8.0f);

  return static_cast<wdUInt16>(mulEight * 8);
}

wdResourceLoadDesc wdRenderToTexture2DResource::UpdateContent(wdStreamReader* Stream)
{
  if (Stream == nullptr)
  {
    wdResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = wdResourceState::LoadedResourceMissing;

    return res;
  }

  wdRenderToTexture2DResourceDescriptor td;
  wdImage* pImage = nullptr;
  bool bIsFallback = false;
  wdTexFormat texFormat;

  // load image data
  {
    Stream->ReadBytes(&pImage, sizeof(wdImage*));
    *Stream >> bIsFallback;
    texFormat.ReadHeader(*Stream);

    td.m_SamplerDesc.m_AddressU = texFormat.m_AddressModeU;
    td.m_SamplerDesc.m_AddressV = texFormat.m_AddressModeV;
    td.m_SamplerDesc.m_AddressW = texFormat.m_AddressModeW;
  }

  const bool bIsRenderTarget = texFormat.m_iRenderTargetResolutionX != 0;

  WD_ASSERT_DEV(bIsRenderTarget, "Trying to create a RenderToTexture resource from data that is not set up as a render-target");

  {
    WD_ASSERT_DEV(m_uiLoadedTextures == 0, "not implemented");

    if (texFormat.m_iRenderTargetResolutionX == -1)
    {
      if (texFormat.m_iRenderTargetResolutionY == 1)
      {
        texFormat.m_iRenderTargetResolutionX = GetNextBestResolution(cvar_RenderingOffscreenTargetResolution1 * texFormat.m_fResolutionScale);
        texFormat.m_iRenderTargetResolutionY = texFormat.m_iRenderTargetResolutionX;
      }
      else if (texFormat.m_iRenderTargetResolutionY == 2)
      {
        texFormat.m_iRenderTargetResolutionX = GetNextBestResolution(cvar_RenderingOffscreenTargetResolution2 * texFormat.m_fResolutionScale);
        texFormat.m_iRenderTargetResolutionY = texFormat.m_iRenderTargetResolutionX;
      }
      else
      {
        WD_REPORT_FAILURE(
          "Invalid render target configuration: {0} x {1}", texFormat.m_iRenderTargetResolutionX, texFormat.m_iRenderTargetResolutionY);
      }
    }

    td.m_Format = static_cast<wdGALResourceFormat::Enum>(texFormat.m_GalRenderTargetFormat);
    td.m_uiWidth = texFormat.m_iRenderTargetResolutionX;
    td.m_uiHeight = texFormat.m_iRenderTargetResolutionY;

    wdTextureUtils::ConfigureSampler(static_cast<wdTextureFilterSetting::Enum>(texFormat.m_TextureFilter.GetValue()), td.m_SamplerDesc);

    m_uiLoadedTextures = 0;

    CreateResource(std::move(td));
  }

  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::Loaded;
  return res;
}

void wdRenderToTexture2DResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(wdRenderToTexture2DResource);
  out_NewMemoryUsage.m_uiMemoryGPU = m_uiMemoryGPU[0] + m_uiMemoryGPU[1];
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Textures_Texture2DResource);
