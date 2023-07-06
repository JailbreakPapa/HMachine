
#pragma once

#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/Device/Device.h>

// TODO: This should not be included in a header, it exposes Windows.h to the outside
#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <dxgi.h>

struct ID3D11Device;
struct ID3D11Device3;
struct ID3D11DeviceContext;
struct ID3D11Debug;
struct IDXGIFactory1;
struct IDXGIAdapter1;
struct IDXGIDevice1;
struct ID3D11Resource;
struct ID3D11Query;
struct IDXGIAdapter;

typedef wdGALFormatLookupEntry<DXGI_FORMAT, (DXGI_FORMAT)0> wdGALFormatLookupEntryDX11;
typedef wdGALFormatLookupTable<wdGALFormatLookupEntryDX11> wdGALFormatLookupTableDX11;

class wdGALPassDX11;

/// \brief The DX11 device implementation of the graphics abstraction layer.
class WD_RENDERERDX11_DLL wdGALDeviceDX11 : public wdGALDevice
{
private:
  friend wdInternal::NewInstance<wdGALDevice> CreateDX11Device(wdAllocatorBase* pAllocator, const wdGALDeviceCreationDescription& description);
  wdGALDeviceDX11(const wdGALDeviceCreationDescription& Description);

public:
  virtual ~wdGALDeviceDX11();

public:
  ID3D11Device* GetDXDevice() const;
  ID3D11Device3* GetDXDevice3() const;
  ID3D11DeviceContext* GetDXImmediateContext() const;
  IDXGIFactory1* GetDXGIFactory() const;
  wdGALRenderCommandEncoder* GetRenderCommandEncoder() const;

  const wdGALFormatLookupTableDX11& GetFormatLookupTable() const;

  void ReportLiveGpuObjects();

  void FlushDeadObjects();

  // These functions need to be implemented by a render API abstraction
protected:
  // Init & shutdown functions

  /// \brief Internal version of device init that allows to modify device creation flags and graphics adapter.
  ///
  /// \param pUsedAdapter
  ///   Null means default adapter.
  wdResult InitPlatform(DWORD flags, IDXGIAdapter* pUsedAdapter);

  virtual wdResult InitPlatform() override;
  virtual wdResult ShutdownPlatform() override;

  // Pipeline & Pass functions

  virtual void BeginPipelinePlatform(const char* szName, wdGALSwapChain* pSwapChain) override;
  virtual void EndPipelinePlatform(wdGALSwapChain* pSwapChain) override;

  virtual wdGALPass* BeginPassPlatform(const char* szName) override;
  virtual void EndPassPlatform(wdGALPass* pPass) override;


  // State creation functions

  virtual wdGALBlendState* CreateBlendStatePlatform(const wdGALBlendStateCreationDescription& Description) override;
  virtual void DestroyBlendStatePlatform(wdGALBlendState* pBlendState) override;

  virtual wdGALDepthStencilState* CreateDepthStencilStatePlatform(const wdGALDepthStencilStateCreationDescription& Description) override;
  virtual void DestroyDepthStencilStatePlatform(wdGALDepthStencilState* pDepthStencilState) override;

  virtual wdGALRasterizerState* CreateRasterizerStatePlatform(const wdGALRasterizerStateCreationDescription& Description) override;
  virtual void DestroyRasterizerStatePlatform(wdGALRasterizerState* pRasterizerState) override;

  virtual wdGALSamplerState* CreateSamplerStatePlatform(const wdGALSamplerStateCreationDescription& Description) override;
  virtual void DestroySamplerStatePlatform(wdGALSamplerState* pSamplerState) override;


  // Resource creation functions

  virtual wdGALShader* CreateShaderPlatform(const wdGALShaderCreationDescription& Description) override;
  virtual void DestroyShaderPlatform(wdGALShader* pShader) override;

  virtual wdGALBuffer* CreateBufferPlatform(const wdGALBufferCreationDescription& Description, wdArrayPtr<const wdUInt8> pInitialData) override;
  virtual void DestroyBufferPlatform(wdGALBuffer* pBuffer) override;

  virtual wdGALTexture* CreateTexturePlatform(const wdGALTextureCreationDescription& Description, wdArrayPtr<wdGALSystemMemoryDescription> pInitialData) override;
  virtual void DestroyTexturePlatform(wdGALTexture* pTexture) override;

  virtual wdGALResourceView* CreateResourceViewPlatform(wdGALResourceBase* pResource, const wdGALResourceViewCreationDescription& Description) override;
  virtual void DestroyResourceViewPlatform(wdGALResourceView* pResourceView) override;

  virtual wdGALRenderTargetView* CreateRenderTargetViewPlatform(wdGALTexture* pTexture, const wdGALRenderTargetViewCreationDescription& Description) override;
  virtual void DestroyRenderTargetViewPlatform(wdGALRenderTargetView* pRenderTargetView) override;

  wdGALUnorderedAccessView* CreateUnorderedAccessViewPlatform(wdGALResourceBase* pResource, const wdGALUnorderedAccessViewCreationDescription& Description) override;
  virtual void DestroyUnorderedAccessViewPlatform(wdGALUnorderedAccessView* pUnorderedAccessView) override;

  // Other rendering creation functions

  virtual wdGALQuery* CreateQueryPlatform(const wdGALQueryCreationDescription& Description) override;
  virtual void DestroyQueryPlatform(wdGALQuery* pQuery) override;

  virtual wdGALVertexDeclaration* CreateVertexDeclarationPlatform(const wdGALVertexDeclarationCreationDescription& Description) override;
  virtual void DestroyVertexDeclarationPlatform(wdGALVertexDeclaration* pVertexDeclaration) override;

  // Timestamp functions

  virtual wdGALTimestampHandle GetTimestampPlatform() override;
  virtual wdResult GetTimestampResultPlatform(wdGALTimestampHandle hTimestamp, wdTime& result) override;

  // Swap chain functions

  void PresentPlatform(const wdGALSwapChain* pSwapChain, bool bVSync);

  // Misc functions

  virtual void BeginFramePlatform(const wdUInt64 uiRenderFrame) override;
  virtual void EndFramePlatform() override;

  virtual void FillCapabilitiesPlatform() override;

  virtual void WaitIdlePlatform() override;

  /// \endcond

private:
  friend class wdGALCommandEncoderImplDX11;

  struct TempResourceType
  {
    enum Enum
    {
      Buffer,
      Texture,

      ENUM_COUNT
    };
  };

  ID3D11Query* GetTimestamp(wdGALTimestampHandle hTimestamp);

  ID3D11Resource* FindTempBuffer(wdUInt32 uiSize);
  ID3D11Resource* FindTempTexture(wdUInt32 uiWidth, wdUInt32 uiHeight, wdUInt32 uiDepth, wdGALResourceFormat::Enum format);
  void FreeTempResources(wdUInt64 uiFrame);

  void FillFormatLookupTable();


  void InsertFencePlatform(ID3D11DeviceContext* pContext, ID3D11Query* pFence);

  bool IsFenceReachedPlatform(ID3D11DeviceContext* pContext, ID3D11Query* pFence);

  void WaitForFencePlatform(ID3D11DeviceContext* pContext, ID3D11Query* pFence);

  ID3D11Device* m_pDevice;
  ID3D11Device3* m_pDevice3;
  ID3D11DeviceContext* m_pImmediateContext;

  ID3D11Debug* m_pDebug;

  IDXGIFactory1* m_pDXGIFactory;

  IDXGIAdapter1* m_pDXGIAdapter;

  IDXGIDevice1* m_pDXGIDevice;

  wdGALFormatLookupTableDX11 m_FormatLookupTable;

  wdUInt32 m_uiFeatureLevel; // D3D_FEATURE_LEVEL can't be forward declared

  wdUniquePtr<wdGALPassDX11> m_pDefaultPass;

  struct PerFrameData
  {
    ID3D11Query* m_pFence = nullptr;
    ID3D11Query* m_pDisjointTimerQuery = nullptr;
    double m_fInvTicksPerSecond = -1.0;
    wdUInt64 m_uiFrame = -1;
  };

  PerFrameData m_PerFrameData[4];
  wdUInt8 m_uiCurrentPerFrameData = 0;
  wdUInt8 m_uiNextPerFrameData = 0;

  wdUInt64 m_uiFrameCounter = 0;

  struct UsedTempResource
  {
    WD_DECLARE_POD_TYPE();

    ID3D11Resource* m_pResource;
    wdUInt64 m_uiFrame;
    wdUInt32 m_uiHash;
  };

  wdMap<wdUInt32, wdDynamicArray<ID3D11Resource*>, wdCompareHelper<wdUInt32>, wdLocalAllocatorWrapper> m_FreeTempResources[TempResourceType::ENUM_COUNT];
  wdDeque<UsedTempResource, wdLocalAllocatorWrapper> m_UsedTempResources[TempResourceType::ENUM_COUNT];

  wdDynamicArray<ID3D11Query*, wdLocalAllocatorWrapper> m_Timestamps;
  wdUInt32 m_uiCurrentTimestamp = 0;
  wdUInt32 m_uiNextTimestamp = 0;

  struct GPUTimingScope* m_pFrameTimingScope = nullptr;
  struct GPUTimingScope* m_pPipelineTimingScope = nullptr;
  struct GPUTimingScope* m_pPassTimingScope = nullptr;

  wdTime m_SyncTimeDiff;
  bool m_bSyncTimeNeeded = true;
};

#include <RendererDX11/Device/Implementation/DeviceDX11_inl.h>
