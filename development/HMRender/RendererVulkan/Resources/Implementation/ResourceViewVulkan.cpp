#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/ResourceViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

bool IsArrayView(const wdGALTextureCreationDescription& texDesc, const wdGALResourceViewCreationDescription& viewDesc)
{
  return texDesc.m_uiArraySize > 1 || viewDesc.m_uiArraySize > 1;
}

const vk::DescriptorBufferInfo& wdGALResourceViewVulkan::GetBufferInfo() const
{
  // Vulkan buffers get constantly swapped out for new ones so the vk::Buffer pointer is not persistent.
  // We need to acquire the latest one on every request for rendering.
  m_resourceBufferInfo.buffer = static_cast<const wdGALBufferVulkan*>(GetResource())->GetVkBuffer();
  return m_resourceBufferInfo;
}

wdGALResourceViewVulkan::wdGALResourceViewVulkan(wdGALResourceBase* pResource, const wdGALResourceViewCreationDescription& Description)
  : wdGALResourceView(pResource, Description)
{
}

wdGALResourceViewVulkan::~wdGALResourceViewVulkan() {}

wdResult wdGALResourceViewVulkan::InitPlatform(wdGALDevice* pDevice)
{
  wdGALDeviceVulkan* pVulkanDevice = static_cast<wdGALDeviceVulkan*>(pDevice);

  const wdGALTexture* pTexture = nullptr;
  if (!m_Description.m_hTexture.IsInvalidated())
    pTexture = pDevice->GetTexture(m_Description.m_hTexture);

  const wdGALBuffer* pBuffer = nullptr;
  if (!m_Description.m_hBuffer.IsInvalidated())
    pBuffer = pDevice->GetBuffer(m_Description.m_hBuffer);

  if (pTexture == nullptr && pBuffer == nullptr)
  {
    wdLog::Error("No valid texture handle or buffer handle given for resource view creation!");
    return WD_FAILURE;
  }

  if (pTexture)
  {
    auto pParentTexture = static_cast<const wdGALTextureVulkan*>(pTexture->GetParentResource());
    auto image = pParentTexture->GetImage();
    const wdGALTextureCreationDescription& texDesc = pTexture->GetDescription();

    const bool bIsArrayView = IsArrayView(texDesc, m_Description);
    const bool bIsDepth = wdGALResourceFormat::IsDepthFormat(pTexture->GetDescription().m_Format);

    wdGALResourceFormat::Enum viewFormat = m_Description.m_OverrideViewFormat == wdGALResourceFormat::Invalid ? texDesc.m_Format : m_Description.m_OverrideViewFormat;
    vk::ImageViewCreateInfo viewCreateInfo;
    viewCreateInfo.format = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eResourceViewType;
    viewCreateInfo.image = image;
    viewCreateInfo.subresourceRange = wdConversionUtilsVulkan::GetSubresourceRange(texDesc, m_Description);
    viewCreateInfo.subresourceRange.aspectMask &= ~vk::ImageAspectFlagBits::eStencil;


    m_resourceImageInfo.imageLayout = bIsDepth ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eShaderReadOnlyOptimal;
    m_resourceImageInfoArray.imageLayout = m_resourceImageInfo.imageLayout;

    if (pParentTexture->GetFormatOverrideEnabled())
    {
      WD_ASSERT_DEV(m_Description.m_OverrideViewFormat == wdGALResourceFormat::Invalid, "Resource views on this texture can not use a override view format (not implemented)");
      viewCreateInfo.format = pParentTexture->GetImageFormat();
    }

    m_range = viewCreateInfo.subresourceRange;
    if (texDesc.m_Type == wdGALTextureType::Texture3D) // no array support
    {
      viewCreateInfo.viewType = wdConversionUtilsVulkan::GetImageViewType(texDesc.m_Type, false);
      VK_SUCCEED_OR_RETURN_WD_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfo.imageView));
    }
    else if (m_Description.m_uiArraySize == 1) // can be array or not
    {
      viewCreateInfo.viewType = wdConversionUtilsVulkan::GetImageViewType(texDesc.m_Type, false);
      VK_SUCCEED_OR_RETURN_WD_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfo.imageView));
      viewCreateInfo.viewType = wdConversionUtilsVulkan::GetImageViewType(texDesc.m_Type, true);
      VK_SUCCEED_OR_RETURN_WD_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfoArray.imageView));
    }
    else // Can only be array
    {
      viewCreateInfo.viewType = wdConversionUtilsVulkan::GetImageViewType(texDesc.m_Type, true);
      VK_SUCCEED_OR_RETURN_WD_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfoArray.imageView));
    }
  }
  else if (pBuffer)
  {
    if (!pBuffer->GetDescription().m_bAllowRawViews && m_Description.m_bRawView)
    {
      wdLog::Error("Trying to create a raw view for a buffer with no raw view flag is invalid!");
      return WD_FAILURE;
    }

    auto pParentBuffer = static_cast<const wdGALBufferVulkan*>(pBuffer);
    if (pBuffer->GetDescription().m_bUseAsStructuredBuffer)
    {
      m_resourceBufferInfo.offset = pBuffer->GetDescription().m_uiStructSize * m_Description.m_uiFirstElement;
      m_resourceBufferInfo.range = pBuffer->GetDescription().m_uiStructSize * m_Description.m_uiNumElements;
    }
    else if (m_Description.m_bRawView)
    {
      m_resourceBufferInfo.offset = sizeof(wdUInt32) * m_Description.m_uiFirstElement;
      m_resourceBufferInfo.range = sizeof(wdUInt32) * m_Description.m_uiNumElements;
    }
    else
    {
      wdGALResourceFormat::Enum viewFormat = m_Description.m_OverrideViewFormat;
      if (viewFormat == wdGALResourceFormat::Invalid)
        viewFormat = wdGALResourceFormat::RUInt;

      vk::BufferViewCreateInfo viewCreateInfo;
      viewCreateInfo.buffer = pParentBuffer->GetVkBuffer();
      viewCreateInfo.format = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eResourceViewType;
      viewCreateInfo.offset = pBuffer->GetDescription().m_uiStructSize * m_Description.m_uiFirstElement;
      viewCreateInfo.range = pBuffer->GetDescription().m_uiStructSize * m_Description.m_uiNumElements;

      VK_SUCCEED_OR_RETURN_WD_FAILURE(pVulkanDevice->GetVulkanDevice().createBufferView(&viewCreateInfo, nullptr, &m_bufferView));
    }
  }

  return WD_SUCCESS;
}

wdResult wdGALResourceViewVulkan::DeInitPlatform(wdGALDevice* pDevice)
{
  wdGALDeviceVulkan* pVulkanDevice = static_cast<wdGALDeviceVulkan*>(pDevice);
  pVulkanDevice->DeleteLater(m_resourceImageInfo.imageView);
  pVulkanDevice->DeleteLater(m_resourceImageInfoArray.imageView);
  m_resourceImageInfo = vk::DescriptorImageInfo();
  m_resourceImageInfoArray = vk::DescriptorImageInfo();
  m_resourceBufferInfo = vk::DescriptorBufferInfo();
  pVulkanDevice->DeleteLater(m_bufferView);
  return WD_SUCCESS;
}



WD_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_ResourceViewVulkan);
