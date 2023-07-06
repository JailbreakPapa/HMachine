WD_ALWAYS_INLINE const vk::DescriptorImageInfo& wdGALResourceViewVulkan::GetImageInfo(bool bIsArray) const
{
  WD_ASSERT_DEBUG((bIsArray ? m_resourceImageInfoArray : m_resourceImageInfo).imageView, "View does not support bIsArray: {}", bIsArray);
  return bIsArray ? m_resourceImageInfoArray : m_resourceImageInfo;
}

WD_ALWAYS_INLINE vk::ImageSubresourceRange wdGALResourceViewVulkan::GetRange() const
{
  return m_range;
}

WD_ALWAYS_INLINE const vk::BufferView& wdGALResourceViewVulkan::GetBufferView() const
{
  return m_bufferView;
}
