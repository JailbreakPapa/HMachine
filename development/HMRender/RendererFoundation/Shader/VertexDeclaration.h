
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class WD_RENDERERFOUNDATION_DLL wdGALVertexDeclaration : public wdGALObject<wdGALVertexDeclarationCreationDescription>
{
public:
protected:
  friend class wdGALDevice;

  virtual wdResult InitPlatform(wdGALDevice* pDevice) = 0;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) = 0;

  wdGALVertexDeclaration(const wdGALVertexDeclarationCreationDescription& Description);

  virtual ~wdGALVertexDeclaration();
};
