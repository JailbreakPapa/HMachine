
#pragma once

#include <RendererFoundation/Resources/ResourceView.h>

struct ID3D11ShaderResourceView;

class wdGALResourceViewDX11 : public wdGALResourceView
{
public:
  WD_ALWAYS_INLINE ID3D11ShaderResourceView* GetDXResourceView() const;

protected:
  friend class wdGALDeviceDX11;
  friend class wdMemoryUtils;

  wdGALResourceViewDX11(wdGALResourceBase* pResource, const wdGALResourceViewCreationDescription& Description);

  ~wdGALResourceViewDX11();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) override;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

  ID3D11ShaderResourceView* m_pDXResourceView;
};

#include <RendererDX11/Resources/Implementation/ResourceViewDX11_inl.h>
