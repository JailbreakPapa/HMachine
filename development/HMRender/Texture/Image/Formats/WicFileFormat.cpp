#include <Texture/TexturePCH.h>

#include <Foundation/Basics/Platform/Win/HResultUtils.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/IO/Stream.h>
#include <Texture/Image/Formats/ImageFormatMappings.h>
#include <Texture/Image/Formats/WicFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>

#if WD_ENABLED(WD_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/IO/StreamUtils.h>
#  include <Foundation/Profiling/Profiling.h>
#  include <Texture/DirectXTex/DirectXTex.h>

using namespace DirectX;

WD_DEFINE_AS_POD_TYPE(DirectX::Image); // Allow for storing this struct in wd containers

wdWicFileFormat g_wicFormat;

namespace
{
  /// \brief Try to init COM, return true if we are the first(!) to successfully do so
  bool InitializeCOM()
  {
    HRESULT result = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (result == S_OK)
    {
      // We were the first one - deinit on shutdown
      return true;
    }
    else if (SUCCEEDED(result))
    {
      // We were not the first one, but we still succeeded, so we deinit COM right away.
      // Otherwise we might be the last one to call CoUninitialize(), but that is supposed to be the one who called
      // CoInitialize[Ex]() first.
      CoUninitialize();
    }

    // We won't call CoUninitialize() on shutdown as either we were not the first one to init COM successfully (and uninitialized it right away),
    // or our call to CoInitializeEx() didn't succeed, because it was already called with another concurrency model specifier.
    return false;
  }
} // namespace


wdWicFileFormat::wdWicFileFormat() = default;

wdWicFileFormat::~wdWicFileFormat()
{
  if (m_bCoUninitOnShutdown)
  {
    // We were the first one to successfully initialize COM, so we are the one who needs to shut it down.
    CoUninitialize();
  }
}

wdResult wdWicFileFormat::ReadFileData(wdStreamReader& stream, wdDynamicArray<wdUInt8>& storage) const
{
  if (m_bTryCoInit)
  {
    m_bCoUninitOnShutdown = InitializeCOM();
    m_bTryCoInit = false;
  }

  wdStreamUtils::ReadAllAndAppend(stream, storage);

  if (storage.IsEmpty())
  {
    wdLog::Error("Failure to retrieve image data.");
    return WD_FAILURE;
  }

  return WD_SUCCESS;
}

static void SetHeader(wdImageHeader& ref_header, wdImageFormat::Enum imageFormat, const TexMetadata& metadata)
{
  ref_header.SetImageFormat(imageFormat);

  ref_header.SetWidth(wdUInt32(metadata.width));
  ref_header.SetHeight(wdUInt32(metadata.height));
  ref_header.SetDepth(wdUInt32(metadata.depth));

  ref_header.SetNumMipLevels(1);
  ref_header.SetNumArrayIndices(wdUInt32(metadata.IsCubemap() ? (metadata.arraySize / 6) : metadata.arraySize));
  ref_header.SetNumFaces(metadata.IsCubemap() ? 6 : 1);
}

wdResult wdWicFileFormat::ReadImageHeader(wdStreamReader& inout_stream, wdImageHeader& ref_header, const char* szFileExtension) const
{
  WD_PROFILE_SCOPE("wdWicFileFormat::ReadImageHeader");

  wdDynamicArray<wdUInt8> storage;
  WD_SUCCEED_OR_RETURN(ReadFileData(inout_stream, storage));

  TexMetadata metadata;
  ScratchImage scratchImage;
  WIC_FLAGS wicFlags = WIC_FLAGS_ALL_FRAMES | WIC_FLAGS_IGNORE_SRGB /* just treat PNG, JPG etc as non-sRGB, we determine this through our 'Usage' later */;

  HRESULT loadResult = GetMetadataFromWICMemory(storage.GetData(), storage.GetCount(), wicFlags, metadata);
  if (FAILED(loadResult))
  {
    wdLog::Error("Failure to load image metadata. HRESULT:{}", wdArgErrorCode(loadResult));
    return WD_FAILURE;
  }

  wdImageFormat::Enum imageFormat = wdImageFormatMappings::FromDxgiFormat(metadata.format);

  if (imageFormat == wdImageFormat::UNKNOWN)
  {
    wdLog::Warning("Unable to use image format from '{}' file - trying conversion.", szFileExtension);
    wicFlags |= WIC_FLAGS_FORCE_RGB;
    GetMetadataFromWICMemory(storage.GetData(), storage.GetCount(), wicFlags, metadata);
    imageFormat = wdImageFormatMappings::FromDxgiFormat(metadata.format);
  }

  if (imageFormat == wdImageFormat::UNKNOWN)
  {
    wdLog::Error("Unable to use image format from '{}' file.", szFileExtension);
    return WD_FAILURE;
  }

  SetHeader(ref_header, imageFormat, metadata);

  return WD_SUCCESS;
}

wdResult wdWicFileFormat::ReadImage(wdStreamReader& inout_stream, wdImage& ref_image, const char* szFileExtension) const
{
  WD_PROFILE_SCOPE("wdWicFileFormat::ReadImage");

  wdDynamicArray<wdUInt8> storage;
  WD_SUCCEED_OR_RETURN(ReadFileData(inout_stream, storage));

  TexMetadata metadata;
  ScratchImage scratchImage;
  WIC_FLAGS wicFlags = WIC_FLAGS_ALL_FRAMES | WIC_FLAGS_IGNORE_SRGB /* just treat PNG, JPG etc as non-sRGB, we determine this through our 'Usage' later */;

  // Read WIC data from local storage
  HRESULT loadResult = LoadFromWICMemory(storage.GetData(), storage.GetCount(), wicFlags, nullptr, scratchImage);
  if (FAILED(loadResult))
  {
    wdLog::Error("Failure to load image data. HRESULT:{}", wdArgErrorCode(loadResult));
    return WD_FAILURE;
  }

  // Determine image format, re-reading image data if necessary
  metadata = scratchImage.GetMetadata();

  wdImageFormat::Enum imageFormat = wdImageFormatMappings::FromDxgiFormat(metadata.format);

  if (imageFormat == wdImageFormat::UNKNOWN)
  {
    wdLog::Warning("Unable to use image format from '{}' file - trying conversion.", szFileExtension);
    wicFlags |= WIC_FLAGS_FORCE_RGB;
    LoadFromWICMemory(storage.GetData(), storage.GetCount(), wicFlags, nullptr, scratchImage);
    imageFormat = wdImageFormatMappings::FromDxgiFormat(metadata.format);
  }

  if (imageFormat == wdImageFormat::UNKNOWN)
  {
    wdLog::Error("Unable to use image format from '{}' file.", szFileExtension);
    return WD_FAILURE;
  }

  // Prepare destination image header and allocate storage
  wdImageHeader imageHeader;
  SetHeader(imageHeader, imageFormat, metadata);

  ref_image.ResetAndAlloc(imageHeader);

  // Read image data into destination image
  wdUInt64 destRowPitch = imageHeader.GetRowPitch();
  wdUInt32 itemIdx = 0;
  for (wdUInt32 arrayIdx = 0; arrayIdx < imageHeader.GetNumArrayIndices(); ++arrayIdx)
  {
    for (wdUInt32 faceIdx = 0; faceIdx < imageHeader.GetNumFaces(); ++faceIdx, ++itemIdx)
    {
      for (wdUInt32 sliceIdx = 0; sliceIdx < imageHeader.GetDepth(); ++sliceIdx)
      {
        const Image* sourceImage = scratchImage.GetImage(0, itemIdx, sliceIdx);
        wdUInt8* destPixels = ref_image.GetPixelPointer<wdUInt8>(0, faceIdx, arrayIdx, 0, 0, sliceIdx);

        if (sourceImage && destPixels && sourceImage->pixels)
        {
          if (destRowPitch == sourceImage->rowPitch)
          {
            // Fast path: Just copy the entire thing
            wdMemoryUtils::Copy(destPixels, sourceImage->pixels, static_cast<size_t>(imageHeader.GetHeight() * destRowPitch));
          }
          else
          {
            // Row pitches don't match - copy row by row
            wdUInt64 bytesPerRow = wdMath::Min(destRowPitch, wdUInt64(sourceImage->rowPitch));
            const uint8_t* sourcePixels = sourceImage->pixels;
            for (wdUInt32 rowIdx = 0; rowIdx < imageHeader.GetHeight(); ++rowIdx)
            {
              wdMemoryUtils::Copy(destPixels, sourcePixels, static_cast<size_t>(bytesPerRow));

              destPixels += destRowPitch;
              sourcePixels += sourceImage->rowPitch;
            }
          }
        }
      }
    }
  }

  return WD_SUCCESS;
}

wdResult wdWicFileFormat::WriteImage(wdStreamWriter& inout_stream, const wdImageView& image, const char* szFileExtension) const
{
  if (m_bTryCoInit)
  {
    m_bCoUninitOnShutdown = InitializeCOM();
    m_bTryCoInit = false;
  }

  using namespace DirectX;

  // Convert into suitable output format
  wdImageFormat::Enum compatibleFormats[] = {
    wdImageFormat::R8G8B8A8_UNORM,
    wdImageFormat::R8G8B8A8_UNORM_SRGB,
    wdImageFormat::R8_UNORM,
    wdImageFormat::R16G16B16A16_UNORM,
    wdImageFormat::R16_UNORM,
    wdImageFormat::R32G32B32A32_FLOAT,
    wdImageFormat::R32G32B32_FLOAT,
  };

  // Find a compatible format closest to the one the image currently has
  wdImageFormat::Enum format = wdImageConversion::FindClosestCompatibleFormat(image.GetImageFormat(), compatibleFormats);

  if (format == wdImageFormat::UNKNOWN)
  {
    wdLog::Error("No conversion from format '{0}' to a format suitable for '{}' files known.", wdImageFormat::GetName(image.GetImageFormat()), szFileExtension);
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

  // Store wdImage data in DirectXTex images
  wdDynamicArray<Image> outputImages;
  DXGI_FORMAT imageFormat = DXGI_FORMAT(wdImageFormatMappings::ToDxgiFormat(image.GetImageFormat()));
  for (wdUInt32 arrayIdx = 0; arrayIdx < image.GetNumArrayIndices(); ++arrayIdx)
  {
    for (wdUInt32 faceIdx = 0; faceIdx < image.GetNumFaces(); ++faceIdx)
    {
      for (wdUInt32 sliceIdx = 0; sliceIdx < image.GetDepth(); ++sliceIdx)
      {
        Image& currentImage = outputImages.ExpandAndGetRef();
        currentImage.width = image.GetWidth();
        currentImage.height = image.GetHeight();
        currentImage.format = imageFormat;
        currentImage.rowPitch = static_cast<size_t>(image.GetRowPitch());
        currentImage.slicePitch = static_cast<size_t>(image.GetDepthPitch());
        currentImage.pixels = const_cast<uint8_t*>(image.GetPixelPointer<uint8_t>(0, faceIdx, arrayIdx, 0, 0, sliceIdx));
      }
    }
  }

  if (!outputImages.IsEmpty())
  {
    // Store images in output blob
    Blob targetBlob;
    WIC_FLAGS flags = WIC_FLAGS_NONE;
    HRESULT res = SaveToWICMemory(outputImages.GetData(), outputImages.GetCount(), flags, GetWICCodec(WIC_CODEC_TIFF), targetBlob);
    if (FAILED(res))
    {
      wdLog::Error("Failed to save image data to local memory blob - result: {}!", wdHRESULTtoString(res));
      return WD_FAILURE;
    }

    // Push blob into output stream
    if (inout_stream.WriteBytes(targetBlob.GetBufferPointer(), targetBlob.GetBufferSize()) != WD_SUCCESS)
    {
      wdLog::Error("Failed to write image data!");
      return WD_FAILURE;
    }
  }

  return WD_SUCCESS;
}

bool wdWicFileFormat::CanReadFileType(const char* szExtension) const
{
  return wdStringUtils::IsEqual_NoCase(szExtension, "png") || wdStringUtils::IsEqual_NoCase(szExtension, "jpg") ||
         wdStringUtils::IsEqual_NoCase(szExtension, "jpeg") ||
         // wdStringUtils::IsEqual_NoCase(szExtension, "hdr") ||
         wdStringUtils::IsEqual_NoCase(szExtension, "tif") || wdStringUtils::IsEqual_NoCase(szExtension, "tiff");
}

bool wdWicFileFormat::CanWriteFileType(const char* szExtension) const
{
  // png, jpg and jpeg are handled by STB (wdStbImageFileFormats)
  return wdStringUtils::IsEqual_NoCase(szExtension, "tif") || wdStringUtils::IsEqual_NoCase(szExtension, "tiff");
}

#endif

WD_STATICLINK_FILE(Texture, Texture_Image_Formats_WicFileFormat);
