#include <Texture/TexturePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <Texture/TexConv/TexConvProcessor.h>

wdResult wdTexConvProcessor::Assemble2DTexture(const wdImageHeader& refImg, wdImage& dst) const
{
  WD_PROFILE_SCOPE("Assemble2DTexture");

  dst.ResetAndAlloc(refImg);

  wdColor* pPixelOut = dst.GetPixelPointer<wdColor>();

  return Assemble2DSlice(m_Descriptor.m_ChannelMappings[0], refImg.GetWidth(), refImg.GetHeight(), pPixelOut);
}

wdResult wdTexConvProcessor::Assemble2DSlice(const wdTexConvSliceChannelMapping& mapping, wdUInt32 uiResolutionX, wdUInt32 uiResolutionY, wdColor* pPixelOut) const
{
  wdHybridArray<const wdColor*, 16> pSource;
  for (wdUInt32 i = 0; i < m_Descriptor.m_InputImages.GetCount(); ++i)
  {
    pSource.ExpandAndGetRef() = m_Descriptor.m_InputImages[i].GetPixelPointer<wdColor>();
  }

  const float fZero = 0.0f;
  const float fOne = 1.0f;
  const float* pSourceValues[4] = {nullptr, nullptr, nullptr, nullptr};
  wdUInt32 uiSourceStrides[4] = {0, 0, 0, 0};

  for (wdUInt32 channel = 0; channel < 4; ++channel)
  {
    const auto& cm = mapping.m_Channel[channel];
    const wdInt32 inputIndex = cm.m_iInputImageIndex;

    if (inputIndex != -1)
    {
      const wdColor* pSourcePixel = pSource[inputIndex];
      uiSourceStrides[channel] = 4;

      switch (cm.m_ChannelValue)
      {
        case wdTexConvChannelValue::Red:
          pSourceValues[channel] = &pSourcePixel->r;
          break;
        case wdTexConvChannelValue::Green:
          pSourceValues[channel] = &pSourcePixel->g;
          break;
        case wdTexConvChannelValue::Blue:
          pSourceValues[channel] = &pSourcePixel->b;
          break;
        case wdTexConvChannelValue::Alpha:
          pSourceValues[channel] = &pSourcePixel->a;
          break;

        default:
          WD_ASSERT_NOT_IMPLEMENTED;
          break;
      }
    }
    else
    {
      uiSourceStrides[channel] = 0; // because of the constant value

      switch (cm.m_ChannelValue)
      {
        case wdTexConvChannelValue::Black:
          pSourceValues[channel] = &fZero;
          break;

        case wdTexConvChannelValue::White:
          pSourceValues[channel] = &fOne;
          break;

        default:
          if (channel == 3)
            pSourceValues[channel] = &fOne;
          else
            pSourceValues[channel] = &fZero;
          break;
      }
    }
  }

  const bool bFlip = m_Descriptor.m_bFlipHorizontal;

  if (!bFlip && (pSourceValues[0] + 1 == pSourceValues[1]) && (pSourceValues[1] + 1 == pSourceValues[2]) &&
      (pSourceValues[2] + 1 == pSourceValues[3]))
  {
    WD_PROFILE_SCOPE("Assemble2DSlice(memcpy)");

    wdMemoryUtils::Copy<wdColor>(pPixelOut, reinterpret_cast<const wdColor*>(pSourceValues[0]), uiResolutionX * uiResolutionY);
  }
  else
  {
    WD_PROFILE_SCOPE("Assemble2DSlice(gather)");

    for (wdUInt32 y = 0; y < uiResolutionY; ++y)
    {
      const wdUInt32 pixelWriteRowOffset = uiResolutionX * (bFlip ? (uiResolutionY - y - 1) : y);

      for (wdUInt32 x = 0; x < uiResolutionX; ++x)
      {
        float* dst = &pPixelOut[pixelWriteRowOffset + x].r;

        for (wdUInt32 c = 0; c < 4; ++c)
        {
          dst[c] = *pSourceValues[c];
          pSourceValues[c] += uiSourceStrides[c];
        }
      }
    }
  }

  return WD_SUCCESS;
}

wdResult wdTexConvProcessor::DetermineTargetResolution(const wdImage& image, wdEnum<wdImageFormat> OutputImageFormat, wdUInt32& out_uiTargetResolutionX, wdUInt32& out_uiTargetResolutionY) const
{
  WD_PROFILE_SCOPE("DetermineResolution");

  WD_ASSERT_DEV(out_uiTargetResolutionX == 0 && out_uiTargetResolutionY == 0, "Target resolution already determined");

  const wdUInt32 uiOrgResX = image.GetWidth();
  const wdUInt32 uiOrgResY = image.GetHeight();

  out_uiTargetResolutionX = uiOrgResX;
  out_uiTargetResolutionY = uiOrgResY;

  out_uiTargetResolutionX /= (1 << m_Descriptor.m_uiDownscaleSteps);
  out_uiTargetResolutionY /= (1 << m_Descriptor.m_uiDownscaleSteps);

  out_uiTargetResolutionX = wdMath::Clamp(out_uiTargetResolutionX, m_Descriptor.m_uiMinResolution, m_Descriptor.m_uiMaxResolution);
  out_uiTargetResolutionY = wdMath::Clamp(out_uiTargetResolutionY, m_Descriptor.m_uiMinResolution, m_Descriptor.m_uiMaxResolution);

  // keep original aspect ratio
  if (uiOrgResX > uiOrgResY)
  {
    out_uiTargetResolutionY = (out_uiTargetResolutionX * uiOrgResY) / uiOrgResX;
  }
  else if (uiOrgResX < uiOrgResY)
  {
    out_uiTargetResolutionX = (out_uiTargetResolutionY * uiOrgResX) / uiOrgResY;
  }

  if (m_Descriptor.m_OutputType == wdTexConvOutputType::Volume)
  {
    wdUInt32 uiScaleFactor = uiOrgResY / out_uiTargetResolutionY;
    out_uiTargetResolutionX = uiOrgResX / uiScaleFactor;
  }

  if (OutputImageFormat != wdImageFormat::UNKNOWN && wdImageFormat::RequiresFirstLevelBlockAlignment(OutputImageFormat))
  {
    const wdUInt32 blockWidth = wdImageFormat::GetBlockWidth(OutputImageFormat);

    wdUInt32 currentWidth = out_uiTargetResolutionX;
    wdUInt32 currentHeight = out_uiTargetResolutionY;
    bool issueWarning = false;

    if (out_uiTargetResolutionX % blockWidth != 0)
    {
      out_uiTargetResolutionX = wdMath::RoundUp(out_uiTargetResolutionX, static_cast<wdUInt16>(blockWidth));
      issueWarning = true;
    }

    wdUInt32 blockHeight = wdImageFormat::GetBlockHeight(OutputImageFormat);
    if (out_uiTargetResolutionY % blockHeight != 0)
    {
      out_uiTargetResolutionY = wdMath::RoundUp(out_uiTargetResolutionY, static_cast<wdUInt16>(blockHeight));
      issueWarning = true;
    }

    if (issueWarning)
    {
      wdLog::Warning(
        "Chosen output image format is compressed, but target resolution does not fulfill block size requirements. {}x{} -> downscale {} / "
        "clamp({}, {}) -> {}x{}, adjusted to {}x{}",
        uiOrgResX, uiOrgResY, m_Descriptor.m_uiDownscaleSteps, m_Descriptor.m_uiMinResolution, m_Descriptor.m_uiMaxResolution, currentWidth,
        currentHeight, out_uiTargetResolutionX, out_uiTargetResolutionY);
    }
  }

  return WD_SUCCESS;
}

WD_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_Texture2D);
