#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>

wdImageView::wdImageView()
{
  Clear();
}

wdImageView::wdImageView(const wdImageHeader& header, wdConstByteBlobPtr imageData)
{
  ResetAndViewExternalStorage(header, imageData);
}

void wdImageView::Clear()
{
  wdImageHeader::Clear();
  m_SubImageOffsets.Clear();
  m_DataPtr.Clear();
}

bool wdImageView::IsValid() const
{
  return !m_DataPtr.IsEmpty();
}

void wdImageView::ResetAndViewExternalStorage(const wdImageHeader& header, wdConstByteBlobPtr imageData)
{
  static_cast<wdImageHeader&>(*this) = header;

  wdUInt64 dataSize = ComputeLayout();

  WD_ASSERT_DEV(imageData.GetCount() == dataSize, "Provided image storage ({} bytes) doesn't match required data size ({} bytes)",
    imageData.GetCount(), dataSize);

  // Const cast is safe here as we will only perform non-const access if this is an wdImage which owns mutable access to the storage
  m_DataPtr = wdBlobPtr<wdUInt8>(const_cast<wdUInt8*>(static_cast<const wdUInt8*>(imageData.GetPtr())), imageData.GetCount());
}

wdResult wdImageView::SaveTo(const char* szFileName) const
{
  WD_LOG_BLOCK("Writing Image", szFileName);

  if (m_Format == wdImageFormat::UNKNOWN)
  {
    wdLog::Error("Cannot write image '{0}' - image data is invalid or empty", szFileName);
    return WD_FAILURE;
  }

  wdFileWriter writer;
  if (writer.Open(szFileName) == WD_FAILURE)
  {
    wdLog::Error("Failed to open image file '{0}'", szFileName);
    return WD_FAILURE;
  }

  wdStringView it = wdPathUtils::GetFileExtension(szFileName);

  if (wdImageFileFormat* pFormat = wdImageFileFormat::GetWriterFormat(it.GetStartPointer()))
  {
    if (pFormat->WriteImage(writer, *this, it.GetStartPointer()) != WD_SUCCESS)
    {
      wdLog::Error("Failed to write image file '{0}'", szFileName);
      return WD_FAILURE;
    }

    return WD_SUCCESS;
  }

  wdLog::Error("No known image file format for extension '{0}'", it);
  return WD_FAILURE;
}

const wdImageHeader& wdImageView::GetHeader() const
{
  return *this;
}

wdImageView wdImageView::GetRowView(
  wdUInt32 uiMipLevel /*= 0*/, wdUInt32 uiFace /*= 0*/, wdUInt32 uiArrayIndex /*= 0*/, wdUInt32 y /*= 0*/, wdUInt32 z /*= 0*/, wdUInt32 uiPlaneIndex /*= 0*/) const
{
  wdImageHeader header;
  header.SetNumMipLevels(1);
  header.SetNumFaces(1);
  header.SetNumArrayIndices(1);

  // Scale dimensions relative to the block size of the subformat
  wdImageFormat::Enum subFormat = wdImageFormat::GetPlaneSubFormat(m_Format, uiPlaneIndex);
  header.SetWidth(GetWidth(uiMipLevel) * wdImageFormat::GetBlockWidth(subFormat) / wdImageFormat::GetBlockWidth(m_Format, uiPlaneIndex));
  header.SetHeight(wdImageFormat::GetBlockHeight(m_Format, 0) * wdImageFormat::GetBlockHeight(subFormat) / wdImageFormat::GetBlockHeight(m_Format, uiPlaneIndex));
  header.SetDepth(wdImageFormat::GetBlockDepth(subFormat) / wdImageFormat::GetBlockDepth(m_Format, uiPlaneIndex));
  header.SetImageFormat(wdImageFormat::GetPlaneSubFormat(m_Format, uiPlaneIndex));

  wdUInt64 offset = 0;

  offset += GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);
  offset += z * GetDepthPitch(uiMipLevel, uiPlaneIndex);
  offset += y * GetRowPitch(uiMipLevel, uiPlaneIndex);

  wdBlobPtr<const wdUInt8> dataSlice = m_DataPtr.GetSubArray(offset, GetRowPitch(uiMipLevel, uiPlaneIndex));
  return wdImageView(header, wdConstByteBlobPtr(dataSlice.GetPtr(), dataSlice.GetCount()));
}

void wdImageView::ReinterpretAs(wdImageFormat::Enum format)
{
  WD_ASSERT_DEBUG(
    wdImageFormat::IsCompressed(format) == wdImageFormat::IsCompressed(GetImageFormat()), "Cannot reinterpret compressed and non-compressed formats");

  WD_ASSERT_DEBUG(wdImageFormat::GetBitsPerPixel(GetImageFormat()) == wdImageFormat::GetBitsPerPixel(format),
    "Cannot reinterpret between formats of different sizes");

  SetImageFormat(format);
}

wdUInt64 wdImageView::ComputeLayout()
{
  m_SubImageOffsets.Clear();
  m_SubImageOffsets.Reserve(m_uiNumMipLevels * m_uiNumFaces * m_uiNumArrayIndices * GetPlaneCount());

  wdUInt64 uiDataSize = 0;

  for (wdUInt32 uiArrayIndex = 0; uiArrayIndex < m_uiNumArrayIndices; uiArrayIndex++)
  {
    for (wdUInt32 uiFace = 0; uiFace < m_uiNumFaces; uiFace++)
    {
      for (wdUInt32 uiMipLevel = 0; uiMipLevel < m_uiNumMipLevels; uiMipLevel++)
      {
        for (wdUInt32 uiPlaneIndex = 0; uiPlaneIndex < GetPlaneCount(); uiPlaneIndex++)
        {
          m_SubImageOffsets.PushBack(uiDataSize);

          uiDataSize += GetDepthPitch(uiMipLevel, uiPlaneIndex) * GetDepth(uiMipLevel);
        }
      }
    }
  }

  // Push back total size as a marker
  m_SubImageOffsets.PushBack(uiDataSize);

  return uiDataSize;
}

void wdImageView::ValidateSubImageIndices(wdUInt32 uiMipLevel, wdUInt32 uiFace, wdUInt32 uiArrayIndex, wdUInt32 uiPlaneIndex) const
{
  WD_ASSERT_DEV(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
  WD_ASSERT_DEV(uiFace < m_uiNumFaces, "Invalid uiFace");
  WD_ASSERT_DEV(uiArrayIndex < m_uiNumArrayIndices, "Invalid array slice");
  WD_ASSERT_DEV(uiPlaneIndex < GetPlaneCount(), "Invalid plane index");
}

const wdUInt64& wdImageView::GetSubImageOffset(wdUInt32 uiMipLevel, wdUInt32 uiFace, wdUInt32 uiArrayIndex, wdUInt32 uiPlaneIndex) const
{
  ValidateSubImageIndices(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);
  return m_SubImageOffsets[uiPlaneIndex + GetPlaneCount() * (uiMipLevel + m_uiNumMipLevels * (uiFace + m_uiNumFaces * uiArrayIndex))];
}

wdImage::wdImage()
{
  Clear();
}

wdImage::wdImage(const wdImageHeader& header)
{
  ResetAndAlloc(header);
}

wdImage::wdImage(const wdImageHeader& header, wdByteBlobPtr externalData)
{
  ResetAndUseExternalStorage(header, externalData);
}

wdImage::wdImage(wdImage&& other)
{
  ResetAndMove(std::move(other));
}

wdImage::wdImage(const wdImageView& other)
{
  ResetAndCopy(other);
}

void wdImage::operator=(wdImage&& rhs)
{
  ResetAndMove(std::move(rhs));
}

void wdImage::Clear()
{
  m_InternalStorage.Clear();

  wdImageView::Clear();
}

void wdImage::ResetAndAlloc(const wdImageHeader& header)
{
  const wdUInt64 requiredSize = header.ComputeDataSize();

  // it is debatable whether this function should reuse external storage, at all
  // however, it is especially dangerous to rely on the external storage being big enough, since many functions just take an wdImage as a
  // destination parameter and expect it to behave correctly when any of the Reset functions is called on it; it is not intuitive, that
  // Reset may fail due to how the image was previously reset

  // therefore, if external storage is insufficient, fall back to internal storage

  if (!UsesExternalStorage() || m_DataPtr.GetCount() < requiredSize)
  {
    m_InternalStorage.SetCountUninitialized(requiredSize);
    m_DataPtr = m_InternalStorage.GetBlobPtr<wdUInt8>();
  }

  wdImageView::ResetAndViewExternalStorage(header, wdConstByteBlobPtr(m_DataPtr.GetPtr(), m_DataPtr.GetCount()));
}

void wdImage::ResetAndUseExternalStorage(const wdImageHeader& header, wdByteBlobPtr externalData)
{
  m_InternalStorage.Clear();

  wdImageView::ResetAndViewExternalStorage(header, externalData);
}

void wdImage::ResetAndMove(wdImage&& other)
{
  static_cast<wdImageHeader&>(*this) = other.GetHeader();

  if (other.UsesExternalStorage())
  {
    m_InternalStorage.Clear();
    m_SubImageOffsets = std::move(other.m_SubImageOffsets);
    m_DataPtr = other.m_DataPtr;
    other.Clear();
  }
  else
  {
    m_InternalStorage = std::move(other.m_InternalStorage);
    m_SubImageOffsets = std::move(other.m_SubImageOffsets);
    m_DataPtr = m_InternalStorage.GetBlobPtr<wdUInt8>();
    other.Clear();
  }
}

void wdImage::ResetAndCopy(const wdImageView& other)
{
  ResetAndAlloc(other.GetHeader());

  memcpy(GetBlobPtr<wdUInt8>().GetPtr(), other.GetBlobPtr<wdUInt8>().GetPtr(), static_cast<size_t>(other.GetBlobPtr<wdUInt8>().GetCount()));
}

wdResult wdImage::LoadFrom(const char* szFileName)
{
  WD_LOG_BLOCK("Loading Image", szFileName);

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
    if (pFormat->ReadImage(reader, *this, it.GetStartPointer()) != WD_SUCCESS)
    {
      wdLog::Warning("Failed to read image file '{0}'", wdArgSensitive(szFileName, "File"));
      return WD_FAILURE;
    }

    return WD_SUCCESS;
  }

  wdLog::Warning("No known image file format for extension '{0}'", it);

  return WD_FAILURE;
}

wdResult wdImage::Convert(wdImageFormat::Enum targetFormat)
{
  return wdImageConversion::Convert(*this, *this, targetFormat);
}

wdImageView wdImageView::GetSubImageView(wdUInt32 uiMipLevel /*= 0*/, wdUInt32 uiFace /*= 0*/, wdUInt32 uiArrayIndex /*= 0*/) const
{
  wdImageHeader header;
  header.SetNumMipLevels(1);
  header.SetNumFaces(1);
  header.SetNumArrayIndices(1);
  header.SetWidth(GetWidth(uiMipLevel));
  header.SetHeight(GetHeight(uiMipLevel));
  header.SetDepth(GetDepth(uiMipLevel));
  header.SetImageFormat(m_Format);

  const wdUInt64& offset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, 0);
  wdUInt64 size = *(&offset + GetPlaneCount()) - offset;

  wdBlobPtr<const wdUInt8> subView = m_DataPtr.GetSubArray(offset, size);

  return wdImageView(header, wdConstByteBlobPtr(subView.GetPtr(), subView.GetCount()));
}

wdImage wdImage::GetSubImageView(wdUInt32 uiMipLevel /*= 0*/, wdUInt32 uiFace /*= 0*/, wdUInt32 uiArrayIndex /*= 0*/)
{
  wdImageView constView = wdImageView::GetSubImageView(uiMipLevel, uiFace, uiArrayIndex);

  // Create an wdImage attached to the view. Const cast is safe here since we own the storage.
  return wdImage(
    constView.GetHeader(), wdByteBlobPtr(const_cast<wdUInt8*>(constView.GetBlobPtr<wdUInt8>().GetPtr()), constView.GetBlobPtr<wdUInt8>().GetCount()));
}

wdImageView wdImageView::GetPlaneView(wdUInt32 uiMipLevel /*= 0*/, wdUInt32 uiFace /*= 0*/, wdUInt32 uiArrayIndex /*= 0*/, wdUInt32 uiPlaneIndex /*= 0*/) const
{
  wdImageHeader header;
  header.SetNumMipLevels(1);
  header.SetNumFaces(1);
  header.SetNumArrayIndices(1);

  // Scale dimensions relative to the block size of the first plane which determines the "nominal" width, height and depth
  wdImageFormat::Enum subFormat = wdImageFormat::GetPlaneSubFormat(m_Format, uiPlaneIndex);
  header.SetWidth(GetWidth(uiMipLevel) * wdImageFormat::GetBlockWidth(subFormat) / wdImageFormat::GetBlockWidth(m_Format, uiPlaneIndex));
  header.SetHeight(GetHeight(uiMipLevel) * wdImageFormat::GetBlockHeight(subFormat) / wdImageFormat::GetBlockHeight(m_Format, uiPlaneIndex));
  header.SetDepth(GetDepth(uiMipLevel) * wdImageFormat::GetBlockDepth(subFormat) / wdImageFormat::GetBlockDepth(m_Format, uiPlaneIndex));
  header.SetImageFormat(subFormat);

  const wdUInt64& offset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);
  wdUInt64 size = *(&offset + 1) - offset;

  wdBlobPtr<const wdUInt8> subView = m_DataPtr.GetSubArray(offset, size);

  return wdImageView(header, wdConstByteBlobPtr(subView.GetPtr(), subView.GetCount()));
}

wdImage wdImage::GetPlaneView(wdUInt32 uiMipLevel /* = 0 */, wdUInt32 uiFace /* = 0 */, wdUInt32 uiArrayIndex /* = 0 */, wdUInt32 uiPlaneIndex /* = 0 */)
{
  wdImageView constView = wdImageView::GetPlaneView(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex);

  // Create an wdImage attached to the view. Const cast is safe here since we own the storage.
  return wdImage(
    constView.GetHeader(), wdByteBlobPtr(const_cast<wdUInt8*>(constView.GetBlobPtr<wdUInt8>().GetPtr()), constView.GetBlobPtr<wdUInt8>().GetCount()));
}

wdImage wdImage::GetSliceView(wdUInt32 uiMipLevel /*= 0*/, wdUInt32 uiFace /*= 0*/, wdUInt32 uiArrayIndex /*= 0*/, wdUInt32 z /*= 0*/, wdUInt32 uiPlaneIndex /*= 0*/)
{
  wdImageView constView = wdImageView::GetSliceView(uiMipLevel, uiFace, uiArrayIndex, z, uiPlaneIndex);

  // Create an wdImage attached to the view. Const cast is safe here since we own the storage.
  return wdImage(
    constView.GetHeader(), wdByteBlobPtr(const_cast<wdUInt8*>(constView.GetBlobPtr<wdUInt8>().GetPtr()), constView.GetBlobPtr<wdUInt8>().GetCount()));
}

wdImageView wdImageView::GetSliceView(wdUInt32 uiMipLevel /*= 0*/, wdUInt32 uiFace /*= 0*/, wdUInt32 uiArrayIndex /*= 0*/, wdUInt32 z /*= 0*/, wdUInt32 uiPlaneIndex /*= 0*/) const
{
  wdImageHeader header;
  header.SetNumMipLevels(1);
  header.SetNumFaces(1);
  header.SetNumArrayIndices(1);

  // Scale dimensions relative to the block size of the first plane which determines the "nominal" width, height and depth
  wdImageFormat::Enum subFormat = wdImageFormat::GetPlaneSubFormat(m_Format, uiPlaneIndex);
  header.SetWidth(GetWidth(uiMipLevel) * wdImageFormat::GetBlockWidth(subFormat) / wdImageFormat::GetBlockWidth(m_Format, uiPlaneIndex));
  header.SetHeight(GetHeight(uiMipLevel) * wdImageFormat::GetBlockHeight(subFormat) / wdImageFormat::GetBlockHeight(m_Format, uiPlaneIndex));
  header.SetDepth(wdImageFormat::GetBlockDepth(subFormat) / wdImageFormat::GetBlockDepth(m_Format, uiPlaneIndex));
  header.SetImageFormat(subFormat);

  wdUInt64 offset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex) + z * GetDepthPitch(uiMipLevel, uiPlaneIndex);
  wdUInt64 size = GetDepthPitch(uiMipLevel, uiPlaneIndex);

  wdBlobPtr<const wdUInt8> subView = m_DataPtr.GetSubArray(offset, size);

  return wdImageView(header, wdConstByteBlobPtr(subView.GetPtr(), subView.GetCount()));
}

bool wdImage::UsesExternalStorage() const
{
  return m_InternalStorage.GetBlobPtr<wdUInt8>() != m_DataPtr;
}

WD_STATICLINK_FILE(Texture, Texture_Image_Implementation_Image);
