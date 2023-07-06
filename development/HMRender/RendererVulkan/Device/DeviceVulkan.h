
#pragma once

#include <Foundation/System/PlatformFeatures.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

using wdGALFormatLookupEntryVulkan = wdGALFormatLookupEntry<vk::Format, (vk::Format)0>;
using wdGALFormatLookupTableVulkan = wdGALFormatLookupTable<wdGALFormatLookupEntryVulkan>;

class wdGALBufferVulkan;
class wdGALTextureVulkan;
class wdGALPassVulkan;
class wdPipelineBarrierVulkan;
class wdCommandBufferPoolVulkan;
class wdStagingBufferPoolVulkan;
class wdQueryPoolVulkan;
class wdInitContextVulkan;

/// \brief The Vulkan device implementation of the graphics abstraction layer.
class WD_RENDERERVULKAN_DLL wdGALDeviceVulkan : public wdGALDevice
{
private:
  friend wdInternal::NewInstance<wdGALDevice> CreateVulkanDevice(wdAllocatorBase* pAllocator, const wdGALDeviceCreationDescription& Description);
  wdGALDeviceVulkan(const wdGALDeviceCreationDescription& Description);

public:
  virtual ~wdGALDeviceVulkan();

public:
  struct PendingDeletion
  {
    WD_DECLARE_POD_TYPE();
    vk::ObjectType m_type;
    void* m_pObject;
    union
    {
      wdVulkanAllocation m_allocation;
      void* m_pContext;
    };
  };

  struct ReclaimResource
  {
    WD_DECLARE_POD_TYPE();
    vk::ObjectType m_type;
    void* m_pObject;
    void* m_pContext = nullptr;
  };

  struct Extensions
  {
    bool m_bSurface = false;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    bool m_bWin32Surface = false;
#elif WD_ENABLED(WD_SUPPORTS_GLFW)
#else
#  error "Vulkan Platform not supported"
#endif

    bool m_bDebugUtils = false;
    PFN_vkCreateDebugUtilsMessengerEXT pfn_vkCreateDebugUtilsMessengerEXT = nullptr;
    PFN_vkDestroyDebugUtilsMessengerEXT pfn_vkDestroyDebugUtilsMessengerEXT = nullptr;
    PFN_vkSetDebugUtilsObjectNameEXT pfn_vkSetDebugUtilsObjectNameEXT = nullptr;

    bool m_bDeviceSwapChain = false;
    bool m_bShaderViewportIndexLayer = false;

    vk::PhysicalDeviceCustomBorderColorFeaturesEXT m_borderColorEXT;
    bool m_bBorderColorFloat = false;
  };

  struct Queue
  {
    vk::Queue m_queue;
    wdUInt32 m_uiQueueFamily = -1;
    wdUInt32 m_uiQueueIndex = 0;
  };

  wdUInt64 GetCurrentFrame() const { return m_uiFrameCounter; }
  wdUInt64 GetSafeFrame() const { return m_uiSafeFrame; }

  vk::Instance GetVulkanInstance() const;
  vk::Device GetVulkanDevice() const;
  const Queue& GetGraphicsQueue() const;
  const Queue& GetTransferQueue() const;

  vk::PhysicalDevice GetVulkanPhysicalDevice() const;
  const vk::PhysicalDeviceProperties& GetPhysicalDeviceProperties() const { return m_properties; }
  const Extensions& GetExtensions() const { return m_extensions; }
  vk::PipelineStageFlags GetSupportedStages() const;

  vk::CommandBuffer& GetCurrentCommandBuffer();
  wdPipelineBarrierVulkan& GetCurrentPipelineBarrier();
  wdQueryPoolVulkan& GetQueryPool() const;
  wdStagingBufferPoolVulkan& GetStagingBufferPool() const;
  wdInitContextVulkan& GetInitContext() const;
  wdProxyAllocator& GetAllocator();

  wdGALTextureHandle CreateTextureInternal(const wdGALTextureCreationDescription& Description, wdArrayPtr<wdGALSystemMemoryDescription> pInitialData, vk::Format OverrideFormat, bool bLinearCPU = false);
  wdGALBufferHandle CreateBufferInternal(const wdGALBufferCreationDescription& Description, wdArrayPtr<const wdUInt8> pInitialData, bool bCPU = false);

  const wdGALFormatLookupTableVulkan& GetFormatLookupTable() const;

  wdInt32 GetMemoryIndex(vk::MemoryPropertyFlags properties, const vk::MemoryRequirements& requirements) const;

  vk::Fence Submit(vk::Semaphore waitSemaphore, vk::PipelineStageFlags waitStage, vk::Semaphore signalSemaphore);

  void DeleteLater(const PendingDeletion& deletion);

  template <typename T>
  void DeleteLater(T& object, wdVulkanAllocation& allocation)
  {
    if (object)
    {
      DeleteLater({object.objectType, (void*)object, allocation});
    }
    object = nullptr;
    allocation = nullptr;
  }

  template <typename T>
  void DeleteLater(T& object, void* pContext)
  {
    if (object)
    {
      PendingDeletion del = {object.objectType, (void*)object, nullptr};
      del.m_pContext = pContext;
      DeleteLater(static_cast<const PendingDeletion&>(del));
    }
    object = nullptr;
  }

  template <typename T>
  void DeleteLater(T& object)
  {
    if (object)
    {
      DeleteLater({object.objectType, (void*)object, nullptr});
    }
    object = nullptr;
  }

  void ReclaimLater(const ReclaimResource& reclaim);

  template <typename T>
  void ReclaimLater(T& object, void* pContext = nullptr)
  {
    ReclaimLater({object.objectType, (void*)object, pContext});
    object = nullptr;
  }

  void SetDebugName(const vk::DebugUtilsObjectNameInfoEXT& info, wdVulkanAllocation allocation = nullptr);

  template <typename T>
  void SetDebugName(const char* szName, T& object, wdVulkanAllocation allocation = nullptr)
  {
#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
    if (object)
    {
      vk::DebugUtilsObjectNameInfoEXT nameInfo;
      nameInfo.objectType = object.objectType;
      nameInfo.objectHandle = (uint64_t) static_cast<typename T::NativeType>(object);
      nameInfo.pObjectName = szName;

      SetDebugName(nameInfo, allocation);
    }
#endif
  }

  void ReportLiveGpuObjects();

  static void UploadBufferStaging(wdStagingBufferPoolVulkan* pStagingBufferPool, wdPipelineBarrierVulkan* pPipelineBarrier, vk::CommandBuffer commandBuffer, const wdGALBufferVulkan* pBuffer, wdArrayPtr<const wdUInt8> pInitialData, vk::DeviceSize dstOffset = 0);
  static void UploadTextureStaging(wdStagingBufferPoolVulkan* pStagingBufferPool, wdPipelineBarrierVulkan* pPipelineBarrier, vk::CommandBuffer commandBuffer, const wdGALTextureVulkan* pTexture, const vk::ImageSubresourceLayers& subResource, const wdGALSystemMemoryDescription& data);

  struct OnBeforeImageDestroyedData
  {
    vk::Image image;
    wdGALDeviceVulkan& GALDeviceVulkan;
  };
  wdEvent<OnBeforeImageDestroyedData> OnBeforeImageDestroyed;


  // These functions need to be implemented by a render API abstraction
protected:
  // Init & shutdown functions

  vk::Result SelectInstanceExtensions(wdHybridArray<const char*, 6>& extensions);
  vk::Result SelectDeviceExtensions(vk::DeviceCreateInfo& deviceCreateInfo, wdHybridArray<const char*, 6>& extensions);

  virtual wdResult InitPlatform() override;
  virtual wdResult ShutdownPlatform() override;

  // Pipeline & Pass functions

  virtual void BeginPipelinePlatform(const char* szName, wdGALSwapChain* pSwapChain) override;
  virtual void EndPipelinePlatform(wdGALSwapChain* pSwapChain) override;

  virtual wdGALPass* BeginPassPlatform(const char* szName) override;
  virtual void EndPassPlatform(wdGALPass* pPass) override;


  // State creation functions

  virtual wdGALBlendState* CreateBlendStatePlatform(const wdGALBlendStateCreationDescription& Description) override;
  virtual void DestroyBlendStatePlatform(wdGALBlendState* pBlendState) override;

  virtual wdGALDepthStencilState* CreateDepthStencilStatePlatform(const wdGALDepthStencilStateCreationDescription& Description) override;
  virtual void DestroyDepthStencilStatePlatform(wdGALDepthStencilState* pDepthStencilState) override;

  virtual wdGALRasterizerState* CreateRasterizerStatePlatform(const wdGALRasterizerStateCreationDescription& Description) override;
  virtual void DestroyRasterizerStatePlatform(wdGALRasterizerState* pRasterizerState) override;

  virtual wdGALSamplerState* CreateSamplerStatePlatform(const wdGALSamplerStateCreationDescription& Description) override;
  virtual void DestroySamplerStatePlatform(wdGALSamplerState* pSamplerState) override;


  // Resource creation functions

  virtual wdGALShader* CreateShaderPlatform(const wdGALShaderCreationDescription& Description) override;
  virtual void DestroyShaderPlatform(wdGALShader* pShader) override;

  virtual wdGALBuffer* CreateBufferPlatform(const wdGALBufferCreationDescription& Description, wdArrayPtr<const wdUInt8> pInitialData) override;
  virtual void DestroyBufferPlatform(wdGALBuffer* pBuffer) override;

  virtual wdGALTexture* CreateTexturePlatform(const wdGALTextureCreationDescription& Description, wdArrayPtr<wdGALSystemMemoryDescription> pInitialData) override;
  virtual void DestroyTexturePlatform(wdGALTexture* pTexture) override;

  virtual wdGALResourceView* CreateResourceViewPlatform(wdGALResourceBase* pResource, const wdGALResourceViewCreationDescription& Description) override;
  virtual void DestroyResourceViewPlatform(wdGALResourceView* pResourceView) override;

  virtual wdGALRenderTargetView* CreateRenderTargetViewPlatform(wdGALTexture* pTexture, const wdGALRenderTargetViewCreationDescription& Description) override;
  virtual void DestroyRenderTargetViewPlatform(wdGALRenderTargetView* pRenderTargetView) override;

  wdGALUnorderedAccessView* CreateUnorderedAccessViewPlatform(wdGALResourceBase* pResource, const wdGALUnorderedAccessViewCreationDescription& Description) override;
  virtual void DestroyUnorderedAccessViewPlatform(wdGALUnorderedAccessView* pResource) override;

  // Other rendering creation functions

  virtual wdGALQuery* CreateQueryPlatform(const wdGALQueryCreationDescription& Description) override;
  virtual void DestroyQueryPlatform(wdGALQuery* pQuery) override;

  virtual wdGALVertexDeclaration* CreateVertexDeclarationPlatform(const wdGALVertexDeclarationCreationDescription& Description) override;
  virtual void DestroyVertexDeclarationPlatform(wdGALVertexDeclaration* pVertexDeclaration) override;

  // Timestamp functions

  virtual wdGALTimestampHandle GetTimestampPlatform() override;
  virtual wdResult GetTimestampResultPlatform(wdGALTimestampHandle hTimestamp, wdTime& result) override;

  // Misc functions

  virtual void BeginFramePlatform(const wdUInt64 uiRenderFrame) override;
  virtual void EndFramePlatform() override;

  virtual void FillCapabilitiesPlatform() override;

  virtual void WaitIdlePlatform() override;

  /// \endcond

private:
  struct PerFrameData
  {
    /// \brief These are all fences passed into submit calls. For some reason waiting for the fence of the last submit is not enough. At least I can't get it to work (neither semaphores nor barriers make it past the validation layer).
    wdHybridArray<vk::Fence, 2> m_CommandBufferFences;

    vk::CommandBuffer m_currentCommandBuffer;
    //ID3D11Query* m_pDisjointTimerQuery = nullptr;
    double m_fInvTicksPerSecond = -1.0;
    wdUInt64 m_uiFrame = -1;

    wdMutex m_pendingDeletionsMutex;
    wdDeque<PendingDeletion> m_pendingDeletions;
    wdDeque<PendingDeletion> m_pendingDeletionsPrevious;

    wdMutex m_reclaimResourcesMutex;
    wdDeque<ReclaimResource> m_reclaimResources;
    wdDeque<ReclaimResource> m_reclaimResourcesPrevious;
  };

  void DeletePendingResources(wdDeque<PendingDeletion>& pendingDeletions);
  void ReclaimResources(wdDeque<ReclaimResource>& resources);

  void FillFormatLookupTable();

  wdUInt64 m_uiFrameCounter = 1; ///< We start at 1 so m_uiFrameCounter and m_uiSafeFrame are not equal at the start.
  wdUInt64 m_uiSafeFrame = 0;
  wdUInt8 m_uiCurrentPerFrameData = 0;
  wdUInt8 m_uiNextPerFrameData = 1;

  vk::Instance m_instance;
  vk::PhysicalDevice m_physicalDevice;
  vk::PhysicalDeviceProperties m_properties;
  vk::Device m_device;
  Queue m_graphicsQueue;
  Queue m_transferQueue;

  wdGALFormatLookupTableVulkan m_FormatLookupTable;
  vk::PipelineStageFlags m_supportedStages;
  vk::PhysicalDeviceMemoryProperties m_memoryProperties;

  wdUniquePtr<wdGALPassVulkan> m_pDefaultPass;
  wdUniquePtr<wdPipelineBarrierVulkan> m_pPipelineBarrier;
  wdUniquePtr<wdCommandBufferPoolVulkan> m_pCommandBufferPool;
  wdUniquePtr<wdStagingBufferPoolVulkan> m_pStagingBufferPool;
  wdUniquePtr<wdQueryPoolVulkan> m_pQueryPool;
  wdUniquePtr<wdInitContextVulkan> m_pInitContext;

  // We daisy-chain all command buffers in a frame in sequential order via this semaphore for now.
  vk::Semaphore m_lastCommandBufferFinished;

  PerFrameData m_PerFrameData[4];

#if WD_ENABLED(WD_USE_PROFILING)
  struct GPUTimingScope* m_pFrameTimingScope = nullptr;
  struct GPUTimingScope* m_pPipelineTimingScope = nullptr;
  struct GPUTimingScope* m_pPassTimingScope = nullptr;
#endif

  Extensions m_extensions;
#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
#endif
};

#include <RendererVulkan/Device/Implementation/DeviceVulkan_inl.h>
