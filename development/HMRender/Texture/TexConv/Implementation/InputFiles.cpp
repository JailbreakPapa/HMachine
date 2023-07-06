#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>

wdResult wdTexConvProcessor::LoadInputImages()
{
  WD_PROFILE_SCOPE("Load Images");

  if (m_Descriptor.m_InputImages.IsEmpty() && m_Descriptor.m_InputFiles.IsEmpty())
  {
    wdLog::Error("No input images have been specified.");
    return WD_FAILURE;
  }

  if (!m_Descriptor.m_InputImages.IsEmpty() && !m_Descriptor.m_InputFiles.IsEmpty())
  {
    wdLog::Error("Both input files and input images have been specified. You need to either specify files or images.");
    return WD_FAILURE;
  }

  if (!m_Descriptor.m_InputImages.IsEmpty())
  {
    // make sure the two arrays have the same size
    m_Descriptor.m_InputFiles.SetCount(m_Descriptor.m_InputImages.GetCount());

    wdStringBuilder tmp;
    for (wdUInt32 i = 0; i < m_Descriptor.m_InputFiles.GetCount(); ++i)
    {
      tmp.Format("InputImage{}", wdArgI(i, 2, true));
      m_Descriptor.m_InputFiles[i] = tmp;
    }
  }
  else
  {
    m_Descriptor.m_InputImages.Reserve(m_Descriptor.m_InputFiles.GetCount());

    for (const auto& file : m_Descriptor.m_InputFiles)
    {
      auto& img = m_Descriptor.m_InputImages.ExpandAndGetRef();
      if (img.LoadFrom(file).Failed())
      {
        wdLog::Error("Could not load input file '{0}'.", wdArgSensitive(file, "File"));
        return WD_FAILURE;
      }
    }
  }

  for (wdUInt32 i = 0; i < m_Descriptor.m_InputFiles.GetCount(); ++i)
  {
    const auto& img = m_Descriptor.m_InputImages[i];

    if (img.GetImageFormat() == wdImageFormat::UNKNOWN)
    {
      wdLog::Error("Unknown image format for '{}'", wdArgSensitive(m_Descriptor.m_InputFiles[i], "File"));
      return WD_FAILURE;
    }
  }

  return WD_SUCCESS;
}

wdResult wdTexConvProcessor::ConvertAndScaleImage(const char* szImageName, wdImage& inout_Image, wdUInt32 uiResolutionX, wdUInt32 uiResolutionY, wdEnum<wdTexConvUsage> usage)
{
  const bool bSingleChannel = wdImageFormat::GetNumChannels(inout_Image.GetImageFormat()) == 1;

  if (inout_Image.Convert(wdImageFormat::R32G32B32A32_FLOAT).Failed())
  {
    wdLog::Error("Could not convert '{}' to RGBA 32-Bit Float format.", szImageName);
    return WD_FAILURE;
  }

  // some scale operations fail when they are done in place, so use a scratch image as destination for now
  wdImage scratch;
  if (wdImageUtils::Scale(inout_Image, scratch, uiResolutionX, uiResolutionY, nullptr, wdImageAddressMode::Clamp, wdImageAddressMode::Clamp).Failed())
  {
    wdLog::Error("Could not resize '{}' to {}x{}", szImageName, uiResolutionX, uiResolutionY);
    return WD_FAILURE;
  }

  inout_Image.ResetAndMove(std::move(scratch));

  if (usage == wdTexConvUsage::Color && bSingleChannel)
  {
    // replicate single channel ("red" textures) into the other channels
    WD_SUCCEED_OR_RETURN(wdImageUtils::CopyChannel(inout_Image, 1, inout_Image, 0));
    WD_SUCCEED_OR_RETURN(wdImageUtils::CopyChannel(inout_Image, 2, inout_Image, 0));
  }

  return WD_SUCCESS;
}

wdResult wdTexConvProcessor::ConvertAndScaleInputImages(wdUInt32 uiResolutionX, wdUInt32 uiResolutionY, wdEnum<wdTexConvUsage> usage)
{
  WD_PROFILE_SCOPE("ConvertAndScaleInputImages");

  for (wdUInt32 idx = 0; idx < m_Descriptor.m_InputImages.GetCount(); ++idx)
  {
    auto& img = m_Descriptor.m_InputImages[idx];
    const char* szName = m_Descriptor.m_InputFiles[idx];

    WD_SUCCEED_OR_RETURN(ConvertAndScaleImage(szName, img, uiResolutionX, uiResolutionY, usage));
  }

  return WD_SUCCESS;
}

WD_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_InputFiles);
