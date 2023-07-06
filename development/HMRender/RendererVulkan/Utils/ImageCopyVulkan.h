#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/Shader/ShaderUtils.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>

#include <vulkan/vulkan.hpp>

class wdGALBufferVulkan;
class wdGALTextureVulkan;
class wdGALRenderTargetViewVulkan;
class wdGALResourceViewVulkan;
class wdGALUnorderedAccessViewVulkan;


/// \brief
class WD_RENDERERVULKAN_DLL wdImageCopyVulkan
{
public:
  wdImageCopyVulkan(wdGALDeviceVulkan& GALDeviceVulkan);
  ~wdImageCopyVulkan();
  void Init(const wdGALTextureVulkan* pSource, const wdGALTextureVulkan* pTarget, wdShaderUtils::wdBuiltinShaderType type);

  void Copy(const wdVec3U32& sourceOffset, const vk::ImageSubresourceLayers& sourceLayers, const wdVec3U32& targetOffset, const vk::ImageSubresourceLayers& targetLayers, const wdVec3U32& extends);

  static void Initialize(wdGALDeviceVulkan& GALDeviceVulkan);
  static void DeInitialize(wdGALDeviceVulkan& GALDeviceVulkan);

  struct RenderPassCacheKey
  {
    WD_DECLARE_POD_TYPE();

    vk::Format targetFormat;
    vk::SampleCountFlagBits targetSamples;
  };

  struct FramebufferCacheKey
  {
    WD_DECLARE_POD_TYPE();

    vk::RenderPass m_renderpass;
    vk::ImageView m_targetView;
    wdVec3U32 m_extends;
    uint32_t m_layerCount;
  };

  struct ImageViewCacheKey
  {
    WD_DECLARE_POD_TYPE();

    vk::Image m_image;
    vk::ImageSubresourceLayers m_subresourceLayers;
  };

  struct ImageViewCacheValue
  {
    WD_DECLARE_POD_TYPE();

    vk::ImageSubresourceLayers m_subresourceLayers;
    vk::ImageView m_imageView;
  };

private:
  void RenderInternal(const wdVec3U32& sourceOffset, const vk::ImageSubresourceLayers& sourceLayers, const wdVec3U32& targetOffset, const vk::ImageSubresourceLayers& targetLayers, const wdVec3U32& extends);

  static void OnBeforeImageDestroyed(wdGALDeviceVulkan::OnBeforeImageDestroyedData data);


private:
  wdGALDeviceVulkan& m_GALDeviceVulkan;

  // Init input
  const wdGALTextureVulkan* m_pSource = nullptr;
  const wdGALTextureVulkan* m_pTarget = nullptr;
  wdShaderUtils::wdBuiltinShaderType m_type = wdShaderUtils::wdBuiltinShaderType::CopyImage;

  // Init derived Vulkan objects
  vk::RenderPass m_renderPass;
  wdShaderUtils::wdBuiltinShader m_shader;
  wdGALVertexDeclarationHandle m_hVertexDecl;
  wdResourceCacheVulkan::PipelineLayoutDesc m_LayoutDesc;
  wdResourceCacheVulkan::GraphicsPipelineDesc m_PipelineDesc;
  vk::Pipeline m_pipeline;

  // Cache to keep important resources alive
  // This avoids recreating them every frame
  struct Cache
  {
    Cache(wdAllocatorBase* pAllocator);
    ~Cache();

    wdHashTable<wdGALShaderHandle, wdGALVertexDeclarationHandle> m_vertexDeclarations;
    wdHashTable<RenderPassCacheKey, vk::RenderPass> m_renderPasses;
    wdHashTable<ImageViewCacheKey, vk::ImageView> m_sourceImageViews;
    wdHashTable<vk::Image, ImageViewCacheValue> m_imageToSourceImageViewCacheKey;
    wdHashTable<ImageViewCacheKey, vk::ImageView> m_targetImageViews;
    wdHashTable<vk::Image, ImageViewCacheValue> m_imageToTargetImageViewCacheKey;
    wdHashTable<FramebufferCacheKey, vk::Framebuffer> m_framebuffers;
    wdHashTable<wdShaderUtils::wdBuiltinShaderType, wdShaderUtils::wdBuiltinShader> m_shaders;

    wdEventSubscriptionID m_onBeforeImageDeletedSubscription;
  };

  static wdUniquePtr<Cache> s_cache;
};
