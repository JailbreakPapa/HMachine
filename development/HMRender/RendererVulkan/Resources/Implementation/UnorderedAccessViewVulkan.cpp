#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Resources/UnorderedAccessViewVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

bool IsArrayView(const wdGALTextureCreationDescription& texDesc, const wdGALUnorderedAccessViewCreationDescription& viewDesc)
{
  return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstArraySlice > 0;
}

const vk::DescriptorBufferInfo& wdGALUnorderedAccessViewVulkan::GetBufferInfo() const
{
  // Vulkan buffers get constantly swapped out for new ones so the vk::Buffer pointer is not persistent.
  // We need to acquire the latest one on every request for rendering.
  m_resourceBufferInfo.buffer = static_cast<const wdGALBufferVulkan*>(GetResource())->GetVkBuffer();
  return m_resourceBufferInfo;
}

wdGALUnorderedAccessViewVulkan::wdGALUnorderedAccessViewVulkan(
  wdGALResourceBase* pResource, const wdGALUnorderedAccessViewCreationDescription& Description)
  : wdGALUnorderedAccessView(pResource, Description)
{
}

wdGALUnorderedAccessViewVulkan::~wdGALUnorderedAccessViewVulkan() {}

wdResult wdGALUnorderedAccessViewVulkan::InitPlatform(wdGALDevice* pDevice)
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

    wdGALResourceFormat::Enum viewFormat = m_Description.m_OverrideViewFormat == wdGALResourceFormat::Invalid ? texDesc.m_Format : m_Description.m_OverrideViewFormat;
    vk::ImageViewCreateInfo viewCreateInfo;
    viewCreateInfo.format = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(viewFormat).m_eResourceViewType;
    viewCreateInfo.image = image;
    viewCreateInfo.subresourceRange = wdConversionUtilsVulkan::GetSubresourceRange(texDesc, m_Description);
    viewCreateInfo.viewType = wdConversionUtilsVulkan::GetImageViewType(texDesc.m_Type, bIsArrayView);
    if (texDesc.m_Type == wdGALTextureType::TextureCube)
      viewCreateInfo.viewType = vk::ImageViewType::e2DArray; // There is no RWTextureCube / RWTextureCubeArray in HLSL

    m_resourceImageInfo.imageLayout = vk::ImageLayout::eGeneral;

    m_range = viewCreateInfo.subresourceRange;
    VK_SUCCEED_OR_RETURN_WD_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &m_resourceImageInfo.imageView));
    pVulkanDevice->SetDebugName("UAV-Texture", m_resourceImageInfo.imageView);
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
      m_resourceBufferInfo.offset = pBuffer->GetDescription().m_uiStructSize * m_Description.m_uiFirstElement;
      m_resourceBufferInfo.range = pBuffer->GetDescription().m_uiStructSize * m_Description.m_uiNumElements;
    }
  }

  return WD_SUCCESS;
}

wdResult wdGALUnorderedAccessViewVulkan::DeInitPlatform(wdGALDevice* pDevice)
{
  wdGALDeviceVulkan* pVulkanDevice = static_cast<wdGALDeviceVulkan*>(pDevice);
  pVulkanDevice->DeleteLater(m_resourceImageInfo.imageView);
  m_resourceImageInfo = vk::DescriptorImageInfo();
  m_resourceBufferInfo = vk::DescriptorBufferInfo();
  return WD_SUCCESS;
}

WD_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_UnorderedAccessViewVulkan);
