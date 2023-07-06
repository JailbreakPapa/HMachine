WD_ALWAYS_INLINE const vk::DescriptorImageInfo& wdGALUnorderedAccessViewVulkan::GetImageInfo() const
{
  return m_resourceImageInfo;
}

WD_ALWAYS_INLINE vk::ImageSubresourceRange wdGALUnorderedAccessViewVulkan::GetRange() const
{
  return m_range;
}
