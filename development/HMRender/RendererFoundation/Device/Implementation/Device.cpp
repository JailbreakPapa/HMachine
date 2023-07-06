#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/ProxyTexture.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/ResourceView.h>
#include <RendererFoundation/Resources/UnorderedAccesView.h>
#include <RendererFoundation/Shader/VertexDeclaration.h>
#include <RendererFoundation/State/State.h>

namespace
{
  struct GALObjectType
  {
    enum Enum
    {
      BlendState,
      DepthStencilState,
      RasterizerState,
      SamplerState,
      Shader,
      Buffer,
      Texture,
      ResourceView,
      RenderTargetView,
      UnorderedAccessView,
      SwapChain,
      Query,
      VertexDeclaration
    };
  };

  WD_CHECK_AT_COMPILETIME(sizeof(wdGALBlendStateHandle) == sizeof(wdUInt32));
  WD_CHECK_AT_COMPILETIME(sizeof(wdGALDepthStencilStateHandle) == sizeof(wdUInt32));
  WD_CHECK_AT_COMPILETIME(sizeof(wdGALRasterizerStateHandle) == sizeof(wdUInt32));
  WD_CHECK_AT_COMPILETIME(sizeof(wdGALSamplerStateHandle) == sizeof(wdUInt32));
  WD_CHECK_AT_COMPILETIME(sizeof(wdGALShaderHandle) == sizeof(wdUInt32));
  WD_CHECK_AT_COMPILETIME(sizeof(wdGALBufferHandle) == sizeof(wdUInt32));
  WD_CHECK_AT_COMPILETIME(sizeof(wdGALTextureHandle) == sizeof(wdUInt32));
  WD_CHECK_AT_COMPILETIME(sizeof(wdGALResourceViewHandle) == sizeof(wdUInt32));
  WD_CHECK_AT_COMPILETIME(sizeof(wdGALRenderTargetViewHandle) == sizeof(wdUInt32));
  WD_CHECK_AT_COMPILETIME(sizeof(wdGALUnorderedAccessViewHandle) == sizeof(wdUInt32));
  WD_CHECK_AT_COMPILETIME(sizeof(wdGALSwapChainHandle) == sizeof(wdUInt32));
  WD_CHECK_AT_COMPILETIME(sizeof(wdGALQueryHandle) == sizeof(wdUInt32));
  WD_CHECK_AT_COMPILETIME(sizeof(wdGALVertexDeclarationHandle) == sizeof(wdUInt32));
} // namespace

wdGALDevice* wdGALDevice::s_pDefaultDevice = nullptr;


wdGALDevice::wdGALDevice(const wdGALDeviceCreationDescription& desc)
  : m_Allocator("GALDevice", wdFoundation::GetDefaultAllocator())
  , m_AllocatorWrapper(&m_Allocator)
  , m_Description(desc)
{
}

wdGALDevice::~wdGALDevice()
{
  // Check for object leaks
  {
    WD_LOG_BLOCK("wdGALDevice object leak report");

    if (!m_Shaders.IsEmpty())
      wdLog::Warning("{0} shaders have not been cleaned up", m_Shaders.GetCount());

    if (!m_BlendStates.IsEmpty())
      wdLog::Warning("{0} blend states have not been cleaned up", m_BlendStates.GetCount());

    if (!m_DepthStencilStates.IsEmpty())
      wdLog::Warning("{0} depth stencil states have not been cleaned up", m_DepthStencilStates.GetCount());

    if (!m_RasterizerStates.IsEmpty())
      wdLog::Warning("{0} rasterizer states have not been cleaned up", m_RasterizerStates.GetCount());

    if (!m_Buffers.IsEmpty())
      wdLog::Warning("{0} buffers have not been cleaned up", m_Buffers.GetCount());

    if (!m_Textures.IsEmpty())
      wdLog::Warning("{0} textures have not been cleaned up", m_Textures.GetCount());

    if (!m_ResourceViews.IsEmpty())
      wdLog::Warning("{0} resource views have not been cleaned up", m_ResourceViews.GetCount());

    if (!m_RenderTargetViews.IsEmpty())
      wdLog::Warning("{0} render target views have not been cleaned up", m_RenderTargetViews.GetCount());

    if (!m_UnorderedAccessViews.IsEmpty())
      wdLog::Warning("{0} unordered access views have not been cleaned up", m_UnorderedAccessViews.GetCount());

    if (!m_SwapChains.IsEmpty())
      wdLog::Warning("{0} swap chains have not been cleaned up", m_SwapChains.GetCount());

    if (!m_Queries.IsEmpty())
      wdLog::Warning("{0} queries have not been cleaned up", m_Queries.GetCount());

    if (!m_VertexDeclarations.IsEmpty())
      wdLog::Warning("{0} vertex declarations have not been cleaned up", m_VertexDeclarations.GetCount());
  }
}

wdResult wdGALDevice::Init()
{
  WD_LOG_BLOCK("wdGALDevice::Init");

  wdResult PlatformInitResult = InitPlatform();

  if (PlatformInitResult == WD_FAILURE)
  {
    return WD_FAILURE;
  }

  // Fill the capabilities
  FillCapabilitiesPlatform();

  wdLog::Info("Adapter: '{}' - {} VRAM, {} Sys RAM, {} Shared RAM", m_Capabilities.m_sAdapterName, wdArgFileSize(m_Capabilities.m_uiDedicatedVRAM),
    wdArgFileSize(m_Capabilities.m_uiDedicatedSystemRAM), wdArgFileSize(m_Capabilities.m_uiSharedSystemRAM));

  if (!m_Capabilities.m_bHardwareAccelerated)
  {
    wdLog::Warning("Selected graphics adapter has no hardware acceleration.");
  }

  WD_GALDEVICE_LOCK_AND_CHECK();

  wdProfilingSystem::InitializeGPUData();

  {
    wdGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = wdGALDeviceEvent::AfterInit;
    m_Events.Broadcast(e);
  }

  return WD_SUCCESS;
}

wdResult wdGALDevice::Shutdown()
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  WD_LOG_BLOCK("wdGALDevice::Shutdown");

  {
    wdGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = wdGALDeviceEvent::BeforeShutdown;
    m_Events.Broadcast(e);
  }

  DestroyDeadObjects();

  // make sure we are not listed as the default device anymore
  if (wdGALDevice::HasDefaultDevice() && wdGALDevice::GetDefaultDevice() == this)
  {
    wdGALDevice::SetDefaultDevice(nullptr);
  }

  return ShutdownPlatform();
}

void wdGALDevice::BeginPipeline(const char* szName, wdGALSwapChainHandle hSwapChain)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  WD_ASSERT_DEV(!m_bBeginPipelineCalled, "Nested Pipelines are not allowed: You must call wdGALDevice::EndPipeline before you can call wdGALDevice::BeginPipeline again");
  m_bBeginPipelineCalled = true;

  wdGALSwapChain* pSwapChain = nullptr;
  m_SwapChains.TryGetValue(hSwapChain, pSwapChain);
  BeginPipelinePlatform(szName, pSwapChain);
}

void wdGALDevice::EndPipeline(wdGALSwapChainHandle hSwapChain)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  WD_ASSERT_DEV(m_bBeginPipelineCalled, "You must have called wdGALDevice::BeginPipeline before you can call wdGALDevice::EndPipeline");
  m_bBeginPipelineCalled = false;

  wdGALSwapChain* pSwapChain = nullptr;
  m_SwapChains.TryGetValue(hSwapChain, pSwapChain);
  EndPipelinePlatform(pSwapChain);
}

wdGALPass* wdGALDevice::BeginPass(const char* szName)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  WD_ASSERT_DEV(!m_bBeginPassCalled, "Nested Passes are not allowed: You must call wdGALDevice::EndPass before you can call wdGALDevice::BeginPass again");
  m_bBeginPassCalled = true;

  return BeginPassPlatform(szName);
}

void wdGALDevice::EndPass(wdGALPass* pPass)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  WD_ASSERT_DEV(m_bBeginPassCalled, "You must have called wdGALDevice::BeginPass before you can call wdGALDevice::EndPass");
  m_bBeginPassCalled = false;

  EndPassPlatform(pPass);
}

wdGALBlendStateHandle wdGALDevice::CreateBlendState(const wdGALBlendStateCreationDescription& desc)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  // Hash desc and return potential existing one (including inc. refcount)
  wdUInt32 uiHash = desc.CalculateHash();

  {
    wdGALBlendStateHandle hBlendState;
    if (m_BlendStateTable.TryGetValue(uiHash, hBlendState))
    {
      wdGALBlendState* pBlendState = m_BlendStates[hBlendState];
      if (pBlendState->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::BlendState, hBlendState);
      }

      pBlendState->AddRef();
      return hBlendState;
    }
  }

  wdGALBlendState* pBlendState = CreateBlendStatePlatform(desc);

  if (pBlendState != nullptr)
  {
    WD_ASSERT_DEBUG(pBlendState->GetDescription().CalculateHash() == uiHash, "BlendState hash doesn't match");

    pBlendState->AddRef();

    wdGALBlendStateHandle hBlendState(m_BlendStates.Insert(pBlendState));
    m_BlendStateTable.Insert(uiHash, hBlendState);

    return hBlendState;
  }

  return wdGALBlendStateHandle();
}

void wdGALDevice::DestroyBlendState(wdGALBlendStateHandle hBlendState)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  wdGALBlendState* pBlendState = nullptr;

  if (m_BlendStates.TryGetValue(hBlendState, pBlendState))
  {
    pBlendState->ReleaseRef();

    if (pBlendState->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::BlendState, hBlendState);
    }
  }
  else
  {
    wdLog::Warning("DestroyBlendState called on invalid handle (double free?)");
  }
}

wdGALDepthStencilStateHandle wdGALDevice::CreateDepthStencilState(const wdGALDepthStencilStateCreationDescription& desc)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  // Hash desc and return potential existing one (including inc. refcount)
  wdUInt32 uiHash = desc.CalculateHash();

  {
    wdGALDepthStencilStateHandle hDepthStencilState;
    if (m_DepthStencilStateTable.TryGetValue(uiHash, hDepthStencilState))
    {
      wdGALDepthStencilState* pDepthStencilState = m_DepthStencilStates[hDepthStencilState];
      if (pDepthStencilState->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::DepthStencilState, hDepthStencilState);
      }

      pDepthStencilState->AddRef();
      return hDepthStencilState;
    }
  }

  wdGALDepthStencilState* pDepthStencilState = CreateDepthStencilStatePlatform(desc);

  if (pDepthStencilState != nullptr)
  {
    WD_ASSERT_DEBUG(pDepthStencilState->GetDescription().CalculateHash() == uiHash, "DepthStencilState hash doesn't match");

    pDepthStencilState->AddRef();

    wdGALDepthStencilStateHandle hDepthStencilState(m_DepthStencilStates.Insert(pDepthStencilState));
    m_DepthStencilStateTable.Insert(uiHash, hDepthStencilState);

    return hDepthStencilState;
  }

  return wdGALDepthStencilStateHandle();
}

void wdGALDevice::DestroyDepthStencilState(wdGALDepthStencilStateHandle hDepthStencilState)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  wdGALDepthStencilState* pDepthStencilState = nullptr;

  if (m_DepthStencilStates.TryGetValue(hDepthStencilState, pDepthStencilState))
  {
    pDepthStencilState->ReleaseRef();

    if (pDepthStencilState->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::DepthStencilState, hDepthStencilState);
    }
  }
  else
  {
    wdLog::Warning("DestroyDepthStencilState called on invalid handle (double free?)");
  }
}

wdGALRasterizerStateHandle wdGALDevice::CreateRasterizerState(const wdGALRasterizerStateCreationDescription& desc)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  // Hash desc and return potential existing one (including inc. refcount)
  wdUInt32 uiHash = desc.CalculateHash();

  {
    wdGALRasterizerStateHandle hRasterizerState;
    if (m_RasterizerStateTable.TryGetValue(uiHash, hRasterizerState))
    {
      wdGALRasterizerState* pRasterizerState = m_RasterizerStates[hRasterizerState];
      if (pRasterizerState->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::RasterizerState, hRasterizerState);
      }

      pRasterizerState->AddRef();
      return hRasterizerState;
    }
  }

  wdGALRasterizerState* pRasterizerState = CreateRasterizerStatePlatform(desc);

  if (pRasterizerState != nullptr)
  {
    WD_ASSERT_DEBUG(pRasterizerState->GetDescription().CalculateHash() == uiHash, "RasterizerState hash doesn't match");

    pRasterizerState->AddRef();

    wdGALRasterizerStateHandle hRasterizerState(m_RasterizerStates.Insert(pRasterizerState));
    m_RasterizerStateTable.Insert(uiHash, hRasterizerState);

    return hRasterizerState;
  }

  return wdGALRasterizerStateHandle();
}

void wdGALDevice::DestroyRasterizerState(wdGALRasterizerStateHandle hRasterizerState)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  wdGALRasterizerState* pRasterizerState = nullptr;

  if (m_RasterizerStates.TryGetValue(hRasterizerState, pRasterizerState))
  {
    pRasterizerState->ReleaseRef();

    if (pRasterizerState->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::RasterizerState, hRasterizerState);
    }
  }
  else
  {
    wdLog::Warning("DestroyRasterizerState called on invalid handle (double free?)");
  }
}

wdGALSamplerStateHandle wdGALDevice::CreateSamplerState(const wdGALSamplerStateCreationDescription& desc)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  /// \todo Platform independent validation

  // Hash desc and return potential existing one (including inc. refcount)
  wdUInt32 uiHash = desc.CalculateHash();

  {
    wdGALSamplerStateHandle hSamplerState;
    if (m_SamplerStateTable.TryGetValue(uiHash, hSamplerState))
    {
      wdGALSamplerState* pSamplerState = m_SamplerStates[hSamplerState];
      if (pSamplerState->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::SamplerState, hSamplerState);
      }

      pSamplerState->AddRef();
      return hSamplerState;
    }
  }

  wdGALSamplerState* pSamplerState = CreateSamplerStatePlatform(desc);

  if (pSamplerState != nullptr)
  {
    WD_ASSERT_DEBUG(pSamplerState->GetDescription().CalculateHash() == uiHash, "SamplerState hash doesn't match");

    pSamplerState->AddRef();

    wdGALSamplerStateHandle hSamplerState(m_SamplerStates.Insert(pSamplerState));
    m_SamplerStateTable.Insert(uiHash, hSamplerState);

    return hSamplerState;
  }

  return wdGALSamplerStateHandle();
}

void wdGALDevice::DestroySamplerState(wdGALSamplerStateHandle hSamplerState)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  wdGALSamplerState* pSamplerState = nullptr;

  if (m_SamplerStates.TryGetValue(hSamplerState, pSamplerState))
  {
    pSamplerState->ReleaseRef();

    if (pSamplerState->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::SamplerState, hSamplerState);
    }
  }
  else
  {
    wdLog::Warning("DestroySamplerState called on invalid handle (double free?)");
  }
}



wdGALShaderHandle wdGALDevice::CreateShader(const wdGALShaderCreationDescription& desc)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  bool bHasByteCodes = false;

  for (wdUInt32 uiStage = 0; uiStage < wdGALShaderStage::ENUM_COUNT; uiStage++)
  {
    if (desc.HasByteCodeForStage((wdGALShaderStage::Enum)uiStage))
    {
      bHasByteCodes = true;
      break;
    }
  }

  if (!bHasByteCodes)
  {
    wdLog::Error("Can't create a shader which supplies no bytecodes at all!");
    return wdGALShaderHandle();
  }

  wdGALShader* pShader = CreateShaderPlatform(desc);

  if (pShader == nullptr)
  {
    return wdGALShaderHandle();
  }
  else
  {
    return wdGALShaderHandle(m_Shaders.Insert(pShader));
  }
}

void wdGALDevice::DestroyShader(wdGALShaderHandle hShader)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  wdGALShader* pShader = nullptr;

  if (m_Shaders.TryGetValue(hShader, pShader))
  {
    AddDeadObject(GALObjectType::Shader, hShader);
  }
  else
  {
    wdLog::Warning("DestroyShader called on invalid handle (double free?)");
  }
}


wdGALBufferHandle wdGALDevice::CreateBuffer(const wdGALBufferCreationDescription& desc, wdArrayPtr<const wdUInt8> initialData)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  if (desc.m_uiTotalSize == 0)
  {
    wdLog::Error("Trying to create a buffer with size of 0 is not possible!");
    return wdGALBufferHandle();
  }

  if (desc.m_ResourceAccess.IsImmutable())
  {
    if (initialData.IsEmpty())
    {
      wdLog::Error("Trying to create an immutable buffer but not supplying initial data is not possible!");
      return wdGALBufferHandle();
    }

    wdUInt32 uiBufferSize = desc.m_uiTotalSize;
    if (uiBufferSize != initialData.GetCount())
    {
      wdLog::Error("Trying to create a buffer with invalid initial data!");
      return wdGALBufferHandle();
    }
  }

  /// \todo Platform independent validation (buffer type supported)

  wdGALBuffer* pBuffer = CreateBufferPlatform(desc, initialData);

  return FinalizeBufferInternal(desc, pBuffer);
}

wdGALBufferHandle wdGALDevice::FinalizeBufferInternal(const wdGALBufferCreationDescription& desc, wdGALBuffer* pBuffer)
{
  if (pBuffer != nullptr)
  {
    wdGALBufferHandle hBuffer(m_Buffers.Insert(pBuffer));

    // Create default resource view
    if (desc.m_bAllowShaderResourceView && desc.m_BufferType == wdGALBufferType::Generic)
    {
      wdGALResourceViewCreationDescription viewDesc;
      viewDesc.m_hBuffer = hBuffer;
      viewDesc.m_uiFirstElement = 0;
      viewDesc.m_uiNumElements = (desc.m_uiStructSize != 0) ? (desc.m_uiTotalSize / desc.m_uiStructSize) : desc.m_uiTotalSize;

      pBuffer->m_hDefaultResourceView = CreateResourceView(viewDesc);
    }

    return hBuffer;
  }

  return wdGALBufferHandle();
}

void wdGALDevice::DestroyBuffer(wdGALBufferHandle hBuffer)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  wdGALBuffer* pBuffer = nullptr;

  if (m_Buffers.TryGetValue(hBuffer, pBuffer))
  {
    AddDeadObject(GALObjectType::Buffer, hBuffer);
  }
  else
  {
    wdLog::Warning("DestroyBuffer called on invalid handle (double free?)");
  }
}

// Helper functions for buffers (for common, simple use cases)
wdGALBufferHandle wdGALDevice::CreateVertexBuffer(wdUInt32 uiVertexSize, wdUInt32 uiVertexCount, wdArrayPtr<const wdUInt8> initialData, bool bDataIsMutable /*= false */)
{
  wdGALBufferCreationDescription desc;
  desc.m_uiStructSize = uiVertexSize;
  desc.m_uiTotalSize = uiVertexSize * uiVertexCount;
  desc.m_BufferType = wdGALBufferType::VertexBuffer;
  desc.m_ResourceAccess.m_bImmutable = !initialData.IsEmpty() && !bDataIsMutable;

  return CreateBuffer(desc, initialData);
}

wdGALBufferHandle wdGALDevice::CreateIndexBuffer(wdGALIndexType::Enum indexType, wdUInt32 uiIndexCount, wdArrayPtr<const wdUInt8> initialData, bool bDataIsMutable /*= false*/)
{
  wdGALBufferCreationDescription desc;
  desc.m_uiStructSize = wdGALIndexType::GetSize(indexType);
  desc.m_uiTotalSize = desc.m_uiStructSize * uiIndexCount;
  desc.m_BufferType = wdGALBufferType::IndexBuffer;
  desc.m_ResourceAccess.m_bImmutable = !bDataIsMutable && !initialData.IsEmpty();

  return CreateBuffer(desc, initialData);
}

wdGALBufferHandle wdGALDevice::CreateConstantBuffer(wdUInt32 uiBufferSize)
{
  wdGALBufferCreationDescription desc;
  desc.m_uiStructSize = 0;
  desc.m_uiTotalSize = uiBufferSize;
  desc.m_BufferType = wdGALBufferType::ConstantBuffer;
  desc.m_ResourceAccess.m_bImmutable = false;

  return CreateBuffer(desc);
}


wdGALTextureHandle wdGALDevice::CreateTexture(const wdGALTextureCreationDescription& desc, wdArrayPtr<wdGALSystemMemoryDescription> initialData)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  /// \todo Platform independent validation (desc width & height < platform maximum, format, etc.)

  if (desc.m_ResourceAccess.IsImmutable() && (initialData.IsEmpty() || initialData.GetCount() < desc.m_uiMipLevelCount) &&
      !desc.m_bCreateRenderTarget)
  {
    wdLog::Error("Trying to create an immutable texture but not supplying initial data (or not enough data pointers) is not possible!");
    return wdGALTextureHandle();
  }

  if (desc.m_uiWidth == 0 || desc.m_uiHeight == 0)
  {
    wdLog::Error("Trying to create a texture with width or height == 0 is not possible!");
    return wdGALTextureHandle();
  }

  wdGALTexture* pTexture = CreateTexturePlatform(desc, initialData);

  return FinalizeTextureInternal(desc, pTexture);
}

wdGALTextureHandle wdGALDevice::FinalizeTextureInternal(const wdGALTextureCreationDescription& desc, wdGALTexture* pTexture)
{
  if (pTexture != nullptr)
  {
    wdGALTextureHandle hTexture(m_Textures.Insert(pTexture));

    // Create default resource view
    if (desc.m_bAllowShaderResourceView)
    {
      wdGALResourceViewCreationDescription viewDesc;
      viewDesc.m_hTexture = hTexture;
      viewDesc.m_uiArraySize = desc.m_uiArraySize;
      pTexture->m_hDefaultResourceView = CreateResourceView(viewDesc);
    }

    // Create default render target view
    if (desc.m_bCreateRenderTarget)
    {
      wdGALRenderTargetViewCreationDescription rtDesc;
      rtDesc.m_hTexture = hTexture;
      rtDesc.m_uiFirstSlice = 0;
      rtDesc.m_uiSliceCount = desc.m_uiArraySize;

      pTexture->m_hDefaultRenderTargetView = CreateRenderTargetView(rtDesc);
    }

    return hTexture;
  }

  return wdGALTextureHandle();
}

void wdGALDevice::DestroyTexture(wdGALTextureHandle hTexture)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  wdGALTexture* pTexture = nullptr;
  if (m_Textures.TryGetValue(hTexture, pTexture))
  {
    AddDeadObject(GALObjectType::Texture, hTexture);
  }
  else
  {
    wdLog::Warning("DestroyTexture called on invalid handle (double free?)");
  }
}

wdGALTextureHandle wdGALDevice::CreateProxyTexture(wdGALTextureHandle hParentTexture, wdUInt32 uiSlice)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  wdGALTexture* pParentTexture = nullptr;

  if (!hParentTexture.IsInvalidated())
  {
    pParentTexture = Get<TextureTable, wdGALTexture>(hParentTexture, m_Textures);
  }

  if (pParentTexture == nullptr)
  {
    wdLog::Error("No valid texture handle given for proxy texture creation!");
    return wdGALTextureHandle();
  }

  const auto& parentDesc = pParentTexture->GetDescription();
  WD_ASSERT_DEV(parentDesc.m_Type != wdGALTextureType::Texture2DProxy, "Can't create a proxy texture of a proxy texture.");
  WD_ASSERT_DEV(parentDesc.m_Type == wdGALTextureType::TextureCube || parentDesc.m_uiArraySize > 1,
    "Proxy textures can only be created for cubemaps or array textures.");

  wdGALProxyTexture* pProxyTexture = WD_NEW(&m_Allocator, wdGALProxyTexture, *pParentTexture);
  wdGALTextureHandle hProxyTexture(m_Textures.Insert(pProxyTexture));

  const auto& desc = pProxyTexture->GetDescription();

  // Create default resource view
  if (desc.m_bAllowShaderResourceView)
  {
    wdGALResourceViewCreationDescription viewDesc;
    viewDesc.m_hTexture = hProxyTexture;
    viewDesc.m_uiFirstArraySlice = uiSlice;
    viewDesc.m_uiArraySize = 1;

    pProxyTexture->m_hDefaultResourceView = CreateResourceView(viewDesc);
  }

  // Create default render target view
  if (desc.m_bCreateRenderTarget)
  {
    wdGALRenderTargetViewCreationDescription rtDesc;
    rtDesc.m_hTexture = hProxyTexture;
    rtDesc.m_uiFirstSlice = uiSlice;
    rtDesc.m_uiSliceCount = 1;

    pProxyTexture->m_hDefaultRenderTargetView = CreateRenderTargetView(rtDesc);
  }

  return hProxyTexture;
}

void wdGALDevice::DestroyProxyTexture(wdGALTextureHandle hProxyTexture)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  wdGALTexture* pTexture = nullptr;
  if (m_Textures.TryGetValue(hProxyTexture, pTexture))
  {
    WD_ASSERT_DEV(pTexture->GetDescription().m_Type == wdGALTextureType::Texture2DProxy, "Given texture is not a proxy texture");

    AddDeadObject(GALObjectType::Texture, hProxyTexture);
  }
  else
  {
    wdLog::Warning("DestroyProxyTexture called on invalid handle (double free?)");
  }
}

wdGALResourceViewHandle wdGALDevice::GetDefaultResourceView(wdGALTextureHandle hTexture)
{
  if (const wdGALTexture* pTexture = GetTexture(hTexture))
  {
    return pTexture->m_hDefaultResourceView;
  }

  return wdGALResourceViewHandle();
}

wdGALResourceViewHandle wdGALDevice::GetDefaultResourceView(wdGALBufferHandle hBuffer)
{
  if (const wdGALBuffer* pBuffer = GetBuffer(hBuffer))
  {
    return pBuffer->m_hDefaultResourceView;
  }

  return wdGALResourceViewHandle();
}

wdGALResourceViewHandle wdGALDevice::CreateResourceView(const wdGALResourceViewCreationDescription& desc)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  wdGALResourceBase* pResource = nullptr;

  if (!desc.m_hTexture.IsInvalidated())
    pResource = Get<TextureTable, wdGALTexture>(desc.m_hTexture, m_Textures);

  if (!desc.m_hBuffer.IsInvalidated())
    pResource = Get<BufferTable, wdGALBuffer>(desc.m_hBuffer, m_Buffers);

  if (pResource == nullptr)
  {
    wdLog::Error("No valid texture handle or buffer handle given for resource view creation!");
    return wdGALResourceViewHandle();
  }

  // Hash desc and return potential existing one
  wdUInt32 uiHash = desc.CalculateHash();

  {
    wdGALResourceViewHandle hResourceView;
    if (pResource->m_ResourceViews.TryGetValue(uiHash, hResourceView))
    {
      return hResourceView;
    }
  }

  wdGALResourceView* pResourceView = CreateResourceViewPlatform(pResource, desc);

  if (pResourceView != nullptr)
  {
    wdGALResourceViewHandle hResourceView(m_ResourceViews.Insert(pResourceView));
    pResource->m_ResourceViews.Insert(uiHash, hResourceView);

    return hResourceView;
  }

  return wdGALResourceViewHandle();
}

void wdGALDevice::DestroyResourceView(wdGALResourceViewHandle hResourceView)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  wdGALResourceView* pResourceView = nullptr;

  if (m_ResourceViews.TryGetValue(hResourceView, pResourceView))
  {
    AddDeadObject(GALObjectType::ResourceView, hResourceView);
  }
  else
  {
    wdLog::Warning("DestroyResourceView called on invalid handle (double free?)");
  }
}

wdGALRenderTargetViewHandle wdGALDevice::GetDefaultRenderTargetView(wdGALTextureHandle hTexture)
{
  if (const wdGALTexture* pTexture = GetTexture(hTexture))
  {
    return pTexture->m_hDefaultRenderTargetView;
  }

  return wdGALRenderTargetViewHandle();
}

wdGALRenderTargetViewHandle wdGALDevice::CreateRenderTargetView(const wdGALRenderTargetViewCreationDescription& desc)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  wdGALTexture* pTexture = nullptr;

  if (!desc.m_hTexture.IsInvalidated())
    pTexture = Get<TextureTable, wdGALTexture>(desc.m_hTexture, m_Textures);

  if (pTexture == nullptr)
  {
    wdLog::Error("No valid texture handle given for render target view creation!");
    return wdGALRenderTargetViewHandle();
  }

  /// \todo Platform independent validation

  // Hash desc and return potential existing one
  wdUInt32 uiHash = desc.CalculateHash();

  {
    wdGALRenderTargetViewHandle hRenderTargetView;
    if (pTexture->m_RenderTargetViews.TryGetValue(uiHash, hRenderTargetView))
    {
      return hRenderTargetView;
    }
  }

  wdGALRenderTargetView* pRenderTargetView = CreateRenderTargetViewPlatform(pTexture, desc);

  if (pRenderTargetView != nullptr)
  {
    wdGALRenderTargetViewHandle hRenderTargetView(m_RenderTargetViews.Insert(pRenderTargetView));
    pTexture->m_RenderTargetViews.Insert(uiHash, hRenderTargetView);

    return hRenderTargetView;
  }

  return wdGALRenderTargetViewHandle();
}

void wdGALDevice::DestroyRenderTargetView(wdGALRenderTargetViewHandle hRenderTargetView)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  wdGALRenderTargetView* pRenderTargetView = nullptr;

  if (m_RenderTargetViews.TryGetValue(hRenderTargetView, pRenderTargetView))
  {
    AddDeadObject(GALObjectType::RenderTargetView, hRenderTargetView);
  }
  else
  {
    wdLog::Warning("DestroyRenderTargetView called on invalid handle (double free?)");
  }
}

wdGALUnorderedAccessViewHandle wdGALDevice::CreateUnorderedAccessView(const wdGALUnorderedAccessViewCreationDescription& desc)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  if (!desc.m_hTexture.IsInvalidated() && !desc.m_hBuffer.IsInvalidated())
  {
    wdLog::Error("Can't pass both a texture and buffer to a wdGALUnorderedAccessViewCreationDescription.");
    return wdGALUnorderedAccessViewHandle();
  }

  wdGALResourceBase* pResource = nullptr;
  wdGALTexture* pTexture = nullptr;
  wdGALBuffer* pBuffer = nullptr;

  if (!desc.m_hTexture.IsInvalidated())
  {
    pResource = pTexture = Get<TextureTable, wdGALTexture>(desc.m_hTexture, m_Textures);
  }
  else if (!desc.m_hBuffer.IsInvalidated())
  {
    pResource = pBuffer = Get<BufferTable, wdGALBuffer>(desc.m_hBuffer, m_Buffers);
  }

  if (pResource == nullptr)
  {
    wdLog::Error("No valid texture handle or buffer handle given for unordered access view creation!");
    return wdGALUnorderedAccessViewHandle();
  }

  // Some platform independent validation.
  {
    if (pTexture)
    {
      // Is this really platform independent?
      if (pTexture->GetDescription().m_SampleCount != wdGALMSAASampleCount::None)
      {
        wdLog::Error("Can't create unordered access view on textures with multisampling.");
        return wdGALUnorderedAccessViewHandle();
      }
    }
    else
    {
      if (desc.m_OverrideViewFormat == wdGALResourceFormat::Invalid)
      {
        wdLog::Error("Invalid resource format is not allowed for buffer unordered access views!");
        return wdGALUnorderedAccessViewHandle();
      }

      if (!pBuffer->GetDescription().m_bAllowRawViews && desc.m_bRawView)
      {
        wdLog::Error("Trying to create a raw view for a buffer with no raw view flag is invalid!");
        return wdGALUnorderedAccessViewHandle();
      }
    }
  }

  // Hash desc and return potential existing one
  wdUInt32 uiHash = desc.CalculateHash();

  {
    wdGALUnorderedAccessViewHandle hUnorderedAccessView;
    if (pResource->m_UnorderedAccessViews.TryGetValue(uiHash, hUnorderedAccessView))
    {
      return hUnorderedAccessView;
    }
  }

  wdGALUnorderedAccessView* pUnorderedAccessViewView = CreateUnorderedAccessViewPlatform(pResource, desc);

  if (pUnorderedAccessViewView != nullptr)
  {
    wdGALUnorderedAccessViewHandle hUnorderedAccessView(m_UnorderedAccessViews.Insert(pUnorderedAccessViewView));
    pResource->m_UnorderedAccessViews.Insert(uiHash, hUnorderedAccessView);

    return hUnorderedAccessView;
  }

  return wdGALUnorderedAccessViewHandle();
}

void wdGALDevice::DestroyUnorderedAccessView(wdGALUnorderedAccessViewHandle hUnorderedAccessViewHandle)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  wdGALUnorderedAccessView* pUnorderedAccesssView = nullptr;

  if (m_UnorderedAccessViews.TryGetValue(hUnorderedAccessViewHandle, pUnorderedAccesssView))
  {
    AddDeadObject(GALObjectType::UnorderedAccessView, hUnorderedAccessViewHandle);
  }
  else
  {
    wdLog::Warning("DestroyUnorderedAccessView called on invalid handle (double free?)");
  }
}

wdGALSwapChainHandle wdGALDevice::CreateSwapChain(const SwapChainFactoryFunction& func)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  ///// \todo Platform independent validation
  //if (desc.m_pWindow == nullptr)
  //{
  //  wdLog::Error("The desc for the swap chain creation contained an invalid (nullptr) window handle!");
  //  return wdGALSwapChainHandle();
  //}

  wdGALSwapChain* pSwapChain = func(&m_Allocator);
  //wdGALSwapChainDX11* pSwapChain = WD_NEW(&m_Allocator, wdGALSwapChainDX11, Description);

  if (!pSwapChain->InitPlatform(this).Succeeded())
  {
    WD_DELETE(&m_Allocator, pSwapChain);
    return wdGALSwapChainHandle();
  }

  return wdGALSwapChainHandle(m_SwapChains.Insert(pSwapChain));
}

wdResult wdGALDevice::UpdateSwapChain(wdGALSwapChainHandle hSwapChain, wdEnum<wdGALPresentMode> newPresentMode)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  wdGALSwapChain* pSwapChain = nullptr;

  if (m_SwapChains.TryGetValue(hSwapChain, pSwapChain))
  {
    return pSwapChain->UpdateSwapChain(this, newPresentMode);
  }
  else
  {
    wdLog::Warning("UpdateSwapChain called on invalid handle.");
    return WD_FAILURE;
  }
}

void wdGALDevice::DestroySwapChain(wdGALSwapChainHandle hSwapChain)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  wdGALSwapChain* pSwapChain = nullptr;

  if (m_SwapChains.TryGetValue(hSwapChain, pSwapChain))
  {
    AddDeadObject(GALObjectType::SwapChain, hSwapChain);
  }
  else
  {
    wdLog::Warning("DestroySwapChain called on invalid handle (double free?)");
  }
}

wdGALQueryHandle wdGALDevice::CreateQuery(const wdGALQueryCreationDescription& desc)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  wdGALQuery* pQuery = CreateQueryPlatform(desc);

  if (pQuery == nullptr)
  {
    return wdGALQueryHandle();
  }
  else
  {
    return wdGALQueryHandle(m_Queries.Insert(pQuery));
  }
}

void wdGALDevice::DestroyQuery(wdGALQueryHandle hQuery)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  wdGALQuery* pQuery = nullptr;

  if (m_Queries.TryGetValue(hQuery, pQuery))
  {
    AddDeadObject(GALObjectType::Query, hQuery);
  }
  else
  {
    wdLog::Warning("DestroyQuery called on invalid handle (double free?)");
  }
}

wdGALVertexDeclarationHandle wdGALDevice::CreateVertexDeclaration(const wdGALVertexDeclarationCreationDescription& desc)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  /// \todo Platform independent validation

  // Hash desc and return potential existing one (including inc. refcount)
  wdUInt32 uiHash = desc.CalculateHash();

  {
    wdGALVertexDeclarationHandle hVertexDeclaration;
    if (m_VertexDeclarationTable.TryGetValue(uiHash, hVertexDeclaration))
    {
      wdGALVertexDeclaration* pVertexDeclaration = m_VertexDeclarations[hVertexDeclaration];
      if (pVertexDeclaration->GetRefCount() == 0)
      {
        ReviveDeadObject(GALObjectType::VertexDeclaration, hVertexDeclaration);
      }

      pVertexDeclaration->AddRef();
      return hVertexDeclaration;
    }
  }

  wdGALVertexDeclaration* pVertexDeclaration = CreateVertexDeclarationPlatform(desc);

  if (pVertexDeclaration != nullptr)
  {
    pVertexDeclaration->AddRef();

    wdGALVertexDeclarationHandle hVertexDeclaration(m_VertexDeclarations.Insert(pVertexDeclaration));
    m_VertexDeclarationTable.Insert(uiHash, hVertexDeclaration);

    return hVertexDeclaration;
  }

  return wdGALVertexDeclarationHandle();
}

void wdGALDevice::DestroyVertexDeclaration(wdGALVertexDeclarationHandle hVertexDeclaration)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  wdGALVertexDeclaration* pVertexDeclaration = nullptr;

  if (m_VertexDeclarations.TryGetValue(hVertexDeclaration, pVertexDeclaration))
  {
    pVertexDeclaration->ReleaseRef();

    if (pVertexDeclaration->GetRefCount() == 0)
    {
      AddDeadObject(GALObjectType::VertexDeclaration, hVertexDeclaration);
    }
  }
  else
  {
    wdLog::Warning("DestroyVertexDeclaration called on invalid handle (double free?)");
  }
}

wdGALTextureHandle wdGALDevice::GetBackBufferTextureFromSwapChain(wdGALSwapChainHandle hSwapChain)
{
  wdGALSwapChain* pSwapChain = nullptr;

  if (m_SwapChains.TryGetValue(hSwapChain, pSwapChain))
  {
    return pSwapChain->GetBackBufferTexture();
  }
  else
  {
    WD_REPORT_FAILURE("Swap chain handle invalid");
    return wdGALTextureHandle();
  }
}



// Misc functions

void wdGALDevice::BeginFrame(const wdUInt64 uiRenderFrame)
{
  {
    WD_PROFILE_SCOPE("BeforeBeginFrame");
    wdGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = wdGALDeviceEvent::BeforeBeginFrame;
    m_Events.Broadcast(e);
  }

  {
    WD_GALDEVICE_LOCK_AND_CHECK();
    WD_ASSERT_DEV(!m_bBeginFrameCalled, "You must call wdGALDevice::EndFrame before you can call wdGALDevice::BeginFrame again");
    m_bBeginFrameCalled = true;

    BeginFramePlatform(uiRenderFrame);
  }

  // TODO: move to beginrendering/compute calls
  //m_pPrimaryContext->ClearStatisticsCounters();

  {
    wdGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = wdGALDeviceEvent::AfterBeginFrame;
    m_Events.Broadcast(e);
  }
}

void wdGALDevice::EndFrame()
{
  {
    wdGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = wdGALDeviceEvent::BeforeEndFrame;
    m_Events.Broadcast(e);
  }

  {
    WD_GALDEVICE_LOCK_AND_CHECK();
    WD_ASSERT_DEV(m_bBeginFrameCalled, "You must have called wdGALDevice::Begin before you can call wdGALDevice::EndFrame");

    DestroyDeadObjects();

    EndFramePlatform();

    m_bBeginFrameCalled = false;
  }

  {
    wdGALDeviceEvent e;
    e.m_pDevice = this;
    e.m_Type = wdGALDeviceEvent::AfterEndFrame;
    m_Events.Broadcast(e);
  }
}

const wdGALDeviceCapabilities& wdGALDevice::GetCapabilities() const
{
  return m_Capabilities;
}

wdUInt64 wdGALDevice::GetMemoryConsumptionForTexture(const wdGALTextureCreationDescription& desc) const
{
  // This generic implementation is only an approximation, but it can be overridden by specific devices
  // to give an accurate memory consumption figure.
  wdUInt64 uiMemory = wdUInt64(desc.m_uiWidth) * wdUInt64(desc.m_uiHeight) * wdUInt64(desc.m_uiDepth);
  uiMemory *= desc.m_uiArraySize;
  uiMemory *= wdGALResourceFormat::GetBitsPerElement(desc.m_Format);
  uiMemory /= 8; // Bits per pixel
  uiMemory *= desc.m_SampleCount;

  // Also account for mip maps
  if (desc.m_uiMipLevelCount > 1)
  {
    uiMemory += static_cast<wdUInt64>((1.0 / 3.0) * uiMemory);
  }

  return uiMemory;
}


wdUInt64 wdGALDevice::GetMemoryConsumptionForBuffer(const wdGALBufferCreationDescription& desc) const
{
  return desc.m_uiTotalSize;
}


void wdGALDevice::WaitIdle()
{
  WaitIdlePlatform();
}

void wdGALDevice::DestroyViews(wdGALResourceBase* pResource)
{
  WD_GALDEVICE_LOCK_AND_CHECK();

  for (auto it = pResource->m_ResourceViews.GetIterator(); it.IsValid(); ++it)
  {
    wdGALResourceViewHandle hResourceView = it.Value();
    wdGALResourceView* pResourceView = m_ResourceViews[hResourceView];

    m_ResourceViews.Remove(hResourceView);

    DestroyResourceViewPlatform(pResourceView);
  }
  pResource->m_ResourceViews.Clear();
  pResource->m_hDefaultResourceView.Invalidate();

  for (auto it = pResource->m_RenderTargetViews.GetIterator(); it.IsValid(); ++it)
  {
    wdGALRenderTargetViewHandle hRenderTargetView = it.Value();
    wdGALRenderTargetView* pRenderTargetView = m_RenderTargetViews[hRenderTargetView];

    m_RenderTargetViews.Remove(hRenderTargetView);

    DestroyRenderTargetViewPlatform(pRenderTargetView);
  }
  pResource->m_RenderTargetViews.Clear();
  pResource->m_hDefaultRenderTargetView.Invalidate();

  for (auto it = pResource->m_UnorderedAccessViews.GetIterator(); it.IsValid(); ++it)
  {
    wdGALUnorderedAccessViewHandle hUnorderedAccessView = it.Value();
    wdGALUnorderedAccessView* pUnorderedAccessView = m_UnorderedAccessViews[hUnorderedAccessView];

    m_UnorderedAccessViews.Remove(hUnorderedAccessView);

    DestroyUnorderedAccessViewPlatform(pUnorderedAccessView);
  }
  pResource->m_UnorderedAccessViews.Clear();
}

void wdGALDevice::DestroyDeadObjects()
{
  // Can't use range based for here since new objects might be added during iteration
  for (wdUInt32 i = 0; i < m_DeadObjects.GetCount(); ++i)
  {
    const auto& deadObject = m_DeadObjects[i];

    switch (deadObject.m_uiType)
    {
      case GALObjectType::BlendState:
      {
        wdGALBlendStateHandle hBlendState(wdGAL::wd16_16Id(deadObject.m_uiHandle));
        wdGALBlendState* pBlendState = nullptr;

        WD_VERIFY(m_BlendStates.Remove(hBlendState, &pBlendState), "BlendState not found in idTable");
        WD_VERIFY(m_BlendStateTable.Remove(pBlendState->GetDescription().CalculateHash()), "BlendState not found in de-duplication table");

        DestroyBlendStatePlatform(pBlendState);

        break;
      }
      case GALObjectType::DepthStencilState:
      {
        wdGALDepthStencilStateHandle hDepthStencilState(wdGAL::wd16_16Id(deadObject.m_uiHandle));
        wdGALDepthStencilState* pDepthStencilState = nullptr;

        WD_VERIFY(m_DepthStencilStates.Remove(hDepthStencilState, &pDepthStencilState), "DepthStencilState not found in idTable");
        WD_VERIFY(m_DepthStencilStateTable.Remove(pDepthStencilState->GetDescription().CalculateHash()),
          "DepthStencilState not found in de-duplication table");

        DestroyDepthStencilStatePlatform(pDepthStencilState);

        break;
      }
      case GALObjectType::RasterizerState:
      {
        wdGALRasterizerStateHandle hRasterizerState(wdGAL::wd16_16Id(deadObject.m_uiHandle));
        wdGALRasterizerState* pRasterizerState = nullptr;

        WD_VERIFY(m_RasterizerStates.Remove(hRasterizerState, &pRasterizerState), "RasterizerState not found in idTable");
        WD_VERIFY(
          m_RasterizerStateTable.Remove(pRasterizerState->GetDescription().CalculateHash()), "RasterizerState not found in de-duplication table");

        DestroyRasterizerStatePlatform(pRasterizerState);

        break;
      }
      case GALObjectType::SamplerState:
      {
        wdGALSamplerStateHandle hSamplerState(wdGAL::wd16_16Id(deadObject.m_uiHandle));
        wdGALSamplerState* pSamplerState = nullptr;

        WD_VERIFY(m_SamplerStates.Remove(hSamplerState, &pSamplerState), "SamplerState not found in idTable");
        WD_VERIFY(m_SamplerStateTable.Remove(pSamplerState->GetDescription().CalculateHash()), "SamplerState not found in de-duplication table");

        DestroySamplerStatePlatform(pSamplerState);

        break;
      }
      case GALObjectType::Shader:
      {
        wdGALShaderHandle hShader(wdGAL::wd18_14Id(deadObject.m_uiHandle));
        wdGALShader* pShader = nullptr;

        m_Shaders.Remove(hShader, &pShader);

        DestroyShaderPlatform(pShader);

        break;
      }
      case GALObjectType::Buffer:
      {
        wdGALBufferHandle hBuffer(wdGAL::wd18_14Id(deadObject.m_uiHandle));
        wdGALBuffer* pBuffer = nullptr;

        m_Buffers.Remove(hBuffer, &pBuffer);

        DestroyViews(pBuffer);
        DestroyBufferPlatform(pBuffer);

        break;
      }
      case GALObjectType::Texture:
      {
        wdGALTextureHandle hTexture(wdGAL::wd18_14Id(deadObject.m_uiHandle));
        wdGALTexture* pTexture = nullptr;

        m_Textures.Remove(hTexture, &pTexture);

        DestroyViews(pTexture);
        DestroyTexturePlatform(pTexture);

        break;
      }
      case GALObjectType::ResourceView:
      {
        wdGALResourceViewHandle hResourceView(wdGAL::wd18_14Id(deadObject.m_uiHandle));
        wdGALResourceView* pResourceView = nullptr;

        m_ResourceViews.Remove(hResourceView, &pResourceView);

        wdGALResourceBase* pResource = pResourceView->m_pResource;
        WD_ASSERT_DEBUG(pResource != nullptr, "");

        WD_VERIFY(pResource->m_ResourceViews.Remove(pResourceView->GetDescription().CalculateHash()), "");
        pResourceView->m_pResource = nullptr;

        DestroyResourceViewPlatform(pResourceView);

        break;
      }
      case GALObjectType::RenderTargetView:
      {
        wdGALRenderTargetViewHandle hRenderTargetView(wdGAL::wd18_14Id(deadObject.m_uiHandle));
        wdGALRenderTargetView* pRenderTargetView = nullptr;

        m_RenderTargetViews.Remove(hRenderTargetView, &pRenderTargetView);

        wdGALTexture* pTexture = pRenderTargetView->m_pTexture;
        WD_ASSERT_DEBUG(pTexture != nullptr, "");
        WD_VERIFY(pTexture->m_RenderTargetViews.Remove(pRenderTargetView->GetDescription().CalculateHash()), "");
        pRenderTargetView->m_pTexture = nullptr;

        DestroyRenderTargetViewPlatform(pRenderTargetView);

        break;
      }
      case GALObjectType::UnorderedAccessView:
      {
        wdGALUnorderedAccessViewHandle hUnorderedAccessViewHandle(wdGAL::wd18_14Id(deadObject.m_uiHandle));
        wdGALUnorderedAccessView* pUnorderedAccesssView = nullptr;

        m_UnorderedAccessViews.Remove(hUnorderedAccessViewHandle, &pUnorderedAccesssView);

        wdGALResourceBase* pResource = pUnorderedAccesssView->m_pResource;
        WD_ASSERT_DEBUG(pResource != nullptr, "");

        WD_VERIFY(pResource->m_UnorderedAccessViews.Remove(pUnorderedAccesssView->GetDescription().CalculateHash()), "");
        pUnorderedAccesssView->m_pResource = nullptr;

        DestroyUnorderedAccessViewPlatform(pUnorderedAccesssView);

        break;
      }
      case GALObjectType::SwapChain:
      {
        wdGALSwapChainHandle hSwapChain(wdGAL::wd16_16Id(deadObject.m_uiHandle));
        wdGALSwapChain* pSwapChain = nullptr;

        m_SwapChains.Remove(hSwapChain, &pSwapChain);

        if (pSwapChain != nullptr)
        {
          pSwapChain->DeInitPlatform(this).IgnoreResult();
          WD_DELETE(&m_Allocator, pSwapChain);
        }

        break;
      }
      case GALObjectType::Query:
      {
        wdGALQueryHandle hQuery(wdGAL::wd20_12Id(deadObject.m_uiHandle));
        wdGALQuery* pQuery = nullptr;

        m_Queries.Remove(hQuery, &pQuery);

        DestroyQueryPlatform(pQuery);

        break;
      }
      case GALObjectType::VertexDeclaration:
      {
        wdGALVertexDeclarationHandle hVertexDeclaration(wdGAL::wd18_14Id(deadObject.m_uiHandle));
        wdGALVertexDeclaration* pVertexDeclaration = nullptr;

        m_VertexDeclarations.Remove(hVertexDeclaration, &pVertexDeclaration);
        m_VertexDeclarationTable.Remove(pVertexDeclaration->GetDescription().CalculateHash());

        DestroyVertexDeclarationPlatform(pVertexDeclaration);

        break;
      }
      default:
        WD_ASSERT_NOT_IMPLEMENTED;
    }
  }

  m_DeadObjects.Clear();
}

const wdGALSwapChain* wdGALDevice::GetSwapChainInternal(wdGALSwapChainHandle hSwapChain, const wdRTTI* pRequestedType) const
{
  const wdGALSwapChain* pSwapChain = GetSwapChain(hSwapChain);
  if (pSwapChain)
  {
    if (!pSwapChain->GetDescription().m_pSwapChainType->IsDerivedFrom(pRequestedType))
      return nullptr;
  }
  return pSwapChain;
}

WD_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_Device);
