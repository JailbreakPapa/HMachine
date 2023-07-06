#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/Texture.h>

wdGALTexture::wdGALTexture(const wdGALTextureCreationDescription& Description)
  : wdGALResource(Description)
{
}

wdGALTexture::~wdGALTexture() {}



WD_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_Texture);
