
#pragma once

#include <RendererFoundation/Resources/Buffer.h>

#include <RendererVulkan/Device/DeviceVulkan.h>

#include <vulkan/vulkan.hpp>

class WD_RENDERERVULKAN_DLL wdGALBufferVulkan : public wdGALBuffer
{
public:
  void DiscardBuffer() const;
  WD_ALWAYS_INLINE vk::Buffer GetVkBuffer() const;
  const vk::DescriptorBufferInfo& GetBufferInfo() const;

  WD_ALWAYS_INLINE vk::IndexType GetIndexType() const;
  WD_ALWAYS_INLINE wdVulkanAllocation GetAllocation() const;
  WD_ALWAYS_INLINE const wdVulkanAllocationInfo& GetAllocationInfo() const;
  WD_ALWAYS_INLINE vk::PipelineStageFlags GetUsedByPipelineStage() const;
  WD_ALWAYS_INLINE vk::AccessFlags GetAccessMask() const;
  static vk::DeviceSize GetAlignment(const wdGALDeviceVulkan* pDevice, vk::BufferUsageFlags usage);

protected:
  struct BufferVulkan
  {
    vk::Buffer m_buffer;
    wdVulkanAllocation m_alloc;
    mutable wdUInt64 m_currentFrame = 0;
  };

  friend class wdGALDeviceVulkan;
  friend class wdMemoryUtils;

  wdGALBufferVulkan(const wdGALBufferCreationDescription& Description, bool bCPU = false);

  virtual ~wdGALBufferVulkan();

  virtual wdResult InitPlatform(wdGALDevice* pDevice, wdArrayPtr<const wdUInt8> pInitialData) override;
  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;
  virtual void SetDebugNamePlatform(const char* szName) const override;
  void CreateBuffer() const;

  mutable BufferVulkan m_currentBuffer;
  mutable vk::DescriptorBufferInfo m_resourceBufferInfo;
  mutable wdDeque<BufferVulkan> m_usedBuffers;
  mutable wdVulkanAllocationInfo m_allocInfo;

  // Data for memory barriers and access
  vk::PipelineStageFlags m_stages = {};
  vk::AccessFlags m_access = {};
  vk::IndexType m_indexType = vk::IndexType::eUint16; // Only applicable for index buffers
  vk::BufferUsageFlags m_usage = {};
  vk::DeviceSize m_size = 0;

  wdGALDeviceVulkan* m_pDeviceVulkan = nullptr;
  vk::Device m_device;

  bool m_bCPU = false;
  mutable wdString m_sDebugName;
};

#include <RendererVulkan/Resources/Implementation/BufferVulkan_inl.h>
