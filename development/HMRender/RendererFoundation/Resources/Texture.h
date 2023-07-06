
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class WD_RENDERERFOUNDATION_DLL wdGALTexture : public wdGALResource<wdGALTextureCreationDescription>
{
public:
protected:
  friend class wdGALDevice;

  wdGALTexture(const wdGALTextureCreationDescription& Description);

  virtual ~wdGALTexture();

  virtual wdResult InitPlatform(wdGALDevice* pDevice, wdArrayPtr<wdGALSystemMemoryDescription> pInitialData) = 0;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) = 0;
};
