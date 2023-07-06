
#pragma once

#include <RendererFoundation/Resources/Texture.h>

class WD_RENDERERFOUNDATION_DLL wdGALProxyTexture : public wdGALTexture
{
public:
  virtual ~wdGALProxyTexture();

  virtual const wdGALResourceBase* GetParentResource() const override;

protected:
  friend class wdGALDevice;

  wdGALProxyTexture(const wdGALTexture& parentTexture);

  virtual wdResult InitPlatform(wdGALDevice* pDevice, wdArrayPtr<wdGALSystemMemoryDescription> pInitialData) override;
  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  const wdGALTexture* m_pParentTexture;
};
