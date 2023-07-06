
/// \brief Used to guard wdGALDevice functions from multi-threaded access and to verify that executing them on non-main-threads is allowed
#define WD_GALDEVICE_LOCK_AND_CHECK()                                                                                                                \
  WD_LOCK(m_Mutex);                                                                                                                                  \
  VerifyMultithreadedAccess()

WD_ALWAYS_INLINE const wdGALDeviceCreationDescription* wdGALDevice::GetDescription() const
{
  return &m_Description;
}

WD_ALWAYS_INLINE wdResult wdGALDevice::GetTimestampResult(wdGALTimestampHandle hTimestamp, wdTime& ref_result)
{
  return GetTimestampResultPlatform(hTimestamp, ref_result);
}

WD_ALWAYS_INLINE wdGALTimestampHandle wdGALDevice::GetTimestamp()
{
  return GetTimestampPlatform();
}

template <typename IdTableType, typename ReturnType>
WD_ALWAYS_INLINE ReturnType* wdGALDevice::Get(typename IdTableType::TypeOfId hHandle, const IdTableType& IdTable) const
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  ReturnType* pObject = nullptr;
  IdTable.TryGetValue(hHandle, pObject);
  return pObject;
}

inline const wdGALSwapChain* wdGALDevice::GetSwapChain(wdGALSwapChainHandle hSwapChain) const
{
  return Get<SwapChainTable, wdGALSwapChain>(hSwapChain, m_SwapChains);
}

inline const wdGALShader* wdGALDevice::GetShader(wdGALShaderHandle hShader) const
{
  return Get<ShaderTable, wdGALShader>(hShader, m_Shaders);
}

inline const wdGALTexture* wdGALDevice::GetTexture(wdGALTextureHandle hTexture) const
{
  return Get<TextureTable, wdGALTexture>(hTexture, m_Textures);
}

inline const wdGALBuffer* wdGALDevice::GetBuffer(wdGALBufferHandle hBuffer) const
{
  return Get<BufferTable, wdGALBuffer>(hBuffer, m_Buffers);
}

inline const wdGALDepthStencilState* wdGALDevice::GetDepthStencilState(wdGALDepthStencilStateHandle hDepthStencilState) const
{
  return Get<DepthStencilStateTable, wdGALDepthStencilState>(hDepthStencilState, m_DepthStencilStates);
}

inline const wdGALBlendState* wdGALDevice::GetBlendState(wdGALBlendStateHandle hBlendState) const
{
  return Get<BlendStateTable, wdGALBlendState>(hBlendState, m_BlendStates);
}

inline const wdGALRasterizerState* wdGALDevice::GetRasterizerState(wdGALRasterizerStateHandle hRasterizerState) const
{
  return Get<RasterizerStateTable, wdGALRasterizerState>(hRasterizerState, m_RasterizerStates);
}

inline const wdGALVertexDeclaration* wdGALDevice::GetVertexDeclaration(wdGALVertexDeclarationHandle hVertexDeclaration) const
{
  return Get<VertexDeclarationTable, wdGALVertexDeclaration>(hVertexDeclaration, m_VertexDeclarations);
}

inline const wdGALSamplerState* wdGALDevice::GetSamplerState(wdGALSamplerStateHandle hSamplerState) const
{
  return Get<SamplerStateTable, wdGALSamplerState>(hSamplerState, m_SamplerStates);
}

inline const wdGALResourceView* wdGALDevice::GetResourceView(wdGALResourceViewHandle hResourceView) const
{
  return Get<ResourceViewTable, wdGALResourceView>(hResourceView, m_ResourceViews);
}

inline const wdGALRenderTargetView* wdGALDevice::GetRenderTargetView(wdGALRenderTargetViewHandle hRenderTargetView) const
{
  return Get<RenderTargetViewTable, wdGALRenderTargetView>(hRenderTargetView, m_RenderTargetViews);
}

inline const wdGALUnorderedAccessView* wdGALDevice::GetUnorderedAccessView(wdGALUnorderedAccessViewHandle hUnorderedAccessView) const
{
  return Get<UnorderedAccessViewTable, wdGALUnorderedAccessView>(hUnorderedAccessView, m_UnorderedAccessViews);
}

inline const wdGALQuery* wdGALDevice::GetQuery(wdGALQueryHandle hQuery) const
{
  return Get<QueryTable, wdGALQuery>(hQuery, m_Queries);
}

// static
WD_ALWAYS_INLINE void wdGALDevice::SetDefaultDevice(wdGALDevice* pDefaultDevice)
{
  s_pDefaultDevice = pDefaultDevice;
}

// static
WD_ALWAYS_INLINE wdGALDevice* wdGALDevice::GetDefaultDevice()
{
  WD_ASSERT_DEBUG(s_pDefaultDevice != nullptr, "Default device not set.");
  return s_pDefaultDevice;
}

// static
WD_ALWAYS_INLINE bool wdGALDevice::HasDefaultDevice()
{
  return s_pDefaultDevice != nullptr;
}

template <typename HandleType>
WD_FORCE_INLINE void wdGALDevice::AddDeadObject(wdUInt32 uiType, HandleType handle)
{
  auto& deadObject = m_DeadObjects.ExpandAndGetRef();
  deadObject.m_uiType = uiType;
  deadObject.m_uiHandle = handle.GetInternalID().m_Data;
}

template <typename HandleType>
void wdGALDevice::ReviveDeadObject(wdUInt32 uiType, HandleType handle)
{
  wdUInt32 uiHandle = handle.GetInternalID().m_Data;

  for (wdUInt32 i = 0; i < m_DeadObjects.GetCount(); ++i)
  {
    const auto& deadObject = m_DeadObjects[i];

    if (deadObject.m_uiType == uiType && deadObject.m_uiHandle == uiHandle)
    {
      m_DeadObjects.RemoveAtAndCopy(i);
      return;
    }
  }
}

WD_ALWAYS_INLINE void wdGALDevice::VerifyMultithreadedAccess() const
{
#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  WD_ASSERT_DEV(m_Capabilities.m_bMultithreadedResourceCreation || wdThreadUtils::IsMainThread(),
    "This device does not support multi-threaded resource creation, therefore this function can only be executed on the main thread.");
#endif
}
