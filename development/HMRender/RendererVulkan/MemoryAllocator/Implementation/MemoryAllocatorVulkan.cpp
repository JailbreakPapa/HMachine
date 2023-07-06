#include <RendererVulkan/RendererVulkanPCH.h>


VKAPI_ATTR void VKAPI_CALL vkGetDeviceBufferMemoryRequirements(
  VkDevice device,
  const VkDeviceBufferMemoryRequirements* pInfo,
  VkMemoryRequirements2* pMemoryRequirements)
{
  WD_REPORT_FAILURE("FIXME: Added to prevent the error: The procedure entry point vkGetDeviceBufferMemoryRequirements could not be located in the dynamic link library wdRendererVulkan.dll.");
}

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>

#define VMA_VULKAN_VERSION 1001000
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_STATS_STRING_ENABLED 1


//
//#define VMA_DEBUG_LOG(format, ...)   \
//  do                                 \
//  {                                  \
//    wdStringBuilder tmp;             \
//    tmp.Printf(format, __VA_ARGS__); \
//    wdLog::Error("{}", tmp);         \
//  } while (false)

#include <RendererVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>

#define VMA_IMPLEMENTATION

#ifndef VA_IGNORE_THIS_FILE
#  define VA_INCLUDE_HIDDEN <vma/vk_mem_alloc.h>
#else
#  define VA_INCLUDE_HIDDEN ""
#endif

#include VA_INCLUDE_HIDDEN

WD_CHECK_AT_COMPILETIME(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT == wdVulkanAllocationCreateFlags::DedicatedMemory);
WD_CHECK_AT_COMPILETIME(VMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT == wdVulkanAllocationCreateFlags::NeverAllocate);
WD_CHECK_AT_COMPILETIME(VMA_ALLOCATION_CREATE_MAPPED_BIT == wdVulkanAllocationCreateFlags::Mapped);
WD_CHECK_AT_COMPILETIME(VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT == wdVulkanAllocationCreateFlags::CanAlias);
WD_CHECK_AT_COMPILETIME(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT == wdVulkanAllocationCreateFlags::HostAccessSequentialWrite);
WD_CHECK_AT_COMPILETIME(VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT == wdVulkanAllocationCreateFlags::HostAccessRandom);
WD_CHECK_AT_COMPILETIME(VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT == wdVulkanAllocationCreateFlags::StrategyMinMemory);
WD_CHECK_AT_COMPILETIME(VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT == wdVulkanAllocationCreateFlags::StrategyMinTime);

WD_CHECK_AT_COMPILETIME(VMA_MEMORY_USAGE_UNKNOWN == wdVulkanMemoryUsage::Unknown);
WD_CHECK_AT_COMPILETIME(VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED == wdVulkanMemoryUsage::GpuLazilyAllocated);
WD_CHECK_AT_COMPILETIME(VMA_MEMORY_USAGE_AUTO == wdVulkanMemoryUsage::Auto);
WD_CHECK_AT_COMPILETIME(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE == wdVulkanMemoryUsage::AutoPreferDevice);
WD_CHECK_AT_COMPILETIME(VMA_MEMORY_USAGE_AUTO_PREFER_HOST == wdVulkanMemoryUsage::AutoPreferHost);

WD_CHECK_AT_COMPILETIME(sizeof(wdVulkanAllocation) == sizeof(VmaAllocation));

WD_CHECK_AT_COMPILETIME(sizeof(wdVulkanAllocationInfo) == sizeof(VmaAllocationInfo));


struct wdMemoryAllocatorVulkan::Impl
{
  WD_DECLARE_POD_TYPE();
  VmaAllocator m_allocator;
};

wdMemoryAllocatorVulkan::Impl* wdMemoryAllocatorVulkan::m_pImpl = nullptr;

vk::Result wdMemoryAllocatorVulkan::Initialize(vk::PhysicalDevice physicalDevice, vk::Device device, vk::Instance instance)
{
  WD_ASSERT_DEV(m_pImpl == nullptr, "wdMemoryAllocatorVulkan::Initialize was already called");
  m_pImpl = WD_DEFAULT_NEW(Impl);

  VmaVulkanFunctions vulkanFunctions = {};
  vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
  vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

  VmaAllocatorCreateInfo allocatorCreateInfo = {};
  allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_1;
  allocatorCreateInfo.physicalDevice = physicalDevice;
  allocatorCreateInfo.device = device;
  allocatorCreateInfo.instance = instance;
  allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

  vk::Result res = (vk::Result)vmaCreateAllocator(&allocatorCreateInfo, &m_pImpl->m_allocator);
  if (res != vk::Result::eSuccess)
  {
    WD_DEFAULT_DELETE(m_pImpl);
  }
  return res;
}

void wdMemoryAllocatorVulkan::DeInitialize()
{
  WD_ASSERT_DEV(m_pImpl != nullptr, "wdMemoryAllocatorVulkan is not initialized.");

  vmaDestroyAllocator(m_pImpl->m_allocator);
  WD_DEFAULT_DELETE(m_pImpl);
}

vk::Result wdMemoryAllocatorVulkan::CreateImage(const vk::ImageCreateInfo& imageCreateInfo, const wdVulkanAllocationCreateInfo& allocationCreateInfo, vk::Image& out_image, wdVulkanAllocation& out_alloc, wdVulkanAllocationInfo* pAllocInfo)
{
  VmaAllocationCreateInfo allocCreateInfo = {};
  allocCreateInfo.usage = (VmaMemoryUsage)allocationCreateInfo.m_usage.GetValue();
  allocCreateInfo.flags = allocationCreateInfo.m_flags.GetValue() | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
  allocCreateInfo.pUserData = (void*)allocationCreateInfo.m_pUserData;

  return (vk::Result)vmaCreateImage(m_pImpl->m_allocator, reinterpret_cast<const VkImageCreateInfo*>(&imageCreateInfo), &allocCreateInfo, reinterpret_cast<VkImage*>(&out_image), reinterpret_cast<VmaAllocation*>(&out_alloc), reinterpret_cast<VmaAllocationInfo*>(pAllocInfo));
}

void wdMemoryAllocatorVulkan::DestroyImage(vk::Image& image, wdVulkanAllocation& alloc)
{
  vmaSetAllocationUserData(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), nullptr);
  vmaDestroyImage(m_pImpl->m_allocator, reinterpret_cast<VkImage&>(image), reinterpret_cast<VmaAllocation&>(alloc));
  image = nullptr;
  alloc = nullptr;
}

vk::Result wdMemoryAllocatorVulkan::CreateBuffer(const vk::BufferCreateInfo& bufferCreateInfo, const wdVulkanAllocationCreateInfo& allocationCreateInfo, vk::Buffer& out_buffer, wdVulkanAllocation& out_alloc, wdVulkanAllocationInfo* pAllocInfo)
{
  VmaAllocationCreateInfo allocCreateInfo = {};
  allocCreateInfo.usage = (VmaMemoryUsage)allocationCreateInfo.m_usage.GetValue();
  allocCreateInfo.flags = allocationCreateInfo.m_flags.GetValue() | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
  allocCreateInfo.pUserData = (void*)allocationCreateInfo.m_pUserData;

  return (vk::Result)vmaCreateBuffer(m_pImpl->m_allocator, reinterpret_cast<const VkBufferCreateInfo*>(&bufferCreateInfo), &allocCreateInfo, reinterpret_cast<VkBuffer*>(&out_buffer), reinterpret_cast<VmaAllocation*>(&out_alloc), reinterpret_cast<VmaAllocationInfo*>(pAllocInfo));
}

void wdMemoryAllocatorVulkan::DestroyBuffer(vk::Buffer& buffer, wdVulkanAllocation& alloc)
{
  vmaSetAllocationUserData(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), nullptr);
  vmaDestroyBuffer(m_pImpl->m_allocator, reinterpret_cast<VkBuffer&>(buffer), reinterpret_cast<VmaAllocation&>(alloc));
  buffer = nullptr;
  alloc = nullptr;
}

wdVulkanAllocationInfo wdMemoryAllocatorVulkan::GetAllocationInfo(wdVulkanAllocation alloc)
{
  VmaAllocationInfo info;
  vmaGetAllocationInfo(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), &info);

  return reinterpret_cast<wdVulkanAllocationInfo&>(info);
}

void wdMemoryAllocatorVulkan::SetAllocationUserData(wdVulkanAllocation alloc, const char* pUserData)
{
  vmaSetAllocationUserData(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), (void*)pUserData);
}

vk::Result wdMemoryAllocatorVulkan::MapMemory(wdVulkanAllocation alloc, void** pData)
{
  return (vk::Result)vmaMapMemory(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), pData);
}

void wdMemoryAllocatorVulkan::UnmapMemory(wdVulkanAllocation alloc)
{
  vmaUnmapMemory(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc));
}

vk::Result wdMemoryAllocatorVulkan::FlushAllocation(wdVulkanAllocation alloc, vk::DeviceSize offset, vk::DeviceSize size)
{
  return (vk::Result)vmaFlushAllocation(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), offset, size);
}

vk::Result wdMemoryAllocatorVulkan::InvalidateAllocation(wdVulkanAllocation alloc, vk::DeviceSize offset, vk::DeviceSize size)
{
  return (vk::Result)vmaInvalidateAllocation(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), offset, size);
}
