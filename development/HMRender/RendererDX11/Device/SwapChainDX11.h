
#pragma once

#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/SwapChain.h>

struct IDXGISwapChain;

class wdGALSwapChainDX11 : public wdGALWindowSwapChain
{
public:
  virtual void AcquireNextRenderTarget(wdGALDevice* pDevice) override;
  virtual void PresentRenderTarget(wdGALDevice* pDevice) override;
  virtual wdResult UpdateSwapChain(wdGALDevice* pDevice, wdEnum<wdGALPresentMode> newPresentMode) override;

protected:
  friend class wdGALDeviceDX11;
  friend class wdMemoryUtils;

  wdGALSwapChainDX11(const wdGALWindowSwapChainCreationDescription& Description);

  virtual ~wdGALSwapChainDX11();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) override;
  wdResult CreateBackBufferInternal(wdGALDeviceDX11* pDXDevice);
  void DestroyBackBufferInternal(wdGALDeviceDX11* pDXDevice);
  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;


  IDXGISwapChain* m_pDXSwapChain;

  wdGALTextureHandle m_hBackBufferTexture;

  wdEnum<wdGALPresentMode> m_CurrentPresentMode;
  bool m_bCanMakeDirectScreenshots = true;
  // We can't do screenshots if we're using any of the FLIP swap effects.
  // If the user requests screenshots anyways, we need to put another buffer in between.
  // For ease of use, this is m_hBackBufferTexture and the actual "OS backbuffer" is this texture.
  // In any other case this handle is unused.
  wdGALTextureHandle m_hActualBackBufferTexture;
};

#include <RendererDX11/Device/Implementation/SwapChainDX11_inl.h>
