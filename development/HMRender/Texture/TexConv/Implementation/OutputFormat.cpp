#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/TexConv/TexConvProcessor.h>

static wdImageFormat::Enum DetermineOutputFormatPC(
  wdTexConvUsage::Enum targetFormat, wdTexConvCompressionMode::Enum compressionMode, wdUInt32 uiNumChannels)
{
  if (targetFormat == wdTexConvUsage::NormalMap || targetFormat == wdTexConvUsage::NormalMap_Inverted || targetFormat == wdTexConvUsage::BumpMap)
  {
    if (compressionMode >= wdTexConvCompressionMode::High)
      return wdImageFormat::BC5_UNORM;

    if (compressionMode >= wdTexConvCompressionMode::Medium)
      return wdImageFormat::R8G8_UNORM;

    return wdImageFormat::R16G16_UNORM;
  }

  if (targetFormat == wdTexConvUsage::Color)
  {
    if (compressionMode >= wdTexConvCompressionMode::High && uiNumChannels < 4)
      return wdImageFormat::BC1_UNORM_SRGB;

    if (compressionMode >= wdTexConvCompressionMode::Medium)
      return wdImageFormat::BC7_UNORM_SRGB;

    return wdImageFormat::R8G8B8A8_UNORM_SRGB;
  }

  if (targetFormat == wdTexConvUsage::Linear)
  {
    switch (uiNumChannels)
    {
      case 1:
        if (compressionMode >= wdTexConvCompressionMode::Medium)
          return wdImageFormat::BC4_UNORM;

        return wdImageFormat::R8_UNORM;

      case 2:
        if (compressionMode >= wdTexConvCompressionMode::Medium)
          return wdImageFormat::BC5_UNORM;

        return wdImageFormat::R8G8_UNORM;

      case 3:
        if (compressionMode >= wdTexConvCompressionMode::High)
          return wdImageFormat::BC1_UNORM;

        if (compressionMode >= wdTexConvCompressionMode::Medium)
          return wdImageFormat::BC7_UNORM;

        return wdImageFormat::R8G8B8A8_UNORM;

      case 4:
        if (compressionMode >= wdTexConvCompressionMode::Medium)
          return wdImageFormat::BC7_UNORM;

        return wdImageFormat::R8G8B8A8_UNORM;

      default:
        WD_ASSERT_NOT_IMPLEMENTED;
    }
  }

  if (targetFormat == wdTexConvUsage::Hdr)
  {
    switch (uiNumChannels)
    {
      case 1:
        if (compressionMode >= wdTexConvCompressionMode::High)
          return wdImageFormat::BC6H_UF16;

        return wdImageFormat::R16_FLOAT;

      case 2:
        return wdImageFormat::R16G16_FLOAT;

      case 3:
        if (compressionMode >= wdTexConvCompressionMode::High)
          return wdImageFormat::BC6H_UF16;

        if (compressionMode >= wdTexConvCompressionMode::Medium)
          return wdImageFormat::R11G11B10_FLOAT;

        return wdImageFormat::R16G16B16A16_FLOAT;

      case 4:
        return wdImageFormat::R16G16B16A16_FLOAT;
    }
  }

  return wdImageFormat::UNKNOWN;
}

wdResult wdTexConvProcessor::ChooseOutputFormat(wdEnum<wdImageFormat>& out_Format, wdEnum<wdTexConvUsage> usage, wdUInt32 uiNumChannels) const
{
  WD_PROFILE_SCOPE("ChooseOutputFormat");

  WD_ASSERT_DEV(out_Format == wdImageFormat::UNKNOWN, "Output format already set");

  switch (m_Descriptor.m_TargetPlatform)
  {
      // case  wdTexConvTargetPlatform::Android:
      //  out_Format = DetermineOutputFormatAndroid(m_Descriptor.m_TargetFormat, m_Descriptor.m_CompressionMode);
      //  break;

    case wdTexConvTargetPlatform::PC:
      out_Format = DetermineOutputFormatPC(usage, m_Descriptor.m_CompressionMode, uiNumChannels);
      break;

    default:
      WD_ASSERT_NOT_IMPLEMENTED;
  }

  if (out_Format == wdImageFormat::UNKNOWN)
  {
    wdLog::Error("Failed to decide for an output image format.");
    return WD_FAILURE;
  }

  return WD_SUCCESS;
}



WD_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_OutputFormat);
