#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/RendererDX11DLL.h>
#include <RendererDX11/State/StateDX11.h>

#include <d3d11.h>
#include <d3d11_3.h>


// Mapping tables to map wdGAL constants to DX11 constants
#include <RendererDX11/State/Implementation/StateDX11_MappingTables.inl>

// Blend state

wdGALBlendStateDX11::wdGALBlendStateDX11(const wdGALBlendStateCreationDescription& Description)
  : wdGALBlendState(Description)
  , m_pDXBlendState(nullptr)
{
}

wdGALBlendStateDX11::~wdGALBlendStateDX11() {}

static D3D11_BLEND_OP ToD3DBlendOp(wdGALBlendOp::Enum e)
{
  switch (e)
  {
    case wdGALBlendOp::Add:
      return D3D11_BLEND_OP_ADD;
    case wdGALBlendOp::Max:
      return D3D11_BLEND_OP_MAX;
    case wdGALBlendOp::Min:
      return D3D11_BLEND_OP_MIN;
    case wdGALBlendOp::RevSubtract:
      return D3D11_BLEND_OP_REV_SUBTRACT;
    case wdGALBlendOp::Subtract:
      return D3D11_BLEND_OP_SUBTRACT;
    default:
      WD_ASSERT_NOT_IMPLEMENTED;
  }

  return D3D11_BLEND_OP_ADD;
}

static D3D11_BLEND ToD3DBlend(wdGALBlend::Enum e)
{
  switch (e)
  {
    case wdGALBlend::BlendFactor:
      WD_ASSERT_NOT_IMPLEMENTED;
      // if this is used, it also must be implemented in wdGALContextDX11::SetBlendStatePlatform
      return D3D11_BLEND_BLEND_FACTOR;
    case wdGALBlend::DestAlpha:
      return D3D11_BLEND_DEST_ALPHA;
    case wdGALBlend::DestColor:
      return D3D11_BLEND_DEST_COLOR;
    case wdGALBlend::InvBlendFactor:
      WD_ASSERT_NOT_IMPLEMENTED;
      // if this is used, it also must be implemented in wdGALContextDX11::SetBlendStatePlatform
      return D3D11_BLEND_INV_BLEND_FACTOR;
    case wdGALBlend::InvDestAlpha:
      return D3D11_BLEND_INV_DEST_ALPHA;
    case wdGALBlend::InvDestColor:
      return D3D11_BLEND_INV_DEST_COLOR;
    case wdGALBlend::InvSrcAlpha:
      return D3D11_BLEND_INV_SRC_ALPHA;
    case wdGALBlend::InvSrcColor:
      return D3D11_BLEND_INV_SRC_COLOR;
    case wdGALBlend::One:
      return D3D11_BLEND_ONE;
    case wdGALBlend::SrcAlpha:
      return D3D11_BLEND_SRC_ALPHA;
    case wdGALBlend::SrcAlphaSaturated:
      return D3D11_BLEND_SRC_ALPHA_SAT;
    case wdGALBlend::SrcColor:
      return D3D11_BLEND_SRC_COLOR;
    case wdGALBlend::Zero:
      return D3D11_BLEND_ZERO;

    default:
      WD_ASSERT_NOT_IMPLEMENTED;
  }

  return D3D11_BLEND_ONE;
}

wdResult wdGALBlendStateDX11::InitPlatform(wdGALDevice* pDevice)
{
  D3D11_BLEND_DESC DXDesc;
  DXDesc.AlphaToCoverageEnable = m_Description.m_bAlphaToCoverage;
  DXDesc.IndependentBlendEnable = m_Description.m_bIndependentBlend;

  for (wdInt32 i = 0; i < 8; ++i)
  {
    DXDesc.RenderTarget[i].BlendEnable = m_Description.m_RenderTargetBlendDescriptions[i].m_bBlendingEnabled;
    DXDesc.RenderTarget[i].BlendOp = ToD3DBlendOp(m_Description.m_RenderTargetBlendDescriptions[i].m_BlendOp);
    DXDesc.RenderTarget[i].BlendOpAlpha = ToD3DBlendOp(m_Description.m_RenderTargetBlendDescriptions[i].m_BlendOpAlpha);
    DXDesc.RenderTarget[i].DestBlend = ToD3DBlend(m_Description.m_RenderTargetBlendDescriptions[i].m_DestBlend);
    DXDesc.RenderTarget[i].DestBlendAlpha = ToD3DBlend(m_Description.m_RenderTargetBlendDescriptions[i].m_DestBlendAlpha);
    DXDesc.RenderTarget[i].SrcBlend = ToD3DBlend(m_Description.m_RenderTargetBlendDescriptions[i].m_SourceBlend);
    DXDesc.RenderTarget[i].SrcBlendAlpha = ToD3DBlend(m_Description.m_RenderTargetBlendDescriptions[i].m_SourceBlendAlpha);
    DXDesc.RenderTarget[i].RenderTargetWriteMask = m_Description.m_RenderTargetBlendDescriptions[i].m_uiWriteMask &
                                                   0x0F; // D3D11: RenderTargetWriteMask can only have the least significant 4 bits set.
  }

  if (FAILED(static_cast<wdGALDeviceDX11*>(pDevice)->GetDXDevice()->CreateBlendState(&DXDesc, &m_pDXBlendState)))
  {
    return WD_FAILURE;
  }

  return WD_SUCCESS;
}

wdResult wdGALBlendStateDX11::DeInitPlatform(wdGALDevice* pDevice)
{
  WD_GAL_DX11_RELEASE(m_pDXBlendState);
  return WD_SUCCESS;
}

// Depth Stencil state

wdGALDepthStencilStateDX11::wdGALDepthStencilStateDX11(const wdGALDepthStencilStateCreationDescription& Description)
  : wdGALDepthStencilState(Description)
  , m_pDXDepthStencilState(nullptr)
{
}

wdGALDepthStencilStateDX11::~wdGALDepthStencilStateDX11() {}

wdResult wdGALDepthStencilStateDX11::InitPlatform(wdGALDevice* pDevice)
{
  D3D11_DEPTH_STENCIL_DESC DXDesc;
  DXDesc.DepthEnable = m_Description.m_bDepthTest;
  DXDesc.DepthWriteMask = m_Description.m_bDepthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
  DXDesc.DepthFunc = GALCompareFuncToDX11[m_Description.m_DepthTestFunc];
  DXDesc.StencilEnable = m_Description.m_bStencilTest;
  DXDesc.StencilReadMask = m_Description.m_uiStencilReadMask;
  DXDesc.StencilWriteMask = m_Description.m_uiStencilWriteMask;

  DXDesc.FrontFace.StencilFailOp = GALStencilOpTableIndexToDX11[m_Description.m_FrontFaceStencilOp.m_FailOp];
  DXDesc.FrontFace.StencilDepthFailOp = GALStencilOpTableIndexToDX11[m_Description.m_FrontFaceStencilOp.m_DepthFailOp];
  DXDesc.FrontFace.StencilPassOp = GALStencilOpTableIndexToDX11[m_Description.m_FrontFaceStencilOp.m_PassOp];
  DXDesc.FrontFace.StencilFunc = GALCompareFuncToDX11[m_Description.m_FrontFaceStencilOp.m_StencilFunc];

  const wdGALStencilOpDescription& backFaceStencilOp =
    m_Description.m_bSeparateFrontAndBack ? m_Description.m_BackFaceStencilOp : m_Description.m_FrontFaceStencilOp;
  DXDesc.BackFace.StencilFailOp = GALStencilOpTableIndexToDX11[backFaceStencilOp.m_FailOp];
  DXDesc.BackFace.StencilDepthFailOp = GALStencilOpTableIndexToDX11[backFaceStencilOp.m_DepthFailOp];
  DXDesc.BackFace.StencilPassOp = GALStencilOpTableIndexToDX11[backFaceStencilOp.m_PassOp];
  DXDesc.BackFace.StencilFunc = GALCompareFuncToDX11[backFaceStencilOp.m_StencilFunc];


  if (FAILED(static_cast<wdGALDeviceDX11*>(pDevice)->GetDXDevice()->CreateDepthStencilState(&DXDesc, &m_pDXDepthStencilState)))
  {
    return WD_FAILURE;
  }
  else
  {
    return WD_SUCCESS;
  }
}

wdResult wdGALDepthStencilStateDX11::DeInitPlatform(wdGALDevice* pDevice)
{
  WD_GAL_DX11_RELEASE(m_pDXDepthStencilState);
  return WD_SUCCESS;
}


// Rasterizer state

wdGALRasterizerStateDX11::wdGALRasterizerStateDX11(const wdGALRasterizerStateCreationDescription& Description)
  : wdGALRasterizerState(Description)
  , m_pDXRasterizerState(nullptr)
{
}

wdGALRasterizerStateDX11::~wdGALRasterizerStateDX11() {}



wdResult wdGALRasterizerStateDX11::InitPlatform(wdGALDevice* pDevice)
{
  const bool NeedsStateDesc2 = m_Description.m_bConservativeRasterization;

  if (NeedsStateDesc2)
  {
    D3D11_RASTERIZER_DESC2 DXDesc2;
    DXDesc2.CullMode = GALCullModeToDX11[m_Description.m_CullMode];
    DXDesc2.DepthBias = m_Description.m_iDepthBias;
    DXDesc2.DepthBiasClamp = m_Description.m_fDepthBiasClamp;
    DXDesc2.DepthClipEnable = TRUE;
    DXDesc2.FillMode = m_Description.m_bWireFrame ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
    DXDesc2.FrontCounterClockwise = m_Description.m_bFrontCounterClockwise;
    DXDesc2.MultisampleEnable = TRUE;
    DXDesc2.AntialiasedLineEnable = TRUE;
    DXDesc2.ScissorEnable = m_Description.m_bScissorTest;
    DXDesc2.SlopeScaledDepthBias = m_Description.m_fSlopeScaledDepthBias;
    DXDesc2.ConservativeRaster =
      m_Description.m_bConservativeRasterization ? D3D11_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    DXDesc2.ForcedSampleCount = 0;

    if (!pDevice->GetCapabilities().m_bConservativeRasterization && m_Description.m_bConservativeRasterization)
    {
      wdLog::Error("Rasterizer state description enables conservative rasterization which is not available!");
      return WD_FAILURE;
    }

    ID3D11RasterizerState2* pDXRasterizerState2 = nullptr;

    if (FAILED(static_cast<wdGALDeviceDX11*>(pDevice)->GetDXDevice3()->CreateRasterizerState2(&DXDesc2, &pDXRasterizerState2)))
    {
      return WD_FAILURE;
    }
    else
    {
      m_pDXRasterizerState = pDXRasterizerState2;
      return WD_SUCCESS;
    }
  }
  else
  {
    D3D11_RASTERIZER_DESC DXDesc;
    DXDesc.CullMode = GALCullModeToDX11[m_Description.m_CullMode];
    DXDesc.DepthBias = m_Description.m_iDepthBias;
    DXDesc.DepthBiasClamp = m_Description.m_fDepthBiasClamp;
    DXDesc.DepthClipEnable = TRUE;
    DXDesc.FillMode = m_Description.m_bWireFrame ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
    DXDesc.FrontCounterClockwise = m_Description.m_bFrontCounterClockwise;
    DXDesc.MultisampleEnable = TRUE;
    DXDesc.AntialiasedLineEnable = TRUE;
    DXDesc.ScissorEnable = m_Description.m_bScissorTest;
    DXDesc.SlopeScaledDepthBias = m_Description.m_fSlopeScaledDepthBias;

    if (FAILED(static_cast<wdGALDeviceDX11*>(pDevice)->GetDXDevice()->CreateRasterizerState(&DXDesc, &m_pDXRasterizerState)))
    {
      return WD_FAILURE;
    }
    else
    {
      return WD_SUCCESS;
    }
  }
}


wdResult wdGALRasterizerStateDX11::DeInitPlatform(wdGALDevice* pDevice)
{
  WD_GAL_DX11_RELEASE(m_pDXRasterizerState);
  return WD_SUCCESS;
}


// Sampler state

wdGALSamplerStateDX11::wdGALSamplerStateDX11(const wdGALSamplerStateCreationDescription& Description)
  : wdGALSamplerState(Description)
  , m_pDXSamplerState(nullptr)
{
}

wdGALSamplerStateDX11::~wdGALSamplerStateDX11() {}

/*
 */

wdResult wdGALSamplerStateDX11::InitPlatform(wdGALDevice* pDevice)
{
  D3D11_SAMPLER_DESC DXDesc;
  DXDesc.AddressU = GALTextureAddressModeToDX11[m_Description.m_AddressU];
  DXDesc.AddressV = GALTextureAddressModeToDX11[m_Description.m_AddressV];
  DXDesc.AddressW = GALTextureAddressModeToDX11[m_Description.m_AddressW];
  DXDesc.BorderColor[0] = m_Description.m_BorderColor.r;
  DXDesc.BorderColor[1] = m_Description.m_BorderColor.g;
  DXDesc.BorderColor[2] = m_Description.m_BorderColor.b;
  DXDesc.BorderColor[3] = m_Description.m_BorderColor.a;
  DXDesc.ComparisonFunc = GALCompareFuncToDX11[m_Description.m_SampleCompareFunc];

  if (m_Description.m_MagFilter == wdGALTextureFilterMode::Anisotropic || m_Description.m_MinFilter == wdGALTextureFilterMode::Anisotropic ||
      m_Description.m_MipFilter == wdGALTextureFilterMode::Anisotropic)
  {
    if (m_Description.m_SampleCompareFunc == wdGALCompareFunc::Never)
      DXDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    else
      DXDesc.Filter = D3D11_FILTER_COMPARISON_ANISOTROPIC;
  }
  else
  {
    wdUInt32 uiTableIndex = 0;

    if (m_Description.m_MipFilter == wdGALTextureFilterMode::Linear)
      uiTableIndex |= 1;

    if (m_Description.m_MagFilter == wdGALTextureFilterMode::Linear)
      uiTableIndex |= 2;

    if (m_Description.m_MinFilter == wdGALTextureFilterMode::Linear)
      uiTableIndex |= 4;

    if (m_Description.m_SampleCompareFunc != wdGALCompareFunc::Never)
      uiTableIndex |= 8;

    DXDesc.Filter = GALFilterTableIndexToDX11[uiTableIndex];
  }

  DXDesc.MaxAnisotropy = m_Description.m_uiMaxAnisotropy;
  DXDesc.MaxLOD = m_Description.m_fMaxMip;
  DXDesc.MinLOD = m_Description.m_fMinMip;
  DXDesc.MipLODBias = m_Description.m_fMipLodBias;

  if (FAILED(static_cast<wdGALDeviceDX11*>(pDevice)->GetDXDevice()->CreateSamplerState(&DXDesc, &m_pDXSamplerState)))
  {
    return WD_FAILURE;
  }
  else
  {
    return WD_SUCCESS;
  }
}


wdResult wdGALSamplerStateDX11::DeInitPlatform(wdGALDevice* pDevice)
{
  WD_GAL_DX11_RELEASE(m_pDXSamplerState);
  return WD_SUCCESS;
}



WD_STATICLINK_FILE(RendererDX11, RendererDX11_State_Implementation_StateDX11);
