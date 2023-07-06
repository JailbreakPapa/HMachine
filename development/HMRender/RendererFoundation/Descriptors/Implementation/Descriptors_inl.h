inline bool wdShaderResourceType::IsArray(wdShaderResourceType::Enum format)
{
  switch (format)
  {
    case wdShaderResourceType::Texture1DArray:
    case wdShaderResourceType::Texture2DArray:
    case wdShaderResourceType::Texture2DMSArray:
    case wdShaderResourceType::TextureCubeArray:
      return true;
    default:
      return false;
  }
}

inline wdGALShaderCreationDescription::wdGALShaderCreationDescription()
  : wdHashableStruct()
{
}

inline wdGALShaderCreationDescription::~wdGALShaderCreationDescription()
{
  for (wdUInt32 i = 0; i < wdGALShaderStage::ENUM_COUNT; ++i)
  {
    wdGALShaderByteCode* pByteCode = m_ByteCodes[i];
    m_ByteCodes[i] = nullptr;

    if (pByteCode != nullptr && pByteCode->GetRefCount() == 0)
    {
      WD_DEFAULT_DELETE(pByteCode);
    }
  }
}

inline bool wdGALShaderCreationDescription::HasByteCodeForStage(wdGALShaderStage::Enum stage) const
{
  return m_ByteCodes[stage] != nullptr && m_ByteCodes[stage]->IsValid();
}

inline void wdGALTextureCreationDescription::SetAsRenderTarget(
  wdUInt32 uiWidth, wdUInt32 uiHeight, wdGALResourceFormat::Enum format, wdGALMSAASampleCount::Enum sampleCount /*= wdGALMSAASampleCount::None*/)
{
  m_uiWidth = uiWidth;
  m_uiHeight = uiHeight;
  m_uiDepth = 1;
  m_uiMipLevelCount = 1;
  m_uiArraySize = 1;
  m_SampleCount = sampleCount;
  m_Format = format;
  m_Type = wdGALTextureType::Texture2D;
  m_bAllowShaderResourceView = true;
  m_bAllowUAV = false;
  m_bCreateRenderTarget = true;
  m_bAllowDynamicMipGeneration = false;
  m_ResourceAccess.m_bReadBack = false;
  m_ResourceAccess.m_bImmutable = true;
  m_pExisitingNativeObject = nullptr;
}

WD_FORCE_INLINE wdGALVertexAttribute::wdGALVertexAttribute(
  wdGALVertexAttributeSemantic::Enum semantic, wdGALResourceFormat::Enum format, wdUInt16 uiOffset, wdUInt8 uiVertexBufferSlot, bool bInstanceData)
  : m_eSemantic(semantic)
  , m_eFormat(format)
  , m_uiOffset(uiOffset)
  , m_uiVertexBufferSlot(uiVertexBufferSlot)
  , m_bInstanceData(bInstanceData)
{
}
