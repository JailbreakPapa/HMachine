
#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct WD_RENDERERFOUNDATION_DLL wdGALResourceFormat
{
  using StorageType = wdUInt8;

  enum Enum : wdUInt8
  {
    Invalid = 0,

    RGBAFloat,
    XYZWFloat = RGBAFloat,
    RGBAUInt,
    RGBAInt,

    RGBFloat,
    XYZFloat = RGBFloat,
    UVWFloat = RGBFloat,
    RGBUInt,
    RGBInt,

    B5G6R5UNormalized,
    BGRAUByteNormalized,
    BGRAUByteNormalizedsRGB,

    RGBAHalf,
    XYZWHalf = RGBAHalf,
    RGBAUShort,
    RGBAUShortNormalized,
    RGBAShort,
    RGBAShortNormalized,

    RGFloat,
    XYFloat = RGFloat,
    UVFloat = RGFloat,
    RGUInt,
    RGInt,

    RGB10A2UInt,
    RGB10A2UIntNormalized,
    RG11B10Float,

    RGBAUByteNormalized,
    RGBAUByteNormalizedsRGB,
    RGBAUByte,
    RGBAByteNormalized,
    RGBAByte,

    RGHalf,
    XYHalf = RGHalf,
    UVHalf = RGHalf,
    RGUShort,
    RGUShortNormalized,
    RGShort,
    RGShortNormalized,
    RGUByte,
    RGUByteNormalized,
    RGByte,
    RGByteNormalized,

    DFloat,

    RFloat,
    RUInt,
    RInt,
    RHalf,
    RUShort,
    RUShortNormalized,
    RShort,
    RShortNormalized,
    RUByte,
    RUByteNormalized,
    RByte,
    RByteNormalized,

    AUByteNormalized,

    D16,
    D24S8,

    BC1,
    BC1sRGB,
    BC2,
    BC2sRGB,
    BC3,
    BC3sRGB,
    BC4UNormalized,
    BC4Normalized,
    BC5UNormalized,
    BC5Normalized,
    BC6UFloat,
    BC6Float,
    BC7UNormalized,
    BC7UNormalizedsRGB,

    ENUM_COUNT,

    Default = RGBAUByteNormalizedsRGB
  };


  // General format Meta-Informations:

  /// \brief The size in bits per element (usually pixels, except for mesh stream elements) of a single element of the given resource format.
  static wdUInt32 GetBitsPerElement(wdGALResourceFormat::Enum format);

  /// \brief The number of color channels this format contains.
  static wdUInt8 GetChannelCount(wdGALResourceFormat::Enum format);

  /// \todo A combination of propertyflags, something like srgb, normalized, ...
  // Would be very useful for some GL stuff and Testing.

  /// \brief Returns whether the given resource format is a depth format
  static bool IsDepthFormat(wdGALResourceFormat::Enum format);
  static bool IsStencilFormat(wdGALResourceFormat::Enum format);

  static bool IsSrgb(wdGALResourceFormat::Enum format);

private:
  static const wdUInt8 s_BitsPerElement[wdGALResourceFormat::ENUM_COUNT];

  static const wdUInt8 s_ChannelCount[wdGALResourceFormat::ENUM_COUNT];
};

WD_DECLARE_REFLECTABLE_TYPE(WD_RENDERERFOUNDATION_DLL, wdGALResourceFormat);


template <typename NativeFormatType, NativeFormatType InvalidFormat>
class wdGALFormatLookupEntry
{
public:
  inline wdGALFormatLookupEntry();

  inline wdGALFormatLookupEntry(NativeFormatType storage);

  inline wdGALFormatLookupEntry<NativeFormatType, InvalidFormat>& RT(NativeFormatType renderTargetType);

  inline wdGALFormatLookupEntry<NativeFormatType, InvalidFormat>& D(NativeFormatType depthOnlyType);

  inline wdGALFormatLookupEntry<NativeFormatType, InvalidFormat>& S(NativeFormatType stencilOnlyType);

  inline wdGALFormatLookupEntry<NativeFormatType, InvalidFormat>& DS(NativeFormatType depthStencilType);

  inline wdGALFormatLookupEntry<NativeFormatType, InvalidFormat>& VA(NativeFormatType vertexAttributeType);

  inline wdGALFormatLookupEntry<NativeFormatType, InvalidFormat>& RV(NativeFormatType resourceViewType);

  NativeFormatType m_eStorage;
  NativeFormatType m_eRenderTarget;
  NativeFormatType m_eDepthOnlyType;
  NativeFormatType m_eStencilOnlyType;
  NativeFormatType m_eDepthStencilType;
  NativeFormatType m_eVertexAttributeType;
  NativeFormatType m_eResourceViewType;
};

// Reusable table class to store lookup information (from wdGALResourceFormat to the various formats for texture/buffer storage, views)
template <typename FormatClass>
class wdGALFormatLookupTable
{
public:
  wdGALFormatLookupTable();

  WD_ALWAYS_INLINE const FormatClass& GetFormatInfo(wdGALResourceFormat::Enum format) const;

  WD_ALWAYS_INLINE void SetFormatInfo(wdGALResourceFormat::Enum format, const FormatClass& newFormatInfo);

private:
  FormatClass m_Formats[wdGALResourceFormat::ENUM_COUNT];
};

#include <RendererFoundation/Resources/Implementation/ResourceFormats_inl.h>
