#pragma once

#include <Texture/TextureDLL.h>

#include <Foundation/Reflection/Reflection.h>

struct wdTexConvOutputType
{
  enum Enum
  {
    None,
    Texture2D,
    Volume,
    Cubemap,
    Atlas,

    Default = Texture2D
  };

  using StorageType = wdUInt8;
};

struct wdTexConvCompressionMode
{
  enum Enum
  {
    // note: order of enum values matters
    None = 0,   // uncompressed
    Medium = 1, // compressed with high quality, if possible
    High = 2,   // strongest compression, if possible

    Default = Medium,
  };

  using StorageType = wdUInt8;
};

WD_DECLARE_REFLECTABLE_TYPE(WD_TEXTURE_DLL, wdTexConvCompressionMode);

struct wdTexConvUsage
{
  enum Enum
  {
    Auto, ///< Target format will be detected from heuristics (filename, content)

    // Exact format will be decided together with wdTexConvCompressionMode

    Color,
    Linear,
    Hdr,

    NormalMap,
    NormalMap_Inverted,

    BumpMap,

    Default = Auto
  };

  using StorageType = wdUInt8;
};

WD_DECLARE_REFLECTABLE_TYPE(WD_TEXTURE_DLL, wdTexConvUsage);

struct wdTexConvMipmapMode
{
  enum Enum
  {
    None, ///< Mipmap generation is disabled, output will have no mipmaps
    Linear,
    Kaiser,

    Default = Kaiser
  };

  using StorageType = wdUInt8;
};

WD_DECLARE_REFLECTABLE_TYPE(WD_TEXTURE_DLL, wdTexConvMipmapMode);

struct wdTexConvTargetPlatform
{
  enum Enum
  {
    PC,
    Android,

    Default = PC
  };

  using StorageType = wdUInt8;
};

/// \brief Defines which channel of another texture to read to get a value
struct wdTexConvChannelValue
{
  enum Enum
  {
    Red,   ///< read the RED channel
    Green, ///< read the GREEN channel
    Blue,  ///< read the BLUE channel
    Alpha, ///< read the ALPHA channel

    Black, ///< don't read any channel, just take the constant value 0
    White, ///< don't read any channel, just take the constant value 0xFF / 1.0f
  };
};

/// \brief Defines which filter kernel is used to approximate the x/y bump map gradients
struct wdTexConvBumpMapFilter
{
  enum Enum
  {
    Finite, ///< Simple finite differences in a 4-Neighborhood
    Sobel,  ///< Sobel kernel (8-Neighborhood)
    Scharr, ///< Scharr kernel (8-Neighborhood)

    Default = Finite
  };

  using StorageType = wdUInt8;
};
