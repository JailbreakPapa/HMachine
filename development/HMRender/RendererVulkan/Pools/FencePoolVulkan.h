#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

/// \brief Simple pool for fences
///
/// Do not call ReclaimFence manually, instead call wdGALDeviceVulkan::ReclaimLater which will make sure to reclaim the fence once it is no longer in use.
/// Fences are reclaimed once the frame in wdGALDeviceVulkan is reused (currently 4 frames are in rotation). Do not call resetFences, this is already done by ReclaimFence.
/// Usage:
/// \code{.cpp}
///   vk::Fence f = wdFencePoolVulkan::RequestFence();
///   <insert fence somewhere>
///   <wait for fence>
///   wdGALDeviceVulkan* pDevice = ...;
///   pDevice->ReclaimLater(f);
/// \endcode
class WD_RENDERERVULKAN_DLL wdFencePoolVulkan
{
public:
  static void Initialize(vk::Device device);
  static void DeInitialize();

  static vk::Fence RequestFence();
  static void ReclaimFence(vk::Fence& fence);

private:
  static wdHybridArray<vk::Fence, 4> s_Fences;
  static vk::Device s_device;
};
