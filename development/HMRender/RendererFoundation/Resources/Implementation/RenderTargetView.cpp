#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/RenderTargetView.h>


wdGALRenderTargetView::wdGALRenderTargetView(wdGALTexture* pTexture, const wdGALRenderTargetViewCreationDescription& description)
  : wdGALObject(description)
  , m_pTexture(pTexture)
{
  WD_ASSERT_DEV(m_pTexture != nullptr, "Texture must not be null");
}

wdGALRenderTargetView::~wdGALRenderTargetView() {}



WD_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_RenderTargetView);
