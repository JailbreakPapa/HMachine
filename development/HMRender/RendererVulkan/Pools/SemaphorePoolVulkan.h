#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

/// \brief Simple pool for semaphores
///
/// Do not call ReclaimSemaphore manually, instead call wdGALDeviceVulkan::ReclaimLater which will make sure to reclaim the semaphore once it is no longer in use.
/// Usage:
/// \code{.cpp}
///   vk::Semaphore s = wdSemaphorePoolVulkan::RequestSemaphore();
///   ...
///   wdGALDeviceVulkan* pDevice = ...;
///   pDevice->ReclaimLater(s);
/// \endcode
class WD_RENDERERVULKAN_DLL wdSemaphorePoolVulkan
{
public:
  static void Initialize(vk::Device device);
  static void DeInitialize();

  static vk::Semaphore RequestSemaphore();
  static void ReclaimSemaphore(vk::Semaphore& semaphore);

private:
  static wdHybridArray<vk::Semaphore, 4> s_semaphores;
  static vk::Device s_device;
};
