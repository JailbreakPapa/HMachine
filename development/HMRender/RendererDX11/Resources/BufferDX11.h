
#pragma once

#include <RendererFoundation/Resources/Buffer.h>
#include <dxgi.h>

struct ID3D11Buffer;

class WD_RENDERERDX11_DLL wdGALBufferDX11 : public wdGALBuffer
{
public:
  ID3D11Buffer* GetDXBuffer() const;

  DXGI_FORMAT GetIndexFormat() const;

protected:
  friend class wdGALDeviceDX11;
  friend class wdMemoryUtils;

  wdGALBufferDX11(const wdGALBufferCreationDescription& Description);

  virtual ~wdGALBufferDX11();

  virtual wdResult InitPlatform(wdGALDevice* pDevice, wdArrayPtr<const wdUInt8> pInitialData) override;
  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  ID3D11Buffer* m_pDXBuffer;

  DXGI_FORMAT m_IndexFormat; // Only applicable for index buffers
};

#include <RendererDX11/Resources/Implementation/BufferDX11_inl.h>
