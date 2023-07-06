#pragma once

#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <Texture/TextureDLL.h>
#include <Foundation/Types/Bitflags.h>

class wdStreamReader;
class wdStreamWriter;
class wdImage;
class wdImageView;
class wdStringBuilder;
class wdImageHeader;

class WD_TEXTURE_DLL wdImageFileFormat : public wdEnumerable<wdImageFileFormat>
{
public:
  /// \brief Reads only the header information for an image and ignores the data. Much faster than reading the entire image, if the pixel data is not needed.
  virtual wdResult ReadImageHeader(wdStreamReader& inout_stream, wdImageHeader& ref_header, const char* szFileExtension) const = 0;

  /// \brief Reads the data from the given stream and creates the image from it. Errors are written to the given wdLogInterface.
  virtual wdResult ReadImage(wdStreamReader& inout_stream, wdImage& ref_image, const char* szFileExtension) const = 0;

  /// \brief Writes the data to the given stream in this format. Errors are written to the given wdLogInterface.
  virtual wdResult WriteImage(wdStreamWriter& inout_stream, const wdImageView& image, const char* szFileExtension) const = 0;

  /// \brief Should return true, if files with the given extension can be read.
  virtual bool CanReadFileType(const char* szExtension) const = 0;

  /// \brief Should return true, if files with the given extension can be written.
  virtual bool CanWriteFileType(const char* szExtension) const = 0;

  /// \brief Returns an wdImageFileFormat that can read the given extension. Returns nullptr if there is no appropriate wdImageFileFormat.
  static wdImageFileFormat* GetReaderFormat(const char* szExtension);

  /// \brief Returns an wdImageFileFormat that can write the given extension. Returns nullptr if there is no appropriate wdImageFileFormat.
  static wdImageFileFormat* GetWriterFormat(const char* szExtension);

  static wdResult ReadImageHeader(const char* szFileName, wdImageHeader& ref_header);

  WD_DECLARE_ENUMERABLE_CLASS(wdImageFileFormat);
};
