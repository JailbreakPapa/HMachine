#pragma once

#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Size.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageEnums.h>
#include <Texture/Image/ImageFilter.h>

class WD_TEXTURE_DLL wdImageUtils
{
public:
  /// \brief Returns the image with the difference (absolute values) between ImageA and ImageB.
  static void ComputeImageDifferenceABS(const wdImageView& imageA, const wdImageView& imageB, wdImage& out_difference);

  /// \brief Computes the mean square error for the block at (offsetx, offsety) to (offsetx + uiBlockSize, offsety + uiBlockSize).
  /// DifferenceImage is expected to be an image that represents the difference between two images.
  static wdUInt32 ComputeMeanSquareError(const wdImageView& differenceImage, wdUInt8 uiBlockSize, wdUInt32 uiOffsetx, wdUInt32 uiOffsety);

  /// \brief Computes the mean square error of DifferenceImage, by computing the MSE for blocks of uiBlockSize and returning the maximum MSE
  /// that was found.
  static wdUInt32 ComputeMeanSquareError(const wdImageView& differenceImage, wdUInt8 uiBlockSize);

  /// \brief Rescales pixel values to use the full value range by scaling from [min, max] to [0, 255].
  /// Computes combined min/max for RGB and separate min/max for alpha.
  static void Normalize(wdImage& ref_image);
  static void Normalize(wdImage& ref_image, wdUInt8& ref_uiMinRgb, wdUInt8& ref_uiMaxRgb, wdUInt8& ref_uiMinAlpha, wdUInt8& ref_uiMaxAlpha);

  /// \brief Extracts the alpha channel from 8bpp 4 channel images into a 8bpp single channel image.
  static void ExtractAlphaChannel(const wdImageView& inputImage, wdImage& ref_outputImage);

  /// \brief Returns the sub-image of \a input that starts at \a offset and has the size \a newsize
  static void CropImage(const wdImageView& input, const wdVec2I32& vOffset, const wdSizeU32& newsize, wdImage& ref_output);

  /// \brief rotates a sub image by 180 degrees in place. Only works with uncompressed images.
  static void RotateSubImage180(wdImage& ref_image, wdUInt32 uiMipLevel = 0, wdUInt32 uiFace = 0, wdUInt32 uiArrayIndex = 0);

  /// \brief Copies the source image into the destination image at the specified location.
  ///
  /// The image must fit, no scaling or cropping is done. Image formats must be identical. Compressed formats are not supported.
  /// If the target location leaves not enough room for the source image to be copied, bad stuff will happen.
  static wdResult Copy(const wdImageView& srcImg, const wdRectU32& srcRect, wdImage& ref_dstImg, const wdVec3U32& vDstOffset, wdUInt32 uiDstMipLevel = 0,
    wdUInt32 uiDstFace = 0, wdUInt32 uiDstArrayIndex = 0);

  /// \brief Copies the lower uiNumMips data of a 2D image into another one.
  static wdResult ExtractLowerMipChain(const wdImageView& src, wdImage& ref_dst, wdUInt32 uiNumMips);

  /// Mip map generation options
  struct MipMapOptions
  {
    /// The filter to use for mipmap generation. Defaults to bilinear filtering (Triangle filter) if none is given.
    const wdImageFilter* m_filter = nullptr;

    /// Rescale RGB components to unit length.
    bool m_renormalizeNormals = false;

    /// If true, the alpha values are scaled to preserve the average coverage when alpha testing is enabled,
    bool m_preserveCoverage = false;

    /// The alpha test threshold to use when m_preserveCoverage == true.
    float m_alphaThreshold = 0.5f;

    /// The address mode for samples when filtering outside of the image dimensions in the horizontal direction.
    wdImageAddressMode::Enum m_addressModeU = wdImageAddressMode::Clamp;

    /// The address mode for samples when filtering outside of the image dimensions in the vertical direction.
    wdImageAddressMode::Enum m_addressModeV = wdImageAddressMode::Clamp;

    /// The address mode for samples when filtering outside of the image dimensions in the depth direction.
    wdImageAddressMode::Enum m_addressModeW = wdImageAddressMode::Clamp;

    /// The border color if texture address mode equals BORDER.
    wdColor m_borderColor = wdColor::Black;

    /// How many mip maps should be generated. Pass 0 to generate all mip map levels.
    wdUInt32 m_numMipMaps = 0;
  };

  /// Scales the image.
  static wdResult Scale(const wdImageView& source, wdImage& ref_target, wdUInt32 uiWidth, wdUInt32 uiHeight, const wdImageFilter* pFilter = nullptr,
    wdImageAddressMode::Enum addressModeU = wdImageAddressMode::Clamp, wdImageAddressMode::Enum addressModeV = wdImageAddressMode::Clamp,
    const wdColor& borderColor = wdColor::Black);

  /// Scales the image.
  static wdResult Scale3D(const wdImageView& source, wdImage& ref_target, wdUInt32 uiWidth, wdUInt32 uiHeight, wdUInt32 uiDepth,
    const wdImageFilter* pFilter = nullptr, wdImageAddressMode::Enum addressModeU = wdImageAddressMode::Clamp,
    wdImageAddressMode::Enum addressModeV = wdImageAddressMode::Clamp, wdImageAddressMode::Enum addressModeW = wdImageAddressMode::Clamp,
    const wdColor& borderColor = wdColor::Black);

  /// Genererates the mip maps for the image. The input texture must be in wdImageFormat::R32_G32_B32_A32_FLOAT
  static void GenerateMipMaps(const wdImageView& source, wdImage& ref_target, const MipMapOptions& options);

  /// Assumes that the Red and Green components of an image contain XY of an unit length normal and reconstructs the Z component into B
  static void ReconstructNormalZ(wdImage& ref_source);

  /// Renormalizes a normal map to unit length.
  static void RenormalizeNormalMap(wdImage& ref_image);

  /// Adjust the roughness in lower mip levels so it maintains the same look from all distances.
  static void AdjustRoughness(wdImage& ref_roughnessMap, const wdImageView& normalMap);

  /// \brief Changes the exposure of an HDR image by 2^bias
  static void ChangeExposure(wdImage& ref_image, float fBias);

  /// \brief Creates a cubemap from srcImg and stores it in dstImg.
  ///
  /// If srcImg is already a cubemap, the data will be copied 1:1 to dstImg.
  /// If it is a 2D texture, it is analyzed and sub-images are copied to the proper faces of the output cubemap.
  ///
  /// Supported input layouts are:
  ///  * Vertical Cross
  ///  * Horizontal Cross
  ///  * Spherical mapping
  static wdResult CreateCubemapFromSingleFile(wdImage& ref_dstImg, const wdImageView& srcImg);

  /// \brief Copies the 6 given source images to the faces of dstImg.
  ///
  /// All input images must have the same square, power-of-two dimensions and mustn't be compressed.
  static wdResult CreateCubemapFrom6Files(wdImage& ref_dstImg, const wdImageView* pSourceImages);

  static wdResult CreateVolumeTextureFromSingleFile(wdImage& ref_dstImg, const wdImageView& srcImg);

  static wdUInt32 GetSampleIndex(wdUInt32 uiNumTexels, wdInt32 iIndex, wdImageAddressMode::Enum addressMode, bool& out_bUseBorderColor);

  /// \brief Samples the image at the given UV coordinates with nearest filtering.
  ///
  /// This function has to validate that the image is of the right format, and has to query the pixel pointer, which is slow.
  /// If you need to sample the image very often, use the overload that takes a pixel pointer instead of an image.
  static wdColor NearestSample(const wdImageView& image, wdImageAddressMode::Enum addressMode, wdVec2 vUv);

  /// \brief Samples the image at the given UV coordinates with nearest filtering.
  ///
  /// Prefer this function over the one that takes an wdImageView when you need to sample the image very often,
  /// as it does away with internal validation that would be redundant. Also, the pixel pointer given to this function
  /// should be retrieved only once from the source image, as wdImage::GetPixelPointer() is rather slow due to validation overhead.
  static wdColor NearestSample(const wdColor* pPixelPointer, wdUInt32 uiWidth, wdUInt32 uiHeight, wdImageAddressMode::Enum addressMode, wdVec2 vUv);

  /// \brief Samples the image at the given UV coordinates with bilinear filtering.
  ///
  /// This function has to validate that the image is of the right format, and has to query the pixel pointer, which is slow.
  /// If you need to sample the image very often, use the overload that takes a pixel pointer instead of an image.
  static wdColor BilinearSample(const wdImageView& image, wdImageAddressMode::Enum addressMode, wdVec2 vUv);

  /// \brief Samples the image at the given UV coordinates with bilinear filtering.
  ///
  /// Prefer this function over the one that takes an wdImageView when you need to sample the image very often,
  /// as it does away with internal validation that would be redundant. Also, the pixel pointer given to this function
  /// should be retrieved only once from the source image, as wdImage::GetPixelPointer() is rather slow due to validation overhead.
  static wdColor BilinearSample(const wdColor* pPixelPointer, wdUInt32 uiWidth, wdUInt32 uiHeight, wdImageAddressMode::Enum addressMode, wdVec2 vUv);

  /// \brief Copies channel 0, 1, 2 or 3 from srcImg into dstImg.
  ///
  /// Currently only supports images of format R32G32B32A32_FLOAT and with identical resolution.
  /// Returns failure if any of those requirements are not met.
  static wdResult CopyChannel(wdImage& ref_dstImg, wdUInt8 uiDstChannelIdx, const wdImage& srcImg, wdUInt8 uiSrcChannelIdx);
};
