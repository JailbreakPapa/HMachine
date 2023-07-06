
#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <Foundation/Math/Size.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

class WD_RENDERERFOUNDATION_DLL wdGALSwapChain : public wdGALObject<wdGALSwapChainCreationDescription>
{
public:
  const wdGALRenderTargets& GetRenderTargets() const { return m_RenderTargets; }
  wdGALTextureHandle GetBackBufferTexture() const { return m_RenderTargets.m_hRTs[0]; }
  wdSizeU32 GetCurrentSize() const { return m_CurrentSize; }

  virtual void AcquireNextRenderTarget(wdGALDevice* pDevice) = 0;
  virtual void PresentRenderTarget(wdGALDevice* pDevice) = 0;
  virtual wdResult UpdateSwapChain(wdGALDevice* pDevice, wdEnum<wdGALPresentMode> newPresentMode) = 0;

  virtual ~wdGALSwapChain();

protected:
  friend class wdGALDevice;

  wdGALSwapChain(const wdRTTI* pSwapChainType);

  virtual wdResult InitPlatform(wdGALDevice* pDevice) = 0;
  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) = 0;

  wdGALRenderTargets m_RenderTargets;
  wdSizeU32 m_CurrentSize = {};
};
WD_DECLARE_REFLECTABLE_TYPE(WD_RENDERERFOUNDATION_DLL, wdGALSwapChain);


class WD_RENDERERFOUNDATION_DLL wdGALWindowSwapChain : public wdGALSwapChain
{
public:
  using Functor = wdDelegate<wdGALSwapChainHandle(const wdGALWindowSwapChainCreationDescription&)>;
  static void SetFactoryMethod(Functor factory);

  static wdGALSwapChainHandle Create(const wdGALWindowSwapChainCreationDescription& desc);

public:
  const wdGALWindowSwapChainCreationDescription& GetWindowDescription() const { return m_WindowDesc; }

protected:
  wdGALWindowSwapChain(const wdGALWindowSwapChainCreationDescription& Description);

protected:
  static Functor s_Factory;

protected:
  wdGALWindowSwapChainCreationDescription m_WindowDesc;
};
WD_DECLARE_REFLECTABLE_TYPE(WD_RENDERERFOUNDATION_DLL, wdGALWindowSwapChain);

#include <RendererFoundation/Device/Implementation/SwapChain_inl.h>
