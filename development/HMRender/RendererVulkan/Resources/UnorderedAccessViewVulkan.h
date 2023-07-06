
#pragma once

#include <RendererFoundation/Resources/UnorderedAccesView.h>

#include <vulkan/vulkan.hpp>

class wdGALBufferVulkan;

class wdGALUnorderedAccessViewVulkan : public wdGALUnorderedAccessView
{
public:
  WD_ALWAYS_INLINE const vk::DescriptorImageInfo& GetImageInfo() const;
  const vk::DescriptorBufferInfo& GetBufferInfo() const;
  vk::ImageSubresourceRange GetRange() const;

protected:
  friend class wdGALDeviceVulkan;
  friend class wdMemoryUtils;

  wdGALUnorderedAccessViewVulkan(wdGALResourceBase* pResource, const wdGALUnorderedAccessViewCreationDescription& Description);
  ~wdGALUnorderedAccessViewVulkan();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) override;
  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

private:
  mutable vk::DescriptorImageInfo m_resourceImageInfo;
  mutable vk::DescriptorBufferInfo m_resourceBufferInfo;
  vk::ImageSubresourceRange m_range;
};

#include <RendererVulkan/Resources/Implementation/UnorderedAccessViewVulkan_inl.h>
