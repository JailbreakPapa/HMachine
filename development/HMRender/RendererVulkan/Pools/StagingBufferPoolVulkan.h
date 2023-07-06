#pragma once

#include <RendererVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

class wdGALDeviceVulkan;

struct wdStagingBufferVulkan
{
  vk::Buffer m_buffer;
  wdVulkanAllocation m_alloc;
  wdVulkanAllocationInfo m_allocInfo;
};

class WD_RENDERERVULKAN_DLL wdStagingBufferPoolVulkan
{
public:
  void Initialize(wdGALDeviceVulkan* pDevice);
  void DeInitialize();

  wdStagingBufferVulkan AllocateBuffer(vk::DeviceSize alignment, vk::DeviceSize size);
  void ReclaimBuffer(wdStagingBufferVulkan& buffer);

private:
  wdGALDeviceVulkan* m_pDevice = nullptr;
  vk::Device m_device;
};
