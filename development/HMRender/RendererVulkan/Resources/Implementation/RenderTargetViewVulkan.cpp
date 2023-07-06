#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

bool IsArrayView(const wdGALTextureCreationDescription& texDesc, const wdGALRenderTargetViewCreationDescription& viewDesc)
{
  return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstSlice > 0;
}

wdGALRenderTargetViewVulkan::wdGALRenderTargetViewVulkan(wdGALTexture* pTexture, const wdGALRenderTargetViewCreationDescription& Description)
  : wdGALRenderTargetView(pTexture, Description)
{
}

wdGALRenderTargetViewVulkan::~wdGALRenderTargetViewVulkan() {}

wdResult wdGALRenderTargetViewVulkan::InitPlatform(wdGALDevice* pDevice)
{
  const wdGALTexture* pTexture = nullptr;
  if (!m_Description.m_hTexture.IsInvalidated())
    pTexture = pDevice->GetTexture(m_Description.m_hTexture);

  if (pTexture == nullptr)
  {
    wdLog::Error("No valid texture handle given for render target view creation!");
    return WD_FAILURE;
  }

  const wdGALTextureCreationDescription& texDesc = pTexture->GetDescription();
  wdGALResourceFormat::Enum viewFormat = texDesc.m_Format;

  if (m_Description.m_OverrideViewFormat != wdGALResourceFormat::Invalid)
    viewFormat = m_Description.m_OverrideViewFormat;

  wdGALDeviceVulkan* pVulkanDevice = static_cast<wdGALDeviceVulkan*>(pDevice);
  auto pTextureVulkan = static_cast<const wdGALTextureVulkan*>(pTexture->GetParentResource());
  vk::Format vkViewFormat = pTextureVulkan->GetImageFormat();

  const bool bIsDepthFormat = wdConversionUtilsVulkan::IsDepthFormat(vkViewFormat);

  if (vkViewFormat == vk::Format::eUndefined)
  {
    wdLog::Error("Couldn't get Vulkan format for view!");
    return WD_FAILURE;
  }


  vk::Image vkImage = pTextureVulkan->GetImage();
  const bool bIsArrayView = IsArrayView(texDesc, m_Description);

  if (pTextureVulkan->GetFormatOverrideEnabled())
  {
    vkViewFormat = pTextureVulkan->GetImageFormat();
  }

  vk::ImageViewCreateInfo imageViewCreationInfo;
  if (bIsDepthFormat)
  {
    imageViewCreationInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    if (texDesc.m_Format == wdGALResourceFormat::D24S8)
    {
      imageViewCreationInfo.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
    }
  }
  else
  {
    imageViewCreationInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  }

  imageViewCreationInfo.image = vkImage;
  imageViewCreationInfo.format = vkViewFormat;

  if (!bIsArrayView)
  {
    imageViewCreationInfo.viewType = vk::ImageViewType::e2D;
    imageViewCreationInfo.subresourceRange.baseMipLevel = m_Description.m_uiMipLevel;
    imageViewCreationInfo.subresourceRange.levelCount = 1;
    imageViewCreationInfo.subresourceRange.layerCount = 1;
  }
  else
  {
    imageViewCreationInfo.viewType = vk::ImageViewType::e2DArray;
    imageViewCreationInfo.subresourceRange.baseMipLevel = m_Description.m_uiMipLevel;
    imageViewCreationInfo.subresourceRange.levelCount = 1;
    imageViewCreationInfo.subresourceRange.baseArrayLayer = m_Description.m_uiFirstSlice;
    imageViewCreationInfo.subresourceRange.layerCount = m_Description.m_uiSliceCount;
  }
  m_range = imageViewCreationInfo.subresourceRange;
  m_bfullRange = m_range == pTextureVulkan->GetFullRange();

  VK_SUCCEED_OR_RETURN_WD_FAILURE(pVulkanDevice->GetVulkanDevice().createImageView(&imageViewCreationInfo, nullptr, &m_imageView));
  pVulkanDevice->SetDebugName("RTV", m_imageView);
  return WD_SUCCESS;
}

wdResult wdGALRenderTargetViewVulkan::DeInitPlatform(wdGALDevice* pDevice)
{
  wdGALDeviceVulkan* pVulkanDevice = static_cast<wdGALDeviceVulkan*>(pDevice);
  pVulkanDevice->DeleteLater(m_imageView);
  return WD_SUCCESS;
}



WD_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_RenderTargetViewVulkan);
