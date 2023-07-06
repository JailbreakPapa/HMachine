#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class WD_RENDERERFOUNDATION_DLL wdGALShader : public wdGALObject<wdGALShaderCreationDescription>
{
public:
  virtual void SetDebugName(const char* szName) const = 0;

protected:
  friend class wdGALDevice;

  virtual wdResult InitPlatform(wdGALDevice* pDevice) = 0;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) = 0;

  wdGALShader(const wdGALShaderCreationDescription& Description);

  virtual ~wdGALShader();
};
