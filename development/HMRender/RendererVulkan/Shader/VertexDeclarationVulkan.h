
#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/VertexDeclaration.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

class wdGALVertexDeclarationVulkan : public wdGALVertexDeclaration
{
public:
  WD_ALWAYS_INLINE wdArrayPtr<const vk::VertexInputAttributeDescription> GetAttributes() const;
  WD_ALWAYS_INLINE wdArrayPtr<const vk::VertexInputBindingDescription> GetBindings() const;

protected:
  friend class wdGALDeviceVulkan;
  friend class wdMemoryUtils;

  virtual wdResult InitPlatform(wdGALDevice* pDevice) override;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

  wdGALVertexDeclarationVulkan(const wdGALVertexDeclarationCreationDescription& Description);

  virtual ~wdGALVertexDeclarationVulkan();

  wdHybridArray<vk::VertexInputAttributeDescription, WD_GAL_MAX_VERTEX_BUFFER_COUNT> m_attributes;
  wdHybridArray<vk::VertexInputBindingDescription, WD_GAL_MAX_VERTEX_BUFFER_COUNT> m_bindings;
};

#include <RendererVulkan/Shader/Implementation/VertexDeclarationVulkan_inl.h>
