#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

wdResult wdTexConvProcessor::ForceSRGBFormats()
{
  // if the output is going to be sRGB, assume the incoming RGB data is also already in sRGB
  if (m_Descriptor.m_Usage == wdTexConvUsage::Color)
  {
    for (const auto& mapping : m_Descriptor.m_ChannelMappings)
    {
      // do not enforce sRGB conversion for textures that are mapped to the alpha channel
      for (wdUInt32 i = 0; i < 3; ++i)
      {
        const wdInt32 iTex = mapping.m_Channel[i].m_iInputImageIndex;
        if (iTex != -1)
        {
          auto& img = m_Descriptor.m_InputImages[iTex];
          img.ReinterpretAs(wdImageFormat::AsSrgb(img.GetImageFormat()));
        }
      }
    }
  }

  return WD_SUCCESS;
}

wdResult wdTexConvProcessor::GenerateMipmaps(wdImage& img, wdUInt32 uiNumMips, MipmapChannelMode channelMode /*= MipmapChannelMode::AllChannels*/) const
{
  WD_PROFILE_SCOPE("GenerateMipmaps");

  wdImageUtils::MipMapOptions opt;
  opt.m_numMipMaps = uiNumMips;

  wdImageFilterBox filterLinear;
  wdImageFilterSincWithKaiserWindow filterKaiser;

  switch (m_Descriptor.m_MipmapMode)
  {
    case wdTexConvMipmapMode::None:
      return WD_SUCCESS;

    case wdTexConvMipmapMode::Linear:
      opt.m_filter = &filterLinear;
      break;

    case wdTexConvMipmapMode::Kaiser:
      opt.m_filter = &filterKaiser;
      break;
  }

  opt.m_addressModeU = m_Descriptor.m_AddressModeU;
  opt.m_addressModeV = m_Descriptor.m_AddressModeV;
  opt.m_addressModeW = m_Descriptor.m_AddressModeW;

  opt.m_preserveCoverage = m_Descriptor.m_bPreserveMipmapCoverage;
  opt.m_alphaThreshold = m_Descriptor.m_fMipmapAlphaThreshold;

  opt.m_renormalizeNormals = m_Descriptor.m_Usage == wdTexConvUsage::NormalMap || m_Descriptor.m_Usage == wdTexConvUsage::NormalMap_Inverted || m_Descriptor.m_Usage == wdTexConvUsage::BumpMap;

  // Copy red to alpha channel if we only have a single channel input texture
  if (opt.m_preserveCoverage && channelMode == MipmapChannelMode::SingleChannel)
  {
    auto imgData = img.GetBlobPtr<wdColor>();
    auto pData = imgData.GetPtr();
    while (pData < imgData.GetEndPtr())
    {
      pData->a = pData->r;
      ++pData;
    }
  }

  wdImage scratch;
  wdImageUtils::GenerateMipMaps(img, scratch, opt);
  img.ResetAndMove(std::move(scratch));

  if (img.GetNumMipLevels() <= 1)
  {
    wdLog::Error("Mipmap generation failed.");
    return WD_FAILURE;
  }

  // Copy alpha channel back to red
  if (opt.m_preserveCoverage && channelMode == MipmapChannelMode::SingleChannel)
  {
    auto imgData = img.GetBlobPtr<wdColor>();
    auto pData = imgData.GetPtr();
    while (pData < imgData.GetEndPtr())
    {
      pData->r = pData->a;
      ++pData;
    }
  }

  return WD_SUCCESS;
}

wdResult wdTexConvProcessor::PremultiplyAlpha(wdImage& image) const
{
  WD_PROFILE_SCOPE("PremultiplyAlpha");

  if (!m_Descriptor.m_bPremultiplyAlpha)
    return WD_SUCCESS;

  for (wdColor& col : image.GetBlobPtr<wdColor>())
  {
    col.r *= col.a;
    col.g *= col.a;
    col.b *= col.a;
  }

  return WD_SUCCESS;
}

wdResult wdTexConvProcessor::AdjustHdrExposure(wdImage& img) const
{
  WD_PROFILE_SCOPE("AdjustHdrExposure");

  wdImageUtils::ChangeExposure(img, m_Descriptor.m_fHdrExposureBias);
  return WD_SUCCESS;
}

wdResult wdTexConvProcessor::ConvertToNormalMap(wdArrayPtr<wdImage> imgs) const
{
  WD_PROFILE_SCOPE("ConvertToNormalMap");

  for (wdImage& img : imgs)
  {
    WD_SUCCEED_OR_RETURN(ConvertToNormalMap(img));
  }

  return WD_SUCCESS;
}

wdResult wdTexConvProcessor::ConvertToNormalMap(wdImage& bumpMap) const
{
  wdImageHeader newImageHeader = bumpMap.GetHeader();
  newImageHeader.SetNumMipLevels(1);
  wdImage newImage;
  newImage.ResetAndAlloc(newImageHeader);

  struct Accum
  {
    float x = 0.f;
    float y = 0.f;
  };
  wdDelegate<Accum(wdUInt32, wdUInt32)> filterKernel;

  // we'll assume that both the input bump map and the new image are using
  // RGBA 32 bit floating point as an internal format which should be tightly packed
  WD_ASSERT_DEV(bumpMap.GetImageFormat() == wdImageFormat::R32G32B32A32_FLOAT && bumpMap.GetRowPitch() % sizeof(wdColor) == 0, "");

  const wdColor* bumpPixels = bumpMap.GetPixelPointer<wdColor>(0, 0, 0, 0, 0, 0);
  const auto getBumpPixel = [&](wdUInt32 x, wdUInt32 y) -> float
  {
    const wdColor* ptr = bumpPixels + y * bumpMap.GetWidth() + x;
    return ptr->r;
  };

  wdColor* newPixels = newImage.GetPixelPointer<wdColor>(0, 0, 0, 0, 0, 0);
  auto getNewPixel = [&](wdUInt32 x, wdUInt32 y) -> wdColor&
  {
    wdColor* ptr = newPixels + y * newImage.GetWidth() + x;
    return *ptr;
  };

  switch (m_Descriptor.m_BumpMapFilter)
  {
    case wdTexConvBumpMapFilter::Finite:
      filterKernel = [&](wdUInt32 x, wdUInt32 y)
      {
        constexpr float linearKernel[3] = {-1, 0, 1};

        Accum accum;
        for (int i = -1; i <= 1; ++i)
        {
          const wdInt32 rx = wdMath::Clamp(i + static_cast<wdInt32>(x), 0, static_cast<wdInt32>(newImage.GetWidth()) - 1);
          const wdInt32 ry = wdMath::Clamp(i + static_cast<wdInt32>(y), 0, static_cast<wdInt32>(newImage.GetHeight()) - 1);

          const float depthX = getBumpPixel(rx, y);
          const float depthY = getBumpPixel(x, ry);

          accum.x += depthX * linearKernel[i + 1];
          accum.y += depthY * linearKernel[i + 1];
        }

        return accum;
      };
      break;
    case wdTexConvBumpMapFilter::Sobel:
      filterKernel = [&](wdUInt32 x, wdUInt32 y)
      {
        constexpr float kernel[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
        constexpr float weight = 1.f / 4.f;

        Accum accum;
        for (wdInt32 i = -1; i <= 1; ++i)
        {
          for (wdInt32 j = -1; j <= 1; ++j)
          {
            const wdInt32 rx = wdMath::Clamp(j + static_cast<wdInt32>(x), 0, static_cast<wdInt32>(newImage.GetWidth()) - 1);
            const wdInt32 ry = wdMath::Clamp(i + static_cast<wdInt32>(y), 0, static_cast<wdInt32>(newImage.GetHeight()) - 1);

            const float depth = getBumpPixel(rx, ry);

            accum.x += depth * kernel[i + 1][j + 1];
            accum.y += depth * kernel[j + 1][i + 1];
          }
        }

        accum.x *= weight;
        accum.y *= weight;

        return accum;
      };
      break;
    case wdTexConvBumpMapFilter::Scharr:
      filterKernel = [&](wdUInt32 x, wdUInt32 y)
      {
        constexpr float kernel[3][3] = {{-3, 0, 3}, {-10, 0, 10}, {-3, 0, 3}};
        constexpr float weight = 1.f / 16.f;

        Accum accum;
        for (wdInt32 i = -1; i <= 1; ++i)
        {
          for (wdInt32 j = -1; j <= 1; ++j)
          {
            const wdInt32 rx = wdMath::Clamp(j + static_cast<wdInt32>(x), 0, static_cast<wdInt32>(newImage.GetWidth()) - 1);
            const wdInt32 ry = wdMath::Clamp(i + static_cast<wdInt32>(y), 0, static_cast<wdInt32>(newImage.GetHeight()) - 1);

            const float depth = getBumpPixel(rx, ry);

            accum.x += depth * kernel[i + 1][j + 1];
            accum.y += depth * kernel[j + 1][i + 1];
          }
        }

        accum.x *= weight;
        accum.y *= weight;

        return accum;
      };
      break;
  };

  for (wdUInt32 y = 0; y < bumpMap.GetHeight(); ++y)
  {
    for (wdUInt32 x = 0; x < bumpMap.GetWidth(); ++x)
    {
      Accum accum = filterKernel(x, y);

      wdVec3 normal = wdVec3(1.f, 0.f, accum.x).CrossRH(wdVec3(0.f, 1.f, accum.y));
      normal.NormalizeIfNotZero(wdVec3(0, 0, 1), 0.001f).IgnoreResult();
      normal.y = -normal.y;

      normal = normal * 0.5f + wdVec3(0.5f);

      wdColor& newPixel = getNewPixel(x, y);
      newPixel.SetRGBA(normal.x, normal.y, normal.z, 0.f);
    }
  }

  bumpMap.ResetAndMove(std::move(newImage));

  return WD_SUCCESS;
}

wdResult wdTexConvProcessor::ClampInputValues(wdArrayPtr<wdImage> images, float maxValue) const
{
  for (wdImage& image : images)
  {
    WD_SUCCEED_OR_RETURN(ClampInputValues(image, maxValue));
  }

  return WD_SUCCESS;
}

wdResult wdTexConvProcessor::ClampInputValues(wdImage& image, float maxValue) const
{
  // we'll assume that at this point in the processing pipeline, the format is
  // RGBA32F which should result in tightly packed mipmaps.
  WD_ASSERT_DEV(image.GetImageFormat() == wdImageFormat::R32G32B32A32_FLOAT && image.GetRowPitch() % sizeof(float[4]) == 0, "");

  for (auto& value : image.GetBlobPtr<float>())
  {
    if (wdMath::IsNaN(value))
    {
      value = 0.f;
    }
    else
    {
      value = wdMath::Clamp(value, -maxValue, maxValue);
    }
  }

  return WD_SUCCESS;
}

static bool FillAvgImageColor(wdImage& ref_img)
{
  wdColor avg = wdColor::ZeroColor();
  wdUInt32 uiValidCount = 0;

  for (const wdColor& col : ref_img.GetBlobPtr<wdColor>())
  {
    if (col.a > 0.0f)
    {
      avg += col;
      ++uiValidCount;
    }
  }

  if (uiValidCount == 0 || uiValidCount == ref_img.GetBlobPtr<wdColor>().GetCount())
  {
    // nothing to do
    return false;
  }

  avg /= static_cast<float>(uiValidCount);
  avg.NormalizeToLdrRange();
  avg.a = 0.0f;

  for (wdColor& col : ref_img.GetBlobPtr<wdColor>())
  {
    if (col.a == 0.0f)
    {
      col = avg;
    }
  }

  return true;
}

static void ClearAlpha(wdImage& ref_img, float fAlphaThreshold)
{
  for (wdColor& col : ref_img.GetBlobPtr<wdColor>())
  {
    if (col.a <= fAlphaThreshold)
    {
      col.a = 0.0f;
    }
  }
}

inline static wdColor GetPixelValue(const wdColor* pPixels, wdInt32 iWidth, wdInt32 x, wdInt32 y)
{
  return pPixels[y * iWidth + x];
}

inline static void SetPixelValue(wdColor* pPixels, wdInt32 iWidth, wdInt32 x, wdInt32 y, const wdColor& col)
{
  pPixels[y * iWidth + x] = col;
}

static wdColor GetAvgColor(wdColor* pPixels, wdInt32 iWidth, wdInt32 iHeight, wdInt32 x, wdInt32 y, float fMarkAlpha)
{
  wdColor colAt = GetPixelValue(pPixels, iWidth, x, y);

  if (colAt.a > 0)
    return colAt;

  wdColor avg;
  avg.SetZero();
  wdUInt32 uiValidCount = 0;

  const wdInt32 iRadius = 1;

  for (wdInt32 cy = wdMath::Max<wdInt32>(0, y - iRadius); cy <= wdMath::Min<wdInt32>(y + iRadius, iHeight - 1); ++cy)
  {
    for (wdInt32 cx = wdMath::Max<wdInt32>(0, x - iRadius); cx <= wdMath::Min<wdInt32>(x + iRadius, iWidth - 1); ++cx)
    {
      const wdColor col = GetPixelValue(pPixels, iWidth, cx, cy);

      if (col.a > fMarkAlpha)
      {
        avg += col;
        ++uiValidCount;
      }
    }
  }

  if (uiValidCount == 0)
    return colAt;

  avg /= static_cast<float>(uiValidCount);
  avg.a = fMarkAlpha;

  return avg;
}

static void DilateColors(wdColor* pPixels, wdInt32 iWidth, wdInt32 iHeight, float fMarkAlpha)
{
  for (wdInt32 y = 0; y < iHeight; ++y)
  {
    for (wdInt32 x = 0; x < iWidth; ++x)
    {
      const wdColor avg = GetAvgColor(pPixels, iWidth, iHeight, x, y, fMarkAlpha);

      SetPixelValue(pPixels, iWidth, x, y, avg);
    }
  }
}

wdResult wdTexConvProcessor::DilateColor2D(wdImage& img) const
{
  if (m_Descriptor.m_uiDilateColor == 0)
    return WD_SUCCESS;

  WD_PROFILE_SCOPE("DilateColor2D");

  if (!FillAvgImageColor(img))
    return WD_SUCCESS;

  const wdUInt32 uiNumPasses = m_Descriptor.m_uiDilateColor;

  wdColor* pPixels = img.GetPixelPointer<wdColor>();
  const wdInt32 iWidth = static_cast<wdInt32>(img.GetWidth());
  const wdInt32 iHeight = static_cast<wdInt32>(img.GetHeight());

  for (wdUInt32 pass = uiNumPasses; pass > 0; --pass)
  {
    const float fAlphaThreshold = (static_cast<float>(pass) / uiNumPasses) / 256.0f; // between 0 and 1/256
    DilateColors(pPixels, iWidth, iHeight, fAlphaThreshold);
  }

  ClearAlpha(img, 1.0f / 256.0f);

  return WD_SUCCESS;
}

WD_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_TextureModifications);
