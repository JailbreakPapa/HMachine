#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>

#include <d3d11.h>

wdGALTextureDX11::wdGALTextureDX11(const wdGALTextureCreationDescription& Description)
  : wdGALTexture(Description)
  , m_pDXTexture(nullptr)
  , m_pDXStagingTexture(nullptr)
  , m_pExisitingNativeObject(Description.m_pExisitingNativeObject)
{
}

wdGALTextureDX11::~wdGALTextureDX11() {}

WD_DEFINE_AS_POD_TYPE(D3D11_SUBRESOURCE_DATA);

wdResult wdGALTextureDX11::InitPlatform(wdGALDevice* pDevice, wdArrayPtr<wdGALSystemMemoryDescription> pInitialData)
{
  wdGALDeviceDX11* pDXDevice = static_cast<wdGALDeviceDX11*>(pDevice);

  if (m_pExisitingNativeObject != nullptr)
  {
    /// \todo Validation if interface of corresponding texture object exists
    m_pDXTexture = static_cast<ID3D11Resource*>(m_pExisitingNativeObject);
    if (!m_Description.m_ResourceAccess.IsImmutable() || m_Description.m_ResourceAccess.m_bReadBack)
    {
      wdResult res = CreateStagingTexture(pDXDevice);
      if (res == WD_FAILURE)
      {
        m_pDXTexture = nullptr;
        return res;
      }
    }
    return WD_SUCCESS;
  }


  switch (m_Description.m_Type)
  {
    case wdGALTextureType::Texture2D:
    case wdGALTextureType::TextureCube:
    {
      D3D11_TEXTURE2D_DESC Tex2DDesc;
      Tex2DDesc.ArraySize = (m_Description.m_Type == wdGALTextureType::Texture2D ? m_Description.m_uiArraySize : (m_Description.m_uiArraySize * 6));
      Tex2DDesc.BindFlags = 0;

      if (m_Description.m_bAllowShaderResourceView || m_Description.m_bAllowDynamicMipGeneration)
        Tex2DDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
      if (m_Description.m_bAllowUAV)
        Tex2DDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

      if (m_Description.m_bCreateRenderTarget || m_Description.m_bAllowDynamicMipGeneration)
        Tex2DDesc.BindFlags |= wdGALResourceFormat::IsDepthFormat(m_Description.m_Format) ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET;

      Tex2DDesc.CPUAccessFlags = 0; // We always use staging textures to update the data
      Tex2DDesc.Usage = m_Description.m_ResourceAccess.IsImmutable() ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;

      if (m_Description.m_bCreateRenderTarget || m_Description.m_bAllowUAV)
        Tex2DDesc.Usage = D3D11_USAGE_DEFAULT;

      Tex2DDesc.Format = pDXDevice->GetFormatLookupTable().GetFormatInfo(m_Description.m_Format).m_eStorage;

      if (Tex2DDesc.Format == DXGI_FORMAT_UNKNOWN)
      {
        wdLog::Error("No storage format available for given format: {0}", m_Description.m_Format);
        return WD_FAILURE;
      }

      Tex2DDesc.Width = m_Description.m_uiWidth;
      Tex2DDesc.Height = m_Description.m_uiHeight;
      Tex2DDesc.MipLevels = m_Description.m_uiMipLevelCount;

      Tex2DDesc.MiscFlags = 0;

      if (m_Description.m_bAllowDynamicMipGeneration)
        Tex2DDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

      if (m_Description.m_Type == wdGALTextureType::TextureCube)
        Tex2DDesc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;

      Tex2DDesc.SampleDesc.Count = m_Description.m_SampleCount;
      Tex2DDesc.SampleDesc.Quality = 0;

      wdHybridArray<D3D11_SUBRESOURCE_DATA, 16> InitialData;
      if (!pInitialData.IsEmpty())
      {
        wdUInt32 uiInitialDataCount = (m_Description.m_uiMipLevelCount * Tex2DDesc.ArraySize);
        WD_ASSERT_DEV(pInitialData.GetCount() == uiInitialDataCount, "The array of initial data values is not equal to the amount of mip levels!");

        InitialData.SetCountUninitialized(uiInitialDataCount);

        for (wdUInt32 i = 0; i < uiInitialDataCount; i++)
        {
          InitialData[i].pSysMem = pInitialData[i].m_pData;
          InitialData[i].SysMemPitch = pInitialData[i].m_uiRowPitch;
          InitialData[i].SysMemSlicePitch = pInitialData[i].m_uiSlicePitch;
        }
      }

      if (FAILED(pDXDevice->GetDXDevice()->CreateTexture2D(
            &Tex2DDesc, pInitialData.IsEmpty() ? nullptr : &InitialData[0], reinterpret_cast<ID3D11Texture2D**>(&m_pDXTexture))))
      {
        return WD_FAILURE;
      }
      else
      {
        if (!m_Description.m_ResourceAccess.IsImmutable() || m_Description.m_ResourceAccess.m_bReadBack)
          return CreateStagingTexture(pDXDevice);

        return WD_SUCCESS;
      }
    }
    break;

    case wdGALTextureType::Texture3D:
    {
      D3D11_TEXTURE3D_DESC Tex3DDesc;
      Tex3DDesc.BindFlags = 0;

      if (m_Description.m_bAllowShaderResourceView)
        Tex3DDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
      if (m_Description.m_bAllowUAV)
        Tex3DDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;

      if (m_Description.m_bCreateRenderTarget)
        Tex3DDesc.BindFlags |= wdGALResourceFormat::IsDepthFormat(m_Description.m_Format) ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET;

      Tex3DDesc.CPUAccessFlags = 0; // We always use staging textures to update the data
      Tex3DDesc.Usage = m_Description.m_ResourceAccess.IsImmutable() ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;

      if (m_Description.m_bCreateRenderTarget || m_Description.m_bAllowUAV)
        Tex3DDesc.Usage = D3D11_USAGE_DEFAULT;

      Tex3DDesc.Format = pDXDevice->GetFormatLookupTable().GetFormatInfo(m_Description.m_Format).m_eStorage;

      if (Tex3DDesc.Format == DXGI_FORMAT_UNKNOWN)
      {
        wdLog::Error("No storage format available for given format: {0}", m_Description.m_Format);
        return WD_FAILURE;
      }

      Tex3DDesc.Width = m_Description.m_uiWidth;
      Tex3DDesc.Height = m_Description.m_uiHeight;
      Tex3DDesc.Depth = m_Description.m_uiDepth;
      Tex3DDesc.MipLevels = m_Description.m_uiMipLevelCount;

      Tex3DDesc.MiscFlags = 0;

      if (m_Description.m_bAllowDynamicMipGeneration)
        Tex3DDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;

      if (m_Description.m_Type == wdGALTextureType::TextureCube)
        Tex3DDesc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;

      wdHybridArray<D3D11_SUBRESOURCE_DATA, 16> InitialData;
      if (!pInitialData.IsEmpty())
      {
        const wdUInt32 uiInitialDataCount = m_Description.m_uiMipLevelCount;
        WD_ASSERT_DEV(pInitialData.GetCount() == uiInitialDataCount, "The array of initial data values is not equal to the amount of mip levels!");

        InitialData.SetCountUninitialized(uiInitialDataCount);

        for (wdUInt32 i = 0; i < uiInitialDataCount; i++)
        {
          InitialData[i].pSysMem = pInitialData[i].m_pData;
          InitialData[i].SysMemPitch = pInitialData[i].m_uiRowPitch;
          InitialData[i].SysMemSlicePitch = pInitialData[i].m_uiSlicePitch;
        }
      }

      if (FAILED(pDXDevice->GetDXDevice()->CreateTexture3D(
            &Tex3DDesc, pInitialData.IsEmpty() ? nullptr : &InitialData[0], reinterpret_cast<ID3D11Texture3D**>(&m_pDXTexture))))
      {
        return WD_FAILURE;
      }
      else
      {
        if (!m_Description.m_ResourceAccess.IsImmutable() || m_Description.m_ResourceAccess.m_bReadBack)
          return CreateStagingTexture(pDXDevice);

        return WD_SUCCESS;
      }
    }
    break;

    default:
      WD_ASSERT_NOT_IMPLEMENTED;
      return WD_FAILURE;
  }


  return WD_FAILURE;
}

wdResult wdGALTextureDX11::DeInitPlatform(wdGALDevice* pDevice)
{
  WD_GAL_DX11_RELEASE(m_pDXTexture);
  WD_GAL_DX11_RELEASE(m_pDXStagingTexture);
  return WD_SUCCESS;
}

void wdGALTextureDX11::SetDebugNamePlatform(const char* szName) const
{
  wdUInt32 uiLength = wdStringUtils::GetStringElementCount(szName);

  if (m_pDXTexture != nullptr)
  {
    m_pDXTexture->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }
}

wdResult wdGALTextureDX11::CreateStagingTexture(wdGALDeviceDX11* pDevice)
{

  switch (m_Description.m_Type)
  {
    case wdGALTextureType::Texture2D:
    case wdGALTextureType::TextureCube:
    {
      D3D11_TEXTURE2D_DESC Desc;
      static_cast<ID3D11Texture2D*>(m_pDXTexture)->GetDesc(&Desc);
      Desc.BindFlags = 0;
      Desc.CPUAccessFlags = 0;
      // Need to remove this flag on the staging resource or texture readback no longer works.
      Desc.MiscFlags &= ~D3D11_RESOURCE_MISC_GENERATE_MIPS;
      Desc.Usage = D3D11_USAGE_STAGING;
      Desc.SampleDesc.Count = 1; // We need to disable MSAA for the readback texture, the conversion needs to happen during readback!

      if (m_Description.m_ResourceAccess.m_bReadBack)
        Desc.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
      if (!m_Description.m_ResourceAccess.IsImmutable())
        Desc.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;

      if (FAILED(pDevice->GetDXDevice()->CreateTexture2D(&Desc, nullptr, reinterpret_cast<ID3D11Texture2D**>(&m_pDXStagingTexture))))
      {
        wdLog::Error("Couldn't create staging resource for data upload and/or read back!");
        return WD_FAILURE;
      }
    }
    break;

    default:
      WD_ASSERT_NOT_IMPLEMENTED;
  }



  return WD_SUCCESS;
}



WD_STATICLINK_FILE(RendererDX11, RendererDX11_Resources_Implementation_TextureDX11);
