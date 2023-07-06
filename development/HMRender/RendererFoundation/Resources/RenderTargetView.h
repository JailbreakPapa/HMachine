
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class WD_RENDERERFOUNDATION_DLL wdGALRenderTargetView : public wdGALObject<wdGALRenderTargetViewCreationDescription>
{
public:
  WD_ALWAYS_INLINE wdGALTexture* GetTexture() const { return m_pTexture; }

protected:
  friend class wdGALDevice;

  wdGALRenderTargetView(wdGALTexture* pTexture, const wdGALRenderTargetViewCreationDescription& description);

  virtual ~wdGALRenderTargetView();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) = 0;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) = 0;

  wdGALTexture* m_pTexture;
};
