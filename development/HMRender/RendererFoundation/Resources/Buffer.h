
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class WD_RENDERERFOUNDATION_DLL wdGALBuffer : public wdGALResource<wdGALBufferCreationDescription>
{
public:
  WD_ALWAYS_INLINE wdUInt32 GetSize() const;

protected:
  friend class wdGALDevice;

  wdGALBuffer(const wdGALBufferCreationDescription& Description);

  virtual ~wdGALBuffer();

  virtual wdResult InitPlatform(wdGALDevice* pDevice, wdArrayPtr<const wdUInt8> pInitialData) = 0;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) = 0;
};

#include <RendererFoundation/Resources/Implementation/Buffer_inl.h>
