#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/QueryDX11.h>

#include <d3d11.h>

wdGALQueryDX11::wdGALQueryDX11(const wdGALQueryCreationDescription& Description)
  : wdGALQuery(Description)
  , m_pDXQuery(nullptr)
{
}

wdGALQueryDX11::~wdGALQueryDX11() {}

wdResult wdGALQueryDX11::InitPlatform(wdGALDevice* pDevice)
{
  wdGALDeviceDX11* pDXDevice = static_cast<wdGALDeviceDX11*>(pDevice);

  D3D11_QUERY_DESC desc;
  if (m_Description.m_type == wdGALQueryType::AnySamplesPassed)
    desc.MiscFlags = m_Description.m_bDrawIfUnknown ? D3D11_QUERY_MISC_PREDICATEHINT : 0;
  else
    desc.MiscFlags = 0;

  switch (m_Description.m_type)
  {
    case wdGALQueryType::NumSamplesPassed:
      desc.Query = D3D11_QUERY_OCCLUSION;
      break;
    case wdGALQueryType::AnySamplesPassed:
      desc.Query = D3D11_QUERY_OCCLUSION_PREDICATE;
      break;
    default:
      WD_ASSERT_NOT_IMPLEMENTED;
  }

  if (SUCCEEDED(pDXDevice->GetDXDevice()->CreateQuery(&desc, &m_pDXQuery)))
  {
    return WD_SUCCESS;
  }
  else
  {
    wdLog::Error("Creation of native DirectX query failed!");
    return WD_FAILURE;
  }
}

wdResult wdGALQueryDX11::DeInitPlatform(wdGALDevice* pDevice)
{
  WD_GAL_DX11_RELEASE(m_pDXQuery);
  return WD_SUCCESS;
}

void wdGALQueryDX11::SetDebugNamePlatform(const char* szName) const
{
  wdUInt32 uiLength = wdStringUtils::GetStringElementCount(szName);

  if (m_pDXQuery != nullptr)
  {
    m_pDXQuery->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }
}

WD_STATICLINK_FILE(RendererDX11, RendererDX11_Resources_Implementation_QueryDX11);
