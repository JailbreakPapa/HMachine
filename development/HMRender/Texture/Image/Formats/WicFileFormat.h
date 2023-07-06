#pragma once

#include <Texture/Image/Formats/ImageFileFormat.h>

#if WD_ENABLED(WD_PLATFORM_WINDOWS_DESKTOP)

/// \brief File format implementation for loading TIFF files using WIC
class WD_TEXTURE_DLL wdWicFileFormat : public wdImageFileFormat
{
public:
  wdWicFileFormat();
  virtual ~wdWicFileFormat();

  virtual wdResult ReadImageHeader(wdStreamReader& inout_stream, wdImageHeader& ref_header, const char* szFileExtension) const override;
  virtual wdResult ReadImage(wdStreamReader& inout_stream, wdImage& ref_image, const char* szFileExtension) const override;
  virtual wdResult WriteImage(wdStreamWriter& inout_stream, const wdImageView& image, const char* szFileExtension) const override;

  virtual bool CanReadFileType(const char* szExtension) const override;
  virtual bool CanWriteFileType(const char* szExtension) const override;

private:
  mutable bool m_bTryCoInit = true; // Helper for keeping track of whether we have tried to init COM exactly once
  mutable bool m_bCoUninitOnShutdown =
    false; // Helper for keeping track of whether we have to uninitialize COM (because we were the first to initialize it)

  wdResult ReadFileData(wdStreamReader& stream, wdDynamicArray<wdUInt8>& storage) const;
};

#endif
