#include <Texture/TexturePCH.h>

#include <Texture/Image/Formats/TgaFileFormat.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageConversion.h>


wdTgaFileFormat g_TgaFormat;

struct TgaImageDescriptor
{
  wdUInt8 m_iAlphaBits : 4;
  wdUInt8 m_bFlipH : 1;
  wdUInt8 m_bFlipV : 1;
  wdUInt8 m_Ignored : 2;
};

// see Wikipedia for details:
// http://de.wikipedia.org/wiki/Targa_Image_File
struct TgaHeader
{
  wdInt8 m_iImageIDLength;
  wdInt8 m_Ignored1;
  wdInt8 m_ImageType;
  wdInt8 m_Ignored2[9];
  wdInt16 m_iImageWidth;
  wdInt16 m_iImageHeight;
  wdInt8 m_iBitsPerPixel;
  TgaImageDescriptor m_ImageDescriptor;
};

WD_CHECK_AT_COMPILETIME(sizeof(TgaHeader) == 18);


static inline wdColorLinearUB GetPixelColor(const wdImageView& image, wdUInt32 x, wdUInt32 y, const wdUInt32 uiHeight)
{
  wdColorLinearUB c(255, 255, 255, 255);

  const wdUInt8* pPixel = image.GetPixelPointer<wdUInt8>(0, 0, 0, x, uiHeight - y - 1, 0);

  switch (image.GetImageFormat())
  {
    case wdImageFormat::R8G8B8A8_UNORM:
      c.r = pPixel[0];
      c.g = pPixel[1];
      c.b = pPixel[2];
      c.a = pPixel[3];
      break;
    case wdImageFormat::B8G8R8A8_UNORM:
      c.a = pPixel[3];
      // fall through
    case wdImageFormat::B8G8R8_UNORM:
    case wdImageFormat::B8G8R8X8_UNORM:
      c.r = pPixel[2];
      c.g = pPixel[1];
      c.b = pPixel[0];
      break;

    default:
      WD_ASSERT_NOT_IMPLEMENTED;
  }

  return c;
}


wdResult wdTgaFileFormat::WriteImage(wdStreamWriter& inout_stream, const wdImageView& image, const char* szFileExtension) const
{
  // Technically almost arbitrary formats are supported, but we only use the common ones.
  wdImageFormat::Enum compatibleFormats[] = {
    wdImageFormat::R8G8B8A8_UNORM,
    wdImageFormat::B8G8R8A8_UNORM,
    wdImageFormat::B8G8R8X8_UNORM,
    wdImageFormat::B8G8R8_UNORM,
  };

  // Find a compatible format closest to the one the image currently has
  wdImageFormat::Enum format = wdImageConversion::FindClosestCompatibleFormat(image.GetImageFormat(), compatibleFormats);

  if (format == wdImageFormat::UNKNOWN)
  {
    wdLog::Error("No conversion from format '{0}' to a format suitable for TGA files known.", wdImageFormat::GetName(image.GetImageFormat()));
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

  const bool bCompress = true;

  // Write the header
  {
    wdUInt8 uiHeader[18];
    wdMemoryUtils::ZeroFill(uiHeader, 18);

    if (!bCompress)
    {
      // uncompressed TGA
      uiHeader[2] = 2;
    }
    else
    {
      // compressed TGA
      uiHeader[2] = 10;
    }

    uiHeader[13] = static_cast<wdUInt8>(image.GetWidth(0) / 256);
    uiHeader[15] = static_cast<wdUInt8>(image.GetHeight(0) / 256);
    uiHeader[12] = static_cast<wdUInt8>(image.GetWidth(0) % 256);
    uiHeader[14] = static_cast<wdUInt8>(image.GetHeight(0) % 256);
    uiHeader[16] = static_cast<wdUInt8>(wdImageFormat::GetBitsPerPixel(image.GetImageFormat()));

    inout_stream.WriteBytes(uiHeader, 18).IgnoreResult();
  }

  const bool bAlpha = image.GetImageFormat() != wdImageFormat::B8G8R8_UNORM;

  const wdUInt32 uiWidth = image.GetWidth(0);
  const wdUInt32 uiHeight = image.GetHeight(0);

  if (!bCompress)
  {
    // Write image uncompressed

    for (wdUInt32 y = 0; y < uiWidth; ++y)
    {
      for (wdUInt32 x = 0; x < uiHeight; ++x)
      {
        const wdColorLinearUB c = GetPixelColor(image, x, y, uiHeight);

        inout_stream << c.b;
        inout_stream << c.g;
        inout_stream << c.r;

        if (bAlpha)
          inout_stream << c.a;
      }
    }
  }
  else
  {
    // write image RLE compressed

    wdInt32 iRLE = 0;

    wdColorLinearUB pc = {};
    wdStaticArray<wdColorLinearUB, 129> unequal;
    wdInt32 iEqual = 0;

    for (wdUInt32 y = 0; y < uiHeight; ++y)
    {
      for (wdUInt32 x = 0; x < uiWidth; ++x)
      {
        const wdColorLinearUB c = GetPixelColor(image, x, y, uiHeight);

        if (iRLE == 0) // no comparison possible yet
        {
          pc = c;
          iRLE = 1;
          unequal.PushBack(c);
        }
        else if (iRLE == 1) // has one value gathered for comparison
        {
          if (c == pc)
          {
            iRLE = 2;   // two values were equal
            iEqual = 2; // go into equal-mode
          }
          else
          {
            iRLE = 3; // two values were unequal
            pc = c;   // go into unequal-mode
            unequal.PushBack(c);
          }
        }
        else if (iRLE == 2) // equal values
        {
          if ((c == pc) && (iEqual < 128))
            ++iEqual;
          else
          {
            wdUInt8 uiRepeat = static_cast<wdUInt8>(iEqual + 127);

            inout_stream << uiRepeat;
            inout_stream << pc.b;
            inout_stream << pc.g;
            inout_stream << pc.r;

            if (bAlpha)
              inout_stream << pc.a;

            pc = c;
            iRLE = 1;
            unequal.Clear();
            unequal.PushBack(c);
          }
        }
        else if (iRLE == 3)
        {
          if ((c != pc) && (unequal.GetCount() < 128))
          {
            unequal.PushBack(c);
            pc = c;
          }
          else
          {
            wdUInt8 uiRepeat = (unsigned char)(unequal.GetCount()) - 1;
            inout_stream << uiRepeat;

            for (wdUInt32 i = 0; i < unequal.GetCount(); ++i)
            {
              inout_stream << unequal[i].b;
              inout_stream << unequal[i].g;
              inout_stream << unequal[i].r;

              if (bAlpha)
                inout_stream << unequal[i].a;
            }

            pc = c;
            iRLE = 1;
            unequal.Clear();
            unequal.PushBack(c);
          }
        }
      }
    }


    if (iRLE == 1) // has one value gathered for comparison
    {
      wdUInt8 uiRepeat = 0;

      inout_stream << uiRepeat;
      inout_stream << pc.b;
      inout_stream << pc.g;
      inout_stream << pc.r;

      if (bAlpha)
        inout_stream << pc.a;
    }
    else if (iRLE == 2) // equal values
    {
      wdUInt8 uiRepeat = static_cast<wdUInt8>(iEqual + 127);

      inout_stream << uiRepeat;
      inout_stream << pc.b;
      inout_stream << pc.g;
      inout_stream << pc.r;

      if (bAlpha)
        inout_stream << pc.a;
    }
    else if (iRLE == 3)
    {
      wdUInt8 uiRepeat = (wdUInt8)(unequal.GetCount()) - 1;
      inout_stream << uiRepeat;

      for (wdUInt32 i = 0; i < unequal.GetCount(); ++i)
      {
        inout_stream << unequal[i].b;
        inout_stream << unequal[i].g;
        inout_stream << unequal[i].r;

        if (bAlpha)
          inout_stream << unequal[i].a;
      }
    }
  }

  return WD_SUCCESS;
}

static wdResult ReadImageHeaderImpl(wdStreamReader& inout_stream, wdImageHeader& ref_header, const char* szFileExtension, TgaHeader& ref_tgaHeader)
{
  inout_stream >> ref_tgaHeader.m_iImageIDLength;
  inout_stream >> ref_tgaHeader.m_Ignored1;
  inout_stream >> ref_tgaHeader.m_ImageType;
  inout_stream.ReadBytes(&ref_tgaHeader.m_Ignored2, 9);
  inout_stream >> ref_tgaHeader.m_iImageWidth;
  inout_stream >> ref_tgaHeader.m_iImageHeight;
  inout_stream >> ref_tgaHeader.m_iBitsPerPixel;
  inout_stream >> reinterpret_cast<wdUInt8&>(ref_tgaHeader.m_ImageDescriptor);

  // ignore optional data
  inout_stream.SkipBytes(ref_tgaHeader.m_iImageIDLength);

  const wdUInt32 uiBytesPerPixel = ref_tgaHeader.m_iBitsPerPixel / 8;

  // check whether width, height an BitsPerPixel are valid
  if ((ref_tgaHeader.m_iImageWidth <= 0) || (ref_tgaHeader.m_iImageHeight <= 0) || ((uiBytesPerPixel != 1) && (uiBytesPerPixel != 3) && (uiBytesPerPixel != 4)) || (ref_tgaHeader.m_ImageType != 2 && ref_tgaHeader.m_ImageType != 3 && ref_tgaHeader.m_ImageType != 10 && ref_tgaHeader.m_ImageType != 11))
  {
    wdLog::Error("TGA has an invalid header: Width = {0}, Height = {1}, BPP = {2}, ImageType = {3}", ref_tgaHeader.m_iImageWidth, ref_tgaHeader.m_iImageHeight, ref_tgaHeader.m_iBitsPerPixel, ref_tgaHeader.m_ImageType);
    return WD_FAILURE;
  }

  // Set image data

  if (uiBytesPerPixel == 1)
    ref_header.SetImageFormat(wdImageFormat::R8_UNORM);
  else if (uiBytesPerPixel == 3)
    ref_header.SetImageFormat(wdImageFormat::B8G8R8_UNORM);
  else
    ref_header.SetImageFormat(wdImageFormat::B8G8R8A8_UNORM);

  ref_header.SetNumMipLevels(1);
  ref_header.SetNumArrayIndices(1);
  ref_header.SetNumFaces(1);

  ref_header.SetWidth(ref_tgaHeader.m_iImageWidth);
  ref_header.SetHeight(ref_tgaHeader.m_iImageHeight);
  ref_header.SetDepth(1);

  return WD_SUCCESS;
}

wdResult wdTgaFileFormat::ReadImageHeader(wdStreamReader& inout_stream, wdImageHeader& ref_header, const char* szFileExtension) const
{
  WD_PROFILE_SCOPE("wdTgaFileFormat::ReadImageHeader");

  TgaHeader tgaHeader;
  return ReadImageHeaderImpl(inout_stream, ref_header, szFileExtension, tgaHeader);
}

wdResult wdTgaFileFormat::ReadImage(wdStreamReader& inout_stream, wdImage& ref_image, const char* szFileExtension) const
{
  WD_PROFILE_SCOPE("wdTgaFileFormat::ReadImage");

  wdImageHeader imageHeader;
  TgaHeader tgaHeader;
  WD_SUCCEED_OR_RETURN(ReadImageHeaderImpl(inout_stream, imageHeader, szFileExtension, tgaHeader));

  const wdUInt32 uiBytesPerPixel = tgaHeader.m_iBitsPerPixel / 8;

  ref_image.ResetAndAlloc(imageHeader);

  if (tgaHeader.m_ImageType == 3)
  {
    // uncompressed greyscale

    const wdUInt32 uiBytesPerRow = uiBytesPerPixel * tgaHeader.m_iImageWidth;

    if (tgaHeader.m_ImageDescriptor.m_bFlipH)
    {
      // read each row (gets rid of the row pitch
      for (wdInt32 y = 0; y < tgaHeader.m_iImageHeight; ++y)
      {
        const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
        for (wdInt32 x = tgaHeader.m_iImageWidth - 1; x >= 0; --x)
        {
          inout_stream.ReadBytes(ref_image.GetPixelPointer<void>(0, 0, 0, x, row, 0), uiBytesPerPixel);
        }
      }
    }
    else
    {
      // read each row (gets rid of the row pitch
      for (wdInt32 y = 0; y < tgaHeader.m_iImageHeight; ++y)
      {
        const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
        inout_stream.ReadBytes(ref_image.GetPixelPointer<void>(0, 0, 0, 0, row, 0), uiBytesPerRow);
      }
    }
  }
  else if (tgaHeader.m_ImageType == 2)
  {
    // uncompressed

    const wdUInt32 uiBytesPerRow = uiBytesPerPixel * tgaHeader.m_iImageWidth;

    if (tgaHeader.m_ImageDescriptor.m_bFlipH)
    {
      // read each row (gets rid of the row pitch
      for (wdInt32 y = 0; y < tgaHeader.m_iImageHeight; ++y)
      {
        const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
        for (wdInt32 x = tgaHeader.m_iImageWidth - 1; x >= 0; --x)
        {
          inout_stream.ReadBytes(ref_image.GetPixelPointer<void>(0, 0, 0, x, row, 0), uiBytesPerPixel);
        }
      }
    }
    else
    {
      // read each row (gets rid of the row pitch
      for (wdInt32 y = 0; y < tgaHeader.m_iImageHeight; ++y)
      {
        const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
        inout_stream.ReadBytes(ref_image.GetPixelPointer<void>(0, 0, 0, 0, row, 0), uiBytesPerRow);
      }
    }
  }
  else
  {
    // compressed

    wdInt32 iCurrentPixel = 0;
    const int iPixelCount = tgaHeader.m_iImageWidth * tgaHeader.m_iImageHeight;

    do
    {
      wdUInt8 uiChunkHeader = 0;

      inout_stream >> uiChunkHeader;

      const wdInt32 numToRead = (uiChunkHeader & 127) + 1;

      if (iCurrentPixel + numToRead > iPixelCount)
      {
        wdLog::Error("TGA contents are invalid");
        return WD_FAILURE;
      }

      if (uiChunkHeader < 128)
      {
        // If the header is < 128, it means it is the number of RAW color packets minus 1
        // that follow the header
        // add 1 to get number of following color values

        // Read RAW color values
        for (wdInt32 i = 0; i < numToRead; ++i)
        {
          const wdInt32 x = iCurrentPixel % tgaHeader.m_iImageWidth;
          const wdInt32 y = iCurrentPixel / tgaHeader.m_iImageWidth;

          const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
          const auto col = tgaHeader.m_ImageDescriptor.m_bFlipH ? tgaHeader.m_iImageWidth - x - 1 : x;
          inout_stream.ReadBytes(ref_image.GetPixelPointer<void>(0, 0, 0, col, row, 0), uiBytesPerPixel);

          ++iCurrentPixel;
        }
      }
      else // chunk header > 128 RLE data, next color repeated (chunk header - 127) times
      {
        wdUInt8 uiBuffer[4] = {255, 255, 255, 255};

        // read the current color
        inout_stream.ReadBytes(uiBuffer, uiBytesPerPixel);

        // if it is a 24-Bit TGA (3 channels), the fourth channel stays at 255 all the time, since the 4th value in ucBuffer is never overwritten

        // copy the color into the image data as many times as dictated
        for (wdInt32 i = 0; i < numToRead; ++i)
        {
          const wdInt32 x = iCurrentPixel % tgaHeader.m_iImageWidth;
          const wdInt32 y = iCurrentPixel / tgaHeader.m_iImageWidth;

          const auto row = tgaHeader.m_ImageDescriptor.m_bFlipV ? y : tgaHeader.m_iImageHeight - y - 1;
          const auto col = tgaHeader.m_ImageDescriptor.m_bFlipH ? tgaHeader.m_iImageWidth - x - 1 : x;
          wdUInt8* pPixel = ref_image.GetPixelPointer<wdUInt8>(0, 0, 0, col, row, 0);

          // BGR
          pPixel[0] = uiBuffer[0];

          if (uiBytesPerPixel > 1)
          {
            pPixel[1] = uiBuffer[1];
            pPixel[2] = uiBuffer[2];

            // Alpha
            if (uiBytesPerPixel == 4)
              pPixel[3] = uiBuffer[3];
          }

          ++iCurrentPixel;
        }
      }
    } while (iCurrentPixel < iPixelCount);
  }

  return WD_SUCCESS;
}

bool wdTgaFileFormat::CanReadFileType(const char* szExtension) const
{
  return wdStringUtils::IsEqual_NoCase(szExtension, "tga");
}

bool wdTgaFileFormat::CanWriteFileType(const char* szExtension) const
{
  return CanReadFileType(szExtension);
}



WD_STATICLINK_FILE(Texture, Texture_Image_Formats_TgaFileFormat);
