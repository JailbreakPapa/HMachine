#include <Texture/TexturePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/StaticArray.h>
#include <Texture/Image/ImageFormat.h>

namespace
{
  struct wdImageFormatMetaData
  {
    wdImageFormatMetaData()
    {
      wdMemoryUtils::ZeroFillArray(m_uiBitsPerChannel);
      wdMemoryUtils::ZeroFillArray(m_uiChannelMasks);

      m_planeData.SetCount(1);
    }

    const char* m_szName{nullptr};

    struct PlaneData
    {
      wdUInt8 m_uiBitsPerBlock{0}; ///< Bits per block for compressed formats; for uncompressed formats (which always have a block size of 1x1x1), this is equal to bits per pixel.
      wdUInt8 m_uiBlockWidth{1};
      wdUInt8 m_uiBlockHeight{1};
      wdUInt8 m_uiBlockDepth{1};
      wdImageFormat::Enum m_subFormat{wdImageFormat::UNKNOWN}; ///< Subformats when viewing only a subslice of the data.
    };

    wdStaticArray<PlaneData, 2> m_planeData;

    wdUInt8 m_uiNumChannels{0};

    wdUInt8 m_uiBitsPerChannel[wdImageFormatChannel::COUNT];
    wdUInt32 m_uiChannelMasks[wdImageFormatChannel::COUNT];


    bool m_requireFirstLevelBlockAligned{false}; ///< Only for compressed formats: If true, the first level's dimensions must be a multiple of the
                                                 ///< block size; if false, padding can be applied for compressing the first mip level, too.
    bool m_isDepth{false};
    bool m_isStencil{false};

    wdImageFormatDataType::Enum m_dataType{wdImageFormatDataType::NONE};
    wdImageFormatType::Enum m_formatType{wdImageFormatType::UNKNOWN};

    wdImageFormat::Enum m_asLinear{wdImageFormat::UNKNOWN};
    wdImageFormat::Enum m_asSrgb{wdImageFormat::UNKNOWN};

    wdUInt32 getNumBlocksX(wdUInt32 uiWidth, wdUInt32 uiPlaneIndex) const
    {
      return (uiWidth - 1) / m_planeData[uiPlaneIndex].m_uiBlockWidth + 1;
    }

    wdUInt32 getNumBlocksY(wdUInt32 uiHeight, wdUInt32 uiPlaneIndex) const
    {
      return (uiHeight - 1) / m_planeData[uiPlaneIndex].m_uiBlockHeight + 1;
    }

    wdUInt32 getNumBlocksZ(wdUInt32 uiDepth, wdUInt32 uiPlaneIndex) const
    {
      return (uiDepth - 1) / m_planeData[uiPlaneIndex].m_uiBlockDepth + 1;
    }

    wdUInt32 getRowPitch(wdUInt32 uiWidth, wdUInt32 uiPlaneIndex) const
    {
      return getNumBlocksX(uiWidth, uiPlaneIndex) * m_planeData[uiPlaneIndex].m_uiBitsPerBlock / 8;
    }
  };

  wdStaticArray<wdImageFormatMetaData, wdImageFormat::NUM_FORMATS> s_formatMetaData;

  void InitFormatLinear(wdImageFormat::Enum format, const char* szName, wdImageFormatDataType::Enum dataType, wdUInt8 uiBitsPerPixel, wdUInt8 uiBitsR,
    wdUInt8 uiBitsG, wdUInt8 uiBitsB, wdUInt8 uiBitsA, wdUInt8 uiNumChannels)
  {
    s_formatMetaData[format].m_szName = szName;

    s_formatMetaData[format].m_planeData[0].m_uiBitsPerBlock = uiBitsPerPixel;
    s_formatMetaData[format].m_dataType = dataType;
    s_formatMetaData[format].m_formatType = wdImageFormatType::LINEAR;

    s_formatMetaData[format].m_uiNumChannels = uiNumChannels;

    s_formatMetaData[format].m_uiBitsPerChannel[wdImageFormatChannel::R] = uiBitsR;
    s_formatMetaData[format].m_uiBitsPerChannel[wdImageFormatChannel::G] = uiBitsG;
    s_formatMetaData[format].m_uiBitsPerChannel[wdImageFormatChannel::B] = uiBitsB;
    s_formatMetaData[format].m_uiBitsPerChannel[wdImageFormatChannel::A] = uiBitsA;

    s_formatMetaData[format].m_asLinear = format;
    s_formatMetaData[format].m_asSrgb = format;
  }

#define INIT_FORMAT_LINEAR(format, dataType, uiBitsPerPixel, uiBitsR, uiBitsG, uiBitsB, uiBitsA, uiNumChannels) \
  InitFormatLinear(wdImageFormat::format, #format, wdImageFormatDataType::dataType, uiBitsPerPixel, uiBitsR, uiBitsG, uiBitsB, uiBitsA, uiNumChannels)

  void InitFormatCompressed(wdImageFormat::Enum format, const char* szName, wdImageFormatDataType::Enum dataType, wdUInt8 uiBitsPerBlock,
    wdUInt8 uiBlockWidth, wdUInt8 uiBlockHeight, wdUInt8 uiBlockDepth, bool bRequireFirstLevelBlockAligned, wdUInt8 uiNumChannels)
  {
    s_formatMetaData[format].m_szName = szName;

    s_formatMetaData[format].m_planeData[0].m_uiBitsPerBlock = uiBitsPerBlock;
    s_formatMetaData[format].m_planeData[0].m_uiBlockWidth = uiBlockWidth;
    s_formatMetaData[format].m_planeData[0].m_uiBlockHeight = uiBlockHeight;
    s_formatMetaData[format].m_planeData[0].m_uiBlockDepth = uiBlockDepth;
    s_formatMetaData[format].m_dataType = dataType;
    s_formatMetaData[format].m_formatType = wdImageFormatType::BLOCK_COMPRESSED;

    s_formatMetaData[format].m_uiNumChannels = uiNumChannels;

    s_formatMetaData[format].m_requireFirstLevelBlockAligned = bRequireFirstLevelBlockAligned;

    s_formatMetaData[format].m_asLinear = format;
    s_formatMetaData[format].m_asSrgb = format;
  }

#define INIT_FORMAT_COMPRESSED(                                                                                                                    \
  format, dataType, uiBitsPerBlock, uiBlockWidth, uiBlockHeight, uiBlockDepth, requireFirstLevelBlockAligned, uiNumChannels)                       \
  InitFormatCompressed(wdImageFormat::format, #format, wdImageFormatDataType::dataType, uiBitsPerBlock, uiBlockWidth, uiBlockHeight, uiBlockDepth, \
    requireFirstLevelBlockAligned, uiNumChannels)

  void InitFormatDepth(wdImageFormat::Enum format, const char* szName, wdImageFormatDataType::Enum dataType, wdUInt8 uiBitsPerPixel, bool bIsStencil,
    wdUInt8 uiBitsD, wdUInt8 uiBitsS)
  {
    s_formatMetaData[format].m_szName = szName;

    s_formatMetaData[format].m_planeData[0].m_uiBitsPerBlock = uiBitsPerPixel;
    s_formatMetaData[format].m_dataType = dataType;
    s_formatMetaData[format].m_formatType = wdImageFormatType::LINEAR;

    s_formatMetaData[format].m_isDepth = true;
    s_formatMetaData[format].m_isStencil = bIsStencil;

    s_formatMetaData[format].m_uiNumChannels = bIsStencil ? 2 : 1;

    s_formatMetaData[format].m_uiBitsPerChannel[wdImageFormatChannel::D] = uiBitsD;
    s_formatMetaData[format].m_uiBitsPerChannel[wdImageFormatChannel::S] = uiBitsS;

    s_formatMetaData[format].m_asLinear = format;
    s_formatMetaData[format].m_asSrgb = format;
  }

#define INIT_FORMAT_DEPTH(format, dataType, uiBitsPerPixel, isStencil, uiBitsD, uiBitsS) \
  InitFormatDepth(wdImageFormat::format, #format, wdImageFormatDataType::dataType, uiBitsPerPixel, isStencil, uiBitsD, uiBitsS);

  void SetupSrgbPair(wdImageFormat::Enum linearFormat, wdImageFormat::Enum srgbFormat)
  {
    s_formatMetaData[linearFormat].m_asLinear = linearFormat;
    s_formatMetaData[linearFormat].m_asSrgb = srgbFormat;

    s_formatMetaData[srgbFormat].m_asLinear = linearFormat;
    s_formatMetaData[srgbFormat].m_asSrgb = srgbFormat;
  }

} // namespace

static void SetupImageFormatTable()
{
  if (!s_formatMetaData.IsEmpty())
    return;

  s_formatMetaData.SetCount(wdImageFormat::NUM_FORMATS);

  s_formatMetaData[wdImageFormat::UNKNOWN].m_szName = "UNKNOWN";

  INIT_FORMAT_LINEAR(R32G32B32A32_FLOAT, FLOAT, 128, 32, 32, 32, 32, 4);
  INIT_FORMAT_LINEAR(R32G32B32A32_UINT, UINT, 128, 32, 32, 32, 32, 4);
  INIT_FORMAT_LINEAR(R32G32B32A32_SINT, SINT, 128, 32, 32, 32, 32, 4);

  INIT_FORMAT_LINEAR(R32G32B32_FLOAT, FLOAT, 96, 32, 32, 32, 0, 3);
  INIT_FORMAT_LINEAR(R32G32B32_UINT, UINT, 96, 32, 32, 32, 0, 3);
  INIT_FORMAT_LINEAR(R32G32B32_SINT, SINT, 96, 32, 32, 32, 0, 3);

  INIT_FORMAT_LINEAR(R32G32_FLOAT, FLOAT, 64, 32, 32, 0, 0, 2);
  INIT_FORMAT_LINEAR(R32G32_UINT, UINT, 64, 32, 32, 0, 0, 2);
  INIT_FORMAT_LINEAR(R32G32_SINT, SINT, 64, 32, 32, 0, 0, 2);

  INIT_FORMAT_LINEAR(R32_FLOAT, FLOAT, 32, 32, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R32_UINT, UINT, 32, 32, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R32_SINT, SINT, 32, 32, 0, 0, 0, 1);

  INIT_FORMAT_LINEAR(R16G16B16A16_FLOAT, FLOAT, 64, 16, 16, 16, 16, 4);
  INIT_FORMAT_LINEAR(R16G16B16A16_UINT, UINT, 64, 16, 16, 16, 16, 4);
  INIT_FORMAT_LINEAR(R16G16B16A16_SINT, SINT, 64, 16, 16, 16, 16, 4);
  INIT_FORMAT_LINEAR(R16G16B16A16_UNORM, UNORM, 64, 16, 16, 16, 16, 4);
  INIT_FORMAT_LINEAR(R16G16B16A16_SNORM, SNORM, 64, 16, 16, 16, 16, 4);

  INIT_FORMAT_LINEAR(R16G16B16_UNORM, UNORM, 48, 16, 16, 16, 0, 3);

  INIT_FORMAT_LINEAR(R16G16_FLOAT, FLOAT, 32, 16, 16, 0, 0, 2);
  INIT_FORMAT_LINEAR(R16G16_UINT, UINT, 32, 16, 16, 0, 0, 2);
  INIT_FORMAT_LINEAR(R16G16_SINT, SINT, 32, 16, 16, 0, 0, 2);
  INIT_FORMAT_LINEAR(R16G16_UNORM, UNORM, 32, 16, 16, 0, 0, 2);
  INIT_FORMAT_LINEAR(R16G16_SNORM, SNORM, 32, 16, 16, 0, 0, 2);

  INIT_FORMAT_LINEAR(R16_FLOAT, FLOAT, 16, 16, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R16_UINT, UINT, 16, 16, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R16_SINT, SINT, 16, 16, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R16_UNORM, UNORM, 16, 16, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R16_SNORM, SNORM, 16, 16, 0, 0, 0, 1);

  INIT_FORMAT_LINEAR(R8G8B8A8_UINT, UINT, 32, 8, 8, 8, 8, 4);
  INIT_FORMAT_LINEAR(R8G8B8A8_SINT, SINT, 32, 8, 8, 8, 8, 4);
  INIT_FORMAT_LINEAR(R8G8B8A8_SNORM, SNORM, 32, 8, 8, 8, 8, 4);

  INIT_FORMAT_LINEAR(R8G8B8A8_UNORM, UNORM, 32, 8, 8, 8, 8, 4);
  INIT_FORMAT_LINEAR(R8G8B8A8_UNORM_SRGB, UNORM, 32, 8, 8, 8, 8, 4);
  SetupSrgbPair(wdImageFormat::R8G8B8A8_UNORM, wdImageFormat::R8G8B8A8_UNORM_SRGB);

  s_formatMetaData[wdImageFormat::R8G8B8A8_UNORM].m_uiChannelMasks[wdImageFormatChannel::R] = 0x000000FF;
  s_formatMetaData[wdImageFormat::R8G8B8A8_UNORM].m_uiChannelMasks[wdImageFormatChannel::G] = 0x0000FF00;
  s_formatMetaData[wdImageFormat::R8G8B8A8_UNORM].m_uiChannelMasks[wdImageFormatChannel::B] = 0x00FF0000;
  s_formatMetaData[wdImageFormat::R8G8B8A8_UNORM].m_uiChannelMasks[wdImageFormatChannel::A] = 0xFF000000;

  INIT_FORMAT_LINEAR(R8G8B8_UNORM, UNORM, 24, 8, 8, 8, 0, 3);
  INIT_FORMAT_LINEAR(R8G8B8_UNORM_SRGB, UNORM, 24, 8, 8, 8, 0, 3);
  SetupSrgbPair(wdImageFormat::R8G8B8_UNORM, wdImageFormat::R8G8B8_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B8G8R8A8_UNORM, UNORM, 32, 8, 8, 8, 8, 4);
  INIT_FORMAT_LINEAR(B8G8R8A8_UNORM_SRGB, UNORM, 32, 8, 8, 8, 8, 4);
  SetupSrgbPair(wdImageFormat::B8G8R8A8_UNORM, wdImageFormat::B8G8R8A8_UNORM_SRGB);

  s_formatMetaData[wdImageFormat::B8G8R8A8_UNORM].m_uiChannelMasks[wdImageFormatChannel::R] = 0x00FF0000;
  s_formatMetaData[wdImageFormat::B8G8R8A8_UNORM].m_uiChannelMasks[wdImageFormatChannel::G] = 0x0000FF00;
  s_formatMetaData[wdImageFormat::B8G8R8A8_UNORM].m_uiChannelMasks[wdImageFormatChannel::B] = 0x000000FF;
  s_formatMetaData[wdImageFormat::B8G8R8A8_UNORM].m_uiChannelMasks[wdImageFormatChannel::A] = 0xFF000000;

  INIT_FORMAT_LINEAR(B8G8R8X8_UNORM, UNORM, 32, 8, 8, 8, 0, 3);
  INIT_FORMAT_LINEAR(B8G8R8X8_UNORM_SRGB, UNORM, 32, 8, 8, 8, 0, 3);
  SetupSrgbPair(wdImageFormat::B8G8R8X8_UNORM, wdImageFormat::B8G8R8X8_UNORM_SRGB);

  s_formatMetaData[wdImageFormat::B8G8R8X8_UNORM].m_uiChannelMasks[wdImageFormatChannel::R] = 0x00FF0000;
  s_formatMetaData[wdImageFormat::B8G8R8X8_UNORM].m_uiChannelMasks[wdImageFormatChannel::G] = 0x0000FF00;
  s_formatMetaData[wdImageFormat::B8G8R8X8_UNORM].m_uiChannelMasks[wdImageFormatChannel::B] = 0x000000FF;
  s_formatMetaData[wdImageFormat::B8G8R8X8_UNORM].m_uiChannelMasks[wdImageFormatChannel::A] = 0x00000000;

  INIT_FORMAT_LINEAR(B8G8R8_UNORM, UNORM, 24, 8, 8, 8, 0, 3);
  INIT_FORMAT_LINEAR(B8G8R8_UNORM_SRGB, UNORM, 24, 8, 8, 8, 0, 3);
  SetupSrgbPair(wdImageFormat::B8G8R8_UNORM, wdImageFormat::B8G8R8_UNORM_SRGB);

  s_formatMetaData[wdImageFormat::B8G8R8_UNORM].m_uiChannelMasks[wdImageFormatChannel::R] = 0x00FF0000;
  s_formatMetaData[wdImageFormat::B8G8R8_UNORM].m_uiChannelMasks[wdImageFormatChannel::G] = 0x0000FF00;
  s_formatMetaData[wdImageFormat::B8G8R8_UNORM].m_uiChannelMasks[wdImageFormatChannel::B] = 0x000000FF;
  s_formatMetaData[wdImageFormat::B8G8R8_UNORM].m_uiChannelMasks[wdImageFormatChannel::A] = 0x00000000;

  INIT_FORMAT_LINEAR(R8G8_UINT, UINT, 16, 8, 8, 0, 0, 2);
  INIT_FORMAT_LINEAR(R8G8_SINT, SINT, 16, 8, 8, 0, 0, 2);
  INIT_FORMAT_LINEAR(R8G8_UNORM, UNORM, 16, 8, 8, 0, 0, 2);
  INIT_FORMAT_LINEAR(R8G8_SNORM, SNORM, 16, 8, 8, 0, 0, 2);

  INIT_FORMAT_LINEAR(R8_UINT, UINT, 8, 8, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R8_SINT, SINT, 8, 8, 0, 0, 0, 1);
  INIT_FORMAT_LINEAR(R8_SNORM, SNORM, 8, 8, 0, 0, 0, 1);

  INIT_FORMAT_LINEAR(R8_UNORM, UNORM, 8, 8, 0, 0, 0, 1);
  s_formatMetaData[wdImageFormat::R8_UNORM].m_uiChannelMasks[wdImageFormatChannel::R] = 0xFF;
  s_formatMetaData[wdImageFormat::R8_UNORM].m_uiChannelMasks[wdImageFormatChannel::G] = 0x00;
  s_formatMetaData[wdImageFormat::R8_UNORM].m_uiChannelMasks[wdImageFormatChannel::B] = 0x00;
  s_formatMetaData[wdImageFormat::R8_UNORM].m_uiChannelMasks[wdImageFormatChannel::A] = 0x00;

  INIT_FORMAT_COMPRESSED(BC1_UNORM, UNORM, 64, 4, 4, 1, true, 4);
  INIT_FORMAT_COMPRESSED(BC1_UNORM_SRGB, UNORM, 64, 4, 4, 1, true, 4);
  SetupSrgbPair(wdImageFormat::BC1_UNORM, wdImageFormat::BC1_UNORM_SRGB);

  INIT_FORMAT_COMPRESSED(BC2_UNORM, UNORM, 128, 4, 4, 1, true, 4);
  INIT_FORMAT_COMPRESSED(BC2_UNORM_SRGB, UNORM, 128, 4, 4, 1, true, 4);
  SetupSrgbPair(wdImageFormat::BC2_UNORM, wdImageFormat::BC2_UNORM_SRGB);

  INIT_FORMAT_COMPRESSED(BC3_UNORM, UNORM, 128, 4, 4, 1, true, 4);
  INIT_FORMAT_COMPRESSED(BC3_UNORM_SRGB, UNORM, 128, 4, 4, 1, true, 4);
  SetupSrgbPair(wdImageFormat::BC3_UNORM, wdImageFormat::BC3_UNORM_SRGB);

  INIT_FORMAT_COMPRESSED(BC4_UNORM, UNORM, 64, 4, 4, 1, true, 1);
  INIT_FORMAT_COMPRESSED(BC4_SNORM, SNORM, 64, 4, 4, 1, true, 1);

  INIT_FORMAT_COMPRESSED(BC5_UNORM, UNORM, 128, 4, 4, 1, true, 2);
  INIT_FORMAT_COMPRESSED(BC5_SNORM, SNORM, 128, 4, 4, 1, true, 2);

  INIT_FORMAT_COMPRESSED(BC6H_UF16, FLOAT, 128, 4, 4, 1, true, 3);
  INIT_FORMAT_COMPRESSED(BC6H_SF16, FLOAT, 128, 4, 4, 1, true, 3);

  INIT_FORMAT_COMPRESSED(BC7_UNORM, UNORM, 128, 4, 4, 1, true, 4);
  INIT_FORMAT_COMPRESSED(BC7_UNORM_SRGB, UNORM, 128, 4, 4, 1, true, 4);
  SetupSrgbPair(wdImageFormat::BC7_UNORM, wdImageFormat::BC7_UNORM_SRGB);



  INIT_FORMAT_LINEAR(B5G5R5A1_UNORM, UNORM, 16, 5, 5, 5, 1, 4);
  INIT_FORMAT_LINEAR(B5G5R5A1_UNORM_SRGB, UNORM, 16, 5, 5, 5, 1, 4);
  SetupSrgbPair(wdImageFormat::B5G5R5A1_UNORM, wdImageFormat::B5G5R5A1_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B4G4R4A4_UNORM, UNORM, 16, 4, 4, 4, 4, 4);
  s_formatMetaData[wdImageFormat::B4G4R4A4_UNORM].m_uiChannelMasks[wdImageFormatChannel::R] = 0x0F00;
  s_formatMetaData[wdImageFormat::B4G4R4A4_UNORM].m_uiChannelMasks[wdImageFormatChannel::G] = 0x00F0;
  s_formatMetaData[wdImageFormat::B4G4R4A4_UNORM].m_uiChannelMasks[wdImageFormatChannel::B] = 0x000F;
  s_formatMetaData[wdImageFormat::B4G4R4A4_UNORM].m_uiChannelMasks[wdImageFormatChannel::A] = 0xF000;
  INIT_FORMAT_LINEAR(B4G4R4A4_UNORM_SRGB, UNORM, 16, 4, 4, 4, 4, 4);
  SetupSrgbPair(wdImageFormat::B4G4R4A4_UNORM, wdImageFormat::B4G4R4A4_UNORM_SRGB);

  INIT_FORMAT_LINEAR(A4B4G4R4_UNORM, UNORM, 16, 4, 4, 4, 4, 4);
  s_formatMetaData[wdImageFormat::A4B4G4R4_UNORM].m_uiChannelMasks[wdImageFormatChannel::R] = 0xF000;
  s_formatMetaData[wdImageFormat::A4B4G4R4_UNORM].m_uiChannelMasks[wdImageFormatChannel::G] = 0x0F00;
  s_formatMetaData[wdImageFormat::A4B4G4R4_UNORM].m_uiChannelMasks[wdImageFormatChannel::B] = 0x00F0;
  s_formatMetaData[wdImageFormat::A4B4G4R4_UNORM].m_uiChannelMasks[wdImageFormatChannel::A] = 0x000F;
  INIT_FORMAT_LINEAR(A4B4G4R4_UNORM_SRGB, UNORM, 16, 4, 4, 4, 4, 4);
  SetupSrgbPair(wdImageFormat::A4B4G4R4_UNORM, wdImageFormat::A4B4G4R4_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B5G6R5_UNORM, UNORM, 16, 5, 6, 5, 0, 3);
  s_formatMetaData[wdImageFormat::B5G6R5_UNORM].m_uiChannelMasks[wdImageFormatChannel::R] = 0xF800;
  s_formatMetaData[wdImageFormat::B5G6R5_UNORM].m_uiChannelMasks[wdImageFormatChannel::G] = 0x07E0;
  s_formatMetaData[wdImageFormat::B5G6R5_UNORM].m_uiChannelMasks[wdImageFormatChannel::B] = 0x001F;
  s_formatMetaData[wdImageFormat::B5G6R5_UNORM].m_uiChannelMasks[wdImageFormatChannel::A] = 0x0000;
  INIT_FORMAT_LINEAR(B5G6R5_UNORM_SRGB, UNORM, 16, 5, 6, 5, 0, 3);
  SetupSrgbPair(wdImageFormat::B5G6R5_UNORM, wdImageFormat::B5G6R5_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B5G5R5A1_UNORM, UNORM, 16, 5, 5, 5, 1, 3);
  s_formatMetaData[wdImageFormat::B5G5R5A1_UNORM].m_uiChannelMasks[wdImageFormatChannel::R] = 0x7C00;
  s_formatMetaData[wdImageFormat::B5G5R5A1_UNORM].m_uiChannelMasks[wdImageFormatChannel::G] = 0x03E0;
  s_formatMetaData[wdImageFormat::B5G5R5A1_UNORM].m_uiChannelMasks[wdImageFormatChannel::B] = 0x001F;
  s_formatMetaData[wdImageFormat::B5G5R5A1_UNORM].m_uiChannelMasks[wdImageFormatChannel::A] = 0x8000;
  INIT_FORMAT_LINEAR(B5G5R5A1_UNORM_SRGB, UNORM, 16, 5, 5, 5, 1, 3);
  SetupSrgbPair(wdImageFormat::B5G5R5A1_UNORM, wdImageFormat::B5G5R5A1_UNORM_SRGB);

  INIT_FORMAT_LINEAR(B5G5R5X1_UNORM, UNORM, 16, 5, 5, 5, 0, 3);
  s_formatMetaData[wdImageFormat::B5G5R5X1_UNORM].m_uiChannelMasks[wdImageFormatChannel::R] = 0x7C00;
  s_formatMetaData[wdImageFormat::B5G5R5X1_UNORM].m_uiChannelMasks[wdImageFormatChannel::G] = 0x03E0;
  s_formatMetaData[wdImageFormat::B5G5R5X1_UNORM].m_uiChannelMasks[wdImageFormatChannel::B] = 0x001F;
  s_formatMetaData[wdImageFormat::B5G5R5X1_UNORM].m_uiChannelMasks[wdImageFormatChannel::A] = 0x0000;
  INIT_FORMAT_LINEAR(B5G5R5X1_UNORM_SRGB, UNORM, 16, 5, 5, 5, 0, 3);
  SetupSrgbPair(wdImageFormat::B5G5R5X1_UNORM, wdImageFormat::B5G5R5X1_UNORM_SRGB);

  INIT_FORMAT_LINEAR(A1B5G5R5_UNORM, UNORM, 16, 5, 5, 5, 1, 3);
  s_formatMetaData[wdImageFormat::A1B5G5R5_UNORM].m_uiChannelMasks[wdImageFormatChannel::R] = 0x7C00;
  s_formatMetaData[wdImageFormat::A1B5G5R5_UNORM].m_uiChannelMasks[wdImageFormatChannel::G] = 0x03E0;
  s_formatMetaData[wdImageFormat::A1B5G5R5_UNORM].m_uiChannelMasks[wdImageFormatChannel::B] = 0x001F;
  s_formatMetaData[wdImageFormat::A1B5G5R5_UNORM].m_uiChannelMasks[wdImageFormatChannel::A] = 0x8000;
  INIT_FORMAT_LINEAR(A1B5G5R5_UNORM_SRGB, UNORM, 16, 5, 5, 5, 1, 3);
  SetupSrgbPair(wdImageFormat::A1B5G5R5_UNORM, wdImageFormat::A1B5G5R5_UNORM_SRGB);

  INIT_FORMAT_LINEAR(X1B5G5R5_UNORM, UNORM, 16, 5, 5, 5, 1, 3);
  s_formatMetaData[wdImageFormat::X1B5G5R5_UNORM].m_uiChannelMasks[wdImageFormatChannel::R] = 0x7C00;
  s_formatMetaData[wdImageFormat::X1B5G5R5_UNORM].m_uiChannelMasks[wdImageFormatChannel::G] = 0x03E0;
  s_formatMetaData[wdImageFormat::X1B5G5R5_UNORM].m_uiChannelMasks[wdImageFormatChannel::B] = 0x001F;
  s_formatMetaData[wdImageFormat::X1B5G5R5_UNORM].m_uiChannelMasks[wdImageFormatChannel::A] = 0x0000;
  INIT_FORMAT_LINEAR(X1B5G5R5_UNORM_SRGB, UNORM, 16, 5, 5, 5, 1, 3);
  SetupSrgbPair(wdImageFormat::X1B5G5R5_UNORM, wdImageFormat::X1B5G5R5_UNORM_SRGB);

  INIT_FORMAT_LINEAR(R11G11B10_FLOAT, FLOAT, 32, 11, 11, 10, 0, 3);
  INIT_FORMAT_LINEAR(R10G10B10A2_UINT, UINT, 32, 10, 10, 10, 2, 4);
  INIT_FORMAT_LINEAR(R10G10B10A2_UNORM, UNORM, 32, 10, 10, 10, 2, 4);

  // msdn.microsoft.com/library/windows/desktop/bb943991(v=vs.85).aspx documents R10G10B10A2 as having an alpha mask of 0
  s_formatMetaData[wdImageFormat::R10G10B10A2_UNORM].m_uiChannelMasks[wdImageFormatChannel::R] = 0x000003FF;
  s_formatMetaData[wdImageFormat::R10G10B10A2_UNORM].m_uiChannelMasks[wdImageFormatChannel::G] = 0x000FFC00;
  s_formatMetaData[wdImageFormat::R10G10B10A2_UNORM].m_uiChannelMasks[wdImageFormatChannel::B] = 0x3FF00000;
  s_formatMetaData[wdImageFormat::R10G10B10A2_UNORM].m_uiChannelMasks[wdImageFormatChannel::A] = 0;

  INIT_FORMAT_DEPTH(D32_FLOAT, DEPTH_STENCIL, 32, false, 32, 0);
  INIT_FORMAT_DEPTH(D32_FLOAT_S8X24_UINT, DEPTH_STENCIL, 64, true, 32, 8);
  INIT_FORMAT_DEPTH(D24_UNORM_S8_UINT, DEPTH_STENCIL, 32, true, 24, 8);
  INIT_FORMAT_DEPTH(D16_UNORM, DEPTH_STENCIL, 16, false, 16, 0);

  INIT_FORMAT_COMPRESSED(ASTC_4x4_UNORM, UNORM, 128, 4, 4, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_5x4_UNORM, UNORM, 128, 5, 4, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_5x5_UNORM, UNORM, 128, 5, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_6x5_UNORM, UNORM, 128, 6, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_6x6_UNORM, UNORM, 128, 6, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x5_UNORM, UNORM, 128, 8, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x6_UNORM, UNORM, 128, 8, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x5_UNORM, UNORM, 128, 10, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x6_UNORM, UNORM, 128, 10, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x8_UNORM, UNORM, 128, 8, 8, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x8_UNORM, UNORM, 128, 10, 8, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x10_UNORM, UNORM, 128, 10, 10, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_12x10_UNORM, UNORM, 128, 12, 10, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_12x12_UNORM, UNORM, 128, 12, 12, 1, false, 4);

  INIT_FORMAT_COMPRESSED(ASTC_4x4_UNORM_SRGB, UNORM, 128, 4, 4, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_5x4_UNORM_SRGB, UNORM, 128, 5, 4, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_5x5_UNORM_SRGB, UNORM, 128, 5, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_6x5_UNORM_SRGB, UNORM, 128, 6, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_6x6_UNORM_SRGB, UNORM, 128, 6, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x5_UNORM_SRGB, UNORM, 128, 8, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x6_UNORM_SRGB, UNORM, 128, 8, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x5_UNORM_SRGB, UNORM, 128, 10, 5, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x6_UNORM_SRGB, UNORM, 128, 10, 6, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_8x8_UNORM_SRGB, UNORM, 128, 8, 8, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x8_UNORM_SRGB, UNORM, 128, 10, 8, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_10x10_UNORM_SRGB, UNORM, 128, 10, 10, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_12x10_UNORM_SRGB, UNORM, 128, 12, 10, 1, false, 4);
  INIT_FORMAT_COMPRESSED(ASTC_12x12_UNORM_SRGB, UNORM, 128, 12, 12, 1, false, 4);

  SetupSrgbPair(wdImageFormat::ASTC_4x4_UNORM, wdImageFormat::ASTC_4x4_UNORM_SRGB);
  SetupSrgbPair(wdImageFormat::ASTC_5x4_UNORM, wdImageFormat::ASTC_5x4_UNORM_SRGB);
  SetupSrgbPair(wdImageFormat::ASTC_5x5_UNORM, wdImageFormat::ASTC_5x5_UNORM_SRGB);
  SetupSrgbPair(wdImageFormat::ASTC_6x5_UNORM, wdImageFormat::ASTC_6x5_UNORM_SRGB);
  SetupSrgbPair(wdImageFormat::ASTC_6x6_UNORM, wdImageFormat::ASTC_6x6_UNORM_SRGB);
  SetupSrgbPair(wdImageFormat::ASTC_8x5_UNORM, wdImageFormat::ASTC_8x5_UNORM_SRGB);
  SetupSrgbPair(wdImageFormat::ASTC_8x6_UNORM, wdImageFormat::ASTC_8x6_UNORM_SRGB);
  SetupSrgbPair(wdImageFormat::ASTC_10x5_UNORM, wdImageFormat::ASTC_10x5_UNORM_SRGB);
  SetupSrgbPair(wdImageFormat::ASTC_10x6_UNORM, wdImageFormat::ASTC_10x6_UNORM_SRGB);
  SetupSrgbPair(wdImageFormat::ASTC_8x8_UNORM, wdImageFormat::ASTC_8x8_UNORM_SRGB);
  SetupSrgbPair(wdImageFormat::ASTC_10x8_UNORM, wdImageFormat::ASTC_10x8_UNORM_SRGB);
  SetupSrgbPair(wdImageFormat::ASTC_10x10_UNORM, wdImageFormat::ASTC_10x10_UNORM_SRGB);
  SetupSrgbPair(wdImageFormat::ASTC_12x10_UNORM, wdImageFormat::ASTC_12x10_UNORM_SRGB);
  SetupSrgbPair(wdImageFormat::ASTC_12x12_UNORM, wdImageFormat::ASTC_12x12_UNORM_SRGB);

  s_formatMetaData[wdImageFormat::NV12].m_szName = "NV12";
  s_formatMetaData[wdImageFormat::NV12].m_formatType = wdImageFormatType::PLANAR;
  s_formatMetaData[wdImageFormat::NV12].m_uiNumChannels = 3;

  s_formatMetaData[wdImageFormat::NV12].m_planeData.SetCount(2);

  s_formatMetaData[wdImageFormat::NV12].m_planeData[0].m_uiBitsPerBlock = 8;
  s_formatMetaData[wdImageFormat::NV12].m_planeData[0].m_uiBlockWidth = 1;
  s_formatMetaData[wdImageFormat::NV12].m_planeData[0].m_uiBlockHeight = 1;
  s_formatMetaData[wdImageFormat::NV12].m_planeData[0].m_uiBlockDepth = 1;
  s_formatMetaData[wdImageFormat::NV12].m_planeData[0].m_subFormat = wdImageFormat::R8_UNORM;

  s_formatMetaData[wdImageFormat::NV12].m_planeData[1].m_uiBitsPerBlock = 16;
  s_formatMetaData[wdImageFormat::NV12].m_planeData[1].m_uiBlockWidth = 2;
  s_formatMetaData[wdImageFormat::NV12].m_planeData[1].m_uiBlockHeight = 2;
  s_formatMetaData[wdImageFormat::NV12].m_planeData[1].m_uiBlockDepth = 1;
  s_formatMetaData[wdImageFormat::NV12].m_planeData[1].m_subFormat = wdImageFormat::R8G8_UNORM;
}

static const WD_ALWAYS_INLINE wdImageFormatMetaData& GetImageFormatMetaData(wdImageFormat::Enum format)
{
  if (s_formatMetaData.IsEmpty())
  {
    SetupImageFormatTable();
  }

  return s_formatMetaData[format];
}

// clang-format off
WD_BEGIN_SUBSYSTEM_DECLARATION(Image, ImageFormats)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    SetupImageFormatTable();
  }

WD_END_SUBSYSTEM_DECLARATION;
// clang-format on

wdUInt32 wdImageFormat::GetBitsPerPixel(Enum format, wdUInt32 uiPlaneIndex)
{
  const wdImageFormatMetaData& metaData = GetImageFormatMetaData(format);
  auto pixelsPerBlock = metaData.m_planeData[uiPlaneIndex].m_uiBlockWidth * metaData.m_planeData[uiPlaneIndex].m_uiBlockHeight * metaData.m_planeData[uiPlaneIndex].m_uiBlockDepth;
  return (metaData.m_planeData[uiPlaneIndex].m_uiBitsPerBlock + pixelsPerBlock - 1) / pixelsPerBlock; // Return rounded-up value
}


float wdImageFormat::GetExactBitsPerPixel(Enum format, wdUInt32 uiPlaneIndex)
{
  const wdImageFormatMetaData& metaData = GetImageFormatMetaData(format);
  auto pixelsPerBlock = metaData.m_planeData[uiPlaneIndex].m_uiBlockWidth * metaData.m_planeData[uiPlaneIndex].m_uiBlockHeight * metaData.m_planeData[uiPlaneIndex].m_uiBlockDepth;
  return static_cast<float>(metaData.m_planeData[uiPlaneIndex].m_uiBitsPerBlock) / pixelsPerBlock;
}


wdUInt32 wdImageFormat::GetBitsPerBlock(Enum format, wdUInt32 uiPlaneIndex)
{
  return GetImageFormatMetaData(format).m_planeData[uiPlaneIndex].m_uiBitsPerBlock;
}


wdUInt32 wdImageFormat::GetNumChannels(Enum format)
{
  return GetImageFormatMetaData(format).m_uiNumChannels;
}

wdImageFormat::Enum wdImageFormat::FromPixelMask(
  wdUInt32 uiRedMask, wdUInt32 uiGreenMask, wdUInt32 uiBlueMask, wdUInt32 uiAlphaMask, wdUInt32 uiBitsPerPixel)
{
  // Some DDS files in the wild are encoded as this
  if (uiBitsPerPixel == 8 && uiRedMask == 0xff && uiGreenMask == 0xff && uiBlueMask == 0xff)
  {
    return R8_UNORM;
  }

  for (wdUInt32 index = 0; index < NUM_FORMATS; index++)
  {
    Enum format = static_cast<Enum>(index);
    if (GetChannelMask(format, wdImageFormatChannel::R) == uiRedMask && GetChannelMask(format, wdImageFormatChannel::G) == uiGreenMask &&
        GetChannelMask(format, wdImageFormatChannel::B) == uiBlueMask && GetChannelMask(format, wdImageFormatChannel::A) == uiAlphaMask &&
        GetBitsPerPixel(format) == uiBitsPerPixel && GetDataType(format) == wdImageFormatDataType::UNORM && !IsCompressed(format))
    {
      return format;
    }
  }

  return UNKNOWN;
}


wdImageFormat::Enum wdImageFormat::GetPlaneSubFormat(Enum format, wdUInt32 uiPlaneIndex)
{
  const auto& metadata = GetImageFormatMetaData(format);

  if (metadata.m_formatType == wdImageFormatType::PLANAR)
  {
    return metadata.m_planeData[uiPlaneIndex].m_subFormat;
  }
  else
  {
    WD_ASSERT_DEV(uiPlaneIndex == 0, "Invalid plane index {0} for format {0}", uiPlaneIndex, wdImageFormat::GetName(format));
    return format;
  }
}

bool wdImageFormat::IsCompatible(Enum left, Enum right)
{
  if (left == right)
  {
    return true;
  }
  switch (left)
  {
    case wdImageFormat::R32G32B32A32_FLOAT:
    case wdImageFormat::R32G32B32A32_UINT:
    case wdImageFormat::R32G32B32A32_SINT:
      return (right == wdImageFormat::R32G32B32A32_FLOAT || right == wdImageFormat::R32G32B32A32_UINT || right == wdImageFormat::R32G32B32A32_SINT);
    case wdImageFormat::R32G32B32_FLOAT:
    case wdImageFormat::R32G32B32_UINT:
    case wdImageFormat::R32G32B32_SINT:
      return (right == wdImageFormat::R32G32B32_FLOAT || right == wdImageFormat::R32G32B32_UINT || right == wdImageFormat::R32G32B32_SINT);
    case wdImageFormat::R32G32_FLOAT:
    case wdImageFormat::R32G32_UINT:
    case wdImageFormat::R32G32_SINT:
      return (right == wdImageFormat::R32G32_FLOAT || right == wdImageFormat::R32G32_UINT || right == wdImageFormat::R32G32_SINT);
    case wdImageFormat::R32_FLOAT:
    case wdImageFormat::R32_UINT:
    case wdImageFormat::R32_SINT:
      return (right == wdImageFormat::R32_FLOAT || right == wdImageFormat::R32_UINT || right == wdImageFormat::R32_SINT);
    case wdImageFormat::R16G16B16A16_FLOAT:
    case wdImageFormat::R16G16B16A16_UINT:
    case wdImageFormat::R16G16B16A16_SINT:
    case wdImageFormat::R16G16B16A16_UNORM:
    case wdImageFormat::R16G16B16A16_SNORM:
      return (right == wdImageFormat::R16G16B16A16_FLOAT || right == wdImageFormat::R16G16B16A16_UINT || right == wdImageFormat::R16G16B16A16_SINT ||
              right == wdImageFormat::R16G16B16A16_UNORM || right == wdImageFormat::R16G16B16A16_SNORM);
    case wdImageFormat::R16G16_FLOAT:
    case wdImageFormat::R16G16_UINT:
    case wdImageFormat::R16G16_SINT:
    case wdImageFormat::R16G16_UNORM:
    case wdImageFormat::R16G16_SNORM:
      return (right == wdImageFormat::R16G16_FLOAT || right == wdImageFormat::R16G16_UINT || right == wdImageFormat::R16G16_SINT ||
              right == wdImageFormat::R16G16_UNORM || right == wdImageFormat::R16G16_SNORM);
    case wdImageFormat::R8G8B8A8_UINT:
    case wdImageFormat::R8G8B8A8_SINT:
    case wdImageFormat::R8G8B8A8_UNORM:
    case wdImageFormat::R8G8B8A8_SNORM:
    case wdImageFormat::R8G8B8A8_UNORM_SRGB:
      return (right == wdImageFormat::R8G8B8A8_UINT || right == wdImageFormat::R8G8B8A8_SINT || right == wdImageFormat::R8G8B8A8_UNORM ||
              right == wdImageFormat::R8G8B8A8_SNORM || right == wdImageFormat::R8G8B8A8_UNORM_SRGB);
    case wdImageFormat::B8G8R8A8_UNORM:
    case wdImageFormat::B8G8R8A8_UNORM_SRGB:
      return (right == wdImageFormat::B8G8R8A8_UNORM || right == wdImageFormat::B8G8R8A8_UNORM_SRGB);
    case wdImageFormat::B8G8R8X8_UNORM:
    case wdImageFormat::B8G8R8X8_UNORM_SRGB:
      return (right == wdImageFormat::B8G8R8X8_UNORM || right == wdImageFormat::B8G8R8X8_UNORM_SRGB);
    case wdImageFormat::B8G8R8_UNORM:
    case wdImageFormat::B8G8R8_UNORM_SRGB:
      return (right == wdImageFormat::B8G8R8_UNORM || right == wdImageFormat::B8G8R8_UNORM_SRGB);
    case wdImageFormat::R8G8_UINT:
    case wdImageFormat::R8G8_SINT:
    case wdImageFormat::R8G8_UNORM:
    case wdImageFormat::R8G8_SNORM:
      return (right == wdImageFormat::R8G8_UINT || right == wdImageFormat::R8G8_SINT || right == wdImageFormat::R8G8_UNORM ||
              right == wdImageFormat::R8G8_SNORM);
    case wdImageFormat::R8_UINT:
    case wdImageFormat::R8_SINT:
    case wdImageFormat::R8_UNORM:
    case wdImageFormat::R8_SNORM:
      return (
        right == wdImageFormat::R8_UINT || right == wdImageFormat::R8_SINT || right == wdImageFormat::R8_UNORM || right == wdImageFormat::R8_SNORM);
    case wdImageFormat::BC1_UNORM:
    case wdImageFormat::BC1_UNORM_SRGB:
      return (right == wdImageFormat::BC1_UNORM || right == wdImageFormat::BC1_UNORM_SRGB);
    case wdImageFormat::BC2_UNORM:
    case wdImageFormat::BC2_UNORM_SRGB:
      return (right == wdImageFormat::BC2_UNORM || right == wdImageFormat::BC2_UNORM_SRGB);
    case wdImageFormat::BC3_UNORM:
    case wdImageFormat::BC3_UNORM_SRGB:
      return (right == wdImageFormat::BC3_UNORM || right == wdImageFormat::BC3_UNORM_SRGB);
    case wdImageFormat::BC4_UNORM:
    case wdImageFormat::BC4_SNORM:
      return (right == wdImageFormat::BC4_UNORM || right == wdImageFormat::BC4_SNORM);
    case wdImageFormat::BC5_UNORM:
    case wdImageFormat::BC5_SNORM:
      return (right == wdImageFormat::BC5_UNORM || right == wdImageFormat::BC5_SNORM);
    case wdImageFormat::BC6H_UF16:
    case wdImageFormat::BC6H_SF16:
      return (right == wdImageFormat::BC6H_UF16 || right == wdImageFormat::BC6H_SF16);
    case wdImageFormat::BC7_UNORM:
    case wdImageFormat::BC7_UNORM_SRGB:
      return (right == wdImageFormat::BC7_UNORM || right == wdImageFormat::BC7_UNORM_SRGB);
    case wdImageFormat::R10G10B10A2_UINT:
    case wdImageFormat::R10G10B10A2_UNORM:
      return (right == wdImageFormat::R10G10B10A2_UINT || right == wdImageFormat::R10G10B10A2_UNORM);
    case wdImageFormat::ASTC_4x4_UNORM:
    case wdImageFormat::ASTC_4x4_UNORM_SRGB:
      return (right == wdImageFormat::ASTC_4x4_UNORM || right == wdImageFormat::ASTC_4x4_UNORM_SRGB);
    case wdImageFormat::ASTC_5x4_UNORM:
    case wdImageFormat::ASTC_5x4_UNORM_SRGB:
      return (right == wdImageFormat::ASTC_5x4_UNORM || right == wdImageFormat::ASTC_5x4_UNORM_SRGB);
    case wdImageFormat::ASTC_5x5_UNORM:
    case wdImageFormat::ASTC_5x5_UNORM_SRGB:
      return (right == wdImageFormat::ASTC_5x5_UNORM || right == wdImageFormat::ASTC_5x5_UNORM_SRGB);
    case wdImageFormat::ASTC_6x5_UNORM:
    case wdImageFormat::ASTC_6x5_UNORM_SRGB:
      return (right == wdImageFormat::ASTC_6x5_UNORM || right == wdImageFormat::ASTC_6x5_UNORM_SRGB);
    case wdImageFormat::ASTC_6x6_UNORM:
    case wdImageFormat::ASTC_6x6_UNORM_SRGB:
      return (right == wdImageFormat::ASTC_6x6_UNORM || right == wdImageFormat::ASTC_6x6_UNORM_SRGB);
    case wdImageFormat::ASTC_8x5_UNORM:
    case wdImageFormat::ASTC_8x5_UNORM_SRGB:
      return (right == wdImageFormat::ASTC_8x5_UNORM || right == wdImageFormat::ASTC_8x5_UNORM_SRGB);
    case wdImageFormat::ASTC_8x6_UNORM:
    case wdImageFormat::ASTC_8x6_UNORM_SRGB:
      return (right == wdImageFormat::ASTC_8x6_UNORM || right == wdImageFormat::ASTC_8x6_UNORM_SRGB);
    case wdImageFormat::ASTC_10x5_UNORM:
    case wdImageFormat::ASTC_10x5_UNORM_SRGB:
      return (right == wdImageFormat::ASTC_10x5_UNORM || right == wdImageFormat::ASTC_10x5_UNORM_SRGB);
    case wdImageFormat::ASTC_10x6_UNORM:
    case wdImageFormat::ASTC_10x6_UNORM_SRGB:
      return (right == wdImageFormat::ASTC_10x6_UNORM || right == wdImageFormat::ASTC_10x6_UNORM_SRGB);
    case wdImageFormat::ASTC_8x8_UNORM:
    case wdImageFormat::ASTC_8x8_UNORM_SRGB:
      return (right == wdImageFormat::ASTC_8x8_UNORM || right == wdImageFormat::ASTC_8x8_UNORM_SRGB);
    case wdImageFormat::ASTC_10x8_UNORM:
    case wdImageFormat::ASTC_10x8_UNORM_SRGB:
      return (right == wdImageFormat::ASTC_10x8_UNORM || right == wdImageFormat::ASTC_10x8_UNORM_SRGB);
    case wdImageFormat::ASTC_10x10_UNORM:
    case wdImageFormat::ASTC_10x10_UNORM_SRGB:
      return (right == wdImageFormat::ASTC_10x10_UNORM || right == wdImageFormat::ASTC_10x10_UNORM_SRGB);
    case wdImageFormat::ASTC_12x10_UNORM:
    case wdImageFormat::ASTC_12x10_UNORM_SRGB:
      return (right == wdImageFormat::ASTC_12x10_UNORM || right == wdImageFormat::ASTC_12x10_UNORM_SRGB);
    case wdImageFormat::ASTC_12x12_UNORM:
    case wdImageFormat::ASTC_12x12_UNORM_SRGB:
      return (right == wdImageFormat::ASTC_12x12_UNORM || right == wdImageFormat::ASTC_12x12_UNORM_SRGB);
    default:
      WD_ASSERT_DEV(false, "Encountered unhandled format: {0}", wdImageFormat::GetName(left));
      return false;
  }
}


bool wdImageFormat::RequiresFirstLevelBlockAlignment(Enum format)
{
  return GetImageFormatMetaData(format).m_requireFirstLevelBlockAligned;
}

const char* wdImageFormat::GetName(Enum format)
{
  return GetImageFormatMetaData(format).m_szName;
}

wdUInt32 wdImageFormat::GetPlaneCount(Enum format)
{
  return GetImageFormatMetaData(format).m_planeData.GetCount();
}

wdUInt32 wdImageFormat::GetChannelMask(Enum format, wdImageFormatChannel::Enum c)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[c];
}

wdUInt32 wdImageFormat::GetBitsPerChannel(Enum format, wdImageFormatChannel::Enum c)
{
  return GetImageFormatMetaData(format).m_uiBitsPerChannel[c];
}

wdUInt32 wdImageFormat::GetRedMask(Enum format)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[wdImageFormatChannel::R];
}

wdUInt32 wdImageFormat::GetGreenMask(Enum format)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[wdImageFormatChannel::G];
}

wdUInt32 wdImageFormat::GetBlueMask(Enum format)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[wdImageFormatChannel::B];
}

wdUInt32 wdImageFormat::GetAlphaMask(Enum format)
{
  return GetImageFormatMetaData(format).m_uiChannelMasks[wdImageFormatChannel::A];
}

wdUInt32 wdImageFormat::GetBlockWidth(Enum format, wdUInt32 uiPlaneIndex)
{
  return GetImageFormatMetaData(format).m_planeData[uiPlaneIndex].m_uiBlockWidth;
}

wdUInt32 wdImageFormat::GetBlockHeight(Enum format, wdUInt32 uiPlaneIndex)
{
  return GetImageFormatMetaData(format).m_planeData[uiPlaneIndex].m_uiBlockHeight;
}

wdUInt32 wdImageFormat::GetBlockDepth(Enum format, wdUInt32 uiPlaneIndex)
{
  return GetImageFormatMetaData(format).m_planeData[uiPlaneIndex].m_uiBlockDepth;
}

wdImageFormatDataType::Enum wdImageFormat::GetDataType(Enum format)
{
  return GetImageFormatMetaData(format).m_dataType;
}

bool wdImageFormat::IsCompressed(Enum format)
{
  return GetImageFormatMetaData(format).m_formatType == wdImageFormatType::BLOCK_COMPRESSED;
}

bool wdImageFormat::IsDepth(Enum format)
{
  return GetImageFormatMetaData(format).m_isDepth;
}

bool wdImageFormat::IsSrgb(Enum format)
{
  return GetImageFormatMetaData(format).m_asLinear != format;
}

bool wdImageFormat::IsStencil(Enum format)
{
  return GetImageFormatMetaData(format).m_isStencil;
}

wdImageFormat::Enum wdImageFormat::AsSrgb(Enum format)
{
  return GetImageFormatMetaData(format).m_asSrgb;
}

wdImageFormat::Enum wdImageFormat::AsLinear(Enum format)
{
  return GetImageFormatMetaData(format).m_asLinear;
}

wdUInt32 wdImageFormat::GetNumBlocksX(Enum format, wdUInt32 uiWidth, wdUInt32 uiPlaneIndex)
{
  return (uiWidth - 1) / GetBlockWidth(format, uiPlaneIndex) + 1;
}

wdUInt32 wdImageFormat::GetNumBlocksY(Enum format, wdUInt32 uiHeight, wdUInt32 uiPlaneIndex)
{
  return (uiHeight - 1) / GetBlockHeight(format, uiPlaneIndex) + 1;
}

wdUInt32 wdImageFormat::GetNumBlocksZ(Enum format, wdUInt32 uiDepth, wdUInt32 uiPlaneIndex)
{
  return (uiDepth - 1) / GetBlockDepth(format, uiPlaneIndex) + 1;
}

wdUInt64 wdImageFormat::GetRowPitch(Enum format, wdUInt32 uiWidth, wdUInt32 uiPlaneIndex)
{
  return static_cast<wdUInt64>(GetNumBlocksX(format, uiWidth, uiPlaneIndex)) * GetBitsPerBlock(format, uiPlaneIndex) / 8;
}

wdUInt64 wdImageFormat::GetDepthPitch(Enum format, wdUInt32 uiWidth, wdUInt32 uiHeight, wdUInt32 uiPlaneIndex)
{
  return static_cast<wdUInt64>(GetNumBlocksY(format, uiHeight, uiPlaneIndex)) * static_cast<wdUInt64>(GetRowPitch(format, uiWidth, uiPlaneIndex));
}

wdImageFormatType::Enum wdImageFormat::GetType(Enum format)
{
  return GetImageFormatMetaData(format).m_formatType;
}

WD_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageFormat);
