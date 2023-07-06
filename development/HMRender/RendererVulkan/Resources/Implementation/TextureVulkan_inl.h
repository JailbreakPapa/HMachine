vk::Image wdGALTextureVulkan::GetImage() const
{
  return m_image;
}

vk::ImageLayout wdGALTextureVulkan::GetPreferredLayout() const
{
  return m_preferredLayout;
}

vk::ImageLayout wdGALTextureVulkan::GetPreferredLayout(vk::ImageLayout targetLayout) const
{
  return targetLayout;
  //#TODO_VULKAN Maintaining UAVs in general layout causes verification failures. For now, switch back and forth between layouts.
  //return m_preferredLayout == vk::ImageLayout::eGeneral ? vk::ImageLayout::eGeneral : targetLayout;
}

vk::PipelineStageFlags wdGALTextureVulkan::GetUsedByPipelineStage() const
{
  return m_stages;
}

vk::AccessFlags wdGALTextureVulkan::GetAccessMask() const
{
  return m_access;
}

wdVulkanAllocation wdGALTextureVulkan::GetAllocation() const
{
  return m_alloc;
}

const wdVulkanAllocationInfo& wdGALTextureVulkan::GetAllocationInfo() const
{
  return m_allocInfo;
}

bool wdGALTextureVulkan::GetFormatOverrideEnabled() const
{
  return m_formatOverride;
}

bool wdGALTextureVulkan::IsLinearLayout() const
{
  return m_bLinearCPU;
}

wdGALTextureVulkan::StagingMode wdGALTextureVulkan::GetStagingMode() const
{
  return m_stagingMode;
}

wdGALTextureHandle wdGALTextureVulkan::GetStagingTexture() const
{
  return m_hStagingTexture;
}

wdGALBufferHandle wdGALTextureVulkan::GetStagingBuffer() const
{
  return m_hStagingBuffer;
}
