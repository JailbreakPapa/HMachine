#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Pools/FencePoolVulkan.h>

vk::Device wdFencePoolVulkan::s_device;
wdHybridArray<vk::Fence, 4> wdFencePoolVulkan::s_Fences;

void wdFencePoolVulkan::Initialize(vk::Device device)
{
  s_device = device;
}

void wdFencePoolVulkan::DeInitialize()
{
  for (vk::Fence& fence : s_Fences)
  {
    s_device.destroyFence(fence, nullptr);
  }
  s_Fences.Clear();
  s_Fences.Compact();

  s_device = nullptr;
}

vk::Fence wdFencePoolVulkan::RequestFence()
{
  WD_ASSERT_DEBUG(s_device, "wdFencePoolVulkan::Initialize not called");
  if (!s_Fences.IsEmpty())
  {
    vk::Fence Fence = s_Fences.PeekBack();
    s_Fences.PopBack();
    return Fence;
  }
  else
  {
    vk::Fence fence;
    vk::FenceCreateInfo createInfo = {};
    VK_ASSERT_DEV(s_device.createFence(&createInfo, nullptr, &fence));
    return fence;
  }
}

void wdFencePoolVulkan::ReclaimFence(vk::Fence& fence)
{
  vk::Result fenceStatus = s_device.getFenceStatus(fence);
  if (fenceStatus == vk::Result::eNotReady)
  {
    //#TODO_VULKAN Workaround for fences that were waited for (and thus signaled) returning VK_NOT_READY if AMDs profiler is active.
    //The fence will simply take another round through the reclaim process and will eventually turn signaled.
    static_cast<wdGALDeviceVulkan*>(wdGALDevice::GetDefaultDevice())->ReclaimLater(fence);
    return;
  }
  VK_ASSERT_DEV(fenceStatus);
  s_device.resetFences(1, &fence);
  WD_ASSERT_DEBUG(s_device, "wdFencePoolVulkan::Initialize not called");
  s_Fences.PushBack(fence);
}
