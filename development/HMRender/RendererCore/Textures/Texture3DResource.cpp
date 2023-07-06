
#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>

#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Image.h>

#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/ezTexFormat/ezTexFormat.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdTexture3DResource, 1, wdRTTIDefaultAllocator<wdTexture3DResource>)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WD_RESOURCE_IMPLEMENT_COMMON_CODE(wdTexture3DResource);

wdTexture3DResource::wdTexture3DResource()
  : wdResource(DoUpdate::OnAnyThread, wdTextureUtils::s_bForceFullQualityAlways ? 1 : 2)
{
}

wdTexture3DResource::wdTexture3DResource(wdResource::DoUpdate ResourceUpdateThread)
  : wdResource(ResourceUpdateThread, wdTextureUtils::s_bForceFullQualityAlways ? 1 : 2)
{
}

wdResourceLoadDesc wdTexture3DResource::UnloadData(Unload WhatToUnload)
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

void wdTexture3DResource::FillOutDescriptor(wdTexture3DResourceDescriptor& ref_td, const wdImage* pImage, bool bSRGB, wdUInt32 uiNumMipLevels,
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

  if (ref_td.m_DescGAL.m_uiDepth > 1)
    ref_td.m_DescGAL.m_Type = wdGALTextureType::Texture3D;

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

          id.m_uiRowPitch = wdMath::Max<wdUInt32>(4, pImage->GetWidth(mip)) * uiMemPitchFactor;
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


wdResourceLoadDesc wdTexture3DResource::UpdateContent(wdStreamReader* Stream)
{
  if (Stream == nullptr)
  {
    wdResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = wdResourceState::LoadedResourceMissing;

    return res;
  }

  wdTexture3DResourceDescriptor td;
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

    const wdUInt32 uiNumMipmapsLowRes =
      wdTextureUtils::s_bForceFullQualityAlways ? pImage->GetNumMipLevels() : wdMath::Min(pImage->GetNumMipLevels(), 6U);
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

void wdTexture3DResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(wdTexture3DResource);
  out_NewMemoryUsage.m_uiMemoryGPU = m_uiMemoryGPU[0] + m_uiMemoryGPU[1];
}

WD_RESOURCE_IMPLEMENT_CREATEABLE(wdTexture3DResource, wdTexture3DResourceDescriptor)
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
  m_uiDepth = descriptor.m_DescGAL.m_uiDepth;

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

WD_STATICLINK_FILE(RendererCore, RendererCore_Textures_Texture3DResource);
