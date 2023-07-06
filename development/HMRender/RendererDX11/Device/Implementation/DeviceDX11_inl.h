
WD_ALWAYS_INLINE ID3D11Device* wdGALDeviceDX11::GetDXDevice() const
{
  return m_pDevice;
}

WD_ALWAYS_INLINE ID3D11Device3* wdGALDeviceDX11::GetDXDevice3() const
{
  return m_pDevice3;
}

WD_ALWAYS_INLINE ID3D11DeviceContext* wdGALDeviceDX11::GetDXImmediateContext() const
{
  return m_pImmediateContext;
}

WD_ALWAYS_INLINE IDXGIFactory1* wdGALDeviceDX11::GetDXGIFactory() const
{
  return m_pDXGIFactory;
}

WD_ALWAYS_INLINE const wdGALFormatLookupTableDX11& wdGALDeviceDX11::GetFormatLookupTable() const
{
  return m_FormatLookupTable;
}

inline ID3D11Query* wdGALDeviceDX11::GetTimestamp(wdGALTimestampHandle hTimestamp)
{
  if (hTimestamp.m_uiIndex < m_Timestamps.GetCount())
  {
    return m_Timestamps[static_cast<wdUInt32>(hTimestamp.m_uiIndex)];
  }

  return nullptr;
}
