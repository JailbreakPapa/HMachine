#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/ProxyTexture.h>

namespace
{
  wdGALTextureCreationDescription MakeProxyDesc(const wdGALTextureCreationDescription& parentDesc)
  {
    wdGALTextureCreationDescription desc = parentDesc;
    desc.m_Type = wdGALTextureType::Texture2DProxy;
    return desc;
  }
} // namespace

wdGALProxyTexture::wdGALProxyTexture(const wdGALTexture& parentTexture)
  : wdGALTexture(MakeProxyDesc(parentTexture.GetDescription()))
  , m_pParentTexture(&parentTexture)
{
}

wdGALProxyTexture::~wdGALProxyTexture() {}


const wdGALResourceBase* wdGALProxyTexture::GetParentResource() const
{
  return m_pParentTexture;
}

wdResult wdGALProxyTexture::InitPlatform(wdGALDevice* pDevice, wdArrayPtr<wdGALSystemMemoryDescription> pInitialData)
{
  return WD_SUCCESS;
}

wdResult wdGALProxyTexture::DeInitPlatform(wdGALDevice* pDevice)
{
  return WD_SUCCESS;
}

void wdGALProxyTexture::SetDebugNamePlatform(const char* szName) const {}


WD_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_ProxyTexture);
