wdInt32 wdImageFilterWeights::GetFirstSourceSampleIndex(wdUInt32 uiDstSampleIndex) const
{
  wdSimdFloat dstSampleInSourceSpace = (wdSimdFloat(uiDstSampleIndex) + wdSimdFloat(0.5f)) * m_fDestToSourceScale;

  return wdInt32(wdMath::Floor(dstSampleInSourceSpace - m_fWidthInSourceSpace));
}

inline wdArrayPtr<const float> wdImageFilterWeights::ViewWeights() const
{
  return m_Weights;
}
