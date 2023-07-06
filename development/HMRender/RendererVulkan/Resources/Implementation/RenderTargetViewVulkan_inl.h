

WD_ALWAYS_INLINE vk::ImageView wdGALRenderTargetViewVulkan::GetImageView() const
{
  return m_imageView;
}

WD_ALWAYS_INLINE bool wdGALRenderTargetViewVulkan::IsFullRange() const
{
  return m_bfullRange;
}

WD_ALWAYS_INLINE vk::ImageSubresourceRange wdGALRenderTargetViewVulkan::GetRange() const
{
  return m_range;
}
