#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Memory/MemoryUtils.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/InitContext.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>
#include <RendererVulkan/Utils/PipelineBarrierVulkan.h>

vk::Extent3D wdGALTextureVulkan::GetMipLevelSize(wdUInt32 uiMipLevel) const
{
  vk::Extent3D size = {m_Description.m_uiWidth, m_Description.m_uiHeight, m_Description.m_uiDepth};
  size.width = wdMath::Max(1u, size.width >> uiMipLevel);
  size.height = wdMath::Max(1u, size.height >> uiMipLevel);
  size.depth = wdMath::Max(1u, size.depth >> uiMipLevel);
  return size;
}

vk::ImageSubresourceRange wdGALTextureVulkan::GetFullRange() const
{
  vk::ImageSubresourceRange range;
  range.aspectMask = GetAspectMask();
  range.baseArrayLayer = 0;
  range.baseMipLevel = 0;
  range.layerCount = m_Description.m_Type == wdGALTextureType::TextureCube ? m_Description.m_uiArraySize * 6 : m_Description.m_uiArraySize;
  range.levelCount = m_Description.m_uiMipLevelCount;
  return range;
}

vk::ImageAspectFlags wdGALTextureVulkan::GetAspectMask() const
{
  vk::ImageAspectFlags mask = wdConversionUtilsVulkan::IsDepthFormat(m_imageFormat) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  if (wdConversionUtilsVulkan::IsStencilFormat(m_imageFormat))
    mask |= vk::ImageAspectFlagBits::eStencil;
  return mask;
}

wdGALTextureVulkan::wdGALTextureVulkan(const wdGALTextureCreationDescription& Description)
  : wdGALTexture(Description)
  , m_image(nullptr)
  , m_pExisitingNativeObject(Description.m_pExisitingNativeObject)
{
}

wdGALTextureVulkan::wdGALTextureVulkan(const wdGALTextureCreationDescription& Description, vk::Format OverrideFormat, bool bLinearCPU)
  : wdGALTexture(Description)
  , m_image(nullptr)
  , m_pExisitingNativeObject(Description.m_pExisitingNativeObject)
  , m_imageFormat(OverrideFormat)
  , m_formatOverride(true)
  , m_bLinearCPU(bLinearCPU)
{
}

wdGALTextureVulkan::~wdGALTextureVulkan() {}

wdResult wdGALTextureVulkan::InitPlatform(wdGALDevice* pDevice, wdArrayPtr<wdGALSystemMemoryDescription> pInitialData)
{
  m_pDevice = static_cast<wdGALDeviceVulkan*>(pDevice);

  vk::ImageCreateInfo createInfo = {};
  //#TODO_VULKAN VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT / VkImageFormatListCreateInfoKHR to allow changing the format in a view is slow.
  createInfo.flags |= vk::ImageCreateFlagBits::eMutableFormat;
  if (m_imageFormat == vk::Format::eUndefined)
  {
    m_imageFormat = m_pDevice->GetFormatLookupTable().GetFormatInfo(m_Description.m_Format).m_eResourceViewType;
  }

  if ((m_imageFormat == vk::Format::eR8G8B8A8Srgb || m_imageFormat == vk::Format::eB8G8R8A8Unorm) && m_Description.m_bCreateRenderTarget)
  {
    // printf("");
  }
  createInfo.format = m_imageFormat;
  if (createInfo.format == vk::Format::eUndefined)
  {
    wdLog::Error("No storage format available for given format: {0}", m_Description.m_Format);
    return WD_FAILURE;
  }
  const bool bIsDepth = wdConversionUtilsVulkan::IsDepthFormat(m_imageFormat);

  m_stages = vk::PipelineStageFlagBits::eTransfer;
  m_access = vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite;
  m_preferredLayout = vk::ImageLayout::eGeneral;

  createInfo.initialLayout = vk::ImageLayout::eUndefined;
  createInfo.sharingMode = vk::SharingMode::eExclusive;
  createInfo.pQueueFamilyIndices = nullptr;
  createInfo.queueFamilyIndexCount = 0;
  createInfo.tiling = vk::ImageTiling::eOptimal;
  createInfo.usage |= vk::ImageUsageFlagBits::eTransferDst;
  if (m_Description.m_ResourceAccess.m_bReadBack)
    createInfo.usage |= vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled;

  createInfo.extent.width = m_Description.m_uiWidth;
  createInfo.extent.height = m_Description.m_uiHeight;
  createInfo.extent.depth = m_Description.m_uiDepth;
  createInfo.mipLevels = m_Description.m_uiMipLevelCount;

  createInfo.samples = static_cast<vk::SampleCountFlagBits>(m_Description.m_SampleCount.GetValue());

  // m_bAllowDynamicMipGeneration has to be emulated via a shader so we need to enable shader resource view and render target support.
  if (m_Description.m_bAllowShaderResourceView || m_Description.m_bAllowDynamicMipGeneration)
  {
    // Needed for blit-based generation
    createInfo.usage |= vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc;
    // Needed for shader-based generation
    createInfo.usage |= vk::ImageUsageFlagBits::eSampled;
    m_stages |= m_pDevice->GetSupportedStages();
    m_access |= vk::AccessFlagBits::eShaderRead;
    m_preferredLayout = bIsDepth ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eShaderReadOnlyOptimal;
  }
  //VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT
  if (m_Description.m_bCreateRenderTarget || m_Description.m_bAllowDynamicMipGeneration)
  {
    if (bIsDepth)
    {
      createInfo.usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
      m_stages |= vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
      m_access |= vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
      m_preferredLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    }
    else
    {
      createInfo.usage |= vk::ImageUsageFlagBits::eColorAttachment;
      m_stages |= vk::PipelineStageFlagBits::eColorAttachmentOutput;
      m_access |= vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
      m_preferredLayout = vk::ImageLayout::eColorAttachmentOptimal;
    }
  }

  if (m_Description.m_bAllowUAV)
  {
    createInfo.usage |= vk::ImageUsageFlagBits::eStorage;
    m_stages |= m_pDevice->GetSupportedStages();
    m_access |= vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
    m_preferredLayout = vk::ImageLayout::eGeneral;
  }

  switch (m_Description.m_Type)
  {
    case wdGALTextureType::Texture2D:
    case wdGALTextureType::TextureCube:
    {
      createInfo.imageType = vk::ImageType::e2D;
      createInfo.arrayLayers = (m_Description.m_Type == wdGALTextureType::Texture2D ? m_Description.m_uiArraySize : (m_Description.m_uiArraySize * 6));

      if (m_Description.m_Type == wdGALTextureType::TextureCube)
        createInfo.flags |= vk::ImageCreateFlagBits::eCubeCompatible;
    }
    break;

    case wdGALTextureType::Texture3D:
    {
      createInfo.arrayLayers = 1;
      createInfo.imageType = vk::ImageType::e3D;
      if (m_Description.m_bCreateRenderTarget)
      {
        createInfo.flags |= vk::ImageCreateFlagBits::e2DArrayCompatible;
      }
    }
    break;

    default:
      WD_ASSERT_NOT_IMPLEMENTED;
      return WD_FAILURE;
  }

  if (m_pExisitingNativeObject == nullptr)
  {
    wdVulkanAllocationCreateInfo allocInfo;
    allocInfo.m_usage = wdVulkanMemoryUsage::Auto;
    if (m_bLinearCPU)
    {
      createInfo.tiling = vk::ImageTiling::eLinear;
      createInfo.usage |= vk::ImageUsageFlagBits::eTransferSrc;
      m_stages |= vk::PipelineStageFlagBits::eHost;
      m_access |= vk::AccessFlagBits::eHostRead;
      createInfo.flags = {}; // Clear all flags as we don't need them and they usually are not supported on NVidia in linear mode.

      allocInfo.m_flags = wdVulkanAllocationCreateFlags::HostAccessRandom;
    }
    vk::ImageFormatProperties props2;
    VK_ASSERT_DEBUG(m_pDevice->GetVulkanPhysicalDevice().getImageFormatProperties(createInfo.format, createInfo.imageType, createInfo.tiling, createInfo.usage, createInfo.flags, &props2));

    VK_SUCCEED_OR_RETURN_WD_FAILURE(wdMemoryAllocatorVulkan::CreateImage(createInfo, allocInfo, m_image, m_alloc, &m_allocInfo));
  }
  else
  {
    m_image = static_cast<VkImage>(m_pExisitingNativeObject);
  }
  m_pDevice->GetInitContext().InitTexture(this, createInfo, pInitialData);

  if (m_Description.m_ResourceAccess.m_bReadBack)
  {
    return CreateStagingBuffer(createInfo);
  }

  return WD_SUCCESS;
}

wdGALTextureVulkan::StagingMode wdGALTextureVulkan::ComputeStagingMode(const vk::ImageCreateInfo& createInfo) const
{
  if (!m_Description.m_ResourceAccess.m_bReadBack)
    return StagingMode::None;

  // We want the staging texture to always have the intended format and not the override format given by the parent texture.
  vk::Format stagingFormat = m_pDevice->GetFormatLookupTable().GetFormatInfo(m_Description.m_Format).m_eStorage;

  WD_ASSERT_DEV(!wdConversionUtilsVulkan::IsStencilFormat(m_imageFormat), "Stencil read-back not implemented.");
  WD_ASSERT_DEV(!wdConversionUtilsVulkan::IsDepthFormat(stagingFormat), "Depth read-back should use a color format for CPU staging.");

  const vk::FormatProperties srcFormatProps = m_pDevice->GetVulkanPhysicalDevice().getFormatProperties(m_imageFormat);
  const vk::FormatProperties dstFormatProps = m_pDevice->GetVulkanPhysicalDevice().getFormatProperties(stagingFormat);

  const bool bFormatsEqual = m_imageFormat == stagingFormat && createInfo.samples == vk::SampleCountFlagBits::e1;
  const bool bSupportsCopy = bFormatsEqual && (srcFormatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eTransferSrc) && (dstFormatProps.linearTilingFeatures & vk::FormatFeatureFlagBits::eTransferDst);
  if (bFormatsEqual)
  {
    WD_ASSERT_DEV(srcFormatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eTransferSrc, "Source format can't be read, readback impossible.");
    return StagingMode::Buffer;
  }
  else
  {
    vk::ImageFormatProperties props;
    vk::Result res = m_pDevice->GetVulkanPhysicalDevice().getImageFormatProperties(stagingFormat, createInfo.imageType, vk::ImageTiling::eLinear, vk::ImageUsageFlagBits::eColorAttachment, {}, &props);

    const bool bCanUseDirectTexture = (res == vk::Result::eSuccess) && createInfo.arrayLayers <= props.maxArrayLayers && createInfo.mipLevels <= props.maxMipLevels && createInfo.extent.depth <= props.maxExtent.depth && createInfo.extent.width <= props.maxExtent.width && createInfo.extent.height <= props.maxExtent.height && (createInfo.samples & props.sampleCounts);
    return bCanUseDirectTexture ? StagingMode::Texture : StagingMode::TextureAndBuffer;
  }
}

wdUInt32 wdGALTextureVulkan::ComputeSubResourceOffsets(wdDynamicArray<SubResourceOffset>& subResourceSizes) const
{
  const wdUInt32 alignment = (wdUInt32)wdGALBufferVulkan::GetAlignment(m_pDevice, vk::BufferUsageFlagBits::eTransferDst);
  const vk::Format stagingFormat = m_pDevice->GetFormatLookupTable().GetFormatInfo(m_Description.m_Format).m_eStorage;
  const wdUInt8 uiBlockSize = vk::blockSize(stagingFormat);
  const auto blockExtent = vk::blockExtent(stagingFormat);
  const wdUInt32 arrayLayers = (m_Description.m_Type == wdGALTextureType::TextureCube ? (m_Description.m_uiArraySize * 6) : m_Description.m_uiArraySize);
  const wdUInt32 mipLevels = m_Description.m_uiMipLevelCount;

  subResourceSizes.Reserve(arrayLayers * mipLevels);
  wdUInt32 uiOffset = 0;
  for (wdUInt32 uiLayer = 0; uiLayer < arrayLayers; uiLayer++)
  {
    for (wdUInt32 uiMipLevel = 0; uiMipLevel < mipLevels; uiMipLevel++)
    {
      const wdUInt32 uiSubresourceIndex = uiMipLevel + uiLayer * mipLevels;
      WD_ASSERT_DEBUG(subResourceSizes.GetCount() == uiSubresourceIndex, "");

      const vk::Extent3D imageExtent = GetMipLevelSize(uiMipLevel);
      const VkExtent3D blockCount = {
        (imageExtent.width + blockExtent[0] - 1) / blockExtent[0],
        (imageExtent.height + blockExtent[1] - 1) / blockExtent[1],
        (imageExtent.depth + blockExtent[2] - 1) / blockExtent[2]};

      const wdUInt32 uiTotalSize = uiBlockSize * blockCount.width * blockCount.height * blockCount.depth;
      subResourceSizes.PushBack({uiOffset, uiTotalSize, blockCount.width / blockExtent[0], blockCount.height / blockExtent[1]});
      uiOffset += wdMemoryUtils::AlignSize(uiTotalSize, alignment);
    }
  }
  return uiOffset;
}

wdResult wdGALTextureVulkan::CreateStagingBuffer(const vk::ImageCreateInfo& createInfo)
{
  m_stagingMode = wdGALTextureVulkan::ComputeStagingMode(createInfo);
  if (m_stagingMode == StagingMode::Texture || m_stagingMode == StagingMode::TextureAndBuffer)
  {
    wdGALTextureCreationDescription stagingDesc = m_Description;
    stagingDesc.m_SampleCount = wdGALMSAASampleCount::None;
    stagingDesc.m_bAllowShaderResourceView = false;
    stagingDesc.m_bAllowUAV = false;
    stagingDesc.m_bCreateRenderTarget = true;
    stagingDesc.m_bAllowDynamicMipGeneration = false;
    stagingDesc.m_ResourceAccess.m_bReadBack = false;
    stagingDesc.m_ResourceAccess.m_bImmutable = false;
    stagingDesc.m_pExisitingNativeObject = nullptr;

    const bool bLinearCPU = m_stagingMode == StagingMode::Texture;
    const vk::Format stagingFormat = m_pDevice->GetFormatLookupTable().GetFormatInfo(m_Description.m_Format).m_eStorage;

    m_hStagingTexture = m_pDevice->CreateTextureInternal(stagingDesc, {}, stagingFormat, bLinearCPU);
    if (m_hStagingTexture.IsInvalidated())
    {
      wdLog::Error("Failed to create staging texture for read-back");
      return WD_FAILURE;
    }
  }
  if (m_stagingMode == StagingMode::Buffer || m_stagingMode == StagingMode::TextureAndBuffer)
  {
    wdGALBufferCreationDescription stagingBuffer;
    stagingBuffer.m_BufferType = wdGALBufferType::Generic;

    wdHybridArray<SubResourceOffset, 8> subResourceSizes;
    stagingBuffer.m_uiTotalSize = ComputeSubResourceOffsets(subResourceSizes);
    stagingBuffer.m_uiStructSize = 1;
    stagingBuffer.m_bAllowRawViews = true;
    stagingBuffer.m_ResourceAccess.m_bImmutable = false;

    m_hStagingBuffer = m_pDevice->CreateBufferInternal(stagingBuffer, {}, true);
    if (m_hStagingBuffer.IsInvalidated())
    {
      wdLog::Error("Failed to create staging buffer for read-back");
      return WD_FAILURE;
    }
  }

  return WD_SUCCESS;
}
wdResult wdGALTextureVulkan::DeInitPlatform(wdGALDevice* pDevice)
{
  wdGALDeviceVulkan* pVulkanDevice = static_cast<wdGALDeviceVulkan*>(pDevice);
  if (m_image && !m_pExisitingNativeObject)
  {
    pVulkanDevice->DeleteLater(m_image, m_alloc);
  }
  m_image = nullptr;

  if (!m_hStagingTexture.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hStagingTexture);
    m_hStagingTexture.Invalidate();
  }
  if (!m_hStagingBuffer.IsInvalidated())
  {
    pDevice->DestroyBuffer(m_hStagingBuffer);
    m_hStagingBuffer.Invalidate();
  }
  return WD_SUCCESS;
}

void wdGALTextureVulkan::SetDebugNamePlatform(const char* szName) const
{
  m_pDevice->SetDebugName(szName, m_image, m_alloc);
  if (!m_hStagingTexture.IsInvalidated())
  {
    auto pStagingTexture = static_cast<const wdGALTextureVulkan*>(m_pDevice->GetTexture(m_hStagingTexture));
    pStagingTexture->SetDebugName(szName);
  }
  if (!m_hStagingBuffer.IsInvalidated())
  {
    auto pStagingBuffer = static_cast<const wdGALBufferVulkan*>(m_pDevice->GetBuffer(m_hStagingBuffer));
    pStagingBuffer->SetDebugName(szName);
  }
}



WD_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_TextureVulkan);
