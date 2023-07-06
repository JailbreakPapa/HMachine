#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>

#include <d3d11.h>

bool IsArrayView(const wdGALTextureCreationDescription& texDesc, const wdGALRenderTargetViewCreationDescription& viewDesc)
{
  return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstSlice > 0;
}

wdGALRenderTargetViewDX11::wdGALRenderTargetViewDX11(wdGALTexture* pTexture, const wdGALRenderTargetViewCreationDescription& Description)
  : wdGALRenderTargetView(pTexture, Description)
  , m_pRenderTargetView(nullptr)
  , m_pDepthStencilView(nullptr)
  , m_pUnorderedAccessView(nullptr)
{
}

wdGALRenderTargetViewDX11::~wdGALRenderTargetViewDX11() {}

wdResult wdGALRenderTargetViewDX11::InitPlatform(wdGALDevice* pDevice)
{
  const wdGALTexture* pTexture = nullptr;
  if (!m_Description.m_hTexture.IsInvalidated())
    pTexture = pDevice->GetTexture(m_Description.m_hTexture);

  if (pTexture == nullptr)
  {
    wdLog::Error("No valid texture handle given for rendertarget view creation!");
    return WD_FAILURE;
  }

  const wdGALTextureCreationDescription& texDesc = pTexture->GetDescription();
  wdGALResourceFormat::Enum viewFormat = texDesc.m_Format;

  if (m_Description.m_OverrideViewFormat != wdGALResourceFormat::Invalid)
    viewFormat = m_Description.m_OverrideViewFormat;


  wdGALDeviceDX11* pDXDevice = static_cast<wdGALDeviceDX11*>(pDevice);

  DXGI_FORMAT DXViewFormat = DXGI_FORMAT_UNKNOWN;

  const bool bIsDepthFormat = wdGALResourceFormat::IsDepthFormat(viewFormat);
  if (bIsDepthFormat)
  {
    DXViewFormat = pDXDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eDepthStencilType;
  }
  else
  {
    DXViewFormat = pDXDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eRenderTarget;
  }

  if (DXViewFormat == DXGI_FORMAT_UNKNOWN)
  {
    wdLog::Error("Couldn't get DXGI format for view!");
    return WD_FAILURE;
  }

  ID3D11Resource* pDXResource = static_cast<const wdGALTextureDX11*>(pTexture->GetParentResource())->GetDXTexture();
  const bool bIsArrayView = IsArrayView(texDesc, m_Description);

  if (bIsDepthFormat)
  {
    D3D11_DEPTH_STENCIL_VIEW_DESC DSViewDesc;
    DSViewDesc.Format = DXViewFormat;

    if (texDesc.m_SampleCount == wdGALMSAASampleCount::None)
    {
      if (!bIsArrayView)
      {
        DSViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        DSViewDesc.Texture2D.MipSlice = m_Description.m_uiMipLevel;
      }
      else
      {
        DSViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        DSViewDesc.Texture2DArray.MipSlice = m_Description.m_uiMipLevel;
        DSViewDesc.Texture2DArray.FirstArraySlice = m_Description.m_uiFirstSlice;
        DSViewDesc.Texture2DArray.ArraySize = m_Description.m_uiSliceCount;
      }
    }
    else
    {
      if (!bIsArrayView)
      {
        DSViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
        // DSViewDesc.Texture2DMS.UnusedField_NothingToDefine;
      }
      else
      {
        DSViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY;
        DSViewDesc.Texture2DMSArray.FirstArraySlice = m_Description.m_uiFirstSlice;
        DSViewDesc.Texture2DMSArray.ArraySize = m_Description.m_uiSliceCount;
      }
    }

    DSViewDesc.Flags = 0;
    if (m_Description.m_bReadOnly)
      DSViewDesc.Flags |= (D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL);

    if (FAILED(pDXDevice->GetDXDevice()->CreateDepthStencilView(pDXResource, &DSViewDesc, &m_pDepthStencilView)))
    {
      wdLog::Error("Couldn't create depth stencil view!");
      return WD_FAILURE;
    }
    else
    {
      return WD_SUCCESS;
    }
  }
  else
  {
    D3D11_RENDER_TARGET_VIEW_DESC RTViewDesc;
    RTViewDesc.Format = DXViewFormat;

    if (texDesc.m_SampleCount == wdGALMSAASampleCount::None)
    {
      if (!bIsArrayView)
      {
        RTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        RTViewDesc.Texture2D.MipSlice = m_Description.m_uiMipLevel;
      }
      else
      {
        RTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        RTViewDesc.Texture2DArray.MipSlice = m_Description.m_uiMipLevel;
        RTViewDesc.Texture2DArray.FirstArraySlice = m_Description.m_uiFirstSlice;
        RTViewDesc.Texture2DArray.ArraySize = m_Description.m_uiSliceCount;
      }
    }
    else
    {
      if (!bIsArrayView)
      {
        RTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
        // RTViewDesc.Texture2DMS.UnusedField_NothingToDefine;
      }
      else
      {
        RTViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
        RTViewDesc.Texture2DMSArray.FirstArraySlice = m_Description.m_uiFirstSlice;
        RTViewDesc.Texture2DMSArray.ArraySize = m_Description.m_uiSliceCount;
      }
    }

    if (FAILED(pDXDevice->GetDXDevice()->CreateRenderTargetView(pDXResource, &RTViewDesc, &m_pRenderTargetView)))
    {
      wdLog::Error("Couldn't create rendertarget view!");
      return WD_FAILURE;
    }
    else
    {
      return WD_SUCCESS;
    }
  }
}

wdResult wdGALRenderTargetViewDX11::DeInitPlatform(wdGALDevice* pDevice)
{
  WD_GAL_DX11_RELEASE(m_pRenderTargetView);
  WD_GAL_DX11_RELEASE(m_pDepthStencilView);
  WD_GAL_DX11_RELEASE(m_pUnorderedAccessView);

  return WD_SUCCESS;
}



WD_STATICLINK_FILE(RendererDX11, RendererDX11_Resources_Implementation_RenderTargetViewDX11);
