
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class WD_RENDERERFOUNDATION_DLL wdGALResourceView : public wdGALObject<wdGALResourceViewCreationDescription>
{
public:
  WD_ALWAYS_INLINE wdGALResourceBase* GetResource() const { return m_pResource; }

protected:
  friend class wdGALDevice;

  wdGALResourceView(wdGALResourceBase* pResource, const wdGALResourceViewCreationDescription& description);

  virtual ~wdGALResourceView();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) = 0;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) = 0;

  wdGALResourceBase* m_pResource;
};
