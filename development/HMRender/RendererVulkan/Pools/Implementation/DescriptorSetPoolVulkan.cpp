#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Pools/DescriptorSetPoolVulkan.h>


vk::DescriptorPool wdDescriptorSetPoolVulkan::s_currentPool;
wdHybridArray<vk::DescriptorPool, 4> wdDescriptorSetPoolVulkan::s_freePools;
vk::Device wdDescriptorSetPoolVulkan::s_device;
wdHashTable<vk::DescriptorType, float> wdDescriptorSetPoolVulkan::s_descriptorWeights;

void wdDescriptorSetPoolVulkan::Initialize(vk::Device device)
{
  s_device = device;
  s_descriptorWeights[vk::DescriptorType::eSampler] = 0.5f;       // Image sampler.
  s_descriptorWeights[vk::DescriptorType::eSampledImage] = 2.0f;  // Read-only image view.
  s_descriptorWeights[vk::DescriptorType::eStorageImage] = 1.0f;  // Read / write image view.
  s_descriptorWeights[vk::DescriptorType::eUniformBuffer] = 1.0f; // Read-only struct (constant buffer)
  s_descriptorWeights[vk::DescriptorType::eStorageBuffer] = 1.0f; // Read / write struct (UAV).
  s_descriptorWeights[vk::DescriptorType::eUniformTexelBuffer] = 0.5f; // Read-only linear texel buffer with view.

  // Not used by WD so far.
  s_descriptorWeights[vk::DescriptorType::eStorageTexelBuffer] = 0.0f;   // Read / write linear texel buffer with view.
  s_descriptorWeights[vk::DescriptorType::eUniformBufferDynamic] = 0.0f; // Same as eUniformBuffer but allows updating the memory offset into the buffer dynamically.
  s_descriptorWeights[vk::DescriptorType::eStorageBufferDynamic] = 0.0f; // Same as eStorageBuffer but allows updating the memory offset into the buffer dynamically.

  // Not supported by WD so far.
  s_descriptorWeights[vk::DescriptorType::eInputAttachment] = 0.0f; //frame-buffer local read-only image view.
  s_descriptorWeights[vk::DescriptorType::eCombinedImageSampler] = 0.0f;
  s_descriptorWeights[vk::DescriptorType::eInlineUniformBlock] = 0.0f;
  s_descriptorWeights[vk::DescriptorType::eAccelerationStructureKHR] = 0.0f;
  s_descriptorWeights[vk::DescriptorType::eAccelerationStructureNV] = 0.0f;
  s_descriptorWeights[vk::DescriptorType::eMutableVALVE] = 0.0f;
}

void wdDescriptorSetPoolVulkan::DeInitialize()
{
  s_descriptorWeights.Clear();
  s_descriptorWeights.Compact();

  for (vk::DescriptorPool& pool : s_freePools)
  {
    s_device.destroyDescriptorPool(pool, nullptr);
  }
  s_freePools.Clear();
  s_freePools.Compact();
  if (s_currentPool)
  {
    s_device.resetDescriptorPool(s_currentPool);
    s_device.destroyDescriptorPool(s_currentPool, nullptr);
    s_currentPool = nullptr;
  }

  s_device = nullptr;
}

wdHashTable<vk::DescriptorType, float>& wdDescriptorSetPoolVulkan::AccessDescriptorPoolWeights()
{
  return s_descriptorWeights;
}

vk::DescriptorSet wdDescriptorSetPoolVulkan::CreateDescriptorSet(vk::DescriptorSetLayout layout)
{
  vk::DescriptorSet set;
  if (!s_currentPool)
  {
    s_currentPool = GetNewPool();
  }

  vk::DescriptorSetAllocateInfo allocateInfo;
  allocateInfo.pSetLayouts = &layout;
  allocateInfo.descriptorPool = s_currentPool;
  allocateInfo.descriptorSetCount = 1;

  vk::Result res = s_device.allocateDescriptorSets(&allocateInfo, &set);
  bool bPoolExhausted = false;

  switch (res)
  {
    case vk::Result::eSuccess:
      break;
    case vk::Result::eErrorFragmentedPool:
    case vk::Result::eErrorOutOfPoolMemory:
      bPoolExhausted = true;
      break;
    default:
      VK_ASSERT_DEV(res);
      break;
  }

  if (bPoolExhausted)
  {
    wdGALDeviceVulkan* pDevice = static_cast<wdGALDeviceVulkan*>(wdGALDevice::GetDefaultDevice());
    pDevice->ReclaimLater(s_currentPool);
    s_currentPool = GetNewPool();
    allocateInfo.descriptorPool = s_currentPool;
    VK_ASSERT_DEV(s_device.allocateDescriptorSets(&allocateInfo, &set));
  }

  return set;
}

void wdDescriptorSetPoolVulkan::UpdateDescriptorSet(vk::DescriptorSet descriptorSet, wdArrayPtr<vk::WriteDescriptorSet> update)
{
  s_device.updateDescriptorSets(update.GetCount(), update.GetPtr(), 0, nullptr);
}

void wdDescriptorSetPoolVulkan::ReclaimPool(vk::DescriptorPool& descriptorPool)
{
  s_device.resetDescriptorPool(descriptorPool);
  s_freePools.PushBack(descriptorPool);
}

vk::DescriptorPool wdDescriptorSetPoolVulkan::GetNewPool()
{
  if (s_freePools.IsEmpty())
  {
    wdHybridArray<vk::DescriptorPoolSize, 20> poolSizes;
    for (auto weight : s_descriptorWeights)
    {
      if (static_cast<wdUInt32>(weight.Value() * s_uiPoolBaseSize) > 0)
        poolSizes.PushBack(vk::DescriptorPoolSize(weight.Key(), static_cast<wdUInt32>(weight.Value() * s_uiPoolBaseSize)));
    }
    vk::DescriptorPoolCreateInfo poolCreateInfo;
    poolCreateInfo.flags = {};
    poolCreateInfo.maxSets = s_uiPoolBaseSize;
    poolCreateInfo.poolSizeCount = poolSizes.GetCount();
    poolCreateInfo.pPoolSizes = poolSizes.GetData();

    vk::DescriptorPool descriptorPool;
    VK_ASSERT_DEV(s_device.createDescriptorPool(&poolCreateInfo, nullptr, &descriptorPool));
    return descriptorPool;
  }
  else
  {
    vk::DescriptorPool pool = s_freePools.PeekBack();
    s_freePools.PopBack();
    return pool;
  }
}
