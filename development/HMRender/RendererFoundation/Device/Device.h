
#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/DeviceCapabilities.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class wdColor;

/// \brief The wdRenderDevice class is the primary interface for interactions with rendering APIs
/// It contains a set of (non-virtual) functions to set state, create resources etc. which rely on
/// API specific implementations provided by protected virtual functions.
/// Redundant state changes are prevented at the platform independent level in the non-virtual functions.
class WD_RENDERERFOUNDATION_DLL wdGALDevice
{
public:
  wdEvent<const wdGALDeviceEvent&> m_Events;

  // Init & shutdown functions

  wdResult Init();
  wdResult Shutdown();

  // Pipeline & Pass functions

  void BeginPipeline(const char* szName, wdGALSwapChainHandle hSwapChain);
  void EndPipeline(wdGALSwapChainHandle hSwapChain);

  wdGALPass* BeginPass(const char* szName);
  void EndPass(wdGALPass* pPass);

  // State creation functions

  wdGALBlendStateHandle CreateBlendState(const wdGALBlendStateCreationDescription& description);
  void DestroyBlendState(wdGALBlendStateHandle hBlendState);

  wdGALDepthStencilStateHandle CreateDepthStencilState(const wdGALDepthStencilStateCreationDescription& description);
  void DestroyDepthStencilState(wdGALDepthStencilStateHandle hDepthStencilState);

  wdGALRasterizerStateHandle CreateRasterizerState(const wdGALRasterizerStateCreationDescription& description);
  void DestroyRasterizerState(wdGALRasterizerStateHandle hRasterizerState);

  wdGALSamplerStateHandle CreateSamplerState(const wdGALSamplerStateCreationDescription& description);
  void DestroySamplerState(wdGALSamplerStateHandle hSamplerState);

  // Resource creation functions

  wdGALShaderHandle CreateShader(const wdGALShaderCreationDescription& description);
  void DestroyShader(wdGALShaderHandle hShader);

  wdGALBufferHandle CreateBuffer(const wdGALBufferCreationDescription& description, wdArrayPtr<const wdUInt8> initialData = wdArrayPtr<const wdUInt8>());
  void DestroyBuffer(wdGALBufferHandle hBuffer);

  // Helper functions for buffers (for common, simple use cases)

  wdGALBufferHandle CreateVertexBuffer(wdUInt32 uiVertexSize, wdUInt32 uiVertexCount, wdArrayPtr<const wdUInt8> initialData = wdArrayPtr<const wdUInt8>(), bool bDataIsMutable = false);
  wdGALBufferHandle CreateIndexBuffer(wdGALIndexType::Enum indexType, wdUInt32 uiIndexCount, wdArrayPtr<const wdUInt8> initialData = wdArrayPtr<const wdUInt8>(), bool bDataIsMutable = false);
  wdGALBufferHandle CreateConstantBuffer(wdUInt32 uiBufferSize);

  wdGALTextureHandle CreateTexture(const wdGALTextureCreationDescription& description, wdArrayPtr<wdGALSystemMemoryDescription> initialData = wdArrayPtr<wdGALSystemMemoryDescription>());
  void DestroyTexture(wdGALTextureHandle hTexture);

  wdGALTextureHandle CreateProxyTexture(wdGALTextureHandle hParentTexture, wdUInt32 uiSlice);
  void DestroyProxyTexture(wdGALTextureHandle hProxyTexture);

  // Resource views
  wdGALResourceViewHandle GetDefaultResourceView(wdGALTextureHandle hTexture);
  wdGALResourceViewHandle GetDefaultResourceView(wdGALBufferHandle hBuffer);

  wdGALResourceViewHandle CreateResourceView(const wdGALResourceViewCreationDescription& description);
  void DestroyResourceView(wdGALResourceViewHandle hResourceView);

  // Render target views
  wdGALRenderTargetViewHandle GetDefaultRenderTargetView(wdGALTextureHandle hTexture);

  wdGALRenderTargetViewHandle CreateRenderTargetView(const wdGALRenderTargetViewCreationDescription& description);
  void DestroyRenderTargetView(wdGALRenderTargetViewHandle hRenderTargetView);

  // Unordered access views
  wdGALUnorderedAccessViewHandle CreateUnorderedAccessView(const wdGALUnorderedAccessViewCreationDescription& description);
  void DestroyUnorderedAccessView(wdGALUnorderedAccessViewHandle hUnorderedAccessView);


  // Other rendering creation functions

  using SwapChainFactoryFunction = wdDelegate<wdGALSwapChain*(wdAllocatorBase*)>;
  wdGALSwapChainHandle CreateSwapChain(const SwapChainFactoryFunction& func);
  wdResult UpdateSwapChain(wdGALSwapChainHandle hSwapChain, wdEnum<wdGALPresentMode> newPresentMode);
  void DestroySwapChain(wdGALSwapChainHandle hSwapChain);

  wdGALQueryHandle CreateQuery(const wdGALQueryCreationDescription& description);
  void DestroyQuery(wdGALQueryHandle hQuery);

  wdGALVertexDeclarationHandle CreateVertexDeclaration(const wdGALVertexDeclarationCreationDescription& description);
  void DestroyVertexDeclaration(wdGALVertexDeclarationHandle hVertexDeclaration);

  // Timestamp functions

  wdResult GetTimestampResult(wdGALTimestampHandle hTimestamp, wdTime& ref_result);

  /// \todo Map functions to save on memcpys

  // Swap chain functions

  wdGALTextureHandle GetBackBufferTextureFromSwapChain(wdGALSwapChainHandle hSwapChain);


  // Misc functions

  void BeginFrame(const wdUInt64 uiRenderFrame = 0);
  void EndFrame();

  wdGALTimestampHandle GetTimestamp();

  const wdGALDeviceCreationDescription* GetDescription() const;

  const wdGALSwapChain* GetSwapChain(wdGALSwapChainHandle hSwapChain) const;
  template <typename T>
  const T* GetSwapChain(wdGALSwapChainHandle hSwapChain) const
  {
    return static_cast<const T*>(GetSwapChainInternal(hSwapChain, wdGetStaticRTTI<T>()));
  }

  const wdGALShader* GetShader(wdGALShaderHandle hShader) const;
  const wdGALTexture* GetTexture(wdGALTextureHandle hTexture) const;
  const wdGALBuffer* GetBuffer(wdGALBufferHandle hBuffer) const;
  const wdGALDepthStencilState* GetDepthStencilState(wdGALDepthStencilStateHandle hDepthStencilState) const;
  const wdGALBlendState* GetBlendState(wdGALBlendStateHandle hBlendState) const;
  const wdGALRasterizerState* GetRasterizerState(wdGALRasterizerStateHandle hRasterizerState) const;
  const wdGALVertexDeclaration* GetVertexDeclaration(wdGALVertexDeclarationHandle hVertexDeclaration) const;
  const wdGALSamplerState* GetSamplerState(wdGALSamplerStateHandle hSamplerState) const;
  const wdGALResourceView* GetResourceView(wdGALResourceViewHandle hResourceView) const;
  const wdGALRenderTargetView* GetRenderTargetView(wdGALRenderTargetViewHandle hRenderTargetView) const;
  const wdGALUnorderedAccessView* GetUnorderedAccessView(wdGALUnorderedAccessViewHandle hUnorderedAccessView) const;
  const wdGALQuery* GetQuery(wdGALQueryHandle hQuery) const;

  const wdGALDeviceCapabilities& GetCapabilities() const;

  virtual wdUInt64 GetMemoryConsumptionForTexture(const wdGALTextureCreationDescription& description) const;
  virtual wdUInt64 GetMemoryConsumptionForBuffer(const wdGALBufferCreationDescription& description) const;

  static void SetDefaultDevice(wdGALDevice* pDefaultDevice);
  static wdGALDevice* GetDefaultDevice();
  static bool HasDefaultDevice();

  /// \brief Waits for the GPU to be idle and destroys any pending resources and GPU objects.
  void WaitIdle();

  // public in case someone external needs to lock multiple operations
  mutable wdMutex m_Mutex;

private:
  static wdGALDevice* s_pDefaultDevice;

protected:
  wdGALDevice(const wdGALDeviceCreationDescription& Description);

  virtual ~wdGALDevice();

  template <typename IdTableType, typename ReturnType>
  ReturnType* Get(typename IdTableType::TypeOfId hHandle, const IdTableType& IdTable) const;

  void DestroyViews(wdGALResourceBase* pResource);

  template <typename HandleType>
  void AddDeadObject(wdUInt32 uiType, HandleType handle);

  template <typename HandleType>
  void ReviveDeadObject(wdUInt32 uiType, HandleType handle);

  void DestroyDeadObjects();

  /// \brief Asserts that either this device supports multi-threaded resource creation, or that this function is executed on the main thread.
  void VerifyMultithreadedAccess() const;

  const wdGALSwapChain* GetSwapChainInternal(wdGALSwapChainHandle hSwapChain, const wdRTTI* pRequestedType) const;

  wdGALTextureHandle FinalizeTextureInternal(const wdGALTextureCreationDescription& desc, wdGALTexture* pTexture);
  wdGALBufferHandle FinalizeBufferInternal(const wdGALBufferCreationDescription& desc, wdGALBuffer* pBuffer);

  wdProxyAllocator m_Allocator;
  wdLocalAllocatorWrapper m_AllocatorWrapper;

  using ShaderTable = wdIdTable<wdGALShaderHandle::IdType, wdGALShader*, wdLocalAllocatorWrapper>;
  using BlendStateTable = wdIdTable<wdGALBlendStateHandle::IdType, wdGALBlendState*, wdLocalAllocatorWrapper>;
  using DepthStencilStateTable = wdIdTable<wdGALDepthStencilStateHandle::IdType, wdGALDepthStencilState*, wdLocalAllocatorWrapper>;
  using RasterizerStateTable = wdIdTable<wdGALRasterizerStateHandle::IdType, wdGALRasterizerState*, wdLocalAllocatorWrapper>;
  using BufferTable = wdIdTable<wdGALBufferHandle::IdType, wdGALBuffer*, wdLocalAllocatorWrapper>;
  using TextureTable = wdIdTable<wdGALTextureHandle::IdType, wdGALTexture*, wdLocalAllocatorWrapper>;
  using ResourceViewTable = wdIdTable<wdGALResourceViewHandle::IdType, wdGALResourceView*, wdLocalAllocatorWrapper>;
  using SamplerStateTable = wdIdTable<wdGALSamplerStateHandle::IdType, wdGALSamplerState*, wdLocalAllocatorWrapper>;
  using RenderTargetViewTable = wdIdTable<wdGALRenderTargetViewHandle::IdType, wdGALRenderTargetView*, wdLocalAllocatorWrapper>;
  using UnorderedAccessViewTable = wdIdTable<wdGALUnorderedAccessViewHandle::IdType, wdGALUnorderedAccessView*, wdLocalAllocatorWrapper>;
  using SwapChainTable = wdIdTable<wdGALSwapChainHandle::IdType, wdGALSwapChain*, wdLocalAllocatorWrapper>;
  using QueryTable = wdIdTable<wdGALQueryHandle::IdType, wdGALQuery*, wdLocalAllocatorWrapper>;
  using VertexDeclarationTable = wdIdTable<wdGALVertexDeclarationHandle::IdType, wdGALVertexDeclaration*, wdLocalAllocatorWrapper>;

  ShaderTable m_Shaders;
  BlendStateTable m_BlendStates;
  DepthStencilStateTable m_DepthStencilStates;
  RasterizerStateTable m_RasterizerStates;
  BufferTable m_Buffers;
  TextureTable m_Textures;
  ResourceViewTable m_ResourceViews;
  SamplerStateTable m_SamplerStates;
  RenderTargetViewTable m_RenderTargetViews;
  UnorderedAccessViewTable m_UnorderedAccessViews;
  SwapChainTable m_SwapChains;
  QueryTable m_Queries;
  VertexDeclarationTable m_VertexDeclarations;


  // Hash tables used to prevent state object duplication
  wdHashTable<wdUInt32, wdGALBlendStateHandle, wdHashHelper<wdUInt32>, wdLocalAllocatorWrapper> m_BlendStateTable;
  wdHashTable<wdUInt32, wdGALDepthStencilStateHandle, wdHashHelper<wdUInt32>, wdLocalAllocatorWrapper> m_DepthStencilStateTable;
  wdHashTable<wdUInt32, wdGALRasterizerStateHandle, wdHashHelper<wdUInt32>, wdLocalAllocatorWrapper> m_RasterizerStateTable;
  wdHashTable<wdUInt32, wdGALSamplerStateHandle, wdHashHelper<wdUInt32>, wdLocalAllocatorWrapper> m_SamplerStateTable;
  wdHashTable<wdUInt32, wdGALVertexDeclarationHandle, wdHashHelper<wdUInt32>, wdLocalAllocatorWrapper> m_VertexDeclarationTable;

  struct DeadObject
  {
    WD_DECLARE_POD_TYPE();

    wdUInt32 m_uiType;
    wdUInt32 m_uiHandle;
  };

  wdDynamicArray<DeadObject, wdLocalAllocatorWrapper> m_DeadObjects;

  wdGALDeviceCreationDescription m_Description;

  wdGALDeviceCapabilities m_Capabilities;

  // Deactivate Doxygen document generation for the following block. (API abstraction only)
  /// \cond

  // These functions need to be implemented by a render API abstraction
protected:
  friend class wdMemoryUtils;

  // Init & shutdown functions

  virtual wdResult InitPlatform() = 0;
  virtual wdResult ShutdownPlatform() = 0;

  // Pipeline & Pass functions

  virtual void BeginPipelinePlatform(const char* szName, wdGALSwapChain* pSwapChain) = 0;
  virtual void EndPipelinePlatform(wdGALSwapChain* pSwapChain) = 0;

  virtual wdGALPass* BeginPassPlatform(const char* szName) = 0;
  virtual void EndPassPlatform(wdGALPass* pPass) = 0;

  // State creation functions

  virtual wdGALBlendState* CreateBlendStatePlatform(const wdGALBlendStateCreationDescription& Description) = 0;
  virtual void DestroyBlendStatePlatform(wdGALBlendState* pBlendState) = 0;

  virtual wdGALDepthStencilState* CreateDepthStencilStatePlatform(const wdGALDepthStencilStateCreationDescription& Description) = 0;
  virtual void DestroyDepthStencilStatePlatform(wdGALDepthStencilState* pDepthStencilState) = 0;

  virtual wdGALRasterizerState* CreateRasterizerStatePlatform(const wdGALRasterizerStateCreationDescription& Description) = 0;
  virtual void DestroyRasterizerStatePlatform(wdGALRasterizerState* pRasterizerState) = 0;

  virtual wdGALSamplerState* CreateSamplerStatePlatform(const wdGALSamplerStateCreationDescription& Description) = 0;
  virtual void DestroySamplerStatePlatform(wdGALSamplerState* pSamplerState) = 0;

  // Resource creation functions

  virtual wdGALShader* CreateShaderPlatform(const wdGALShaderCreationDescription& Description) = 0;
  virtual void DestroyShaderPlatform(wdGALShader* pShader) = 0;

  virtual wdGALBuffer* CreateBufferPlatform(const wdGALBufferCreationDescription& Description, wdArrayPtr<const wdUInt8> pInitialData) = 0;
  virtual void DestroyBufferPlatform(wdGALBuffer* pBuffer) = 0;

  virtual wdGALTexture* CreateTexturePlatform(const wdGALTextureCreationDescription& Description, wdArrayPtr<wdGALSystemMemoryDescription> pInitialData) = 0;
  virtual void DestroyTexturePlatform(wdGALTexture* pTexture) = 0;

  virtual wdGALResourceView* CreateResourceViewPlatform(wdGALResourceBase* pResource, const wdGALResourceViewCreationDescription& Description) = 0;
  virtual void DestroyResourceViewPlatform(wdGALResourceView* pResourceView) = 0;

  virtual wdGALRenderTargetView* CreateRenderTargetViewPlatform(wdGALTexture* pTexture, const wdGALRenderTargetViewCreationDescription& Description) = 0;
  virtual void DestroyRenderTargetViewPlatform(wdGALRenderTargetView* pRenderTargetView) = 0;

  virtual wdGALUnorderedAccessView* CreateUnorderedAccessViewPlatform(wdGALResourceBase* pResource, const wdGALUnorderedAccessViewCreationDescription& Description) = 0;
  virtual void DestroyUnorderedAccessViewPlatform(wdGALUnorderedAccessView* pUnorderedAccessView) = 0;

  // Other rendering creation functions

  virtual wdGALQuery* CreateQueryPlatform(const wdGALQueryCreationDescription& Description) = 0;
  virtual void DestroyQueryPlatform(wdGALQuery* pQuery) = 0;

  virtual wdGALVertexDeclaration* CreateVertexDeclarationPlatform(const wdGALVertexDeclarationCreationDescription& Description) = 0;
  virtual void DestroyVertexDeclarationPlatform(wdGALVertexDeclaration* pVertexDeclaration) = 0;

  // Timestamp functions

  virtual wdGALTimestampHandle GetTimestampPlatform() = 0;
  virtual wdResult GetTimestampResultPlatform(wdGALTimestampHandle hTimestamp, wdTime& result) = 0;

  // Misc functions

  virtual void BeginFramePlatform(const wdUInt64 uiRenderFrame) = 0;
  virtual void EndFramePlatform() = 0;

  virtual void FillCapabilitiesPlatform() = 0;

  virtual void WaitIdlePlatform() = 0;


  /// \endcond

private:
  bool m_bBeginFrameCalled = false;
  bool m_bBeginPipelineCalled = false;
  bool m_bBeginPassCalled = false;
};

#include <RendererFoundation/Device/Implementation/Device_inl.h>
