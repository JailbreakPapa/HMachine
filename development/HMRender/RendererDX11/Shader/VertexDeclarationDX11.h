
#pragma once

#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/VertexDeclaration.h>

struct ID3D11InputLayout;

class wdGALVertexDeclarationDX11 : public wdGALVertexDeclaration
{
public:
  WD_ALWAYS_INLINE ID3D11InputLayout* GetDXInputLayout() const;

protected:
  friend class wdGALDeviceDX11;
  friend class wdMemoryUtils;

  virtual wdResult InitPlatform(wdGALDevice* pDevice) override;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

  wdGALVertexDeclarationDX11(const wdGALVertexDeclarationCreationDescription& Description);

  virtual ~wdGALVertexDeclarationDX11();

  ID3D11InputLayout* m_pDXInputLayout;
};

#include <RendererDX11/Shader/Implementation/VertexDeclarationDX11_inl.h>
