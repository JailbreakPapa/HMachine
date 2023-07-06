#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/ezTexFormat/ezTexFormat.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdTextureCubeResource, 1, wdRTTIDefaultAllocator<wdTextureCubeResource>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_RESOURCE_IMPLEMENT_COMMON_CODE(wdTextureCubeResource);
// clang-format on

wdTextureCubeResource::wdTextureCubeResource()
  : wdResource(DoUpdate::OnAnyThread, wdTextureUtils::s_bForceFullQualityAlways ? 1 : 2)
{
  m_uiLoadedTextures = 0;
  m_uiMemoryGPU[0] = 0;
  m_uiMemoryGPU[1] = 0;
  m_Format = wdGALResourceFormat::Invalid;
  m_uiWidthAndHeight = 0;
}

wdResourceLoadDesc wdTextureCubeResource::UnloadData(Unload WhatToUnload)
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

wdResourceLoadDesc wdTextureCubeResource::UpdateContent(wdStreamReader* Stream)
{
  if (Stream == nullptr)
  {
    wdResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = wdResourceState::LoadedResourceMissing;

    return res;
  }

  wdImage* pImage = nullptr;
  Stream->ReadBytes(&pImage, sizeof(wdImage*));

  bool bIsFallback = false;
  *Stream >> bIsFallback;

  wdTexFormat texFormat;
  texFormat.ReadHeader(*Stream);

  const wdUInt32 uiNumMipmapsLowRes = wdTextureUtils::s_bForceFullQualityAlways ? pImage->GetNumMipLevels() : 6;

  const wdUInt32 uiNumMipLevels = wdMath::Min(m_uiLoadedTextures == 0 ? uiNumMipmapsLowRes : pImage->GetNumMipLevels(), pImage->GetNumMipLevels());
  const wdUInt32 uiHighestMipLevel = pImage->GetNumMipLevels() - uiNumMipLevels;

  if (pImage->GetWidth(uiHighestMipLevel) != pImage->GetHeight(uiHighestMipLevel))
  {
    wdLog::Error("Cubemap width '{0}' is not identical to height '{1}'", pImage->GetWidth(uiHighestMipLevel), pImage->GetHeight(uiHighestMipLevel));

    wdResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = wdResourceState::LoadedResourceMissing;

    return res;
  }

  m_Format = wdTextureUtils::ImageFormatToGalFormat(pImage->GetImageFormat(), texFormat.m_bSRGB);
  m_uiWidthAndHeight = pImage->GetWidth(uiHighestMipLevel);

  wdGALTextureCreationDescription texDesc;
  texDesc.m_Format = m_Format;
  texDesc.m_uiWidth = m_uiWidthAndHeight;
  texDesc.m_uiHeight = m_uiWidthAndHeight;
  texDesc.m_uiDepth = pImage->GetDepth(uiHighestMipLevel);
  texDesc.m_uiMipLevelCount = uiNumMipLevels;
  texDesc.m_uiArraySize = pImage->GetNumArrayIndices();

  if (texDesc.m_uiDepth > 1)
    texDesc.m_Type = wdGALTextureType::Texture3D;

  if (pImage->GetNumFaces() == 6)
    texDesc.m_Type = wdGALTextureType::TextureCube;

  WD_ASSERT_DEV(pImage->GetNumFaces() == 1 || pImage->GetNumFaces() == 6, "Invalid number of image faces (resource: '{0}')", GetResourceID());

  m_uiMemoryGPU[m_uiLoadedTextures] = 0;

  wdHybridArray<wdGALSystemMemoryDescription, 32> InitData;

  for (wdUInt32 array_index = 0; array_index < pImage->GetNumArrayIndices(); ++array_index)
  {
    for (wdUInt32 face = 0; face < pImage->GetNumFaces(); ++face)
    {
      for (wdUInt32 mip = uiHighestMipLevel; mip < pImage->GetNumMipLevels(); ++mip)
      {
        wdGALSystemMemoryDescription& id = InitData.ExpandAndGetRef();

        id.m_pData = pImage->GetPixelPointer<wdUInt8>(mip, face, array_index);

        WD_ASSERT_DEV(pImage->GetDepthPitch(mip) < wdMath::MaxValue<wdUInt32>(), "Depth pitch exceeds wdGAL limits.");

        if (wdImageFormat::GetType(pImage->GetImageFormat()) == wdImageFormatType::BLOCK_COMPRESSED)
        {
          const wdUInt32 uiMemPitchFactor = wdGALResourceFormat::GetBitsPerElement(m_Format) * 4 / 8;

          id.m_uiRowPitch = wdMath::Max<wdUInt32>(4, pImage->GetWidth(mip)) * uiMemPitchFactor;
        }
        else
        {
          id.m_uiRowPitch = static_cast<wdUInt32>(pImage->GetRowPitch(mip));
        }

        id.m_uiSlicePitch = static_cast<wdUInt32>(pImage->GetDepthPitch(mip));

        m_uiMemoryGPU[m_uiLoadedTextures] += id.m_uiSlicePitch;
      }
    }
  }

  const wdArrayPtr<wdGALSystemMemoryDescription> InitDataPtr(InitData);

  wdTextureCubeResourceDescriptor td;
  td.m_DescGAL = texDesc;
  td.m_SamplerDesc.m_AddressU = texFormat.m_AddressModeU;
  td.m_SamplerDesc.m_AddressV = texFormat.m_AddressModeV;
  td.m_SamplerDesc.m_AddressW = texFormat.m_AddressModeW;
  td.m_InitialContent = InitDataPtr;

  wdTextureUtils::ConfigureSampler(static_cast<wdTextureFilterSetting::Enum>(texFormat.m_TextureFilter.GetValue()), td.m_SamplerDesc);

  // ignore its return value here, we build our own
  CreateResource(std::move(td));

  {
    wdResourceLoadDesc res;
    res.m_uiQualityLevelsDiscardable = m_uiLoadedTextures;

    if (uiHighestMipLevel == 0)
      res.m_uiQualityLevelsLoadable = 0;
    else
      res.m_uiQualityLevelsLoadable = 1;

    res.m_State = wdResourceState::Loaded;

    return res;
  }
}

void wdTextureCubeResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(wdTextureCubeResource);
  out_NewMemoryUsage.m_uiMemoryGPU = m_uiMemoryGPU[0] + m_uiMemoryGPU[1];
}

WD_RESOURCE_IMPLEMENT_CREATEABLE(wdTextureCubeResource, wdTextureCubeResourceDescriptor)
{
  wdResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = descriptor.m_uiQualityLevelsDiscardable;
  ret.m_uiQualityLevelsLoadable = descriptor.m_uiQualityLevelsLoadable;
  ret.m_State = wdResourceState::Loaded;

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  WD_ASSERT_DEV(descriptor.m_DescGAL.m_uiWidth == descriptor.m_DescGAL.m_uiHeight, "Cubemap width and height must be identical");

  m_Format = descriptor.m_DescGAL.m_Format;
  m_uiWidthAndHeight = descriptor.m_DescGAL.m_uiWidth;

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



WD_STATICLINK_FILE(RendererCore, RendererCore_Textures_TextureCubeResource);
