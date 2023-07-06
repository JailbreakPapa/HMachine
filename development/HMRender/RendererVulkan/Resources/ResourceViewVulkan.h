
#pragma once

#include <RendererFoundation/Resources/ResourceView.h>

#include <vulkan/vulkan.hpp>

class wdGALBufferVulkan;
class wdGALTextureVulkan;

class wdGALResourceViewVulkan : public wdGALResourceView
{
public:
  const vk::DescriptorImageInfo& GetImageInfo(bool bIsArray) const;
  const vk::DescriptorBufferInfo& GetBufferInfo() const;
  vk::ImageSubresourceRange GetRange() const;
  const vk::BufferView& GetBufferView() const;

protected:
  friend class wdGALDeviceVulkan;
  friend class wdMemoryUtils;

  wdGALResourceViewVulkan(wdGALResourceBase* pResource, const wdGALResourceViewCreationDescription& Description);
  ~wdGALResourceViewVulkan();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) override;
  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

  vk::ImageSubresourceRange m_range;
  mutable vk::DescriptorImageInfo m_resourceImageInfo;
  mutable vk::DescriptorImageInfo m_resourceImageInfoArray;
  mutable vk::DescriptorBufferInfo m_resourceBufferInfo;
  vk::BufferView m_bufferView;
};

#include <RendererVulkan/Resources/Implementation/ResourceViewVulkan_inl.h>
