#pragma once

#include <Texture/TexConv/TexConvEnums.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/UniquePtr.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageEnums.h>

struct wdTexConvChannelMapping
{
  wdInt8 m_iInputImageIndex = -1;
  wdTexConvChannelValue::Enum m_ChannelValue;
};

/// Describes from which input file to read which channel and then write it to the R, G, B, or A channel of the
/// output file. The four elements of the array represent the four channels of the output image.
struct wdTexConvSliceChannelMapping
{
  wdTexConvChannelMapping m_Channel[4] = {
    wdTexConvChannelMapping{-1, wdTexConvChannelValue::Red},
    wdTexConvChannelMapping{-1, wdTexConvChannelValue::Green},
    wdTexConvChannelMapping{-1, wdTexConvChannelValue::Blue},
    wdTexConvChannelMapping{-1, wdTexConvChannelValue::Alpha},
  };
};

class WD_TEXTURE_DLL wdTexConvDesc
{
  WD_DISALLOW_COPY_AND_ASSIGN(wdTexConvDesc);

public:
  wdTexConvDesc() = default;

  wdHybridArray<wdString, 4> m_InputFiles;
  wdDynamicArray<wdImage> m_InputImages;

  wdHybridArray<wdTexConvSliceChannelMapping, 6> m_ChannelMappings;

  // output type / platform
  wdEnum<wdTexConvOutputType> m_OutputType;
  wdEnum<wdTexConvTargetPlatform> m_TargetPlatform; // TODO: implement android

  // low resolution output
  wdUInt32 m_uiLowResMipmaps = 0;

  // thumbnail output
  wdUInt32 m_uiThumbnailOutputResolution = 0;

  // Format / Compression
  wdEnum<wdTexConvUsage> m_Usage;
  wdEnum<wdTexConvCompressionMode> m_CompressionMode;

  // resolution clamp and downscale
  wdUInt32 m_uiMinResolution = 16;
  wdUInt32 m_uiMaxResolution = 1024 * 8;
  wdUInt32 m_uiDownscaleSteps = 0;

  // Mipmaps / filtering
  wdEnum<wdTexConvMipmapMode> m_MipmapMode;
  wdEnum<wdTextureFilterSetting> m_FilterMode; // only used when writing to wd specific formats
  wdEnum<wdImageAddressMode> m_AddressModeU;
  wdEnum<wdImageAddressMode> m_AddressModeV;
  wdEnum<wdImageAddressMode> m_AddressModeW;
  bool m_bPreserveMipmapCoverage = false;
  float m_fMipmapAlphaThreshold = 0.5f;

  // Misc options
  wdUInt8 m_uiDilateColor = 0;
  bool m_bFlipHorizontal = false;
  bool m_bPremultiplyAlpha = false;
  float m_fHdrExposureBias = 0.0f;
  float m_fMaxValue = 64000.f;

  // wd specific
  wdUInt64 m_uiAssetHash = 0;
  wdUInt16 m_uiAssetVersion = 0;

  // Texture Atlas
  wdString m_sTextureAtlasDescFile;

  // Bump map filter
  wdEnum<wdTexConvBumpMapFilter> m_BumpMapFilter;
};
