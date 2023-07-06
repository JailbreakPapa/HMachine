#pragma once

#include <RendererFoundation/Resources/Query.h>

struct ID3D11Query;

class wdGALQueryDX11 : public wdGALQuery
{
public:
  WD_ALWAYS_INLINE ID3D11Query* GetDXQuery() const;

protected:
  friend class wdGALDeviceDX11;
  friend class wdMemoryUtils;

  wdGALQueryDX11(const wdGALQueryCreationDescription& Description);
  ~wdGALQueryDX11();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) override;
  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  ID3D11Query* m_pDXQuery;
};

#include <RendererDX11/Resources/Implementation/QueryDX11_inl.h>
