#pragma once

template <typename T>
struct wdImageSizeofHelper
{
  static constexpr size_t Size = sizeof(T);
};

template <>
struct wdImageSizeofHelper<void>
{
  static constexpr size_t Size = 1;
};

template <>
struct wdImageSizeofHelper<const void>
{
  static constexpr size_t Size = 1;
};

template <typename T>
wdBlobPtr<const T> wdImageView::GetBlobPtr() const
{
  for (wdUInt32 uiPlaneIndex = 0; uiPlaneIndex < GetPlaneCount(); ++uiPlaneIndex)
  {
    ValidateDataTypeAccessor<T>(uiPlaneIndex);
  }
  return wdBlobPtr<const T>(reinterpret_cast<T*>(static_cast<wdUInt8*>(m_DataPtr.GetPtr())), m_DataPtr.GetCount() / wdImageSizeofHelper<T>::Size);
}

inline wdConstByteBlobPtr wdImageView::GetByteBlobPtr() const
{
  for (wdUInt32 uiPlaneIndex = 0; uiPlaneIndex < GetPlaneCount(); ++uiPlaneIndex)
  {
    ValidateDataTypeAccessor<wdUInt8>(uiPlaneIndex);
  }
  return wdConstByteBlobPtr(static_cast<wdUInt8*>(m_DataPtr.GetPtr()), m_DataPtr.GetCount());
}

template <typename T>
wdBlobPtr<T> wdImage::GetBlobPtr()
{
  wdBlobPtr<const T> constPtr = wdImageView::GetBlobPtr<T>();

  return wdBlobPtr<T>(const_cast<T*>(static_cast<const T*>(constPtr.GetPtr())), constPtr.GetCount());
}

inline wdByteBlobPtr wdImage::GetByteBlobPtr()
{
  wdConstByteBlobPtr constPtr = wdImageView::GetByteBlobPtr();

  return wdByteBlobPtr(const_cast<wdUInt8*>(constPtr.GetPtr()), constPtr.GetCount());
}

template <typename T>
const T* wdImageView::GetPixelPointer(wdUInt32 uiMipLevel /*= 0*/, wdUInt32 uiFace /*= 0*/, wdUInt32 uiArrayIndex /*= 0*/, wdUInt32 x /*= 0*/,
  wdUInt32 y /*= 0*/, wdUInt32 z /*= 0*/, wdUInt32 uiPlaneIndex /*= 0*/) const
{
  ValidateDataTypeAccessor<T>(uiPlaneIndex);
  WD_ASSERT_DEV(x < GetNumBlocksX(uiMipLevel, uiPlaneIndex), "Invalid x coordinate");
  WD_ASSERT_DEV(y < GetNumBlocksY(uiMipLevel, uiPlaneIndex), "Invalid y coordinate");
  WD_ASSERT_DEV(z < GetNumBlocksZ(uiMipLevel, uiPlaneIndex), "Invalid z coordinate");

  wdUInt64 offset = GetSubImageOffset(uiMipLevel, uiFace, uiArrayIndex, uiPlaneIndex) +
                    z * GetDepthPitch(uiMipLevel, uiPlaneIndex) +
                    y * GetRowPitch(uiMipLevel, uiPlaneIndex) +
                    x * wdImageFormat::GetBitsPerBlock(m_Format, uiPlaneIndex) / 8;
  return reinterpret_cast<const T*>(&m_DataPtr[offset]);
}

template <typename T>
T* wdImage::GetPixelPointer(
  wdUInt32 uiMipLevel /*= 0*/, wdUInt32 uiFace /*= 0*/, wdUInt32 uiArrayIndex /*= 0*/, wdUInt32 x /*= 0*/, wdUInt32 y /*= 0*/, wdUInt32 z /*= 0*/, wdUInt32 uiPlaneIndex /*= 0*/)
{
  return const_cast<T*>(wdImageView::GetPixelPointer<T>(uiMipLevel, uiFace, uiArrayIndex, x, y, z, uiPlaneIndex));
}


template <typename T>
void wdImageView::ValidateDataTypeAccessor(wdUInt32 uiPlaneIndex) const
{
  wdUInt32 bytesPerBlock = wdImageFormat::GetBitsPerBlock(GetImageFormat(), uiPlaneIndex) / 8;
  WD_ASSERT_DEV(bytesPerBlock % wdImageSizeofHelper<T>::Size == 0, "Accessor type is not suitable for interpreting contained data");
}
