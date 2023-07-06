#include <Texture/TexturePCH.h>

#if WD_ENABLED(WD_PLATFORM_LINUX)

#  include <Texture/DirectXTex/BC.h>
#  include <Texture/Image/ImageConversion.h>

#  include <Foundation/Threading/TaskSystem.h>


wdImageConversionEntry g_DXTexCpuConversions[] = {
  wdImageConversionEntry(wdImageFormat::R32G32B32A32_FLOAT, wdImageFormat::BC6H_UF16, wdImageConversionFlags::Default),

  wdImageConversionEntry(wdImageFormat::R8G8B8A8_UNORM, wdImageFormat::BC1_UNORM, wdImageConversionFlags::Default),
  wdImageConversionEntry(wdImageFormat::R8G8B8A8_UNORM, wdImageFormat::BC7_UNORM, wdImageConversionFlags::Default),

  wdImageConversionEntry(wdImageFormat::R8G8B8A8_UNORM_SRGB, wdImageFormat::BC1_UNORM_SRGB, wdImageConversionFlags::Default),
  wdImageConversionEntry(wdImageFormat::R8G8B8A8_UNORM_SRGB, wdImageFormat::BC7_UNORM_SRGB, wdImageConversionFlags::Default),
};

class wdImageConversion_CompressDxTexCpu : public wdImageConversionStepCompressBlocks
{
public:
  virtual wdArrayPtr<const wdImageConversionEntry> GetSupportedConversions() const override
  {
    return g_DXTexCpuConversions;
  }

  virtual wdResult CompressBlocks(wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt32 numBlocksX, wdUInt32 numBlocksY,
    wdImageFormat::Enum sourceFormat, wdImageFormat::Enum targetFormat) const override
  {
    if (targetFormat == wdImageFormat::BC7_UNORM || targetFormat == wdImageFormat::BC7_UNORM_SRGB)
    {
      const wdUInt32 srcStride = numBlocksX * 4 * 4;
      const wdUInt32 targetStride = numBlocksX * 16;

      wdTaskSystem::ParallelForIndexed(0, numBlocksY, [srcStride, targetStride, source, target, numBlocksX](wdUInt32 startIndex, wdUInt32 endIndex) {
        const wdUInt8* srcIt = source.GetPtr() + srcStride * startIndex * 4;
        wdUInt8* targetIt = target.GetPtr() + targetStride * startIndex;
        for (wdUInt32 blockY = startIndex; blockY < endIndex; ++blockY)
        {
          for (wdUInt32 blockX = 0; blockX < numBlocksX; ++blockX)
          {
            DirectX::XMVECTOR temp[16];
            for (wdUInt32 y = 0; y < 4; y++)
            {
              for (wdUInt32 x = 0; x < 4; x++)
              {
                const wdUInt8* pixel = srcIt + y * srcStride + x * 4;
                temp[y * 4 + x] = DirectX::XMVectorSet(pixel[0] / 255.0f, pixel[1] / 255.0f, pixel[2] / 255.0f, pixel[3] / 255.0f);
              }
            }
            DirectX::D3DXEncodeBC7(targetIt, temp, 0);

            srcIt += 4 * 4;
            targetIt += 16;
          }
          srcIt += 3 * srcStride;
        }
      });

      return WD_SUCCESS;
    }
    else if (targetFormat == wdImageFormat::BC1_UNORM || targetFormat == wdImageFormat::BC1_UNORM_SRGB)
    {
      const wdUInt32 srcStride = numBlocksX * 4 * 4;
      const wdUInt32 targetStride = numBlocksX * 8;

      wdTaskSystem::ParallelForIndexed(0, numBlocksY, [srcStride, targetStride, source, target, numBlocksX](wdUInt32 startIndex, wdUInt32 endIndex) {
        const wdUInt8* srcIt = source.GetPtr() + srcStride * startIndex * 4;
        wdUInt8* targetIt = target.GetPtr() + targetStride * startIndex;
        for (wdUInt32 blockY = startIndex; blockY < endIndex; ++blockY)
        {
          for (wdUInt32 blockX = 0; blockX < numBlocksX; ++blockX)
          {
            DirectX::XMVECTOR temp[16];
            for (wdUInt32 y = 0; y < 4; y++)
            {
              for (wdUInt32 x = 0; x < 4; x++)
              {
                const wdUInt8* pixel = srcIt + y * srcStride + x * 4;
                temp[y * 4 + x] = DirectX::XMVectorSet(pixel[0] / 255.0f, pixel[1] / 255.0f, pixel[2] / 255.0f, pixel[3] / 255.0f);
              }
            }
            DirectX::D3DXEncodeBC1(targetIt, temp, 1.0f, 0);

            srcIt += 4 * 4;
            targetIt += 8;
          }
          srcIt += 3 * srcStride;
        }
      });

      return WD_SUCCESS;
    }
    else if (targetFormat == wdImageFormat::BC6H_UF16)
    {
      const wdUInt32 srcStride = numBlocksX * 4 * 4 * sizeof(float);
      const wdUInt32 targetStride = numBlocksX * 16;

      wdTaskSystem::ParallelForIndexed(0, numBlocksY, [srcStride, targetStride, source, target, numBlocksX](wdUInt32 startIndex, wdUInt32 endIndex) {
        const wdUInt8* srcIt = source.GetPtr() + srcStride * startIndex * 4;
        wdUInt8* targetIt = target.GetPtr() + targetStride * startIndex;
        for (wdUInt32 blockY = startIndex; blockY < endIndex; ++blockY)
        {
          for (wdUInt32 blockX = 0; blockX < numBlocksX; ++blockX)
          {
            DirectX::XMVECTOR temp[16];
            for (wdUInt32 y = 0; y < 4; y++)
            {
              for (wdUInt32 x = 0; x < 4; x++)
              {
                const float* pixel = reinterpret_cast<const float*>(srcIt + y * srcStride + x * 4 * sizeof(float));
                temp[y * 4 + x] = DirectX::XMVectorSet(pixel[0], pixel[1], pixel[2], pixel[3]);
              }
            }
            DirectX::D3DXEncodeBC6HU(targetIt, temp, 0);

            srcIt += 4 * 4 * sizeof(float);
            targetIt += 16;
          }
          srcIt += 3 * srcStride;
        }
      });

      return WD_SUCCESS;
    }

    return WD_FAILURE;
  }
};

static wdImageConversion_CompressDxTexCpu s_conversion_compressDxTexCpu;

#endif

WD_STATICLINK_FILE(Texture, Texture_Image_Conversions_DXTexCpuConversions);