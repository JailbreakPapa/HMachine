#pragma once

#include <Texture/Image/ImageFormat.h>

/// \brief Helper class containing methods to convert between wdImageFormat::Enum and platform-specific image formats.
class WD_TEXTURE_DLL wdImageFormatMappings
{
public:
  /// \brief Maps an wdImageFormat::Enum to an equivalent Direct3D DXGI_FORMAT.
  static wdUInt32 ToDxgiFormat(wdImageFormat::Enum format);

  /// \brief Maps a Direct3D DXGI_FORMAT to an equivalent wdImageFormat::Enum.
  static wdImageFormat::Enum FromDxgiFormat(wdUInt32 uiDxgiFormat);

  /// \brief Maps an wdImageFormat::Enum to an equivalent FourCC code.
  static wdUInt32 ToFourCc(wdImageFormat::Enum format);

  /// \brief Maps a FourCC code to an equivalent wdImageFormat::Enum.
  static wdImageFormat::Enum FromFourCc(wdUInt32 uiFourCc);
};
