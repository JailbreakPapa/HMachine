#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Types.h>
#include <Texture/TextureDLL.h>

struct WD_TEXTURE_DLL wdImageAddressMode
{
  using StorageType = wdUInt8;

  enum Enum
  {
    Repeat,
    Clamp,
    ClampBorder,
    Mirror,

    ENUM_COUNT,

    Default = Repeat
  };
};

WD_DECLARE_REFLECTABLE_TYPE(WD_TEXTURE_DLL, wdImageAddressMode);

//////////////////////////////////////////////////////////////////////////
// wdTextureFilterSetting
//////////////////////////////////////////////////////////////////////////

struct WD_TEXTURE_DLL wdTextureFilterSetting
{
  using StorageType = wdUInt8;

  enum Enum
  {
    FixedNearest,
    FixedBilinear,
    FixedTrilinear,
    FixedAnisotropic2x,
    FixedAnisotropic4x,
    FixedAnisotropic8x,
    FixedAnisotropic16x,

    LowestQuality,
    LowQuality,
    DefaultQuality,
    HighQuality,
    HighestQuality,

    Default = DefaultQuality
  };
};

WD_DECLARE_REFLECTABLE_TYPE(WD_TEXTURE_DLL, wdTextureFilterSetting);
