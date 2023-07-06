

WD_ALWAYS_INLINE ID3D11RenderTargetView* wdGALRenderTargetViewDX11::GetRenderTargetView() const
{
  return m_pRenderTargetView;
}

WD_ALWAYS_INLINE ID3D11DepthStencilView* wdGALRenderTargetViewDX11::GetDepthStencilView() const
{
  return m_pDepthStencilView;
}

WD_ALWAYS_INLINE ID3D11UnorderedAccessView* wdGALRenderTargetViewDX11::GetUnorderedAccessView() const
{
  return m_pUnorderedAccessView;
}