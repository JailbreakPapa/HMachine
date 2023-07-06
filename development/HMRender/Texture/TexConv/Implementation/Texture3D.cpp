#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

wdResult wdTexConvProcessor::Assemble3DTexture(wdImage& dst) const
{
  WD_PROFILE_SCOPE("Assemble3DTexture");

  const auto& images = m_Descriptor.m_InputImages;

  return wdImageUtils::CreateVolumeTextureFromSingleFile(dst, images[0]);
}


WD_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_Texture3D);
