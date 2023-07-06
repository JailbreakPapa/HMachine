#include <Texture/TexturePCH.h>

#include <Texture/Image/ImageConversion.h>

namespace
{
  // https://docs.microsoft.com/en-us/windows/win32/medfound/recommended-8-bit-yuv-formats-for-video-rendering#converting-8-bit-yuv-to-rgb888
  wdVec3I32 RGB2YUV(wdVec3I32 vRgb)
  {
    wdVec3I32 yuv;
    yuv.x = ((66 * vRgb.x + 129 * vRgb.y + 25 * vRgb.z + 128) >> 8) + 16;
    yuv.y = ((-38 * vRgb.x - 74 * vRgb.y + 112 * vRgb.z + 128) >> 8) + 128;
    yuv.z = ((112 * vRgb.x - 94 * vRgb.y - 18 * vRgb.z + 128) >> 8) + 128;
    return yuv;
  }

  wdVec3I32 YUV2RGB(wdVec3I32 vYuv)
  {
    wdVec3I32 rgb;

    wdInt32 C = vYuv.x - 16;
    wdInt32 D = vYuv.y - 128;
    wdInt32 E = vYuv.z - 128;

    rgb.x = wdMath::Clamp((298 * C + 409 * E + 128) >> 8, 0, 255);
    rgb.y = wdMath::Clamp((298 * C - 100 * D - 208 * E + 128) >> 8, 0, 255);
    rgb.z = wdMath::Clamp((298 * C + 516 * D + 128) >> 8, 0, 255);
    return rgb;
  }
} // namespace

struct wdImageConversion_NV12_sRGB : public wdImageConversionStepDeplanarize
{
  virtual wdArrayPtr<const wdImageConversionEntry> GetSupportedConversions() const override
  {
    static wdImageConversionEntry supportedConversions[] = {
      wdImageConversionEntry(wdImageFormat::NV12, wdImageFormat::R8G8B8A8_UNORM_SRGB, wdImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual wdResult ConvertPixels(wdArrayPtr<wdImageView> source, wdImage target, wdUInt32 uiNumPixelsX, wdUInt32 uiNumPixelsY, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const override
  {
    for (wdUInt32 y = 0; y < uiNumPixelsY; y += 2)
    {
      const wdUInt8* luma0 = source[0].GetPixelPointer<wdUInt8>(0, 0, 0, 0, y);
      const wdUInt8* luma1 = source[0].GetPixelPointer<wdUInt8>(0, 0, 0, 0, y + 1);
      const wdUInt8* chroma = source[1].GetPixelPointer<wdUInt8>(0, 0, 0, 0, y / 2);

      wdUInt8* rgba0 = target.GetPixelPointer<wdUInt8>(0, 0, 0, 0, y);
      wdUInt8* rgba1 = target.GetPixelPointer<wdUInt8>(0, 0, 0, 0, y + 1);

      for (wdUInt32 x = 0; x < uiNumPixelsX; x += 2)
      {
        wdVec3I32 p00 = YUV2RGB(wdVec3I32(luma0[0], chroma[0], chroma[1]));
        wdVec3I32 p01 = YUV2RGB(wdVec3I32(luma0[1], chroma[0], chroma[1]));
        wdVec3I32 p10 = YUV2RGB(wdVec3I32(luma1[0], chroma[0], chroma[1]));
        wdVec3I32 p11 = YUV2RGB(wdVec3I32(luma1[1], chroma[0], chroma[1]));

        rgba0[0] = static_cast<wdUInt8>(p00.x);
        rgba0[1] = static_cast<wdUInt8>(p00.y);
        rgba0[2] = static_cast<wdUInt8>(p00.z);
        rgba0[3] = static_cast<wdUInt8>(0xff);
        rgba0[4] = static_cast<wdUInt8>(p01.x);
        rgba0[5] = static_cast<wdUInt8>(p01.y);
        rgba0[6] = static_cast<wdUInt8>(p01.z);
        rgba0[7] = static_cast<wdUInt8>(0xff);

        rgba1[0] = static_cast<wdUInt8>(p10.x);
        rgba1[1] = static_cast<wdUInt8>(p10.y);
        rgba1[2] = static_cast<wdUInt8>(p10.z);
        rgba1[3] = static_cast<wdUInt8>(0xff);
        rgba1[4] = static_cast<wdUInt8>(p11.x);
        rgba1[5] = static_cast<wdUInt8>(p11.y);
        rgba1[6] = static_cast<wdUInt8>(p11.z);
        rgba1[7] = static_cast<wdUInt8>(0xff);

        luma0 += 2;
        luma1 += 2;
        chroma += 2;

        rgba0 += 8;
        rgba1 += 8;
      }
    }

    return WD_SUCCESS;
  }
};

struct wdImageConversion_sRGB_NV12 : public wdImageConversionStepPlanarize
{
  virtual wdArrayPtr<const wdImageConversionEntry> GetSupportedConversions() const override
  {
    static wdImageConversionEntry supportedConversions[] = {
      wdImageConversionEntry(wdImageFormat::R8G8B8A8_UNORM_SRGB, wdImageFormat::NV12, wdImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual wdResult ConvertPixels(const wdImageView& source, wdArrayPtr<wdImage> target, wdUInt32 uiNumPixelsX, wdUInt32 uiNumPixelsY, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const override
  {
    for (wdUInt32 y = 0; y < uiNumPixelsY; y += 2)
    {
      const wdUInt8* rgba0 = source.GetPixelPointer<wdUInt8>(0, 0, 0, 0, y);
      const wdUInt8* rgba1 = source.GetPixelPointer<wdUInt8>(0, 0, 0, 0, y + 1);

      wdUInt8* luma0 = target[0].GetPixelPointer<wdUInt8>(0, 0, 0, 0, y);
      wdUInt8* luma1 = target[0].GetPixelPointer<wdUInt8>(0, 0, 0, 0, y + 1);
      wdUInt8* chroma = target[1].GetPixelPointer<wdUInt8>(0, 0, 0, 0, y / 2);

      for (wdUInt32 x = 0; x < uiNumPixelsX; x += 2)
      {
        wdVec3I32 p00 = RGB2YUV(wdVec3I32(rgba0[0], rgba0[1], rgba0[2]));
        wdVec3I32 p01 = RGB2YUV(wdVec3I32(rgba0[4], rgba0[5], rgba0[6]));
        wdVec3I32 p10 = RGB2YUV(wdVec3I32(rgba1[0], rgba1[1], rgba1[2]));
        wdVec3I32 p11 = RGB2YUV(wdVec3I32(rgba1[4], rgba1[5], rgba1[6]));

        luma0[0] = static_cast<wdUInt8>(p00.x);
        luma0[1] = static_cast<wdUInt8>(p01.x);
        luma1[0] = static_cast<wdUInt8>(p10.x);
        luma1[1] = static_cast<wdUInt8>(p11.x);

        wdVec3I32 c = (p00 + p01 + p10 + p11);

        chroma[0] = static_cast<wdUInt8>(c.y >> 2);
        chroma[1] = static_cast<wdUInt8>(c.z >> 2);

        luma0 += 2;
        luma1 += 2;
        chroma += 2;

        rgba0 += 8;
        rgba1 += 8;
      }
    }

    return WD_SUCCESS;
  }
};

static wdImageConversion_NV12_sRGB s_conversion_NV12_sRGB;
static wdImageConversion_sRGB_NV12 s_conversion_sRGB_NV12;

WD_STATICLINK_FILE(Texture, Texture_Image_Conversions_PlanarConversions);
