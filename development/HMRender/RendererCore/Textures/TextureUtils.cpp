#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/TextureUtils.h>

bool wdTextureUtils::s_bForceFullQualityAlways = false;

wdGALResourceFormat::Enum wdTextureUtils::ImageFormatToGalFormat(wdImageFormat::Enum format, bool bSRGB)
{
  switch (format)
  {
    case wdImageFormat::R8G8B8A8_UNORM:
      if (bSRGB)
        return wdGALResourceFormat::RGBAUByteNormalizedsRGB;
      else
        return wdGALResourceFormat::RGBAUByteNormalized;

      // case wdImageFormat::R8G8B8A8_TYPELESS:
    case wdImageFormat::R8G8B8A8_UNORM_SRGB:
      return wdGALResourceFormat::RGBAUByteNormalizedsRGB;

    case wdImageFormat::R8G8B8A8_UINT:
      return wdGALResourceFormat::RGBAUInt;

    case wdImageFormat::R8G8B8A8_SNORM:
      return wdGALResourceFormat::RGBAByteNormalized;

    case wdImageFormat::R8G8B8A8_SINT:
      return wdGALResourceFormat::RGBAInt;

    case wdImageFormat::B8G8R8A8_UNORM:
      if (bSRGB)
        return wdGALResourceFormat::BGRAUByteNormalizedsRGB;
      else
        return wdGALResourceFormat::BGRAUByteNormalized;

    case wdImageFormat::B8G8R8X8_UNORM:
      if (bSRGB)
        return wdGALResourceFormat::BGRAUByteNormalizedsRGB;
      else
        return wdGALResourceFormat::BGRAUByteNormalized;

      // case wdImageFormat::B8G8R8A8_TYPELESS:
    case wdImageFormat::B8G8R8A8_UNORM_SRGB:
      return wdGALResourceFormat::BGRAUByteNormalizedsRGB;

      // case wdImageFormat::B8G8R8X8_TYPELESS:
    case wdImageFormat::B8G8R8X8_UNORM_SRGB:
      return wdGALResourceFormat::BGRAUByteNormalizedsRGB;

      // case wdImageFormat::B8G8R8_UNORM:

      // case wdImageFormat::BC1_TYPELESS:
    case wdImageFormat::BC1_UNORM:
      if (bSRGB)
        return wdGALResourceFormat::BC1sRGB;
      else
        return wdGALResourceFormat::BC1;

    case wdImageFormat::BC1_UNORM_SRGB:
      return wdGALResourceFormat::BC1sRGB;

      // case wdImageFormat::BC2_TYPELESS:
    case wdImageFormat::BC2_UNORM:
      if (bSRGB)
        return wdGALResourceFormat::BC2sRGB;
      else
        return wdGALResourceFormat::BC2;

    case wdImageFormat::BC2_UNORM_SRGB:
      return wdGALResourceFormat::BC2sRGB;

      // case wdImageFormat::BC3_TYPELESS:
    case wdImageFormat::BC3_UNORM:
      if (bSRGB)
        return wdGALResourceFormat::BC3sRGB;
      else
        return wdGALResourceFormat::BC3;

    case wdImageFormat::BC3_UNORM_SRGB:
      return wdGALResourceFormat::BC3sRGB;

      // case wdImageFormat::BC4_TYPELESS:
    case wdImageFormat::BC4_UNORM:
      return wdGALResourceFormat::BC4UNormalized;

    case wdImageFormat::BC4_SNORM:
      return wdGALResourceFormat::BC4Normalized;

      // case wdImageFormat::BC5_TYPELESS:
    case wdImageFormat::BC5_UNORM:
      return wdGALResourceFormat::BC5UNormalized;

    case wdImageFormat::BC5_SNORM:
      return wdGALResourceFormat::BC5Normalized;

      // case wdImageFormat::BC6H_TYPELESS:
    case wdImageFormat::BC6H_UF16:
      return wdGALResourceFormat::BC6UFloat;

    case wdImageFormat::BC6H_SF16:
      return wdGALResourceFormat::BC6Float;

      // case wdImageFormat::BC7_TYPELESS:
    case wdImageFormat::BC7_UNORM:
      if (bSRGB)
        return wdGALResourceFormat::BC7UNormalizedsRGB;
      else
        return wdGALResourceFormat::BC7UNormalized;

    case wdImageFormat::BC7_UNORM_SRGB:
      return wdGALResourceFormat::BC7UNormalizedsRGB;

    case wdImageFormat::B5G6R5_UNORM:
      return wdGALResourceFormat::B5G6R5UNormalized; /// \todo Not supported by some GPUs ?

    case wdImageFormat::R16_FLOAT:
      return wdGALResourceFormat::RHalf;

    case wdImageFormat::R32_FLOAT:
      return wdGALResourceFormat::RFloat;

    case wdImageFormat::R16G16_FLOAT:
      return wdGALResourceFormat::RGHalf;

    case wdImageFormat::R32G32_FLOAT:
      return wdGALResourceFormat::RGFloat;

    case wdImageFormat::R32G32B32_FLOAT:
      return wdGALResourceFormat::RGBFloat;

    case wdImageFormat::R16G16B16A16_FLOAT:
      return wdGALResourceFormat::RGBAHalf;

    case wdImageFormat::R32G32B32A32_FLOAT:
      return wdGALResourceFormat::RGBAFloat;

    case wdImageFormat::R16G16B16A16_UNORM:
      return wdGALResourceFormat::RGBAUShortNormalized;

    case wdImageFormat::R8_UNORM:
      return wdGALResourceFormat::RUByteNormalized;

    case wdImageFormat::R8G8_UNORM:
      return wdGALResourceFormat::RGUByteNormalized;

    case wdImageFormat::R16G16_UNORM:
      return wdGALResourceFormat::RGUShortNormalized;

    case wdImageFormat::R11G11B10_FLOAT:
      return wdGALResourceFormat::RG11B10Float;

    default:
      WD_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  return wdGALResourceFormat::Invalid;
}

wdImageFormat::Enum wdTextureUtils::GalFormatToImageFormat(wdGALResourceFormat::Enum format)
{
  switch (format)
  {
    case wdGALResourceFormat::RGBAFloat:
      return wdImageFormat::R32G32B32A32_FLOAT;
    case wdGALResourceFormat::RGBAUInt:
      return wdImageFormat::R8G8B8A8_UINT;
    case wdGALResourceFormat::RGBAInt:
      return wdImageFormat::R8G8B8A8_SINT;
    case wdGALResourceFormat::RGBFloat:
      return wdImageFormat::R32G32B32_FLOAT;
    case wdGALResourceFormat::RGBUInt:
      return wdImageFormat::R32G32B32_UINT;
    case wdGALResourceFormat::RGBInt:
      return wdImageFormat::R32G32B32_SINT;
    case wdGALResourceFormat::B5G6R5UNormalized:
      return wdImageFormat::B5G6R5_UNORM;
    case wdGALResourceFormat::BGRAUByteNormalized:
      return wdImageFormat::B8G8R8A8_UNORM;
    case wdGALResourceFormat::BGRAUByteNormalizedsRGB:
      return wdImageFormat::B8G8R8A8_UNORM_SRGB;
    case wdGALResourceFormat::RGBAHalf:
      return wdImageFormat::R16G16B16A16_FLOAT;
    case wdGALResourceFormat::RGBAUShort:
      return wdImageFormat::R16G16B16A16_UINT;
    case wdGALResourceFormat::RGBAUShortNormalized:
      return wdImageFormat::R16G16B16A16_UNORM;
    case wdGALResourceFormat::RGBAShort:
      return wdImageFormat::R16G16B16A16_SINT;
    case wdGALResourceFormat::RGBAShortNormalized:
      return wdImageFormat::R16G16B16A16_SNORM;
    case wdGALResourceFormat::RGFloat:
      return wdImageFormat::R32G32_FLOAT;
    case wdGALResourceFormat::RGUInt:
      return wdImageFormat::R32G32_UINT;
    case wdGALResourceFormat::RGInt:
      return wdImageFormat::R32G32_SINT;
    case wdGALResourceFormat::RG11B10Float:
      return wdImageFormat::R11G11B10_FLOAT;
    case wdGALResourceFormat::RGBAUByteNormalized:
      return wdImageFormat::R8G8B8A8_UNORM;
    case wdGALResourceFormat::RGBAUByteNormalizedsRGB:
      return wdImageFormat::R8G8B8A8_UNORM_SRGB;
    case wdGALResourceFormat::RGBAUByte:
      return wdImageFormat::R8G8B8A8_UINT;
    case wdGALResourceFormat::RGBAByteNormalized:
      return wdImageFormat::R8G8B8A8_SNORM;
    case wdGALResourceFormat::RGBAByte:
      return wdImageFormat::R8G8B8A8_SINT;
    case wdGALResourceFormat::RGHalf:
      return wdImageFormat::R16G16_FLOAT;
    case wdGALResourceFormat::RGUShort:
      return wdImageFormat::R16G16_UINT;
    case wdGALResourceFormat::RGUShortNormalized:
      return wdImageFormat::R16G16_UNORM;
    case wdGALResourceFormat::RGShort:
      return wdImageFormat::R16G16_SINT;
    case wdGALResourceFormat::RGShortNormalized:
      return wdImageFormat::R16G16_SNORM;
    case wdGALResourceFormat::RGUByte:
      return wdImageFormat::R8G8_UINT;
    case wdGALResourceFormat::RGUByteNormalized:
      return wdImageFormat::R8G8_UNORM;
    case wdGALResourceFormat::RGByte:
      return wdImageFormat::R8G8_SINT;
    case wdGALResourceFormat::RGByteNormalized:
      return wdImageFormat::R8G8_SNORM;
    case wdGALResourceFormat::DFloat:
      return wdImageFormat::R32_FLOAT;
    case wdGALResourceFormat::RFloat:
      return wdImageFormat::R32_FLOAT;
    case wdGALResourceFormat::RUInt:
      return wdImageFormat::R32_UINT;
    case wdGALResourceFormat::RInt:
      return wdImageFormat::R32_SINT;
    case wdGALResourceFormat::RHalf:
      return wdImageFormat::R16_FLOAT;
    case wdGALResourceFormat::RUShort:
      return wdImageFormat::R16_UINT;
    case wdGALResourceFormat::RUShortNormalized:
      return wdImageFormat::R16_UNORM;
    case wdGALResourceFormat::RShort:
      return wdImageFormat::R16_SINT;
    case wdGALResourceFormat::RShortNormalized:
      return wdImageFormat::R16_SNORM;
    case wdGALResourceFormat::RUByte:
      return wdImageFormat::R8_UINT;
    case wdGALResourceFormat::RUByteNormalized:
      return wdImageFormat::R8_UNORM;
    case wdGALResourceFormat::RByte:
      return wdImageFormat::R8_SINT;
    case wdGALResourceFormat::RByteNormalized:
      return wdImageFormat::R8_SNORM;
    case wdGALResourceFormat::AUByteNormalized:
      return wdImageFormat::R8_UNORM;
    case wdGALResourceFormat::D16:
      return wdImageFormat::R16_FLOAT;
    case wdGALResourceFormat::BC1:
      return wdImageFormat::BC1_UNORM;
    case wdGALResourceFormat::BC1sRGB:
      return wdImageFormat::BC1_UNORM_SRGB;
    case wdGALResourceFormat::BC2:
      return wdImageFormat::BC2_UNORM;
    case wdGALResourceFormat::BC2sRGB:
      return wdImageFormat::BC2_UNORM_SRGB;
    case wdGALResourceFormat::BC3:
      return wdImageFormat::BC3_UNORM;
    case wdGALResourceFormat::BC3sRGB:
      return wdImageFormat::BC3_UNORM_SRGB;
    case wdGALResourceFormat::BC4UNormalized:
      return wdImageFormat::BC4_UNORM;
    case wdGALResourceFormat::BC4Normalized:
      return wdImageFormat::BC4_SNORM;
    case wdGALResourceFormat::BC5UNormalized:
      return wdImageFormat::BC5_UNORM;
    case wdGALResourceFormat::BC5Normalized:
      return wdImageFormat::BC5_SNORM;
    case wdGALResourceFormat::BC6UFloat:
      return wdImageFormat::BC6H_UF16;
    case wdGALResourceFormat::BC6Float:
      return wdImageFormat::BC6H_SF16;
    case wdGALResourceFormat::BC7UNormalized:
      return wdImageFormat::BC7_UNORM;
    case wdGALResourceFormat::BC7UNormalizedsRGB:
      return wdImageFormat::BC7_UNORM_SRGB;
    case wdGALResourceFormat::RGB10A2UInt:
    case wdGALResourceFormat::RGB10A2UIntNormalized:
    case wdGALResourceFormat::D24S8:
    default:
    {
#if WD_ENABLED(WD_COMPILE_FOR_DEBUG)
      wdStringBuilder sFormat;
      WD_ASSERT_DEBUG(wdReflectionUtils::EnumerationToString(wdGetStaticRTTI<wdGALResourceFormat>(), format, sFormat, wdReflectionUtils::EnumConversionMode::ValueNameOnly), "Cannot convert GAL format '{}' to string", format);
      WD_ASSERT_DEBUG(false, "The GL format: '{}' does not have a matching image format.", sFormat);
#endif
    }
  }
  return wdImageFormat::UNKNOWN;
}

wdImageFormat::Enum wdTextureUtils::GalFormatToImageFormat(wdGALResourceFormat::Enum format, bool bRemoveSRGB)
{
  wdImageFormat::Enum imageFormat = GalFormatToImageFormat(format);
  if (bRemoveSRGB)
  {
    imageFormat = wdImageFormat::AsLinear(imageFormat);
  }
  return imageFormat;
}

void wdTextureUtils::ConfigureSampler(wdTextureFilterSetting::Enum filter, wdGALSamplerStateCreationDescription& out_sampler)
{
  const wdTextureFilterSetting::Enum thisFilter = wdRenderContext::GetDefaultInstance()->GetSpecificTextureFilter(filter);

  out_sampler.m_MinFilter = wdGALTextureFilterMode::Linear;
  out_sampler.m_MagFilter = wdGALTextureFilterMode::Linear;
  out_sampler.m_MipFilter = wdGALTextureFilterMode::Linear;
  out_sampler.m_uiMaxAnisotropy = 1;

  switch (thisFilter)
  {
    case wdTextureFilterSetting::FixedNearest:
      out_sampler.m_MinFilter = wdGALTextureFilterMode::Point;
      out_sampler.m_MagFilter = wdGALTextureFilterMode::Point;
      out_sampler.m_MipFilter = wdGALTextureFilterMode::Point;
      break;
    case wdTextureFilterSetting::FixedBilinear:
      out_sampler.m_MipFilter = wdGALTextureFilterMode::Point;
      break;
    case wdTextureFilterSetting::FixedTrilinear:
      break;
    case wdTextureFilterSetting::FixedAnisotropic2x:
      out_sampler.m_MinFilter = wdGALTextureFilterMode::Anisotropic;
      out_sampler.m_MagFilter = wdGALTextureFilterMode::Anisotropic;
      out_sampler.m_uiMaxAnisotropy = 2;
      break;
    case wdTextureFilterSetting::FixedAnisotropic4x:
      out_sampler.m_MinFilter = wdGALTextureFilterMode::Anisotropic;
      out_sampler.m_MagFilter = wdGALTextureFilterMode::Anisotropic;
      out_sampler.m_uiMaxAnisotropy = 4;
      break;
    case wdTextureFilterSetting::FixedAnisotropic8x:
      out_sampler.m_MinFilter = wdGALTextureFilterMode::Anisotropic;
      out_sampler.m_MagFilter = wdGALTextureFilterMode::Anisotropic;
      out_sampler.m_uiMaxAnisotropy = 8;
      break;
    case wdTextureFilterSetting::FixedAnisotropic16x:
      out_sampler.m_MinFilter = wdGALTextureFilterMode::Anisotropic;
      out_sampler.m_MagFilter = wdGALTextureFilterMode::Anisotropic;
      out_sampler.m_uiMaxAnisotropy = 16;
      break;
    default:
      break;
  }
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Textures_TextureUtils);
