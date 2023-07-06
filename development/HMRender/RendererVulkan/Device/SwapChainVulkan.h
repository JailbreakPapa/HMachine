
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

class wdGALDeviceVulkan;

class wdGALSwapChainVulkan : public wdGALWindowSwapChain
{
public:
  virtual void AcquireNextRenderTarget(wdGALDevice* pDevice) override;
  virtual void PresentRenderTarget(wdGALDevice* pDevice) override;
  virtual wdResult UpdateSwapChain(wdGALDevice* pDevice, wdEnum<wdGALPresentMode> newPresentMode) override;

  WD_ALWAYS_INLINE vk::SwapchainKHR GetVulkanSwapChain() const;

protected:
  friend class wdGALDeviceVulkan;
  friend class wdMemoryUtils;

  wdGALSwapChainVulkan(const wdGALWindowSwapChainCreationDescription& Description);

  virtual ~wdGALSwapChainVulkan();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) override;
  wdResult CreateSwapChainInternal();
  void DestroySwapChainInternal(wdGALDeviceVulkan* pVulkanDevice);
  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

protected:
  wdGALDeviceVulkan* m_pVulkanDevice = nullptr;
  wdEnum<wdGALPresentMode> m_currentPresentMode;

  vk::SurfaceKHR m_vulkanSurface;
  vk::SwapchainKHR m_vulkanSwapChain;
  wdHybridArray<vk::Image, 3> m_swapChainImages;
  wdHybridArray<wdGALTextureHandle, 3> m_swapChainTextures;
  wdHybridArray<vk::Fence, 3> m_swapChainImageInUseFences;
  wdUInt32 m_uiCurrentSwapChainImage = 0;

  vk::Semaphore m_currentPipelineImageAvailableSemaphore;
};

#include <RendererVulkan/Device/Implementation/SwapChainVulkan_inl.h>
