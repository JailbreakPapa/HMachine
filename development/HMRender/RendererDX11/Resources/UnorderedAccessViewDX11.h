
#pragma once

#include <RendererFoundation/Resources/UnorderedAccesView.h>

struct ID3D11UnorderedAccessView;

class wdGALUnorderedAccessViewDX11 : public wdGALUnorderedAccessView
{
public:
  WD_ALWAYS_INLINE ID3D11UnorderedAccessView* GetDXResourceView() const;

protected:
  friend class wdGALDeviceDX11;
  friend class wdMemoryUtils;

  wdGALUnorderedAccessViewDX11(wdGALResourceBase* pResource, const wdGALUnorderedAccessViewCreationDescription& Description);

  ~wdGALUnorderedAccessViewDX11();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) override;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

  ID3D11UnorderedAccessView* m_pDXUnorderedAccessView;
};

#include <RendererDX11/Resources/Implementation/UnorderedAccessViewDX11_inl.h>
