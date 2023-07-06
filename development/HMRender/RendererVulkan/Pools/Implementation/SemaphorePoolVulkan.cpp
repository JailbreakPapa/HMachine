#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Pools/SemaphorePoolVulkan.h>

vk::Device wdSemaphorePoolVulkan::s_device;
wdHybridArray<vk::Semaphore, 4> wdSemaphorePoolVulkan::s_semaphores;

void wdSemaphorePoolVulkan::Initialize(vk::Device device)
{
  s_device = device;
}

void wdSemaphorePoolVulkan::DeInitialize()
{
  for (vk::Semaphore& semaphore : s_semaphores)
  {
    s_device.destroySemaphore(semaphore, nullptr);
  }
  s_semaphores.Clear();
  s_semaphores.Compact();

  s_device = nullptr;
}

vk::Semaphore wdSemaphorePoolVulkan::RequestSemaphore()
{
  WD_ASSERT_DEBUG(s_device, "wdSemaphorePoolVulkan::Initialize not called");
  if (!s_semaphores.IsEmpty())
  {
    vk::Semaphore semaphore = s_semaphores.PeekBack();
    s_semaphores.PopBack();
    return semaphore;
  }
  else
  {
    vk::Semaphore semaphore;
    vk::SemaphoreCreateInfo semaphoreCreateInfo;
    VK_ASSERT_DEV(s_device.createSemaphore(&semaphoreCreateInfo, nullptr, &semaphore));
    return semaphore;
  }
}

void wdSemaphorePoolVulkan::ReclaimSemaphore(vk::Semaphore& semaphore)
{
  WD_ASSERT_DEBUG(s_device, "wdSemaphorePoolVulkan::Initialize not called");
  s_semaphores.PushBack(semaphore);
}
