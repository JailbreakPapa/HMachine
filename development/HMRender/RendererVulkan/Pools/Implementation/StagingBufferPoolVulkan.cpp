#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Pools/StagingBufferPoolVulkan.h>

#include <RendererVulkan/Device/DeviceVulkan.h>

void wdStagingBufferPoolVulkan::Initialize(wdGALDeviceVulkan* pDevice)
{
  m_pDevice = pDevice;
  m_device = pDevice->GetVulkanDevice();
}

void wdStagingBufferPoolVulkan::DeInitialize()
{
  m_device = nullptr;
}

wdStagingBufferVulkan wdStagingBufferPoolVulkan::AllocateBuffer(vk::DeviceSize alignment, vk::DeviceSize size)
{
  //#TODO_VULKAN alignment
  wdStagingBufferVulkan buffer;

  WD_ASSERT_DEBUG(m_device, "wdStagingBufferPoolVulkan::Initialize not called");
  vk::BufferCreateInfo bufferCreateInfo = {};
  bufferCreateInfo.size = size;
  bufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;

  bufferCreateInfo.pQueueFamilyIndices = nullptr;
  bufferCreateInfo.queueFamilyIndexCount = 0;
  bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;


  wdVulkanAllocationCreateInfo allocInfo;
  allocInfo.m_usage = wdVulkanMemoryUsage::Auto;
  allocInfo.m_flags = wdVulkanAllocationCreateFlags::HostAccessSequentialWrite;

  VK_ASSERT_DEV(wdMemoryAllocatorVulkan::CreateBuffer(bufferCreateInfo, allocInfo, buffer.m_buffer, buffer.m_alloc, &buffer.m_allocInfo));

  return buffer;
}

void wdStagingBufferPoolVulkan::ReclaimBuffer(wdStagingBufferVulkan& buffer)
{
  m_pDevice->DeleteLater(buffer.m_buffer, buffer.m_alloc);

  //WD_ASSERT_DEBUG(m_device, "wdStagingBufferPoolVulkan::Initialize not called");
  //wdMemoryAllocatorVulkan::DestroyBuffer(buffer.m_buffer, buffer.m_alloc);
}
