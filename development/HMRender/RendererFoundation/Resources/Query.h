#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class WD_RENDERERFOUNDATION_DLL wdGALQuery : public wdGALResource<wdGALQueryCreationDescription>
{
public:
protected:
  friend class wdGALDevice;
  friend class wdGALCommandEncoder;

  wdGALQuery(const wdGALQueryCreationDescription& Description);

  virtual ~wdGALQuery();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) = 0;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) = 0;

  bool m_bStarted;
};
