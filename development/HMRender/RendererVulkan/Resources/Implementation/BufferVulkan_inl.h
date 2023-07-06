
vk::Buffer wdGALBufferVulkan::GetVkBuffer() const
{
  m_currentBuffer.m_currentFrame = m_pDeviceVulkan->GetCurrentFrame();
  return m_currentBuffer.m_buffer;
}

vk::IndexType wdGALBufferVulkan::GetIndexType() const
{
  return m_indexType;
}

wdVulkanAllocation wdGALBufferVulkan::GetAllocation() const
{
  return m_currentBuffer.m_alloc;
}

const wdVulkanAllocationInfo& wdGALBufferVulkan::GetAllocationInfo() const
{
  return m_allocInfo;
}

vk::PipelineStageFlags wdGALBufferVulkan::GetUsedByPipelineStage() const
{
  return m_stages;
}

vk::AccessFlags wdGALBufferVulkan::GetAccessMask() const
{
  return m_access;
}
