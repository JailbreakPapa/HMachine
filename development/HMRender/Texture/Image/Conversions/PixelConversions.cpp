#include <Texture/TexturePCH.h>

#include <Foundation/Math/Float16.h>
#include <Texture/Image/Conversions/PixelConversions.h>
#include <Texture/Image/ImageConversion.h>

#if WD_SIMD_IMPLEMENTATION == WD_SIMD_IMPLEMENTATION_SSE && WD_SSE_LEVEL >= WD_SSE_20
#  include <emmintrin.h>
#endif

#if WD_SIMD_IMPLEMENTATION == WD_SIMD_IMPLEMENTATION_SSE && WD_SSE_LEVEL >= WD_SSE_30
#  include <tmmintrin.h>
#endif

namespace
{
  // 3D vector: 11/11/10 floating-point components
  // The 3D vector is packed into 32 bits as follows: a 5-bit biased exponent
  // and 6-bit mantissa for x component, a 5-bit biased exponent and
  // 6-bit mantissa for y component, a 5-bit biased exponent and a 5-bit
  // mantissa for z. The z component is stored in the most significant bits
  // and the x component in the least significant bits. No sign bits so
  // all partial-precision numbers are positive.
  // (Z10Y11X11): [32] ZZZZZzzz zzzYYYYY yyyyyyXX XXXxxxxx [0
  union R11G11B10
  {
    struct Parts
    {
      wdUInt32 xm : 6; // x-mantissa
      wdUInt32 xe : 5; // x-exponent
      wdUInt32 ym : 6; // y-mantissa
      wdUInt32 ye : 5; // y-exponent
      wdUInt32 zm : 5; // z-mantissa
      wdUInt32 ze : 5; // z-exponent
    } p;
    wdUInt32 v;
  };
} // namespace

wdColorBaseUB wdDecompressA4B4G4R4(wdUInt16 uiColor)
{
  wdColorBaseUB result;
  result.r = ((uiColor & 0xF000u) * 17) >> 12;
  result.g = ((uiColor & 0x0F00u) * 17) >> 8;
  result.b = ((uiColor & 0x00F0u) * 17) >> 4;
  result.a = ((uiColor & 0x000Fu) * 17);
  return result;
}

wdUInt16 wdCompressA4B4G4R4(wdColorBaseUB color)
{
  wdUInt32 r = (color.r * 15 + 135) >> 8;
  wdUInt32 g = (color.g * 15 + 135) >> 8;
  wdUInt32 b = (color.b * 15 + 135) >> 8;
  wdUInt32 a = (color.a * 15 + 135) >> 8;
  return static_cast<wdUInt16>((r << 12) | (g << 8) | (b << 4) | a);
}

wdColorBaseUB wdDecompressB4G4R4A4(wdUInt16 uiColor)
{
  wdColorBaseUB result;
  result.r = ((uiColor & 0x0F00u) * 17) >> 8;
  result.g = ((uiColor & 0x00F0u) * 17) >> 4;
  result.b = ((uiColor & 0x000Fu) * 17);
  result.a = ((uiColor & 0xF000u) * 17) >> 12;
  return result;
}

wdUInt16 wdCompressB4G4R4A4(wdColorBaseUB color)
{
  wdUInt32 r = (color.r * 15 + 135) >> 8;
  wdUInt32 g = (color.g * 15 + 135) >> 8;
  wdUInt32 b = (color.b * 15 + 135) >> 8;
  wdUInt32 a = (color.a * 15 + 135) >> 8;
  return static_cast<wdUInt16>((a << 12) | (r << 8) | (g << 4) | b);
}

wdColorBaseUB wdDecompressB5G6R5(wdUInt16 uiColor)
{
  wdColorBaseUB result;
  result.r = static_cast<wdUInt8>(((uiColor & 0xF800u) * 527 + 47104) >> 17);
  result.g = static_cast<wdUInt8>(((uiColor & 0x07E0u) * 259 + 1056) >> 11);
  result.b = static_cast<wdUInt8>(((uiColor & 0x001Fu) * 527 + 23) >> 6);
  result.a = 0xFF;

  return result;
}

wdUInt16 wdCompressB5G6R5(wdColorBaseUB color)
{
  wdUInt32 r = (color.r * 249 + 1024) >> 11;
  wdUInt32 g = (color.g * 253 + 512) >> 10;
  wdUInt32 b = (color.b * 249 + 1024) >> 11;
  return static_cast<wdUInt16>((r << 11) | (g << 5) | b);
}

wdColorBaseUB wdDecompressB5G5R5X1(wdUInt16 uiColor)
{
  wdColorBaseUB result;
  result.r = static_cast<wdUInt8>(((uiColor & 0x7C00u) * 527 + 23552) >> 16);
  result.g = static_cast<wdUInt8>(((uiColor & 0x03E0u) * 527 + 736) >> 11);
  result.b = static_cast<wdUInt8>(((uiColor & 0x001Fu) * 527 + 23) >> 6);
  result.a = 0xFF;
  return result;
}

wdUInt16 wdCompressB5G5R5X1(wdColorBaseUB color)
{
  wdUInt32 r = (color.r * 249 + 1024) >> 11;
  wdUInt32 g = (color.g * 249 + 1024) >> 11;
  wdUInt32 b = (color.b * 249 + 1024) >> 11;
  return static_cast<wdUInt16>((1 << 15) | (r << 10) | (g << 5) | b);
}

wdColorBaseUB wdDecompressB5G5R5A1(wdUInt16 uiColor)
{
  wdColorBaseUB result;
  result.r = static_cast<wdUInt8>(((uiColor & 0x7C00u) * 527 + 23552) >> 16);
  result.g = static_cast<wdUInt8>(((uiColor & 0x03E0u) * 527 + 736) >> 11);
  result.b = static_cast<wdUInt8>(((uiColor & 0x001Fu) * 527 + 23) >> 6);
  result.a = static_cast<wdUInt8>(((uiColor & 0x8000u) * 255) >> 15);
  return result;
}

wdUInt16 wdCompressB5G5R5A1(wdColorBaseUB color)
{
  wdUInt32 r = (color.r * 249 + 1024) >> 11;
  wdUInt32 g = (color.g * 249 + 1024) >> 11;
  wdUInt32 b = (color.b * 249 + 1024) >> 11;
  wdUInt32 a = (color.a) >> 7;
  return static_cast<wdUInt16>((a << 15) | (r << 10) | (g << 5) | b);
}

wdColorBaseUB wdDecompressX1B5G5R5(wdUInt16 uiColor)
{
  wdColorBaseUB result;
  result.r = static_cast<wdUInt8>(((uiColor & 0xF800u) * 527 + 23552) >> 17);
  result.g = static_cast<wdUInt8>(((uiColor & 0x07C0u) * 527 + 736) >> 12);
  result.b = static_cast<wdUInt8>(((uiColor & 0x003Eu) * 527 + 23) >> 7);
  result.a = 0xFF;
  return result;
}

wdUInt16 wdCompressX1B5G5R5(wdColorBaseUB color)
{
  wdUInt32 r = (color.r * 249 + 1024) >> 11;
  wdUInt32 g = (color.g * 249 + 1024) >> 11;
  wdUInt32 b = (color.b * 249 + 1024) >> 11;
  return static_cast<wdUInt16>((r << 11) | (g << 6) | (b << 1) | 1);
}

wdColorBaseUB wdDecompressA1B5G5R5(wdUInt16 uiColor)
{
  wdColorBaseUB result;
  result.r = static_cast<wdUInt8>(((uiColor & 0xF800u) * 527 + 23552) >> 17);
  result.g = static_cast<wdUInt8>(((uiColor & 0x07C0u) * 527 + 736) >> 12);
  result.b = static_cast<wdUInt8>(((uiColor & 0x003Eu) * 527 + 23) >> 7);
  result.a = static_cast<wdUInt8>((uiColor & 0x0001u) * 255);
  return result;
}

wdUInt16 wdCompressA1B5G5R5(wdColorBaseUB color)
{
  wdUInt32 r = (color.r * 249 + 1024) >> 11;
  wdUInt32 g = (color.g * 249 + 1024) >> 11;
  wdUInt32 b = (color.b * 249 + 1024) >> 11;
  wdUInt32 a = color.a >> 7;
  return static_cast<wdUInt16>((r << 11) | (g << 6) | (b << 1) | a);
}

template <wdColorBaseUB (*decompressFunc)(wdUInt16), wdImageFormat::Enum templateSourceFormat>
class wdImageConversionStep_Decompress16bpp : wdImageConversionStepLinear
{
  virtual wdArrayPtr<const wdImageConversionEntry> GetSupportedConversions() const override
  {
    wdImageFormat::Enum sourceFormatSrgb = wdImageFormat::AsSrgb(templateSourceFormat);
    WD_ASSERT_DEV(
      sourceFormatSrgb != templateSourceFormat, "Format '%s' should have a corresponding sRGB format", wdImageFormat::GetName(templateSourceFormat));

    static wdImageConversionEntry supportedConversions[] = {
      wdImageConversionEntry(templateSourceFormat, wdImageFormat::R8G8B8A8_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(sourceFormatSrgb, wdImageFormat::R8G8B8A8_UNORM_SRGB, wdImageConversionFlags::Default),
    };

    return supportedConversions;
  }

  virtual wdResult ConvertPixels(wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt64 numElements, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const override
  {
    wdUInt32 sourceStride = 2;
    wdUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (numElements)
    {
      *reinterpret_cast<wdColorBaseUB*>(targetPointer) = decompressFunc(*reinterpret_cast<const wdUInt16*>(sourcePointer));

      sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return WD_SUCCESS;
  }
};

template <wdUInt16 (*compressFunc)(wdColorBaseUB), wdImageFormat::Enum templateTargetFormat>
class wdImageConversionStep_Compress16bpp : wdImageConversionStepLinear
{
  virtual wdArrayPtr<const wdImageConversionEntry> GetSupportedConversions() const override
  {
    wdImageFormat::Enum targetFormatSrgb = wdImageFormat::AsSrgb(templateTargetFormat);
    WD_ASSERT_DEV(
      targetFormatSrgb != templateTargetFormat, "Format '%s' should have a corresponding sRGB format", wdImageFormat::GetName(templateTargetFormat));

    static wdImageConversionEntry supportedConversions[] = {
      wdImageConversionEntry(wdImageFormat::R8G8B8A8_UNORM, templateTargetFormat, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8B8A8_UNORM_SRGB, targetFormatSrgb, wdImageConversionFlags::Default),
    };

    return supportedConversions;
  }

  virtual wdResult ConvertPixels(wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt64 numElements, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const override
  {
    wdUInt32 sourceStride = 4;
    wdUInt32 targetStride = 2;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (numElements)
    {
      *reinterpret_cast<wdUInt16*>(targetPointer) = compressFunc(*reinterpret_cast<const wdColorBaseUB*>(sourcePointer));

      sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride);
      numElements--;
    }

    return WD_SUCCESS;
  }
};

#if WD_SIMD_IMPLEMENTATION == WD_SIMD_IMPLEMENTATION_SSE

static bool IsAligned(const void* pPointer)
{
  return reinterpret_cast<size_t>(pPointer) % 16 == 0;
}

#endif

struct wdImageSwizzleConversion32_2103 : public wdImageConversionStepLinear
{
  virtual wdArrayPtr<const wdImageConversionEntry> GetSupportedConversions() const override
  {
    static wdImageConversionEntry supportedConversions[] = {
      wdImageConversionEntry(wdImageFormat::B8G8R8A8_UNORM, wdImageFormat::R8G8B8A8_UNORM, wdImageConversionFlags::InPlace),
      wdImageConversionEntry(wdImageFormat::R8G8B8A8_UNORM, wdImageFormat::B8G8R8A8_UNORM, wdImageConversionFlags::InPlace),
      wdImageConversionEntry(wdImageFormat::R8G8B8A8_UNORM, wdImageFormat::B8G8R8X8_UNORM, wdImageConversionFlags::InPlace),
      wdImageConversionEntry(wdImageFormat::B8G8R8A8_UNORM_SRGB, wdImageFormat::R8G8B8A8_UNORM_SRGB, wdImageConversionFlags::InPlace),
      wdImageConversionEntry(wdImageFormat::R8G8B8A8_UNORM_SRGB, wdImageFormat::B8G8R8A8_UNORM_SRGB, wdImageConversionFlags::InPlace),
      wdImageConversionEntry(wdImageFormat::R8G8B8A8_UNORM_SRGB, wdImageFormat::B8G8R8X8_UNORM_SRGB, wdImageConversionFlags::InPlace),
    };
    return supportedConversions;
  }

  virtual wdResult ConvertPixels(wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt64 uiNumElements, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const override
  {
    wdUInt32 sourceStride = 4;
    wdUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

#if WD_SIMD_IMPLEMENTATION == WD_SIMD_IMPLEMENTATION_SSE
    if (IsAligned(sourcePointer) && IsAligned(targetPointer))
    {
#  if WD_SSE_LEVEL >= WD_SSE_30
      const wdUInt32 elementsPerBatch = 8;

      __m128i shuffleMask = _mm_set_epi8(15, 12, 13, 14, 11, 8, 9, 10, 7, 4, 5, 6, 3, 0, 1, 2);

      // Intel optimization manual, Color Pixel Format Conversion Using SSE3
      while (uiNumElements >= elementsPerBatch)
      {
        __m128i in0 = reinterpret_cast<const __m128i*>(sourcePointer)[0];
        __m128i in1 = reinterpret_cast<const __m128i*>(sourcePointer)[1];

        reinterpret_cast<__m128i*>(targetPointer)[0] = _mm_shuffle_epi8(in0, shuffleMask);
        reinterpret_cast<__m128i*>(targetPointer)[1] = _mm_shuffle_epi8(in1, shuffleMask);

        sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride * elementsPerBatch);
        targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride * elementsPerBatch);
        uiNumElements -= elementsPerBatch;
      }
#  else
      const wdUInt32 elementsPerBatch = 8;

      __m128i mask1 = _mm_set1_epi32(0xff00ff00);
      __m128i mask2 = _mm_set1_epi32(0x00ff00ff);

      // Intel optimization manual, Color Pixel Format Conversion Using SSE2
      while (numElements >= elementsPerBatch)
      {
        __m128i in0 = reinterpret_cast<const __m128i*>(sourcePointer)[0];
        __m128i in1 = reinterpret_cast<const __m128i*>(sourcePointer)[1];

        reinterpret_cast<__m128i*>(targetPointer)[0] =
          _mm_or_si128(_mm_and_si128(in0, mask1), _mm_and_si128(_mm_or_si128(_mm_slli_epi32(in0, 16), _mm_srli_epi32(in0, 16)), mask2));
        reinterpret_cast<__m128i*>(targetPointer)[1] =
          _mm_or_si128(_mm_and_si128(in1, mask1), _mm_and_si128(_mm_or_si128(_mm_slli_epi32(in1, 16), _mm_srli_epi32(in1, 16)), mask2));

        sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride * elementsPerBatch);
        targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride * elementsPerBatch);
        numElements -= elementsPerBatch;
      }
#  endif
    }
#endif

    while (uiNumElements)
    {
      wdUInt8 a, b, c, d;
      a = reinterpret_cast<const wdUInt8*>(sourcePointer)[2];
      b = reinterpret_cast<const wdUInt8*>(sourcePointer)[1];
      c = reinterpret_cast<const wdUInt8*>(sourcePointer)[0];
      d = reinterpret_cast<const wdUInt8*>(sourcePointer)[3];
      reinterpret_cast<wdUInt8*>(targetPointer)[0] = a;
      reinterpret_cast<wdUInt8*>(targetPointer)[1] = b;
      reinterpret_cast<wdUInt8*>(targetPointer)[2] = c;
      reinterpret_cast<wdUInt8*>(targetPointer)[3] = d;

      sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return WD_SUCCESS;
  }
};

struct wdImageConversion_BGRX_BGRA : public wdImageConversionStepLinear
{
  virtual wdArrayPtr<const wdImageConversionEntry> GetSupportedConversions() const override
  {
    static wdImageConversionEntry supportedConversions[] = {
      {wdImageFormat::B8G8R8X8_UNORM, wdImageFormat::B8G8R8A8_UNORM, wdImageConversionFlags::InPlace},
      {wdImageFormat::B8G8R8X8_UNORM_SRGB, wdImageFormat::B8G8R8A8_UNORM_SRGB, wdImageConversionFlags::InPlace},
    };
    return supportedConversions;
  }

  virtual wdResult ConvertPixels(wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt64 uiNumElements, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const override
  {
    wdUInt32 sourceStride = 4;
    wdUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

#if WD_SIMD_IMPLEMENTATION == WD_SIMD_IMPLEMENTATION_SSE && WD_SSE_LEVEL >= WD_SSE_20
    if (IsAligned(sourcePointer) && IsAligned(targetPointer))
    {
      const wdUInt32 elementsPerBatch = 4;

      __m128i mask = _mm_set1_epi32(0xFF000000);

      while (uiNumElements >= elementsPerBatch)
      {
        const __m128i* pSource = reinterpret_cast<const __m128i*>(sourcePointer);
        __m128i* pTarget = reinterpret_cast<__m128i*>(targetPointer);

        pTarget[0] = _mm_or_si128(pSource[0], mask);

        sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride * elementsPerBatch);
        targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride * elementsPerBatch);
        uiNumElements -= elementsPerBatch;
      }
    }
#endif

    while (uiNumElements)
    {
      wdUInt32 x = *(reinterpret_cast<const wdUInt32*>(sourcePointer));

#if WD_ENABLED(WD_PLATFORM_LITTLE_ENDIAN)
      x |= 0xFF000000;
#else
      x |= 0x000000FF;
#endif

      *(reinterpret_cast<wdUInt32*>(targetPointer)) = x;

      sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return WD_SUCCESS;
  }
};

class wdImageConversion_F32_U8 : public wdImageConversionStepLinear
{
public:
  virtual wdArrayPtr<const wdImageConversionEntry> GetSupportedConversions() const override
  {
    static wdImageConversionEntry supportedConversions[] = {
      wdImageConversionEntry(wdImageFormat::R32_FLOAT, wdImageFormat::R8_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32_FLOAT, wdImageFormat::R8G8_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32B32_FLOAT, wdImageFormat::R8G8B8_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32B32A32_FLOAT, wdImageFormat::R8G8B8A8_UNORM, wdImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual wdResult ConvertPixels(wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt64 uiNumElements, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    uiNumElements *= wdImageFormat::GetBitsPerPixel(targetFormat) / 8;

    wdUInt32 sourceStride = 4;
    wdUInt32 targetStride = 1;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

#if WD_SIMD_IMPLEMENTATION == WD_SIMD_IMPLEMENTATION_SSE && WD_SSE_LEVEL >= WD_SSE_20
    {
      const wdUInt32 elementsPerBatch = 16;

      __m128 zero = _mm_setzero_ps();
      __m128 one = _mm_set1_ps(1.0f);
      __m128 scale = _mm_set1_ps(255.0f);
      __m128 half = _mm_set1_ps(0.5f);

      while (uiNumElements >= elementsPerBatch)
      {
        __m128 float0 = _mm_loadu_ps(static_cast<const float*>(sourcePointer) + 0);
        __m128 float1 = _mm_loadu_ps(static_cast<const float*>(sourcePointer) + 4);
        __m128 float2 = _mm_loadu_ps(static_cast<const float*>(sourcePointer) + 8);
        __m128 float3 = _mm_loadu_ps(static_cast<const float*>(sourcePointer) + 12);

        // Clamp NaN to zero
        float0 = _mm_and_ps(_mm_cmpord_ps(float0, zero), float0);
        float1 = _mm_and_ps(_mm_cmpord_ps(float1, zero), float1);
        float2 = _mm_and_ps(_mm_cmpord_ps(float2, zero), float2);
        float3 = _mm_and_ps(_mm_cmpord_ps(float3, zero), float3);

        // Saturate
        float0 = _mm_max_ps(zero, _mm_min_ps(one, float0));
        float1 = _mm_max_ps(zero, _mm_min_ps(one, float1));
        float2 = _mm_max_ps(zero, _mm_min_ps(one, float2));
        float3 = _mm_max_ps(zero, _mm_min_ps(one, float3));

        float0 = _mm_mul_ps(float0, scale);
        float1 = _mm_mul_ps(float1, scale);
        float2 = _mm_mul_ps(float2, scale);
        float3 = _mm_mul_ps(float3, scale);

        // Add 0.5f and truncate for rounding as required by D3D spec
        float0 = _mm_add_ps(float0, half);
        float1 = _mm_add_ps(float1, half);
        float2 = _mm_add_ps(float2, half);
        float3 = _mm_add_ps(float3, half);

        __m128i int0 = _mm_cvttps_epi32(float0);
        __m128i int1 = _mm_cvttps_epi32(float1);
        __m128i int2 = _mm_cvttps_epi32(float2);
        __m128i int3 = _mm_cvttps_epi32(float3);

        __m128i short0 = _mm_packs_epi32(int0, int1);
        __m128i short1 = _mm_packs_epi32(int2, int3);

        _mm_storeu_si128(reinterpret_cast<__m128i*>(targetPointer), _mm_packus_epi16(short0, short1));

        sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride * elementsPerBatch);
        targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride * elementsPerBatch);
        uiNumElements -= elementsPerBatch;
      }
    }
#endif

    while (uiNumElements)
    {

      *reinterpret_cast<wdUInt8*>(targetPointer) = wdMath::ColorFloatToByte(*reinterpret_cast<const float*>(sourcePointer));

      sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return WD_SUCCESS;
  }
};

class wdImageConversion_F32_sRGB : public wdImageConversionStepLinear
{
public:
  virtual wdArrayPtr<const wdImageConversionEntry> GetSupportedConversions() const override
  {
    static wdImageConversionEntry supportedConversions[] = {
      wdImageConversionEntry(wdImageFormat::R32G32B32A32_FLOAT, wdImageFormat::R8G8B8A8_UNORM_SRGB, wdImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual wdResult ConvertPixels(wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt64 uiNumElements, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const override
  {
    wdUInt32 sourceStride = 16;
    wdUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      *reinterpret_cast<wdColorGammaUB*>(targetPointer) = *reinterpret_cast<const wdColor*>(sourcePointer);

      sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return WD_SUCCESS;
  }
};

class wdImageConversion_F32_U16 : public wdImageConversionStepLinear
{
public:
  virtual wdArrayPtr<const wdImageConversionEntry> GetSupportedConversions() const override
  {
    static wdImageConversionEntry supportedConversions[] = {
      wdImageConversionEntry(wdImageFormat::R32_FLOAT, wdImageFormat::R16_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32_FLOAT, wdImageFormat::R16G16_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32B32_FLOAT, wdImageFormat::R16G16B16_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32B32A32_FLOAT, wdImageFormat::R16G16B16A16_UNORM, wdImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual wdResult ConvertPixels(wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt64 uiNumElements, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    uiNumElements *= wdImageFormat::GetBitsPerPixel(targetFormat) / 16;

    wdUInt32 sourceStride = 4;
    wdUInt32 targetStride = 2;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {

      *reinterpret_cast<wdUInt16*>(targetPointer) = wdMath::ColorFloatToShort(*reinterpret_cast<const float*>(sourcePointer));

      sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return WD_SUCCESS;
  }
};

class wdImageConversion_F32_F16 : public wdImageConversionStepLinear
{
public:
  virtual wdArrayPtr<const wdImageConversionEntry> GetSupportedConversions() const override
  {
    static wdImageConversionEntry supportedConversions[] = {
      wdImageConversionEntry(wdImageFormat::R32_FLOAT, wdImageFormat::R16_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32_FLOAT, wdImageFormat::R16G16_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32B32A32_FLOAT, wdImageFormat::R16G16B16A16_FLOAT, wdImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual wdResult ConvertPixels(wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt64 uiNumElements, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    uiNumElements *= wdImageFormat::GetBitsPerPixel(targetFormat) / 16;

    wdUInt32 sourceStride = 4;
    wdUInt32 targetStride = 2;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {

      *reinterpret_cast<wdFloat16*>(targetPointer) = *reinterpret_cast<const float*>(sourcePointer);

      sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return WD_SUCCESS;
  }
};

class wdImageConversion_F32_S8 : public wdImageConversionStepLinear
{
public:
public:
  virtual wdArrayPtr<const wdImageConversionEntry> GetSupportedConversions() const override
  {
    static wdImageConversionEntry supportedConversions[] = {
      wdImageConversionEntry(wdImageFormat::R32_FLOAT, wdImageFormat::R8_SNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32_FLOAT, wdImageFormat::R8G8_SNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32B32A32_FLOAT, wdImageFormat::R8G8B8A8_SNORM, wdImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual wdResult ConvertPixels(wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt64 uiNumElements, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    uiNumElements *= wdImageFormat::GetBitsPerPixel(targetFormat) / 8;

    wdUInt32 sourceStride = 4;
    wdUInt32 targetStride = 1;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {

      *reinterpret_cast<wdInt8*>(targetPointer) = wdMath::ColorFloatToSignedByte(*reinterpret_cast<const float*>(sourcePointer));

      sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return WD_SUCCESS;
  }
};

class wdImageConversion_U8_F32 : public wdImageConversionStepLinear
{
public:
  virtual wdArrayPtr<const wdImageConversionEntry> GetSupportedConversions() const override
  {
    static wdImageConversionEntry supportedConversions[] = {
      wdImageConversionEntry(wdImageFormat::R8_UNORM, wdImageFormat::R32_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8_UNORM, wdImageFormat::R32G32_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8B8_UNORM, wdImageFormat::R32G32B32_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8B8A8_UNORM, wdImageFormat::R32G32B32A32_FLOAT, wdImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual wdResult ConvertPixels(wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt64 uiNumElements, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    uiNumElements *= wdImageFormat::GetBitsPerPixel(targetFormat) / 32;

    wdUInt32 sourceStride = 1;
    wdUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      *reinterpret_cast<float*>(targetPointer) = wdMath::ColorByteToFloat(*reinterpret_cast<const wdUInt8*>(sourcePointer));

      sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return WD_SUCCESS;
  }
};

class wdImageConversion_sRGB_F32 : public wdImageConversionStepLinear
{
public:
  virtual wdArrayPtr<const wdImageConversionEntry> GetSupportedConversions() const override
  {
    static wdImageConversionEntry supportedConversions[] = {
      wdImageConversionEntry(wdImageFormat::R8G8B8A8_UNORM_SRGB, wdImageFormat::R32G32B32A32_FLOAT, wdImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual wdResult ConvertPixels(wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt64 uiNumElements, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const override
  {
    wdUInt32 sourceStride = 4;
    wdUInt32 targetStride = 16;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      *reinterpret_cast<wdColor*>(targetPointer) = *reinterpret_cast<const wdColorGammaUB*>(sourcePointer);

      sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return WD_SUCCESS;
  }
};

class wdImageConversion_U16_F32 : public wdImageConversionStepLinear
{
public:
  virtual wdArrayPtr<const wdImageConversionEntry> GetSupportedConversions() const override
  {
    static wdImageConversionEntry supportedConversions[] = {
      wdImageConversionEntry(wdImageFormat::R16_UNORM, wdImageFormat::R32_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R16G16_UNORM, wdImageFormat::R32G32_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R16G16B16_UNORM, wdImageFormat::R32G32B32_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R16G16B16A16_UNORM, wdImageFormat::R32G32B32A32_FLOAT, wdImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual wdResult ConvertPixels(wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt64 uiNumElements, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    uiNumElements *= wdImageFormat::GetBitsPerPixel(targetFormat) / 32;

    wdUInt32 sourceStride = 2;
    wdUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      *reinterpret_cast<float*>(targetPointer) = wdMath::ColorShortToFloat(*reinterpret_cast<const wdUInt16*>(sourcePointer));

      sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return WD_SUCCESS;
  }
};

class wdImageConversion_F16_F32 : public wdImageConversionStepLinear
{
public:
  virtual wdArrayPtr<const wdImageConversionEntry> GetSupportedConversions() const override
  {
    static wdImageConversionEntry supportedConversions[] = {
      wdImageConversionEntry(wdImageFormat::R16_FLOAT, wdImageFormat::R32_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R16G16_FLOAT, wdImageFormat::R32G32_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R16G16B16A16_FLOAT, wdImageFormat::R32G32B32A32_FLOAT, wdImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual wdResult ConvertPixels(wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt64 uiNumElements, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    uiNumElements *= wdImageFormat::GetBitsPerPixel(targetFormat) / 32;

    wdUInt32 sourceStride = 2;
    wdUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      *reinterpret_cast<float*>(targetPointer) = *reinterpret_cast<const wdFloat16*>(sourcePointer);

      sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return WD_SUCCESS;
  }
};

class wdImageConversion_S8_F32 : public wdImageConversionStepLinear
{
public:
  virtual wdArrayPtr<const wdImageConversionEntry> GetSupportedConversions() const override
  {
    static wdImageConversionEntry supportedConversions[] = {
      wdImageConversionEntry(wdImageFormat::R8_SNORM, wdImageFormat::R32_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8_SNORM, wdImageFormat::R32G32_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8B8A8_SNORM, wdImageFormat::R32G32B32A32_FLOAT, wdImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual wdResult ConvertPixels(wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt64 uiNumElements, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const override
  {
    // Work with single channels instead of pixels
    uiNumElements *= wdImageFormat::GetBitsPerPixel(targetFormat) / 32;

    wdUInt32 sourceStride = 1;
    wdUInt32 targetStride = 4;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      *reinterpret_cast<float*>(targetPointer) = wdMath::ColorSignedByteToFloat(*reinterpret_cast<const wdInt8*>(sourcePointer));

      sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return WD_SUCCESS;
  }
};

struct wdImageConversion_Pad_To_RGBA_U8 : public wdImageConversionStepLinear
{
public:
  virtual wdArrayPtr<const wdImageConversionEntry> GetSupportedConversions() const override
  {
    static wdImageConversionEntry supportedConversions[] = {
      wdImageConversionEntry(wdImageFormat::R8_UNORM, wdImageFormat::R8G8B8A8_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8_UNORM, wdImageFormat::R8G8B8A8_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8B8_UNORM, wdImageFormat::R8G8B8A8_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8B8_UNORM_SRGB, wdImageFormat::R8G8B8A8_UNORM_SRGB, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::B8G8R8_UNORM, wdImageFormat::B8G8R8A8_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::B8G8R8_UNORM_SRGB, wdImageFormat::B8G8R8A8_UNORM_SRGB, wdImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual wdResult ConvertPixels(wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt64 uiNumElements, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const override
  {
    wdUInt32 sourceStride = wdImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    wdUInt32 targetStride = wdImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const wdUInt8* sourcePointer = static_cast<const wdUInt8*>(source.GetPtr());
    wdUInt8* targetPointer = static_cast<wdUInt8*>(target.GetPtr());

    const wdUInt32 numChannels = sourceStride / sizeof(wdUInt8);

#if WD_ENABLED(WD_PLATFORM_LITTLE_ENDIAN)
    if (numChannels == 3)
    {
      // Fast path for RGB -> RGBA
      const wdUInt32 elementsPerBatch = 4;

      while (uiNumElements >= elementsPerBatch)
      {
        wdUInt32 source0 = reinterpret_cast<const wdUInt32*>(sourcePointer)[0];
        wdUInt32 source1 = reinterpret_cast<const wdUInt32*>(sourcePointer)[1];
        wdUInt32 source2 = reinterpret_cast<const wdUInt32*>(sourcePointer)[2];

        wdUInt32 target0 = source0 | 0xFF000000;
        wdUInt32 target1 = (source0 >> 24) | (source1 << 8) | 0xFF000000;
        wdUInt32 target2 = (source1 >> 16) | (source2 << 16) | 0xFF000000;
        wdUInt32 target3 = (source2 >> 8) | 0xFF000000;

        reinterpret_cast<wdUInt32*>(targetPointer)[0] = target0;
        reinterpret_cast<wdUInt32*>(targetPointer)[1] = target1;
        reinterpret_cast<wdUInt32*>(targetPointer)[2] = target2;
        reinterpret_cast<wdUInt32*>(targetPointer)[3] = target3;

        sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride * elementsPerBatch);
        targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride * elementsPerBatch);
        uiNumElements -= elementsPerBatch;
      }
    }
#endif


    while (uiNumElements)
    {
      // Copy existing channels
      memcpy(targetPointer, sourcePointer, numChannels);

      // Fill others with zero
      memset(targetPointer + numChannels, 0, 3 * sizeof(wdUInt8) - numChannels);

      // Set alpha to 1
      targetPointer[3] = 0xFF;

      sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return WD_SUCCESS;
  }
};

struct wdImageConversion_Pad_To_RGBA_F32 : public wdImageConversionStepLinear
{
public:
  virtual wdArrayPtr<const wdImageConversionEntry> GetSupportedConversions() const override
  {
    static wdImageConversionEntry supportedConversions[] = {
      wdImageConversionEntry(wdImageFormat::R32_FLOAT, wdImageFormat::R32G32B32A32_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32_FLOAT, wdImageFormat::R32G32B32A32_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32B32_FLOAT, wdImageFormat::R32G32B32A32_FLOAT, wdImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual wdResult ConvertPixels(wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt64 uiNumElements, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const override
  {
    wdUInt32 sourceStride = wdImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    wdUInt32 targetStride = wdImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const float* sourcePointer = static_cast<const float*>(static_cast<const void*>(source.GetPtr()));
    float* targetPointer = static_cast<float*>(static_cast<void*>(target.GetPtr()));

    const wdUInt32 numChannels = sourceStride / sizeof(float);

    while (uiNumElements)
    {
      // Copy existing channels
      memcpy(targetPointer, sourcePointer, numChannels * sizeof(float));

      // Fill others with zero
      memset(targetPointer + numChannels, 0, sizeof(float) * (3 - numChannels));

      // Set alpha to 1
      targetPointer[3] = 1.0f;

      sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return WD_SUCCESS;
  }
};

struct wdImageConversion_DiscardChannels : public wdImageConversionStepLinear
{
public:
  virtual wdArrayPtr<const wdImageConversionEntry> GetSupportedConversions() const override
  {
    static wdImageConversionEntry supportedConversions[] = {
      wdImageConversionEntry(wdImageFormat::R32G32B32A32_FLOAT, wdImageFormat::R32G32B32_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32B32A32_FLOAT, wdImageFormat::R32G32_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32B32A32_FLOAT, wdImageFormat::R32_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32B32A32_UINT, wdImageFormat::R32G32B32_UINT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32B32A32_UINT, wdImageFormat::R32G32_UINT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32B32A32_UINT, wdImageFormat::R32_UINT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32B32A32_SINT, wdImageFormat::R32G32B32_SINT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32B32A32_SINT, wdImageFormat::R32G32_SINT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32B32A32_SINT, wdImageFormat::R32_SINT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32B32_FLOAT, wdImageFormat::R32G32_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32B32_FLOAT, wdImageFormat::R32_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32B32_UINT, wdImageFormat::R32G32_UINT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32B32_UINT, wdImageFormat::R32_UINT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32B32_SINT, wdImageFormat::R32G32_SINT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32B32_SINT, wdImageFormat::R32_SINT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R16G16B16A16_FLOAT, wdImageFormat::R16G16_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R16G16B16A16_FLOAT, wdImageFormat::R16_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R16G16B16A16_UNORM, wdImageFormat::R16G16B16_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R16G16B16A16_UNORM, wdImageFormat::R16G16_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R16G16B16A16_UNORM, wdImageFormat::R16_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R16G16B16A16_UINT, wdImageFormat::R16G16_UINT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R16G16B16A16_UINT, wdImageFormat::R16_UINT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R16G16B16A16_SNORM, wdImageFormat::R16G16_SNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R16G16B16A16_SNORM, wdImageFormat::R16_SNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R16G16B16A16_SINT, wdImageFormat::R16G16_SINT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R16G16B16A16_SINT, wdImageFormat::R16_SINT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R16G16B16_UNORM, wdImageFormat::R16G16_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R16G16B16_UNORM, wdImageFormat::R16_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32_FLOAT, wdImageFormat::R32_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32_UINT, wdImageFormat::R32_UINT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32_SINT, wdImageFormat::R32_SINT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::D32_FLOAT_S8X24_UINT, wdImageFormat::D32_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8B8A8_UNORM, wdImageFormat::R8G8B8_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8B8A8_UNORM, wdImageFormat::R8G8_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8B8A8_UNORM, wdImageFormat::R8_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8B8A8_UNORM_SRGB, wdImageFormat::R8G8B8_UNORM_SRGB, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8B8A8_UINT, wdImageFormat::R8G8_UINT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8B8A8_UINT, wdImageFormat::R8_UINT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8B8A8_SNORM, wdImageFormat::R8G8_SNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8B8A8_SNORM, wdImageFormat::R8_SNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8B8A8_SINT, wdImageFormat::R8G8_SINT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8B8A8_SINT, wdImageFormat::R8_SINT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::B8G8R8A8_UNORM, wdImageFormat::B8G8R8_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::B8G8R8A8_UNORM_SRGB, wdImageFormat::B8G8R8_UNORM_SRGB, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::B8G8R8X8_UNORM, wdImageFormat::B8G8R8_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::B8G8R8X8_UNORM_SRGB, wdImageFormat::B8G8R8_UNORM_SRGB, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R16G16_FLOAT, wdImageFormat::R16_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R16G16_UNORM, wdImageFormat::R16_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R16G16_UINT, wdImageFormat::R16_UINT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R16G16_SNORM, wdImageFormat::R16_SNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R16G16_SINT, wdImageFormat::R16_SINT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8B8_UNORM, wdImageFormat::R8G8_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8B8_UNORM, wdImageFormat::R8_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8_UNORM, wdImageFormat::R8_UNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8_UINT, wdImageFormat::R8_UINT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8_SNORM, wdImageFormat::R8_SNORM, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R8G8_SINT, wdImageFormat::R8_SINT, wdImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual wdResult ConvertPixels(wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt64 uiNumElements, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const override
  {
    wdUInt32 sourceStride = wdImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    wdUInt32 targetStride = wdImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    if (wdImageFormat::GetBitsPerPixel(sourceFormat) == 32 && wdImageFormat::GetBitsPerPixel(targetFormat) == 24)
    {
      // Fast path for RGBA -> RGB
      while (uiNumElements)
      {
        const wdUInt8* src = static_cast<const wdUInt8*>(sourcePointer);
        wdUInt8* dst = static_cast<wdUInt8*>(targetPointer);

        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];

        sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
        targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride);
        uiNumElements--;
      }
    }

    while (uiNumElements)
    {
      memcpy(targetPointer, sourcePointer, targetStride);

      sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return WD_SUCCESS;
  }
};

class wdImageConversion_FLOAT_to_R11G11B10 : public wdImageConversionStepLinear
{
public:
  virtual wdArrayPtr<const wdImageConversionEntry> GetSupportedConversions() const override
  {
    static wdImageConversionEntry supportedConversions[] = {
      wdImageConversionEntry(wdImageFormat::R32G32B32A32_FLOAT, wdImageFormat::R11G11B10_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R32G32B32_FLOAT, wdImageFormat::R11G11B10_FLOAT, wdImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual wdResult ConvertPixels(wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt64 uiNumElements, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const override
  {
    wdUInt32 sourceStride = wdImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    wdUInt32 targetStride = wdImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      // Adapted from DirectXMath's XMStoreFloat3PK
      wdUInt32 IValue[3];
      memcpy(IValue, sourcePointer, 12);

      wdUInt32 Result[3];

      // X & Y Channels (5-bit exponent, 6-bit mantissa)
      for (wdUInt32 j = 0; j < 2; ++j)
      {
        wdUInt32 Sign = IValue[j] & 0x80000000;
        wdUInt32 I = IValue[j] & 0x7FFFFFFF;

        if ((I & 0x7F800000) == 0x7F800000)
        {
          // INF or NAN
          Result[j] = 0x7c0;
          if ((I & 0x7FFFFF) != 0)
          {
            Result[j] = 0x7c0 | (((I >> 17) | (I >> 11) | (I >> 6) | (I)) & 0x3f);
          }
          else if (Sign)
          {
            // -INF is clamped to 0 since 3PK is positive only
            Result[j] = 0;
          }
        }
        else if (Sign)
        {
          // 3PK is positive only, so clamp to zero
          Result[j] = 0;
        }
        else if (I > 0x477E0000U)
        {
          // The number is too large to be represented as a float11, set to max
          Result[j] = 0x7BF;
        }
        else
        {
          if (I < 0x38800000U)
          {
            // The number is too small to be represented as a normalized float11
            // Convert it to a denormalized value.
            wdUInt32 Shift = 113U - (I >> 23U);
            I = (0x800000U | (I & 0x7FFFFFU)) >> Shift;
          }
          else
          {
            // Rebias the exponent to represent the value as a normalized float11
            I += 0xC8000000U;
          }

          Result[j] = ((I + 0xFFFFU + ((I >> 17U) & 1U)) >> 17U) & 0x7ffU;
        }
      }

      // Z Channel (5-bit exponent, 5-bit mantissa)
      wdUInt32 Sign = IValue[2] & 0x80000000;
      wdUInt32 I = IValue[2] & 0x7FFFFFFF;

      if ((I & 0x7F800000) == 0x7F800000)
      {
        // INF or NAN
        Result[2] = 0x3e0;
        if (I & 0x7FFFFF)
        {
          Result[2] = 0x3e0 | (((I >> 18) | (I >> 13) | (I >> 3) | (I)) & 0x1f);
        }
        else if (Sign)
        {
          // -INF is clamped to 0 since 3PK is positive only
          Result[2] = 0;
        }
      }
      else if (Sign)
      {
        // 3PK is positive only, so clamp to zero
        Result[2] = 0;
      }
      else if (I > 0x477C0000U)
      {
        // The number is too large to be represented as a float10, set to max
        Result[2] = 0x3df;
      }
      else
      {
        if (I < 0x38800000U)
        {
          // The number is too small to be represented as a normalized float10
          // Convert it to a denormalized value.
          wdUInt32 Shift = 113U - (I >> 23U);
          I = (0x800000U | (I & 0x7FFFFFU)) >> Shift;
        }
        else
        {
          // Rebias the exponent to represent the value as a normalized float10
          I += 0xC8000000U;
        }

        Result[2] = ((I + 0x1FFFFU + ((I >> 18U) & 1U)) >> 18U) & 0x3ffU;
      }

      // Pack Result into memory
      *reinterpret_cast<wdUInt32*>(targetPointer) = (Result[0] & 0x7ff) | ((Result[1] & 0x7ff) << 11) | ((Result[2] & 0x3ff) << 22);

      sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return WD_SUCCESS;
  }
};

class wdImageConversion_R11G11B10_to_FLOAT : public wdImageConversionStepLinear
{
public:
  virtual wdArrayPtr<const wdImageConversionEntry> GetSupportedConversions() const override
  {
    static wdImageConversionEntry supportedConversions[] = {
      wdImageConversionEntry(wdImageFormat::R11G11B10_FLOAT, wdImageFormat::R32G32B32A32_FLOAT, wdImageConversionFlags::Default),
      wdImageConversionEntry(wdImageFormat::R11G11B10_FLOAT, wdImageFormat::R32G32B32_FLOAT, wdImageConversionFlags::Default),
    };

    return supportedConversions;
  }

  virtual wdResult ConvertPixels(wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt64 uiNumElements, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const override
  {
    wdUInt32 sourceStride = wdImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    wdUInt32 targetStride = wdImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      const R11G11B10* pSource = reinterpret_cast<const R11G11B10*>(sourcePointer);
      wdUInt32* targetUi = reinterpret_cast<wdUInt32*>(targetPointer);

      // Adapted from XMLoadFloat3PK
      wdUInt32 Mantissa;
      wdUInt32 Exponent;

      // X Channel (6-bit mantissa)
      Mantissa = pSource->p.xm;

      if (pSource->p.xe == 0x1f) // INF or NAN
      {
        targetUi[0] = 0x7f800000 | (pSource->p.xm << 17);
      }
      else
      {
        if (pSource->p.xe != 0) // The value is normalized
        {
          Exponent = pSource->p.xe;
        }
        else if (Mantissa != 0) // The value is denormalized
        {
          // Normalize the value in the resulting float
          Exponent = 1;

          do
          {
            Exponent--;
            Mantissa <<= 1;
          } while ((Mantissa & 0x40) == 0);

          Mantissa &= 0x3F;
        }
        else // The value is zero
        {
          Exponent = (wdUInt32)-112;
        }

        targetUi[0] = ((Exponent + 112) << 23) | (Mantissa << 17);
      }

      // Y Channel (6-bit mantissa)
      Mantissa = pSource->p.ym;

      if (pSource->p.ye == 0x1f) // INF or NAN
      {
        targetUi[1] = 0x7f800000 | (pSource->p.ym << 17);
      }
      else
      {
        if (pSource->p.ye != 0) // The value is normalized
        {
          Exponent = pSource->p.ye;
        }
        else if (Mantissa != 0) // The value is denormalized
        {
          // Normalize the value in the resulting float
          Exponent = 1;

          do
          {
            Exponent--;
            Mantissa <<= 1;
          } while ((Mantissa & 0x40) == 0);

          Mantissa &= 0x3F;
        }
        else // The value is zero
        {
          Exponent = (wdUInt32)-112;
        }

        targetUi[1] = ((Exponent + 112) << 23) | (Mantissa << 17);
      }

      // Z Channel (5-bit mantissa)
      Mantissa = pSource->p.zm;

      if (pSource->p.ze == 0x1f) // INF or NAN
      {
        targetUi[2] = 0x7f800000 | (pSource->p.zm << 17);
      }
      else
      {
        if (pSource->p.ze != 0) // The value is normalized
        {
          Exponent = pSource->p.ze;
        }
        else if (Mantissa != 0) // The value is denormalized
        {
          // Normalize the value in the resulting float
          Exponent = 1;

          do
          {
            Exponent--;
            Mantissa <<= 1;
          } while ((Mantissa & 0x20) == 0);

          Mantissa &= 0x1F;
        }
        else // The value is zero
        {
          Exponent = (wdUInt32)-112;
        }

        targetUi[2] = ((Exponent + 112) << 23) | (Mantissa << 18);
      }

      if (targetStride > sizeof(float) * 3)
      {
        reinterpret_cast<float*>(targetPointer)[3] = 1.0f; // Write alpha channel
      }
      sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return WD_SUCCESS;
  }
};

class wdImageConversion_R11G11B10_to_HALF : public wdImageConversionStepLinear
{
public:
  virtual wdArrayPtr<const wdImageConversionEntry> GetSupportedConversions() const override
  {
    static wdImageConversionEntry supportedConversions[] = {
      wdImageConversionEntry(wdImageFormat::R11G11B10_FLOAT, wdImageFormat::R16G16B16A16_FLOAT, wdImageConversionFlags::Default)};
    return supportedConversions;
  }

  virtual wdResult ConvertPixels(wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt64 uiNumElements, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const override
  {
    wdUInt32 sourceStride = wdImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    wdUInt32 targetStride = wdImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    while (uiNumElements)
    {
      wdUInt16* result = reinterpret_cast<wdUInt16*>(targetPointer);
      const R11G11B10* r11g11b10 = reinterpret_cast<const R11G11B10*>(sourcePointer);

      // We can do a straight forward conversion here because R11G11B10 uses the same number of bits for the exponent as a half
      // This means that all special values, e.g. denormals, inf, nan map exactly.
      result[0] = static_cast<wdUInt16>((r11g11b10->p.xe << 10) | (r11g11b10->p.xm << 4));
      result[1] = static_cast<wdUInt16>((r11g11b10->p.ye << 10) | (r11g11b10->p.ym << 4));
      result[2] = static_cast<wdUInt16>((r11g11b10->p.ze << 10) | (r11g11b10->p.zm << 5));
      result[3] = 0x3C00; // hex value of 1.0f as half

      sourcePointer = wdMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = wdMemoryUtils::AddByteOffset(targetPointer, targetStride);
      uiNumElements--;
    }

    return WD_SUCCESS;
  }
};



#define ADD_16BPP_CONVERSION(format)                                                                                                                 \
  static wdImageConversionStep_Decompress16bpp<wdDecompress##format, wdImageFormat::format##_UNORM> s_conversion_wdDecompress##format;               \
  static wdImageConversionStep_Compress16bpp<wdCompress##format, wdImageFormat::format##_UNORM> s_conversion_wdCompress##format

ADD_16BPP_CONVERSION(A4B4G4R4);
ADD_16BPP_CONVERSION(B4G4R4A4);
ADD_16BPP_CONVERSION(B5G6R5);
ADD_16BPP_CONVERSION(B5G5R5X1);
ADD_16BPP_CONVERSION(B5G5R5A1);
ADD_16BPP_CONVERSION(X1B5G5R5);
ADD_16BPP_CONVERSION(A1B5G5R5);

static wdImageSwizzleConversion32_2103 s_conversion_swizzle2103;
static wdImageConversion_BGRX_BGRA s_conversion_BGRX_BGRA;
static wdImageConversion_F32_U8 s_conversion_F32_U8;
static wdImageConversion_F32_sRGB s_conversion_F32_sRGB;
static wdImageConversion_F32_U16 s_conversion_F32_U16;
static wdImageConversion_F32_F16 s_conversion_F32_F16;
static wdImageConversion_F32_S8 s_conversion_F32_S8;
static wdImageConversion_U8_F32 s_conversion_U8_F32;
static wdImageConversion_sRGB_F32 s_conversion_sRGB_F32;
static wdImageConversion_U16_F32 s_conversion_U16_F32;
static wdImageConversion_F16_F32 s_conversion_F16_F32;
static wdImageConversion_S8_F32 s_conversion_S8_F32;
static wdImageConversion_Pad_To_RGBA_U8 s_conversion_Pad_To_RGBA_U8;
static wdImageConversion_Pad_To_RGBA_F32 s_conversion_Pad_To_RGBA_F32;
static wdImageConversion_DiscardChannels s_conversion_DiscardChannels;

static wdImageConversion_R11G11B10_to_FLOAT s_conversion_R11G11B10_to_FLOAT;
static wdImageConversion_R11G11B10_to_HALF s_conversion_R11G11B10_to_HALF;
static wdImageConversion_FLOAT_to_R11G11B10 s_conversion_FLOAT_to_R11G11B10;

WD_STATICLINK_FILE(Texture, Texture_Image_Conversions_PixelConversions);
