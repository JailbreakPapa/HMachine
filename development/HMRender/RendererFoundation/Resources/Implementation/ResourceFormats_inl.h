
// static
WD_ALWAYS_INLINE wdUInt32 wdGALResourceFormat::GetBitsPerElement(wdGALResourceFormat::Enum format)
{
  return s_BitsPerElement[format];
}

// static
WD_ALWAYS_INLINE wdUInt8 wdGALResourceFormat::GetChannelCount(wdGALResourceFormat::Enum format)
{
  return s_ChannelCount[format];
}

// static
WD_FORCE_INLINE bool wdGALResourceFormat::IsDepthFormat(wdGALResourceFormat::Enum format)
{
  return format == DFloat || format == D16 || format == D24S8;
}

// static
WD_FORCE_INLINE bool wdGALResourceFormat::IsStencilFormat(Enum format)
{
  return format == D24S8;
}

// static
WD_FORCE_INLINE bool wdGALResourceFormat::IsSrgb(wdGALResourceFormat::Enum format)
{
  return format == BGRAUByteNormalizedsRGB || format == RGBAUByteNormalizedsRGB || format == BC1sRGB || format == BC2sRGB || format == BC3sRGB ||
         format == BC7UNormalizedsRGB;
}


template <typename NativeFormatType, NativeFormatType InvalidFormat>
wdGALFormatLookupEntry<NativeFormatType, InvalidFormat>::wdGALFormatLookupEntry()
  : m_eStorage(InvalidFormat)
  , m_eRenderTarget(InvalidFormat)
  , m_eDepthOnlyType(InvalidFormat)
  , m_eStencilOnlyType(InvalidFormat)
  , m_eDepthStencilType(InvalidFormat)
  , m_eVertexAttributeType(InvalidFormat)
  , m_eResourceViewType(InvalidFormat)
{
}


template <typename NativeFormatType, NativeFormatType InvalidFormat>
wdGALFormatLookupEntry<NativeFormatType, InvalidFormat>::wdGALFormatLookupEntry(NativeFormatType storage)
  : m_eStorage(storage)
  , m_eRenderTarget(InvalidFormat)
  , m_eDepthOnlyType(InvalidFormat)
  , m_eStencilOnlyType(InvalidFormat)
  , m_eDepthStencilType(InvalidFormat)
  , m_eVertexAttributeType(InvalidFormat)
  , m_eResourceViewType(InvalidFormat)
{
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
wdGALFormatLookupEntry<NativeFormatType, InvalidFormat>& wdGALFormatLookupEntry<NativeFormatType, InvalidFormat>::RT(
  NativeFormatType renderTargetType)
{
  m_eRenderTarget = renderTargetType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
wdGALFormatLookupEntry<NativeFormatType, InvalidFormat>& wdGALFormatLookupEntry<NativeFormatType, InvalidFormat>::D(NativeFormatType depthOnlyType)
{
  m_eDepthOnlyType = depthOnlyType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
wdGALFormatLookupEntry<NativeFormatType, InvalidFormat>& wdGALFormatLookupEntry<NativeFormatType, InvalidFormat>::S(NativeFormatType stencilOnlyType)
{
  m_eStencilOnlyType = stencilOnlyType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
wdGALFormatLookupEntry<NativeFormatType, InvalidFormat>& wdGALFormatLookupEntry<NativeFormatType, InvalidFormat>::DS(
  NativeFormatType depthStencilType)
{
  m_eDepthStencilType = depthStencilType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
wdGALFormatLookupEntry<NativeFormatType, InvalidFormat>& wdGALFormatLookupEntry<NativeFormatType, InvalidFormat>::VA(
  NativeFormatType vertexAttributeType)
{
  m_eVertexAttributeType = vertexAttributeType;
  return *this;
}

template <typename NativeFormatType, NativeFormatType InvalidFormat>
wdGALFormatLookupEntry<NativeFormatType, InvalidFormat>& wdGALFormatLookupEntry<NativeFormatType, InvalidFormat>::RV(
  NativeFormatType resourceViewType)
{
  m_eResourceViewType = resourceViewType;
  return *this;
}


template <typename FormatClass>
wdGALFormatLookupTable<FormatClass>::wdGALFormatLookupTable()
{
  for (wdUInt32 i = 0; i < wdGALResourceFormat::ENUM_COUNT; i++)
  {
    m_Formats[i] = FormatClass();
  }
}

template <typename FormatClass>
const FormatClass& wdGALFormatLookupTable<FormatClass>::GetFormatInfo(wdGALResourceFormat::Enum format) const
{
  return m_Formats[format];
}

template <typename FormatClass>
void wdGALFormatLookupTable<FormatClass>::SetFormatInfo(wdGALResourceFormat::Enum format, const FormatClass& newFormatInfo)
{
  m_Formats[format] = newFormatInfo;
}
