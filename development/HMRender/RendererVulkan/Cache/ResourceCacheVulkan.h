#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Math/Size.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>

#include <vulkan/vulkan.hpp>

class wdGALRasterizerStateVulkan;
class wdGALBlendStateVulkan;
class wdGALDepthStencilStateVulkan;
class wdGALShaderVulkan;
class wdGALVertexDeclarationVulkan;
class wdRefCounted;

WD_DEFINE_AS_POD_TYPE(vk::DynamicState);

/// \brief Creates and caches persistent Vulkan resources. Resources are never freed until the device is shut down.
class WD_RENDERERVULKAN_DLL wdResourceCacheVulkan
{
public:
  static void Initialize(wdGALDeviceVulkan* pDevice, vk::Device device);
  static void DeInitialize();

  static vk::RenderPass RequestRenderPass(const wdGALRenderingSetup& renderingSetup);
  static vk::Framebuffer RequestFrameBuffer(vk::RenderPass renderPass, const wdGALRenderTargetSetup& renderTargetSetup, wdSizeU32& out_Size, wdEnum<wdGALMSAASampleCount>& out_msaa, wdUInt32& out_uiLayers);

  struct PipelineLayoutDesc
  {
    WD_DECLARE_POD_TYPE();
    vk::DescriptorSetLayout m_layout;
  };

  struct GraphicsPipelineDesc
  {
    WD_DECLARE_POD_TYPE();
    vk::RenderPass m_renderPass;
    vk::PipelineLayout m_layout;
    wdEnum<wdGALPrimitiveTopology> m_topology;
    wdEnum<wdGALMSAASampleCount> m_msaa;
    wdUInt8 m_uiAttachmentCount = 0;
    const wdGALRasterizerStateVulkan* m_pCurrentRasterizerState = nullptr;
    const wdGALBlendStateVulkan* m_pCurrentBlendState = nullptr;
    const wdGALDepthStencilStateVulkan* m_pCurrentDepthStencilState = nullptr;
    const wdGALShaderVulkan* m_pCurrentShader = nullptr;
    const wdGALVertexDeclarationVulkan* m_pCurrentVertexDecl = nullptr;
    wdUInt32 m_VertexBufferStrides[WD_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  };

  struct ComputePipelineDesc
  {
    WD_DECLARE_POD_TYPE();
    vk::PipelineLayout m_layout;
    const wdGALShaderVulkan* m_pCurrentShader = nullptr;
  };

  static vk::PipelineLayout RequestPipelineLayout(const PipelineLayoutDesc& desc);
  static vk::Pipeline RequestGraphicsPipeline(const GraphicsPipelineDesc& desc);
  static vk::Pipeline RequestComputePipeline(const ComputePipelineDesc& desc);

  struct DescriptorSetLayoutDesc
  {
    mutable wdUInt32 m_uiHash = 0;
    wdHybridArray<vk::DescriptorSetLayoutBinding, 6> m_bindings;
  };
  static vk::DescriptorSetLayout RequestDescriptorSetLayout(const wdGALShaderVulkan::DescriptorSetLayoutDesc& desc);

  /// \brief Invalidates any caches that use this resource. Basically all pointer types in GraphicsPipelineDesc except for wdGALShaderVulkan.
  static void ResourceDeleted(const wdRefCounted* pResource);
  /// \brief Invalidates any caches that use this shader resource.
  static void ShaderDeleted(const wdGALShaderVulkan* pShader);

private:
  struct FramebufferKey
  {
    vk::RenderPass m_renderPass;
    wdGALRenderTargetSetup m_renderTargetSetup;
  };

  /// \brief Hashable version without pointers of vk::FramebufferCreateInfo
  struct FramebufferDesc
  {
    VkRenderPass renderPass;
    wdSizeU32 m_size = {0, 0};
    uint32_t layers = 1;
    wdHybridArray<vk::ImageView, WD_GAL_MAX_RENDERTARGET_COUNT + 1> attachments;
    wdEnum<wdGALMSAASampleCount> m_msaa;
  };

  /// \brief Hashable version without pointers or redundant data of vk::AttachmentDescription
  struct AttachmentDesc
  {
    WD_DECLARE_POD_TYPE();
    vk::Format format = vk::Format::eUndefined;
    vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
    // Not set at all right now
    vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled;
    // Not set at all right now
    vk::ImageLayout initialLayout = vk::ImageLayout::eUndefined;
    // No support for eDontCare in WD right now
    vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eClear;
    vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eStore;
    vk::AttachmentLoadOp stencilLoadOp = vk::AttachmentLoadOp::eClear;
    vk::AttachmentStoreOp stencilStoreOp = vk::AttachmentStoreOp::eStore;
  };

  /// \brief Hashable version without pointers of vk::RenderPassCreateInfo
  struct RenderPassDesc
  {
    wdHybridArray<AttachmentDesc, WD_GAL_MAX_RENDERTARGET_COUNT> attachments;
  };

  struct ResourceCacheHash
  {
    static wdUInt32 Hash(const RenderPassDesc& renderingSetup);
    static bool Equal(const RenderPassDesc& a, const RenderPassDesc& b);

    static wdUInt32 Hash(const wdGALRenderingSetup& renderingSetup);
    static bool Equal(const wdGALRenderingSetup& a, const wdGALRenderingSetup& b);

    static wdUInt32 Hash(const FramebufferKey& renderTargetSetup);
    static bool Equal(const FramebufferKey& a, const FramebufferKey& b);

    static wdUInt32 Hash(const PipelineLayoutDesc& desc);
    static bool Equal(const PipelineLayoutDesc& a, const PipelineLayoutDesc& b);

    static bool Less(const GraphicsPipelineDesc& a, const GraphicsPipelineDesc& b);
    static wdUInt32 Hash(const GraphicsPipelineDesc& desc);
    static bool Equal(const GraphicsPipelineDesc& a, const GraphicsPipelineDesc& b);

    static bool Less(const ComputePipelineDesc& a, const ComputePipelineDesc& b);
    static bool Equal(const ComputePipelineDesc& a, const ComputePipelineDesc& b);

    static wdUInt32 Hash(const wdGALShaderVulkan::DescriptorSetLayoutDesc& desc) { return desc.m_uiHash; }
    static bool Equal(const wdGALShaderVulkan::DescriptorSetLayoutDesc& a, const wdGALShaderVulkan::DescriptorSetLayoutDesc& b);
  };

  struct FrameBufferCache
  {
    vk::Framebuffer m_frameBuffer;
    wdSizeU32 m_size;
    wdEnum<wdGALMSAASampleCount> m_msaa;
    wdUInt32 m_layers = 0;
    WD_DECLARE_POD_TYPE();
  };

  static vk::RenderPass RequestRenderPassInternal(const RenderPassDesc& desc);
  static void GetRenderPassDesc(const wdGALRenderingSetup& renderingSetup, RenderPassDesc& out_desc);
  static void GetFrameBufferDesc(vk::RenderPass renderPass, const wdGALRenderTargetSetup& renderTargetSetup, FramebufferDesc& out_desc);

public:
  using GraphicsPipelineMap = wdMap<wdResourceCacheVulkan::GraphicsPipelineDesc, vk::Pipeline, wdResourceCacheVulkan::ResourceCacheHash>;
  using ComputePipelineMap = wdMap<wdResourceCacheVulkan::ComputePipelineDesc, vk::Pipeline, wdResourceCacheVulkan::ResourceCacheHash>;


private:
  static wdGALDeviceVulkan* s_pDevice;
  static vk::Device s_device;
  // We have a N to 1 mapping for wdGALRenderingSetup to vk::RenderPass as multiple wdGALRenderingSetup can share the same RenderPassDesc.
  // Thus, we have a two stage resolve to the vk::RenderPass. If a wdGALRenderingSetup is not present in s_shallowRenderPasses we create the RenderPassDesc which has a 1 to 1 relationship with vk::RenderPass and look that one up in s_renderPasses. Finally we add the entry to s_shallowRenderPasses to make sure a shallow lookup will work on the next query.
  static wdHashTable<wdGALRenderingSetup, vk::RenderPass, ResourceCacheHash> s_shallowRenderPasses; //#TODO_VULKAN cache invalidation
  static wdHashTable<RenderPassDesc, vk::RenderPass, ResourceCacheHash> s_renderPasses;
  static wdHashTable<FramebufferKey, FrameBufferCache, ResourceCacheHash> s_frameBuffers; //#TODO_VULKAN cache invalidation

  static wdHashTable<PipelineLayoutDesc, vk::PipelineLayout, ResourceCacheHash> s_pipelineLayouts;
  static GraphicsPipelineMap s_graphicsPipelines;
  static ComputePipelineMap s_computePipelines;
  static wdMap<const wdRefCounted*, wdHybridArray<GraphicsPipelineMap::Iterator, 1>> s_graphicsPipelineUsedBy;
  static wdMap<const wdRefCounted*, wdHybridArray<ComputePipelineMap::Iterator, 1>> s_computePipelineUsedBy;

  static wdHashTable<wdGALShaderVulkan::DescriptorSetLayoutDesc, vk::DescriptorSetLayout, ResourceCacheHash> s_descriptorSetLayouts;
};
