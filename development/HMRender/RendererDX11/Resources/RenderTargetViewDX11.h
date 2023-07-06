
#pragma once

#include <RendererFoundation/Resources/RenderTargetView.h>

struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11UnorderedAccessView;

class wdGALRenderTargetViewDX11 : public wdGALRenderTargetView
{
public:
  WD_ALWAYS_INLINE ID3D11RenderTargetView* GetRenderTargetView() const;

  WD_ALWAYS_INLINE ID3D11DepthStencilView* GetDepthStencilView() const;

  WD_ALWAYS_INLINE ID3D11UnorderedAccessView* GetUnorderedAccessView() const;

protected:
  friend class wdGALDeviceDX11;
  friend class wdMemoryUtils;

  wdGALRenderTargetViewDX11(wdGALTexture* pTexture, const wdGALRenderTargetViewCreationDescription& Description);

  virtual ~wdGALRenderTargetViewDX11();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) override;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

  ID3D11RenderTargetView* m_pRenderTargetView;

  ID3D11DepthStencilView* m_pDepthStencilView;

  ID3D11UnorderedAccessView* m_pUnorderedAccessView;
};

#include <RendererDX11/Resources/Implementation/RenderTargetViewDX11_inl.h>
