#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

wdResult wdTexConvProcessor::AssembleCubemap(wdImage& dst) const
{
  WD_PROFILE_SCOPE("AssembleCubemap");

  const auto& cm = m_Descriptor.m_ChannelMappings;
  const auto& images = m_Descriptor.m_InputImages;

  if (m_Descriptor.m_ChannelMappings.GetCount() == 6)
  {
    wdImageView faces[6];
    faces[0] = images[cm[0].m_Channel[0].m_iInputImageIndex];
    faces[1] = images[cm[1].m_Channel[0].m_iInputImageIndex];
    faces[2] = images[cm[2].m_Channel[0].m_iInputImageIndex];
    faces[3] = images[cm[3].m_Channel[0].m_iInputImageIndex];
    faces[4] = images[cm[4].m_Channel[0].m_iInputImageIndex];
    faces[5] = images[cm[5].m_Channel[0].m_iInputImageIndex];

    if (wdImageUtils::CreateCubemapFrom6Files(dst, faces).Failed())
    {
      wdLog::Error("Failed to assemble cubemap from 6 images. Images must be square, with power-of-two resolutions.");
      return WD_FAILURE;
    }
  }
  else
  {
    if (wdImageUtils::CreateCubemapFromSingleFile(dst, images[0]).Failed())
    {
      wdLog::Error("Failed to assemble cubemap from single image.");
      return WD_FAILURE;
    }
  }

  return WD_SUCCESS;
}

WD_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_TextureCube);
