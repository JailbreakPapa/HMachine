#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

WD_DEFINE_AS_POD_TYPE(vk::DescriptorType);

template <>
struct wdHashHelper<vk::DescriptorType>
{
  WD_ALWAYS_INLINE static wdUInt32 Hash(vk::DescriptorType value) { return wdHashHelper<wdUInt32>::Hash(wdUInt32(value)); }
  WD_ALWAYS_INLINE static bool Equal(vk::DescriptorType a, vk::DescriptorType b) { return a == b; }
};

class WD_RENDERERVULKAN_DLL wdDescriptorSetPoolVulkan
{
public:
  static void Initialize(vk::Device device);
  static void DeInitialize();
  static wdHashTable<vk::DescriptorType, float>& AccessDescriptorPoolWeights();

  static vk::DescriptorSet CreateDescriptorSet(vk::DescriptorSetLayout layout);
  static void UpdateDescriptorSet(vk::DescriptorSet descriptorSet, wdArrayPtr<vk::WriteDescriptorSet> update);
  static void ReclaimPool(vk::DescriptorPool& descriptorPool);

private:
  static constexpr wdUInt32 s_uiPoolBaseSize = 1024;

  static vk::DescriptorPool GetNewPool();

  static vk::DescriptorPool s_currentPool;
  static wdHybridArray<vk::DescriptorPool, 4> s_freePools;
  static vk::Device s_device;
  static wdHashTable<vk::DescriptorType, float> s_descriptorWeights;
};
