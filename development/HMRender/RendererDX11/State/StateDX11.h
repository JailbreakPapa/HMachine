
#pragma once

#include <RendererFoundation/State/State.h>


struct ID3D11BlendState;
struct ID3D11DepthStencilState;
struct ID3D11RasterizerState;
struct ID3D11RasterizerState2;
struct ID3D11SamplerState;

class WD_RENDERERDX11_DLL wdGALBlendStateDX11 : public wdGALBlendState
{
public:
  WD_ALWAYS_INLINE ID3D11BlendState* GetDXBlendState() const;

protected:
  friend class wdGALDeviceDX11;
  friend class wdMemoryUtils;

  wdGALBlendStateDX11(const wdGALBlendStateCreationDescription& Description);

  ~wdGALBlendStateDX11();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) override;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

  ID3D11BlendState* m_pDXBlendState;
};

class WD_RENDERERDX11_DLL wdGALDepthStencilStateDX11 : public wdGALDepthStencilState
{
public:
  WD_ALWAYS_INLINE ID3D11DepthStencilState* GetDXDepthStencilState() const;

protected:
  friend class wdGALDeviceDX11;
  friend class wdMemoryUtils;

  wdGALDepthStencilStateDX11(const wdGALDepthStencilStateCreationDescription& Description);

  ~wdGALDepthStencilStateDX11();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) override;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

  ID3D11DepthStencilState* m_pDXDepthStencilState;
};

class WD_RENDERERDX11_DLL wdGALRasterizerStateDX11 : public wdGALRasterizerState
{
public:
  WD_ALWAYS_INLINE ID3D11RasterizerState* GetDXRasterizerState() const;

protected:
  friend class wdGALDeviceDX11;
  friend class wdMemoryUtils;

  wdGALRasterizerStateDX11(const wdGALRasterizerStateCreationDescription& Description);

  ~wdGALRasterizerStateDX11();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) override;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

  ID3D11RasterizerState* m_pDXRasterizerState;
};

class WD_RENDERERDX11_DLL wdGALSamplerStateDX11 : public wdGALSamplerState
{
public:
  WD_ALWAYS_INLINE ID3D11SamplerState* GetDXSamplerState() const;

protected:
  friend class wdGALDeviceDX11;
  friend class wdMemoryUtils;

  wdGALSamplerStateDX11(const wdGALSamplerStateCreationDescription& Description);

  ~wdGALSamplerStateDX11();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) override;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

  ID3D11SamplerState* m_pDXSamplerState;
};


#include <RendererDX11/State/Implementation/StateDX11_inl.h>
