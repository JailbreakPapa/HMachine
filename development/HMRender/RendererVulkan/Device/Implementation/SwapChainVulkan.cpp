#include <RendererVulkan/RendererVulkanPCH.h>

#include <Core/System/Window.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/System/PlatformFeatures.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/SwapChainVulkan.h>
#include <RendererVulkan/Pools/SemaphorePoolVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>
#include <RendererVulkan/Utils/PipelineBarrierVulkan.h>

#if WD_ENABLED(WD_SUPPORTS_GLFW)
#  include <GLFW/glfw3.h>
#endif

#if WD_ENABLED(WD_PLATFORM_LINUX)
#  include <xcb/xcb.h>
#endif

namespace
{
  wdResult GetAlternativeFormat(vk::Format& format, vk::ComponentMapping& componentMapping)
  {
    switch (format)
    {
      case vk::Format::eR8G8B8A8Srgb:
        format = vk::Format::eB8G8R8A8Srgb;
        componentMapping = vk::ComponentMapping{vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eA};
        return WD_SUCCESS;
      case vk::Format::eR8G8B8A8Unorm:
        format = vk::Format::eB8G8R8A8Unorm;
        componentMapping = vk::ComponentMapping{vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eA};
        return WD_SUCCESS;
      default:
        return WD_FAILURE;
    }
  }

  wdGALResourceFormat::Enum GetResourceFormat(vk::Format& format)
  {
    switch (format)
    {
      case vk::Format::eR8G8B8A8Srgb:
        return wdGALResourceFormat::RGBAUByteNormalizedsRGB;
      case vk::Format::eR8G8B8A8Unorm:
        return wdGALResourceFormat::RGBAUByteNormalized;
      case vk::Format::eB8G8R8A8Srgb:
        return wdGALResourceFormat::BGRAUByteNormalizedsRGB;
      case vk::Format::eB8G8R8A8Unorm:
        return wdGALResourceFormat::BGRAUByteNormalized;
      default:
        return wdGALResourceFormat::ENUM_COUNT;
    }
  }
} // namespace

void wdGALSwapChainVulkan::AcquireNextRenderTarget(wdGALDevice* pDevice)
{
  WD_PROFILE_SCOPE("AcquireNextRenderTarget");

  WD_ASSERT_DEV(!m_currentPipelineImageAvailableSemaphore, "Pipeline semaphores leaked");
  m_currentPipelineImageAvailableSemaphore = wdSemaphorePoolVulkan::RequestSemaphore();

  if (m_swapChainImageInUseFences[m_uiCurrentSwapChainImage])
  {
    //#TODO_VULKAN waiting for fence does not seem to be necessary, is it already done by acquireNextImageKHR?
    // m_pVulkanDevice->GetVulkanDevice().waitForFences(1, &m_swapChainImageInUseFences[m_uiCurrentSwapChainImage], true, 1000000000ui64);
    m_swapChainImageInUseFences[m_uiCurrentSwapChainImage] = nullptr;
  }

  vk::Result result = m_pVulkanDevice->GetVulkanDevice().acquireNextImageKHR(m_vulkanSwapChain, std::numeric_limits<uint64_t>::max(), m_currentPipelineImageAvailableSemaphore, nullptr, &m_uiCurrentSwapChainImage);
  if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
  {
    wdLog::Warning("Swap-chain does not match the target window size and should be recreated.");
  }

#ifdef VK_LOG_LAYOUT_CHANGES
  wdLog::Warning("AcquireNextRenderTarget {}", wdArgP(static_cast<void*>(m_swapChainImages[m_uiCurrentSwapChainImage])));
#endif

  m_RenderTargets.m_hRTs[0] = m_swapChainTextures[m_uiCurrentSwapChainImage];
}

void wdGALSwapChainVulkan::PresentRenderTarget(wdGALDevice* pDevice)
{
  WD_PROFILE_SCOPE("PresentRenderTarget");

  auto pVulkanDevice = static_cast<wdGALDeviceVulkan*>(pDevice);
  {
    auto view = wdGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(m_RenderTargets.m_hRTs[0]);
    const wdGALTextureVulkan* pTexture = static_cast<const wdGALTextureVulkan*>(pVulkanDevice->GetTexture(m_swapChainTextures[m_uiCurrentSwapChainImage]));
    // Move image into ePresentSrcKHR layout.
    pVulkanDevice->GetCurrentPipelineBarrier().EnsureImageLayout(pTexture, pTexture->GetFullRange(), vk::ImageLayout::ePresentSrcKHR, vk::PipelineStageFlagBits::eBottomOfPipe, {});
  }

  // Submit command buffer
  vk::Semaphore currentPipelineRenderFinishedSemaphore = wdSemaphorePoolVulkan::RequestSemaphore();
  vk::Fence renderFence = pVulkanDevice->Submit(m_currentPipelineImageAvailableSemaphore, vk::PipelineStageFlagBits::eColorAttachmentOutput, currentPipelineRenderFinishedSemaphore);
  pVulkanDevice->ReclaimLater(m_currentPipelineImageAvailableSemaphore);

  {
    // Present image
    vk::PresentInfoKHR presentInfo;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &currentPipelineRenderFinishedSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_vulkanSwapChain;
    presentInfo.pImageIndices = &m_uiCurrentSwapChainImage;

    m_pVulkanDevice->GetGraphicsQueue().m_queue.presentKHR(&presentInfo);

#ifdef VK_LOG_LAYOUT_CHANGES
    wdLog::Warning("PresentInfoKHR {}", wdArgP(m_swapChainImages[m_uiCurrentSwapChainImage]));
#endif

    m_swapChainImageInUseFences[m_uiCurrentSwapChainImage] = renderFence;
  }

  pVulkanDevice->ReclaimLater(currentPipelineRenderFinishedSemaphore);
}

wdResult wdGALSwapChainVulkan::UpdateSwapChain(wdGALDevice* pDevice, wdEnum<wdGALPresentMode> newPresentMode)
{
  WD_ASSERT_DEBUG(!m_currentPipelineImageAvailableSemaphore, "UpdateSwapChain must not be called between AcquireNextRenderTarget and PresentRenderTarget.");
  m_currentPresentMode = newPresentMode;
  return CreateSwapChainInternal();
}

wdGALSwapChainVulkan::wdGALSwapChainVulkan(const wdGALWindowSwapChainCreationDescription& Description)
  : wdGALWindowSwapChain(Description)
  , m_vulkanSwapChain(nullptr)
{
}

wdGALSwapChainVulkan::~wdGALSwapChainVulkan() {}

wdResult wdGALSwapChainVulkan::InitPlatform(wdGALDevice* pDevice)
{
  m_pVulkanDevice = static_cast<wdGALDeviceVulkan*>(pDevice);
  m_currentPresentMode = m_WindowDesc.m_InitialPresentMode;

#if defined(VK_USE_PLATFORM_WIN32_KHR)
  vk::Win32SurfaceCreateInfoKHR surfaceCreateInfo = {};
  surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
  surfaceCreateInfo.hwnd = (HWND)m_WindowDesc.m_pWindow->GetNativeWindowHandle();

  m_vulkanSurface = m_pVulkanDevice->GetVulkanInstance().createWin32SurfaceKHR(surfaceCreateInfo);
#elif WD_ENABLED(WD_PLATFORM_LINUX)
  wdWindowHandle windowHandle = m_WindowDesc.m_pWindow->GetNativeWindowHandle();
  switch (windowHandle.type)
  {
    case wdWindowHandle::Type::Invalid:
      wdLog::Error("Invalid native window handle for window \"{}\"", m_WindowDesc.m_pWindow);
      return WD_FAILURE;
    case wdWindowHandle::Type::GLFW:
    {
      VkSurfaceKHR glfwSurface = VK_NULL_HANDLE;
      VK_SUCCEED_OR_RETURN_WD_FAILURE(glfwCreateWindowSurface(m_pVulkanDevice->GetVulkanInstance(), windowHandle.glfwWindow, nullptr, &glfwSurface));
      m_vulkanSurface = glfwSurface;
    }
    break;
    case wdWindowHandle::Type::XCB:
    {
      vk::XcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
      WD_ASSERT_DEV(windowHandle.xcbWindow.m_pConnection != nullptr && windowHandle.xcbWindow.m_Window != 0, "Invalid xcb handle");
      surfaceCreateInfo.connection = windowHandle.xcbWindow.m_pConnection;
      surfaceCreateInfo.window = windowHandle.xcbWindow.m_Window;

      m_vulkanSurface = m_pVulkanDevice->GetVulkanInstance().createXcbSurfaceKHR(surfaceCreateInfo);
    }
    break;
  }
#elif WD_ENABLED(WD_SUPPORTS_GLFW)
  VkSurfaceKHR glfwSurface = VK_NULL_HANDLE;
  VK_SUCCEED_OR_RETURN_WD_FAILURE(glfwCreateWindowSurface(m_pVulkanDevice->GetVulkanInstance(), m_WindowDesc.m_pWindow->GetNativeWindowHandle(), nullptr, &glfwSurface));
  m_vulkanSurface = glfwSurface;
#else
#  error Platform not supported
#endif

  if (!m_vulkanSurface)
  {
    wdLog::Error("Failed to create Vulkan surface for window \"{}\"", m_WindowDesc.m_pWindow);
    return WD_FAILURE;
  }

  // We have created a surface on a window, the window must not be destroyed while the surface is still alive.
  m_WindowDesc.m_pWindow->AddReference();
  vk::Bool32 surfaceSupported = false;

  VK_SUCCEED_OR_RETURN_WD_FAILURE(m_pVulkanDevice->GetVulkanPhysicalDevice().getSurfaceSupportKHR(m_pVulkanDevice->GetGraphicsQueue().m_uiQueueFamily, m_vulkanSurface, &surfaceSupported));

  if (!surfaceSupported)
  {
    wdLog::Error("Vulkan device does not support surfaces");
    m_pVulkanDevice->DeleteLater(m_vulkanSurface);
    return WD_FAILURE;
  }

  return CreateSwapChainInternal();
}

wdResult wdGALSwapChainVulkan::CreateSwapChainInternal()
{
  uint32_t uiPresentModes = 0;
  VK_SUCCEED_OR_RETURN_WD_FAILURE(m_pVulkanDevice->GetVulkanPhysicalDevice().getSurfacePresentModesKHR(m_vulkanSurface, &uiPresentModes, nullptr));
  wdHybridArray<vk::PresentModeKHR, 4> presentModes;
  presentModes.SetCount(uiPresentModes);
  VK_SUCCEED_OR_RETURN_WD_FAILURE(m_pVulkanDevice->GetVulkanPhysicalDevice().getSurfacePresentModesKHR(m_vulkanSurface, &uiPresentModes, presentModes.GetData()));

  const vk::SurfaceCapabilitiesKHR surfaceCapabilities = m_pVulkanDevice->GetVulkanPhysicalDevice().getSurfaceCapabilitiesKHR(m_vulkanSurface);

  uint32_t uiNumSurfaceFormats = 0;
  VK_SUCCEED_OR_RETURN_WD_FAILURE(m_pVulkanDevice->GetVulkanPhysicalDevice().getSurfaceFormatsKHR(m_vulkanSurface, &uiNumSurfaceFormats, nullptr));
  std::vector<vk::SurfaceFormatKHR> supportedFormats;
  supportedFormats.resize(uiNumSurfaceFormats);
  VK_SUCCEED_OR_RETURN_WD_FAILURE(m_pVulkanDevice->GetVulkanPhysicalDevice().getSurfaceFormatsKHR(m_vulkanSurface, &uiNumSurfaceFormats, supportedFormats.data()));

  vk::Format desiredFormat = m_pVulkanDevice->GetFormatLookupTable().GetFormatInfo(m_WindowDesc.m_BackBufferFormat).m_eRenderTarget;
  vk::ColorSpaceKHR desiredColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
  vk::ComponentMapping backBufferComponentMapping;

  bool formatFound = false;
  for (vk::SurfaceFormatKHR& supportedFormat : supportedFormats)
  {
    if (supportedFormat.format == desiredFormat && supportedFormat.colorSpace == desiredColorSpace)
    {
      formatFound = true;
      break;
    }
  }

  if (!formatFound && GetAlternativeFormat(desiredFormat, backBufferComponentMapping).Succeeded())
  {
    for (vk::SurfaceFormatKHR& supportedFormat : supportedFormats)
    {
      if (supportedFormat.format == desiredFormat && supportedFormat.colorSpace == desiredColorSpace)
      {
        formatFound = true;
        break;
      }
    }
  }

  if (!formatFound)
  {
    wdStringBuilder backBufferFormatNice = "<unknown>";
    wdReflectionUtils::EnumerationToString(wdGetStaticRTTI<wdGALResourceFormat>(), m_WindowDesc.m_BackBufferFormat, backBufferFormatNice);
    wdLog::Error("The requested back buffer format {} mapping to the vulkan format {} is not supported on this system.", backBufferFormatNice, vk::to_string(desiredFormat).c_str());
    wdLog::Info("Available formats are:");
    for (vk::SurfaceFormatKHR& supportedFormat : supportedFormats)
    {
      wdLog::Info("  format: {}  color space: {}", vk::to_string(supportedFormat.format).c_str(), vk::to_string(supportedFormat.colorSpace).c_str());
    }
    return WD_FAILURE;
  }

  // Does the device support RGBA textures or only BRGA?
  // Do we need to store somewhere if the texture is swizzeled?

  vk::SwapchainCreateInfoKHR swapChainCreateInfo = {};
  swapChainCreateInfo.clipped = VK_FALSE;
  swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  swapChainCreateInfo.imageArrayLayers = 1;
  swapChainCreateInfo.imageColorSpace = desiredColorSpace;
  swapChainCreateInfo.imageExtent.width = m_WindowDesc.m_pWindow->GetClientAreaSize().width;
  swapChainCreateInfo.imageExtent.height = m_WindowDesc.m_pWindow->GetClientAreaSize().height;
  swapChainCreateInfo.imageExtent.width = wdMath::Clamp(swapChainCreateInfo.imageExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
  swapChainCreateInfo.imageExtent.height = wdMath::Clamp(swapChainCreateInfo.imageExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
  swapChainCreateInfo.imageFormat = desiredFormat;

  swapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst; // We need eTransferDst to be able to resolve msaa textures into the backbuffer.
  if (m_WindowDesc.m_bAllowScreenshots)
    swapChainCreateInfo.imageUsage |= vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled;

  swapChainCreateInfo.minImageCount = wdMath::Max(m_WindowDesc.m_bDoubleBuffered ? 2u : 1u, surfaceCapabilities.minImageCount);
  if (surfaceCapabilities.maxImageCount != 0)
    swapChainCreateInfo.minImageCount = wdMath::Min(swapChainCreateInfo.minImageCount, surfaceCapabilities.maxImageCount);

  swapChainCreateInfo.presentMode = wdConversionUtilsVulkan::GetPresentMode(m_currentPresentMode, presentModes);
  swapChainCreateInfo.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
  swapChainCreateInfo.surface = m_vulkanSurface;

  swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
  swapChainCreateInfo.pQueueFamilyIndices = nullptr;
  swapChainCreateInfo.queueFamilyIndexCount = 0;

  // We must pass in the old swap chain or NVidia will crash.
  swapChainCreateInfo.oldSwapchain = m_vulkanSwapChain;
  DestroySwapChainInternal(m_pVulkanDevice);

  m_vulkanSwapChain = m_pVulkanDevice->GetVulkanDevice().createSwapchainKHR(swapChainCreateInfo);

  if (!m_vulkanSwapChain)
  {
    wdLog::Error("Failed to create Vulkan swap chain!");
    return WD_FAILURE;
  }

  wdUInt32 uiSwapChainImages = 0;
  VK_SUCCEED_OR_RETURN_WD_FAILURE(m_pVulkanDevice->GetVulkanDevice().getSwapchainImagesKHR(m_vulkanSwapChain, &uiSwapChainImages, nullptr));
  m_swapChainImages.SetCount(uiSwapChainImages);
  VK_SUCCEED_OR_RETURN_WD_FAILURE(m_pVulkanDevice->GetVulkanDevice().getSwapchainImagesKHR(m_vulkanSwapChain, &uiSwapChainImages, m_swapChainImages.GetData()));

  WD_ASSERT_DEV(uiSwapChainImages < 4, "If we have more than 3 swap chain images we can't hold ontp fences owned by wdDeviceVulkan::PerFrameData anymore as that reclaims all fences once it reuses the frame data (which is 4 right now). Thus, we can't safely pass in the fence in wdGALSwapChainVulkan::PresentRenderTarget as it will be reclaimed before we use it.");
  m_swapChainImageInUseFences.SetCount(uiSwapChainImages);

  for (wdUInt32 i = 0; i < uiSwapChainImages; i++)
  {
    m_pVulkanDevice->SetDebugName("SwapChainImage", m_swapChainImages[i]);

    wdGALTextureCreationDescription TexDesc;
    TexDesc.m_Format = GetResourceFormat(desiredFormat);
    TexDesc.m_uiWidth = swapChainCreateInfo.imageExtent.width;
    TexDesc.m_uiHeight = swapChainCreateInfo.imageExtent.height;
    TexDesc.m_SampleCount = m_WindowDesc.m_SampleCount;
    TexDesc.m_pExisitingNativeObject = m_swapChainImages[i];
    TexDesc.m_bAllowShaderResourceView = false;
    TexDesc.m_bCreateRenderTarget = true;
    TexDesc.m_ResourceAccess.m_bImmutable = true;
    TexDesc.m_ResourceAccess.m_bReadBack = m_WindowDesc.m_bAllowScreenshots;
    m_swapChainTextures.PushBack(m_pVulkanDevice->CreateTextureInternal(TexDesc, wdArrayPtr<wdGALSystemMemoryDescription>(), desiredFormat));
  }
  m_CurrentSize = wdSizeU32(swapChainCreateInfo.imageExtent.width, swapChainCreateInfo.imageExtent.height);
  m_RenderTargets.m_hRTs[0] = m_swapChainTextures[0];
  return WD_SUCCESS;
}

void wdGALSwapChainVulkan::DestroySwapChainInternal(wdGALDeviceVulkan* pVulkanDevice)
{
  wdUInt32 uiSwapChainImages = m_swapChainTextures.GetCount();
  for (wdUInt32 i = 0; i < uiSwapChainImages; i++)
  {
    pVulkanDevice->DestroyTexture(m_swapChainTextures[i]);
  }
  m_swapChainTextures.Clear();

  if (m_vulkanSwapChain)
  {
    pVulkanDevice->DeleteLater(m_vulkanSwapChain);
  }
}

wdResult wdGALSwapChainVulkan::DeInitPlatform(wdGALDevice* pDevice)
{
  wdGALDeviceVulkan* pVulkanDevice = static_cast<wdGALDeviceVulkan*>(pDevice);
  DestroySwapChainInternal(pVulkanDevice);
  if (m_vulkanSurface)
  {
    pVulkanDevice->DeleteLater(m_vulkanSurface, (void*)m_WindowDesc.m_pWindow);
  }
  return WD_SUCCESS;
}

WD_STATICLINK_FILE(RendererVulkan, RendererVulkan_Device_Implementation_SwapChainVulkan);
