#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

/// Png and jpeg file format support using stb_image.
///
/// stb_image also supports other formats, but we stick to our own loader code where we can.
/// Also, stb HDR image support is not handled here yet.
class WD_TEXTURE_DLL wdStbImageFileFormats : public wdImageFileFormat
{
public:
  virtual wdResult ReadImageHeader(wdStreamReader& inout_stream, wdImageHeader& ref_header, const char* szFileExtension) const override;
  virtual wdResult ReadImage(wdStreamReader& inout_stream, wdImage& ref_image, const char* szFileExtension) const override;
  virtual wdResult WriteImage(wdStreamWriter& inout_stream, const wdImageView& image, const char* szFileExtension) const override;

  virtual bool CanReadFileType(const char* szExtension) const override;
  virtual bool CanWriteFileType(const char* szExtension) const override;
};
