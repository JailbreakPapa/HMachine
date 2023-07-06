#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/CommandEncoder/CommandEncoderImplVulkan.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Pools/DescriptorSetPoolVulkan.h>
#include <RendererVulkan/Pools/QueryPoolVulkan.h>
#include <RendererVulkan/Pools/StagingBufferPoolVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/FallbackResourcesVulkan.h>
#include <RendererVulkan/Resources/QueryVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>
#include <RendererVulkan/Resources/ResourceViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Resources/UnorderedAccessViewVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>
#include <RendererVulkan/Utils/ImageCopyVulkan.h>
#include <RendererVulkan/Utils/PipelineBarrierVulkan.h>

wdGALCommandEncoderImplVulkan::wdGALCommandEncoderImplVulkan(wdGALDeviceVulkan& device)
  : m_GALDeviceVulkan(device)
{
  m_vkDevice = device.GetVulkanDevice();
}

wdGALCommandEncoderImplVulkan::~wdGALCommandEncoderImplVulkan() = default;

void wdGALCommandEncoderImplVulkan::Reset()
{
  WD_ASSERT_DEBUG(!m_bRenderPassActive, "Render pass was not closed");

  m_bPipelineStateDirty = true;
  m_bViewportDirty = true;
  m_bIndexBufferDirty = true;
  m_bDescriptorsDirty = true;
  m_BoundVertexBuffersRange.Reset();

  m_LayoutDesc = {};
  m_PipelineDesc = {};
  m_frameBuffer = nullptr;

  m_viewport = vk::Viewport();
  m_scissor = vk::Rect2D();

  for (wdUInt32 i = 0; i < WD_GAL_MAX_RENDERTARGET_COUNT; i++)
  {
    m_pBoundRenderTargets[i] = nullptr;
  }
  m_pBoundDepthStencilTarget = nullptr;
  m_uiBoundRenderTargetCount = 0;

  m_pIndexBuffer = nullptr;
  for (wdUInt32 i = 0; i < WD_GAL_MAX_VERTEX_BUFFER_COUNT; i++)
  {
    m_pBoundVertexBuffers[i] = nullptr;
    m_VertexBufferOffsets[i] = 0;
  }

  for (wdUInt32 i = 0; i < WD_GAL_MAX_CONSTANT_BUFFER_COUNT; i++)
  {
    m_pBoundConstantBuffers[i] = nullptr;
  }
  for (wdUInt32 i = 0; i < wdGALShaderStage::ENUM_COUNT; i++)
  {
    m_pBoundShaderResourceViews[i].Clear();
  }
  m_pBoundUnoderedAccessViews.Clear();

  wdMemoryUtils::ZeroFill(&m_pBoundSamplerStates[0][0], wdGALShaderStage::ENUM_COUNT * WD_GAL_MAX_SAMPLER_COUNT);

  m_renderPass = vk::RenderPassBeginInfo();
  m_clearValues.Clear();
}

void wdGALCommandEncoderImplVulkan::MarkDirty()
{
  WD_ASSERT_DEBUG(!m_bRenderPassActive, "Render pass was not closed");

  m_bPipelineStateDirty = true;
  m_bViewportDirty = true;
  m_bIndexBufferDirty = true;
  m_bDescriptorsDirty = true;
  m_BoundVertexBuffersRange.Reset();
  for (wdUInt32 i = 0; i < WD_GAL_MAX_VERTEX_BUFFER_COUNT; i++)
  {
    if (m_pBoundVertexBuffers[i])
      m_BoundVertexBuffersRange.SetToIncludeValue(i);
  }
}

void wdGALCommandEncoderImplVulkan::SetCurrentCommandBuffer(vk::CommandBuffer* commandBuffer, wdPipelineBarrierVulkan* pipelineBarrier)
{
  m_pCommandBuffer = commandBuffer;
  m_pPipelineBarrier = pipelineBarrier;
}

// State setting functions

void wdGALCommandEncoderImplVulkan::SetShaderPlatform(const wdGALShader* pShader)
{
  if (pShader != nullptr)
  {
    m_PipelineDesc.m_pCurrentShader = static_cast<const wdGALShaderVulkan*>(pShader);
    m_ComputeDesc.m_pCurrentShader = m_PipelineDesc.m_pCurrentShader;
    m_bPipelineStateDirty = true;
  }
}

void wdGALCommandEncoderImplVulkan::SetConstantBufferPlatform(wdUInt32 uiSlot, const wdGALBuffer* pBuffer)
{
  // \todo Check if the device supports the slot index?
  m_pBoundConstantBuffers[uiSlot] = pBuffer != nullptr ? static_cast<const wdGALBufferVulkan*>(pBuffer) : nullptr;
  m_bDescriptorsDirty = true;
}

void wdGALCommandEncoderImplVulkan::SetSamplerStatePlatform(wdGALShaderStage::Enum Stage, wdUInt32 uiSlot, const wdGALSamplerState* pSamplerState)
{
  // \todo Check if the device supports the stage / the slot index
  m_pBoundSamplerStates[Stage][uiSlot] = pSamplerState != nullptr ? static_cast<const wdGALSamplerStateVulkan*>(pSamplerState) : nullptr;
  m_bDescriptorsDirty = true;
}

void wdGALCommandEncoderImplVulkan::SetResourceViewPlatform(wdGALShaderStage::Enum Stage, wdUInt32 uiSlot, const wdGALResourceView* pResourceView)
{
  auto& boundShaderResourceViews = m_pBoundShaderResourceViews[Stage];
  boundShaderResourceViews.EnsureCount(uiSlot + 1);
  boundShaderResourceViews[uiSlot] = pResourceView != nullptr ? static_cast<const wdGALResourceViewVulkan*>(pResourceView) : nullptr;
  m_bDescriptorsDirty = true;
}

void wdGALCommandEncoderImplVulkan::SetUnorderedAccessViewPlatform(wdUInt32 uiSlot, const wdGALUnorderedAccessView* pUnorderedAccessView)
{
  m_pBoundUnoderedAccessViews.EnsureCount(uiSlot + 1);
  m_pBoundUnoderedAccessViews[uiSlot] = pUnorderedAccessView != nullptr ? static_cast<const wdGALUnorderedAccessViewVulkan*>(pUnorderedAccessView) : nullptr;
  m_bDescriptorsDirty = true;
}

// Query functions

void wdGALCommandEncoderImplVulkan::BeginQueryPlatform(const wdGALQuery* pQuery)
{
  auto pVulkanQuery = static_cast<const wdGALQueryVulkan*>(pQuery);

  // TODO how to decide the query type etc in Vulkan?

  m_pCommandBuffer->beginQuery(pVulkanQuery->GetPool(), pVulkanQuery->GetID(), {});
}

void wdGALCommandEncoderImplVulkan::EndQueryPlatform(const wdGALQuery* pQuery)
{
  auto pVulkanQuery = static_cast<const wdGALQueryVulkan*>(pQuery);

  m_pCommandBuffer->endQuery(pVulkanQuery->GetPool(), pVulkanQuery->GetID());
}

wdResult wdGALCommandEncoderImplVulkan::GetQueryResultPlatform(const wdGALQuery* pQuery, wdUInt64& uiQueryResult)
{
  auto pVulkanQuery = static_cast<const wdGALQueryVulkan*>(pQuery);
  vk::Result result = m_vkDevice.getQueryPoolResults(pVulkanQuery->GetPool(), pVulkanQuery->GetID(), 1u, sizeof(wdUInt64), &uiQueryResult, 0, vk::QueryResultFlagBits::e64);

  return result == vk::Result::eSuccess ? WD_SUCCESS : WD_FAILURE;
}

void wdGALCommandEncoderImplVulkan::InsertTimestampPlatform(wdGALTimestampHandle hTimestamp)
{
  m_GALDeviceVulkan.GetQueryPool().InsertTimestamp(m_GALDeviceVulkan.GetCurrentCommandBuffer(), hTimestamp);
}

// Resource update functions

void wdGALCommandEncoderImplVulkan::ClearUnorderedAccessViewPlatform(const wdGALUnorderedAccessView* pUnorderedAccessView, wdVec4 clearValues)
{
  // this looks to require custom code, either using buffer copies or
  // clearing via a compute shader

  WD_ASSERT_NOT_IMPLEMENTED;
}

void wdGALCommandEncoderImplVulkan::ClearUnorderedAccessViewPlatform(const wdGALUnorderedAccessView* pUnorderedAccessView, wdVec4U32 clearValues)
{
  // Same as the other clearing variant

  WD_ASSERT_NOT_IMPLEMENTED;
}

void wdGALCommandEncoderImplVulkan::CopyBufferPlatform(const wdGALBuffer* pDestination, const wdGALBuffer* pSource)
{
  auto pDestinationVulkan = static_cast<const wdGALBufferVulkan*>(pDestination);
  auto pSourceVulkan = static_cast<const wdGALBufferVulkan*>(pSource);

  WD_ASSERT_DEV(pSource->GetSize() != pDestination->GetSize(), "Source and destination buffer sizes mismatch!");

  // TODO do this in an immediate command buffer?
  vk::BufferCopy bufferCopy = {};
  bufferCopy.size = pSource->GetSize();

  //#TODO_VULKAN better barrier management of buffers.
  m_pPipelineBarrier->Flush();

  m_pCommandBuffer->copyBuffer(pSourceVulkan->GetVkBuffer(), pDestinationVulkan->GetVkBuffer(), 1, &bufferCopy);

  m_pPipelineBarrier->AccessBuffer(pSourceVulkan, 0, bufferCopy.size, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferRead, pDestinationVulkan->GetUsedByPipelineStage(), pDestinationVulkan->GetAccessMask());
  m_pPipelineBarrier->AccessBuffer(pDestinationVulkan, 0, bufferCopy.size, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, pDestinationVulkan->GetUsedByPipelineStage(), pDestinationVulkan->GetAccessMask());
}

void wdGALCommandEncoderImplVulkan::CopyBufferRegionPlatform(const wdGALBuffer* pDestination, wdUInt32 uiDestOffset, const wdGALBuffer* pSource,
  wdUInt32 uiSourceOffset, wdUInt32 uiByteCount)
{
  auto pDestinationVulkan = static_cast<const wdGALBufferVulkan*>(pDestination);
  auto pSourceVulkan = static_cast<const wdGALBufferVulkan*>(pSource);

  vk::BufferCopy bufferCopy = {};
  bufferCopy.dstOffset = uiDestOffset;
  bufferCopy.srcOffset = uiSourceOffset;
  bufferCopy.size = uiByteCount;

  //#TODO_VULKAN better barrier management of buffers.
  m_pPipelineBarrier->Flush();

  m_pCommandBuffer->copyBuffer(pSourceVulkan->GetVkBuffer(), pDestinationVulkan->GetVkBuffer(), 1, &bufferCopy);

  m_pPipelineBarrier->AccessBuffer(pSourceVulkan, 0, bufferCopy.size, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferRead, pDestinationVulkan->GetUsedByPipelineStage(), pDestinationVulkan->GetAccessMask());
  m_pPipelineBarrier->AccessBuffer(pDestinationVulkan, 0, bufferCopy.size, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, pDestinationVulkan->GetUsedByPipelineStage(), pDestinationVulkan->GetAccessMask());
}

void wdGALCommandEncoderImplVulkan::UpdateBufferPlatform(const wdGALBuffer* pDestination, wdUInt32 uiDestOffset, wdArrayPtr<const wdUInt8> pSourceData,
  wdGALUpdateMode::Enum updateMode)
{
  WD_CHECK_ALIGNMENT_16(pSourceData.GetPtr());

  auto pVulkanDestination = static_cast<const wdGALBufferVulkan*>(pDestination);

  switch (updateMode)
  {
    case wdGALUpdateMode::Discard:
      pVulkanDestination->DiscardBuffer();
      [[fallthrough]];
    case wdGALUpdateMode::NoOverwrite:
    {
      wdVulkanAllocation alloc = pVulkanDestination->GetAllocation();
      void* pData = nullptr;
      VK_ASSERT_DEV(wdMemoryAllocatorVulkan::MapMemory(alloc, &pData));
      WD_ASSERT_DEV(pData, "Implementation error");
      wdMemoryUtils::Copy(wdMemoryUtils::AddByteOffset((wdUInt8*)pData, uiDestOffset), pSourceData.GetPtr(), pSourceData.GetCount());
      wdMemoryAllocatorVulkan::UnmapMemory(alloc);
    }
    break;
    case wdGALUpdateMode::CopyToTempStorage:
    {
      if (m_bRenderPassActive)
      {
        m_pCommandBuffer->endRenderPass();
        m_bRenderPassActive = false;
      }

      WD_ASSERT_DEBUG(!m_bRenderPassActive, "Vulkan does not support copying buffers while a render pass is active. TODO: Fix high level render code to make this impossible.");

      m_GALDeviceVulkan.UploadBufferStaging(&m_GALDeviceVulkan.GetStagingBufferPool(), m_pPipelineBarrier, *m_pCommandBuffer, pVulkanDestination, pSourceData, uiDestOffset);
    }
    break;
    default:
      break;
  }
}

void wdGALCommandEncoderImplVulkan::CopyTexturePlatform(const wdGALTexture* pDestination, const wdGALTexture* pSource)
{
  if (m_bRenderPassActive)
  {
    m_pCommandBuffer->endRenderPass();
    m_bRenderPassActive = false;
  }

  auto destination = static_cast<const wdGALTextureVulkan*>(pDestination->GetParentResource());
  auto source = static_cast<const wdGALTextureVulkan*>(pSource->GetParentResource());

  const wdGALTextureCreationDescription& destDesc = pDestination->GetDescription();
  const wdGALTextureCreationDescription& srcDesc = pSource->GetDescription();

  WD_ASSERT_DEBUG(wdGALResourceFormat::IsDepthFormat(destDesc.m_Format) == wdGALResourceFormat::IsDepthFormat(srcDesc.m_Format), "");
  WD_ASSERT_DEBUG(destDesc.m_uiArraySize == srcDesc.m_uiArraySize, "");
  WD_ASSERT_DEBUG(destDesc.m_uiMipLevelCount == srcDesc.m_uiMipLevelCount, "");
  WD_ASSERT_DEBUG(destDesc.m_uiWidth == srcDesc.m_uiWidth, "");
  WD_ASSERT_DEBUG(destDesc.m_uiHeight == srcDesc.m_uiHeight, "");
  WD_ASSERT_DEBUG(destDesc.m_uiDepth == srcDesc.m_uiDepth, "");

  vk::ImageAspectFlagBits imageAspect = wdGALResourceFormat::IsDepthFormat(destDesc.m_Format) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;

  m_pPipelineBarrier->EnsureImageLayout(source, vk::ImageLayout::eTransferSrcOptimal, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferRead);
  m_pPipelineBarrier->EnsureImageLayout(destination, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite);
  m_pPipelineBarrier->Flush();


  // TODO need to copy every mip level
  wdHybridArray<vk::ImageCopy, 14> imageCopies;

  for (wdUInt32 i = 0; i < destDesc.m_uiMipLevelCount; ++i)
  {
    vk::ImageCopy& imageCopy = imageCopies.ExpandAndGetRef();
    imageCopy.dstOffset = vk::Offset3D();
    imageCopy.dstSubresource.aspectMask = imageAspect;
    imageCopy.dstSubresource.baseArrayLayer = 0;
    imageCopy.dstSubresource.layerCount = destDesc.m_uiArraySize;
    imageCopy.dstSubresource.mipLevel = i;
    imageCopy.extent.width = destDesc.m_uiWidth;
    imageCopy.extent.height = destDesc.m_uiHeight;
    imageCopy.extent.depth = destDesc.m_uiDepth;
    imageCopy.srcOffset = vk::Offset3D();
    imageCopy.srcSubresource.aspectMask = imageAspect;
    imageCopy.srcSubresource.baseArrayLayer = 0;
    imageCopy.srcSubresource.layerCount = srcDesc.m_uiArraySize;
    imageCopy.srcSubresource.mipLevel = i;
  }

  m_pCommandBuffer->copyImage(source->GetImage(), vk::ImageLayout::eTransferSrcOptimal, destination->GetImage(), vk::ImageLayout::eTransferDstOptimal, destDesc.m_uiMipLevelCount, imageCopies.GetData());
}

void wdGALCommandEncoderImplVulkan::CopyTextureRegionPlatform(const wdGALTexture* pDestination, const wdGALTextureSubresource& DestinationSubResource,
  const wdVec3U32& DestinationPoint, const wdGALTexture* pSource,
  const wdGALTextureSubresource& SourceSubResource, const wdBoundingBoxu32& Box)
{
  auto destination = static_cast<const wdGALTextureVulkan*>(pDestination->GetParentResource());
  auto source = static_cast<const wdGALTextureVulkan*>(pSource->GetParentResource());

  const wdGALTextureCreationDescription& destDesc = pDestination->GetDescription();
  const wdGALTextureCreationDescription& srcDesc = pSource->GetDescription();

  WD_ASSERT_DEBUG(wdGALResourceFormat::IsDepthFormat(destDesc.m_Format) == wdGALResourceFormat::IsDepthFormat(srcDesc.m_Format), "");

  vk::ImageAspectFlagBits imageAspect = wdGALResourceFormat::IsDepthFormat(destDesc.m_Format) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;

  wdVec3U32 extent = Box.m_vMax - Box.m_vMin;

  vk::ImageCopy imageCopy = {};
  imageCopy.dstOffset.x = DestinationPoint.x;
  imageCopy.dstOffset.y = DestinationPoint.y;
  imageCopy.dstOffset.z = DestinationPoint.z;
  imageCopy.dstSubresource.aspectMask = imageAspect;
  imageCopy.dstSubresource.baseArrayLayer = DestinationSubResource.m_uiArraySlice;
  imageCopy.dstSubresource.layerCount = 1;
  imageCopy.dstSubresource.mipLevel = DestinationSubResource.m_uiMipLevel;
  imageCopy.extent.width = extent.x;
  imageCopy.extent.height = extent.y;
  imageCopy.extent.depth = extent.z;
  imageCopy.srcOffset.x = Box.m_vMin.x;
  imageCopy.srcOffset.y = Box.m_vMin.y;
  imageCopy.srcOffset.z = Box.m_vMin.z;
  imageCopy.srcSubresource.aspectMask = imageAspect;
  imageCopy.srcSubresource.baseArrayLayer = DestinationSubResource.m_uiArraySlice;
  imageCopy.srcSubresource.layerCount = 1;
  imageCopy.srcSubresource.mipLevel = SourceSubResource.m_uiMipLevel;

  m_pCommandBuffer->copyImage(source->GetImage(), vk::ImageLayout::eGeneral, destination->GetImage(), vk::ImageLayout::eGeneral, 1, &imageCopy);
}

void wdGALCommandEncoderImplVulkan::UpdateTexturePlatform(const wdGALTexture* pDestination, const wdGALTextureSubresource& DestinationSubResource,
  const wdBoundingBoxu32& DestinationBox, const wdGALSystemMemoryDescription& data)
{
  const wdGALTextureVulkan* pDestVulkan = static_cast<const wdGALTextureVulkan*>(pDestination);
  vk::ImageSubresourceRange range = pDestVulkan->GetFullRange();
  range.baseArrayLayer = DestinationSubResource.m_uiArraySlice;
  range.baseMipLevel = DestinationSubResource.m_uiMipLevel;
  range.levelCount = 1;
  range.layerCount = 1;
  m_pPipelineBarrier->EnsureImageLayout(pDestVulkan, range, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite);
  m_pPipelineBarrier->Flush();

  //aaa
  wdUInt32 uiWidth = wdMath::Max(DestinationBox.m_vMax.x - DestinationBox.m_vMin.x, 1u);
  wdUInt32 uiHeight = wdMath::Max(DestinationBox.m_vMax.y - DestinationBox.m_vMin.y, 1u);
  wdUInt32 uiDepth = wdMath::Max(DestinationBox.m_vMax.z - DestinationBox.m_vMin.z, 1u);

  const vk::Format format = pDestVulkan->GetImageFormat();
  const wdUInt8 uiBlockSize = vk::blockSize(format);
  const auto blockExtent = vk::blockExtent(format);
  const VkExtent3D blockCount = {
    (uiWidth + blockExtent[0] - 1) / blockExtent[0],
    (uiHeight + blockExtent[1] - 1) / blockExtent[1],
    (uiDepth + blockExtent[2] - 1) / blockExtent[2]};

  const vk::DeviceSize uiTotalSize = uiBlockSize * blockCount.width * blockCount.height * blockCount.depth;
  wdStagingBufferVulkan stagingBuffer = m_GALDeviceVulkan.GetStagingBufferPool().AllocateBuffer(0, uiTotalSize);

  const wdUInt32 uiBufferRowPitch = uiBlockSize * blockCount.width;
  const wdUInt32 uiBufferSlicePitch = uiBufferRowPitch * blockCount.height;
  WD_ASSERT_DEV(uiBufferRowPitch == data.m_uiRowPitch, "Row pitch with padding is not implemented yet.");

  void* pData = nullptr;
  wdMemoryAllocatorVulkan::MapMemory(stagingBuffer.m_alloc, &pData);
  wdMemoryUtils::RawByteCopy(pData, data.m_pData, uiTotalSize);
  wdMemoryAllocatorVulkan::UnmapMemory(stagingBuffer.m_alloc);

  vk::BufferImageCopy region = {};
  region.imageSubresource.aspectMask = range.aspectMask;
  region.imageSubresource.mipLevel = range.baseMipLevel;
  region.imageSubresource.baseArrayLayer = range.baseArrayLayer;
  region.imageSubresource.layerCount = range.layerCount;

  region.imageOffset = vk::Offset3D(DestinationBox.m_vMin.x, DestinationBox.m_vMin.y, DestinationBox.m_vMin.z);
  region.imageExtent = vk::Extent3D(uiWidth, uiHeight, uiDepth);

  region.bufferOffset = 0;
  region.bufferRowLength = blockExtent[0] * uiBufferRowPitch / uiBlockSize;
  region.bufferImageHeight = blockExtent[1] * uiBufferSlicePitch / uiBufferRowPitch;

  m_pCommandBuffer->copyBufferToImage(stagingBuffer.m_buffer, pDestVulkan->GetImage(), pDestVulkan->GetPreferredLayout(vk::ImageLayout::eTransferDstOptimal), 1, &region);
  m_GALDeviceVulkan.GetStagingBufferPool().ReclaimBuffer(stagingBuffer);
}

void wdGALCommandEncoderImplVulkan::ResolveTexturePlatform(const wdGALTexture* pDestination, const wdGALTextureSubresource& DestinationSubResource,
  const wdGALTexture* pSource, const wdGALTextureSubresource& SourceSubResource)
{
  auto pVulkanDestination = static_cast<const wdGALTextureVulkan*>(pDestination->GetParentResource());
  auto pVulkanSource = static_cast<const wdGALTextureVulkan*>(pSource->GetParentResource());

  const wdGALTextureCreationDescription& destDesc = pDestination->GetDescription();
  const wdGALTextureCreationDescription& srcDesc = pSource->GetDescription();

  WD_ASSERT_DEBUG(wdGALResourceFormat::IsDepthFormat(destDesc.m_Format) == wdGALResourceFormat::IsDepthFormat(srcDesc.m_Format), "");

  // TODO need to determine size of the subresource
  vk::ImageResolve resolveRegion = {};
  resolveRegion.dstSubresource.aspectMask = pVulkanDestination->GetFullRange().aspectMask;
  resolveRegion.dstSubresource.baseArrayLayer = DestinationSubResource.m_uiArraySlice;
  resolveRegion.dstSubresource.layerCount = 1;
  resolveRegion.dstSubresource.mipLevel = DestinationSubResource.m_uiMipLevel;
  resolveRegion.extent.width = wdMath::Min(destDesc.m_uiWidth, srcDesc.m_uiWidth);
  resolveRegion.extent.height = wdMath::Min(destDesc.m_uiHeight, srcDesc.m_uiHeight);
  resolveRegion.extent.depth = wdMath::Min(destDesc.m_uiDepth, srcDesc.m_uiDepth);
  resolveRegion.srcSubresource.aspectMask = pVulkanSource->GetFullRange().aspectMask;
  resolveRegion.srcSubresource.baseArrayLayer = SourceSubResource.m_uiArraySlice;
  resolveRegion.srcSubresource.layerCount = 1;
  resolveRegion.srcSubresource.mipLevel = SourceSubResource.m_uiMipLevel;

  m_pPipelineBarrier->EnsureImageLayout(pVulkanSource, vk::ImageLayout::eTransferSrcOptimal, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferRead);
  m_pPipelineBarrier->EnsureImageLayout(pVulkanDestination, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite);
  m_pPipelineBarrier->Flush();
  if (srcDesc.m_SampleCount != wdGALMSAASampleCount::None)
  {
    m_pCommandBuffer->resolveImage(pVulkanSource->GetImage(), vk::ImageLayout::eTransferSrcOptimal, pVulkanDestination->GetImage(), vk::ImageLayout::eTransferDstOptimal, 1, &resolveRegion);
  }
  else
  {
    // DX11 allows calling resolve on a non-msaa source. For now, allow this as well in Vulkan.
    vk::Extent3D sourceMipLevelSize = pVulkanSource->GetMipLevelSize(SourceSubResource.m_uiMipLevel);
    vk::Offset3D sourceMipLevelEndOffset = {(wdInt32)sourceMipLevelSize.width, (wdInt32)sourceMipLevelSize.height, (wdInt32)sourceMipLevelSize.depth};
    vk::Extent3D dstMipLevelSize = pVulkanDestination->GetMipLevelSize(DestinationSubResource.m_uiMipLevel);
    vk::Offset3D dstMipLevelEndOffset = {(wdInt32)sourceMipLevelSize.width, (wdInt32)sourceMipLevelSize.height, (wdInt32)sourceMipLevelSize.depth};

    vk::ImageBlit imageBlitRegion;
    imageBlitRegion.srcSubresource = resolveRegion.srcSubresource;
    imageBlitRegion.srcOffsets[1] = sourceMipLevelEndOffset;
    imageBlitRegion.dstSubresource = resolveRegion.dstSubresource;
    imageBlitRegion.dstOffsets[1] = dstMipLevelEndOffset;

    m_pCommandBuffer->blitImage(pVulkanSource->GetImage(), vk::ImageLayout::eTransferSrcOptimal, pVulkanDestination->GetImage(), vk::ImageLayout::eTransferDstOptimal, 1, &imageBlitRegion, vk::Filter::eNearest);
  }

  m_pPipelineBarrier->EnsureImageLayout(pVulkanSource, pVulkanSource->GetPreferredLayout(), pVulkanSource->GetUsedByPipelineStage(), pVulkanSource->GetAccessMask());
  m_pPipelineBarrier->EnsureImageLayout(pVulkanDestination, pVulkanDestination->GetPreferredLayout(), pVulkanDestination->GetUsedByPipelineStage(), pVulkanDestination->GetAccessMask());
}

void wdGALCommandEncoderImplVulkan::CopyImageToBuffer(const wdGALTextureVulkan* pSource, const wdGALBufferVulkan* pDestination)
{
  const wdGALTextureCreationDescription& textureDesc = pSource->GetDescription();
  const vk::ImageAspectFlags imageAspect = pSource->GetAspectMask();

  wdHybridArray<wdGALTextureVulkan::SubResourceOffset, 8> subResourceOffsets;
  const wdUInt32 uiBufferSize = pSource->ComputeSubResourceOffsets(subResourceOffsets);

  wdHybridArray<vk::BufferImageCopy, 8> imageCopy;
  const wdUInt32 arraySize = textureDesc.m_Type == wdGALTextureType::TextureCube ? textureDesc.m_uiArraySize * 6 : textureDesc.m_uiArraySize;
  const wdUInt32 mipLevels = textureDesc.m_uiMipLevelCount;

  for (wdUInt32 uiLayer = 0; uiLayer < arraySize; uiLayer++)
  {
    for (wdUInt32 uiMipLevel = 0; uiMipLevel < mipLevels; uiMipLevel++)
    {
      const vk::Extent3D mipLevelSize = pSource->GetMipLevelSize(uiMipLevel);
      const wdUInt32 uiSubresourceIndex = uiMipLevel + uiLayer * mipLevels;
      const wdGALTextureVulkan::SubResourceOffset& offset = subResourceOffsets[uiSubresourceIndex];

      vk::BufferImageCopy& copy = imageCopy.ExpandAndGetRef();

      copy.bufferOffset = offset.m_uiOffset;
      copy.bufferRowLength = offset.m_uiRowLength;
      copy.bufferImageHeight = offset.m_uiImageHeight;
      copy.imageSubresource.aspectMask = imageAspect;
      copy.imageSubresource.mipLevel = uiMipLevel;
      copy.imageSubresource.baseArrayLayer = uiLayer;
      copy.imageSubresource.layerCount = 1;
      copy.imageOffset = vk::Offset3D(0, 0, 0);
      copy.imageExtent = mipLevelSize;
    }
  }

  m_pPipelineBarrier->EnsureImageLayout(pSource, vk::ImageLayout::eTransferSrcOptimal, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferRead);
  m_pPipelineBarrier->Flush();

  m_pCommandBuffer->copyImageToBuffer(pSource->GetImage(), vk::ImageLayout::eTransferSrcOptimal, pDestination->GetVkBuffer(), imageCopy.GetCount(), imageCopy.GetData());

  m_pPipelineBarrier->EnsureImageLayout(pSource, pSource->GetPreferredLayout(), pSource->GetUsedByPipelineStage(), pSource->GetAccessMask());
  m_pPipelineBarrier->AccessBuffer(pDestination, 0, uiBufferSize, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eHost, vk::AccessFlagBits::eMemoryRead);
}

void wdGALCommandEncoderImplVulkan::ReadbackTexturePlatform(const wdGALTexture* pTexture)
{
  if (!m_bClearSubmitted)
  {
    m_pPipelineBarrier->Flush();

    // If we want to readback one of the render targets, we need to first flush the clear.
    // #TODO_VULKAN Check whether pTexture is one of the render targets or change the top-level api to prevent this.
    m_pCommandBuffer->beginRenderPass(m_renderPass, vk::SubpassContents::eInline);
    m_bClearSubmitted = true;
    m_bRenderPassActive = true;
  }

  if (m_bRenderPassActive)
  {
    m_pCommandBuffer->endRenderPass();
    m_bRenderPassActive = false;
  }

  WD_ASSERT_DEV(!m_bRenderPassActive, "Can't readback within a render pass");

  const wdGALTextureVulkan* pVulkanTexture = static_cast<const wdGALTextureVulkan*>(pTexture->GetParentResource());
  const wdGALTextureCreationDescription& textureDesc = pVulkanTexture->GetDescription();
  const vk::ImageAspectFlagBits imageAspect = wdGALResourceFormat::IsDepthFormat(textureDesc.m_Format) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  const bool bMSAASourceTexture = textureDesc.m_SampleCount != wdGALMSAASampleCount::None;
  WD_ASSERT_DEV(!bMSAASourceTexture, "MSAA read-back not implemented!");
  const wdGALTextureVulkan::StagingMode stagingMode = pVulkanTexture->GetStagingMode();
  WD_ASSERT_DEV(stagingMode != wdGALTextureVulkan::StagingMode::None, "No staging resource available for read-back");

  if (stagingMode == wdGALTextureVulkan::StagingMode::Buffer)
  {
    const wdGALBufferVulkan* pStagingBuffer = static_cast<const wdGALBufferVulkan*>(m_GALDeviceVulkan.GetBuffer(pVulkanTexture->GetStagingBuffer()));
    CopyImageToBuffer(pVulkanTexture, pStagingBuffer);
  }
  else
  {
    // Render to texture
    const wdGALTextureVulkan* pStagingTexture = static_cast<const wdGALTextureVulkan*>(m_GALDeviceVulkan.GetTexture(pVulkanTexture->GetStagingTexture()));
    const bool bSourceIsDepth = wdConversionUtilsVulkan::IsDepthFormat(pVulkanTexture->GetImageFormat());

    m_pPipelineBarrier->EnsureImageLayout(pVulkanTexture, bSourceIsDepth ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits::eFragmentShader, vk::AccessFlagBits::eShaderRead);
    m_pPipelineBarrier->EnsureImageLayout(pStagingTexture, vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite);
    m_pPipelineBarrier->Flush();

    wdImageCopyVulkan copy(m_GALDeviceVulkan);

    const bool bStereoSupport = m_GALDeviceVulkan.GetCapabilities().m_bVertexShaderRenderTargetArrayIndex || m_GALDeviceVulkan.GetCapabilities().m_bShaderStageSupported[wdGALShaderStage::GeometryShader];
    if (bStereoSupport)
    {
      copy.Init(pVulkanTexture, pStagingTexture, wdShaderUtils::wdBuiltinShaderType::CopyImageArray);
      const wdUInt32 arraySize = textureDesc.m_Type == wdGALTextureType::TextureCube ? textureDesc.m_uiArraySize * 6 : textureDesc.m_uiArraySize;
      const wdUInt32 mipLevels = textureDesc.m_uiMipLevelCount;
      for (wdUInt32 uiMipLevel = 0; uiMipLevel < textureDesc.m_uiMipLevelCount; uiMipLevel++)
      {
        vk::ImageSubresourceLayers subresourceLayersSource;
        subresourceLayersSource.aspectMask = imageAspect;
        subresourceLayersSource.mipLevel = uiMipLevel;
        subresourceLayersSource.baseArrayLayer = 0;
        subresourceLayersSource.layerCount = arraySize;

        vk::ImageSubresourceLayers subresourceLayersTarget;
        subresourceLayersTarget.aspectMask = pStagingTexture->GetAspectMask();
        subresourceLayersTarget.mipLevel = uiMipLevel;
        subresourceLayersTarget.baseArrayLayer = 0;
        subresourceLayersTarget.layerCount = arraySize;

        vk::Extent3D mipLevelSize = pVulkanTexture->GetMipLevelSize(0);
        copy.Copy({0, 0, 0}, subresourceLayersSource, {0, 0, 0}, subresourceLayersTarget, {mipLevelSize.width, mipLevelSize.height, mipLevelSize.depth});
      }
    }
    else
    {
      copy.Init(pVulkanTexture, pStagingTexture, wdShaderUtils::wdBuiltinShaderType::CopyImage);
      const wdUInt32 arraySize = textureDesc.m_Type == wdGALTextureType::TextureCube ? textureDesc.m_uiArraySize * 6 : textureDesc.m_uiArraySize;
      const wdUInt32 mipLevels = textureDesc.m_uiMipLevelCount;

      for (wdUInt32 uiLayer = 0; uiLayer < arraySize; uiLayer++)
      {
        for (wdUInt32 uiMipLevel = 0; uiMipLevel < mipLevels; uiMipLevel++)
        {
          vk::ImageSubresourceLayers subresourceLayersSource;
          subresourceLayersSource.aspectMask = imageAspect;
          subresourceLayersSource.mipLevel = uiMipLevel;
          subresourceLayersSource.baseArrayLayer = uiLayer;
          subresourceLayersSource.layerCount = 1;

          vk::ImageSubresourceLayers subresourceLayersTarget;
          subresourceLayersTarget.aspectMask = pStagingTexture->GetAspectMask();
          subresourceLayersTarget.mipLevel = uiMipLevel;
          subresourceLayersTarget.baseArrayLayer = uiLayer;
          subresourceLayersTarget.layerCount = 1;

          vk::Extent3D mipLevelSize = pVulkanTexture->GetMipLevelSize(0);
          copy.Copy({0, 0, 0}, subresourceLayersSource, {0, 0, 0}, subresourceLayersTarget, {mipLevelSize.width, mipLevelSize.height, mipLevelSize.depth});
        }
      }
    }

    m_bPipelineStateDirty = true;
    m_bViewportDirty = true;
    m_bDescriptorsDirty = true;

    if (stagingMode == wdGALTextureVulkan::StagingMode::TextureAndBuffer)
    {
      // Copy to buffer
      const wdGALBufferVulkan* pStagingBuffer = static_cast<const wdGALBufferVulkan*>(m_GALDeviceVulkan.GetBuffer(pVulkanTexture->GetStagingBuffer()));
      CopyImageToBuffer(pVulkanTexture, pStagingBuffer);
    }
    else
    {
      // Readback texture directly
      m_pPipelineBarrier->EnsureImageLayout(pStagingTexture, vk::ImageLayout::eTransferSrcOptimal, vk::PipelineStageFlagBits::eHost, vk::AccessFlagBits::eHostRead);
    }
  }

  // There is no need to change the layout back of this texture right now but as the next layout will most certainly not be another eTransferSrcOptimal we might as well change it back to its default state.
  m_pPipelineBarrier->EnsureImageLayout(pVulkanTexture, pVulkanTexture->GetPreferredLayout(), pVulkanTexture->GetUsedByPipelineStage(), pVulkanTexture->GetAccessMask());

  //#TODO_VULKAN readback fence
  m_GALDeviceVulkan.Submit({}, {}, {});
  m_vkDevice.waitIdle();
  m_pPipelineBarrier = &m_GALDeviceVulkan.GetCurrentPipelineBarrier();
  m_pCommandBuffer = &m_GALDeviceVulkan.GetCurrentCommandBuffer();
}

wdUInt32 GetMipSize(wdUInt32 uiSize, wdUInt32 uiMipLevel)
{
  for (wdUInt32 i = 0; i < uiMipLevel; i++)
  {
    uiSize = uiSize / 2;
  }
  return wdMath::Max(1u, uiSize);
}

void wdGALCommandEncoderImplVulkan::CopyTextureReadbackResultPlatform(const wdGALTexture* pTexture, wdArrayPtr<wdGALTextureSubresource> SourceSubResource, wdArrayPtr<wdGALSystemMemoryDescription> TargetData)
{
  //#TODO_VULKAN readback fence
  auto pVulkanTexture = static_cast<const wdGALTextureVulkan*>(pTexture->GetParentResource());
  const wdGALTextureCreationDescription& textureDesc = pVulkanTexture->GetDescription();
  const wdGALTextureVulkan::StagingMode stagingMode = pVulkanTexture->GetStagingMode();
  WD_ASSERT_DEV(stagingMode != wdGALTextureVulkan::StagingMode::None, "No staging resource available for read-back");

  if (stagingMode == wdGALTextureVulkan::StagingMode::Texture)
  {
    const wdGALTextureVulkan* pStagingTexture = static_cast<const wdGALTextureVulkan*>(m_GALDeviceVulkan.GetTexture(pVulkanTexture->GetStagingTexture()));
    vk::ImageAspectFlags stagingAspect = pStagingTexture->GetAspectMask();

    const wdUInt32 uiSubResources = SourceSubResource.GetCount();

    void* pData = nullptr;
    wdMemoryAllocatorVulkan::MapMemory(pStagingTexture->GetAllocation(), &pData);

    for (wdUInt32 i = 0; i < uiSubResources; i++)
    {
      const wdGALTextureSubresource& subRes = SourceSubResource[i];
      const wdGALSystemMemoryDescription& memDesc = TargetData[i];

      vk::ImageSubresource subResource{stagingAspect, subRes.m_uiMipLevel, subRes.m_uiArraySlice};
      vk::SubresourceLayout subResourceLayout;
      m_vkDevice.getImageSubresourceLayout(pStagingTexture->GetImage(), &subResource, &subResourceLayout);
      wdUInt8* pSubResourceData = reinterpret_cast<wdUInt8*>(pData) + subResourceLayout.offset;

      if (subResourceLayout.rowPitch == memDesc.m_uiRowPitch)
      {
        const wdUInt32 uiMemorySize = wdGALResourceFormat::GetBitsPerElement(textureDesc.m_Format) *
                                      GetMipSize(textureDesc.m_uiWidth, subRes.m_uiMipLevel) *
                                      GetMipSize(textureDesc.m_uiHeight, subRes.m_uiMipLevel) / 8;

        memcpy(memDesc.m_pData, pSubResourceData, uiMemorySize);
      }
      else
      {
        // Copy row by row
        const wdUInt32 uiHeight = GetMipSize(textureDesc.m_uiHeight, subRes.m_uiMipLevel);
        for (wdUInt32 y = 0; y < uiHeight; ++y)
        {
          const void* pSource = wdMemoryUtils::AddByteOffset(pSubResourceData, y * subResourceLayout.rowPitch);
          void* pDest = wdMemoryUtils::AddByteOffset(memDesc.m_pData, y * memDesc.m_uiRowPitch);

          memcpy(pDest, pSource, wdGALResourceFormat::GetBitsPerElement(textureDesc.m_Format) * GetMipSize(textureDesc.m_uiWidth, subRes.m_uiMipLevel) / 8);
        }
      }
    }

    wdMemoryAllocatorVulkan::UnmapMemory(pStagingTexture->GetAllocation());
  }
  else // One of the buffer variants.
  {
    const wdGALBufferVulkan* pStagingBuffer = static_cast<const wdGALBufferVulkan*>(m_GALDeviceVulkan.GetBuffer(pVulkanTexture->GetStagingBuffer()));
    const vk::Format stagingFormat = m_GALDeviceVulkan.GetFormatLookupTable().GetFormatInfo(pVulkanTexture->GetDescription().m_Format).m_eStorage;

    wdHybridArray<wdGALTextureVulkan::SubResourceOffset, 8> subResourceOffsets;
    const wdUInt32 uiBufferSize = pVulkanTexture->ComputeSubResourceOffsets(subResourceOffsets);


    const wdUInt32 uiSubResources = SourceSubResource.GetCount();

    void* pData = nullptr;
    wdMemoryAllocatorVulkan::MapMemory(pStagingBuffer->GetAllocation(), &pData);

    const wdUInt32 uiMipLevels = textureDesc.m_uiMipLevelCount;
    for (wdUInt32 i = 0; i < uiSubResources; i++)
    {
      const wdGALTextureSubresource& subRes = SourceSubResource[i];
      const wdUInt32 uiSubresourceIndex = subRes.m_uiMipLevel + subRes.m_uiArraySlice * uiMipLevels;
      const wdGALTextureVulkan::SubResourceOffset offset = subResourceOffsets[uiSubresourceIndex];
      const wdGALSystemMemoryDescription& memDesc = TargetData[i];
      const auto blockExtent = vk::blockExtent(stagingFormat);
      const wdUInt8 uiBlockSize = vk::blockSize(stagingFormat);

      const wdUInt32 uiRowPitch = offset.m_uiRowLength * blockExtent[0] * uiBlockSize;

      wdUInt8* pSubResourceData = reinterpret_cast<wdUInt8*>(pData) + offset.m_uiOffset;

      if (uiRowPitch == memDesc.m_uiRowPitch)
      {
        const wdUInt32 uiMemorySize = wdGALResourceFormat::GetBitsPerElement(textureDesc.m_Format) *
                                      GetMipSize(textureDesc.m_uiWidth, subRes.m_uiMipLevel) *
                                      GetMipSize(textureDesc.m_uiHeight, subRes.m_uiMipLevel) / 8;

        memcpy(memDesc.m_pData, pSubResourceData, uiMemorySize);
      }
      else
      {
        // Copy row by row
        const wdUInt32 uiHeight = GetMipSize(textureDesc.m_uiHeight, subRes.m_uiMipLevel);
        for (wdUInt32 y = 0; y < uiHeight; ++y)
        {
          const void* pSource = wdMemoryUtils::AddByteOffset(pSubResourceData, y * uiRowPitch);
          void* pDest = wdMemoryUtils::AddByteOffset(memDesc.m_pData, y * memDesc.m_uiRowPitch);

          memcpy(pDest, pSource, wdGALResourceFormat::GetBitsPerElement(textureDesc.m_Format) * GetMipSize(textureDesc.m_uiWidth, subRes.m_uiMipLevel) / 8);
        }
      }
    }

    wdMemoryAllocatorVulkan::UnmapMemory(pStagingBuffer->GetAllocation());
  }
}

void wdGALCommandEncoderImplVulkan::GenerateMipMapsPlatform(const wdGALResourceView* pResourceView)
{
  const wdGALResourceViewVulkan* pVulkanResourceView = static_cast<const wdGALResourceViewVulkan*>(pResourceView);
  if (m_bRenderPassActive)
  {
    m_pCommandBuffer->endRenderPass();
    m_bRenderPassActive = false;
  }


  const vk::ImageSubresourceRange viewRange = pVulkanResourceView->GetRange();
  if (viewRange.levelCount == 1)
    return;

  const wdGALTextureVulkan* pVulkanTexture = static_cast<const wdGALTextureVulkan*>(pVulkanResourceView->GetResource()->GetParentResource());
  const vk::FormatProperties formatProps = m_GALDeviceVulkan.GetVulkanPhysicalDevice().getFormatProperties(pVulkanTexture->GetImageFormat());
  const bool bSupportsBlit = ((formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitSrc) && (formatProps.linearTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst));
  // MSAA textures (e.g. backbuffers) need to be converted to non MSAA versions
  const wdGALTextureCreationDescription& textureDesc = pVulkanTexture->GetDescription();
  const bool bMSAASourceTexture = textureDesc.m_SampleCount != wdGALMSAASampleCount::None;
  if (bMSAASourceTexture)
  {
    WD_ASSERT_NOT_IMPLEMENTED;
  }
  else
  {
    if (bSupportsBlit)
    {
      {
        vk::ImageSubresourceRange otherLevels = viewRange;
        otherLevels.baseMipLevel += 1;
        otherLevels.levelCount -= 1;
        m_pPipelineBarrier->EnsureImageLayout(pVulkanTexture, otherLevels, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite);
      }

      for (wdUInt32 uiMipLevel = viewRange.baseMipLevel; uiMipLevel < (viewRange.baseMipLevel + viewRange.levelCount - 1); uiMipLevel++)
      {
        {
          vk::ImageSubresourceRange currentLevel = viewRange;
          currentLevel.baseMipLevel = uiMipLevel;
          currentLevel.levelCount = 1;
          m_pPipelineBarrier->EnsureImageLayout(pVulkanTexture, currentLevel, vk::ImageLayout::eTransferSrcOptimal, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferRead);
          m_pPipelineBarrier->Flush();
        }
        vk::Extent3D sourceMipLevelSize = pVulkanTexture->GetMipLevelSize(uiMipLevel);
        vk::Offset3D sourceMipLevelEndOffset = {(wdInt32)sourceMipLevelSize.width, (wdInt32)sourceMipLevelSize.height, (wdInt32)sourceMipLevelSize.depth};
        vk::Extent3D destinationMipLevelSize = pVulkanTexture->GetMipLevelSize(uiMipLevel + 1);
        vk::Offset3D destinationMipLevelEndOffset = {(wdInt32)destinationMipLevelSize.width, (wdInt32)destinationMipLevelSize.height, (wdInt32)destinationMipLevelSize.depth};

        vk::ImageSubresourceLayers sourceLayers;
        sourceLayers.aspectMask = viewRange.aspectMask;
        sourceLayers.mipLevel = uiMipLevel;
        sourceLayers.baseArrayLayer = viewRange.baseArrayLayer;
        sourceLayers.layerCount = viewRange.layerCount;

        vk::ImageSubresourceLayers destinationLayers = sourceLayers;
        destinationLayers.mipLevel++;

        vk::ImageBlit imageBlitRegion;
        imageBlitRegion.srcSubresource = sourceLayers;
        imageBlitRegion.srcOffsets[1] = sourceMipLevelEndOffset;
        imageBlitRegion.dstSubresource = destinationLayers;
        imageBlitRegion.dstOffsets[1] = destinationMipLevelEndOffset;

        m_pCommandBuffer->blitImage(pVulkanTexture->GetImage(), vk::ImageLayout::eTransferSrcOptimal, pVulkanTexture->GetImage(), vk::ImageLayout::eTransferDstOptimal, 1, &imageBlitRegion, vk::Filter::eLinear);
      }
      // There is no need to change the layout back of this texture right now but as the next layout will most certainly not be another eTransferSrcOptimal we might as well change it back to its default state.
      m_pPipelineBarrier->EnsureImageLayout(pVulkanResourceView, pVulkanTexture->GetPreferredLayout(), pVulkanTexture->GetUsedByPipelineStage(), pVulkanTexture->GetAccessMask());
    }
    else
    {
      {
        vk::ImageSubresourceRange otherLevels = viewRange;
        otherLevels.baseMipLevel += 1;
        otherLevels.levelCount -= 1;
        m_pPipelineBarrier->EnsureImageLayout(pVulkanTexture, otherLevels, vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite);
      }

      wdImageCopyVulkan copy(m_GALDeviceVulkan);
      const bool bStereoSupport = m_GALDeviceVulkan.GetCapabilities().m_bVertexShaderRenderTargetArrayIndex || m_GALDeviceVulkan.GetCapabilities().m_bShaderStageSupported[wdGALShaderStage::GeometryShader];
      if (bStereoSupport)
      {
        copy.Init(pVulkanTexture, pVulkanTexture, wdShaderUtils::wdBuiltinShaderType::DownscaleImageArray);
        for (wdUInt32 uiMipLevel = viewRange.baseMipLevel; uiMipLevel < (viewRange.baseMipLevel + viewRange.levelCount - 1); uiMipLevel++)
        {
          vk::Extent3D sourceMipLevelSize = pVulkanTexture->GetMipLevelSize(uiMipLevel);
          vk::Offset3D sourceMipLevelEndOffset = {(wdInt32)sourceMipLevelSize.width, (wdInt32)sourceMipLevelSize.height, (wdInt32)sourceMipLevelSize.depth};
          vk::Extent3D destinationMipLevelSize = pVulkanTexture->GetMipLevelSize(uiMipLevel + 1);
          vk::Offset3D destinationMipLevelEndOffset = {(wdInt32)destinationMipLevelSize.width, (wdInt32)destinationMipLevelSize.height, (wdInt32)destinationMipLevelSize.depth};

          vk::ImageSubresourceLayers sourceLayers;
          sourceLayers.aspectMask = viewRange.aspectMask;
          sourceLayers.mipLevel = uiMipLevel;
          sourceLayers.baseArrayLayer = viewRange.baseArrayLayer;
          sourceLayers.layerCount = viewRange.layerCount;

          vk::ImageSubresourceLayers destinationLayers = sourceLayers;
          destinationLayers.mipLevel++;

          vk::Extent3D mipLevelSize = pVulkanTexture->GetMipLevelSize(0);
          copy.Copy({0, 0, 0}, sourceLayers, {0, 0, 0}, destinationLayers, {(wdUInt32)destinationMipLevelSize.width, (wdUInt32)destinationMipLevelSize.height, (wdUInt32)destinationMipLevelSize.depth});
        }
      }
      else
      {
        copy.Init(pVulkanTexture, pVulkanTexture, wdShaderUtils::wdBuiltinShaderType::DownscaleImage);
        const wdUInt32 arraySize = textureDesc.m_Type == wdGALTextureType::TextureCube ? textureDesc.m_uiArraySize * 6 : textureDesc.m_uiArraySize;
        const wdUInt32 mipLevels = textureDesc.m_uiMipLevelCount;

        for (wdUInt32 uiLayer = viewRange.baseArrayLayer; uiLayer < (viewRange.baseArrayLayer + viewRange.layerCount); uiLayer++)
        {
          for (wdUInt32 uiMipLevel = viewRange.baseMipLevel; uiMipLevel < (viewRange.baseMipLevel + viewRange.levelCount - 1); uiMipLevel++)
          {
            vk::Extent3D sourceMipLevelSize = pVulkanTexture->GetMipLevelSize(uiMipLevel);
            vk::Offset3D sourceMipLevelEndOffset = {(wdInt32)sourceMipLevelSize.width, (wdInt32)sourceMipLevelSize.height, (wdInt32)sourceMipLevelSize.depth};
            vk::Extent3D destinationMipLevelSize = pVulkanTexture->GetMipLevelSize(uiMipLevel + 1);
            vk::Offset3D destinationMipLevelEndOffset = {(wdInt32)destinationMipLevelSize.width, (wdInt32)destinationMipLevelSize.height, (wdInt32)destinationMipLevelSize.depth};

            vk::ImageSubresourceLayers sourceLayers;
            sourceLayers.aspectMask = viewRange.aspectMask;
            sourceLayers.mipLevel = uiMipLevel;
            sourceLayers.baseArrayLayer = viewRange.baseArrayLayer;
            sourceLayers.layerCount = 1;

            vk::ImageSubresourceLayers destinationLayers = sourceLayers;
            destinationLayers.mipLevel++;

            vk::Extent3D mipLevelSize = pVulkanTexture->GetMipLevelSize(0);
            copy.Copy({0, 0, 0}, sourceLayers, {0, 0, 0}, destinationLayers, {(wdUInt32)destinationMipLevelSize.width, (wdUInt32)destinationMipLevelSize.height, (wdUInt32)destinationMipLevelSize.depth});
          }
        }
      }

      m_pPipelineBarrier->EnsureImageLayout(pVulkanResourceView, pVulkanTexture->GetPreferredLayout(), pVulkanTexture->GetUsedByPipelineStage(), pVulkanTexture->GetAccessMask());

      m_bPipelineStateDirty = true;
      m_bViewportDirty = true;
      m_bDescriptorsDirty = true;
    }
  }
}

void wdGALCommandEncoderImplVulkan::FlushPlatform()
{
  FlushDeferredStateChanges();
}

// Debug helper functions

void wdGALCommandEncoderImplVulkan::PushMarkerPlatform(const char* szMarker)
{
  // TODO early out if device doesn't support debug markers
  constexpr float markerColor[4] = {1, 1, 1, 1};
  vk::DebugUtilsLabelEXT markerInfo = {};
  wdMemoryUtils::Copy(markerInfo.color.data(), markerColor, WD_ARRAY_SIZE(markerColor));
  markerInfo.pLabelName = szMarker;

  m_pCommandBuffer->beginDebugUtilsLabelEXT(markerInfo);
}

void wdGALCommandEncoderImplVulkan::PopMarkerPlatform()
{
  m_pCommandBuffer->endDebugUtilsLabelEXT();
}

void wdGALCommandEncoderImplVulkan::InsertEventMarkerPlatform(const char* szMarker)
{
  constexpr float markerColor[4] = {1, 1, 1, 1};
  vk::DebugUtilsLabelEXT markerInfo = {};
  wdMemoryUtils::Copy(markerInfo.color.data(), markerColor, WD_ARRAY_SIZE(markerColor));
  markerInfo.pLabelName = szMarker;
  m_pCommandBuffer->insertDebugUtilsLabelEXT(markerInfo);
}

//////////////////////////////////////////////////////////////////////////

void wdGALCommandEncoderImplVulkan::BeginRendering(const wdGALRenderingSetup& renderingSetup)
{
  m_PipelineDesc.m_renderPass = wdResourceCacheVulkan::RequestRenderPass(renderingSetup);
  m_PipelineDesc.m_uiAttachmentCount = renderingSetup.m_RenderTargetSetup.GetRenderTargetCount();
  wdSizeU32 size;
  m_frameBuffer = wdResourceCacheVulkan::RequestFrameBuffer(m_PipelineDesc.m_renderPass, renderingSetup.m_RenderTargetSetup, size, m_PipelineDesc.m_msaa, m_uiLayers);

  SetScissorRectPlatform(wdRectU32(size.width, size.height));

  {
    m_renderPass.renderPass = m_PipelineDesc.m_renderPass;
    m_renderPass.framebuffer = m_frameBuffer;
    m_renderPass.renderArea.offset.setX(0).setY(0);
    m_renderPass.renderArea.extent.setHeight(size.height).setWidth(size.width);

    m_clearValues.Clear();

    const bool m_bHasDepth = !renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget().IsInvalidated();
    const wdUInt32 uiColorCount = renderingSetup.m_RenderTargetSetup.GetRenderTargetCount();
    m_bClearSubmitted = !(renderingSetup.m_bClearDepth || renderingSetup.m_bClearStencil || renderingSetup.m_uiRenderTargetClearMask);

    if (m_bHasDepth)
    {
      vk::ClearValue& depthClear = m_clearValues.ExpandAndGetRef();
      depthClear.depthStencil.setDepth(1.0f).setStencil(0);

      const wdGALRenderTargetViewVulkan* pRenderTargetView = static_cast<const wdGALRenderTargetViewVulkan*>(m_GALDeviceVulkan.GetRenderTargetView(renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget()));
      m_depthMask = pRenderTargetView->GetRange().aspectMask;
      m_pPipelineBarrier->EnsureImageLayout(pRenderTargetView, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests, vk::AccessFlagBits::eDepthStencilAttachmentWrite);
    }
    for (wdUInt32 i = 0; i < uiColorCount; i++)
    {
      vk::ClearValue& colorClear = m_clearValues.ExpandAndGetRef();
      wdColor col = renderingSetup.m_ClearColor;
      colorClear.color.setFloat32({col.r, col.g, col.b, col.a});

      const wdGALRenderTargetViewVulkan* pRenderTargetView = static_cast<const wdGALRenderTargetViewVulkan*>(m_GALDeviceVulkan.GetRenderTargetView(renderingSetup.m_RenderTargetSetup.GetRenderTarget(i)));
      m_pPipelineBarrier->EnsureImageLayout(pRenderTargetView, vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite);
    }

    m_renderPass.clearValueCount = m_clearValues.GetCount();
    m_renderPass.pClearValues = m_clearValues.GetData();
  }

  m_bPipelineStateDirty = true;
  m_bViewportDirty = true;
}

void wdGALCommandEncoderImplVulkan::EndRendering()
{
  if (!m_bClearSubmitted)
  {
    m_pPipelineBarrier->Flush();
    // If we end rendering without having flushed the clear, just begin and immediately end rendering.
    m_pCommandBuffer->beginRenderPass(m_renderPass, vk::SubpassContents::eInline);
    m_bClearSubmitted = true;
    m_bRenderPassActive = true;
  }

  if (m_bRenderPassActive)
  {
    m_pCommandBuffer->endRenderPass();
    m_bRenderPassActive = false;
  }

  m_depthMask = {};
  m_uiLayers = 0;
  m_PipelineDesc.m_msaa = wdGALMSAASampleCount::None;
  m_PipelineDesc.m_renderPass = nullptr;
  m_frameBuffer = nullptr;
}

void wdGALCommandEncoderImplVulkan::ClearPlatform(const wdColor& ClearColor, wdUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, wdUInt8 uiStencilClear)
{
  if (!m_bRenderPassActive && !m_bInsideCompute)
  {
    m_pCommandBuffer->beginRenderPass(m_renderPass, vk::SubpassContents::eInline);
    m_bClearSubmitted = true;
    m_bRenderPassActive = true;
  }
  //#TODO_VULKAN Not sure if we need barriers here.
  wdHybridArray<vk::ClearAttachment, 8> attachments;

  // Clear color
  if (uiRenderTargetClearMask != 0)
  {
    for (wdUInt32 i = 0; i < WD_GAL_MAX_RENDERTARGET_COUNT; i++)
    {
      if (uiRenderTargetClearMask & (1u << i) && i < m_PipelineDesc.m_uiAttachmentCount)
      {
        vk::ClearAttachment& attachment = attachments.ExpandAndGetRef();
        attachment.aspectMask = vk::ImageAspectFlagBits::eColor;
        attachment.clearValue.color.setFloat32({ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a});
        attachment.colorAttachment = i;
      }
    }
  }
  // Clear depth / stencil
  if (bClearDepth || bClearStencil)
  {
    vk::ClearAttachment& attachment = attachments.ExpandAndGetRef();
    if (bClearDepth && (m_depthMask & vk::ImageAspectFlagBits::eDepth))
    {
      attachment.aspectMask |= vk::ImageAspectFlagBits::eDepth;
      attachment.clearValue.depthStencil.setDepth(fDepthClear);
    }
    if (bClearStencil && (m_depthMask & vk::ImageAspectFlagBits::eStencil))
    {
      attachment.aspectMask |= vk::ImageAspectFlagBits::eStencil;
      attachment.clearValue.depthStencil.setStencil(uiStencilClear);
    }
  }

  vk::ClearRect rect;
  rect.baseArrayLayer = 0;
  rect.layerCount = m_uiLayers;
  rect.rect = m_renderPass.renderArea;
  m_pCommandBuffer->clearAttachments(attachments.GetCount(), attachments.GetData(), 1, &rect);
}

// Draw functions

void wdGALCommandEncoderImplVulkan::DrawPlatform(wdUInt32 uiVertexCount, wdUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

  m_pCommandBuffer->draw(uiVertexCount, 1, uiStartVertex, 0);
}

void wdGALCommandEncoderImplVulkan::DrawIndexedPlatform(wdUInt32 uiIndexCount, wdUInt32 uiStartIndex)
{
  FlushDeferredStateChanges();

  m_pCommandBuffer->drawIndexed(uiIndexCount, 1, uiStartIndex, 0, 0);
}

void wdGALCommandEncoderImplVulkan::DrawIndexedInstancedPlatform(wdUInt32 uiIndexCountPerInstance, wdUInt32 uiInstanceCount, wdUInt32 uiStartIndex)
{
  FlushDeferredStateChanges();

  m_pCommandBuffer->drawIndexed(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex, 0, 0);
}

void wdGALCommandEncoderImplVulkan::DrawIndexedInstancedIndirectPlatform(const wdGALBuffer* pIndirectArgumentBuffer, wdUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  m_pCommandBuffer->drawIndexedIndirect(static_cast<const wdGALBufferVulkan*>(pIndirectArgumentBuffer)->GetVkBuffer(), uiArgumentOffsetInBytes, 1, 0);
}

void wdGALCommandEncoderImplVulkan::DrawInstancedPlatform(wdUInt32 uiVertexCountPerInstance, wdUInt32 uiInstanceCount, wdUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

  m_pCommandBuffer->draw(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex, 0);
}

void wdGALCommandEncoderImplVulkan::DrawInstancedIndirectPlatform(const wdGALBuffer* pIndirectArgumentBuffer, wdUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  m_pCommandBuffer->drawIndirect(static_cast<const wdGALBufferVulkan*>(pIndirectArgumentBuffer)->GetVkBuffer(), uiArgumentOffsetInBytes, 1, 0);
}

void wdGALCommandEncoderImplVulkan::DrawAutoPlatform()
{
  //FlushDeferredStateChanges();

  WD_ASSERT_NOT_IMPLEMENTED;
}

void wdGALCommandEncoderImplVulkan::BeginStreamOutPlatform()
{
  FlushDeferredStateChanges();
}

void wdGALCommandEncoderImplVulkan::EndStreamOutPlatform()
{
  WD_ASSERT_NOT_IMPLEMENTED;
}

void wdGALCommandEncoderImplVulkan::SetIndexBufferPlatform(const wdGALBuffer* pIndexBuffer)
{
  if (m_pIndexBuffer != pIndexBuffer)
  {
    m_pIndexBuffer = static_cast<const wdGALBufferVulkan*>(pIndexBuffer);
    m_bIndexBufferDirty = true;
  }
}

void wdGALCommandEncoderImplVulkan::SetVertexBufferPlatform(wdUInt32 uiSlot, const wdGALBuffer* pVertexBuffer)
{
  WD_ASSERT_DEV(uiSlot < WD_GAL_MAX_VERTEX_BUFFER_COUNT, "Invalid slot index");
  vk::Buffer buffer = pVertexBuffer != nullptr ? static_cast<const wdGALBufferVulkan*>(pVertexBuffer)->GetVkBuffer() : nullptr;
  wdUInt32 stride = pVertexBuffer != nullptr ? pVertexBuffer->GetDescription().m_uiStructSize : 0;

  if (buffer != m_pBoundVertexBuffers[uiSlot])
  {
    m_pBoundVertexBuffers[uiSlot] = buffer;
    m_BoundVertexBuffersRange.SetToIncludeValue(uiSlot);

    if (m_PipelineDesc.m_VertexBufferStrides[uiSlot] != stride)
    {
      m_PipelineDesc.m_VertexBufferStrides[uiSlot] = stride;
      m_bPipelineStateDirty = true;
    }
  }
}

void wdGALCommandEncoderImplVulkan::SetVertexDeclarationPlatform(const wdGALVertexDeclaration* pVertexDeclaration)
{
  if (m_PipelineDesc.m_pCurrentVertexDecl != pVertexDeclaration)
  {
    m_PipelineDesc.m_pCurrentVertexDecl = static_cast<const wdGALVertexDeclarationVulkan*>(pVertexDeclaration);
    m_bPipelineStateDirty = true;
  }
}

void wdGALCommandEncoderImplVulkan::SetPrimitiveTopologyPlatform(wdGALPrimitiveTopology::Enum Topology)
{
  if (m_PipelineDesc.m_topology != Topology)
  {
    m_PipelineDesc.m_topology = Topology;
    m_bPipelineStateDirty = true;
  }
}

void wdGALCommandEncoderImplVulkan::SetBlendStatePlatform(const wdGALBlendState* pBlendState, const wdColor& BlendFactor, wdUInt32 uiSampleMask)
{
  //#TODO_VULKAN BlendFactor / uiSampleMask ?
  if (m_PipelineDesc.m_pCurrentBlendState != pBlendState)
  {
    m_PipelineDesc.m_pCurrentBlendState = pBlendState != nullptr ? static_cast<const wdGALBlendStateVulkan*>(pBlendState) : nullptr;
    m_bPipelineStateDirty = true;
  }
}

void wdGALCommandEncoderImplVulkan::SetDepthStencilStatePlatform(const wdGALDepthStencilState* pDepthStencilState, wdUInt8 uiStencilRefValue)
{
  //#TODO_VULKAN uiStencilRefValue ?
  if (m_PipelineDesc.m_pCurrentDepthStencilState != pDepthStencilState)
  {
    m_PipelineDesc.m_pCurrentDepthStencilState = pDepthStencilState != nullptr ? static_cast<const wdGALDepthStencilStateVulkan*>(pDepthStencilState) : nullptr;
    m_bPipelineStateDirty = true;
  }
}

void wdGALCommandEncoderImplVulkan::SetRasterizerStatePlatform(const wdGALRasterizerState* pRasterizerState)
{
  if (m_PipelineDesc.m_pCurrentRasterizerState != pRasterizerState)
  {
    m_PipelineDesc.m_pCurrentRasterizerState = pRasterizerState != nullptr ? static_cast<const wdGALRasterizerStateVulkan*>(pRasterizerState) : nullptr;
    if (m_PipelineDesc.m_pCurrentRasterizerState->GetDescription().m_bScissorTest != m_bScissorEnabled)
    {
      m_bScissorEnabled = m_PipelineDesc.m_pCurrentRasterizerState->GetDescription().m_bScissorTest;
      m_bViewportDirty = true;
    }
    m_bPipelineStateDirty = true;
  }
}

void wdGALCommandEncoderImplVulkan::SetViewportPlatform(const wdRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  // We use wdClipSpaceYMode::Regular and rely in the Vulkan 1.1 feature that a negative height performs y-inversion of the clip-space to framebuffer-space transform.
  // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VK_KHR_maintenance1.html
  vk::Viewport viewport = {rect.x, rect.height + rect.y, rect.width, -rect.height, fMinDepth, fMaxDepth};
  if (m_viewport != viewport)
  {
    // Viewport is marked as dynamic in the pipeline layout and thus does not mark m_bPipelineStateDirty.
    m_viewport = viewport;
    m_bViewportDirty = true;
  }
}

void wdGALCommandEncoderImplVulkan::SetScissorRectPlatform(const wdRectU32& rect)
{
  vk::Rect2D scissor(vk::Offset2D(rect.x, rect.y), vk::Extent2D(rect.width, rect.height));
  if (m_scissor != scissor)
  {
    // viewport is marked as dynamic in the pipeline layout and thus does not mark m_bPipelineStateDirty.
    m_scissor = scissor;
    m_bViewportDirty = true;
  }
}

void wdGALCommandEncoderImplVulkan::SetStreamOutBufferPlatform(wdUInt32 uiSlot, const wdGALBuffer* pBuffer, wdUInt32 uiOffset)
{
  WD_ASSERT_NOT_IMPLEMENTED;
}

//////////////////////////////////////////////////////////////////////////

void wdGALCommandEncoderImplVulkan::BeginCompute()
{
  m_bClearSubmitted = true;
  m_bInsideCompute = true;
  m_bPipelineStateDirty = true;
}

void wdGALCommandEncoderImplVulkan::EndCompute()
{
  m_bInsideCompute = false;
}

void wdGALCommandEncoderImplVulkan::DispatchPlatform(wdUInt32 uiThreadGroupCountX, wdUInt32 uiThreadGroupCountY, wdUInt32 uiThreadGroupCountZ)
{
  FlushDeferredStateChanges();
  m_pCommandBuffer->dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

void wdGALCommandEncoderImplVulkan::DispatchIndirectPlatform(const wdGALBuffer* pIndirectArgumentBuffer, wdUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();
  m_pCommandBuffer->dispatchIndirect(static_cast<const wdGALBufferVulkan*>(pIndirectArgumentBuffer)->GetVkBuffer(), uiArgumentOffsetInBytes);
}

//////////////////////////////////////////////////////////////////////////

#if 0
static void SetShaderResources(wdGALShaderStage::Enum stage, ID3D11DeviceContext* pContext, wdUInt32 uiStartSlot, wdUInt32 uiNumSlots,
  ID3D11ShaderResourceView** pShaderResourceViews)
{
  switch (stage)
  {
    case wdGALShaderStage::VertexShader:
      pContext->VSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case wdGALShaderStage::HullShader:
      pContext->HSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case wdGALShaderStage::DomainShader:
      pContext->DSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case wdGALShaderStage::GeometryShader:
      pContext->GSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case wdGALShaderStage::PixelShader:
      pContext->PSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case wdGALShaderStage::ComputeShader:
      pContext->CSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    default:
      WD_ASSERT_NOT_IMPLEMENTED;
  }
}

static void SetConstantBuffers(wdGALShaderStage::Enum stage, ID3D11DeviceContext* pContext, wdUInt32 uiStartSlot, wdUInt32 uiNumSlots,
  ID3D11Buffer** pConstantBuffers)
{
  switch (stage)
  {
    case wdGALShaderStage::VertexShader:
      pContext->VSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case wdGALShaderStage::HullShader:
      pContext->HSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case wdGALShaderStage::DomainShader:
      pContext->DSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case wdGALShaderStage::GeometryShader:
      pContext->GSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case wdGALShaderStage::PixelShader:
      pContext->PSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case wdGALShaderStage::ComputeShader:
      pContext->CSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    default:
      WD_ASSERT_NOT_IMPLEMENTED;
  }
}

static void SetSamplers(wdGALShaderStage::Enum stage, ID3D11DeviceContext* pContext, wdUInt32 uiStartSlot, wdUInt32 uiNumSlots,
  ID3D11SamplerState** pSamplerStates)
{
  switch (stage)
  {
    case wdGALShaderStage::VertexShader:
      pContext->VSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case wdGALShaderStage::HullShader:
      pContext->HSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case wdGALShaderStage::DomainShader:
      pContext->DSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case wdGALShaderStage::GeometryShader:
      pContext->GSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case wdGALShaderStage::PixelShader:
      pContext->PSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case wdGALShaderStage::ComputeShader:
      pContext->CSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    default:
      WD_ASSERT_NOT_IMPLEMENTED;
  }
}
#endif

void wdGALCommandEncoderImplVulkan::FlushDeferredStateChanges()
{


  if (m_bPipelineStateDirty)
  {
    if (!m_PipelineDesc.m_pCurrentShader)
    {
      wdLog::Error("No shader set");
      return;
    }
    const wdGALShaderVulkan::DescriptorSetLayoutDesc& descriptorLayoutDesc = m_PipelineDesc.m_pCurrentShader->GetDescriptorSetLayout();

    m_LayoutDesc.m_layout = wdResourceCacheVulkan::RequestDescriptorSetLayout(descriptorLayoutDesc);
    m_PipelineDesc.m_layout = wdResourceCacheVulkan::RequestPipelineLayout(m_LayoutDesc);
    m_ComputeDesc.m_layout = m_PipelineDesc.m_layout;

    vk::Pipeline pipeline;
    if (m_bInsideCompute)
    {
      pipeline = wdResourceCacheVulkan::RequestComputePipeline(m_ComputeDesc);
    }
    else
    {
      pipeline = wdResourceCacheVulkan::RequestGraphicsPipeline(m_PipelineDesc);
    }

    m_pCommandBuffer->bindPipeline(m_bInsideCompute ? vk::PipelineBindPoint::eCompute : vk::PipelineBindPoint::eGraphics, pipeline);
    m_bPipelineStateDirty = false;
    // Changes to the descriptor layout always require the descriptor set to be re-created.
    m_bDescriptorsDirty = true;
  }

  if (!m_bInsideCompute && m_bViewportDirty)
  {
    m_pCommandBuffer->setViewport(0, 1, &m_viewport);
    if (m_bScissorEnabled)
      m_pCommandBuffer->setScissor(0, 1, &m_scissor);
    else
    {
      vk::Rect2D noScissor({int(m_viewport.x), int(m_viewport.y + m_viewport.height)}, {wdUInt32(m_viewport.width), wdUInt32(-m_viewport.height)});
      m_pCommandBuffer->setScissor(0, 1, &noScissor);
    }
    m_bViewportDirty = false;
  }

  if (!m_bInsideCompute && m_BoundVertexBuffersRange.IsValid())
  {
    const wdUInt32 uiStartSlot = m_BoundVertexBuffersRange.m_uiMin;
    const wdUInt32 uiNumSlots = m_BoundVertexBuffersRange.GetCount();

    wdUInt32 uiCurrentStartSlot = uiStartSlot;
    // Finding valid ranges.
    for (wdUInt32 i = uiStartSlot; i < (uiStartSlot + uiNumSlots); i++)
    {
      if (!m_pBoundVertexBuffers[i])
      {
        if (i - uiCurrentStartSlot > 0)
        {
          // There are some null elements in the array. We can't submit these to Vulkan and need to skip them so flush everything before it.
          m_pCommandBuffer->bindVertexBuffers(uiCurrentStartSlot, i - uiCurrentStartSlot, m_pBoundVertexBuffers + uiCurrentStartSlot, m_VertexBufferOffsets + uiCurrentStartSlot);
        }
        uiCurrentStartSlot = i + 1;
      }
    }
    // The last element in the buffer range must always be valid so we can simply flush the rest.
    if (m_pBoundVertexBuffers[uiCurrentStartSlot])
      m_pCommandBuffer->bindVertexBuffers(uiCurrentStartSlot, m_BoundVertexBuffersRange.m_uiMax - uiCurrentStartSlot + 1, m_pBoundVertexBuffers + uiCurrentStartSlot, m_VertexBufferOffsets + uiCurrentStartSlot);

    m_BoundVertexBuffersRange.Reset();
  }

  if (!m_bInsideCompute && m_bIndexBufferDirty)
  {
    if (m_pIndexBuffer)
      m_pCommandBuffer->bindIndexBuffer(m_pIndexBuffer->GetVkBuffer(), 0, m_pIndexBuffer->GetIndexType());
    m_bIndexBufferDirty = false;
  }

  if (true /*m_bDescriptorsDirty*/)
  {
    //#TODO_VULKAN we always create a new descriptor set as we don't know if a buffer was modified since the last draw call (wdGALBufferVulkan::DiscardBuffer).
    // Need to figure out a fast check if any buffer or buffer of a resource view was discarded.
    m_bDescriptorsDirty = false;

    m_DescriptorWrites.Clear();
    vk::DescriptorSet descriptorSet = wdDescriptorSetPoolVulkan::CreateDescriptorSet(m_LayoutDesc.m_layout);

    wdArrayPtr<const wdGALShaderVulkan::BindingMapping> bindingMapping = m_PipelineDesc.m_pCurrentShader->GetBindingMapping();
    const wdUInt32 uiCount = bindingMapping.GetCount();
    for (wdUInt32 i = 0; i < uiCount; i++)
    {
      const wdGALShaderVulkan::BindingMapping& mapping = bindingMapping[i];
      vk::WriteDescriptorSet& write = m_DescriptorWrites.ExpandAndGetRef();
      write.dstArrayElement = 0;
      write.descriptorType = mapping.m_descriptorType;
      write.dstBinding = mapping.m_uiTarget;
      write.dstSet = descriptorSet;
      write.descriptorCount = 1;
      switch (mapping.m_type)
      {
        case wdGALShaderVulkan::BindingMapping::ConstantBuffer:
        {
          const wdGALBufferVulkan* pBuffer = m_pBoundConstantBuffers[mapping.m_uiSource];
          write.pBufferInfo = &pBuffer->GetBufferInfo();
        }
        break;
        case wdGALShaderVulkan::BindingMapping::ResourceView:
        {
          const wdGALResourceViewVulkan* pResourceView = nullptr;
          if (mapping.m_uiSource < m_pBoundShaderResourceViews[mapping.m_stage].GetCount())
          {
            pResourceView = m_pBoundShaderResourceViews[mapping.m_stage][mapping.m_uiSource];
          }

          if (!pResourceView)
          {
            wdStringBuilder bla = mapping.m_sName;
            bool bDepth = bla.FindSubString_NoCase("shadow") != nullptr || bla.FindSubString_NoCase("depth");
            pResourceView = wdFallbackResourcesVulkan::GetFallbackResourceView(mapping.m_descriptorType, mapping.m_wdType, bDepth);
          }
          if (!pResourceView->GetDescription().m_hTexture.IsInvalidated())
          {
            write.pImageInfo = &pResourceView->GetImageInfo(wdShaderResourceType::IsArray(mapping.m_wdType));

            const wdGALTextureVulkan* pTexture = static_cast<const wdGALTextureVulkan*>(pResourceView->GetResource()->GetParentResource());
            const bool bIsDepth = wdGALResourceFormat::IsDepthFormat(pTexture->GetDescription().m_Format);

            m_pPipelineBarrier->EnsureImageLayout(pResourceView, pTexture->GetPreferredLayout(bIsDepth ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eShaderReadOnlyOptimal), mapping.m_targetStages, vk::AccessFlagBits::eShaderRead);
          }
          else
          {
            if (auto& bufferView = pResourceView->GetBufferView())
            {
              write.pTexelBufferView = &bufferView;
            }
            else
            {
              write.pBufferInfo = &pResourceView->GetBufferInfo();
            }
          }
        }
        break;
        case wdGALShaderVulkan::BindingMapping::UAV:
        {
          const wdGALUnorderedAccessViewVulkan* pUAV = m_pBoundUnoderedAccessViews[mapping.m_uiSource];
          if (!pUAV->GetDescription().m_hTexture.IsInvalidated())
          {
            write.pImageInfo = &pUAV->GetImageInfo();

            const wdGALTextureVulkan* pTexture = static_cast<const wdGALTextureVulkan*>(pUAV->GetResource()->GetParentResource());
            m_pPipelineBarrier->EnsureImageLayout(pUAV, pTexture->GetPreferredLayout(vk::ImageLayout::eGeneral), mapping.m_targetStages, vk::AccessFlagBits::eShaderRead);
          }
          else
          {
            write.pBufferInfo = &pUAV->GetBufferInfo();
          }
        }
        break;
        case wdGALShaderVulkan::BindingMapping::Sampler:
        {
          const wdGALSamplerStateVulkan* pSampler = m_pBoundSamplerStates[mapping.m_stage][mapping.m_uiSource];
          write.pImageInfo = &pSampler->GetImageInfo();
        }
        break;
        default:
          break;
      }
    }

    wdDescriptorSetPoolVulkan::UpdateDescriptorSet(descriptorSet, m_DescriptorWrites);
    m_pCommandBuffer->bindDescriptorSets(m_bInsideCompute ? vk::PipelineBindPoint::eCompute : vk::PipelineBindPoint::eGraphics, m_PipelineDesc.m_layout, 0, 1, &descriptorSet, 0, nullptr);
  }

  if (m_bRenderPassActive && m_pPipelineBarrier->IsDirty())
  {
    m_pCommandBuffer->endRenderPass();
    m_bRenderPassActive = false;
  }
  m_pPipelineBarrier->Flush();

  if (!m_bRenderPassActive && !m_bInsideCompute)
  {
    m_pCommandBuffer->beginRenderPass(m_renderPass, vk::SubpassContents::eInline);
    m_bClearSubmitted = true;
    m_bRenderPassActive = true;
  }
}
