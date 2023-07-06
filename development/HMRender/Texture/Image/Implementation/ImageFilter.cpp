#include <Texture/TexturePCH.h>

#include <Texture/Image/ImageFilter.h>

wdSimdFloat wdImageFilter::GetWidth() const
{
  return m_fWidth;
}

wdImageFilter::wdImageFilter(float width)
  : m_fWidth(width)
{
}

wdImageFilterBox::wdImageFilterBox(float fWidth)
  : wdImageFilter(fWidth)
{
}

wdSimdFloat wdImageFilterBox::SamplePoint(const wdSimdFloat& x) const
{
  wdSimdFloat absX = x.Abs();

  if (absX <= GetWidth())
  {
    return 1.0f;
  }
  else
  {
    return 0.0f;
  }
}

wdImageFilterTriangle::wdImageFilterTriangle(float fWidth)
  : wdImageFilter(fWidth)
{
}

wdSimdFloat wdImageFilterTriangle::SamplePoint(const wdSimdFloat& x) const
{
  wdSimdFloat absX = x.Abs();

  wdSimdFloat width = GetWidth();

  if (absX <= width)
  {
    return width - absX;
  }
  else
  {
    return 0.0f;
  }
}

static wdSimdFloat sinc(const wdSimdFloat& x)
{
  wdSimdFloat absX = x.Abs();

  // Use Taylor expansion for small values to avoid division
  if (absX < 0.0001f)
  {
    // sin(x) / x = (x - x^3/6 + x^5/120 - ...) / x = 1 - x^2/6 + x^4/120 - ...
    return wdSimdFloat(1.0f) - x * x * wdSimdFloat(1.0f / 6.0f);
  }
  else
  {
    return wdMath::Sin(wdAngle::Radian(x)) / x;
  }
}

static wdSimdFloat modifiedBessel0(const wdSimdFloat& x)
{
  // Implementation as I0(x) = sum((1/4 * x * x) ^ k / (k!)^2, k, 0, inf), see
  // http://mathworld.wolfram.com/ModifiedBesselFunctionoftheFirstKind.html

  wdSimdFloat sum = 1.0f;

  wdSimdFloat xSquared = x * x * wdSimdFloat(0.25f);

  wdSimdFloat currentTerm = xSquared;

  for (wdUInt32 i = 2; currentTerm > 0.001f; ++i)
  {
    sum += currentTerm;
    currentTerm *= xSquared / wdSimdFloat(i * i);
  }

  return sum;
}

wdImageFilterSincWithKaiserWindow::wdImageFilterSincWithKaiserWindow(float fWidth, float fBeta)
  : wdImageFilter(fWidth)
  , m_fBeta(fBeta)
  , m_fInvBesselBeta(1.0f / modifiedBessel0(m_fBeta))
{
}

wdSimdFloat wdImageFilterSincWithKaiserWindow::SamplePoint(const wdSimdFloat& x) const
{
  wdSimdFloat scaledX = x / GetWidth();

  wdSimdFloat xSq = 1.0f - scaledX * scaledX;

  if (xSq <= 0.0f)
  {
    return 0.0f;
  }
  else
  {
    return sinc(x * wdSimdFloat(wdMath::Pi<float>())) * modifiedBessel0(m_fBeta * xSq.GetSqrt()) * m_fInvBesselBeta;
  }
}

wdImageFilterWeights::wdImageFilterWeights(const wdImageFilter& filter, wdUInt32 uiSrcSamples, wdUInt32 uiDstSamples)
{
  // Filter weights repeat after the common phase
  wdUInt32 commonPhase = wdMath::GreatestCommonDivisor(uiSrcSamples, uiDstSamples);

  uiSrcSamples /= commonPhase;
  uiDstSamples /= commonPhase;

  m_uiDstSamplesReduced = uiDstSamples;

  m_fSourceToDestScale = float(uiDstSamples) / float(uiSrcSamples);
  m_fDestToSourceScale = float(uiSrcSamples) / float(uiDstSamples);

  wdSimdFloat filterScale, invFilterScale;

  if (uiDstSamples > uiSrcSamples)
  {
    // When upsampling, reconstruct the source by applying the filter in source space and resampling
    filterScale = 1.0f;
    invFilterScale = 1.0f;
  }
  else
  {
    // When downsampling, widen the filter in order to narrow its frequency spectrum, which effectively combines reconstruction + low-pass
    // filter
    filterScale = m_fDestToSourceScale;
    invFilterScale = m_fSourceToDestScale;
  }

  m_fWidthInSourceSpace = filter.GetWidth() * filterScale;

  m_uiNumWeights = wdUInt32(wdMath::Ceil(m_fWidthInSourceSpace * wdSimdFloat(2.0f))) + 1;

  m_Weights.SetCountUninitialized(uiDstSamples * m_uiNumWeights);

  for (wdUInt32 dstSample = 0; dstSample < uiDstSamples; ++dstSample)
  {
    wdSimdFloat dstSampleInSourceSpace = (wdSimdFloat(dstSample) + wdSimdFloat(0.5f)) * m_fDestToSourceScale;

    wdInt32 firstSourceIdx = GetFirstSourceSampleIndex(dstSample);

    wdSimdFloat totalWeight = 0.0f;

    for (wdUInt32 weightIdx = 0; weightIdx < m_uiNumWeights; ++weightIdx)
    {
      wdSimdFloat sourceSample = wdSimdFloat(firstSourceIdx + wdInt32(weightIdx)) + wdSimdFloat(0.5f);

      wdSimdFloat weight = filter.SamplePoint((dstSampleInSourceSpace - sourceSample) * invFilterScale);
      totalWeight += weight;
      m_Weights[dstSample * m_uiNumWeights + weightIdx] = weight;
    }

    // Normalize weights
    wdSimdFloat invWeight = 1.0f / totalWeight;

    for (wdUInt32 weightIdx = 0; weightIdx < m_uiNumWeights; ++weightIdx)
    {
      m_Weights[dstSample * m_uiNumWeights + weightIdx] *= invWeight;
    }
  }
}

wdUInt32 wdImageFilterWeights::GetNumWeights() const
{
  return m_uiNumWeights;
}

wdSimdFloat wdImageFilterWeights::GetWeight(wdUInt32 uiDstSampleIndex, wdUInt32 uiWeightIndex) const
{
  WD_ASSERT_DEBUG(uiWeightIndex < m_uiNumWeights, "Invalid weight index {} (should be < {})", uiWeightIndex, m_uiNumWeights);

  return wdSimdFloat(m_Weights[(uiDstSampleIndex % m_uiDstSamplesReduced) * m_uiNumWeights + uiWeightIndex]);
}



WD_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageFilter);
