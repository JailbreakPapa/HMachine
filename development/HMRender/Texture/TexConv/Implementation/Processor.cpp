#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

// clang=format off
WD_BEGIN_STATIC_REFLECTED_ENUM(wdTexConvCompressionMode, 1)
  WD_ENUM_CONSTANTS(wdTexConvCompressionMode::None, wdTexConvCompressionMode::Medium, wdTexConvCompressionMode::High)
WD_END_STATIC_REFLECTED_ENUM;

WD_BEGIN_STATIC_REFLECTED_ENUM(wdTexConvMipmapMode, 1)
  WD_ENUM_CONSTANTS(wdTexConvMipmapMode::None, wdTexConvMipmapMode::Linear, wdTexConvMipmapMode::Kaiser)
WD_END_STATIC_REFLECTED_ENUM;

WD_BEGIN_STATIC_REFLECTED_ENUM(wdTexConvUsage, 1)
  WD_ENUM_CONSTANT(wdTexConvUsage::Auto), WD_ENUM_CONSTANT(wdTexConvUsage::Color), WD_ENUM_CONSTANT(wdTexConvUsage::Linear),
    WD_ENUM_CONSTANT(wdTexConvUsage::Hdr), WD_ENUM_CONSTANT(wdTexConvUsage::NormalMap), WD_ENUM_CONSTANT(wdTexConvUsage::NormalMap_Inverted),
    WD_ENUM_CONSTANT(wdTexConvUsage::BumpMap),
WD_END_STATIC_REFLECTED_ENUM;
// clang=format on

wdTexConvProcessor::wdTexConvProcessor() = default;

wdResult wdTexConvProcessor::Process()
{
  WD_PROFILE_SCOPE("wdTexConvProcessor::Process");

  if (m_Descriptor.m_OutputType == wdTexConvOutputType::Atlas)
  {
    wdMemoryStreamWriter stream(&m_TextureAtlas);
    WD_SUCCEED_OR_RETURN(GenerateTextureAtlas(stream));
  }
  else
  {
    WD_SUCCEED_OR_RETURN(LoadInputImages());

    WD_SUCCEED_OR_RETURN(AdjustUsage(m_Descriptor.m_InputFiles[0], m_Descriptor.m_InputImages[0], m_Descriptor.m_Usage));

    wdStringBuilder sUsage;
    wdReflectionUtils::EnumerationToString(
      wdGetStaticRTTI<wdTexConvUsage>(), m_Descriptor.m_Usage.GetValue(), sUsage, wdReflectionUtils::EnumConversionMode::ValueNameOnly);
    wdLog::Info("-usage is '{}'", sUsage);

    WD_SUCCEED_OR_RETURN(ForceSRGBFormats());

    wdUInt32 uiNumChannelsUsed = 0;
    WD_SUCCEED_OR_RETURN(DetectNumChannels(m_Descriptor.m_ChannelMappings, uiNumChannelsUsed));

    wdEnum<wdImageFormat> OutputImageFormat;

    WD_SUCCEED_OR_RETURN(ChooseOutputFormat(OutputImageFormat, m_Descriptor.m_Usage, uiNumChannelsUsed));

    wdLog::Info("Output image format is '{}'", wdImageFormat::GetName(OutputImageFormat));

    wdUInt32 uiTargetResolutionX = 0;
    wdUInt32 uiTargetResolutionY = 0;

    WD_SUCCEED_OR_RETURN(DetermineTargetResolution(m_Descriptor.m_InputImages[0], OutputImageFormat, uiTargetResolutionX, uiTargetResolutionY));

    wdLog::Info("Target resolution is '{} x {}'", uiTargetResolutionX, uiTargetResolutionY);

    WD_SUCCEED_OR_RETURN(ConvertAndScaleInputImages(uiTargetResolutionX, uiTargetResolutionY, m_Descriptor.m_Usage));

    WD_SUCCEED_OR_RETURN(ClampInputValues(m_Descriptor.m_InputImages, m_Descriptor.m_fMaxValue));

    if (m_Descriptor.m_Usage == wdTexConvUsage::BumpMap)
    {
      WD_SUCCEED_OR_RETURN(ConvertToNormalMap(m_Descriptor.m_InputImages));
      m_Descriptor.m_Usage = wdTexConvUsage::NormalMap;
    }

    wdImage assembledImg;
    if (m_Descriptor.m_OutputType == wdTexConvOutputType::Texture2D || m_Descriptor.m_OutputType == wdTexConvOutputType::None)
    {
      WD_SUCCEED_OR_RETURN(Assemble2DTexture(m_Descriptor.m_InputImages[0].GetHeader(), assembledImg));

      WD_SUCCEED_OR_RETURN(DilateColor2D(assembledImg));
    }
    else if (m_Descriptor.m_OutputType == wdTexConvOutputType::Cubemap)
    {
      WD_SUCCEED_OR_RETURN(AssembleCubemap(assembledImg));
    }
    else if (m_Descriptor.m_OutputType == wdTexConvOutputType::Volume)
    {
      WD_SUCCEED_OR_RETURN(Assemble3DTexture(assembledImg));
    }

    WD_SUCCEED_OR_RETURN(AdjustHdrExposure(assembledImg));

    WD_SUCCEED_OR_RETURN(GenerateMipmaps(assembledImg, 0, uiNumChannelsUsed == 1 ? MipmapChannelMode::SingleChannel : MipmapChannelMode::AllChannels));

    WD_SUCCEED_OR_RETURN(PremultiplyAlpha(assembledImg));

    WD_SUCCEED_OR_RETURN(GenerateOutput(std::move(assembledImg), m_OutputImage, OutputImageFormat));

    WD_SUCCEED_OR_RETURN(GenerateThumbnailOutput(m_OutputImage, m_ThumbnailOutputImage, m_Descriptor.m_uiThumbnailOutputResolution));

    WD_SUCCEED_OR_RETURN(GenerateLowResOutput(m_OutputImage, m_LowResOutputImage, m_Descriptor.m_uiLowResMipmaps));
  }

  return WD_SUCCESS;
}

wdResult wdTexConvProcessor::DetectNumChannels(wdArrayPtr<const wdTexConvSliceChannelMapping> channelMapping, wdUInt32& uiNumChannels)
{
  WD_PROFILE_SCOPE("DetectNumChannels");

  uiNumChannels = 0;

  for (const auto& mapping : channelMapping)
  {
    for (wdUInt32 i = 0; i < 4; ++i)
    {
      if (mapping.m_Channel[i].m_iInputImageIndex != -1 || mapping.m_Channel[i].m_ChannelValue == wdTexConvChannelValue::Black)
      {
        uiNumChannels = wdMath::Max(uiNumChannels, i + 1);
      }
    }
  }

  if (uiNumChannels == 0)
  {
    wdLog::Error("No proper channel mapping provided.");
    return WD_FAILURE;
  }

  // special case handling to detect when the alpha channel will end up white anyway and thus uiNumChannels could be 3 instead of 4
  // which enables us to use more optimized output formats
  if (uiNumChannels == 4)
  {
    uiNumChannels = 3;

    for (const auto& mapping : channelMapping)
    {
      if (mapping.m_Channel[3].m_ChannelValue == wdTexConvChannelValue::Black)
      {
        // sampling a texture without an alpha channel always returns 1, so to use all 0, we do need the channel
        uiNumChannels = 4;
        return WD_SUCCESS;
      }

      if (mapping.m_Channel[3].m_iInputImageIndex == -1)
      {
        // no fourth channel is needed for this
        continue;
      }

      wdImage& img = m_Descriptor.m_InputImages[mapping.m_Channel[3].m_iInputImageIndex];

      const wdUInt32 uiNumRequiredChannels = (wdUInt32)mapping.m_Channel[3].m_ChannelValue + 1;
      const wdUInt32 uiNumActualChannels = wdImageFormat::GetNumChannels(img.GetImageFormat());

      if (uiNumActualChannels < uiNumRequiredChannels)
      {
        // channel not available -> not needed
        continue;
      }

      if (img.Convert(wdImageFormat::R32G32B32A32_FLOAT).Failed())
      {
        // can't convert -> will fail later anyway
        continue;
      }

      const float* pColors = img.GetPixelPointer<float>();
      pColors += (uiNumRequiredChannels - 1); // offset by 0 to 3 to read red, green, blue or alpha

      WD_ASSERT_DEV(img.GetRowPitch() == img.GetWidth() * sizeof(float) * 4, "Unexpected row pitch");

      for (wdUInt32 i = 0; i < img.GetWidth() * img.GetHeight(); ++i)
      {
        if (!wdMath::IsEqual(*pColors, 1.0f, 1.0f / 255.0f))
        {
          // value is not 1.0f -> the channel is needed
          uiNumChannels = 4;
          return WD_SUCCESS;
        }

        pColors += 4;
      }
    }
  }

  return WD_SUCCESS;
}

wdResult wdTexConvProcessor::GenerateOutput(wdImage&& src, wdImage& dst, wdEnum<wdImageFormat> format)
{
  WD_PROFILE_SCOPE("GenerateOutput");

  dst.ResetAndMove(std::move(src));

  if (dst.Convert(format).Failed())
  {
    wdLog::Error("Failed to convert result image to output format '{}'", wdImageFormat::GetName(format));
    return WD_FAILURE;
  }

  return WD_SUCCESS;
}

wdResult wdTexConvProcessor::GenerateThumbnailOutput(const wdImage& srcImg, wdImage& dstImg, wdUInt32 uiTargetRes)
{
  if (uiTargetRes == 0)
    return WD_SUCCESS;

  WD_PROFILE_SCOPE("GenerateThumbnailOutput");

  wdUInt32 uiBestMip = 0;

  for (wdUInt32 m = 0; m < srcImg.GetNumMipLevels(); ++m)
  {
    if (srcImg.GetWidth(m) <= uiTargetRes && srcImg.GetHeight(m) <= uiTargetRes)
    {
      uiBestMip = m;
      break;
    }

    uiBestMip = m;
  }

  wdImage scratch1, scratch2;
  wdImage* pCurrentScratch = &scratch1;
  wdImage* pOtherScratch = &scratch2;

  pCurrentScratch->ResetAndCopy(srcImg.GetSubImageView(uiBestMip, 0));

  if (pCurrentScratch->GetWidth() > uiTargetRes || pCurrentScratch->GetHeight() > uiTargetRes)
  {
    if (pCurrentScratch->GetWidth() > pCurrentScratch->GetHeight())
    {
      const float fAspectRatio = (float)pCurrentScratch->GetWidth() / (float)uiTargetRes;
      wdUInt32 uiTargetHeight = (wdUInt32)(pCurrentScratch->GetHeight() / fAspectRatio);

      uiTargetHeight = wdMath::Max(uiTargetHeight, 4U);

      if (wdImageUtils::Scale(*pCurrentScratch, *pOtherScratch, uiTargetRes, uiTargetHeight).Failed())
      {
        wdLog::Error("Failed to resize thumbnail image from {}x{} to {}x{}", pCurrentScratch->GetWidth(), pCurrentScratch->GetHeight(), uiTargetRes,
          uiTargetHeight);
        return WD_FAILURE;
      }
    }
    else
    {
      const float fAspectRatio = (float)pCurrentScratch->GetHeight() / (float)uiTargetRes;
      wdUInt32 uiTargetWidth = (wdUInt32)(pCurrentScratch->GetWidth() / fAspectRatio);

      uiTargetWidth = wdMath::Max(uiTargetWidth, 4U);

      if (wdImageUtils::Scale(*pCurrentScratch, *pOtherScratch, uiTargetWidth, uiTargetRes).Failed())
      {
        wdLog::Error("Failed to resize thumbnail image from {}x{} to {}x{}", pCurrentScratch->GetWidth(), pCurrentScratch->GetHeight(), uiTargetWidth,
          uiTargetRes);
        return WD_FAILURE;
      }
    }

    wdMath::Swap(pCurrentScratch, pOtherScratch);
  }

  dstImg.ResetAndMove(std::move(*pCurrentScratch));

  // we want to write out the thumbnail unchanged, so make sure it has a non-sRGB format
  dstImg.ReinterpretAs(wdImageFormat::AsLinear(dstImg.GetImageFormat()));

  if (dstImg.Convert(wdImageFormat::R8G8B8A8_UNORM).Failed())
  {
    wdLog::Error("Failed to convert thumbnail image to RGBA8.");
    return WD_FAILURE;
  }

  // generate alpha checkerboard pattern
  {
    const float fTileSize = 16.0f;

    wdColorLinearUB* pPixels = dstImg.GetPixelPointer<wdColorLinearUB>();
    const wdUInt64 rowPitch = dstImg.GetRowPitch();

    wdInt32 checkCounter = 0;
    wdColor tiles[2]{wdColor::LightGray, wdColor::DarkGray};


    for (wdUInt32 y = 0; y < dstImg.GetHeight(); ++y)
    {
      checkCounter = (wdInt32)wdMath::Floor(y / fTileSize);

      for (wdUInt32 x = 0; x < dstImg.GetWidth(); ++x)
      {
        wdColorLinearUB& col = pPixels[x];

        if (col.a < 255)
        {
          const wdColor colF = col;
          const wdInt32 tileIdx = (checkCounter + (wdInt32)wdMath::Floor(x / fTileSize)) % 2;

          col = wdMath::Lerp(tiles[tileIdx], colF, wdMath::Sqrt(colF.a)).WithAlpha(colF.a);
        }
      }

      pPixels = wdMemoryUtils::AddByteOffset(pPixels, static_cast<ptrdiff_t>(rowPitch));
    }
  }

  return WD_SUCCESS;
}

wdResult wdTexConvProcessor::GenerateLowResOutput(const wdImage& srcImg, wdImage& dstImg, wdUInt32 uiLowResMip)
{
  if (uiLowResMip == 0)
    return WD_SUCCESS;

  WD_PROFILE_SCOPE("GenerateLowResOutput");

  // don't early out here in this case, otherwise external processes may consider the output to be incomplete
  //if (srcImg.GetNumMipLevels() <= uiLowResMip)
  //{
  //  // probably just a low-resolution input image, do not generate output, but also do not fail
  //  wdLog::Warning("LowRes image not generated, original resolution is already below threshold.");
  //  return WD_SUCCESS;
  //}

  if (wdImageUtils::ExtractLowerMipChain(srcImg, dstImg, uiLowResMip).Failed())
  {
    wdLog::Error("Failed to extract low-res mipmap chain from output image.");
    return WD_FAILURE;
  }

  return WD_SUCCESS;
}



WD_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_Processor);
