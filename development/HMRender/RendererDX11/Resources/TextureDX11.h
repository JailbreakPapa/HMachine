#pragma once

#include <RendererFoundation/Resources/Texture.h>


struct ID3D11Resource;

class wdGALTextureDX11 : public wdGALTexture
{
public:
  WD_ALWAYS_INLINE ID3D11Resource* GetDXTexture() const;

  WD_ALWAYS_INLINE ID3D11Resource* GetDXStagingTexture() const;

protected:
  friend class wdGALDeviceDX11;
  friend class wdMemoryUtils;

  wdGALTextureDX11(const wdGALTextureCreationDescription& Description);

  ~wdGALTextureDX11();

  virtual wdResult InitPlatform(wdGALDevice* pDevice, wdArrayPtr<wdGALSystemMemoryDescription> pInitialData) override;
  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  wdResult CreateStagingTexture(wdGALDeviceDX11* pDevice);

  ID3D11Resource* m_pDXTexture;

  ID3D11Resource* m_pDXStagingTexture;

  void* m_pExisitingNativeObject = nullptr;
};

#include <RendererDX11/Resources/Implementation/TextureDX11_inl.h>
