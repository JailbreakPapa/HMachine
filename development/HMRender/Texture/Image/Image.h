#pragma once

#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Logging/Log.h>

#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/ImageHeader.h>

/// \brief A class referencing image data and holding metadata about the image.
class WD_TEXTURE_DLL wdImageView : protected wdImageHeader
{
public:
  /// \brief Constructs an empty image view.
  wdImageView();

  /// \brief Constructs an image view with the given header and image data.
  wdImageView(const wdImageHeader& header, wdConstByteBlobPtr imageData);

  /// \brief Constructs an empty image view.
  void Clear();

  /// \brief Returns false if the image view does not reference any data yet.
  bool IsValid() const;

  /// \brief Constructs an image view with the given header and image data.
  void ResetAndViewExternalStorage(const wdImageHeader& header, wdConstByteBlobPtr imageData);

  /// \brief Convenience function to save the image to the given file.
  wdResult SaveTo(const char* szFileName) const;

  /// \brief Returns the header this image was constructed from.
  const wdImageHeader& GetHeader() const;

  /// \brief Returns a view to the entire data contained in this image.
  template <typename T>
  wdBlobPtr<const T> GetBlobPtr() const;

  wdConstByteBlobPtr GetByteBlobPtr() const;

  /// \brief Returns a view to the given sub-image.
  wdImageView GetSubImageView(wdUInt32 uiMipLevel = 0, wdUInt32 uiFace = 0, wdUInt32 uiArrayIndex = 0) const;

  /// \brief Returns a view to a sub-plane.
  wdImageView GetPlaneView(wdUInt32 uiMipLevel = 0, wdUInt32 uiFace = 0, wdUInt32 uiArrayIndex = 0, wdUInt32 uiPlaneIndex = 0) const;

  /// \brief Returns a view to z slice of the image.
  wdImageView GetSliceView(wdUInt32 uiMipLevel = 0, wdUInt32 uiFace = 0, wdUInt32 uiArrayIndex = 0, wdUInt32 z = 0, wdUInt32 uiPlaneIndex = 0) const;

  /// \brief Returns a view to a row of pixels resp. blocks.
  wdImageView GetRowView(wdUInt32 uiMipLevel = 0, wdUInt32 uiFace = 0, wdUInt32 uiArrayIndex = 0, wdUInt32 y = 0, wdUInt32 z = 0, wdUInt32 uiPlaneIndex = 0) const;

  /// \brief Returns a pointer to a given pixel or block contained in a sub-image.
  template <typename T>
  const T* GetPixelPointer(
    wdUInt32 uiMipLevel = 0, wdUInt32 uiFace = 0, wdUInt32 uiArrayIndex = 0, wdUInt32 x = 0, wdUInt32 y = 0, wdUInt32 z = 0, wdUInt32 uiPlaneIndex = 0) const;

  /// \brief Reinterprets the image with a given format; the format must have the same size in bits per pixel as the current one.
  void ReinterpretAs(wdImageFormat::Enum format);

public:
  using wdImageHeader::GetDepth;
  using wdImageHeader::GetHeight;
  using wdImageHeader::GetWidth;

  using wdImageHeader::GetNumArrayIndices;
  using wdImageHeader::GetNumFaces;
  using wdImageHeader::GetNumMipLevels;
  using wdImageHeader::GetPlaneCount;

  using wdImageHeader::GetImageFormat;

  using wdImageHeader::GetNumBlocksX;
  using wdImageHeader::GetNumBlocksY;
  using wdImageHeader::GetNumBlocksZ;

  using wdImageHeader::GetDepthPitch;
  using wdImageHeader::GetRowPitch;

protected:
  wdUInt64 ComputeLayout();

  void ValidateSubImageIndices(wdUInt32 uiMipLevel, wdUInt32 uiFace, wdUInt32 uiArrayIndex, wdUInt32 uiPlaneIndex) const;
  template <typename T>
  void ValidateDataTypeAccessor(wdUInt32 uiPlaneIndex) const;

  const wdUInt64& GetSubImageOffset(wdUInt32 uiMipLevel, wdUInt32 uiFace, wdUInt32 uiArrayIndex, wdUInt32 uiPlaneIndex) const;

  wdHybridArray<wdUInt64, 16> m_SubImageOffsets;
  wdBlobPtr<wdUInt8> m_DataPtr;
};

/// \brief A class containing image data and associated meta data.
///
/// This class is a lightweight container for image data and the description required for interpreting the data,
/// such as the image format, its dimensions, number of sub-images (i.e. cubemap faces, mip levels and array sub-images).
/// However, it does not provide any methods for interpreting or  modifying of the image data.
///
/// The sub-images are stored in a predefined order compatible with the layout of DDS files, that is, it first stores
/// the mip chain for each image, then all faces in a case of a cubemap, then the individual images of an image array.
class WD_TEXTURE_DLL wdImage : public wdImageView
{
  /// Use Reset() instead
  void operator=(const wdImage& rhs) = delete;

  /// Use Reset() instead
  void operator=(const wdImageView& rhs) = delete;

  /// \brief Constructs an image with the given header; allocating internal storage for it.
  explicit wdImage(const wdImageHeader& header);

  /// \brief Constructs an image with the given header backed by user-supplied external storage.
  explicit wdImage(const wdImageHeader& header, wdByteBlobPtr externalData);

  /// \brief Constructor from image view (copies the image data to internal storage)
  explicit wdImage(const wdImageView& other);

public:
  WD_DECLARE_MEM_RELOCATABLE_TYPE();

  /// \brief Constructs an empty image.
  wdImage();

  /// \brief Move constructor
  wdImage(wdImage&& other);

  void operator=(wdImage&& rhs);

  /// \brief Constructs an empty image. If the image is attached to an external storage, the attachment is discarded.
  void Clear();

  /// \brief Constructs an image with the given header and ensures sufficient storage is allocated.
  ///
  /// \note If this wdImage was previously attached to external storage, this will reuse that storage.
  /// However, if the external storage is not sufficiently large, ResetAndAlloc() will detach from it and allocate internal storage.
  void ResetAndAlloc(const wdImageHeader& header);

  /// \brief Constructs an image with the given header and attaches to the user-supplied external storage.
  ///
  /// The user is responsible to keep the external storage alive as long as this wdImage is alive.
  void ResetAndUseExternalStorage(const wdImageHeader& header, wdByteBlobPtr externalData);

  /// \brief Moves the given data into this object.
  ///
  /// If \a other is attached to an external storage, this object will also be attached to it,
  /// so life-time requirements for the external storage are now bound to this instance.
  void ResetAndMove(wdImage&& other);

  /// \brief Constructs from an image view. Copies the image data to internal storage.
  ///
  /// If the image is currently attached to external storage, the attachment is discarded.
  void ResetAndCopy(const wdImageView& other);

  /// \brief Convenience function to load the image from the given file.
  wdResult LoadFrom(const char* szFileName);

  /// \brief Convenience function to convert the image to the given format.
  wdResult Convert(wdImageFormat::Enum targetFormat);

  /// \brief Returns a view to the entire data contained in this image.
  template <typename T>
  wdBlobPtr<T> GetBlobPtr();

  wdByteBlobPtr GetByteBlobPtr();

  using wdImageView::GetBlobPtr;
  using wdImageView::GetByteBlobPtr;

  /// \brief Returns a view to the given sub-image.
  wdImage GetSubImageView(wdUInt32 uiMipLevel = 0, wdUInt32 uiFace = 0, wdUInt32 uiArrayIndex = 0);

  using wdImageView::GetSubImageView;

  /// \brief Returns a view to a sub-plane.
  wdImage GetPlaneView(wdUInt32 uiMipLevel = 0, wdUInt32 uiFace = 0, wdUInt32 uiArrayIndex = 0, wdUInt32 uiPlaneIndex = 0);

  using wdImageView::GetPlaneView;

  /// \brief Returns a view to z slice of the image.
  wdImage GetSliceView(wdUInt32 uiMipLevel = 0, wdUInt32 uiFace = 0, wdUInt32 uiArrayIndex = 0, wdUInt32 z = 0, wdUInt32 uiPlaneIndex = 0);

  using wdImageView::GetSliceView;

  /// \brief Returns a view to a row of pixels resp. blocks.
  wdImage GetRowView(wdUInt32 uiMipLevel = 0, wdUInt32 uiFace = 0, wdUInt32 uiArrayIndex = 0, wdUInt32 y = 0, wdUInt32 z = 0, wdUInt32 uiPlaneIndex = 0);

  using wdImageView::GetRowView;

  /// \brief Returns a pointer to a given pixel or block contained in a sub-image.
  template <typename T>
  T* GetPixelPointer(wdUInt32 uiMipLevel = 0, wdUInt32 uiFace = 0, wdUInt32 uiArrayIndex = 0, wdUInt32 x = 0, wdUInt32 y = 0, wdUInt32 z = 0, wdUInt32 uiPlaneIndex = 0);

  using wdImageView::GetPixelPointer;

private:
  bool UsesExternalStorage() const;

  wdBlob m_InternalStorage;
};

#include <Texture/Image/Implementation/Image_inl.h>
