#include <Texture/TexturePCH.h>

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Formats/BmpFileFormat.h>
#include <Texture/Image/ImageConversion.h>

wdBmpFileFormat g_bmpFormat;

enum wdBmpCompression
{
  RGB = 0L,
  RLE8 = 1L,
  RLE4 = 2L,
  BITFIELDS = 3L,
  JPEG = 4L,
  PNG = 5L,
};


#pragma pack(push, 1)
struct wdBmpFileHeader
{
  wdUInt16 m_type = 0;
  wdUInt32 m_size = 0;
  wdUInt16 m_reserved1 = 0;
  wdUInt16 m_reserved2 = 0;
  wdUInt32 m_offBits = 0;
};
#pragma pack(pop)

struct wdBmpFileInfoHeader
{
  wdUInt32 m_size = 0;
  wdUInt32 m_width = 0;
  wdUInt32 m_height = 0;
  wdUInt16 m_planes = 0;
  wdUInt16 m_bitCount = 0;
  wdBmpCompression m_compression = wdBmpCompression::RGB;
  wdUInt32 m_sizeImage = 0;
  wdUInt32 m_xPelsPerMeter = 0;
  wdUInt32 m_yPelsPerMeter = 0;
  wdUInt32 m_clrUsed = 0;
  wdUInt32 m_clrImportant = 0;
};

struct wdCIEXYZ
{
  int ciexyzX = 0;
  int ciexyzY = 0;
  int ciexyzZ = 0;
};

struct wdCIEXYZTRIPLE
{
  wdCIEXYZ ciexyzRed;
  wdCIEXYZ ciexyzGreen;
  wdCIEXYZ ciexyzBlue;
};

struct wdBmpFileInfoHeaderV4
{
  wdUInt32 m_redMask = 0;
  wdUInt32 m_greenMask = 0;
  wdUInt32 m_blueMask = 0;
  wdUInt32 m_alphaMask = 0;
  wdUInt32 m_csType = 0;
  wdCIEXYZTRIPLE m_endpoints;
  wdUInt32 m_gammaRed = 0;
  wdUInt32 m_gammaGreen = 0;
  wdUInt32 m_gammaBlue = 0;
};

WD_CHECK_AT_COMPILETIME(sizeof(wdCIEXYZTRIPLE) == 3 * 3 * 4);

// just to be on the safe side
#if WD_ENABLED(WD_PLATFORM_WINDOWS)
WD_CHECK_AT_COMPILETIME(sizeof(wdCIEXYZTRIPLE) == sizeof(CIEXYZTRIPLE));
#endif

struct wdBmpFileInfoHeaderV5
{
  wdUInt32 m_intent;
  wdUInt32 m_profileData;
  wdUInt32 m_profileSize;
  wdUInt32 m_reserved;
};

static const wdUInt16 wdBmpFileMagic = 0x4D42u;

struct wdBmpBgrxQuad
{
  WD_DECLARE_POD_TYPE();

  wdBmpBgrxQuad() {}

  wdBmpBgrxQuad(wdUInt8 uiRed, wdUInt8 uiGreen, wdUInt8 uiBlue)
    : m_blue(uiBlue)
    , m_green(uiGreen)
    , m_red(uiRed)
    , m_reserved(0)
  {
  }

  wdUInt8 m_blue;
  wdUInt8 m_green;
  wdUInt8 m_red;
  wdUInt8 m_reserved;
};

wdResult wdBmpFileFormat::WriteImage(wdStreamWriter& inout_stream, const wdImageView& image, const char* szFileExtension) const
{
  // Technically almost arbitrary formats are supported, but we only use the common ones.
  wdImageFormat::Enum compatibleFormats[] = {
    wdImageFormat::B8G8R8X8_UNORM,
    wdImageFormat::B8G8R8A8_UNORM,
    wdImageFormat::B8G8R8_UNORM,
    wdImageFormat::B5G5R5X1_UNORM,
    wdImageFormat::B5G6R5_UNORM,
  };

  // Find a compatible format closest to the one the image currently has
  wdImageFormat::Enum format = wdImageConversion::FindClosestCompatibleFormat(image.GetImageFormat(), compatibleFormats);

  if (format == wdImageFormat::UNKNOWN)
  {
    wdLog::Error("No conversion from format '{0}' to a format suitable for BMP files known.", wdImageFormat::GetName(image.GetImageFormat()));
    return WD_FAILURE;
  }

  // Convert if not already in a compatible format
  if (format != image.GetImageFormat())
  {
    wdImage convertedImage;
    if (wdImageConversion::Convert(image, convertedImage, format) != WD_SUCCESS)
    {
      // This should never happen
      WD_ASSERT_DEV(false, "wdImageConversion::Convert failed even though the conversion was to the format returned by FindClosestCompatibleFormat.");
      return WD_FAILURE;
    }

    return WriteImage(inout_stream, convertedImage, szFileExtension);
  }

  wdUInt64 uiRowPitch = image.GetRowPitch(0);

  wdUInt32 uiHeight = image.GetHeight(0);

  wdUInt64 dataSize = uiRowPitch * uiHeight;
  if (dataSize >= wdMath::MaxValue<wdUInt32>())
  {
    WD_ASSERT_DEV(false, "Size overflow in BMP file format.");
    return WD_FAILURE;
  }

  wdBmpFileInfoHeader fileInfoHeader;
  fileInfoHeader.m_width = image.GetWidth(0);
  fileInfoHeader.m_height = uiHeight;
  fileInfoHeader.m_planes = 1;
  fileInfoHeader.m_bitCount = static_cast<wdUInt16>(wdImageFormat::GetBitsPerPixel(format));

  fileInfoHeader.m_sizeImage = 0; // Can be zero unless we store the data compressed

  fileInfoHeader.m_xPelsPerMeter = 0;
  fileInfoHeader.m_yPelsPerMeter = 0;
  fileInfoHeader.m_clrUsed = 0;
  fileInfoHeader.m_clrImportant = 0;

  bool bWriteColorMask = false;

  // Prefer to write a V3 header
  wdUInt32 uiHeaderVersion = 3;

  switch (format)
  {
    case wdImageFormat::B8G8R8X8_UNORM:
    case wdImageFormat::B5G5R5X1_UNORM:
    case wdImageFormat::B8G8R8_UNORM:
      fileInfoHeader.m_compression = RGB;
      break;

    case wdImageFormat::B8G8R8A8_UNORM:
      fileInfoHeader.m_compression = BITFIELDS;
      uiHeaderVersion = 4;
      break;

    case wdImageFormat::B5G6R5_UNORM:
      fileInfoHeader.m_compression = BITFIELDS;
      bWriteColorMask = true;
      break;

    default:
      return WD_FAILURE;
  }

  WD_ASSERT_DEV(!bWriteColorMask || uiHeaderVersion <= 3, "Internal bug");

  wdUInt32 uiFileInfoHeaderSize = sizeof(wdBmpFileInfoHeader);
  wdUInt32 uiHeaderSize = sizeof(wdBmpFileHeader);

  if (uiHeaderVersion >= 4)
  {
    uiFileInfoHeaderSize += sizeof(wdBmpFileInfoHeaderV4);
  }
  else if (bWriteColorMask)
  {
    uiHeaderSize += 3 * sizeof(wdUInt32);
  }

  uiHeaderSize += uiFileInfoHeaderSize;

  fileInfoHeader.m_size = uiFileInfoHeaderSize;

  wdBmpFileHeader header;
  header.m_type = wdBmpFileMagic;
  header.m_size = uiHeaderSize + static_cast<wdUInt32>(dataSize);
  header.m_reserved1 = 0;
  header.m_reserved2 = 0;
  header.m_offBits = uiHeaderSize;

  // Write all data
  if (inout_stream.WriteBytes(&header, sizeof(header)) != WD_SUCCESS)
  {
    wdLog::Error("Failed to write header.");
    return WD_FAILURE;
  }

  if (inout_stream.WriteBytes(&fileInfoHeader, sizeof(fileInfoHeader)) != WD_SUCCESS)
  {
    wdLog::Error("Failed to write fileInfoHeader.");
    return WD_FAILURE;
  }

  if (uiHeaderVersion >= 4)
  {
    wdBmpFileInfoHeaderV4 fileInfoHeaderV4;
    memset(&fileInfoHeaderV4, 0, sizeof(fileInfoHeaderV4));

    fileInfoHeaderV4.m_redMask = wdImageFormat::GetRedMask(format);
    fileInfoHeaderV4.m_greenMask = wdImageFormat::GetGreenMask(format);
    fileInfoHeaderV4.m_blueMask = wdImageFormat::GetBlueMask(format);
    fileInfoHeaderV4.m_alphaMask = wdImageFormat::GetAlphaMask(format);

    if (inout_stream.WriteBytes(&fileInfoHeaderV4, sizeof(fileInfoHeaderV4)) != WD_SUCCESS)
    {
      wdLog::Error("Failed to write fileInfoHeaderV4.");
      return WD_FAILURE;
    }
  }
  else if (bWriteColorMask)
  {
    struct
    {
      wdUInt32 m_red;
      wdUInt32 m_green;
      wdUInt32 m_blue;
    } colorMask;


    colorMask.m_red = wdImageFormat::GetRedMask(format);
    colorMask.m_green = wdImageFormat::GetGreenMask(format);
    colorMask.m_blue = wdImageFormat::GetBlueMask(format);

    if (inout_stream.WriteBytes(&colorMask, sizeof(colorMask)) != WD_SUCCESS)
    {
      wdLog::Error("Failed to write colorMask.");
      return WD_FAILURE;
    }
  }

  const wdUInt64 uiPaddedRowPitch = ((uiRowPitch - 1) / 4 + 1) * 4;
  // Write rows in reverse order
  for (wdInt32 iRow = uiHeight - 1; iRow >= 0; iRow--)
  {
    if (inout_stream.WriteBytes(image.GetPixelPointer<void>(0, 0, 0, 0, iRow, 0), uiRowPitch) != WD_SUCCESS)
    {
      wdLog::Error("Failed to write data.");
      return WD_FAILURE;
    }

    wdUInt8 zeroes[4] = {0, 0, 0, 0};
    if (inout_stream.WriteBytes(zeroes, uiPaddedRowPitch - uiRowPitch) != WD_SUCCESS)
    {
      wdLog::Error("Failed to write data.");
      return WD_FAILURE;
    }
  }

  return WD_SUCCESS;
}

namespace
{
  wdUInt32 ExtractBits(const void* pData, wdUInt32 uiBitAddress, wdUInt32 uiNumBits)
  {
    wdUInt32 uiMask = (1U << uiNumBits) - 1;
    wdUInt32 uiByteAddress = uiBitAddress / 8;
    wdUInt32 uiShiftAmount = 7 - (uiBitAddress % 8 + uiNumBits - 1);

    return (reinterpret_cast<const wdUInt8*>(pData)[uiByteAddress] >> uiShiftAmount) & uiMask;
  }

  wdResult ReadImageInfo(wdStreamReader& inout_stream, wdImageHeader& ref_header, wdBmpFileHeader& ref_fileHeader, wdBmpFileInfoHeader& ref_fileInfoHeader, bool& ref_bIndexed,
    bool& ref_bCompressed, wdUInt32& ref_uiBpp, wdUInt32& ref_uiDataSize)
  {
    if (inout_stream.ReadBytes(&ref_fileHeader, sizeof(wdBmpFileHeader)) != sizeof(wdBmpFileHeader))
    {
      wdLog::Error("Failed to read header data.");
      return WD_FAILURE;
    }

    // Some very old BMP variants may have different magic numbers, but we don't support them.
    if (ref_fileHeader.m_type != wdBmpFileMagic)
    {
      wdLog::Error("The file is not a recognized BMP file.");
      return WD_FAILURE;
    }

    // We expect at least header version 3
    wdUInt32 uiHeaderVersion = 3;
    if (inout_stream.ReadBytes(&ref_fileInfoHeader, sizeof(wdBmpFileInfoHeader)) != sizeof(wdBmpFileInfoHeader))
    {
      wdLog::Error("Failed to read header data (V3).");
      return WD_FAILURE;
    }

    int remainingHeaderBytes = ref_fileInfoHeader.m_size - sizeof(ref_fileInfoHeader);

    // File header shorter than expected - happens with corrupt files or e.g. with OS/2 BMP files which may have shorter headers
    if (remainingHeaderBytes < 0)
    {
      wdLog::Error("The file header was shorter than expected.");
      return WD_FAILURE;
    }

    // Newer files may have a header version 4 (required for transparency)
    wdBmpFileInfoHeaderV4 fileInfoHeaderV4;
    if (remainingHeaderBytes >= sizeof(wdBmpFileInfoHeaderV4))
    {
      uiHeaderVersion = 4;
      if (inout_stream.ReadBytes(&fileInfoHeaderV4, sizeof(wdBmpFileInfoHeaderV4)) != sizeof(wdBmpFileInfoHeaderV4))
      {
        wdLog::Error("Failed to read header data (V4).");
        return WD_FAILURE;
      }
      remainingHeaderBytes -= sizeof(wdBmpFileInfoHeaderV4);
    }

    // Skip rest of header
    if (inout_stream.SkipBytes(remainingHeaderBytes) != remainingHeaderBytes)
    {
      wdLog::Error("Failed to skip remaining header data.");
      return WD_FAILURE;
    }

    ref_uiBpp = ref_fileInfoHeader.m_bitCount;

    // Find target format to load the image
    wdImageFormat::Enum format = wdImageFormat::UNKNOWN;

    switch (ref_fileInfoHeader.m_compression)
    {
        // RGB or indexed data
      case RGB:
        switch (ref_uiBpp)
        {
          case 1:
          case 4:
          case 8:
            ref_bIndexed = true;

            // We always decompress indexed to BGRX, since the palette is specified in this format
            format = wdImageFormat::B8G8R8X8_UNORM;
            break;

          case 16:
            format = wdImageFormat::B5G5R5X1_UNORM;
            break;

          case 24:
            format = wdImageFormat::B8G8R8_UNORM;
            break;

          case 32:
            format = wdImageFormat::B8G8R8X8_UNORM;
        }
        break;

        // RGB data, but with the color masks specified in place of the palette
      case BITFIELDS:
        switch (ref_uiBpp)
        {
          case 16:
          case 32:
            // In case of old headers, the color masks appear after the header (and aren't counted as part of it)
            if (uiHeaderVersion < 4)
            {
              // Color masks (w/o alpha channel)
              struct
              {
                wdUInt32 m_red;
                wdUInt32 m_green;
                wdUInt32 m_blue;
              } colorMask;

              if (inout_stream.ReadBytes(&colorMask, sizeof(colorMask)) != sizeof(colorMask))
              {
                return WD_FAILURE;
              }

              format = wdImageFormat::FromPixelMask(colorMask.m_red, colorMask.m_green, colorMask.m_blue, 0, ref_uiBpp);
            }
            else
            {
              // For header version four and higher, the color masks are part of the header
              format = wdImageFormat::FromPixelMask(
                fileInfoHeaderV4.m_redMask, fileInfoHeaderV4.m_greenMask, fileInfoHeaderV4.m_blueMask, fileInfoHeaderV4.m_alphaMask, ref_uiBpp);
            }

            break;
        }
        break;

      case RLE4:
        if (ref_uiBpp == 4)
        {
          ref_bIndexed = true;
          ref_bCompressed = true;
          format = wdImageFormat::B8G8R8X8_UNORM;
        }
        break;

      case RLE8:
        if (ref_uiBpp == 8)
        {
          ref_bIndexed = true;
          ref_bCompressed = true;
          format = wdImageFormat::B8G8R8X8_UNORM;
        }
        break;

      default:
        WD_ASSERT_NOT_IMPLEMENTED;
    }

    if (format == wdImageFormat::UNKNOWN)
    {
      wdLog::Error("Unknown or unsupported BMP encoding.");
      return WD_FAILURE;
    }

    const wdUInt32 uiWidth = ref_fileInfoHeader.m_width;

    if (uiWidth > 65536)
    {
      wdLog::Error("Image specifies width > 65536. Header corrupted?");
      return WD_FAILURE;
    }

    const wdUInt32 uiHeight = ref_fileInfoHeader.m_height;

    if (uiHeight > 65536)
    {
      wdLog::Error("Image specifies height > 65536. Header corrupted?");
      return WD_FAILURE;
    }

    ref_uiDataSize = ref_fileInfoHeader.m_sizeImage;

    if (ref_uiDataSize > 1024 * 1024 * 1024)
    {
      wdLog::Error("Image specifies data size > 1GiB. Header corrupted?");
      return WD_FAILURE;
    }

    const int uiRowPitchIn = (uiWidth * ref_uiBpp + 31) / 32 * 4;

    if (ref_uiDataSize == 0)
    {
      if (ref_fileInfoHeader.m_compression != RGB)
      {
        wdLog::Error("The data size wasn't specified in the header.");
        return WD_FAILURE;
      }
      ref_uiDataSize = uiRowPitchIn * uiHeight;
    }

    // Set image data
    ref_header.SetImageFormat(format);
    ref_header.SetNumMipLevels(1);
    ref_header.SetNumArrayIndices(1);
    ref_header.SetNumFaces(1);

    ref_header.SetWidth(uiWidth);
    ref_header.SetHeight(uiHeight);
    ref_header.SetDepth(1);

    return WD_SUCCESS;
  }

} // namespace

wdResult wdBmpFileFormat::ReadImageHeader(wdStreamReader& inout_stream, wdImageHeader& ref_header, const char* szFileExtension) const
{
  WD_PROFILE_SCOPE("wdBmpFileFormat::ReadImage");

  wdBmpFileHeader fileHeader;
  wdBmpFileInfoHeader fileInfoHeader;
  bool bIndexed = false, bCompressed = false;
  wdUInt32 uiBpp = 0;
  wdUInt32 uiDataSize = 0;

  return ReadImageInfo(inout_stream, ref_header, fileHeader, fileInfoHeader, bIndexed, bCompressed, uiBpp, uiDataSize);
}

wdResult wdBmpFileFormat::ReadImage(wdStreamReader& inout_stream, wdImage& ref_image, const char* szFileExtension) const
{
  WD_PROFILE_SCOPE("wdBmpFileFormat::ReadImage");

  wdBmpFileHeader fileHeader;
  wdImageHeader header;
  wdBmpFileInfoHeader fileInfoHeader;
  bool bIndexed = false, bCompressed = false;
  wdUInt32 uiBpp = 0;
  wdUInt32 uiDataSize = 0;

  WD_SUCCEED_OR_RETURN(ReadImageInfo(inout_stream, header, fileHeader, fileInfoHeader, bIndexed, bCompressed, uiBpp, uiDataSize));

  ref_image.ResetAndAlloc(header);

  wdUInt64 uiRowPitch = ref_image.GetRowPitch(0);

  const int uiRowPitchIn = (header.GetWidth() * uiBpp + 31) / 32 * 4;

  if (bIndexed)
  {
    // If no palette size was specified, the full available palette size will be used
    wdUInt32 paletteSize = fileInfoHeader.m_clrUsed;
    if (paletteSize == 0)
    {
      paletteSize = 1U << uiBpp;
    }
    else if (paletteSize > 65536)
    {
      wdLog::Error("Palette size > 65536.");
      return WD_FAILURE;
    }

    wdDynamicArray<wdBmpBgrxQuad> palette;
    palette.SetCountUninitialized(paletteSize);
    if (inout_stream.ReadBytes(&palette[0], paletteSize * sizeof(wdBmpBgrxQuad)) != paletteSize * sizeof(wdBmpBgrxQuad))
    {
      wdLog::Error("Failed to read palette data.");
      return WD_FAILURE;
    }

    if (bCompressed)
    {
      // Compressed data is always in pairs of bytes
      if (uiDataSize % 2 != 0)
      {
        wdLog::Error("The data size is not a multiple of 2 bytes in an RLE-compressed file.");
        return WD_FAILURE;
      }

      wdDynamicArray<wdUInt8> compressedData;
      compressedData.SetCountUninitialized(uiDataSize);

      if (inout_stream.ReadBytes(&compressedData[0], uiDataSize) != uiDataSize)
      {
        wdLog::Error("Failed to read data.");
        return WD_FAILURE;
      }

      const wdUInt8* pIn = &compressedData[0];
      const wdUInt8* pInEnd = pIn + uiDataSize;

      // Current output position
      wdUInt32 uiRow = fileInfoHeader.m_height - 1;
      wdUInt32 uiCol = 0;

      wdBmpBgrxQuad* pLine = ref_image.GetPixelPointer<wdBmpBgrxQuad>(0, 0, 0, 0, uiRow, 0);

      // Decode RLE data directly to RGBX
      while (pIn < pInEnd)
      {
        wdUInt32 uiByte1 = *pIn++;
        wdUInt32 uiByte2 = *pIn++;

        // Relative mode - the first byte specified a number of indices to be repeated, the second one the indices
        if (uiByte1 > 0)
        {
          // Clamp number of repetitions to row width.
          // The spec isn't clear on this point, but some files pad the number of encoded indices for some reason.
          uiByte1 = wdMath::Min(uiByte1, fileInfoHeader.m_width - uiCol);

          if (uiBpp == 4)
          {
            // Alternate between two indices.
            for (wdUInt32 uiRep = 0; uiRep < uiByte1 / 2; uiRep++)
            {
              pLine[uiCol++] = palette[uiByte2 >> 4];
              pLine[uiCol++] = palette[uiByte2 & 0x0F];
            }

            // Repeat the first index for odd numbers of repetitions.
            if (uiByte1 & 1)
            {
              pLine[uiCol++] = palette[uiByte2 >> 4];
            }
          }
          else /* if (uiBpp == 8) */
          {
            // Repeat a single index.
            for (wdUInt32 uiRep = 0; uiRep < uiByte1; uiRep++)
            {
              pLine[uiCol++] = palette[uiByte2];
            }
          }
        }
        else
        {
          // Absolute mode - the first byte specifies a number of indices encoded separately, or is a special marker
          switch (uiByte2)
          {
              // End of line marker
            case 0:
            {

              // Fill up with palette entry 0
              while (uiCol < fileInfoHeader.m_width)
              {
                pLine[uiCol++] = palette[0];
              }

              // Begin next line
              uiCol = 0;
              uiRow--;
              pLine -= fileInfoHeader.m_width;
            }

            break;

              // End of image marker
            case 1:
              // Check that we really reached the end of the image.
              if (uiRow != 0 && uiCol != fileInfoHeader.m_height - 1)
              {
                wdLog::Error("Unexpected end of image marker found.");
                return WD_FAILURE;
              }
              break;

            case 2:
              wdLog::Error("Found a RLE compression position delta - this is not supported.");
              return WD_FAILURE;

            default:
              // Read uiByte2 number of indices

              // More data than fits into the image or can be read?
              if (uiCol + uiByte2 > fileInfoHeader.m_width || pIn + (uiByte2 + 1) / 2 > pInEnd)
              {
                return WD_FAILURE;
              }

              if (uiBpp == 4)
              {
                for (wdUInt32 uiRep = 0; uiRep < uiByte2 / 2; uiRep++)
                {
                  wdUInt32 uiIndices = *pIn++;
                  pLine[uiCol++] = palette[uiIndices >> 4];
                  pLine[uiCol++] = palette[uiIndices & 0x0f];
                }

                if (uiByte2 & 1)
                {
                  pLine[uiCol++] = palette[*pIn++ >> 4];
                }

                // Pad to word boundary
                pIn += (uiByte2 / 2 + uiByte2) & 1;
              }
              else /* if (uiBpp == 8) */
              {
                for (wdUInt32 uiRep = 0; uiRep < uiByte2; uiRep++)
                {
                  pLine[uiCol++] = palette[*pIn++];
                }

                // Pad to word boundary
                pIn += uiByte2 & 1;
              }
          }
        }
      }
    }
    else
    {
      wdDynamicArray<wdUInt8> indexedData;
      indexedData.SetCountUninitialized(uiDataSize);
      if (inout_stream.ReadBytes(&indexedData[0], uiDataSize) != uiDataSize)
      {
        wdLog::Error("Failed to read data.");
        return WD_FAILURE;
      }

      // Convert to non-indexed
      for (wdUInt32 uiRow = 0; uiRow < fileInfoHeader.m_height; uiRow++)
      {
        wdUInt8* pIn = &indexedData[uiRowPitchIn * uiRow];

        // Convert flipped vertically
        wdBmpBgrxQuad* pOut = ref_image.GetPixelPointer<wdBmpBgrxQuad>(0, 0, 0, 0, fileInfoHeader.m_height - uiRow - 1, 0);
        for (wdUInt32 uiCol = 0; uiCol < ref_image.GetWidth(0); uiCol++)
        {
          wdUInt32 uiIndex = ExtractBits(pIn, uiCol * uiBpp, uiBpp);
          if (uiIndex >= palette.GetCount())
          {
            wdLog::Error("Image contains invalid palette indices.");
            return WD_FAILURE;
          }
          pOut[uiCol] = palette[uiIndex];
        }
      }
    }
  }
  else
  {
    // Format must match the number of bits in the file
    if (wdImageFormat::GetBitsPerPixel(header.GetImageFormat()) != uiBpp)
    {
      wdLog::Error("The number of bits per pixel specified in the file ({0}) does not match the expected value of {1} for the format '{2}'.",
        uiBpp, wdImageFormat::GetBitsPerPixel(header.GetImageFormat()), wdImageFormat::GetName(header.GetImageFormat()));
      return WD_FAILURE;
    }

    // Skip palette data. Having a palette here doesn't make sense, but is not explicitly disallowed by the standard.
    wdUInt32 paletteSize = fileInfoHeader.m_clrUsed * sizeof(wdBmpBgrxQuad);
    if (inout_stream.SkipBytes(paletteSize) != paletteSize)
    {
      wdLog::Error("Failed to skip palette data.");
      return WD_FAILURE;
    }

    // Read rows in reverse order
    for (wdInt32 iRow = fileInfoHeader.m_height - 1; iRow >= 0; iRow--)
    {
      if (inout_stream.ReadBytes(ref_image.GetPixelPointer<void>(0, 0, 0, 0, iRow, 0), uiRowPitch) != uiRowPitch)
      {
        wdLog::Error("Failed to read row data.");
        return WD_FAILURE;
      }
      if (inout_stream.SkipBytes(uiRowPitchIn - uiRowPitch) != uiRowPitchIn - uiRowPitch)
      {
        wdLog::Error("Failed to skip row data.");
        return WD_FAILURE;
      }
    }
  }

  return WD_SUCCESS;
}

bool wdBmpFileFormat::CanReadFileType(const char* szExtension) const
{
  return wdStringUtils::IsEqual_NoCase(szExtension, "bmp") || wdStringUtils::IsEqual_NoCase(szExtension, "dib") ||
         wdStringUtils::IsEqual_NoCase(szExtension, "rle");
}

bool wdBmpFileFormat::CanWriteFileType(const char* szExtension) const
{
  return CanReadFileType(szExtension);
}



WD_STATICLINK_FILE(Texture, Texture_Image_Formats_BmpFileFormat);
