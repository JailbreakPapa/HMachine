#include <Texture/TexturePCH.h>

#if WD_DISABLED(WD_PLATFORM_WINDOWS_UWP)

#  include <Texture/Image/Formats/ExrFileFormat.h>
#  include <Texture/Image/Image.h>

#  include <Foundation/IO/MemoryStream.h>
#  include <Foundation/IO/StreamUtils.h>
#  include <Foundation/Profiling/Profiling.h>

#  include <tinyexr/tinyexr.h>

wdExrFileFormat g_ExrFileFormat;

wdResult ReadImageData(wdStreamReader& stream, wdDynamicArray<wdUInt8>& fileBuffer, wdImageHeader& header, EXRHeader& exrHeader, EXRImage& exrImage)
{
  // read the entire file to memory
  wdStreamUtils::ReadAllAndAppend(stream, fileBuffer);

  // read the EXR version
  EXRVersion exrVersion;

  if (ParseEXRVersionFromMemory(&exrVersion, fileBuffer.GetData(), fileBuffer.GetCount()) != 0)
  {
    wdLog::Error("Invalid EXR file: Cannot read version.");
    return WD_FAILURE;
  }

  if (exrVersion.multipart)
  {
    wdLog::Error("Invalid EXR file: Multi-part formats are not supported.");
    return WD_FAILURE;
  }

  // read the EXR header
  const char* err = nullptr;
  if (ParseEXRHeaderFromMemory(&exrHeader, &exrVersion, fileBuffer.GetData(), fileBuffer.GetCount(), &err) != 0)
  {
    wdLog::Error("Invalid EXR file: '{0}'", err);
    FreeEXRErrorMessage(err);
    return WD_FAILURE;
  }

  for (int c = 1; c < exrHeader.num_channels; ++c)
  {
    if (exrHeader.pixel_types[c - 1] != exrHeader.pixel_types[c])
    {
      wdLog::Error("Unsupported EXR file: all channels should have the same size.");
      break;
    }
  }

  if (LoadEXRImageFromMemory(&exrImage, &exrHeader, fileBuffer.GetData(), fileBuffer.GetCount(), &err) != 0)
  {
    wdLog::Error("Invalid EXR file: '{0}'", err);

    FreeEXRHeader(&exrHeader);
    FreeEXRErrorMessage(err);
    return WD_FAILURE;
  }

  wdImageFormat::Enum imageFormat = wdImageFormat::UNKNOWN;

  switch (exrHeader.num_channels)
  {
    case 1:
    {
      switch (exrHeader.pixel_types[0])
      {
        case TINYEXR_PIXELTYPE_FLOAT:
          imageFormat = wdImageFormat::R32_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_HALF:
          imageFormat = wdImageFormat::R16_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_UINT:
          imageFormat = wdImageFormat::R32_UINT;
          break;
      }

      break;
    }

    case 2:
    {
      switch (exrHeader.pixel_types[0])
      {
        case TINYEXR_PIXELTYPE_FLOAT:
          imageFormat = wdImageFormat::R32G32_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_HALF:
          imageFormat = wdImageFormat::R16G16_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_UINT:
          imageFormat = wdImageFormat::R32G32_UINT;
          break;
      }

      break;
    }

    case 3:
    {
      switch (exrHeader.pixel_types[0])
      {
        case TINYEXR_PIXELTYPE_FLOAT:
          imageFormat = wdImageFormat::R32G32B32_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_HALF:
          imageFormat = wdImageFormat::R16G16B16A16_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_UINT:
          imageFormat = wdImageFormat::R32G32B32_UINT;
          break;
      }

      break;
    }

    case 4:
    {
      switch (exrHeader.pixel_types[0])
      {
        case TINYEXR_PIXELTYPE_FLOAT:
          imageFormat = wdImageFormat::R32G32B32A32_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_HALF:
          imageFormat = wdImageFormat::R16G16B16A16_FLOAT;
          break;

        case TINYEXR_PIXELTYPE_UINT:
          wdImageFormat::R32G32B32A32_UINT;
          break;
      }

      break;
    }
  }

  if (imageFormat == wdImageFormat::UNKNOWN)
  {
    wdLog::Error("Unsupported EXR file: {}-channel files with format '{}' are unsupported.", exrHeader.num_channels, exrHeader.pixel_types[0]);
    return WD_FAILURE;
  }

  header.SetWidth(exrImage.width);
  header.SetHeight(exrImage.height);
  header.SetImageFormat(imageFormat);

  header.SetNumMipLevels(1);
  header.SetNumArrayIndices(1);
  header.SetNumFaces(1);
  header.SetDepth(1);

  return WD_SUCCESS;
}

wdResult wdExrFileFormat::ReadImageHeader(wdStreamReader& stream, wdImageHeader& header, const char* szFileExtension) const
{
  WD_PROFILE_SCOPE("wdExrFileFormat::ReadImageHeader");

  EXRHeader exrHeader;
  InitEXRHeader(&exrHeader);
  WD_SCOPE_EXIT(FreeEXRHeader(&exrHeader));

  EXRImage exrImage;
  InitEXRImage(&exrImage);
  WD_SCOPE_EXIT(FreeEXRImage(&exrImage));

  wdDynamicArray<wdUInt8> fileBuffer;
  return ReadImageData(stream, fileBuffer, header, exrHeader, exrImage);
}

static void CopyChannel(wdUInt8* pDst, const wdUInt8* pSrc, wdUInt32 uiNumElements, wdUInt32 uiElementSize, wdUInt32 uiDstStride)
{
  if (uiDstStride == uiElementSize)
  {
    // fast path to copy everything in one operation
    // this only happens for single-channel formats
    wdMemoryUtils::RawByteCopy(pDst, pSrc, uiNumElements * uiElementSize);
  }
  else
  {
    for (wdUInt32 i = 0; i < uiNumElements; ++i)
    {
      wdMemoryUtils::RawByteCopy(pDst, pSrc, uiElementSize);

      pSrc = wdMemoryUtils::AddByteOffset(pSrc, uiElementSize);
      pDst = wdMemoryUtils::AddByteOffset(pDst, uiDstStride);
    }
  }
}

wdResult wdExrFileFormat::ReadImage(wdStreamReader& stream, wdImage& image, const char* szFileExtension) const
{
  WD_PROFILE_SCOPE("wdExrFileFormat::ReadImage");

  EXRHeader exrHeader;
  InitEXRHeader(&exrHeader);
  WD_SCOPE_EXIT(FreeEXRHeader(&exrHeader));

  EXRImage exrImage;
  InitEXRImage(&exrImage);
  WD_SCOPE_EXIT(FreeEXRImage(&exrImage));

  wdImageHeader header;
  wdDynamicArray<wdUInt8> fileBuffer;

  WD_SUCCEED_OR_RETURN(ReadImageData(stream, fileBuffer, header, exrHeader, exrImage));

  image.ResetAndAlloc(header);

  const wdUInt32 uiPixelCount = header.GetWidth() * header.GetHeight();
  const wdUInt32 uiNumDstChannels = wdImageFormat::GetNumChannels(header.GetImageFormat());
  const wdUInt32 uiNumSrcChannels = exrHeader.num_channels;

  wdUInt32 uiSrcStride = 0;
  switch (exrHeader.pixel_types[0])
  {
    case TINYEXR_PIXELTYPE_FLOAT:
      uiSrcStride = sizeof(float);
      break;

    case TINYEXR_PIXELTYPE_HALF:
      uiSrcStride = sizeof(float) / 2;
      break;

    case TINYEXR_PIXELTYPE_UINT:
      uiSrcStride = sizeof(wdUInt32);
      break;

      WD_DEFAULT_CASE_NOT_IMPLEMENTED;
  }


  // src and dst element size is always identical, we only copy from float->float, half->half or uint->uint
  // however data is interleaved in dst, but not interleaved in src

  const wdUInt32 uiDstStride = uiSrcStride * uiNumDstChannels;
  wdUInt8* pDstBytes = image.GetBlobPtr<wdUInt8>().GetPtr();

  if (uiNumDstChannels > uiNumSrcChannels)
  {
    // if we have more dst channels, than in the input data, fill everything with white
    wdMemoryUtils::PatternFill(pDstBytes, 0xFF, uiDstStride * uiPixelCount);
  }

  wdUInt32 c = 0;

  if (uiNumSrcChannels >= 4)
  {
    const wdUInt8* pSrcBytes = exrImage.images[c++];

    if (uiNumDstChannels >= 4)
    {
      // copy to alpha
      CopyChannel(pDstBytes + 3 * uiSrcStride, pSrcBytes, uiPixelCount, uiSrcStride, uiDstStride);
    }
  }

  if (uiNumSrcChannels >= 3)
  {
    const wdUInt8* pSrcBytes = exrImage.images[c++];

    if (uiNumDstChannels >= 3)
    {
      // copy to blue
      CopyChannel(pDstBytes + 2 * uiSrcStride, pSrcBytes, uiPixelCount, uiSrcStride, uiDstStride);
    }
  }

  if (uiNumSrcChannels >= 2)
  {
    const wdUInt8* pSrcBytes = exrImage.images[c++];

    if (uiNumDstChannels >= 2)
    {
      // copy to green
      CopyChannel(pDstBytes + uiSrcStride, pSrcBytes, uiPixelCount, uiSrcStride, uiDstStride);
    }
  }

  if (uiNumSrcChannels >= 1)
  {
    const wdUInt8* pSrcBytes = exrImage.images[c++];

    if (uiNumDstChannels >= 1)
    {
      // copy to red
      CopyChannel(pDstBytes, pSrcBytes, uiPixelCount, uiSrcStride, uiDstStride);
    }
  }

  return WD_SUCCESS;
}

wdResult wdExrFileFormat::WriteImage(wdStreamWriter& stream, const wdImageView& image, const char* szFileExtension) const
{
  WD_ASSERT_NOT_IMPLEMENTED;
  return WD_FAILURE;
}

bool wdExrFileFormat::CanReadFileType(const char* szExtension) const
{
  return wdStringUtils::IsEqual_NoCase(szExtension, "exr");
}

bool wdExrFileFormat::CanWriteFileType(const char* szExtension) const
{
  return false;
}

#endif

WD_STATICLINK_FILE(Texture, Texture_Image_Formats_ExrFileFormat);
