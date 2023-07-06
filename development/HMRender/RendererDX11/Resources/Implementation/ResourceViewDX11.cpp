#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/ResourceViewDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>

#include <d3d11.h>

bool IsArrayView(const wdGALTextureCreationDescription& texDesc, const wdGALResourceViewCreationDescription& viewDesc)
{
  return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstArraySlice > 0;
}

wdGALResourceViewDX11::wdGALResourceViewDX11(wdGALResourceBase* pResource, const wdGALResourceViewCreationDescription& Description)
  : wdGALResourceView(pResource, Description)
  , m_pDXResourceView(nullptr)
{
}

wdGALResourceViewDX11::~wdGALResourceViewDX11() {}

wdResult wdGALResourceViewDX11::InitPlatform(wdGALDevice* pDevice)
{
  const wdGALTexture* pTexture = nullptr;
  if (!m_Description.m_hTexture.IsInvalidated())
    pTexture = pDevice->GetTexture(m_Description.m_hTexture);

  const wdGALBuffer* pBuffer = nullptr;
  if (!m_Description.m_hBuffer.IsInvalidated())
    pBuffer = pDevice->GetBuffer(m_Description.m_hBuffer);

  if (pTexture == nullptr && pBuffer == nullptr)
  {
    wdLog::Error("No valid texture handle or buffer handle given for resource view creation!");
    return WD_FAILURE;
  }


  wdGALResourceFormat::Enum ViewFormat = m_Description.m_OverrideViewFormat;

  if (pTexture)
  {
    if (ViewFormat == wdGALResourceFormat::Invalid)
      ViewFormat = pTexture->GetDescription().m_Format;
  }
  else if (pBuffer)
  {
    if (ViewFormat == wdGALResourceFormat::Invalid)
      ViewFormat = wdGALResourceFormat::RUInt;

    if (!pBuffer->GetDescription().m_bAllowRawViews && m_Description.m_bRawView)
    {
      wdLog::Error("Trying to create a raw view for a buffer with no raw view flag is invalid!");
      return WD_FAILURE;
    }
  }

  wdGALDeviceDX11* pDXDevice = static_cast<wdGALDeviceDX11*>(pDevice);


  DXGI_FORMAT DXViewFormat = DXGI_FORMAT_UNKNOWN;
  if (wdGALResourceFormat::IsDepthFormat(ViewFormat))
  {
    DXViewFormat = pDXDevice->GetFormatLookupTable().GetFormatInfo(ViewFormat).m_eDepthOnlyType;
  }
  else
  {
    DXViewFormat = pDXDevice->GetFormatLookupTable().GetFormatInfo(ViewFormat).m_eResourceViewType;
  }

  if (DXViewFormat == DXGI_FORMAT_UNKNOWN)
  {
    wdLog::Error("Couldn't get valid DXGI format for resource view! ({0})", ViewFormat);
    return WD_FAILURE;
  }

  D3D11_SHADER_RESOURCE_VIEW_DESC DXSRVDesc;
  DXSRVDesc.Format = DXViewFormat;

  ID3D11Resource* pDXResource = nullptr;

  if (pTexture)
  {
    pDXResource = static_cast<const wdGALTextureDX11*>(pTexture->GetParentResource())->GetDXTexture();
    const wdGALTextureCreationDescription& texDesc = pTexture->GetDescription();

    const bool bIsArrayView = IsArrayView(texDesc, m_Description);

    switch (texDesc.m_Type)
    {
      case wdGALTextureType::Texture2D:
      case wdGALTextureType::Texture2DProxy:

        if (!bIsArrayView)
        {
          if (texDesc.m_SampleCount == wdGALMSAASampleCount::None)
          {
            DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            DXSRVDesc.Texture2D.MipLevels = m_Description.m_uiMipLevelsToUse;
            DXSRVDesc.Texture2D.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
          }
          else
          {
            DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
          }
        }
        else
        {
          if (texDesc.m_SampleCount == wdGALMSAASampleCount::None)
          {
            DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
            DXSRVDesc.Texture2DArray.MipLevels = m_Description.m_uiMipLevelsToUse;
            DXSRVDesc.Texture2DArray.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
            DXSRVDesc.Texture2DArray.ArraySize = m_Description.m_uiArraySize;
            DXSRVDesc.Texture2DArray.FirstArraySlice = m_Description.m_uiFirstArraySlice;
          }
          else
          {
            DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY;
            DXSRVDesc.Texture2DMSArray.ArraySize = m_Description.m_uiArraySize;
            DXSRVDesc.Texture2DMSArray.FirstArraySlice = m_Description.m_uiFirstArraySlice;
          }
        }

        break;

      case wdGALTextureType::TextureCube:

        if (!bIsArrayView)
        {
          DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
          DXSRVDesc.TextureCube.MipLevels = m_Description.m_uiMipLevelsToUse;
          DXSRVDesc.TextureCube.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
        }
        else
        {
          DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
          DXSRVDesc.TextureCube.MipLevels = m_Description.m_uiMipLevelsToUse;
          DXSRVDesc.TextureCube.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;
          DXSRVDesc.TextureCubeArray.NumCubes = m_Description.m_uiArraySize;
          DXSRVDesc.TextureCubeArray.First2DArrayFace = m_Description.m_uiFirstArraySlice;
        }

        break;

      case wdGALTextureType::Texture3D:

        DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
        DXSRVDesc.Texture3D.MipLevels = m_Description.m_uiMipLevelsToUse;
        DXSRVDesc.Texture3D.MostDetailedMip = m_Description.m_uiMostDetailedMipLevel;

        break;

      default:
        WD_ASSERT_NOT_IMPLEMENTED;
        return WD_FAILURE;
    }
  }
  else if (pBuffer)
  {
    pDXResource = static_cast<const wdGALBufferDX11*>(pBuffer)->GetDXBuffer();

    if (pBuffer->GetDescription().m_bUseAsStructuredBuffer)
      DXSRVDesc.Format = DXGI_FORMAT_UNKNOWN;

    DXSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    DXSRVDesc.BufferEx.FirstElement = DXSRVDesc.Buffer.FirstElement = m_Description.m_uiFirstElement;
    DXSRVDesc.BufferEx.NumElements = DXSRVDesc.Buffer.NumElements = m_Description.m_uiNumElements;
    DXSRVDesc.BufferEx.Flags = m_Description.m_bRawView ? D3D11_BUFFEREX_SRV_FLAG_RAW : 0;
  }

  if (FAILED(pDXDevice->GetDXDevice()->CreateShaderResourceView(pDXResource, &DXSRVDesc, &m_pDXResourceView)))
  {
    return WD_FAILURE;
  }
  else
  {
    return WD_SUCCESS;
  }
}

wdResult wdGALResourceViewDX11::DeInitPlatform(wdGALDevice* pDevice)
{
  WD_GAL_DX11_RELEASE(m_pDXResourceView);
  return WD_SUCCESS;
}



WD_STATICLINK_FILE(RendererDX11, RendererDX11_Resources_Implementation_ResourceViewDX11);
