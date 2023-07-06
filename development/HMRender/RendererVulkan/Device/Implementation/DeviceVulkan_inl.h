WD_ALWAYS_INLINE vk::Device wdGALDeviceVulkan::GetVulkanDevice() const
{
  return m_device;
}

WD_ALWAYS_INLINE const wdGALDeviceVulkan::Queue& wdGALDeviceVulkan::GetGraphicsQueue() const
{
  return m_graphicsQueue;
}

WD_ALWAYS_INLINE const wdGALDeviceVulkan::Queue& wdGALDeviceVulkan::GetTransferQueue() const
{
  return m_transferQueue;
}

WD_ALWAYS_INLINE vk::PhysicalDevice wdGALDeviceVulkan::GetVulkanPhysicalDevice() const
{
  return m_physicalDevice;
}

WD_ALWAYS_INLINE vk::Instance wdGALDeviceVulkan::GetVulkanInstance() const
{
  return m_instance;
}


WD_ALWAYS_INLINE const wdGALFormatLookupTableVulkan& wdGALDeviceVulkan::GetFormatLookupTable() const
{
  return m_FormatLookupTable;
}

/*
inline ID3D11Query* wdGALDeviceVulkan::GetTimestamp(wdGALTimestampHandle hTimestamp)
{
  if (hTimestamp.m_uiIndex < m_Timestamps.GetCount())
  {
    return m_Timestamps[static_cast<wdUInt32>(hTimestamp.m_uiIndex)];
  }

  return nullptr;
}

*/
