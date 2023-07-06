#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Formats/ImageFileFormat.h>

WD_ENUMERABLE_CLASS_IMPLEMENTATION(wdImageFileFormat);

wdImageFileFormat* wdImageFileFormat::GetReaderFormat(const char* szExtension)
{
  for (wdImageFileFormat* pFormat = wdImageFileFormat::GetFirstInstance(); pFormat; pFormat = pFormat->GetNextInstance())
  {
    if (pFormat->CanReadFileType(szExtension))
    {
      return pFormat;
    }
  }

  return nullptr;
}

wdImageFileFormat* wdImageFileFormat::GetWriterFormat(const char* szExtension)
{
  for (wdImageFileFormat* pFormat = wdImageFileFormat::GetFirstInstance(); pFormat; pFormat = pFormat->GetNextInstance())
  {
    if (pFormat->CanWriteFileType(szExtension))
    {
      return pFormat;
    }
  }

  return nullptr;
}

wdResult wdImageFileFormat::ReadImageHeader(const char* szFileName, wdImageHeader& ref_header)
{
  WD_LOG_BLOCK("Read Image Header", szFileName);

  WD_PROFILE_SCOPE(wdPathUtils::GetFileNameAndExtension(szFileName).GetStartPointer());

  wdFileReader reader;
  if (reader.Open(szFileName) == WD_FAILURE)
  {
    wdLog::Warning("Failed to open image file '{0}'", wdArgSensitive(szFileName, "File"));
    return WD_FAILURE;
  }

  wdStringView it = wdPathUtils::GetFileExtension(szFileName);

  if (wdImageFileFormat* pFormat = wdImageFileFormat::GetReaderFormat(it.GetStartPointer()))
  {
    if (pFormat->ReadImageHeader(reader, ref_header, it.GetStartPointer()) != WD_SUCCESS)
    {
      wdLog::Warning("Failed to read image file '{0}'", wdArgSensitive(szFileName, "File"));
      return WD_FAILURE;
    }

    return WD_SUCCESS;
  }

  wdLog::Warning("No known image file format for extension '{0}'", it);
  return WD_FAILURE;
}

WD_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageFileFormat);
