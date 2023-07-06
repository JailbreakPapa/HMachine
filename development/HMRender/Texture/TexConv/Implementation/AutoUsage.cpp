#include <Texture/TexturePCH.h>

#include <Texture/TexConv/TexConvProcessor.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>

struct FileSuffixToUsage
{
  const char* m_szSuffix = nullptr;
  const wdTexConvUsage::Enum m_Usage = wdTexConvUsage::Auto;
};

static FileSuffixToUsage suffixToUsageMap[] = {
  //
  {"_d", wdTexConvUsage::Color},       //
  {"diff", wdTexConvUsage::Color},     //
  {"diffuse", wdTexConvUsage::Color},  //
  {"albedo", wdTexConvUsage::Color},   //
  {"col", wdTexConvUsage::Color},      //
  {"color", wdTexConvUsage::Color},    //
  {"emissive", wdTexConvUsage::Color}, //
  {"emit", wdTexConvUsage::Color},     //

  {"_n", wdTexConvUsage::NormalMap},      //
  {"nrm", wdTexConvUsage::NormalMap},     //
  {"norm", wdTexConvUsage::NormalMap},    //
  {"normal", wdTexConvUsage::NormalMap},  //
  {"normals", wdTexConvUsage::NormalMap}, //

  {"_r", wdTexConvUsage::Linear},        //
  {"_rgh", wdTexConvUsage::Linear},      //
  {"_rough", wdTexConvUsage::Linear},    //
  {"roughness", wdTexConvUsage::Linear}, //

  {"_m", wdTexConvUsage::Linear},       //
  {"_met", wdTexConvUsage::Linear},     //
  {"_metal", wdTexConvUsage::Linear},   //
  {"metallic", wdTexConvUsage::Linear}, //

  {"_h", wdTexConvUsage::Linear},     //
  {"height", wdTexConvUsage::Linear}, //
  {"_disp", wdTexConvUsage::Linear},  //

  {"_ao", wdTexConvUsage::Linear},       //
  {"occlusion", wdTexConvUsage::Linear}, //

  {"_alpha", wdTexConvUsage::Linear}, //
};


static wdTexConvUsage::Enum DetectUsageFromFilename(const char* szFile)
{
  wdStringBuilder name = wdPathUtils::GetFileName(szFile);
  name.ToLower();

  for (wdUInt32 i = 0; i < WD_ARRAY_SIZE(suffixToUsageMap); ++i)
  {
    if (name.EndsWith_NoCase(suffixToUsageMap[i].m_szSuffix))
    {
      return suffixToUsageMap[i].m_Usage;
    }
  }

  return wdTexConvUsage::Auto;
}

static wdTexConvUsage::Enum DetectUsageFromImage(const wdImage& image)
{
  const wdImageHeader& header = image.GetHeader();
  const wdImageFormat::Enum format = header.GetImageFormat();

  if (header.GetDepth() > 1)
  {
    // unsupported
    return wdTexConvUsage::Auto;
  }

  if (wdImageFormat::IsSrgb(format))
  {
    // already sRGB so must be color
    return wdTexConvUsage::Color;
  }

  if (format == wdImageFormat::BC5_UNORM)
  {
    return wdTexConvUsage::NormalMap;
  }

  if (wdImageFormat::GetBitsPerChannel(format, wdImageFormatChannel::R) > 8 || format == wdImageFormat::BC6H_SF16 ||
      format == wdImageFormat::BC6H_UF16)
  {
    return wdTexConvUsage::Hdr;
  }

  if (wdImageFormat::GetNumChannels(format) <= 2)
  {
    return wdTexConvUsage::Linear;
  }

  const wdImage* pImgRGBA = &image;
  wdImage convertedRGBA;

  if (image.GetImageFormat() != wdImageFormat::R8G8B8A8_UNORM)
  {
    pImgRGBA = &convertedRGBA;
    if (wdImageConversion::Convert(image, convertedRGBA, wdImageFormat::R8G8B8A8_UNORM).Failed())
    {
      // cannot convert to RGBA -> maybe some weird lookup table format
      return wdTexConvUsage::Auto;
    }
  }

  // analyze the image content
  {
    wdUInt32 sr = 0;
    wdUInt32 sg = 0;
    wdUInt32 sb = 0;

    wdUInt32 uiExtremeNormals = 0;

    wdUInt32 uiNumPixels = header.GetWidth() * header.GetHeight();

    // Sample no more than 10000 pixels
    wdUInt32 uiStride = wdMath::Max(1U, uiNumPixels / 10000);
    uiNumPixels /= uiStride;

    const wdUInt8* pPixel = pImgRGBA->GetPixelPointer<wdUInt8>();

    for (wdUInt32 uiPixel = 0; uiPixel < uiNumPixels; ++uiPixel)
    {
      // definitely not a normal map, if any Z vector points that much backwards
      uiExtremeNormals += (pPixel[2] < 90) ? 1 : 0;

      sr += pPixel[0];
      sg += pPixel[1];
      sb += pPixel[2];

      pPixel += 4 * uiStride;
    }

    // the average color in the image
    sr /= uiNumPixels;
    sg /= uiNumPixels;
    sb /= uiNumPixels;

    if (sb < 230 || sr < 128 - 60 || sr > 128 + 60 || sg < 128 - 60 || sg > 128 + 60)
    {
      // if the average color is not a proper hue of blue, it cannot be a normal map
      return wdTexConvUsage::Color;
    }

    if (uiExtremeNormals > uiNumPixels / 100)
    {
      // more than 1 percent of normals pointing backwards ? => probably not a normalmap
      return wdTexConvUsage::Color;
    }

    // it might just be a normal map, it does have the proper hue of blue
    return wdTexConvUsage::NormalMap;
  }
}

wdResult wdTexConvProcessor::AdjustUsage(const char* szFilename, const wdImage& srcImg, wdEnum<wdTexConvUsage>& inout_Usage)
{
  WD_PROFILE_SCOPE("AdjustUsage");

  if (inout_Usage == wdTexConvUsage::Auto)
  {
    inout_Usage = DetectUsageFromFilename(szFilename);
  }

  if (inout_Usage == wdTexConvUsage::Auto)
  {
    inout_Usage = DetectUsageFromImage(srcImg);
  }

  if (inout_Usage == wdTexConvUsage::Auto)
  {
    wdLog::Error("Failed to deduce target format.");
    return WD_FAILURE;
  }

  return WD_SUCCESS;
}

WD_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_AutoUsage);
