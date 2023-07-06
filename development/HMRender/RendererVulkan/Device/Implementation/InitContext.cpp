#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/InitContext.h>
#include <RendererVulkan/Pools/CommandBufferPoolVulkan.h>
#include <RendererVulkan/Pools/StagingBufferPoolVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>
#include <RendererVulkan/Utils/PipelineBarrierVulkan.h>


wdInitContextVulkan::wdInitContextVulkan(wdGALDeviceVulkan* pDevice)
  : m_pDevice(pDevice)
{
  wdProxyAllocator& allocator = m_pDevice->GetAllocator();
  m_pPipelineBarrier = WD_NEW(&allocator, wdPipelineBarrierVulkan);
  m_pCommandBufferPool = WD_NEW(&allocator, wdCommandBufferPoolVulkan);
  m_pCommandBufferPool->Initialize(m_pDevice->GetVulkanDevice(), m_pDevice->GetGraphicsQueue().m_uiQueueFamily);
  m_pStagingBufferPool = WD_NEW(&allocator, wdStagingBufferPoolVulkan);
  m_pStagingBufferPool->Initialize(m_pDevice);
}

wdInitContextVulkan::~wdInitContextVulkan()
{
  m_pCommandBufferPool->DeInitialize();
  m_pStagingBufferPool->DeInitialize();
}

vk::CommandBuffer wdInitContextVulkan::GetFinishedCommandBuffer()
{
  WD_LOCK(m_Lock);
  if (m_currentCommandBuffer)
  {
    m_pPipelineBarrier->Submit();
    vk::CommandBuffer res = m_currentCommandBuffer;
    res.end();

    m_pDevice->ReclaimLater(m_currentCommandBuffer, m_pCommandBufferPool.Borrow());
    return res;
  }
  return nullptr;
}

void wdInitContextVulkan::EnsureCommandBufferExists()
{
  if (!m_currentCommandBuffer)
  {
    // Restart new command buffer if none is active already.
    m_currentCommandBuffer = m_pCommandBufferPool->RequestCommandBuffer();
    vk::CommandBufferBeginInfo beginInfo;
    m_currentCommandBuffer.begin(&beginInfo);
    m_pPipelineBarrier->SetCommandBuffer(&m_currentCommandBuffer);
  }
}

void wdInitContextVulkan::TextureDestroyed(const wdGALTextureVulkan* pTexture)
{
  WD_LOCK(m_Lock);
  m_pPipelineBarrier->TextureDestroyed(pTexture);
}

void wdInitContextVulkan::InitTexture(const wdGALTextureVulkan* pTexture, vk::ImageCreateInfo& createInfo, wdArrayPtr<wdGALSystemMemoryDescription> pInitialData)
{
  WD_LOCK(m_Lock);

  EnsureCommandBufferExists();
  if (pTexture->GetDescription().m_pExisitingNativeObject == nullptr)
  {
    if (pTexture->GetDescription().m_SampleCount != wdGALMSAASampleCount::None)
    {
      // #TODO_VULKAN how do we clear a MS target to zero?
      m_pPipelineBarrier->SetInitialImageState(pTexture, vk::ImageLayout::eUndefined);
      m_pPipelineBarrier->EnsureImageLayout(pTexture, pTexture->GetPreferredLayout(), pTexture->GetUsedByPipelineStage(), pTexture->GetAccessMask(), true);
      return;
    }

    m_pPipelineBarrier->SetInitialImageState(pTexture, createInfo.initialLayout);

    wdDynamicArray<wdUInt8> tempData;
    wdDynamicArray<wdGALSystemMemoryDescription> initialData;
    if (pInitialData.IsEmpty())
    {
      // If we don't have any initial data to initialize the texture to zero memory to match DX11 behavior.
      const vk::Format format = pTexture->GetImageFormat();
      const wdUInt8 uiBlockSize = vk::blockSize(format);
      const auto blockExtent = vk::blockExtent(format);
      {
        // Compute max size of the temp buffer.
        const vk::Extent3D imageExtent = pTexture->GetMipLevelSize(0);
        const VkExtent3D blockCount = {
          (imageExtent.width + blockExtent[0] - 1) / blockExtent[0],
          (imageExtent.height + blockExtent[1] - 1) / blockExtent[1],
          (imageExtent.depth + blockExtent[2] - 1) / blockExtent[2]};
        const wdUInt32 uiTotalSize = uiBlockSize * blockCount.width * blockCount.height * blockCount.depth;
        tempData.SetCount(uiTotalSize, 0);
      }

      for (wdUInt32 uiLayer = 0; uiLayer < createInfo.arrayLayers; uiLayer++)
      {
        for (wdUInt32 uiMipLevel = 0; uiMipLevel < createInfo.mipLevels; uiMipLevel++)
        {
          const wdUInt32 uiSubresourceIndex = uiMipLevel + uiLayer * createInfo.mipLevels;
          WD_ASSERT_DEBUG(initialData.GetCount() == uiSubresourceIndex, "");

          const vk::Extent3D imageExtent = pTexture->GetMipLevelSize(uiMipLevel);
          const VkExtent3D blockCount = {
            (imageExtent.width + blockExtent[0] - 1) / blockExtent[0],
            (imageExtent.height + blockExtent[1] - 1) / blockExtent[1],
            (imageExtent.depth + blockExtent[2] - 1) / blockExtent[2]};

          wdGALSystemMemoryDescription data;
          data.m_pData = tempData.GetData();
          data.m_uiRowPitch = uiBlockSize * blockCount.width;
          data.m_uiSlicePitch = data.m_uiRowPitch * blockCount.height;
          initialData.PushBack(data);
        }
      }
      pInitialData = initialData.GetArrayPtr();
    }

    m_pPipelineBarrier->EnsureImageLayout(pTexture, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite);
    // We need to flush here as all UploadTextureStaging calls will need to barrier on eTransferDstOptimal anyways.
    m_pPipelineBarrier->Flush();
    for (wdUInt32 uiLayer = 0; uiLayer < createInfo.arrayLayers; uiLayer++)
    {
      for (wdUInt32 uiMipLevel = 0; uiMipLevel < createInfo.mipLevels; uiMipLevel++)
      {
        const wdUInt32 uiSubresourceIndex = uiMipLevel + uiLayer * createInfo.mipLevels;
        WD_ASSERT_DEBUG(uiSubresourceIndex < pInitialData.GetCount(), "Not all data provided in the intial texture data.");
        const wdGALSystemMemoryDescription& subResourceData = pInitialData[uiSubresourceIndex];

        vk::ImageSubresourceLayers subresourceLayers;
        // We do not support stencil uploads right now.
        subresourceLayers.aspectMask = wdConversionUtilsVulkan::IsDepthFormat(pTexture->GetImageFormat()) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
        subresourceLayers.mipLevel = uiMipLevel;
        subresourceLayers.baseArrayLayer = uiLayer;
        subresourceLayers.layerCount = 1;

        m_pDevice->UploadTextureStaging(m_pStagingBufferPool.Borrow(), m_pPipelineBarrier.Borrow(), m_currentCommandBuffer, pTexture, subresourceLayers, subResourceData);
      }
    }
  }
  else
  {
    // We don't actually know what the current state is of an existing native object. The only use case right now are back buffers created by swap chains so for now we just throw away the current content and barrier into the preferred layout.
    m_pPipelineBarrier->SetInitialImageState(pTexture, vk::ImageLayout::eUndefined);
    m_pPipelineBarrier->EnsureImageLayout(pTexture, pTexture->GetPreferredLayout(), pTexture->GetUsedByPipelineStage(), pTexture->GetAccessMask(), true);
  }
}
