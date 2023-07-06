
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class WD_RENDERERFOUNDATION_DLL wdGALUnorderedAccessView : public wdGALObject<wdGALUnorderedAccessViewCreationDescription>
{
public:
  WD_ALWAYS_INLINE wdGALResourceBase* GetResource() const { return m_pResource; }

protected:
  friend class wdGALDevice;

  wdGALUnorderedAccessView(wdGALResourceBase* pResource, const wdGALUnorderedAccessViewCreationDescription& description);

  virtual ~wdGALUnorderedAccessView();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) = 0;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) = 0;

  wdGALResourceBase* m_pResource;
};
