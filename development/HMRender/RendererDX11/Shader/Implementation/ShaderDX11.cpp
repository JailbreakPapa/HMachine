#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>

#include <d3d11.h>

wdGALShaderDX11::wdGALShaderDX11(const wdGALShaderCreationDescription& Description)
  : wdGALShader(Description)
  , m_pVertexShader(nullptr)
  , m_pHullShader(nullptr)
  , m_pDomainShader(nullptr)
  , m_pGeometryShader(nullptr)
  , m_pPixelShader(nullptr)
  , m_pComputeShader(nullptr)
{
}

wdGALShaderDX11::~wdGALShaderDX11() {}

void wdGALShaderDX11::SetDebugName(const char* szName) const
{
  wdUInt32 uiLength = wdStringUtils::GetStringElementCount(szName);

  if (m_pVertexShader != nullptr)
  {
    m_pVertexShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }

  if (m_pHullShader != nullptr)
  {
    m_pHullShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }

  if (m_pDomainShader != nullptr)
  {
    m_pDomainShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }

  if (m_pGeometryShader != nullptr)
  {
    m_pGeometryShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }

  if (m_pPixelShader != nullptr)
  {
    m_pPixelShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }

  if (m_pComputeShader != nullptr)
  {
    m_pComputeShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }
}

wdResult wdGALShaderDX11::InitPlatform(wdGALDevice* pDevice)
{
  wdGALDeviceDX11* pDXDevice = static_cast<wdGALDeviceDX11*>(pDevice);
  ID3D11Device* pD3D11Device = pDXDevice->GetDXDevice();

  if (m_Description.HasByteCodeForStage(wdGALShaderStage::VertexShader))
  {
    if (FAILED(pD3D11Device->CreateVertexShader(m_Description.m_ByteCodes[wdGALShaderStage::VertexShader]->GetByteCode(),
          m_Description.m_ByteCodes[wdGALShaderStage::VertexShader]->GetSize(), nullptr, &m_pVertexShader)))
    {
      wdLog::Error("Couldn't create native vertex shader from bytecode!");
      return WD_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(wdGALShaderStage::HullShader))
  {
    if (FAILED(pD3D11Device->CreateHullShader(m_Description.m_ByteCodes[wdGALShaderStage::HullShader]->GetByteCode(),
          m_Description.m_ByteCodes[wdGALShaderStage::HullShader]->GetSize(), nullptr, &m_pHullShader)))
    {
      wdLog::Error("Couldn't create native hull shader from bytecode!");
      return WD_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(wdGALShaderStage::DomainShader))
  {
    if (FAILED(pD3D11Device->CreateDomainShader(m_Description.m_ByteCodes[wdGALShaderStage::DomainShader]->GetByteCode(),
          m_Description.m_ByteCodes[wdGALShaderStage::DomainShader]->GetSize(), nullptr, &m_pDomainShader)))
    {
      wdLog::Error("Couldn't create native domain shader from bytecode!");
      return WD_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(wdGALShaderStage::GeometryShader))
  {
    if (FAILED(pD3D11Device->CreateGeometryShader(m_Description.m_ByteCodes[wdGALShaderStage::GeometryShader]->GetByteCode(),
          m_Description.m_ByteCodes[wdGALShaderStage::GeometryShader]->GetSize(), nullptr, &m_pGeometryShader)))
    {
      wdLog::Error("Couldn't create native geometry shader from bytecode!");
      return WD_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(wdGALShaderStage::PixelShader))
  {
    if (FAILED(pD3D11Device->CreatePixelShader(m_Description.m_ByteCodes[wdGALShaderStage::PixelShader]->GetByteCode(),
          m_Description.m_ByteCodes[wdGALShaderStage::PixelShader]->GetSize(), nullptr, &m_pPixelShader)))
    {
      wdLog::Error("Couldn't create native pixel shader from bytecode!");
      return WD_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(wdGALShaderStage::ComputeShader))
  {
    if (FAILED(pD3D11Device->CreateComputeShader(m_Description.m_ByteCodes[wdGALShaderStage::ComputeShader]->GetByteCode(),
          m_Description.m_ByteCodes[wdGALShaderStage::ComputeShader]->GetSize(), nullptr, &m_pComputeShader)))
    {
      wdLog::Error("Couldn't create native compute shader from bytecode!");
      return WD_FAILURE;
    }
  }


  return WD_SUCCESS;
}

wdResult wdGALShaderDX11::DeInitPlatform(wdGALDevice* pDevice)
{
  WD_GAL_DX11_RELEASE(m_pVertexShader);
  WD_GAL_DX11_RELEASE(m_pHullShader);
  WD_GAL_DX11_RELEASE(m_pDomainShader);
  WD_GAL_DX11_RELEASE(m_pGeometryShader);
  WD_GAL_DX11_RELEASE(m_pPixelShader);
  WD_GAL_DX11_RELEASE(m_pComputeShader);

  return WD_SUCCESS;
}



WD_STATICLINK_FILE(RendererDX11, RendererDX11_Shader_Implementation_ShaderDX11);
