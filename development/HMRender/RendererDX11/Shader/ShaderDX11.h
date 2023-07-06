
#pragma once

#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/Shader.h>

struct ID3D11VertexShader;
struct ID3D11HullShader;
struct ID3D11DomainShader;
struct ID3D11GeometryShader;
struct ID3D11PixelShader;
struct ID3D11ComputeShader;

class WD_RENDERERDX11_DLL wdGALShaderDX11 : public wdGALShader
{
public:
  void SetDebugName(const char* szName) const override;

  WD_ALWAYS_INLINE ID3D11VertexShader* GetDXVertexShader() const;

  WD_ALWAYS_INLINE ID3D11HullShader* GetDXHullShader() const;

  WD_ALWAYS_INLINE ID3D11DomainShader* GetDXDomainShader() const;

  WD_ALWAYS_INLINE ID3D11GeometryShader* GetDXGeometryShader() const;

  WD_ALWAYS_INLINE ID3D11PixelShader* GetDXPixelShader() const;

  WD_ALWAYS_INLINE ID3D11ComputeShader* GetDXComputeShader() const;


protected:
  friend class wdGALDeviceDX11;
  friend class wdMemoryUtils;

  wdGALShaderDX11(const wdGALShaderCreationDescription& description);

  virtual ~wdGALShaderDX11();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) override;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

  ID3D11VertexShader* m_pVertexShader;
  ID3D11HullShader* m_pHullShader;
  ID3D11DomainShader* m_pDomainShader;
  ID3D11GeometryShader* m_pGeometryShader;
  ID3D11PixelShader* m_pPixelShader;
  ID3D11ComputeShader* m_pComputeShader;
};

#include <RendererDX11/Shader/Implementation/ShaderDX11_inl.h>
