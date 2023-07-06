#include <Texture/TexturePCH.h>

#include <Texture/Image/ImageUtils.h>

#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageEnums.h>
#include <Texture/Image/ImageFilter.h>

template <typename TYPE>
static void SetDiff(const wdImageView& imageA, const wdImageView& imageB, wdImage& out_difference, wdUInt32 w, wdUInt32 h, wdUInt32 d, wdUInt32 uiComp)
{
  const TYPE* pA = imageA.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);
  const TYPE* pB = imageB.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);
  TYPE* pR = out_difference.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);

  for (wdUInt32 i = 0; i < uiComp; ++i)
    pR[i] = pB[i] > pA[i] ? (pB[i] - pA[i]) : (pA[i] - pB[i]);
}

template <typename TYPE>
static wdUInt32 GetError(const wdImageView& difference, wdUInt32 w, wdUInt32 h, wdUInt32 d, wdUInt32 uiComp, wdUInt32 uiPixel)
{
  const TYPE* pR = difference.GetPixelPointer<TYPE>(0, 0, 0, w, h, d);

  wdUInt32 uiErrorSum = 0;

  for (wdUInt32 p = 0; p < uiPixel; ++p)
  {
    wdUInt32 error = 0;

    for (wdUInt32 c = 0; c < uiComp; ++c)
    {
      error += *pR;
      ++pR;
    }

    error /= uiComp;
    uiErrorSum += error * error;
  }

  return uiErrorSum;
}

void wdImageUtils::ComputeImageDifferenceABS(const wdImageView& imageA, const wdImageView& imageB, wdImage& out_difference)
{
  WD_PROFILE_SCOPE("wdImageUtils::ComputeImageDifferenceABS");

  WD_ASSERT_DEV(imageA.GetWidth() == imageB.GetWidth(), "Dimensions do not match");
  WD_ASSERT_DEV(imageA.GetHeight() == imageB.GetHeight(), "Dimensions do not match");
  WD_ASSERT_DEV(imageA.GetDepth() == imageB.GetDepth(), "Dimensions do not match");
  WD_ASSERT_DEV(imageA.GetImageFormat() == imageB.GetImageFormat(), "Format does not match");

  wdImageHeader differenceHeader;

  differenceHeader.SetWidth(imageA.GetWidth());
  differenceHeader.SetHeight(imageA.GetHeight());
  differenceHeader.SetDepth(imageA.GetDepth());
  differenceHeader.SetImageFormat(imageA.GetImageFormat());
  out_difference.ResetAndAlloc(differenceHeader);

  const wdUInt32 uiSize2D = imageA.GetHeight() * imageA.GetWidth();

  for (wdUInt32 d = 0; d < imageA.GetDepth(); ++d)
  {
    // for (wdUInt32 h = 0; h < ImageA.GetHeight(); ++h)
    {
      // for (wdUInt32 w = 0; w < ImageA.GetWidth(); ++w)
      {
        switch (imageA.GetImageFormat())
        {
          case wdImageFormat::R8G8B8A8_UNORM:
          case wdImageFormat::R8G8B8A8_UNORM_SRGB:
          case wdImageFormat::R8G8B8A8_UINT:
          case wdImageFormat::R8G8B8A8_SNORM:
          case wdImageFormat::R8G8B8A8_SINT:
          case wdImageFormat::B8G8R8A8_UNORM:
          case wdImageFormat::B8G8R8X8_UNORM:
          case wdImageFormat::B8G8R8A8_UNORM_SRGB:
          case wdImageFormat::B8G8R8X8_UNORM_SRGB:
          {
            SetDiff<wdUInt8>(imageA, imageB, out_difference, 0, 0, d, 4 * uiSize2D);
          }
          break;

          case wdImageFormat::B8G8R8_UNORM:
          {
            SetDiff<wdUInt8>(imageA, imageB, out_difference, 0, 0, d, 3 * uiSize2D);
          }
          break;

          default:
            WD_REPORT_FAILURE("The wdImageFormat {0} is not implemented", (wdUInt32)imageA.GetImageFormat());
            return;
        }
      }
    }
  }
}

wdUInt32 wdImageUtils::ComputeMeanSquareError(const wdImageView& differenceImage, wdUInt8 uiBlockSize, wdUInt32 uiOffsetx, wdUInt32 uiOffsety)
{
  WD_PROFILE_SCOPE("wdImageUtils::ComputeMeanSquareError(detail)");

  WD_ASSERT_DEV(uiBlockSize > 1, "Blocksize must be at least 2");

  wdUInt32 uiWidth = wdMath::Min(differenceImage.GetWidth(), uiOffsetx + uiBlockSize) - uiOffsetx;
  wdUInt32 uiHeight = wdMath::Min(differenceImage.GetHeight(), uiOffsety + uiBlockSize) - uiOffsety;

  if (uiWidth == 0 || uiHeight == 0)
    return 0;

  switch (differenceImage.GetImageFormat())
  {
      // Supported formats
    case wdImageFormat::R8G8B8A8_UNORM:
    case wdImageFormat::R8G8B8A8_UNORM_SRGB:
    case wdImageFormat::R8G8B8A8_UINT:
    case wdImageFormat::R8G8B8A8_SNORM:
    case wdImageFormat::R8G8B8A8_SINT:
    case wdImageFormat::B8G8R8A8_UNORM:
    case wdImageFormat::B8G8R8A8_UNORM_SRGB:
    case wdImageFormat::B8G8R8_UNORM:
      break;

    default:
      WD_REPORT_FAILURE("The wdImageFormat {0} is not implemented", (wdUInt32)differenceImage.GetImageFormat());
      return 0;
  }


  wdUInt32 error = 0;

  wdUInt64 uiRowPitch = differenceImage.GetRowPitch();
  wdUInt64 uiDepthPitch = differenceImage.GetDepthPitch();
  wdUInt32 uiNumComponents = wdImageFormat::GetNumChannels(differenceImage.GetImageFormat());

  // Treat image as single-component format and scale the width instead
  uiWidth *= uiNumComponents;

  const wdUInt32 uiSize2D = uiWidth * uiHeight;
  const wdUInt8* pSlicePointer = differenceImage.GetPixelPointer<wdUInt8>(0, 0, 0, uiOffsetx, uiOffsety);

  for (wdUInt32 d = 0; d < differenceImage.GetDepth(); ++d)
  {
    const wdUInt8* pRowPointer = pSlicePointer;

    for (wdUInt32 y = 0; y < uiHeight; ++y)
    {
      const wdUInt8* pPixelPointer = pRowPointer;
      for (wdUInt32 x = 0; x < uiWidth; ++x)
      {
        wdUInt32 uiDiff = *pPixelPointer;
        error += uiDiff * uiDiff;

        pPixelPointer++;
      }

      pRowPointer += uiRowPitch;
    }

    pSlicePointer += uiDepthPitch;
  }

  error /= uiSize2D;
  return error;
}

wdUInt32 wdImageUtils::ComputeMeanSquareError(const wdImageView& differenceImage, wdUInt8 uiBlockSize)
{
  WD_PROFILE_SCOPE("wdImageUtils::ComputeMeanSquareError");

  WD_ASSERT_DEV(uiBlockSize > 1, "Blocksize must be at least 2");

  const wdUInt32 uiHalfBlockSize = uiBlockSize / 2;

  const wdUInt32 uiBlocksX = (differenceImage.GetWidth() / uiHalfBlockSize) + 1;
  const wdUInt32 uiBlocksY = (differenceImage.GetHeight() / uiHalfBlockSize) + 1;

  wdUInt32 uiMaxError = 0;

  for (wdUInt32 by = 0; by < uiBlocksY; ++by)
  {
    for (wdUInt32 bx = 0; bx < uiBlocksX; ++bx)
    {
      const wdUInt32 uiBlockError = ComputeMeanSquareError(differenceImage, uiBlockSize, bx * uiHalfBlockSize, by * uiHalfBlockSize);

      uiMaxError = wdMath::Max(uiMaxError, uiBlockError);
    }
  }

  return uiMaxError;
}

template <typename Func, typename ImageType>
static void ApplyFunc(ImageType& inout_image, Func func)
{
  wdUInt32 uiWidth = inout_image.GetWidth();
  wdUInt32 uiHeight = inout_image.GetHeight();
  wdUInt32 uiDepth = inout_image.GetDepth();

  WD_ASSERT_DEV(uiWidth > 0 && uiHeight > 0 && uiDepth > 0, "The image passed to FindMinMax has illegal dimension {}x{}x{}.", uiWidth, uiHeight, uiDepth);

  wdUInt64 uiRowPitch = inout_image.GetRowPitch();
  wdUInt64 uiDepthPitch = inout_image.GetDepthPitch();
  wdUInt32 uiNumChannels = wdImageFormat::GetNumChannels(inout_image.GetImageFormat());

  auto pSlicePointer = inout_image.template GetPixelPointer<wdUInt8>();

  for (wdUInt32 z = 0; z < inout_image.GetDepth(); ++z)
  {
    auto pRowPointer = pSlicePointer;

    for (wdUInt32 y = 0; y < uiHeight; ++y)
    {
      auto pPixelPointer = pRowPointer;
      for (wdUInt32 x = 0; x < uiWidth; ++x)
      {
        for (wdUInt32 c = 0; c < uiNumChannels; ++c)
        {
          func(pPixelPointer++, x, y, z, c);
        }
      }

      pRowPointer += uiRowPitch;
    }

    pSlicePointer += uiDepthPitch;
  }
}

static void FindMinMax(const wdImageView& image, wdUInt8& out_uiMinRgb, wdUInt8& out_uiMaxRgb, wdUInt8& out_uiMinAlpha, wdUInt8& out_uiMaxAlpha)
{
  wdImageFormat::Enum imageFormat = image.GetImageFormat();
  WD_ASSERT_DEV(wdImageFormat::GetBitsPerChannel(imageFormat, wdImageFormatChannel::R) == 8 && wdImageFormat::GetDataType(imageFormat) == wdImageFormatDataType::UNORM, "Only 8bpp unorm formats are supported in FindMinMax");

  out_uiMinRgb = 255u;
  out_uiMinAlpha = 255u;
  out_uiMaxRgb = 0u;
  out_uiMaxAlpha = 0u;

  auto minMax = [&](const wdUInt8* pPixel, wdUInt32 /*x*/, wdUInt32 /*y*/, wdUInt32 /*z*/, wdUInt32 c) {
    wdUInt8 val = *pPixel;

    if (c < 3)
    {
      out_uiMinRgb = wdMath::Min(out_uiMinRgb, val);
      out_uiMaxRgb = wdMath::Max(out_uiMaxRgb, val);
    }
    else
    {
      out_uiMinAlpha = wdMath::Min(out_uiMinAlpha, val);
      out_uiMaxAlpha = wdMath::Max(out_uiMaxAlpha, val);
    }
  };
  ApplyFunc(image, minMax);
}

void wdImageUtils::Normalize(wdImage& inout_image)
{
  wdUInt8 uiMinRgb, uiMaxRgb, uiMinAlpha, uiMaxAlpha;
  Normalize(inout_image, uiMinRgb, uiMaxRgb, uiMinAlpha, uiMaxAlpha);
}

void wdImageUtils::Normalize(wdImage& inout_image, wdUInt8& out_uiMinRgb, wdUInt8& out_uiMaxRgb, wdUInt8& out_uiMinAlpha, wdUInt8& out_uiMaxAlpha)
{
  WD_PROFILE_SCOPE("wdImageUtils::Normalize");

  wdImageFormat::Enum imageFormat = inout_image.GetImageFormat();

  WD_ASSERT_DEV(wdImageFormat::GetBitsPerChannel(imageFormat, wdImageFormatChannel::R) == 8 && wdImageFormat::GetDataType(imageFormat) == wdImageFormatDataType::UNORM, "Only 8bpp unorm formats are supported in NormalizeImage");

  bool ignoreAlpha = false;
  if (imageFormat == wdImageFormat::B8G8R8X8_UNORM || imageFormat == wdImageFormat::B8G8R8X8_UNORM_SRGB)
  {
    ignoreAlpha = true;
  }

  FindMinMax(inout_image, out_uiMinRgb, out_uiMaxRgb, out_uiMinAlpha, out_uiMaxAlpha);
  wdUInt8 uiRangeRgb = out_uiMaxRgb - out_uiMinRgb;
  wdUInt8 uiRangeAlpha = out_uiMaxAlpha - out_uiMinAlpha;

  auto normalize = [&](wdUInt8* pPixel, wdUInt32 /*x*/, wdUInt32 /*y*/, wdUInt32 /*z*/, wdUInt32 c) {
    wdUInt8 val = *pPixel;
    if (c < 3)
    {
      // color channels are uniform when min == max, in that case keep original value as scaling is not meaningful
      if (uiRangeRgb != 0)
      {
        *pPixel = static_cast<wdUInt8>(255u * (static_cast<float>(val - out_uiMinRgb) / (uiRangeRgb)));
      }
    }
    else
    {
      // alpha is uniform when minAlpha == maxAlpha, in that case keep original alpha as scaling is not meaningful
      if (!ignoreAlpha && uiRangeAlpha != 0)
      {
        *pPixel = static_cast<wdUInt8>(255u * (static_cast<float>(val - out_uiMinAlpha) / (uiRangeAlpha)));
      }
    }
  };
  ApplyFunc(inout_image, normalize);
}

void wdImageUtils::ExtractAlphaChannel(const wdImageView& inputImage, wdImage& inout_outputImage)
{
  WD_PROFILE_SCOPE("wdImageUtils::ExtractAlphaChannel");

  switch (wdImageFormat::Enum imageFormat = inputImage.GetImageFormat())
  {
    case wdImageFormat::R8G8B8A8_UNORM:
    case wdImageFormat::R8G8B8A8_UNORM_SRGB:
    case wdImageFormat::R8G8B8A8_UINT:
    case wdImageFormat::R8G8B8A8_SNORM:
    case wdImageFormat::R8G8B8A8_SINT:
    case wdImageFormat::B8G8R8A8_UNORM:
    case wdImageFormat::B8G8R8A8_UNORM_SRGB:
      break;
    default:
      WD_REPORT_FAILURE("ExtractAlpha needs an image with 8bpp and 4 channel. The wdImageFormat {} is not supported.", (wdUInt32)imageFormat);
      return;
  }

  wdImageHeader outputHeader = inputImage.GetHeader();
  outputHeader.SetImageFormat(wdImageFormat::R8_UNORM);
  inout_outputImage.ResetAndAlloc(outputHeader);

  const wdUInt8* pInputSlice = inputImage.GetPixelPointer<wdUInt8>();
  wdUInt8* pOutputSlice = inout_outputImage.GetPixelPointer<wdUInt8>();

  wdUInt64 uiInputRowPitch = inputImage.GetRowPitch();
  wdUInt64 uiInputDepthPitch = inputImage.GetDepthPitch();

  wdUInt64 uiOutputRowPitch = inout_outputImage.GetRowPitch();
  wdUInt64 uiOutputDepthPitch = inout_outputImage.GetDepthPitch();

  for (wdUInt32 d = 0; d < inputImage.GetDepth(); ++d)
  {
    const wdUInt8* pInputRow = pInputSlice;
    wdUInt8* pOutputRow = pOutputSlice;

    for (wdUInt32 y = 0; y < inputImage.GetHeight(); ++y)
    {
      const wdUInt8* pInputPixel = pInputRow;
      wdUInt8* pOutputPixel = pOutputRow;
      for (wdUInt32 x = 0; x < inputImage.GetWidth(); ++x)
      {
        *pOutputPixel = pInputPixel[3];

        pInputPixel += 4;
        ++pOutputPixel;
      }

      pInputRow += uiInputRowPitch;
      pOutputRow += uiOutputRowPitch;
    }

    pInputSlice += uiInputDepthPitch;
    pOutputSlice += uiOutputDepthPitch;
  }
}

void wdImageUtils::CropImage(const wdImageView& input, const wdVec2I32& vOffset, const wdSizeU32& newsize, wdImage& out_output)
{
  WD_PROFILE_SCOPE("wdImageUtils::CropImage");

  WD_ASSERT_DEV(vOffset.x >= 0, "Offset is invalid");
  WD_ASSERT_DEV(vOffset.y >= 0, "Offset is invalid");
  WD_ASSERT_DEV(vOffset.x < (wdInt32)input.GetWidth(), "Offset is invalid");
  WD_ASSERT_DEV(vOffset.y < (wdInt32)input.GetHeight(), "Offset is invalid");

  const wdUInt32 uiNewWidth = wdMath::Min(vOffset.x + newsize.width, input.GetWidth()) - vOffset.x;
  const wdUInt32 uiNewHeight = wdMath::Min(vOffset.y + newsize.height, input.GetHeight()) - vOffset.y;

  wdImageHeader outputHeader;
  outputHeader.SetWidth(uiNewWidth);
  outputHeader.SetHeight(uiNewHeight);
  outputHeader.SetImageFormat(input.GetImageFormat());
  out_output.ResetAndAlloc(outputHeader);

  for (wdUInt32 y = 0; y < uiNewHeight; ++y)
  {
    for (wdUInt32 x = 0; x < uiNewWidth; ++x)
    {
      switch (input.GetImageFormat())
      {
        case wdImageFormat::R8G8B8A8_UNORM:
        case wdImageFormat::R8G8B8A8_UNORM_SRGB:
        case wdImageFormat::R8G8B8A8_UINT:
        case wdImageFormat::R8G8B8A8_SNORM:
        case wdImageFormat::R8G8B8A8_SINT:
        case wdImageFormat::B8G8R8A8_UNORM:
        case wdImageFormat::B8G8R8X8_UNORM:
        case wdImageFormat::B8G8R8A8_UNORM_SRGB:
        case wdImageFormat::B8G8R8X8_UNORM_SRGB:
          out_output.GetPixelPointer<wdUInt32>(0, 0, 0, x, y)[0] = input.GetPixelPointer<wdUInt32>(0, 0, 0, vOffset.x + x, vOffset.y + y)[0];
          break;

        case wdImageFormat::B8G8R8_UNORM:
          out_output.GetPixelPointer<wdUInt8>(0, 0, 0, x, y)[0] = input.GetPixelPointer<wdUInt8>(0, 0, 0, vOffset.x + x, vOffset.y + y)[0];
          out_output.GetPixelPointer<wdUInt8>(0, 0, 0, x, y)[1] = input.GetPixelPointer<wdUInt8>(0, 0, 0, vOffset.x + x, vOffset.y + y)[1];
          out_output.GetPixelPointer<wdUInt8>(0, 0, 0, x, y)[2] = input.GetPixelPointer<wdUInt8>(0, 0, 0, vOffset.x + x, vOffset.y + y)[2];
          break;

        default:
          WD_REPORT_FAILURE("The wdImageFormat {0} is not implemented", (wdUInt32)input.GetImageFormat());
          return;
      }
    }
  }
}

namespace
{
  template <typename T>
  void rotate180(T* pStart, T* pEnd)
  {
    pEnd = pEnd - 1;
    while (pStart < pEnd)
    {
      wdMath::Swap(*pStart, *pEnd);
      pStart++;
      pEnd--;
    }
  }
} // namespace

void wdImageUtils::RotateSubImage180(wdImage& inout_image, wdUInt32 uiMipLevel /*= 0*/, wdUInt32 uiFace /*= 0*/, wdUInt32 uiArrayIndex /*= 0*/)
{
  WD_PROFILE_SCOPE("wdImageUtils::RotateSubImage180");

  wdUInt8* start = inout_image.GetPixelPointer<wdUInt8>(uiMipLevel, uiFace, uiArrayIndex);
  wdUInt8* end = start + inout_image.GetDepthPitch(uiMipLevel);

  wdUInt32 bytesPerPixel = wdImageFormat::GetBitsPerPixel(inout_image.GetImageFormat()) / 8;

  switch (bytesPerPixel)
  {
    case 4:
      rotate180<wdUInt32>(reinterpret_cast<wdUInt32*>(start), reinterpret_cast<wdUInt32*>(end));
      break;
    case 12:
      rotate180<wdVec3>(reinterpret_cast<wdVec3*>(start), reinterpret_cast<wdVec3*>(end));
      break;
    case 16:
      rotate180<wdVec4>(reinterpret_cast<wdVec4*>(start), reinterpret_cast<wdVec4*>(end));
      break;
    default:
      // fallback version
      {
        end -= bytesPerPixel;
        while (start < end)
        {
          for (wdUInt32 i = 0; i < bytesPerPixel; i++)
          {
            wdMath::Swap(start[i], end[i]);
          }
          start += bytesPerPixel;
          end -= bytesPerPixel;
        }
      }
  }
}

wdResult wdImageUtils::Copy(const wdImageView& srcImg, const wdRectU32& srcRect, wdImage& inout_dstImg, const wdVec3U32& vDstOffset, wdUInt32 uiDstMipLevel /*= 0*/, wdUInt32 uiDstFace /*= 0*/, wdUInt32 uiDstArrayIndex /*= 0*/)
{
  if (inout_dstImg.GetImageFormat() != srcImg.GetImageFormat()) // Can only copy when the image formats are identical
    return WD_FAILURE;

  if (wdImageFormat::IsCompressed(inout_dstImg.GetImageFormat())) // Compressed formats are not supported
    return WD_FAILURE;

  WD_PROFILE_SCOPE("wdImageUtils::Copy");

  const wdUInt64 uiDstRowPitch = inout_dstImg.GetRowPitch(uiDstMipLevel);
  const wdUInt64 uiSrcRowPitch = srcImg.GetRowPitch(uiDstMipLevel);
  const wdUInt32 uiCopyBytesPerRow = wdImageFormat::GetBitsPerPixel(srcImg.GetImageFormat()) * srcRect.width / 8;

  wdUInt8* dstPtr = inout_dstImg.GetPixelPointer<wdUInt8>(uiDstMipLevel, uiDstFace, uiDstArrayIndex, vDstOffset.x, vDstOffset.y, vDstOffset.z);
  const wdUInt8* srcPtr = srcImg.GetPixelPointer<wdUInt8>(0, 0, 0, srcRect.x, srcRect.y);

  for (wdUInt32 y = 0; y < srcRect.height; y++)
  {
    wdMemoryUtils::Copy(dstPtr, srcPtr, uiCopyBytesPerRow);

    dstPtr += uiDstRowPitch;
    srcPtr += uiSrcRowPitch;
  }

  return WD_SUCCESS;
}

wdResult wdImageUtils::ExtractLowerMipChain(const wdImageView& srcImg, wdImage& ref_dstImg, wdUInt32 uiNumMips)
{
  const wdImageHeader& srcImgHeader = srcImg.GetHeader();

  if (srcImgHeader.GetNumFaces() != 1 || srcImgHeader.GetNumArrayIndices() != 1)
  {
    // Lower mips aren't stored contiguously for array/cube textures and would require copying. This isn't implemented yet.
    return WD_FAILURE;
  }

  WD_PROFILE_SCOPE("wdImageUtils::ExtractLowerMipChain");

  uiNumMips = wdMath::Min(uiNumMips, srcImgHeader.GetNumMipLevels());

  wdUInt32 startMipLevel = srcImgHeader.GetNumMipLevels() - uiNumMips;

  wdImageFormat::Enum format = srcImgHeader.GetImageFormat();

  if (wdImageFormat::RequiresFirstLevelBlockAlignment(format))
  {
    // Some block compressed image formats require resolutions that are divisible by block size,
    // therefore adjust startMipLevel accordingly
    while (srcImgHeader.GetWidth(startMipLevel) % wdImageFormat::GetBlockWidth(format) != 0 || srcImgHeader.GetHeight(startMipLevel) % wdImageFormat::GetBlockHeight(format) != 0)
    {
      if (uiNumMips >= srcImgHeader.GetNumMipLevels())
        return WD_FAILURE;

      if (startMipLevel == 0)
        return WD_FAILURE;

      ++uiNumMips;
      --startMipLevel;
    }
  }

  wdImageHeader dstImgHeader = srcImgHeader;
  dstImgHeader.SetWidth(srcImgHeader.GetWidth(startMipLevel));
  dstImgHeader.SetHeight(srcImgHeader.GetHeight(startMipLevel));
  dstImgHeader.SetDepth(srcImgHeader.GetDepth(startMipLevel));
  dstImgHeader.SetNumFaces(srcImgHeader.GetNumFaces());
  dstImgHeader.SetNumArrayIndices(srcImgHeader.GetNumArrayIndices());
  dstImgHeader.SetNumMipLevels(uiNumMips);

  const wdUInt8* pDataBegin = srcImg.GetPixelPointer<wdUInt8>(startMipLevel);
  const wdUInt8* pDataEnd = srcImg.GetByteBlobPtr().GetEndPtr();
  const ptrdiff_t dataSize = reinterpret_cast<ptrdiff_t>(pDataEnd) - reinterpret_cast<ptrdiff_t>(pDataBegin);

  const wdConstByteBlobPtr lowResData(pDataBegin, static_cast<wdUInt64>(dataSize));

  wdImageView dataview;
  dataview.ResetAndViewExternalStorage(dstImgHeader, lowResData);

  ref_dstImg.ResetAndCopy(dataview);

  return WD_SUCCESS;
}

wdUInt32 wdImageUtils::GetSampleIndex(wdUInt32 uiNumTexels, wdInt32 iIndex, wdImageAddressMode::Enum addressMode, bool& out_bUseBorderColor)
{
  out_bUseBorderColor = false;
  if (wdUInt32(iIndex) >= uiNumTexels)
  {
    switch (addressMode)
    {
      case wdImageAddressMode::Repeat:
        iIndex %= uiNumTexels;

        if (iIndex < 0)
        {
          iIndex += uiNumTexels;
        }
        return iIndex;

      case wdImageAddressMode::Mirror:
      {
        if (iIndex < 0)
        {
          iIndex = -iIndex - 1;
        }
        bool flip = (iIndex / uiNumTexels) & 1;
        iIndex %= uiNumTexels;
        if (flip)
        {
          iIndex = uiNumTexels - iIndex - 1;
        }
        return iIndex;
      }

      case wdImageAddressMode::Clamp:
        return wdMath::Clamp<wdInt32>(iIndex, 0, uiNumTexels - 1);

      case wdImageAddressMode::ClampBorder:
        out_bUseBorderColor = true;
        return 0;

      default:
        WD_ASSERT_NOT_IMPLEMENTED
        return 0;
    }
  }
  return iIndex;
}

static wdSimdVec4f LoadSample(const wdSimdVec4f* pSource, wdUInt32 uiNumSourceElements, wdUInt32 uiStride, wdInt32 iIndex, wdImageAddressMode::Enum addressMode, const wdSimdVec4f& vBorderColor)
{
  bool useBorderColor = false;
  // result is in the range [-(w-1), (w-1)], bring it to [0, w - 1]
  iIndex = wdImageUtils::GetSampleIndex(uiNumSourceElements, iIndex, addressMode, useBorderColor);
  if (useBorderColor)
  {
    return vBorderColor;
  }
  return pSource[iIndex * uiStride];
}

inline static void FilterLine(
  wdUInt32 uiNumSourceElements, const wdSimdVec4f* __restrict pSourceBegin, wdSimdVec4f* __restrict pTargetBegin, wdUInt32 uiStride, const wdImageFilterWeights& weights, wdArrayPtr<const wdInt32> firstSampleIndices, wdImageAddressMode::Enum addressMode, const wdSimdVec4f& vBorderColor)
{
  // Convolve the image using the precomputed weights
  const wdUInt32 numWeights = weights.GetNumWeights();

  // When the first source index for the output is between 0 and this value,
  // we can fetch all numWeights inputs without taking addressMode into consideration,
  // which makes the inner loop a lot faster.
  const wdInt32 trivialSourceIndicesEnd = static_cast<wdInt32>(uiNumSourceElements) - static_cast<wdInt32>(numWeights);
  const auto weightsView = weights.ViewWeights();
  const float* __restrict nextWeightPtr = weightsView.GetPtr();
  WD_ASSERT_DEBUG((static_cast<wdUInt32>(weightsView.GetCount()) % numWeights) == 0, "");
  for (wdInt32 firstSourceIdx : firstSampleIndices)
  {
    wdSimdVec4f total(0.0f, 0.0f, 0.0f, 0.0f);

    if (firstSourceIdx >= 0 && firstSourceIdx < trivialSourceIndicesEnd)
    {
      const auto* __restrict sourcePtr = pSourceBegin + firstSourceIdx * uiStride;
      for (wdUInt32 weightIdx = 0; weightIdx < numWeights; ++weightIdx)
      {
        total = wdSimdVec4f::MulAdd(*sourcePtr, wdSimdVec4f(*nextWeightPtr++), total);
        sourcePtr += uiStride;
      }
    }
    else
    {
      // Very slow fallback case that respects the addressMode
      // (not a lot of pixels are taking this path, so it's probably fine)
      wdInt32 sourceIdx = firstSourceIdx;
      for (wdUInt32 weightIdx = 0; weightIdx < numWeights; ++weightIdx)
      {
        total = wdSimdVec4f::MulAdd(LoadSample(pSourceBegin, uiNumSourceElements, uiStride, sourceIdx, addressMode, vBorderColor), wdSimdVec4f(*nextWeightPtr++), total);
        sourceIdx++;
      }
    }
    // It's ok to check this once per source index, see the assert above
    // (number of weights in weightsView is divisible by numWeights)
    if (nextWeightPtr == weightsView.GetEndPtr())
    {
      nextWeightPtr = weightsView.GetPtr();
    }
    *pTargetBegin = total;
    pTargetBegin += uiStride;
  }
}

static void DownScaleFastLine(wdUInt32 uiPixelStride, const wdUInt8* pSrc, wdUInt8* pDest, wdUInt32 uiLengthIn, wdUInt32 uiStrideIn, wdUInt32 uiLengthOut, wdUInt32 uiStrideOut)
{
  const wdUInt32 downScaleFactor = uiLengthIn / uiLengthOut;

  const wdUInt32 downScaleFactorLog2 = wdMath::Log2i(static_cast<wdUInt32>(downScaleFactor));
  const wdUInt32 roundOffset = downScaleFactor / 2;

  for (wdUInt32 offset = 0; offset < uiLengthOut; ++offset)
  {
    for (wdUInt32 channel = 0; channel < uiPixelStride; ++channel)
    {
      const wdUInt32 destOffset = offset * uiStrideOut + channel;

      wdUInt32 curChannel = roundOffset;
      for (wdUInt32 index = 0; index < downScaleFactor; ++index)
      {
        curChannel += static_cast<wdUInt32>(pSrc[channel + index * uiStrideIn]);
      }

      curChannel = curChannel >> downScaleFactorLog2;
      pDest[destOffset] = static_cast<wdUInt8>(curChannel);
    }

    pSrc += downScaleFactor * uiStrideIn;
  }
}

static void DownScaleFast(const wdImageView& image, wdImage& out_result, wdUInt32 uiWidth, wdUInt32 uiHeight)
{
  wdImageFormat::Enum format = image.GetImageFormat();

  wdUInt32 originalWidth = image.GetWidth();
  wdUInt32 originalHeight = image.GetHeight();
  wdUInt32 numArrayElements = image.GetNumArrayIndices();
  wdUInt32 numFaces = image.GetNumFaces();

  wdUInt32 pixelStride = wdImageFormat::GetBitsPerPixel(format) / 8;

  wdImageHeader intermediateHeader;
  intermediateHeader.SetWidth(uiWidth);
  intermediateHeader.SetHeight(originalHeight);
  intermediateHeader.SetNumArrayIndices(numArrayElements);
  intermediateHeader.SetNumFaces(numFaces);
  intermediateHeader.SetImageFormat(format);

  wdImage intermediate;
  intermediate.ResetAndAlloc(intermediateHeader);

  for (wdUInt32 arrayIndex = 0; arrayIndex < numArrayElements; arrayIndex++)
  {
    for (wdUInt32 face = 0; face < numFaces; face++)
    {
      for (wdUInt32 row = 0; row < originalHeight; row++)
      {
        DownScaleFastLine(pixelStride, image.GetPixelPointer<wdUInt8>(0, face, arrayIndex, 0, row), intermediate.GetPixelPointer<wdUInt8>(0, face, arrayIndex, 0, row), originalWidth, pixelStride, uiWidth, pixelStride);
      }
    }
  }

  // input and output images may be the same, so we can't access the original image below this point

  wdImageHeader outHeader;
  outHeader.SetWidth(uiWidth);
  outHeader.SetHeight(uiHeight);
  outHeader.SetNumArrayIndices(numArrayElements);
  outHeader.SetNumArrayIndices(numFaces);
  outHeader.SetImageFormat(format);

  out_result.ResetAndAlloc(outHeader);

  WD_ASSERT_DEBUG(intermediate.GetRowPitch() < wdMath::MaxValue<wdUInt32>(), "Row pitch exceeds wdUInt32 max value.");
  WD_ASSERT_DEBUG(out_result.GetRowPitch() < wdMath::MaxValue<wdUInt32>(), "Row pitch exceeds wdUInt32 max value.");

  for (wdUInt32 arrayIndex = 0; arrayIndex < numArrayElements; arrayIndex++)
  {
    for (wdUInt32 face = 0; face < numFaces; face++)
    {
      for (wdUInt32 col = 0; col < uiWidth; col++)
      {
        DownScaleFastLine(pixelStride, intermediate.GetPixelPointer<wdUInt8>(0, face, arrayIndex, col), out_result.GetPixelPointer<wdUInt8>(0, face, arrayIndex, col), originalHeight, static_cast<wdUInt32>(intermediate.GetRowPitch()), uiHeight, static_cast<wdUInt32>(out_result.GetRowPitch()));
      }
    }
  }
}

static float EvaluateAverageCoverage(wdBlobPtr<const wdColor> colors, float fAlphaThreshold)
{
  WD_PROFILE_SCOPE("EvaluateAverageCoverage");

  wdUInt64 totalPixels = colors.GetCount();
  wdUInt64 count = 0;
  for (wdUInt32 idx = 0; idx < totalPixels; ++idx)
  {
    count += colors[idx].a >= fAlphaThreshold;
  }

  return float(count) / float(totalPixels);
}

static void NormalizeCoverage(wdBlobPtr<wdColor> colors, float fAlphaThreshold, float fTargetCoverage)
{
  WD_PROFILE_SCOPE("NormalizeCoverage");

  // Based on the idea in http://the-witness.net/news/2010/09/computing-alpha-mipmaps/. Note we're using a histogram
  // to find the new alpha threshold here rather than bisecting.

  // Generate histogram of alpha values
  wdUInt64 totalPixels = colors.GetCount();
  wdUInt32 alphaHistogram[256] = {};
  for (wdUInt64 idx = 0; idx < totalPixels; ++idx)
  {
    alphaHistogram[wdMath::ColorFloatToByte(colors[idx].a)]++;
  }

  // Find range of alpha thresholds so the number of covered pixels matches by summing up the histogram
  wdInt32 targetCount = wdInt32(fTargetCoverage * totalPixels);
  wdInt32 coverageCount = 0;
  wdInt32 maxThreshold = 255;
  for (; maxThreshold >= 0; maxThreshold--)
  {
    coverageCount += alphaHistogram[maxThreshold];

    if (coverageCount >= targetCount)
    {
      break;
    }
  }

  coverageCount = targetCount;
  wdInt32 minThreshold = 0;
  for (; minThreshold < 256; minThreshold++)
  {
    coverageCount -= alphaHistogram[maxThreshold];

    if (coverageCount <= targetCount)
    {
      break;
    }
  }

  wdInt32 currentThreshold = wdMath::ColorFloatToByte(fAlphaThreshold);

  // Each of the alpha test thresholds in the range [minThreshold; maxThreshold] will result in the same coverage. Pick a new threshold
  // close to the old one so we scale by the smallest necessary amount.
  wdInt32 newThreshold;
  if (currentThreshold < minThreshold)
  {
    newThreshold = minThreshold;
  }
  else if (currentThreshold > maxThreshold)
  {
    newThreshold = maxThreshold;
  }
  else
  {
    // Avoid rescaling altogether if the current threshold already preserves coverage
    return;
  }

  // Rescale alpha values
  float alphaScale = fAlphaThreshold / (newThreshold / 255.0f);
  for (wdUInt64 idx = 0; idx < totalPixels; ++idx)
  {
    colors[idx].a *= alphaScale;
  }
}


wdResult wdImageUtils::Scale(const wdImageView& source, wdImage& ref_target, wdUInt32 uiWidth, wdUInt32 uiHeight, const wdImageFilter* pFilter, wdImageAddressMode::Enum addressModeU, wdImageAddressMode::Enum addressModeV, const wdColor& borderColor)
{
  return Scale3D(source, ref_target, uiWidth, uiHeight, 1, pFilter, addressModeU, addressModeV, wdImageAddressMode::Clamp, borderColor);
}

wdResult wdImageUtils::Scale3D(const wdImageView& source, wdImage& ref_target, wdUInt32 uiWidth, wdUInt32 uiHeight, wdUInt32 uiDepth, const wdImageFilter* pFilter /*= wd_NULL*/, wdImageAddressMode::Enum addressModeU /*= wdImageAddressMode::Clamp*/,
  wdImageAddressMode::Enum addressModeV /*= wdImageAddressMode::Clamp*/, wdImageAddressMode::Enum addressModeW /*= wdImageAddressMode::Clamp*/, const wdColor& borderColor /*= wdColors::Black*/)
{
  WD_PROFILE_SCOPE("wdImageUtils::Scale3D");

  if (uiWidth == 0 || uiHeight == 0 || uiDepth == 0)
  {
    wdImageHeader header;
    header.SetImageFormat(source.GetImageFormat());
    ref_target.ResetAndAlloc(header);
    return WD_SUCCESS;
  }

  const wdImageFormat::Enum format = source.GetImageFormat();

  const wdUInt32 originalWidth = source.GetWidth();
  const wdUInt32 originalHeight = source.GetHeight();
  const wdUInt32 originalDepth = source.GetDepth();
  const wdUInt32 numFaces = source.GetNumFaces();
  const wdUInt32 numArrayElements = source.GetNumArrayIndices();

  if (originalWidth == uiWidth && originalHeight == uiHeight && originalDepth == uiDepth)
  {
    ref_target.ResetAndCopy(source);
    return WD_SUCCESS;
  }

  // Scaling down by an even factor?
  const wdUInt32 downScaleFactorX = originalWidth / uiWidth;
  const wdUInt32 downScaleFactorY = originalHeight / uiHeight;

  if (pFilter == nullptr && (format == wdImageFormat::R8G8B8A8_UNORM || format == wdImageFormat::B8G8R8A8_UNORM || format == wdImageFormat::B8G8R8_UNORM) && downScaleFactorX * uiWidth == originalWidth && downScaleFactorY * uiHeight == originalHeight && uiDepth == 1 && originalDepth == 1 &&
      wdMath::IsPowerOf2(downScaleFactorX) && wdMath::IsPowerOf2(downScaleFactorY))
  {
    DownScaleFast(source, ref_target, uiWidth, uiHeight);
    return WD_SUCCESS;
  }

  // Fallback to default filter
  wdImageFilterTriangle defaultFilter;
  if (!pFilter)
  {
    pFilter = &defaultFilter;
  }

  const wdImageView* stepSource;

  // Manage scratch images for intermediate conversion or filtering
  const wdUInt32 maxNumScratchImages = 2;
  wdImage scratch[maxNumScratchImages];
  bool scratchUsed[maxNumScratchImages] = {};
  auto allocateScratch = [&]() -> wdImage&
  {
    for (wdUInt32 i = 0;; ++i)
    {
      WD_ASSERT_DEV(i < maxNumScratchImages, "Failed to allocate scratch image");
      if (!scratchUsed[i])
      {
        scratchUsed[i] = true;
        return scratch[i];
      }
    }
  };
  auto releaseScratch = [&](const wdImageView& image)
  {
    for (wdUInt32 i = 0; i < maxNumScratchImages; ++i)
    {
      if (&scratch[i] == &image)
      {
        scratchUsed[i] = false;
        return;
      }
    }
  };

  if (format == wdImageFormat::R32G32B32A32_FLOAT)
  {
    stepSource = &source;
  }
  else
  {
    wdImage& conversionScratch = allocateScratch();
    if (wdImageConversion::Convert(source, conversionScratch, wdImageFormat::R32G32B32A32_FLOAT).Failed())
    {
      return WD_FAILURE;
    }

    stepSource = &conversionScratch;
  };

  wdHybridArray<wdInt32, 256> firstSampleIndices;
  firstSampleIndices.Reserve(wdMath::Max(uiWidth, uiHeight, uiDepth));

  if (uiWidth != originalWidth)
  {
    wdImageFilterWeights weights(*pFilter, originalWidth, uiWidth);
    firstSampleIndices.SetCountUninitialized(uiWidth);
    for (wdUInt32 x = 0; x < uiWidth; ++x)
    {
      firstSampleIndices[x] = weights.GetFirstSourceSampleIndex(x);
    }

    wdImage* stepTarget;
    if (uiHeight == originalHeight && uiDepth == originalDepth && format == wdImageFormat::R32G32B32A32_FLOAT)
    {
      stepTarget = &ref_target;
    }
    else
    {
      stepTarget = &allocateScratch();
    }

    wdImageHeader stepHeader = stepSource->GetHeader();
    stepHeader.SetWidth(uiWidth);
    stepTarget->ResetAndAlloc(stepHeader);

    for (wdUInt32 arrayIndex = 0; arrayIndex < numArrayElements; ++arrayIndex)
    {
      for (wdUInt32 face = 0; face < numFaces; ++face)
      {
        for (wdUInt32 z = 0; z < originalDepth; ++z)
        {
          for (wdUInt32 y = 0; y < originalHeight; ++y)
          {
            const wdSimdVec4f* filterSource = stepSource->GetPixelPointer<wdSimdVec4f>(0, face, arrayIndex, 0, y, z);
            wdSimdVec4f* filterTarget = stepTarget->GetPixelPointer<wdSimdVec4f>(0, face, arrayIndex, 0, y, z);
            FilterLine(originalWidth, filterSource, filterTarget, 1, weights, firstSampleIndices, addressModeU, wdSimdVec4f(borderColor.r, borderColor.g, borderColor.b, borderColor.a));
          }
        }
      }
    }

    releaseScratch(*stepSource);
    stepSource = stepTarget;
  }

  if (uiHeight != originalHeight)
  {
    wdImageFilterWeights weights(*pFilter, originalHeight, uiHeight);
    firstSampleIndices.SetCount(uiHeight);
    for (wdUInt32 y = 0; y < uiHeight; ++y)
    {
      firstSampleIndices[y] = weights.GetFirstSourceSampleIndex(y);
    }

    wdImage* stepTarget;
    if (uiDepth == originalDepth && format == wdImageFormat::R32G32B32A32_FLOAT)
    {
      stepTarget = &ref_target;
    }
    else
    {
      stepTarget = &allocateScratch();
    }

    wdImageHeader stepHeader = stepSource->GetHeader();
    stepHeader.SetHeight(uiHeight);
    stepTarget->ResetAndAlloc(stepHeader);

    for (wdUInt32 arrayIndex = 0; arrayIndex < numArrayElements; ++arrayIndex)
    {
      for (wdUInt32 face = 0; face < numFaces; ++face)
      {
        for (wdUInt32 z = 0; z < originalDepth; ++z)
        {
          for (wdUInt32 x = 0; x < uiWidth; ++x)
          {
            const wdSimdVec4f* filterSource = stepSource->GetPixelPointer<wdSimdVec4f>(0, face, arrayIndex, x, 0, z);
            wdSimdVec4f* filterTarget = stepTarget->GetPixelPointer<wdSimdVec4f>(0, face, arrayIndex, x, 0, z);
            FilterLine(originalHeight, filterSource, filterTarget, uiWidth, weights, firstSampleIndices, addressModeV, wdSimdVec4f(borderColor.r, borderColor.g, borderColor.b, borderColor.a));
          }
        }
      }
    }

    releaseScratch(*stepSource);
    stepSource = stepTarget;
  }

  if (uiDepth != originalDepth)
  {
    wdImageFilterWeights weights(*pFilter, originalDepth, uiDepth);
    firstSampleIndices.SetCount(uiDepth);
    for (wdUInt32 z = 0; z < uiDepth; ++z)
    {
      firstSampleIndices[z] = weights.GetFirstSourceSampleIndex(z);
    }

    wdImage* stepTarget;
    if (format == wdImageFormat::R32G32B32A32_FLOAT)
    {
      stepTarget = &ref_target;
    }
    else
    {
      stepTarget = &allocateScratch();
    }

    wdImageHeader stepHeader = stepSource->GetHeader();
    stepHeader.SetDepth(uiDepth);
    stepTarget->ResetAndAlloc(stepHeader);

    for (wdUInt32 arrayIndex = 0; arrayIndex < numArrayElements; ++arrayIndex)
    {
      for (wdUInt32 face = 0; face < numFaces; ++face)
      {
        for (wdUInt32 y = 0; y < uiHeight; ++y)
        {
          for (wdUInt32 x = 0; x < uiWidth; ++x)
          {
            const wdSimdVec4f* filterSource = stepSource->GetPixelPointer<wdSimdVec4f>(0, face, arrayIndex, x, y, 0);
            wdSimdVec4f* filterTarget = stepTarget->GetPixelPointer<wdSimdVec4f>(0, face, arrayIndex, x, y, 0);
            FilterLine(originalHeight, filterSource, filterTarget, uiWidth * uiHeight, weights, firstSampleIndices, addressModeW, wdSimdVec4f(borderColor.r, borderColor.g, borderColor.b, borderColor.a));
          }
        }
      }
    }

    releaseScratch(*stepSource);
    stepSource = stepTarget;
  }

  // Convert back to original format - no-op if stepSource and target are the same
  return wdImageConversion::Convert(*stepSource, ref_target, format);
}

void wdImageUtils::GenerateMipMaps(const wdImageView& source, wdImage& ref_target, const MipMapOptions& options)
{
  WD_PROFILE_SCOPE("wdImageUtils::GenerateMipMaps");

  wdImageHeader header = source.GetHeader();
  WD_ASSERT_DEV(header.GetImageFormat() == wdImageFormat::R32G32B32A32_FLOAT, "The source image must be a RGBA 32-bit float format.");
  WD_ASSERT_DEV(&source != &ref_target, "Source and target must not be the same image.");

  // Make a local copy to be able to tweak some of the options
  wdImageUtils::MipMapOptions mipMapOptions = options;

  // alpha thresholds with extreme values are not supported at the moment
  mipMapOptions.m_alphaThreshold = wdMath::Clamp(mipMapOptions.m_alphaThreshold, 0.05f, 0.95f);

  // Enforce CLAMP addressing mode for cubemaps
  if (source.GetNumFaces() == 6)
  {
    mipMapOptions.m_addressModeU = wdImageAddressMode::Clamp;
    mipMapOptions.m_addressModeV = wdImageAddressMode::Clamp;
  }

  wdUInt32 numMipMaps = header.ComputeNumberOfMipMaps();
  if (mipMapOptions.m_numMipMaps > 0 && mipMapOptions.m_numMipMaps < numMipMaps)
  {
    numMipMaps = mipMapOptions.m_numMipMaps;
  }
  header.SetNumMipLevels(numMipMaps);

  ref_target.ResetAndAlloc(header);

  for (wdUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (wdUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      wdImageHeader currentMipMapHeader = header;
      currentMipMapHeader.SetNumMipLevels(1);
      currentMipMapHeader.SetNumFaces(1);
      currentMipMapHeader.SetNumArrayIndices(1);

      auto sourceView = source.GetSubImageView(0, face, arrayIndex).GetByteBlobPtr();
      auto targetView = ref_target.GetSubImageView(0, face, arrayIndex).GetByteBlobPtr();

      memcpy(targetView.GetPtr(), sourceView.GetPtr(), static_cast<size_t>(targetView.GetCount()));

      float targetCoverage = 0.0f;
      if (mipMapOptions.m_preserveCoverage)
      {
        targetCoverage = EvaluateAverageCoverage(source.GetSubImageView(0, face, arrayIndex).GetBlobPtr<wdColor>(), mipMapOptions.m_alphaThreshold);
      }

      for (wdUInt32 mipMapLevel = 0; mipMapLevel < numMipMaps - 1; mipMapLevel++)
      {
        wdImageHeader nextMipMapHeader = currentMipMapHeader;
        nextMipMapHeader.SetWidth(wdMath::Max(1u, nextMipMapHeader.GetWidth() / 2));
        nextMipMapHeader.SetHeight(wdMath::Max(1u, nextMipMapHeader.GetHeight() / 2));
        nextMipMapHeader.SetDepth(wdMath::Max(1u, nextMipMapHeader.GetDepth() / 2));

        auto sourceData = ref_target.GetSubImageView(mipMapLevel, face, arrayIndex).GetByteBlobPtr();
        wdImage currentMipMap;
        currentMipMap.ResetAndUseExternalStorage(currentMipMapHeader, sourceData);

        auto dstData = ref_target.GetSubImageView(mipMapLevel + 1, face, arrayIndex).GetByteBlobPtr();
        wdImage nextMipMap;
        nextMipMap.ResetAndUseExternalStorage(nextMipMapHeader, dstData);

        wdImageUtils::Scale3D(currentMipMap, nextMipMap, nextMipMapHeader.GetWidth(), nextMipMapHeader.GetHeight(), nextMipMapHeader.GetDepth(), mipMapOptions.m_filter, mipMapOptions.m_addressModeU, mipMapOptions.m_addressModeV, mipMapOptions.m_addressModeW, mipMapOptions.m_borderColor)
          .IgnoreResult();

        if (mipMapOptions.m_preserveCoverage)
        {
          NormalizeCoverage(nextMipMap.GetBlobPtr<wdColor>(), mipMapOptions.m_alphaThreshold, targetCoverage);
        }

        if (mipMapOptions.m_renormalizeNormals)
        {
          RenormalizeNormalMap(nextMipMap);
        }

        currentMipMapHeader = nextMipMapHeader;
      }
    }
  }
}

void wdImageUtils::ReconstructNormalZ(wdImage& ref_image)
{
  WD_PROFILE_SCOPE("wdImageUtils::ReconstructNormalZ");

  WD_ASSERT_DEV(ref_image.GetImageFormat() == wdImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");

  wdSimdVec4f* cur = ref_image.GetBlobPtr<wdSimdVec4f>().GetPtr();
  wdSimdVec4f* const end = ref_image.GetBlobPtr<wdSimdVec4f>().GetEndPtr();

  wdSimdFloat oneScalar = 1.0f;

  wdSimdVec4f two(2.0f);

  wdSimdVec4f minusOne(-1.0f);

  wdSimdVec4f half(0.5f);

  for (; cur < end; cur++)
  {
    wdSimdVec4f normal;
    // unpack from [0,1] to [-1, 1]
    normal = wdSimdVec4f::MulAdd(*cur, two, minusOne);

    // compute Z component
    normal.SetZ((oneScalar - normal.Dot<2>(normal)).GetSqrt());

    // pack back to [0,1]
    *cur = wdSimdVec4f::MulAdd(half, normal, half);
  }
}

void wdImageUtils::RenormalizeNormalMap(wdImage& ref_image)
{
  WD_PROFILE_SCOPE("wdImageUtils::RenormalizeNormalMap");

  WD_ASSERT_DEV(ref_image.GetImageFormat() == wdImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");

  wdSimdVec4f* start = ref_image.GetBlobPtr<wdSimdVec4f>().GetPtr();
  wdSimdVec4f* const end = ref_image.GetBlobPtr<wdSimdVec4f>().GetEndPtr();

  wdSimdVec4f two(2.0f);

  wdSimdVec4f minusOne(-1.0f);

  wdSimdVec4f half(0.5f);

  for (; start < end; start++)
  {
    wdSimdVec4f normal;
    normal = wdSimdVec4f::MulAdd(*start, two, minusOne);
    normal.Normalize<3>();
    *start = wdSimdVec4f::MulAdd(half, normal, half);
  }
}

void wdImageUtils::AdjustRoughness(wdImage& ref_roughnessMap, const wdImageView& normalMap)
{
  WD_PROFILE_SCOPE("wdImageUtils::AdjustRoughness");

  WD_ASSERT_DEV(ref_roughnessMap.GetImageFormat() == wdImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");
  WD_ASSERT_DEV(normalMap.GetImageFormat() == wdImageFormat::R32G32B32A32_FLOAT, "This algorithm currently expects a RGBA 32 Float as input");

  WD_ASSERT_DEV(ref_roughnessMap.GetWidth() >= normalMap.GetWidth() && ref_roughnessMap.GetHeight() >= normalMap.GetHeight(), "The roughness map needs to be bigger or same size than the normal map.");

  wdImage filteredNormalMap;
  wdImageUtils::MipMapOptions options;

  // Box filter normal map without re-normalization so we have the average normal length in each mip map.
  if (ref_roughnessMap.GetWidth() != normalMap.GetWidth() || ref_roughnessMap.GetHeight() != normalMap.GetHeight())
  {
    wdImage temp;
    wdImageUtils::Scale(normalMap, temp, ref_roughnessMap.GetWidth(), ref_roughnessMap.GetHeight()).IgnoreResult();
    wdImageUtils::RenormalizeNormalMap(temp);
    wdImageUtils::GenerateMipMaps(temp, filteredNormalMap, options);
  }
  else
  {
    wdImageUtils::GenerateMipMaps(normalMap, filteredNormalMap, options);
  }

  WD_ASSERT_DEV(ref_roughnessMap.GetNumMipLevels() == filteredNormalMap.GetNumMipLevels(), "Roughness and normal map must have the same number of mip maps");

  wdSimdVec4f two(2.0f);
  wdSimdVec4f minusOne(-1.0f);

  wdUInt32 numMipLevels = ref_roughnessMap.GetNumMipLevels();
  for (wdUInt32 mipLevel = 1; mipLevel < numMipLevels; ++mipLevel)
  {
    wdBlobPtr<wdSimdVec4f> roughnessData = ref_roughnessMap.GetSubImageView(mipLevel, 0, 0).GetBlobPtr<wdSimdVec4f>();
    wdBlobPtr<wdSimdVec4f> normalData = filteredNormalMap.GetSubImageView(mipLevel, 0, 0).GetBlobPtr<wdSimdVec4f>();

    for (wdUInt64 i = 0; i < roughnessData.GetCount(); ++i)
    {
      wdSimdVec4f normal = wdSimdVec4f::MulAdd(normalData[i], two, minusOne);

      float avgNormalLength = normal.GetLength<3>();
      if (avgNormalLength < 1.0f)
      {
        float avgNormalLengthSquare = avgNormalLength * avgNormalLength;
        float kappa = (3.0f * avgNormalLength - avgNormalLength * avgNormalLengthSquare) / (1.0f - avgNormalLengthSquare);
        float variance = 1.0f / (2.0f * kappa);

        float oldRoughness = roughnessData[i].GetComponent<0>();
        float newRoughness = wdMath::Sqrt(oldRoughness * oldRoughness + variance);

        roughnessData[i].Set(newRoughness);
      }
    }
  }
}

void wdImageUtils::ChangeExposure(wdImage& ref_image, float fBias)
{
  WD_ASSERT_DEV(ref_image.GetImageFormat() == wdImageFormat::R32G32B32A32_FLOAT, "This function expects an RGBA 32 float image as input");

  if (fBias == 0.0f)
    return;

  WD_PROFILE_SCOPE("wdImageUtils::ChangeExposure");

  const float multiplier = wdMath::Pow2(fBias);

  for (wdColor& col : ref_image.GetBlobPtr<wdColor>())
  {
    col = multiplier * col;
  }
}

static wdResult CopyImageRectToFace(wdImage& ref_dstImg, const wdImageView& srcImg, wdUInt32 uiOffsetX, wdUInt32 uiOffsetY, wdUInt32 uiFaceIndex)
{
  wdRectU32 r;
  r.x = uiOffsetX;
  r.y = uiOffsetY;
  r.width = ref_dstImg.GetWidth();
  r.height = r.width;

  return wdImageUtils::Copy(srcImg, r, ref_dstImg, wdVec3U32(0), 0, uiFaceIndex);
}

wdResult wdImageUtils::CreateCubemapFromSingleFile(wdImage& ref_dstImg, const wdImageView& srcImg)
{
  WD_PROFILE_SCOPE("wdImageUtils::CreateCubemapFromSingleFile");

  if (srcImg.GetNumFaces() == 6)
  {
    ref_dstImg.ResetAndCopy(srcImg);
    return WD_SUCCESS;
  }
  else if (srcImg.GetNumFaces() == 1)
  {
    if (srcImg.GetWidth() % 3 == 0 && srcImg.GetHeight() % 4 == 0 && srcImg.GetWidth() / 3 == srcImg.GetHeight() / 4)
    {
      // Vertical cube map layout
      //     +---+
      //     | Y+|
      // +---+---+---+
      // | X-| Z+| X+|
      // +---+---+---+
      //     | Y-|
      //     +---+
      //     | Z-|
      //     +---+
      const wdUInt32 faceSize = srcImg.GetWidth() / 3;

      wdImageHeader imgHeader;
      imgHeader.SetWidth(faceSize);
      imgHeader.SetHeight(faceSize);
      imgHeader.SetImageFormat(srcImg.GetImageFormat());
      imgHeader.SetDepth(1);
      imgHeader.SetNumFaces(6);
      imgHeader.SetNumMipLevels(1);
      imgHeader.SetNumArrayIndices(1);

      ref_dstImg.ResetAndAlloc(imgHeader);

      // face order in dds files is: positive x, negative x, positive y, negative y, positive z, negative z

      // Positive X face
      WD_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize * 2, faceSize, 0));

      // Negative X face
      WD_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, 0, faceSize, 1));

      // Positive Y face
      WD_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, 0, 2));

      // Negative Y face
      WD_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize * 2, 3));

      // Positive Z face
      WD_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize, 4));

      // Negative Z face
      WD_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize * 3, 5));
      wdImageUtils::RotateSubImage180(ref_dstImg, 0, 5);
    }
    else if (srcImg.GetWidth() % 4 == 0 && srcImg.GetHeight() % 3 == 0 && srcImg.GetWidth() / 4 == srcImg.GetHeight() / 3)
    {
      // Horizontal cube map layout
      //     +---+
      //     | Y+|
      // +---+---+---+---+
      // | X-| Z+| X+| Z-|
      // +---+---+---+---+
      //     | Y-|
      //     +---+
      const wdUInt32 faceSize = srcImg.GetWidth() / 4;

      wdImageHeader imgHeader;
      imgHeader.SetWidth(faceSize);
      imgHeader.SetHeight(faceSize);
      imgHeader.SetImageFormat(srcImg.GetImageFormat());
      imgHeader.SetDepth(1);
      imgHeader.SetNumFaces(6);
      imgHeader.SetNumMipLevels(1);
      imgHeader.SetNumArrayIndices(1);

      ref_dstImg.ResetAndAlloc(imgHeader);

      // face order in dds files is: positive x, negative x, positive y, negative y, positive z, negative z

      // Positive X face
      WD_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize * 2, faceSize, 0));

      // Negative X face
      WD_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, 0, faceSize, 1));

      // Positive Y face
      WD_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, 0, 2));

      // Negative Y face
      WD_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize * 2, 3));

      // Positive Z face
      WD_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize, faceSize, 4));

      // Negative Z face
      WD_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, srcImg, faceSize * 3, faceSize, 5));
    }
    else
    {
      // Spherical mapping
      if (srcImg.GetWidth() % 4 != 0)
      {
        wdLog::Error("Width of the input image should be a multiple of 4");
        return WD_FAILURE;
      }

      const wdUInt32 faceSize = srcImg.GetWidth() / 4;

      wdImageHeader imgHeader;
      imgHeader.SetWidth(faceSize);
      imgHeader.SetHeight(faceSize);
      imgHeader.SetImageFormat(srcImg.GetImageFormat());
      imgHeader.SetDepth(1);
      imgHeader.SetNumFaces(6);
      imgHeader.SetNumMipLevels(1);
      imgHeader.SetNumArrayIndices(1);

      ref_dstImg.ResetAndAlloc(imgHeader);

      // Corners of the UV space for the respective faces in model space
      const wdVec3 faceCorners[] = {
        wdVec3(0.5, 0.5, 0.5),   // X+
        wdVec3(-0.5, 0.5, -0.5), // X-
        wdVec3(-0.5, 0.5, -0.5), // Y+
        wdVec3(-0.5, -0.5, 0.5), // Y-
        wdVec3(-0.5, 0.5, 0.5),  // Z+
        wdVec3(0.5, 0.5, -0.5)   // Z-
      };

      // UV Axis of the respective faces in model space
      const wdVec3 faceAxis[] = {
        wdVec3(0, 0, -1), wdVec3(0, -1, 0), // X+
        wdVec3(0, 0, 1), wdVec3(0, -1, 0),  // X-
        wdVec3(1, 0, 0), wdVec3(0, 0, 1),   // Y+
        wdVec3(1, 0, 0), wdVec3(0, 0, -1),  // Y-
        wdVec3(1, 0, 0), wdVec3(0, -1, 0),  // Z+
        wdVec3(-1, 0, 0), wdVec3(0, -1, 0)  // Z-
      };

      const float fFaceSize = (float)faceSize;
      const float fHalfPixel = 0.5f / fFaceSize;
      const float fPixel = 1.0f / fFaceSize;

      const float fHalfSrcWidth = srcImg.GetWidth() / 2.0f;
      const float fSrcHeight = (float)srcImg.GetHeight();

      const wdUInt32 srcWidthMinus1 = srcImg.GetWidth() - 1;
      const wdUInt32 srcHeightMinus1 = srcImg.GetHeight() - 1;

      WD_ASSERT_DEBUG(srcImg.GetRowPitch() % sizeof(wdColor) == 0, "Row pitch should be a multiple of sizeof(wdColor)");
      const wdUInt64 srcRowPitch = srcImg.GetRowPitch() / sizeof(wdColor);

      WD_ASSERT_DEBUG(ref_dstImg.GetRowPitch() % sizeof(wdColor) == 0, "Row pitch should be a multiple of sizeof(wdColor)");
      const wdUInt64 faceRowPitch = ref_dstImg.GetRowPitch() / sizeof(wdColor);

      const wdColor* srcData = srcImg.GetPixelPointer<wdColor>();
      const float InvPi = 1.0f / wdMath::Pi<float>();

      for (wdUInt32 faceIndex = 0; faceIndex < 6; faceIndex++)
      {
        wdColor* faceData = ref_dstImg.GetPixelPointer<wdColor>(0, faceIndex);
        for (wdUInt32 y = 0; y < faceSize; y++)
        {
          const float dstV = (float)y * fPixel + fHalfPixel;

          for (wdUInt32 x = 0; x < faceSize; x++)
          {
            const float dstU = (float)x * fPixel + fHalfPixel;
            const wdVec3 modelSpacePos = faceCorners[faceIndex] + dstU * faceAxis[faceIndex * 2] + dstV * faceAxis[faceIndex * 2 + 1];
            const wdVec3 modelSpaceDir = modelSpacePos.GetNormalized();

            const float phi = wdMath::ATan2(modelSpaceDir.x, modelSpaceDir.z).GetRadian() + wdMath::Pi<float>();
            const float r = wdMath::Sqrt(modelSpaceDir.x * modelSpaceDir.x + modelSpaceDir.z * modelSpaceDir.z);
            const float theta = wdMath::ATan2(modelSpaceDir.y, r).GetRadian() + wdMath::Pi<float>() * 0.5f;

            WD_ASSERT_DEBUG(phi >= 0.0f && phi <= 2.0f * wdMath::Pi<float>(), "");
            WD_ASSERT_DEBUG(theta >= 0.0f && theta <= wdMath::Pi<float>(), "");

            const float srcU = phi * InvPi * fHalfSrcWidth;
            const float srcV = (1.0f - theta * InvPi) * fSrcHeight;

            wdUInt32 x1 = (wdUInt32)wdMath::Floor(srcU);
            wdUInt32 x2 = x1 + 1;
            wdUInt32 y1 = (wdUInt32)wdMath::Floor(srcV);
            wdUInt32 y2 = y1 + 1;

            const float fracX = srcU - x1;
            const float fracY = srcV - y1;

            x1 = wdMath::Clamp(x1, 0u, srcWidthMinus1);
            x2 = wdMath::Clamp(x2, 0u, srcWidthMinus1);
            y1 = wdMath::Clamp(y1, 0u, srcHeightMinus1);
            y2 = wdMath::Clamp(y2, 0u, srcHeightMinus1);

            wdColor A = srcData[x1 + y1 * srcRowPitch];
            wdColor B = srcData[x2 + y1 * srcRowPitch];
            wdColor C = srcData[x1 + y2 * srcRowPitch];
            wdColor D = srcData[x2 + y2 * srcRowPitch];

            wdColor interpolated = A * (1 - fracX) * (1 - fracY) + B * (fracX) * (1 - fracY) + C * (1 - fracX) * fracY + D * fracX * fracY;
            faceData[x + y * faceRowPitch] = interpolated;
          }
        }
      }
    }

    return WD_SUCCESS;
  }

  wdLog::Error("Unexpected number of faces in cubemap input image.");
  return WD_FAILURE;
}

wdResult wdImageUtils::CreateCubemapFrom6Files(wdImage& ref_dstImg, const wdImageView* pSourceImages)
{
  WD_PROFILE_SCOPE("wdImageUtils::CreateCubemapFrom6Files");

  wdImageHeader header = pSourceImages[0].GetHeader();
  header.SetNumFaces(6);

  if (header.GetWidth() != header.GetHeight())
    return WD_FAILURE;

  if (!wdMath::IsPowerOf2(header.GetWidth()))
    return WD_FAILURE;

  ref_dstImg.ResetAndAlloc(header);

  for (wdUInt32 i = 0; i < 6; ++i)
  {
    if (pSourceImages[i].GetImageFormat() != ref_dstImg.GetImageFormat())
      return WD_FAILURE;

    if (pSourceImages[i].GetWidth() != ref_dstImg.GetWidth())
      return WD_FAILURE;

    if (pSourceImages[i].GetHeight() != ref_dstImg.GetHeight())
      return WD_FAILURE;

    WD_SUCCEED_OR_RETURN(CopyImageRectToFace(ref_dstImg, pSourceImages[i], 0, 0, i));
  }

  return WD_SUCCESS;
}

wdResult wdImageUtils::CreateVolumeTextureFromSingleFile(wdImage& ref_dstImg, const wdImageView& srcImg)
{
  WD_PROFILE_SCOPE("wdImageUtils::CreateVolumeTextureFromSingleFile");

  const wdUInt32 uiWidthHeight = srcImg.GetHeight();
  const wdUInt32 uiDepth = srcImg.GetWidth() / uiWidthHeight;

  if (!wdMath::IsPowerOf2(uiWidthHeight))
    return WD_FAILURE;
  if (!wdMath::IsPowerOf2(uiDepth))
    return WD_FAILURE;

  wdImageHeader header;
  header.SetWidth(uiWidthHeight);
  header.SetHeight(uiWidthHeight);
  header.SetDepth(uiDepth);
  header.SetImageFormat(srcImg.GetImageFormat());

  ref_dstImg.ResetAndAlloc(header);

  const wdImageView view = srcImg.GetSubImageView();

  for (wdUInt32 d = 0; d < uiDepth; ++d)
  {
    wdRectU32 r;
    r.x = uiWidthHeight * d;
    r.y = 0;
    r.width = uiWidthHeight;
    r.height = uiWidthHeight;

    WD_SUCCEED_OR_RETURN(Copy(view, r, ref_dstImg, wdVec3U32(0, 0, d)));
  }

  return WD_SUCCESS;
}

wdColor wdImageUtils::NearestSample(const wdImageView& image, wdImageAddressMode::Enum addressMode, wdVec2 vUv)
{
  WD_ASSERT_DEBUG(image.GetDepth() == 1 && image.GetNumFaces() == 1 && image.GetNumArrayIndices() == 1, "Only 2d images are supported");
  WD_ASSERT_DEBUG(image.GetImageFormat() == wdImageFormat::R32G32B32A32_FLOAT, "Unsupported format");

  return NearestSample(image.GetPixelPointer<wdColor>(), image.GetWidth(), image.GetHeight(), addressMode, vUv);
}

wdColor wdImageUtils::NearestSample(const wdColor* pPixelPointer, wdUInt32 uiWidth, wdUInt32 uiHeight, wdImageAddressMode::Enum addressMode, wdVec2 vUv)
{
  const wdInt32 w = uiWidth;
  const wdInt32 h = uiHeight;

  vUv = vUv.CompMul(wdVec2(static_cast<float>(w), static_cast<float>(h)));
  const wdInt32 intX = (wdInt32)wdMath::Floor(vUv.x);
  const wdInt32 intY = (wdInt32)wdMath::Floor(vUv.y);

  wdInt32 x = intX;
  wdInt32 y = intY;

  if (addressMode == wdImageAddressMode::Clamp)
  {
    x = wdMath::Clamp(x, 0, w - 1);
    y = wdMath::Clamp(y, 0, h - 1);
  }
  else if (addressMode == wdImageAddressMode::Repeat)
  {
    x = x % w;
    x = x < 0 ? x + w : x;
    y = y % h;
    y = y < 0 ? y + h : y;
  }
  else
  {
    WD_ASSERT_NOT_IMPLEMENTED;
  }

  return *(pPixelPointer + (y * w) + x);
}

wdColor wdImageUtils::BilinearSample(const wdImageView& image, wdImageAddressMode::Enum addressMode, wdVec2 vUv)
{
  WD_ASSERT_DEBUG(image.GetDepth() == 1 && image.GetNumFaces() == 1 && image.GetNumArrayIndices() == 1, "Only 2d images are supported");
  WD_ASSERT_DEBUG(image.GetImageFormat() == wdImageFormat::R32G32B32A32_FLOAT, "Unsupported format");

  return BilinearSample(image.GetPixelPointer<wdColor>(), image.GetWidth(), image.GetHeight(), addressMode, vUv);
}

wdColor wdImageUtils::BilinearSample(const wdColor* pData, wdUInt32 uiWidth, wdUInt32 uiHeight, wdImageAddressMode::Enum addressMode, wdVec2 vUv)
{
  wdInt32 w = uiWidth;
  wdInt32 h = uiHeight;

  vUv = vUv.CompMul(wdVec2(static_cast<float>(w), static_cast<float>(h))) - wdVec2(0.5f);
  const float floorX = wdMath::Floor(vUv.x);
  const float floorY = wdMath::Floor(vUv.y);
  const float fractionX = vUv.x - floorX;
  const float fractionY = vUv.y - floorY;
  const wdInt32 intX = (wdInt32)floorX;
  const wdInt32 intY = (wdInt32)floorY;

  wdColor c[4];
  for (wdUInt32 i = 0; i < 4; ++i)
  {
    wdInt32 x = intX + (i % 2);
    wdInt32 y = intY + (i / 2);

    if (addressMode == wdImageAddressMode::Clamp)
    {
      x = wdMath::Clamp(x, 0, w - 1);
      y = wdMath::Clamp(y, 0, h - 1);
    }
    else if (addressMode == wdImageAddressMode::Repeat)
    {
      x = x % w;
      x = x < 0 ? x + w : x;
      y = y % h;
      y = y < 0 ? y + h : y;
    }
    else
    {
      WD_ASSERT_NOT_IMPLEMENTED;
    }

    c[i] = *(pData + (y * w) + x);
  }

  const wdColor cr0 = wdMath::Lerp(c[0], c[1], fractionX);
  const wdColor cr1 = wdMath::Lerp(c[2], c[3], fractionX);

  return wdMath::Lerp(cr0, cr1, fractionY);
}

wdResult wdImageUtils::CopyChannel(wdImage& ref_dstImg, wdUInt8 uiDstChannelIdx, const wdImage& srcImg, wdUInt8 uiSrcChannelIdx)
{
  WD_PROFILE_SCOPE("wdImageUtils::CopyChannel");

  if (uiSrcChannelIdx >= 4 || uiDstChannelIdx >= 4)
    return WD_FAILURE;

  if (ref_dstImg.GetImageFormat() != wdImageFormat::R32G32B32A32_FLOAT)
    return WD_FAILURE;

  if (srcImg.GetImageFormat() != ref_dstImg.GetImageFormat())
    return WD_FAILURE;

  if (srcImg.GetWidth() != ref_dstImg.GetWidth())
    return WD_FAILURE;

  if (srcImg.GetHeight() != ref_dstImg.GetHeight())
    return WD_FAILURE;

  const wdUInt32 uiNumPixels = srcImg.GetWidth() * srcImg.GetHeight();
  const float* pSrcPixel = srcImg.GetPixelPointer<float>();
  float* pDstPixel = ref_dstImg.GetPixelPointer<float>();

  pSrcPixel += uiSrcChannelIdx;
  pDstPixel += uiDstChannelIdx;

  for (wdUInt32 i = 0; i < uiNumPixels; ++i)
  {
    *pDstPixel = *pSrcPixel;

    pSrcPixel += 4;
    pDstPixel += 4;
  }

  return WD_SUCCESS;
}

WD_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageUtils);
