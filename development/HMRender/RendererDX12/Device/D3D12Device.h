/*
 *   Copyright (c) 2023 Watch Dogs LLC
 *   All rights reserved.
 *   You are only allowed access to this code, if given WRITTEN permission by Watch Dogs LLC.
 */

#pragma once

#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererDX12/RendererDX12PCH.h>
#include <RendererFoundation/Device/Device.h>

#include <dxgi.h>


struct ID3D12CommandAllocator3;
struct ID3D12CommandList3;
struct ID3D12CommandQueue3;
struct ID3D12Device3;
struct ID3D12Debug2;
struct IDXGIFactory3;
struct IDXGIAdapter3;
struct IDXGIDevice3;
struct ID3D12Resource2;
struct ID3D12Query;
struct IDXGIAdapter3;

using wdGALFormatLookupEntryDX12 = wdGALFormatLookupEntry<DXGI_FORMAT, (DXGI_FORMAT)0>;
using wdGALFormatLookupTableDX12 = wdGALFormatLookupTable<wdGALFormatLookupEntryDX12>;


class wdGALPassDX12;

/// \brief The DX12 device implementation of the graphics abstraction layer.

class WD_RENDERERDX12_DLL wdGALDeviceDX12 : public wdGALDevice
{
private:
  friend wdInternal::NewInstance<wdGALDevice> CreateDX12Device(wdAllocatorBase* pAllocator, const wdGALDeviceCreationDescription& desc);
  wdGALDeviceDX12(const wdGALDeviceCreationDescription& desc);

  public:
  virtual ~wdGALDeviceDX12();

  public:
  ID3D12Device* GetDXDevice() const;
  ID3D12Device3* GetDXDevice3() const;
  IDXGIFactory3* GetDXGIFactory() const;

  ID3D12CommandAllocator3* GetDirectCommandAllocator() const;
  ID3D12CommandAllocator3* GetComputeCommandAllocator() const;
  ID3D12CommandAllocator3* GetCopyCommandAllocator() const;

  ID3D12CommandQueue3* GetDirectCommandQueue() const;
  ID3D12CommandQueue3* GetComputeCommandQueue() const;
  ID3D12CommandQueue3* GetCopyCommandQueue() const; 

  wdGALRenderCommandEncoder* GetRenderCommandEncoder() const;
  

  void ReportLiveGpuObjects();

  void FlushDeadObjects();
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
};
