#include <Texture/TexturePCH.h>

#include <Texture/Image/Formats/StbImageFileFormats.h>
#include <Texture/Image/Image.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/StreamUtils.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageConversion.h>
#include <stb_image/stb_image.h>
#include <stb_image/stb_image_write.h>


wdStbImageFileFormats g_StbImageFormats;

// stb_image callbacks would be better than loading the entire file into memory.
// However, it turned out that it does not map well to wdStreamReader

// namespace
//{
//  // fill 'data' with 'size' bytes.  return number of bytes actually read
//  int read(void *user, char *data, int size)
//  {
//    wdStreamReader* pStream = static_cast<wdStreamReader*>(user);
//    return static_cast<int>(pStream->ReadBytes(data, size));
//  }
//  // skip the next 'n' bytes, or 'unget' the last -n bytes if negative
//  void skip(void *user, int n)
//  {
//    wdStreamReader* pStream = static_cast<wdStreamReader*>(user);
//    if(n > 0)
//      pStream->SkipBytes(n);
//    else
//      // ?? We cannot reverse skip.
//
//  }
//  // returns nonzero if we are at end of file/data
//  int eof(void *user)
//  {
//    wdStreamReader* pStream = static_cast<wdStreamReader*>(user);
//    // ?
//  }
//}

namespace
{

  void write_func(void* pContext, void* pData, int iSize)
  {
    wdStreamWriter* writer = static_cast<wdStreamWriter*>(pContext);
    writer->WriteBytes(pData, iSize).IgnoreResult();
  }

  void* ReadImageData(wdStreamReader& inout_stream, wdDynamicArray<wdUInt8>& ref_fileBuffer, wdImageHeader& ref_imageHeader, bool& ref_bIsHDR)
  {
    wdStreamUtils::ReadAllAndAppend(inout_stream, ref_fileBuffer);

    int width, height, numComp;

    ref_bIsHDR = !!stbi_is_hdr_from_memory(ref_fileBuffer.GetData(), ref_fileBuffer.GetCount());

    void* sourceImageData = nullptr;
    if (ref_bIsHDR)
    {
      sourceImageData = stbi_loadf_from_memory(ref_fileBuffer.GetData(), ref_fileBuffer.GetCount(), &width, &height, &numComp, 0);
    }
    else
    {
      sourceImageData = stbi_load_from_memory(ref_fileBuffer.GetData(), ref_fileBuffer.GetCount(), &width, &height, &numComp, 0);
    }
    if (!sourceImageData)
    {
      wdLog::Error("stb_image failed to load: {0}", stbi_failure_reason());
      return nullptr;
    }
    ref_fileBuffer.Clear();

    wdImageFormat::Enum format = wdImageFormat::UNKNOWN;
    switch (numComp)
    {
      case 1:
        format = (ref_bIsHDR) ? wdImageFormat::R32_FLOAT : wdImageFormat::R8_UNORM;
        break;
      case 2:
        format = (ref_bIsHDR) ? wdImageFormat::R32G32_FLOAT : wdImageFormat::R8G8_UNORM;
        break;
      case 3:
        format = (ref_bIsHDR) ? wdImageFormat::R32G32B32_FLOAT : wdImageFormat::R8G8B8_UNORM;
        break;
      case 4:
        format = (ref_bIsHDR) ? wdImageFormat::R32G32B32A32_FLOAT : wdImageFormat::R8G8B8A8_UNORM;
        break;
    }

    // Set properties and allocate.
    ref_imageHeader.SetImageFormat(format);
    ref_imageHeader.SetNumMipLevels(1);
    ref_imageHeader.SetNumArrayIndices(1);
    ref_imageHeader.SetNumFaces(1);

    ref_imageHeader.SetWidth(width);
    ref_imageHeader.SetHeight(height);
    ref_imageHeader.SetDepth(1);

    return sourceImageData;
  }

} // namespace

wdResult wdStbImageFileFormats::ReadImageHeader(wdStreamReader& inout_stream, wdImageHeader& ref_header, const char* szFileExtension) const
{
  WD_PROFILE_SCOPE("wdStbImageFileFormats::ReadImageHeader");

  bool isHDR = false;
  wdDynamicArray<wdUInt8> fileBuffer;
  void* sourceImageData = ReadImageData(inout_stream, fileBuffer, ref_header, isHDR);

  if (sourceImageData == nullptr)
    return WD_FAILURE;

  stbi_image_free(sourceImageData);
  return WD_SUCCESS;
}

wdResult wdStbImageFileFormats::ReadImage(wdStreamReader& inout_stream, wdImage& ref_image, const char* szFileExtension) const
{
  WD_PROFILE_SCOPE("wdStbImageFileFormats::ReadImage");

  bool isHDR = false;
  wdDynamicArray<wdUInt8> fileBuffer;
  wdImageHeader imageHeader;
  void* sourceImageData = ReadImageData(inout_stream, fileBuffer, imageHeader, isHDR);

  if (sourceImageData == nullptr)
    return WD_FAILURE;

  ref_image.ResetAndAlloc(imageHeader);

  const size_t numComp = wdImageFormat::GetNumChannels(imageHeader.GetImageFormat());

  const size_t elementsToCopy = static_cast<size_t>(imageHeader.GetWidth()) * static_cast<size_t>(imageHeader.GetHeight()) * numComp;

  // Set pixels. Different strategies depending on component count.
  if (isHDR)
  {
    float* targetImageData = ref_image.GetBlobPtr<float>().GetPtr();
    wdMemoryUtils::Copy(targetImageData, (const float*)sourceImageData, elementsToCopy);
  }
  else
  {
    wdUInt8* targetImageData = ref_image.GetBlobPtr<wdUInt8>().GetPtr();
    wdMemoryUtils::Copy(targetImageData, (const wdUInt8*)sourceImageData, elementsToCopy);
  }

  stbi_image_free((void*)sourceImageData);
  return WD_SUCCESS;
}

wdResult wdStbImageFileFormats::WriteImage(wdStreamWriter& inout_stream, const wdImageView& image, const char* szFileExtension) const
{
  wdImageFormat::Enum compatibleFormats[] = {wdImageFormat::R8_UNORM, wdImageFormat::R8G8B8_UNORM, wdImageFormat::R8G8B8A8_UNORM};

  // Find a compatible format closest to the one the image currently has
  wdImageFormat::Enum format = wdImageConversion::FindClosestCompatibleFormat(image.GetImageFormat(), compatibleFormats);

  if (format == wdImageFormat::UNKNOWN)
  {
    wdLog::Error("No conversion from format '{0}' to a format suitable for PNG files known.", wdImageFormat::GetName(image.GetImageFormat()));
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

  if (wdStringUtils::IsEqual_NoCase(szFileExtension, "png"))
  {
    if (stbi_write_png_to_func(write_func, &inout_stream, image.GetWidth(), image.GetHeight(), wdImageFormat::GetNumChannels(image.GetImageFormat()), image.GetByteBlobPtr().GetPtr(), 0))
    {
      return WD_SUCCESS;
    }
  }

  if (wdStringUtils::IsEqual_NoCase(szFileExtension, "jpg") || wdStringUtils::IsEqual_NoCase(szFileExtension, "jpeg"))
  {
    if (stbi_write_jpg_to_func(write_func, &inout_stream, image.GetWidth(), image.GetHeight(), wdImageFormat::GetNumChannels(image.GetImageFormat()), image.GetByteBlobPtr().GetPtr(), 95))
    {
      return WD_SUCCESS;
    }
  }

  return WD_FAILURE;
}

bool wdStbImageFileFormats::CanReadFileType(const char* szExtension) const
{
  if (wdStringUtils::IsEqual_NoCase(szExtension, "hdr"))
    return true;

#if WD_DISABLED(WD_PLATFORM_WINDOWS_DESKTOP)

  // on Windows Desktop, we prefer to use WIC (wdWicFileFormat)
  if (wdStringUtils::IsEqual_NoCase(szExtension, "png") || wdStringUtils::IsEqual_NoCase(szExtension, "jpg") || wdStringUtils::IsEqual_NoCase(szExtension, "jpeg"))
  {
    return true;
  }
#endif

  return false;
}

bool wdStbImageFileFormats::CanWriteFileType(const char* szExtension) const
{
  // even when WIC is available, prefer to write these files through STB, to get consistent output
  if (wdStringUtils::IsEqual_NoCase(szExtension, "png") || wdStringUtils::IsEqual_NoCase(szExtension, "jpg") || wdStringUtils::IsEqual_NoCase(szExtension, "jpeg"))
  {
    return true;
  }

  return false;
}



WD_STATICLINK_FILE(Texture, Texture_Image_Formats_StbImageFileFormats);
