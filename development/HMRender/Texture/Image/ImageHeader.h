#pragma once

#include <Foundation/Basics/Assert.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Math.h>

#include <Texture/Image/ImageFormat.h>
#include <Texture/TextureDLL.h>

/// \brief A class containing image meta data, such as format and dimensions.
///
/// This class has no associated behavior or functionality, and its getters and setters have no effect other than changing
/// the contained value. It is intended as a container to be modified by image utils and loaders.
class WD_TEXTURE_DLL wdImageHeader
{
public:
  /// \brief Constructs an image using an unknown format and zero size.
  wdImageHeader() { Clear(); }

  /// \brief Constructs an image using an unknown format and zero size.
  void Clear()
  {
    m_uiNumMipLevels = 1;
    m_uiNumFaces = 1;
    m_uiNumArrayIndices = 1;
    m_uiWidth = 0;
    m_uiHeight = 0;
    m_uiDepth = 1;
    m_Format = wdImageFormat::UNKNOWN;
  }

  /// \brief Sets the image format.
  void SetImageFormat(const wdImageFormat::Enum& format) { m_Format = format; }

  /// \brief Returns the image format.
  wdImageFormat::Enum GetImageFormat() const { return m_Format; }

  /// \brief Sets the image width.
  void SetWidth(wdUInt32 uiWidth) { m_uiWidth = uiWidth; }

  /// \brief Returns the image width for a given mip level, clamped to 1.
  wdUInt32 GetWidth(wdUInt32 uiMipLevel = 0) const
  {
    WD_ASSERT_DEV(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
    return wdMath::Max(m_uiWidth >> uiMipLevel, 1U);
  }

  /// \brief Sets the image height.
  void SetHeight(wdUInt32 uiHeight) { m_uiHeight = uiHeight; }

  /// \brief Returns the image height for a given mip level, clamped to 1.
  wdUInt32 GetHeight(wdUInt32 uiMipLevel = 0) const
  {
    WD_ASSERT_DEV(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
    return wdMath::Max(m_uiHeight >> uiMipLevel, 1U);
  }

  /// \brief Sets the image depth. The default is 1.
  void SetDepth(wdUInt32 uiDepth) { m_uiDepth = uiDepth; }

  /// \brief Returns the image depth for a given mip level, clamped to 1.
  wdUInt32 GetDepth(wdUInt32 uiMipLevel = 0) const
  {
    WD_ASSERT_DEV(uiMipLevel < m_uiNumMipLevels, "Invalid mip level");
    return wdMath::Max(m_uiDepth >> uiMipLevel, 1U);
  }

  /// \brief Sets the number of mip levels, including the full-size image.
  ///
  /// Setting this to 0 will result in an empty image.
  void SetNumMipLevels(wdUInt32 uiNumMipLevels) { m_uiNumMipLevels = uiNumMipLevels; }

  /// \brief Returns the number of mip levels, including the full-size image.
  wdUInt32 GetNumMipLevels() const { return m_uiNumMipLevels; }

  /// \brief Sets the number of cubemap faces. Use 1 for a non-cubemap.
  ///
  /// Setting this to 0 will result in an empty image.
  void SetNumFaces(wdUInt32 uiNumFaces) { m_uiNumFaces = uiNumFaces; }

  /// \brief Returns the number of cubemap faces, or 1 for a non-cubemap.
  wdUInt32 GetNumFaces() const { return m_uiNumFaces; }

  /// \brief Sets the number of array indices.
  ///
  /// Setting this to 0 will result in an empty image.
  void SetNumArrayIndices(wdUInt32 uiNumArrayIndices) { m_uiNumArrayIndices = uiNumArrayIndices; }

  /// \brief Returns the number of array indices.
  wdUInt32 GetNumArrayIndices() const { return m_uiNumArrayIndices; }

  /// \brief Returns the number of image planes.
  wdUInt32 GetPlaneCount() const
  {
    return wdImageFormat::GetPlaneCount(m_Format);
  }

  /// \brief Returns the number of blocks contained in a given mip level in the horizontal direction.
  wdUInt32 GetNumBlocksX(wdUInt32 uiMipLevel = 0, wdUInt32 uiPlaneIndex = 0) const
  {
    return wdImageFormat::GetNumBlocksX(m_Format, GetWidth(uiMipLevel), uiPlaneIndex);
  }

  /// \brief Returns the number of blocks contained in a given mip level in the horizontal direction.
  wdUInt32 GetNumBlocksY(wdUInt32 uiMipLevel = 0, wdUInt32 uiPlaneIndex = 0) const
  {
    return wdImageFormat::GetNumBlocksY(m_Format, GetHeight(uiMipLevel), uiPlaneIndex);
  }

  /// \brief Returns the number of blocks contained in a given mip level in the depth direction.
  wdUInt32 GetNumBlocksZ(wdUInt32 uiMipLevel = 0, wdUInt32 uiPlaneIndex = 0) const
  {
    return wdImageFormat::GetNumBlocksZ(m_Format, GetDepth(uiMipLevel), uiPlaneIndex);
  }

  /// \brief Returns the offset in bytes between two subsequent rows of the given mip level.
  wdUInt64 GetRowPitch(wdUInt32 uiMipLevel = 0, wdUInt32 uiPlaneIndex = 0) const
  {
    return wdImageFormat::GetRowPitch(m_Format, GetWidth(uiMipLevel), uiPlaneIndex);
  }

  /// \brief Returns the offset in bytes between two subsequent depth slices of the given mip level.
  wdUInt64 GetDepthPitch(wdUInt32 uiMipLevel = 0, wdUInt32 uiPlaneIndex = 0) const
  {
    return wdImageFormat::GetDepthPitch(m_Format, GetWidth(uiMipLevel), GetHeight(uiMipLevel), uiPlaneIndex);
  }

  /// \brief Computes the data size required for an image with the header's format and dimensions.
  wdUInt64 ComputeDataSize() const
  {
    wdUInt64 uiDataSize = 0;

    for (wdUInt32 uiMipLevel = 0; uiMipLevel < GetNumMipLevels(); uiMipLevel++)
    {
      for (wdUInt32 uiPlaneIndex = 0; uiPlaneIndex < GetPlaneCount(); ++uiPlaneIndex)
      {
        uiDataSize += GetDepthPitch(uiMipLevel, uiPlaneIndex) * static_cast<wdUInt64>(GetDepth(uiMipLevel));
      }
    }

    return wdMath::SafeMultiply64(uiDataSize, wdMath::SafeMultiply32(GetNumArrayIndices(), GetNumFaces()));
  }

  /// \brief Computes the number of mip maps in the full mip chain.
  wdUInt32 ComputeNumberOfMipMaps() const
  {
    wdUInt32 numMipMaps = 1;
    wdUInt32 width = GetWidth();
    wdUInt32 height = GetHeight();
    wdUInt32 depth = GetDepth();

    while (width > 1 || height > 1 || depth > 1)
    {
      width = wdMath::Max(1u, width / 2);
      height = wdMath::Max(1u, height / 2);
      depth = wdMath::Max(1u, depth / 2);

      numMipMaps++;
    }

    return numMipMaps;
  }

protected:
  wdUInt32 m_uiNumMipLevels;
  wdUInt32 m_uiNumFaces;
  wdUInt32 m_uiNumArrayIndices;

  wdUInt32 m_uiWidth;
  wdUInt32 m_uiHeight;
  wdUInt32 m_uiDepth;

  wdImageFormat::Enum m_Format;
};
