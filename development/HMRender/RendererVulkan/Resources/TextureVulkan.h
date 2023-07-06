#pragma once

#include <RendererFoundation/Resources/Texture.h>

#include <vulkan/vulkan.hpp>

class wdGALBufferVulkan;
class wdGALDeviceVulkan;

class wdGALTextureVulkan : public wdGALTexture
{
public:
  enum class StagingMode : wdUInt8
  {
    None,
    Buffer,          ///< We can use vkCopyImageToBuffer to a CPU buffer.
    Texture,         ///< Formats differ and we need to render to a linear CPU texture to do the conversion.
    TextureAndBuffer ///< Formats differ and linear texture can't be rendered to. Render to optimal layout GPU texture and then use vkCopyImageToBuffer to CPU buffer.
  };
  struct SubResourceOffset
  {
    WD_DECLARE_POD_TYPE();
    wdUInt32 m_uiOffset;
    wdUInt32 m_uiSize;
    wdUInt32 m_uiRowLength;
    wdUInt32 m_uiImageHeight;
  };

  WD_ALWAYS_INLINE vk::Image GetImage() const;
  WD_ALWAYS_INLINE vk::Format GetImageFormat() const { return m_imageFormat; }
  WD_ALWAYS_INLINE vk::ImageLayout GetPreferredLayout() const;
  WD_ALWAYS_INLINE vk::ImageLayout GetPreferredLayout(vk::ImageLayout targetLayout) const;
  WD_ALWAYS_INLINE vk::PipelineStageFlags GetUsedByPipelineStage() const;
  WD_ALWAYS_INLINE vk::AccessFlags GetAccessMask() const;

  WD_ALWAYS_INLINE wdVulkanAllocation GetAllocation() const;
  WD_ALWAYS_INLINE const wdVulkanAllocationInfo& GetAllocationInfo() const;

  WD_ALWAYS_INLINE bool GetFormatOverrideEnabled() const;
  WD_ALWAYS_INLINE bool IsLinearLayout() const;

  vk::Extent3D GetMipLevelSize(wdUInt32 uiMipLevel) const;
  vk::ImageSubresourceRange GetFullRange() const;
  vk::ImageAspectFlags GetAspectMask() const;

  // Read-back staging resources
  WD_ALWAYS_INLINE StagingMode GetStagingMode() const;
  WD_ALWAYS_INLINE wdGALTextureHandle GetStagingTexture() const;
  WD_ALWAYS_INLINE wdGALBufferHandle GetStagingBuffer() const;
  wdUInt32 ComputeSubResourceOffsets(wdDynamicArray<SubResourceOffset>& out_subResourceOffsets) const;

protected:
  friend class wdGALDeviceVulkan;
  friend class wdMemoryUtils;

  wdGALTextureVulkan(const wdGALTextureCreationDescription& Description);
  wdGALTextureVulkan(const wdGALTextureCreationDescription& Description, vk::Format OverrideFormat, bool bLinearCPU);

  ~wdGALTextureVulkan();

  virtual wdResult InitPlatform(wdGALDevice* pDevice, wdArrayPtr<wdGALSystemMemoryDescription> pInitialData) override;
  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  StagingMode ComputeStagingMode(const vk::ImageCreateInfo& createInfo) const;
  wdResult CreateStagingBuffer(const vk::ImageCreateInfo& createInfo);

  vk::Image m_image;
  vk::Format m_imageFormat = vk::Format::eUndefined;
  vk::ImageLayout m_preferredLayout = vk::ImageLayout::eUndefined;
  vk::PipelineStageFlags m_stages = {};
  vk::AccessFlags m_access = {};

  wdVulkanAllocation m_alloc = nullptr;
  wdVulkanAllocationInfo m_allocInfo;

  wdGALDeviceVulkan* m_pDevice = nullptr;
  void* m_pExisitingNativeObject = nullptr;

  bool m_formatOverride = false;
  bool m_bLinearCPU = false;

  StagingMode m_stagingMode = StagingMode::None;
  wdGALTextureHandle m_hStagingTexture;
  wdGALBufferHandle m_hStagingBuffer;
};

#include <RendererVulkan/Resources/Implementation/TextureVulkan_inl.h>
