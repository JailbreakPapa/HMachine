
#pragma once

#include <Foundation/Types/UniquePtr.h>

class wdGALDeviceVulkan;
class wdPipelineBarrierVulkan;
class wdCommandBufferPoolVulkan;
class wdStagingBufferPoolVulkan;

/// \brief Thread-safe context for initializing resources. Records a command buffer that transitions all newly created resources into their initial state.
class wdInitContextVulkan
{
public:
  wdInitContextVulkan(wdGALDeviceVulkan* pDevice);
  ~wdInitContextVulkan();

  /// \brief Returns a finished command buffer of all background loading up to this point.
  ///    The command buffer is already ended and marked to be reclaimed so the only thing done on it should be to submit it.
  vk::CommandBuffer GetFinishedCommandBuffer();

  /// \brief Initializes a texture and moves it into its default state.
  /// \param pTexture The texture to initialize.
  /// \param createInfo The image creation info for the texture. Needed for initial state information.
  /// \param pInitialData The initial data of the texture. If not set, the initial content will be undefined.
  void InitTexture(const wdGALTextureVulkan* pTexture, vk::ImageCreateInfo& createInfo, wdArrayPtr<wdGALSystemMemoryDescription> pInitialData);

  /// \brief Needs to be called by the wdGALDeviceVulkan just before a texture is destroyed to clean up stale barriers.
  void TextureDestroyed(const wdGALTextureVulkan* pTexture);

private:
  void EnsureCommandBufferExists();

  wdGALDeviceVulkan* m_pDevice = nullptr;

  wdMutex m_Lock;
  vk::CommandBuffer m_currentCommandBuffer;
  wdUniquePtr<wdPipelineBarrierVulkan> m_pPipelineBarrier;
  wdUniquePtr<wdCommandBufferPoolVulkan> m_pCommandBufferPool;
  wdUniquePtr<wdStagingBufferPoolVulkan> m_pStagingBufferPool;
};
