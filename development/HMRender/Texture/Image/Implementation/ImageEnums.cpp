#include <Texture/TexturePCH.h>

#include <Texture/Image/ImageEnums.h>

// clang-format off
WD_BEGIN_STATIC_REFLECTED_ENUM(wdImageAddressMode, 1)
  WD_ENUM_CONSTANT(wdImageAddressMode::Repeat),
  WD_ENUM_CONSTANT(wdImageAddressMode::Clamp),
  WD_ENUM_CONSTANT(wdImageAddressMode::ClampBorder),
  WD_ENUM_CONSTANT(wdImageAddressMode::Mirror),
WD_END_STATIC_REFLECTED_ENUM;
// clang-format on

// clang-format off
WD_BEGIN_STATIC_REFLECTED_ENUM(wdTextureFilterSetting, 1)
  WD_ENUM_CONSTANT(wdTextureFilterSetting::FixedNearest),
  WD_ENUM_CONSTANT(wdTextureFilterSetting::FixedBilinear),
  WD_ENUM_CONSTANT(wdTextureFilterSetting::FixedTrilinear),
  WD_ENUM_CONSTANT(wdTextureFilterSetting::FixedAnisotropic2x),
  WD_ENUM_CONSTANT(wdTextureFilterSetting::FixedAnisotropic4x),
  WD_ENUM_CONSTANT(wdTextureFilterSetting::FixedAnisotropic8x),
  WD_ENUM_CONSTANT(wdTextureFilterSetting::FixedAnisotropic16x),
  WD_ENUM_CONSTANT(wdTextureFilterSetting::LowestQuality),
  WD_ENUM_CONSTANT(wdTextureFilterSetting::LowQuality),
  WD_ENUM_CONSTANT(wdTextureFilterSetting::DefaultQuality),
  WD_ENUM_CONSTANT(wdTextureFilterSetting::HighQuality),
  WD_ENUM_CONSTANT(wdTextureFilterSetting::HighestQuality),
WD_END_STATIC_REFLECTED_ENUM;
// clang-format on


WD_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageEnums);
