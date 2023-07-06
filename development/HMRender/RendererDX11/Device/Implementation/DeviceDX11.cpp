#include <RendererDX11/RendererDX11PCH.h>

#include <Core/System/Window.h>
#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/Configuration/Startup.h>
#include <RendererDX11/CommandEncoder/CommandEncoderImplDX11.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Device/PassDX11.h>
#include <RendererDX11/Device/SwapChainDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/QueryDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Resources/ResourceViewDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererDX11/Resources/UnorderedAccessViewDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>
#include <RendererDX11/Shader/VertexDeclarationDX11.h>
#include <RendererDX11/State/StateDX11.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <d3d11.h>
#include <d3d11_3.h>
#include <dxgidebug.h>

#if WD_ENABLED(WD_PLATFORM_WINDOWS_UWP)
#  include <d3d11_1.h>
#endif

wdInternal::NewInstance<wdGALDevice> CreateDX11Device(wdAllocatorBase* pAllocator, const wdGALDeviceCreationDescription& description)
{
  return WD_NEW(pAllocator, wdGALDeviceDX11, description);
}

// clang-format off
WD_BEGIN_SUBSYSTEM_DECLARATION(RendererDX11, DeviceFactory)

ON_CORESYSTEMS_STARTUP
{
  wdGALDeviceFactory::RegisterCreatorFunc("DX11", &CreateDX11Device, "DX11_SM50", "wdShaderCompilerHLSL");
}

ON_CORESYSTEMS_SHUTDOWN
{
  wdGALDeviceFactory::UnregisterCreatorFunc("DX11");
}

WD_END_SUBSYSTEM_DECLARATION;
// clang-format on

wdGALDeviceDX11::wdGALDeviceDX11(const wdGALDeviceCreationDescription& Description)
  : wdGALDevice(Description)
  , m_pDevice(nullptr)
  , m_pDevice3(nullptr)
  , m_pDebug(nullptr)
  , m_pDXGIFactory(nullptr)
  , m_pDXGIAdapter(nullptr)
  , m_pDXGIDevice(nullptr)
  , m_uiFeatureLevel(D3D_FEATURE_LEVEL_9_1)
  , m_uiFrameCounter(0)
{
}

wdGALDeviceDX11::~wdGALDeviceDX11() = default;

// Init & shutdown functions

wdResult wdGALDeviceDX11::InitPlatform(DWORD dwFlags, IDXGIAdapter* pUsedAdapter)
{
  WD_LOG_BLOCK("wdGALDeviceDX11::InitPlatform");

retry:

  if (m_Description.m_bDebugDevice)
    dwFlags |= D3D11_CREATE_DEVICE_DEBUG;
  else
    dwFlags &= ~D3D11_CREATE_DEVICE_DEBUG;

  D3D_FEATURE_LEVEL FeatureLevels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0, D3D_FEATURE_LEVEL_9_3};
  ID3D11DeviceContext* pImmediateContext = nullptr;

  D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE;
  // driverType = D3D_DRIVER_TYPE_REFERENCE; // enables the Reference Device

  if (pUsedAdapter != nullptr)
  {
    // required by the specification
    driverType = D3D_DRIVER_TYPE_UNKNOWN;
  }

  // Manually step through feature levels - if a Win 7 system doesn't have the 11.1 runtime installed
  // The create device call will fail even though the 11.0 (or lower) level could've been
  // initialized successfully
  int FeatureLevelIdx = 0;
  for (FeatureLevelIdx = 0; FeatureLevelIdx < WD_ARRAY_SIZE(FeatureLevels); FeatureLevelIdx++)
  {
    if (SUCCEEDED(D3D11CreateDevice(pUsedAdapter, driverType, nullptr, dwFlags, &FeatureLevels[FeatureLevelIdx], 1, D3D11_SDK_VERSION, &m_pDevice, (D3D_FEATURE_LEVEL*)&m_uiFeatureLevel, &pImmediateContext)))
    {
      break;
    }
  }

  // Nothing could be initialized:
  if (pImmediateContext == nullptr)
  {
    if (m_Description.m_bDebugDevice)
    {
      wdLog::Warning("Couldn't initialize D3D11 debug device!");

      m_Description.m_bDebugDevice = false;
      goto retry;
    }

    wdLog::Error("Couldn't initialize D3D11 device!");
    return WD_FAILURE;
  }
  else
  {
    m_pImmediateContext = pImmediateContext;

    const char* FeatureLevelNames[] = {"11.1", "11.0", "10.1", "10", "9.3"};

    WD_CHECK_AT_COMPILETIME(WD_ARRAY_SIZE(FeatureLevels) == WD_ARRAY_SIZE(FeatureLevelNames));

    wdLog::Success("Initialized D3D11 device with feature level {0}.", FeatureLevelNames[FeatureLevelIdx]);
  }

  if (m_Description.m_bDebugDevice)
  {
    if (SUCCEEDED(m_pDevice->QueryInterface(__uuidof(ID3D11Debug), (void**)&m_pDebug)))
    {
      ID3D11InfoQueue* pInfoQueue = nullptr;
      if (SUCCEEDED(m_pDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&pInfoQueue)))
      {
        // only do this when a debugger is attached, otherwise the app would crash on every DX error
        if (IsDebuggerPresent())
        {
          pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
          pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
          pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, TRUE);
        }

        // Ignore list.
        {
          D3D11_MESSAGE_ID hide[] = {
            // Hide messages about abandoned query results. This can easily happen when a GPUStopwatch is suddenly unused.
            D3D11_MESSAGE_ID_QUERY_BEGIN_ABANDONING_PREVIOUS_RESULTS, D3D11_MESSAGE_ID_QUERY_END_ABANDONING_PREVIOUS_RESULTS,
            // Don't break on invalid input assembly. This can easily happen when using the wrong mesh-material combination.
            D3D11_MESSAGE_ID_CREATEINPUTLAYOUT_MISSINGELEMENT,
            // Add more message IDs here as needed
          };
          D3D11_INFO_QUEUE_FILTER filter;
          wdMemoryUtils::ZeroFill(&filter, 1);
          filter.DenyList.NumIDs = _countof(hide);
          filter.DenyList.pIDList = hide;
          pInfoQueue->AddStorageFilterEntries(&filter);
        }

        pInfoQueue->Release();
      }
    }
  }


  // Create default pass
  m_pDefaultPass = WD_NEW(&m_Allocator, wdGALPassDX11, *this);

  if (FAILED(m_pDevice->QueryInterface(__uuidof(IDXGIDevice1), (void**)&m_pDXGIDevice)))
  {
    wdLog::Error("Couldn't get the DXGIDevice1 interface of the D3D11 device - this may happen when running on Windows Vista without SP2 "
                 "installed!");
    return WD_FAILURE;
  }

  if (FAILED(m_pDevice->QueryInterface(__uuidof(ID3D11Device3), (void**)&m_pDevice3)))
  {
    wdLog::Info("D3D device doesn't support ID3D11Device3, some features might be unavailable.");
  }

  if (FAILED(m_pDXGIDevice->SetMaximumFrameLatency(1)))
  {
    wdLog::Warning("Failed to set max frames latency");
  }

  if (FAILED(m_pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&m_pDXGIAdapter)))
  {
    return WD_FAILURE;
  }

  if (FAILED(m_pDXGIAdapter->GetParent(__uuidof(IDXGIFactory1), (void**)&m_pDXGIFactory)))
  {
    return WD_FAILURE;
  }

  // Fill lookup table
  FillFormatLookupTable();

  wdClipSpaceDepthRange::Default = wdClipSpaceDepthRange::ZeroToOne;
  wdClipSpaceYMode::RenderToTextureDefault = wdClipSpaceYMode::Regular;

  // Per frame data & timer data
  D3D11_QUERY_DESC disjointQueryDesc;
  disjointQueryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
  disjointQueryDesc.MiscFlags = 0;

  D3D11_QUERY_DESC timerQueryDesc;
  timerQueryDesc.Query = D3D11_QUERY_TIMESTAMP;
  timerQueryDesc.MiscFlags = 0;

  for (wdUInt32 i = 0; i < WD_ARRAY_SIZE(m_PerFrameData); ++i)
  {
    auto& perFrameData = m_PerFrameData[i];

    D3D11_QUERY_DESC QueryDesc;
    QueryDesc.Query = D3D11_QUERY_EVENT;
    QueryDesc.MiscFlags = 0;
    if (SUCCEEDED(GetDXDevice()->CreateQuery(&QueryDesc, &perFrameData.m_pFence)))

      if (FAILED(m_pDevice->CreateQuery(&disjointQueryDesc, &perFrameData.m_pDisjointTimerQuery)))
      {
        wdLog::Error("Creation of native DirectX query for disjoint query has failed!");
        return WD_FAILURE;
      }
  }

  //#TODO_DX11 Replace ring buffer with proper pool like in Vulkan to prevent buffer overrun.
  m_Timestamps.SetCountUninitialized(2048);
  for (wdUInt32 i = 0; i < m_Timestamps.GetCount(); ++i)
  {
    if (FAILED(m_pDevice->CreateQuery(&timerQueryDesc, &m_Timestamps[i])))
    {
      wdLog::Error("Creation of native DirectX query for timestamp has failed!");
      return WD_FAILURE;
    }
  }

  m_SyncTimeDiff.SetZero();

  wdGALWindowSwapChain::SetFactoryMethod([this](const wdGALWindowSwapChainCreationDescription& desc) -> wdGALSwapChainHandle { return CreateSwapChain([this, &desc](wdAllocatorBase* pAllocator) -> wdGALSwapChain* { return WD_NEW(pAllocator, wdGALSwapChainDX11, desc); }); });

  return WD_SUCCESS;
}

wdResult wdGALDeviceDX11::InitPlatform()
{
  return InitPlatform(0, nullptr);
}

void wdGALDeviceDX11::ReportLiveGpuObjects()
{
#if WD_ENABLED(WD_PLATFORM_WINDOWS_UWP)
  // not implemented
  return;

#else

  const HMODULE hDxgiDebugDLL = LoadLibraryW(L"Dxgidebug.dll");

  if (hDxgiDebugDLL == nullptr)
    return;

  typedef HRESULT(WINAPI * FnGetDebugInterfacePtr)(REFIID, void**);
  FnGetDebugInterfacePtr GetDebugInterfacePtr = (FnGetDebugInterfacePtr)GetProcAddress(hDxgiDebugDLL, "DXGIGetDebugInterface");

  if (GetDebugInterfacePtr == nullptr)
    return;

  IDXGIDebug* dxgiDebug = nullptr;
  GetDebugInterfacePtr(IID_PPV_ARGS(&dxgiDebug));

  if (dxgiDebug == nullptr)
    return;

  OutputDebugStringW(L" +++++ Live DX11 Objects: +++++\n");

  // prints to OutputDebugString
  dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);

  OutputDebugStringW(L" ----- Live DX11 Objects: -----\n");

  dxgiDebug->Release();

#endif
}

void wdGALDeviceDX11::FlushDeadObjects()
{
  DestroyDeadObjects();
}

wdResult wdGALDeviceDX11::ShutdownPlatform()
{
  wdGALWindowSwapChain::SetFactoryMethod({});
  for (wdUInt32 type = 0; type < TempResourceType::ENUM_COUNT; ++type)
  {
    for (auto it = m_FreeTempResources[type].GetIterator(); it.IsValid(); ++it)
    {
      wdDynamicArray<ID3D11Resource*>& resources = it.Value();
      for (auto pResource : resources)
      {
        WD_GAL_DX11_RELEASE(pResource);
      }
    }
    m_FreeTempResources[type].Clear();

    for (auto& tempResource : m_UsedTempResources[type])
    {
      WD_GAL_DX11_RELEASE(tempResource.m_pResource);
    }
    m_UsedTempResources[type].Clear();
  }

  for (auto& timestamp : m_Timestamps)
  {
    WD_GAL_DX11_RELEASE(timestamp);
  }
  m_Timestamps.Clear();

  for (wdUInt32 i = 0; i < WD_ARRAY_SIZE(m_PerFrameData); ++i)
  {
    auto& perFrameData = m_PerFrameData[i];

    WD_GAL_DX11_RELEASE(perFrameData.m_pFence);
    perFrameData.m_pFence = nullptr;

    WD_GAL_DX11_RELEASE(perFrameData.m_pDisjointTimerQuery);
  }

#if WD_ENABLED(WD_PLATFORM_WINDOWS_UWP)
  // Force immediate destruction of all objects destroyed so far.
  // This is necessary if we want to create a new primary swap chain/device right after this.
  // See: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476425(v=vs.85).aspx#Defer_Issues_with_Flip
  // Strictly speaking we should do this right after we destroy the swap chain and flush all contexts that are affected.
  // However, the particular usecase where this problem comes up is usually a restart scenario.
  if (m_pImmediateContext != nullptr)
  {
    m_pImmediateContext->ClearState();
    m_pImmediateContext->Flush();
  }
#endif

  m_pDefaultPass = nullptr;

  WD_GAL_DX11_RELEASE(m_pImmediateContext);
  WD_GAL_DX11_RELEASE(m_pDevice3);
  WD_GAL_DX11_RELEASE(m_pDevice);
  WD_GAL_DX11_RELEASE(m_pDebug);
  WD_GAL_DX11_RELEASE(m_pDXGIFactory);
  WD_GAL_DX11_RELEASE(m_pDXGIAdapter);
  WD_GAL_DX11_RELEASE(m_pDXGIDevice);

  ReportLiveGpuObjects();

  return WD_SUCCESS;
}

// Pipeline & Pass functions

void wdGALDeviceDX11::BeginPipelinePlatform(const char* szName, wdGALSwapChain* pSwapChain)
{
#if WD_ENABLED(WD_USE_PROFILING)
  m_pPipelineTimingScope = wdProfilingScopeAndMarker::Start(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), szName);
#endif

  if (pSwapChain)
  {
    pSwapChain->AcquireNextRenderTarget(this);
  }
}

void wdGALDeviceDX11::EndPipelinePlatform(wdGALSwapChain* pSwapChain)
{
  if (pSwapChain)
  {
    pSwapChain->PresentRenderTarget(this);
  }

#if WD_ENABLED(WD_USE_PROFILING)
  wdProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), m_pPipelineTimingScope);
#endif
}

wdGALPass* wdGALDeviceDX11::BeginPassPlatform(const char* szName)
{
#if WD_ENABLED(WD_USE_PROFILING)
  m_pPassTimingScope = wdProfilingScopeAndMarker::Start(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), szName);
#endif

  m_pDefaultPass->BeginPass(szName);

  return m_pDefaultPass.Borrow();
}

void wdGALDeviceDX11::EndPassPlatform(wdGALPass* pPass)
{
  WD_ASSERT_DEV(m_pDefaultPass.Borrow() == pPass, "Invalid pass");

#if WD_ENABLED(WD_USE_PROFILING)
  wdProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), m_pPassTimingScope);
#endif

  m_pDefaultPass->EndPass();
}

// State creation functions

wdGALBlendState* wdGALDeviceDX11::CreateBlendStatePlatform(const wdGALBlendStateCreationDescription& Description)
{
  wdGALBlendStateDX11* pState = WD_NEW(&m_Allocator, wdGALBlendStateDX11, Description);

  if (pState->InitPlatform(this).Succeeded())
  {
    return pState;
  }
  else
  {
    WD_DELETE(&m_Allocator, pState);
    return nullptr;
  }
}

void wdGALDeviceDX11::DestroyBlendStatePlatform(wdGALBlendState* pBlendState)
{
  wdGALBlendStateDX11* pState = static_cast<wdGALBlendStateDX11*>(pBlendState);
  pState->DeInitPlatform(this).IgnoreResult();
  WD_DELETE(&m_Allocator, pState);
}

wdGALDepthStencilState* wdGALDeviceDX11::CreateDepthStencilStatePlatform(const wdGALDepthStencilStateCreationDescription& Description)
{
  wdGALDepthStencilStateDX11* pDX11DepthStencilState = WD_NEW(&m_Allocator, wdGALDepthStencilStateDX11, Description);

  if (pDX11DepthStencilState->InitPlatform(this).Succeeded())
  {
    return pDX11DepthStencilState;
  }
  else
  {
    WD_DELETE(&m_Allocator, pDX11DepthStencilState);
    return nullptr;
  }
}

void wdGALDeviceDX11::DestroyDepthStencilStatePlatform(wdGALDepthStencilState* pDepthStencilState)
{
  wdGALDepthStencilStateDX11* pDX11DepthStencilState = static_cast<wdGALDepthStencilStateDX11*>(pDepthStencilState);
  pDX11DepthStencilState->DeInitPlatform(this).IgnoreResult();
  WD_DELETE(&m_Allocator, pDX11DepthStencilState);
}

wdGALRasterizerState* wdGALDeviceDX11::CreateRasterizerStatePlatform(const wdGALRasterizerStateCreationDescription& Description)
{
  wdGALRasterizerStateDX11* pDX11RasterizerState = WD_NEW(&m_Allocator, wdGALRasterizerStateDX11, Description);

  if (pDX11RasterizerState->InitPlatform(this).Succeeded())
  {
    return pDX11RasterizerState;
  }
  else
  {
    WD_DELETE(&m_Allocator, pDX11RasterizerState);
    return nullptr;
  }
}

void wdGALDeviceDX11::DestroyRasterizerStatePlatform(wdGALRasterizerState* pRasterizerState)
{
  wdGALRasterizerStateDX11* pDX11RasterizerState = static_cast<wdGALRasterizerStateDX11*>(pRasterizerState);
  pDX11RasterizerState->DeInitPlatform(this).IgnoreResult();
  WD_DELETE(&m_Allocator, pDX11RasterizerState);
}

wdGALSamplerState* wdGALDeviceDX11::CreateSamplerStatePlatform(const wdGALSamplerStateCreationDescription& Description)
{
  wdGALSamplerStateDX11* pDX11SamplerState = WD_NEW(&m_Allocator, wdGALSamplerStateDX11, Description);

  if (pDX11SamplerState->InitPlatform(this).Succeeded())
  {
    return pDX11SamplerState;
  }
  else
  {
    WD_DELETE(&m_Allocator, pDX11SamplerState);
    return nullptr;
  }
}

void wdGALDeviceDX11::DestroySamplerStatePlatform(wdGALSamplerState* pSamplerState)
{
  wdGALSamplerStateDX11* pDX11SamplerState = static_cast<wdGALSamplerStateDX11*>(pSamplerState);
  pDX11SamplerState->DeInitPlatform(this).IgnoreResult();
  WD_DELETE(&m_Allocator, pDX11SamplerState);
}


// Resource creation functions

wdGALShader* wdGALDeviceDX11::CreateShaderPlatform(const wdGALShaderCreationDescription& Description)
{
  wdGALShaderDX11* pShader = WD_NEW(&m_Allocator, wdGALShaderDX11, Description);

  if (!pShader->InitPlatform(this).Succeeded())
  {
    WD_DELETE(&m_Allocator, pShader);
    return nullptr;
  }

  return pShader;
}

void wdGALDeviceDX11::DestroyShaderPlatform(wdGALShader* pShader)
{
  wdGALShaderDX11* pDX11Shader = static_cast<wdGALShaderDX11*>(pShader);
  pDX11Shader->DeInitPlatform(this).IgnoreResult();
  WD_DELETE(&m_Allocator, pDX11Shader);
}

wdGALBuffer* wdGALDeviceDX11::CreateBufferPlatform(const wdGALBufferCreationDescription& Description, wdArrayPtr<const wdUInt8> pInitialData)
{
  wdGALBufferDX11* pBuffer = WD_NEW(&m_Allocator, wdGALBufferDX11, Description);

  if (!pBuffer->InitPlatform(this, pInitialData).Succeeded())
  {
    WD_DELETE(&m_Allocator, pBuffer);
    return nullptr;
  }

  return pBuffer;
}

void wdGALDeviceDX11::DestroyBufferPlatform(wdGALBuffer* pBuffer)
{
  wdGALBufferDX11* pDX11Buffer = static_cast<wdGALBufferDX11*>(pBuffer);
  pDX11Buffer->DeInitPlatform(this).IgnoreResult();
  WD_DELETE(&m_Allocator, pDX11Buffer);
}

wdGALTexture* wdGALDeviceDX11::CreateTexturePlatform(const wdGALTextureCreationDescription& Description, wdArrayPtr<wdGALSystemMemoryDescription> pInitialData)
{
  wdGALTextureDX11* pTexture = WD_NEW(&m_Allocator, wdGALTextureDX11, Description);

  if (!pTexture->InitPlatform(this, pInitialData).Succeeded())
  {
    WD_DELETE(&m_Allocator, pTexture);
    return nullptr;
  }

  return pTexture;
}

void wdGALDeviceDX11::DestroyTexturePlatform(wdGALTexture* pTexture)
{
  wdGALTextureDX11* pDX11Texture = static_cast<wdGALTextureDX11*>(pTexture);
  pDX11Texture->DeInitPlatform(this).IgnoreResult();
  WD_DELETE(&m_Allocator, pDX11Texture);
}

wdGALResourceView* wdGALDeviceDX11::CreateResourceViewPlatform(wdGALResourceBase* pResource, const wdGALResourceViewCreationDescription& Description)
{
  wdGALResourceViewDX11* pResourceView = WD_NEW(&m_Allocator, wdGALResourceViewDX11, pResource, Description);

  if (!pResourceView->InitPlatform(this).Succeeded())
  {
    WD_DELETE(&m_Allocator, pResourceView);
    return nullptr;
  }

  return pResourceView;
}

void wdGALDeviceDX11::DestroyResourceViewPlatform(wdGALResourceView* pResourceView)
{
  wdGALResourceViewDX11* pDX11ResourceView = static_cast<wdGALResourceViewDX11*>(pResourceView);
  pDX11ResourceView->DeInitPlatform(this).IgnoreResult();
  WD_DELETE(&m_Allocator, pDX11ResourceView);
}

wdGALRenderTargetView* wdGALDeviceDX11::CreateRenderTargetViewPlatform(wdGALTexture* pTexture, const wdGALRenderTargetViewCreationDescription& Description)
{
  wdGALRenderTargetViewDX11* pRTView = WD_NEW(&m_Allocator, wdGALRenderTargetViewDX11, pTexture, Description);

  if (!pRTView->InitPlatform(this).Succeeded())
  {
    WD_DELETE(&m_Allocator, pRTView);
    return nullptr;
  }

  return pRTView;
}

void wdGALDeviceDX11::DestroyRenderTargetViewPlatform(wdGALRenderTargetView* pRenderTargetView)
{
  wdGALRenderTargetViewDX11* pDX11RenderTargetView = static_cast<wdGALRenderTargetViewDX11*>(pRenderTargetView);
  pDX11RenderTargetView->DeInitPlatform(this).IgnoreResult();
  WD_DELETE(&m_Allocator, pDX11RenderTargetView);
}

wdGALUnorderedAccessView* wdGALDeviceDX11::CreateUnorderedAccessViewPlatform(wdGALResourceBase* pTextureOfBuffer, const wdGALUnorderedAccessViewCreationDescription& Description)
{
  wdGALUnorderedAccessViewDX11* pUnorderedAccessView = WD_NEW(&m_Allocator, wdGALUnorderedAccessViewDX11, pTextureOfBuffer, Description);

  if (!pUnorderedAccessView->InitPlatform(this).Succeeded())
  {
    WD_DELETE(&m_Allocator, pUnorderedAccessView);
    return nullptr;
  }

  return pUnorderedAccessView;
}

void wdGALDeviceDX11::DestroyUnorderedAccessViewPlatform(wdGALUnorderedAccessView* pUnorderedAccessView)
{
  wdGALUnorderedAccessViewDX11* pUnorderedAccessViewDX11 = static_cast<wdGALUnorderedAccessViewDX11*>(pUnorderedAccessView);
  pUnorderedAccessViewDX11->DeInitPlatform(this).IgnoreResult();
  WD_DELETE(&m_Allocator, pUnorderedAccessViewDX11);
}



// Other rendering creation functions

wdGALQuery* wdGALDeviceDX11::CreateQueryPlatform(const wdGALQueryCreationDescription& Description)
{
  wdGALQueryDX11* pQuery = WD_NEW(&m_Allocator, wdGALQueryDX11, Description);

  if (!pQuery->InitPlatform(this).Succeeded())
  {
    WD_DELETE(&m_Allocator, pQuery);
    return nullptr;
  }

  return pQuery;
}

void wdGALDeviceDX11::DestroyQueryPlatform(wdGALQuery* pQuery)
{
  wdGALQueryDX11* pQueryDX11 = static_cast<wdGALQueryDX11*>(pQuery);
  pQueryDX11->DeInitPlatform(this).IgnoreResult();
  WD_DELETE(&m_Allocator, pQueryDX11);
}

wdGALVertexDeclaration* wdGALDeviceDX11::CreateVertexDeclarationPlatform(const wdGALVertexDeclarationCreationDescription& Description)
{
  wdGALVertexDeclarationDX11* pVertexDeclaration = WD_NEW(&m_Allocator, wdGALVertexDeclarationDX11, Description);

  if (pVertexDeclaration->InitPlatform(this).Succeeded())
  {
    return pVertexDeclaration;
  }
  else
  {
    WD_DELETE(&m_Allocator, pVertexDeclaration);
    return nullptr;
  }
}

void wdGALDeviceDX11::DestroyVertexDeclarationPlatform(wdGALVertexDeclaration* pVertexDeclaration)
{
  wdGALVertexDeclarationDX11* pVertexDeclarationDX11 = static_cast<wdGALVertexDeclarationDX11*>(pVertexDeclaration);
  pVertexDeclarationDX11->DeInitPlatform(this).IgnoreResult();
  WD_DELETE(&m_Allocator, pVertexDeclarationDX11);
}

wdGALTimestampHandle wdGALDeviceDX11::GetTimestampPlatform()
{
  wdUInt32 uiIndex = m_uiNextTimestamp;
  m_uiNextTimestamp = (m_uiNextTimestamp + 1) % m_Timestamps.GetCount();
  return {uiIndex, m_uiFrameCounter};
}

wdResult wdGALDeviceDX11::GetTimestampResultPlatform(wdGALTimestampHandle hTimestamp, wdTime& result)
{
  // Check whether frequency and sync timer are already available for the frame of the timestamp
  wdUInt64 uiFrameCounter = hTimestamp.m_uiFrameCounter;

  PerFrameData* pPerFrameData = nullptr;
  for (wdUInt32 i = 0; i < WD_ARRAY_SIZE(m_PerFrameData); ++i)
  {
    if (m_PerFrameData[i].m_uiFrame == uiFrameCounter && m_PerFrameData[i].m_fInvTicksPerSecond >= 0.0)
    {
      pPerFrameData = &m_PerFrameData[i];
      break;
    }
  }

  if (pPerFrameData == nullptr)
  {
    return WD_FAILURE;
  }

  ID3D11Query* pQuery = GetTimestamp(hTimestamp);

  wdUInt64 uiTimestamp;
  if (FAILED(m_pImmediateContext->GetData(pQuery, &uiTimestamp, sizeof(uiTimestamp), D3D11_ASYNC_GETDATA_DONOTFLUSH)))
  {
    return WD_FAILURE;
  }

  if (pPerFrameData->m_fInvTicksPerSecond == 0.0)
  {
    result.SetZero();
  }
  else
  {
    result = wdTime::Seconds(double(uiTimestamp) * pPerFrameData->m_fInvTicksPerSecond) + m_SyncTimeDiff;
  }
  return WD_SUCCESS;
}

// Swap chain functions

void wdGALDeviceDX11::PresentPlatform(const wdGALSwapChain* pSwapChain, bool bVSync)
{
}

// Misc functions

void wdGALDeviceDX11::BeginFramePlatform(const wdUInt64 uiRenderFrame)
{
  auto& pCommandEncoder = m_pDefaultPass->m_pCommandEncoderImpl;

  wdStringBuilder sb;
  sb.Format("Frame {}", uiRenderFrame);

#if WD_ENABLED(WD_USE_PROFILING)
  m_pFrameTimingScope = wdProfilingScopeAndMarker::Start(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), sb);
#endif

  // check if fence is reached and wait if the disjoint timer is about to be re-used
  {
    auto& perFrameData = m_PerFrameData[m_uiCurrentPerFrameData];
    if (perFrameData.m_uiFrame != ((wdUInt64)-1))
    {

      bool bFenceReached = IsFenceReachedPlatform(GetDXImmediateContext(), perFrameData.m_pFence);
      if (!bFenceReached && m_uiNextPerFrameData == m_uiCurrentPerFrameData)
      {
        WaitForFencePlatform(GetDXImmediateContext(), perFrameData.m_pFence);
      }
    }
  }

  {
    auto& perFrameData = m_PerFrameData[m_uiNextPerFrameData];
    m_pImmediateContext->Begin(perFrameData.m_pDisjointTimerQuery);

    perFrameData.m_fInvTicksPerSecond = -1.0f;
  }
}

void wdGALDeviceDX11::EndFramePlatform()
{
  auto& pCommandEncoder = m_pDefaultPass->m_pCommandEncoderImpl;

#if WD_ENABLED(WD_USE_PROFILING)
  wdProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), m_pFrameTimingScope);
#endif

  // end disjoint query
  {
    auto& perFrameData = m_PerFrameData[m_uiNextPerFrameData];
    m_pImmediateContext->End(perFrameData.m_pDisjointTimerQuery);
  }

  // check if fence is reached and update per frame data
  {
    auto& perFrameData = m_PerFrameData[m_uiCurrentPerFrameData];
    if (perFrameData.m_uiFrame != ((wdUInt64)-1))
    {
      if (IsFenceReachedPlatform(GetDXImmediateContext(), perFrameData.m_pFence))
      {
        FreeTempResources(perFrameData.m_uiFrame);

        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT data;
        if (FAILED(m_pImmediateContext->GetData(perFrameData.m_pDisjointTimerQuery, &data, sizeof(data), D3D11_ASYNC_GETDATA_DONOTFLUSH)) || data.Disjoint)
        {
          perFrameData.m_fInvTicksPerSecond = 0.0f;
        }
        else
        {
          perFrameData.m_fInvTicksPerSecond = 1.0 / (double)data.Frequency;

          if (m_bSyncTimeNeeded)
          {
            wdGALTimestampHandle hTimestamp = m_pDefaultPass->m_pRenderCommandEncoder->InsertTimestamp();
            ID3D11Query* pQuery = GetTimestamp(hTimestamp);

            wdUInt64 uiTimestamp;
            while (m_pImmediateContext->GetData(pQuery, &uiTimestamp, sizeof(uiTimestamp), 0) != S_OK)
            {
              wdThreadUtils::YieldTimeSlice();
            }

            m_SyncTimeDiff = wdTime::Now() - wdTime::Seconds(double(uiTimestamp) * perFrameData.m_fInvTicksPerSecond);
            m_bSyncTimeNeeded = false;
          }
        }

        m_uiCurrentPerFrameData = (m_uiCurrentPerFrameData + 1) % WD_ARRAY_SIZE(m_PerFrameData);
      }
    }
  }

  {
    auto& perFrameData = m_PerFrameData[m_uiNextPerFrameData];
    perFrameData.m_uiFrame = m_uiFrameCounter;

    // insert fence
    InsertFencePlatform(GetDXImmediateContext(), perFrameData.m_pFence);

    m_uiNextPerFrameData = (m_uiNextPerFrameData + 1) % WD_ARRAY_SIZE(m_PerFrameData);
  }

  ++m_uiFrameCounter;
}

void wdGALDeviceDX11::FillCapabilitiesPlatform()
{
  {
    DXGI_ADAPTER_DESC1 adapterDesc;
    m_pDXGIAdapter->GetDesc1(&adapterDesc);

    m_Capabilities.m_sAdapterName = wdStringUtf8(adapterDesc.Description).GetData();
    m_Capabilities.m_uiDedicatedVRAM = static_cast<wdUInt64>(adapterDesc.DedicatedVideoMemory);
    m_Capabilities.m_uiDedicatedSystemRAM = static_cast<wdUInt64>(adapterDesc.DedicatedSystemMemory);
    m_Capabilities.m_uiSharedSystemRAM = static_cast<wdUInt64>(adapterDesc.SharedSystemMemory);
    m_Capabilities.m_bHardwareAccelerated = (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0;
  }

  m_Capabilities.m_bMultithreadedResourceCreation = true;

  switch (m_uiFeatureLevel)
  {
    case D3D_FEATURE_LEVEL_11_1:
      m_Capabilities.m_bB5G6R5Textures = true;
      m_Capabilities.m_bNoOverwriteBufferUpdate = true;

    case D3D_FEATURE_LEVEL_11_0:
      m_Capabilities.m_bShaderStageSupported[wdGALShaderStage::VertexShader] = true;
      m_Capabilities.m_bShaderStageSupported[wdGALShaderStage::HullShader] = true;
      m_Capabilities.m_bShaderStageSupported[wdGALShaderStage::DomainShader] = true;
      m_Capabilities.m_bShaderStageSupported[wdGALShaderStage::GeometryShader] = true;
      m_Capabilities.m_bShaderStageSupported[wdGALShaderStage::PixelShader] = true;
      m_Capabilities.m_bShaderStageSupported[wdGALShaderStage::ComputeShader] = true;
      m_Capabilities.m_bInstancing = true;
      m_Capabilities.m_b32BitIndices = true;
      m_Capabilities.m_bIndirectDraw = true;
      m_Capabilities.m_bStreamOut = true;
      m_Capabilities.m_uiMaxConstantBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_HW_SLOT_COUNT;
      m_Capabilities.m_bTextureArrays = true;
      m_Capabilities.m_bCubemapArrays = true;
      m_Capabilities.m_uiMaxTextureDimension = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
      m_Capabilities.m_uiMaxCubemapDimension = D3D11_REQ_TEXTURECUBE_DIMENSION;
      m_Capabilities.m_uiMax3DTextureDimension = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
      m_Capabilities.m_uiMaxAnisotropy = D3D11_REQ_MAXANISOTROPY;
      m_Capabilities.m_uiMaxRendertargets = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
      m_Capabilities.m_uiUAVCount = (m_uiFeatureLevel == D3D_FEATURE_LEVEL_11_1 ? 64 : 8);
      m_Capabilities.m_bAlphaToCoverage = true;
      break;

    case D3D_FEATURE_LEVEL_10_1:
    case D3D_FEATURE_LEVEL_10_0:
      m_Capabilities.m_bShaderStageSupported[wdGALShaderStage::VertexShader] = true;
      m_Capabilities.m_bShaderStageSupported[wdGALShaderStage::HullShader] = false;
      m_Capabilities.m_bShaderStageSupported[wdGALShaderStage::DomainShader] = false;
      m_Capabilities.m_bShaderStageSupported[wdGALShaderStage::GeometryShader] = true;
      m_Capabilities.m_bShaderStageSupported[wdGALShaderStage::PixelShader] = true;
      m_Capabilities.m_bShaderStageSupported[wdGALShaderStage::ComputeShader] = false;
      m_Capabilities.m_bInstancing = true;
      m_Capabilities.m_b32BitIndices = true;
      m_Capabilities.m_bIndirectDraw = false;
      m_Capabilities.m_bStreamOut = true;
      m_Capabilities.m_uiMaxConstantBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_HW_SLOT_COUNT;
      m_Capabilities.m_bTextureArrays = true;
      m_Capabilities.m_bCubemapArrays = (m_uiFeatureLevel == D3D_FEATURE_LEVEL_10_1 ? true : false);
      m_Capabilities.m_uiMaxTextureDimension = D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION;
      m_Capabilities.m_uiMaxCubemapDimension = D3D10_REQ_TEXTURECUBE_DIMENSION;
      m_Capabilities.m_uiMax3DTextureDimension = D3D10_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
      m_Capabilities.m_uiMaxAnisotropy = D3D10_REQ_MAXANISOTROPY;
      m_Capabilities.m_uiMaxRendertargets = D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT;
      m_Capabilities.m_uiUAVCount = 0;
      m_Capabilities.m_bAlphaToCoverage = true;
      break;

    case D3D_FEATURE_LEVEL_9_3:
      m_Capabilities.m_bShaderStageSupported[wdGALShaderStage::VertexShader] = true;
      m_Capabilities.m_bShaderStageSupported[wdGALShaderStage::HullShader] = false;
      m_Capabilities.m_bShaderStageSupported[wdGALShaderStage::DomainShader] = false;
      m_Capabilities.m_bShaderStageSupported[wdGALShaderStage::GeometryShader] = false;
      m_Capabilities.m_bShaderStageSupported[wdGALShaderStage::PixelShader] = true;
      m_Capabilities.m_bShaderStageSupported[wdGALShaderStage::ComputeShader] = false;
      m_Capabilities.m_bInstancing = true;
      m_Capabilities.m_b32BitIndices = true;
      m_Capabilities.m_bIndirectDraw = false;
      m_Capabilities.m_bStreamOut = false;
      m_Capabilities.m_uiMaxConstantBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_HW_SLOT_COUNT;
      m_Capabilities.m_bTextureArrays = false;
      m_Capabilities.m_bCubemapArrays = false;
      m_Capabilities.m_uiMaxTextureDimension = D3D_FL9_3_REQ_TEXTURE1D_U_DIMENSION;
      m_Capabilities.m_uiMaxCubemapDimension = D3D_FL9_3_REQ_TEXTURECUBE_DIMENSION;
      m_Capabilities.m_uiMax3DTextureDimension = 0;
      m_Capabilities.m_uiMaxAnisotropy = 16;
      m_Capabilities.m_uiMaxRendertargets = D3D_FL9_3_SIMULTANEOUS_RENDER_TARGET_COUNT;
      m_Capabilities.m_uiUAVCount = 0;
      m_Capabilities.m_bAlphaToCoverage = false;
      break;

    default:
      WD_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  if (m_pDevice3)
  {
    D3D11_FEATURE_DATA_D3D11_OPTIONS2 featureOpts2;
    if (SUCCEEDED(m_pDevice3->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &featureOpts2, sizeof(featureOpts2))))
    {
      m_Capabilities.m_bConservativeRasterization = (featureOpts2.ConservativeRasterizationTier != D3D11_CONSERVATIVE_RASTERIZATION_NOT_SUPPORTED);
    }

    D3D11_FEATURE_DATA_D3D11_OPTIONS3 featureOpts3;
    if (SUCCEEDED(m_pDevice3->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS3, &featureOpts3, sizeof(featureOpts3))))
    {
      m_Capabilities.m_bVertexShaderRenderTargetArrayIndex = featureOpts3.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer != 0;
    }
  }
}

void wdGALDeviceDX11::WaitIdlePlatform()
{
  m_pImmediateContext->Flush();
  DestroyDeadObjects();
}

ID3D11Resource* wdGALDeviceDX11::FindTempBuffer(wdUInt32 uiSize)
{
  const wdUInt32 uiExpGrowthLimit = 16 * 1024 * 1024;

  uiSize = wdMath::Max(uiSize, 256U);
  if (uiSize < uiExpGrowthLimit)
  {
    uiSize = wdMath::PowerOfTwo_Ceil(uiSize);
  }
  else
  {
    uiSize = wdMemoryUtils::AlignSize(uiSize, uiExpGrowthLimit);
  }

  ID3D11Resource* pResource = nullptr;
  auto it = m_FreeTempResources[TempResourceType::Buffer].Find(uiSize);
  if (it.IsValid())
  {
    wdDynamicArray<ID3D11Resource*>& resources = it.Value();
    if (!resources.IsEmpty())
    {
      pResource = resources[0];
      resources.RemoveAtAndSwap(0);
    }
  }

  if (pResource == nullptr)
  {
    D3D11_BUFFER_DESC desc;
    desc.ByteWidth = uiSize;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    ID3D11Buffer* pBuffer = nullptr;
    if (!SUCCEEDED(m_pDevice->CreateBuffer(&desc, nullptr, &pBuffer)))
    {
      return nullptr;
    }

    pResource = pBuffer;
  }

  auto& tempResource = m_UsedTempResources[TempResourceType::Buffer].ExpandAndGetRef();
  tempResource.m_pResource = pResource;
  tempResource.m_uiFrame = m_uiFrameCounter;
  tempResource.m_uiHash = uiSize;

  return pResource;
}


ID3D11Resource* wdGALDeviceDX11::FindTempTexture(wdUInt32 uiWidth, wdUInt32 uiHeight, wdUInt32 uiDepth, wdGALResourceFormat::Enum format)
{
  wdUInt32 data[] = {uiWidth, uiHeight, uiDepth, (wdUInt32)format};
  wdUInt32 uiHash = wdHashingUtils::xxHash32(data, sizeof(data));

  ID3D11Resource* pResource = nullptr;
  auto it = m_FreeTempResources[TempResourceType::Texture].Find(uiHash);
  if (it.IsValid())
  {
    wdDynamicArray<ID3D11Resource*>& resources = it.Value();
    if (!resources.IsEmpty())
    {
      pResource = resources[0];
      resources.RemoveAtAndSwap(0);
    }
  }

  if (pResource == nullptr)
  {
    if (uiDepth == 1)
    {
      D3D11_TEXTURE2D_DESC desc;
      desc.Width = uiWidth;
      desc.Height = uiHeight;
      desc.MipLevels = 1;
      desc.ArraySize = 1;
      desc.Format = GetFormatLookupTable().GetFormatInfo(format).m_eStorage;
      desc.SampleDesc.Count = 1;
      desc.SampleDesc.Quality = 0;
      desc.Usage = D3D11_USAGE_STAGING;
      desc.BindFlags = 0;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      desc.MiscFlags = 0;

      ID3D11Texture2D* pTexture = nullptr;
      if (!SUCCEEDED(m_pDevice->CreateTexture2D(&desc, nullptr, &pTexture)))
      {
        return nullptr;
      }

      pResource = pTexture;
    }
    else
    {
      WD_ASSERT_NOT_IMPLEMENTED;
      return nullptr;
    }
  }

  auto& tempResource = m_UsedTempResources[TempResourceType::Texture].ExpandAndGetRef();
  tempResource.m_pResource = pResource;
  tempResource.m_uiFrame = m_uiFrameCounter;
  tempResource.m_uiHash = uiHash;

  return pResource;
}

void wdGALDeviceDX11::FreeTempResources(wdUInt64 uiFrame)
{
  for (wdUInt32 type = 0; type < TempResourceType::ENUM_COUNT; ++type)
  {
    while (!m_UsedTempResources[type].IsEmpty())
    {
      auto& usedTempResource = m_UsedTempResources[type].PeekFront();
      if (usedTempResource.m_uiFrame == uiFrame)
      {
        auto it = m_FreeTempResources[type].Find(usedTempResource.m_uiHash);
        if (!it.IsValid())
        {
          it = m_FreeTempResources[type].Insert(usedTempResource.m_uiHash, wdDynamicArray<ID3D11Resource*>(&m_Allocator));
        }

        it.Value().PushBack(usedTempResource.m_pResource);
        m_UsedTempResources[type].PopFront();
      }
      else
      {
        break;
      }
    }
  }
}

void wdGALDeviceDX11::FillFormatLookupTable()
{
  ///       The list below is in the same order as the wdGALResourceFormat enum. No format should be missing except the ones that are just
  ///       different names for the same enum value.

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGBAFloat, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32A32_TYPELESS).RT(DXGI_FORMAT_R32G32B32A32_FLOAT).VA(DXGI_FORMAT_R32G32B32A32_FLOAT).RV(DXGI_FORMAT_R32G32B32A32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGBAUInt, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32A32_TYPELESS).RT(DXGI_FORMAT_R32G32B32A32_UINT).VA(DXGI_FORMAT_R32G32B32A32_UINT).RV(DXGI_FORMAT_R32G32B32A32_UINT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGBAInt, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32A32_TYPELESS).RT(DXGI_FORMAT_R32G32B32A32_SINT).VA(DXGI_FORMAT_R32G32B32A32_SINT).RV(DXGI_FORMAT_R32G32B32A32_SINT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGBFloat, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32_TYPELESS).RT(DXGI_FORMAT_R32G32B32_FLOAT).VA(DXGI_FORMAT_R32G32B32_FLOAT).RV(DXGI_FORMAT_R32G32B32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGBUInt, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32_TYPELESS).RT(DXGI_FORMAT_R32G32B32_UINT).VA(DXGI_FORMAT_R32G32B32_UINT).RV(DXGI_FORMAT_R32G32B32_UINT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGBInt, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32_TYPELESS).RT(DXGI_FORMAT_R32G32B32_SINT).VA(DXGI_FORMAT_R32G32B32_SINT).RV(DXGI_FORMAT_R32G32B32_SINT));

  // Supported with DX 11.1
  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::B5G6R5UNormalized, wdGALFormatLookupEntryDX11(DXGI_FORMAT_B5G6R5_UNORM).RT(DXGI_FORMAT_B5G6R5_UNORM).VA(DXGI_FORMAT_B5G6R5_UNORM).RV(DXGI_FORMAT_B5G6R5_UNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::BGRAUByteNormalized, wdGALFormatLookupEntryDX11(DXGI_FORMAT_B8G8R8A8_TYPELESS).RT(DXGI_FORMAT_B8G8R8A8_UNORM).VA(DXGI_FORMAT_B8G8R8A8_UNORM).RV(DXGI_FORMAT_B8G8R8A8_UNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::BGRAUByteNormalizedsRGB, wdGALFormatLookupEntryDX11(DXGI_FORMAT_B8G8R8A8_TYPELESS).RT(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB).RV(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGBAHalf, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_FLOAT).VA(DXGI_FORMAT_R16G16B16A16_FLOAT).RV(DXGI_FORMAT_R16G16B16A16_FLOAT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGBAUShort, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_UINT).VA(DXGI_FORMAT_R16G16B16A16_UINT).RV(DXGI_FORMAT_R16G16B16A16_UINT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGBAUShortNormalized, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_UNORM).VA(DXGI_FORMAT_R16G16B16A16_UNORM).RV(DXGI_FORMAT_R16G16B16A16_UNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGBAShort, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_SINT).VA(DXGI_FORMAT_R16G16B16A16_SINT).RV(DXGI_FORMAT_R16G16B16A16_SINT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGBAShortNormalized, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_SNORM).VA(DXGI_FORMAT_R16G16B16A16_SNORM).RV(DXGI_FORMAT_R16G16B16A16_SNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGFloat, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32_TYPELESS).RT(DXGI_FORMAT_R32G32_FLOAT).VA(DXGI_FORMAT_R32G32_FLOAT).RV(DXGI_FORMAT_R32G32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGUInt, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32_TYPELESS).RT(DXGI_FORMAT_R32G32_UINT).VA(DXGI_FORMAT_R32G32_UINT).RV(DXGI_FORMAT_R32G32_UINT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGInt, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32_TYPELESS).RT(DXGI_FORMAT_R32G32_SINT).VA(DXGI_FORMAT_R32G32_SINT).RV(DXGI_FORMAT_R32G32_SINT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGB10A2UInt, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R10G10B10A2_TYPELESS).RT(DXGI_FORMAT_R10G10B10A2_UINT).VA(DXGI_FORMAT_R10G10B10A2_UINT).RV(DXGI_FORMAT_R10G10B10A2_UINT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGB10A2UIntNormalized, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R10G10B10A2_TYPELESS).RT(DXGI_FORMAT_R10G10B10A2_UNORM).VA(DXGI_FORMAT_R10G10B10A2_UNORM).RV(DXGI_FORMAT_R10G10B10A2_UNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RG11B10Float, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R11G11B10_FLOAT).RT(DXGI_FORMAT_R11G11B10_FLOAT).VA(DXGI_FORMAT_R11G11B10_FLOAT).RV(DXGI_FORMAT_R11G11B10_FLOAT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGBAUByteNormalized, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_UNORM).VA(DXGI_FORMAT_R8G8B8A8_UNORM).RV(DXGI_FORMAT_R8G8B8A8_UNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGBAUByteNormalizedsRGB, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB).RV(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGBAUByte, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_UINT).VA(DXGI_FORMAT_R8G8B8A8_UINT).RV(DXGI_FORMAT_R8G8B8A8_UINT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGBAByteNormalized, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_SNORM).VA(DXGI_FORMAT_R8G8B8A8_SNORM).RV(DXGI_FORMAT_R8G8B8A8_SNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGBAByte, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_SINT).VA(DXGI_FORMAT_R8G8B8A8_SINT).RV(DXGI_FORMAT_R8G8B8A8_SINT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGHalf, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_FLOAT).VA(DXGI_FORMAT_R16G16_FLOAT).RV(DXGI_FORMAT_R16G16_FLOAT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGUShort, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_UINT).VA(DXGI_FORMAT_R16G16_UINT).RV(DXGI_FORMAT_R16G16_UINT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGUShortNormalized, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_UNORM).VA(DXGI_FORMAT_R16G16_UNORM).RV(DXGI_FORMAT_R16G16_UNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGShort, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_SINT).VA(DXGI_FORMAT_R16G16_SINT).RV(DXGI_FORMAT_R16G16_SINT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGShortNormalized, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16_TYPELESS).RT(DXGI_FORMAT_R16G16_SNORM).VA(DXGI_FORMAT_R16G16_SNORM).RV(DXGI_FORMAT_R16G16_SNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGUByte, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8_TYPELESS).RT(DXGI_FORMAT_R8G8_UINT).VA(DXGI_FORMAT_R8G8_UINT).RV(DXGI_FORMAT_R8G8_UINT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGUByteNormalized, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8_TYPELESS).RT(DXGI_FORMAT_R8G8_UNORM).VA(DXGI_FORMAT_R8G8_UNORM).RV(DXGI_FORMAT_R8G8_UNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGByte, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8_TYPELESS).RT(DXGI_FORMAT_R8G8_SINT).VA(DXGI_FORMAT_R8G8_SINT).RV(DXGI_FORMAT_R8G8_SINT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RGByteNormalized, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8_TYPELESS).RT(DXGI_FORMAT_R8G8_SNORM).VA(DXGI_FORMAT_R8G8_SNORM).RV(DXGI_FORMAT_R8G8_SNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::DFloat, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).RV(DXGI_FORMAT_R32_FLOAT).D(DXGI_FORMAT_R32_FLOAT).DS(DXGI_FORMAT_D32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RFloat, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).RT(DXGI_FORMAT_R32_FLOAT).VA(DXGI_FORMAT_R32_FLOAT).RV(DXGI_FORMAT_R32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RUInt, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).RT(DXGI_FORMAT_R32_UINT).VA(DXGI_FORMAT_R32_UINT).RV(DXGI_FORMAT_R32_UINT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RInt, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).RT(DXGI_FORMAT_R32_SINT).VA(DXGI_FORMAT_R32_SINT).RV(DXGI_FORMAT_R32_SINT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RHalf, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_FLOAT).VA(DXGI_FORMAT_R16_FLOAT).RV(DXGI_FORMAT_R16_FLOAT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RUShort, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_UINT).VA(DXGI_FORMAT_R16_UINT).RV(DXGI_FORMAT_R16_UINT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RUShortNormalized, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_UNORM).VA(DXGI_FORMAT_R16_UNORM).RV(DXGI_FORMAT_R16_UNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RShort, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_SINT).VA(DXGI_FORMAT_R16_SINT).RV(DXGI_FORMAT_R16_SINT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RShortNormalized, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RT(DXGI_FORMAT_R16_SNORM).VA(DXGI_FORMAT_R16_SNORM).RV(DXGI_FORMAT_R16_SNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RUByte, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).RT(DXGI_FORMAT_R8_UINT).VA(DXGI_FORMAT_R8_UINT).RV(DXGI_FORMAT_R8_UINT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RUByteNormalized, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).RT(DXGI_FORMAT_R8_UNORM).VA(DXGI_FORMAT_R8_UNORM).RV(DXGI_FORMAT_R8_UNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RByte, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).RT(DXGI_FORMAT_R8_SINT).VA(DXGI_FORMAT_R8_SINT).RV(DXGI_FORMAT_R8_SINT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::RByteNormalized, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R8_TYPELESS).RT(DXGI_FORMAT_R8_SNORM).VA(DXGI_FORMAT_R8_SNORM).RV(DXGI_FORMAT_R8_SNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::AUByteNormalized, wdGALFormatLookupEntryDX11(DXGI_FORMAT_A8_UNORM).RT(DXGI_FORMAT_A8_UNORM).VA(DXGI_FORMAT_A8_UNORM).RV(DXGI_FORMAT_A8_UNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::D16, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R16_TYPELESS).RV(DXGI_FORMAT_R16_UNORM).DS(DXGI_FORMAT_D16_UNORM).D(DXGI_FORMAT_R16_UNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::D24S8, wdGALFormatLookupEntryDX11(DXGI_FORMAT_R24G8_TYPELESS).DS(DXGI_FORMAT_D24_UNORM_S8_UINT).D(DXGI_FORMAT_R24_UNORM_X8_TYPELESS).S(DXGI_FORMAT_X24_TYPELESS_G8_UINT));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::BC1, wdGALFormatLookupEntryDX11(DXGI_FORMAT_BC1_TYPELESS).RV(DXGI_FORMAT_BC1_UNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::BC1sRGB, wdGALFormatLookupEntryDX11(DXGI_FORMAT_BC1_TYPELESS).RV(DXGI_FORMAT_BC1_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::BC2, wdGALFormatLookupEntryDX11(DXGI_FORMAT_BC2_TYPELESS).RV(DXGI_FORMAT_BC2_UNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::BC2sRGB, wdGALFormatLookupEntryDX11(DXGI_FORMAT_BC2_TYPELESS).RV(DXGI_FORMAT_BC2_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::BC3, wdGALFormatLookupEntryDX11(DXGI_FORMAT_BC3_TYPELESS).RV(DXGI_FORMAT_BC3_UNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::BC3sRGB, wdGALFormatLookupEntryDX11(DXGI_FORMAT_BC3_TYPELESS).RV(DXGI_FORMAT_BC3_UNORM_SRGB));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::BC4UNormalized, wdGALFormatLookupEntryDX11(DXGI_FORMAT_BC4_TYPELESS).RV(DXGI_FORMAT_BC4_UNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::BC4Normalized, wdGALFormatLookupEntryDX11(DXGI_FORMAT_BC4_TYPELESS).RV(DXGI_FORMAT_BC4_SNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::BC5UNormalized, wdGALFormatLookupEntryDX11(DXGI_FORMAT_BC5_TYPELESS).RV(DXGI_FORMAT_BC5_UNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::BC5Normalized, wdGALFormatLookupEntryDX11(DXGI_FORMAT_BC5_TYPELESS).RV(DXGI_FORMAT_BC5_SNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::BC6UFloat, wdGALFormatLookupEntryDX11(DXGI_FORMAT_BC6H_TYPELESS).RV(DXGI_FORMAT_BC6H_UF16));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::BC6Float, wdGALFormatLookupEntryDX11(DXGI_FORMAT_BC6H_TYPELESS).RV(DXGI_FORMAT_BC6H_SF16));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::BC7UNormalized, wdGALFormatLookupEntryDX11(DXGI_FORMAT_BC7_TYPELESS).RV(DXGI_FORMAT_BC7_UNORM));

  m_FormatLookupTable.SetFormatInfo(wdGALResourceFormat::BC7UNormalizedsRGB, wdGALFormatLookupEntryDX11(DXGI_FORMAT_BC7_TYPELESS).RV(DXGI_FORMAT_BC7_UNORM_SRGB));
}

wdGALRenderCommandEncoder* wdGALDeviceDX11::GetRenderCommandEncoder() const
{
  return m_pDefaultPass->m_pRenderCommandEncoder.Borrow();
}

void wdGALDeviceDX11::InsertFencePlatform(ID3D11DeviceContext* pContext, ID3D11Query* pFence)
{
  pContext->End(pFence);
}

bool wdGALDeviceDX11::IsFenceReachedPlatform(ID3D11DeviceContext* pContext, ID3D11Query* pFence)
{
  BOOL data = FALSE;
  if (pContext->GetData(pFence, &data, sizeof(data), 0) == S_OK)
  {
    WD_ASSERT_DEV(data == TRUE, "Implementation error");
    return true;
  }

  return false;
}

void wdGALDeviceDX11::WaitForFencePlatform(ID3D11DeviceContext* pContext, ID3D11Query* pFence)
{
  BOOL data = FALSE;
  while (pContext->GetData(pFence, &data, sizeof(data), 0) != S_OK)
  {
    wdThreadUtils::YieldTimeSlice();
  }

  WD_ASSERT_DEV(data == TRUE, "Implementation error");
}

WD_STATICLINK_FILE(RendererDX11, RendererDX11_Device_Implementation_DeviceDX11);
