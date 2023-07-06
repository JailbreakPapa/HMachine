#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

#if WD_DISABLED(WD_PLATFORM_WINDOWS_UWP)

/// EXR file format support using TinyEXR.
class WD_TEXTURE_DLL wdExrFileFormat : public wdImageFileFormat
{
public:
  wdResult ReadImageHeader(wdStreamReader& stream, wdImageHeader& header, const char* szFileExtension) const override;
  wdResult ReadImage(wdStreamReader& stream, wdImage& image, const char* szFileExtension) const override;
  wdResult WriteImage(wdStreamWriter& stream, const wdImageView& image, const char* szFileExtension) const override;

  bool CanReadFileType(const char* szExtension) const override;
  bool CanWriteFileType(const char* szExtension) const override;
};

#endif
