#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/SimdMath/SimdFloat.h>
#include <Texture/TextureDLL.h>

/// \brief Represents a function used for filtering an image.
class WD_TEXTURE_DLL wdImageFilter
{
public:
  /// \brief Samples the filter function at a single point. Note that the distribution isn't necessarily normalized.
  virtual wdSimdFloat SamplePoint(const wdSimdFloat& x) const = 0;

  /// \brief Returns the width of the filter; outside of the interval [-width, width], the filter function is always zero.
  wdSimdFloat GetWidth() const;

protected:
  wdImageFilter(float width);

private:
  wdSimdFloat m_fWidth;
};

/// \brief Box filter
class WD_TEXTURE_DLL wdImageFilterBox : public wdImageFilter
{
public:
  wdImageFilterBox(float fWidth = 0.5f);

  virtual wdSimdFloat SamplePoint(const wdSimdFloat& x) const override;
};

/// \brief Triangle filter
class WD_TEXTURE_DLL wdImageFilterTriangle : public wdImageFilter
{
public:
  wdImageFilterTriangle(float fWidth = 1.0f);

  virtual wdSimdFloat SamplePoint(const wdSimdFloat& x) const override;
};

/// \brief Kaiser-windowed sinc filter
class WD_TEXTURE_DLL wdImageFilterSincWithKaiserWindow : public wdImageFilter
{
public:
  /// \brief Construct a sinc filter with a Kaiser window of the given window width and beta parameter.
  /// Note that the beta parameter (equaling alpha * pi in the mathematical definition of the Kaiser window) is often incorrectly alpha by other
  /// filtering tools.
  wdImageFilterSincWithKaiserWindow(float fWindowWidth = 3.0f, float fBeta = 4.0f);

  virtual wdSimdFloat SamplePoint(const wdSimdFloat& x) const override;

private:
  wdSimdFloat m_fBeta;
  wdSimdFloat m_fInvBesselBeta;
};

/// \brief Pre-computes the required filter weights for rescaling a sequence of image samples.
class WD_TEXTURE_DLL wdImageFilterWeights
{
public:
  /// \brief Pre-compute the weights for the given filter for scaling between the given number of samples.
  wdImageFilterWeights(const wdImageFilter& filter, wdUInt32 uiSrcSamples, wdUInt32 uiDstSamples);

  /// \brief Returns the number of weights.
  wdUInt32 GetNumWeights() const;

  /// \brief Returns the weight used for the source sample GetFirstSourceSampleIndex(dstSampleIndex) + weightIndex
  wdSimdFloat GetWeight(wdUInt32 uiDstSampleIndex, wdUInt32 uiWeightIndex) const;

  /// \brief Returns the index of the first source sample that needs to be weighted to evaluate the destination sample
  inline wdInt32 GetFirstSourceSampleIndex(wdUInt32 uiDstSampleIndex) const;

  wdArrayPtr<const float> ViewWeights() const;

private:
  wdHybridArray<float, 16> m_Weights;
  wdSimdFloat m_fWidthInSourceSpace;
  wdSimdFloat m_fSourceToDestScale;
  wdSimdFloat m_fDestToSourceScale;
  wdUInt32 m_uiNumWeights;
  wdUInt32 m_uiDstSamplesReduced;
};

#include <Texture/Image/Implementation/ImageFilter_inl.h>
