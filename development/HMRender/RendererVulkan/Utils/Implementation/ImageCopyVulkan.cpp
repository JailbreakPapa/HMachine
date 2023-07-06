#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/ResourceView.h>
#include <RendererFoundation/Shader/ShaderUtils.h>
#include <RendererVulkan/Pools/DescriptorSetPoolVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>
#include <RendererVulkan/Resources/ResourceViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Resources/UnorderedAccessViewVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>
#include <RendererVulkan/Utils/ImageCopyVulkan.h>
#include <RendererVulkan/Utils/PipelineBarrierVulkan.h>

template <>
struct wdHashHelper<wdGALShaderHandle>
{
  WD_ALWAYS_INLINE static wdUInt32 Hash(wdGALShaderHandle value)
  {
    return wdHashHelper<wdGALShaderHandle::IdType::StorageType>::Hash(value.GetInternalID().m_Data);
  }

  WD_ALWAYS_INLINE static bool Equal(wdGALShaderHandle a, wdGALShaderHandle b)
  {
    return wdHashHelper<wdGALShaderHandle::IdType::StorageType>::Equal(a.GetInternalID().m_Data, b.GetInternalID().m_Data);
  }
};

template <>
struct wdHashHelper<wdImageCopyVulkan::RenderPassCacheKey>
{
  WD_ALWAYS_INLINE static wdUInt32 Hash(const wdImageCopyVulkan::RenderPassCacheKey& value)
  {
    return wdHashingUtils::CombineHashValues32(static_cast<uint32_t>(value.targetFormat), static_cast<uint32_t>(value.targetSamples));
  }

  WD_ALWAYS_INLINE static bool Equal(const wdImageCopyVulkan::RenderPassCacheKey& a, const wdImageCopyVulkan::RenderPassCacheKey& b)
  {
    return a.targetFormat == b.targetFormat && a.targetSamples == b.targetSamples;
  }
};

template <>
struct wdHashHelper<vk::Image>
{
  WD_ALWAYS_INLINE static wdUInt32 Hash(vk::Image value)
  {
    return wdHashHelper<void*>::Hash((VkImage)value);
  }

  WD_ALWAYS_INLINE static bool Equal(vk::Image a, vk::Image b)
  {
    return a == b;
  }
};

template <>
struct wdHashHelper<wdImageCopyVulkan::FramebufferCacheKey>
{
  WD_ALWAYS_INLINE static wdUInt32 Hash(const wdImageCopyVulkan::FramebufferCacheKey& value)
  {
    wdHashStreamWriter32 writer;
    writer << (VkRenderPass)value.m_renderpass;
    writer << (VkImageView)value.m_targetView;
    writer << value.m_extends.x;
    writer << value.m_extends.y;
    writer << value.m_layerCount;
    return writer.GetHashValue();
  }

  WD_ALWAYS_INLINE static bool Equal(const wdImageCopyVulkan::FramebufferCacheKey& a, const wdImageCopyVulkan::FramebufferCacheKey& b)
  {
    return a.m_renderpass == b.m_renderpass && a.m_targetView == b.m_targetView && a.m_extends == b.m_extends && a.m_layerCount == b.m_layerCount;
  }
};

template <>
struct wdHashHelper<wdImageCopyVulkan::ImageViewCacheKey>
{
  WD_ALWAYS_INLINE static wdUInt32 Hash(const wdImageCopyVulkan::ImageViewCacheKey& value)
  {
    wdHashStreamWriter32 writer;
    writer << (VkImage)value.m_image;
    writer << static_cast<uint32_t>(value.m_subresourceLayers.aspectMask);
    writer << value.m_subresourceLayers.baseArrayLayer;
    writer << value.m_subresourceLayers.layerCount;
    writer << value.m_subresourceLayers.mipLevel;
    return writer.GetHashValue();
  }

  WD_ALWAYS_INLINE static bool Equal(const wdImageCopyVulkan::ImageViewCacheKey& a, const wdImageCopyVulkan::ImageViewCacheKey& b)
  {
    return a.m_image == b.m_image && a.m_subresourceLayers == b.m_subresourceLayers;
  }
};

template <>
struct wdHashHelper<wdShaderUtils::wdBuiltinShaderType>
{
  WD_ALWAYS_INLINE static wdUInt32 Hash(const wdShaderUtils::wdBuiltinShaderType& value)
  {
    wdHashStreamWriter32 writer;
    writer << wdConversionUtilsVulkan::GetUnderlyingValue(value);
    return writer.GetHashValue();
  }

  WD_ALWAYS_INLINE static bool Equal(const wdShaderUtils::wdBuiltinShaderType& a, const wdShaderUtils::wdBuiltinShaderType& b)
  {
    return a == b;
  }
};

wdUniquePtr<wdImageCopyVulkan::Cache> wdImageCopyVulkan::s_cache;

wdImageCopyVulkan::Cache::Cache(wdAllocatorBase* pAllocator)
  : m_vertexDeclarations(pAllocator)
  , m_renderPasses(pAllocator)
  , m_sourceImageViews(pAllocator)
  , m_imageToSourceImageViewCacheKey(pAllocator)
  , m_targetImageViews(pAllocator)
  , m_imageToTargetImageViewCacheKey(pAllocator)
  , m_framebuffers(pAllocator)
  , m_shaders(pAllocator)
{
}

wdImageCopyVulkan::Cache::~Cache() = default;

wdImageCopyVulkan::wdImageCopyVulkan(wdGALDeviceVulkan& GALDeviceVulkan)
  : m_GALDeviceVulkan(GALDeviceVulkan)
{
}

wdImageCopyVulkan::~wdImageCopyVulkan() = default;

void wdImageCopyVulkan::Initialize(wdGALDeviceVulkan& GALDeviceVulkan)
{
  s_cache = WD_NEW(&GALDeviceVulkan.GetAllocator(), wdImageCopyVulkan::Cache, &GALDeviceVulkan.GetAllocator());

  s_cache->m_onBeforeImageDeletedSubscription = GALDeviceVulkan.OnBeforeImageDestroyed.AddEventHandler(wdMakeDelegate(OnBeforeImageDestroyed));
}

void wdImageCopyVulkan::DeInitialize(wdGALDeviceVulkan& GALDeviceVulkan)
{
  GALDeviceVulkan.OnBeforeImageDestroyed.RemoveEventHandler(s_cache->m_onBeforeImageDeletedSubscription);
  for (auto& kv : (s_cache->m_vertexDeclarations))
  {
    GALDeviceVulkan.DestroyVertexDeclaration(kv.Value());
  }
  for (auto& kv : (s_cache->m_renderPasses))
  {
    GALDeviceVulkan.GetVulkanDevice().destroyRenderPass(kv.Value());
  }
  for (auto& kv : (s_cache->m_sourceImageViews))
  {
    GALDeviceVulkan.GetVulkanDevice().destroyImageView(kv.Value());
  }
  for (auto& kv : (s_cache->m_targetImageViews))
  {
    GALDeviceVulkan.GetVulkanDevice().destroyImageView(kv.Value());
  }
  for (auto& kv : (s_cache->m_framebuffers))
  {
    GALDeviceVulkan.GetVulkanDevice().destroyFramebuffer(kv.Value());
  }
  s_cache->m_shaders.Clear();
  s_cache = nullptr;
}

void wdImageCopyVulkan::OnBeforeImageDestroyed(wdGALDeviceVulkan::OnBeforeImageDestroyedData data)
{
  if (auto it = s_cache->m_imageToSourceImageViewCacheKey.Find(data.image); it.IsValid())
  {
    data.GALDeviceVulkan.GetVulkanDevice().destroyImageView(it.Value().m_imageView);

    ImageViewCacheKey cacheKey{data.image, it.Value().m_subresourceLayers};

    bool removed = s_cache->m_sourceImageViews.Remove(cacheKey);
    WD_IGNORE_UNUSED(removed);
    WD_ASSERT_DEV(removed, "m_imageToSourceImageViewCacheKey and m_sourceImageViews should always be in sync");
    s_cache->m_imageToSourceImageViewCacheKey.Remove(it);
  }
  if (auto it = s_cache->m_imageToTargetImageViewCacheKey.Find(data.image); it.IsValid())
  {
    data.GALDeviceVulkan.GetVulkanDevice().destroyImageView(it.Value().m_imageView);

    ImageViewCacheKey cacheKey{data.image, it.Value().m_subresourceLayers};

    bool removed = s_cache->m_targetImageViews.Remove(cacheKey);
    WD_IGNORE_UNUSED(removed);
    WD_ASSERT_DEV(removed, "m_imageToTargetImageViewCacheKey and m_targetImageViews should always be in sync");
    s_cache->m_imageToTargetImageViewCacheKey.Remove(it);
  }
}

void wdImageCopyVulkan::Init(const wdGALTextureVulkan* pSource, const wdGALTextureVulkan* pTarget, wdShaderUtils::wdBuiltinShaderType type)
{
  m_pSource = pSource;
  m_pTarget = pTarget;
  m_type = type;

  auto& targetDesc = m_pTarget->GetDescription();

  vk::Image targetImage = m_pTarget->GetImage();
  vk::Format targetFormat = m_pTarget->GetImageFormat();

  bool bTargetIsDepth = wdConversionUtilsVulkan::IsDepthFormat(targetFormat);
  WD_ASSERT_DEV(bTargetIsDepth == false, "Writing to depth is not implemented");

  // Render pass
  {
    RenderPassCacheKey cacheEntry = {};
    cacheEntry.targetFormat = targetFormat;
    cacheEntry.targetSamples = wdConversionUtilsVulkan::GetSamples(targetDesc.m_SampleCount);

    if (auto it = s_cache->m_renderPasses.Find(cacheEntry); it.IsValid())
    {
      m_renderPass = it.Value();
    }
    else
    {
      wdHybridArray<vk::AttachmentDescription, 4> attachments;
      wdHybridArray<vk::AttachmentReference, 4> colorAttachmentRefs;
      vk::AttachmentDescription& vkAttachment = attachments.ExpandAndGetRef();
      vkAttachment.format = cacheEntry.targetFormat;
      vkAttachment.samples = cacheEntry.targetSamples;
      vkAttachment.loadOp = vk::AttachmentLoadOp::eLoad; //#TODO_VULKAN we could replace this with don't care if we knew that all copy commands render to the entire sub-resource.
      vkAttachment.storeOp = vk::AttachmentStoreOp::eStore;
      vkAttachment.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
      vkAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

      vk::AttachmentReference& colorAttachment = colorAttachmentRefs.ExpandAndGetRef();
      colorAttachment.attachment = 0;
      colorAttachment.layout = vk::ImageLayout::eColorAttachmentOptimal;

      vk::SubpassDescription subpass;
      subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
      subpass.colorAttachmentCount = colorAttachmentRefs.GetCount();
      subpass.pColorAttachments = colorAttachmentRefs.GetData();
      subpass.pDepthStencilAttachment = nullptr;

      vk::SubpassDependency dependency;
      dependency.dstSubpass = 0;
      dependency.dstAccessMask |= vk::AccessFlagBits::eColorAttachmentWrite;

      dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
      dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
      dependency.srcAccessMask = {};
      dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;

      vk::RenderPassCreateInfo renderPassCreateInfo;
      renderPassCreateInfo.attachmentCount = attachments.GetCount();
      renderPassCreateInfo.pAttachments = attachments.GetData();
      renderPassCreateInfo.subpassCount = 1;
      renderPassCreateInfo.pSubpasses = &subpass;
      renderPassCreateInfo.dependencyCount = 1;
      renderPassCreateInfo.pDependencies = &dependency;

      VK_ASSERT_DEV(m_GALDeviceVulkan.GetVulkanDevice().createRenderPass(&renderPassCreateInfo, nullptr, &m_renderPass));
      s_cache->m_renderPasses.Insert(cacheEntry, m_renderPass);
    }
  }

  // Pipeline
  {

    if (auto it = s_cache->m_shaders.Find(type); it.IsValid())
    {
      m_shader = it.Value();
      m_PipelineDesc.m_pCurrentRasterizerState = static_cast<const wdGALRasterizerStateVulkan*>(m_GALDeviceVulkan.GetRasterizerState(m_shader.m_hRasterizerState));
      m_PipelineDesc.m_pCurrentBlendState = static_cast<const wdGALBlendStateVulkan*>(m_GALDeviceVulkan.GetBlendState(m_shader.m_hBlendState));
      m_PipelineDesc.m_pCurrentDepthStencilState = static_cast<const wdGALDepthStencilStateVulkan*>(m_GALDeviceVulkan.GetDepthStencilState(m_shader.m_hDepthStencilState));
      m_PipelineDesc.m_pCurrentShader = static_cast<const wdGALShaderVulkan*>(m_GALDeviceVulkan.GetShader(m_shader.m_hActiveGALShader));

      if (!m_PipelineDesc.m_pCurrentRasterizerState || !m_PipelineDesc.m_pCurrentBlendState || !m_PipelineDesc.m_pCurrentDepthStencilState || !m_PipelineDesc.m_pCurrentShader)
      {
        s_cache->m_shaders.Clear();
        for (auto& kv : (s_cache->m_vertexDeclarations))
        {
          m_GALDeviceVulkan.DestroyVertexDeclaration(kv.Value());
        }
        s_cache->m_vertexDeclarations.Clear();
      }
    }

    if (!m_PipelineDesc.m_pCurrentRasterizerState || !m_PipelineDesc.m_pCurrentBlendState || !m_PipelineDesc.m_pCurrentDepthStencilState || !m_PipelineDesc.m_pCurrentShader)
    {
      wdShaderUtils::RequestBuiltinShader(type, m_shader);
      s_cache->m_shaders.Insert(type, m_shader);

      m_PipelineDesc.m_pCurrentRasterizerState = static_cast<const wdGALRasterizerStateVulkan*>(m_GALDeviceVulkan.GetRasterizerState(m_shader.m_hRasterizerState));
      m_PipelineDesc.m_pCurrentBlendState = static_cast<const wdGALBlendStateVulkan*>(m_GALDeviceVulkan.GetBlendState(m_shader.m_hBlendState));
      m_PipelineDesc.m_pCurrentDepthStencilState = static_cast<const wdGALDepthStencilStateVulkan*>(m_GALDeviceVulkan.GetDepthStencilState(m_shader.m_hDepthStencilState));
      m_PipelineDesc.m_pCurrentShader = static_cast<const wdGALShaderVulkan*>(m_GALDeviceVulkan.GetShader(m_shader.m_hActiveGALShader));

      WD_ASSERT_DEV(m_PipelineDesc.m_pCurrentRasterizerState && m_PipelineDesc.m_pCurrentBlendState && m_PipelineDesc.m_pCurrentDepthStencilState && m_PipelineDesc.m_pCurrentShader, "");
    }

    m_PipelineDesc.m_renderPass = m_renderPass;
    m_PipelineDesc.m_topology = wdGALPrimitiveTopology::Triangles;
    m_PipelineDesc.m_msaa = targetDesc.m_SampleCount;
    m_PipelineDesc.m_uiAttachmentCount = 1;


    // Vertex declaration
    {
      if (auto it = s_cache->m_vertexDeclarations.Find(m_shader.m_hActiveGALShader); it.IsValid())
      {
        m_hVertexDecl = it.Value();
      }
      else
      {
        wdGALVertexDeclarationCreationDescription desc;
        desc.m_hShader = m_shader.m_hActiveGALShader;
        m_hVertexDecl = m_GALDeviceVulkan.CreateVertexDeclaration(desc);
        s_cache->m_vertexDeclarations.Insert(m_shader.m_hActiveGALShader, m_hVertexDecl);
      }
      m_PipelineDesc.m_pCurrentVertexDecl = static_cast<const wdGALVertexDeclarationVulkan*>(m_GALDeviceVulkan.GetVertexDeclaration(m_hVertexDecl));
    }

    const wdGALShaderVulkan::DescriptorSetLayoutDesc& descriptorLayoutDesc = m_PipelineDesc.m_pCurrentShader->GetDescriptorSetLayout();
    m_LayoutDesc.m_layout = wdResourceCacheVulkan::RequestDescriptorSetLayout(descriptorLayoutDesc);
    m_PipelineDesc.m_layout = wdResourceCacheVulkan::RequestPipelineLayout(m_LayoutDesc);
    m_pipeline = wdResourceCacheVulkan::RequestGraphicsPipeline(m_PipelineDesc);
  }
}

void wdImageCopyVulkan::Copy(const wdVec3U32& sourceOffset, const vk::ImageSubresourceLayers& sourceLayers, const wdVec3U32& targetOffset, const vk::ImageSubresourceLayers& targetLayers, const wdVec3U32& extends)
{
  WD_ASSERT_DEV(sourceOffset.IsZero(), "Offset not implemented yet.");
  WD_ASSERT_DEV(targetOffset.IsZero(), "Offset not implemented yet.");
  if (m_type == wdShaderUtils::wdBuiltinShaderType::CopyImage || m_type == wdShaderUtils::wdBuiltinShaderType::CopyImage)
  {
    WD_ASSERT_DEV(sourceLayers.layerCount == 1 && targetLayers.layerCount == 1, "If wdBuiltinShaderType is not one of the array variants, layerCount must be 1.");
  }

  vk::CommandBuffer commandBuffer = m_GALDeviceVulkan.GetCurrentCommandBuffer();
  wdPipelineBarrierVulkan& pipelineBarrier = m_GALDeviceVulkan.GetCurrentPipelineBarrier();

  // Barriers
  {
    const bool bSourceIsDepth = wdConversionUtilsVulkan::IsDepthFormat(m_pSource->GetImageFormat());
    pipelineBarrier.EnsureImageLayout(m_pSource, wdConversionUtilsVulkan::GetSubresourceRange(sourceLayers), bSourceIsDepth ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits::eFragmentShader, vk::AccessFlagBits::eShaderRead);
    pipelineBarrier.EnsureImageLayout(m_pTarget, wdConversionUtilsVulkan::GetSubresourceRange(targetLayers), vk::ImageLayout::eColorAttachmentOptimal, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite);
    pipelineBarrier.Flush();
  }

  RenderInternal(sourceOffset, sourceLayers, targetOffset, targetLayers, extends);
}

void wdImageCopyVulkan::RenderInternal(const wdVec3U32& sourceOffset, const vk::ImageSubresourceLayers& sourceLayers, const wdVec3U32& targetOffset, const vk::ImageSubresourceLayers& targetLayers, const wdVec3U32& extends)
{
  vk::CommandBuffer commandBuffer = m_GALDeviceVulkan.GetCurrentCommandBuffer();
  wdPipelineBarrierVulkan& pipelineBarrier = m_GALDeviceVulkan.GetCurrentPipelineBarrier();

  auto& sourceDesc = m_pSource->GetDescription();
  auto& targetDesc = m_pTarget->GetDescription();

  const bool bSourceIsDepth = wdGALResourceFormat::IsDepthFormat(sourceDesc.m_Format);

  vk::Image sourceImage = m_pSource->GetImage();
  vk::Format sourceFormat = m_pSource->GetImageFormat();

  vk::Image targetImage = m_pTarget->GetImage();
  vk::Format targetFormat = m_pTarget->GetImageFormat();

  vk::ImageView sourceView;
  vk::ImageView targetView;
  vk::Framebuffer frameBuffer;

  // Image Views
  {
    ImageViewCacheKey cacheKey = {};
    cacheKey.m_image = sourceImage;
    cacheKey.m_subresourceLayers = sourceLayers;

    if (auto it = s_cache->m_sourceImageViews.Find(cacheKey); it.IsValid())
    {
      sourceView = it.Value();
    }
    else
    {

      vk::ImageViewCreateInfo viewCreateInfo;
      viewCreateInfo.format = sourceFormat;
      viewCreateInfo.image = sourceImage;
      viewCreateInfo.subresourceRange = wdConversionUtilsVulkan::GetSubresourceRange(sourceLayers);
      viewCreateInfo.subresourceRange.aspectMask &= ~vk::ImageAspectFlagBits::eStencil;
      viewCreateInfo.viewType = wdConversionUtilsVulkan::GetImageViewType(sourceDesc.m_Type, true);
      if (viewCreateInfo.viewType == vk::ImageViewType::eCube || viewCreateInfo.viewType == vk::ImageViewType::eCubeArray)
      {
        viewCreateInfo.viewType = vk::ImageViewType::e2DArray;
      }
      VK_ASSERT_DEV(m_GALDeviceVulkan.GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &sourceView));
      m_GALDeviceVulkan.SetDebugName("ImageCopy-SRV", sourceView);
      s_cache->m_sourceImageViews.Insert(cacheKey, sourceView);
      s_cache->m_imageToSourceImageViewCacheKey.Insert(cacheKey.m_image, ImageViewCacheValue{cacheKey.m_subresourceLayers, sourceView});
    }
  }
  {
    ImageViewCacheKey cacheKey = {};
    cacheKey.m_image = targetImage;
    cacheKey.m_subresourceLayers = targetLayers;

    if (auto it = s_cache->m_targetImageViews.Find(cacheKey); it.IsValid())
    {
      targetView = it.Value();
    }
    else
    {

      vk::ImageViewCreateInfo viewCreateInfo;
      viewCreateInfo.format = targetFormat;
      viewCreateInfo.image = targetImage;
      viewCreateInfo.subresourceRange = wdConversionUtilsVulkan::GetSubresourceRange(targetLayers);
      viewCreateInfo.viewType = wdConversionUtilsVulkan::GetImageViewType(targetDesc.m_Type, true);
      if (viewCreateInfo.viewType == vk::ImageViewType::eCube || viewCreateInfo.viewType == vk::ImageViewType::eCubeArray)
      {
        viewCreateInfo.viewType = vk::ImageViewType::e2DArray;
      }
      VK_ASSERT_DEV(m_GALDeviceVulkan.GetVulkanDevice().createImageView(&viewCreateInfo, nullptr, &targetView));
      m_GALDeviceVulkan.SetDebugName("ImageCopy-RTV", targetView);
      s_cache->m_targetImageViews.Insert(cacheKey, targetView);
      s_cache->m_imageToTargetImageViewCacheKey.Insert(cacheKey.m_image, ImageViewCacheValue{cacheKey.m_subresourceLayers, targetView});
    }
  }

  // Framebuffer
  {
    FramebufferCacheKey cacheEntry = {};
    cacheEntry.m_renderpass = m_renderPass;
    cacheEntry.m_targetView = targetView;
    cacheEntry.m_extends = extends;
    cacheEntry.m_layerCount = targetLayers.layerCount;

    if (auto it = s_cache->m_framebuffers.Find(cacheEntry); it.IsValid())
    {
      frameBuffer = it.Value();
    }
    else
    {
      vk::FramebufferCreateInfo framebufferInfo;
      framebufferInfo.renderPass = m_renderPass;
      framebufferInfo.attachmentCount = 1;
      framebufferInfo.pAttachments = &targetView;
      framebufferInfo.width = extends.x;
      framebufferInfo.height = extends.y;
      framebufferInfo.layers = targetLayers.layerCount;
      VK_ASSERT_DEV(m_GALDeviceVulkan.GetVulkanDevice().createFramebuffer(&framebufferInfo, nullptr, &frameBuffer));

      s_cache->m_framebuffers.Insert(cacheEntry, frameBuffer);
    }
  }

  // Descriptor Set
  vk::DescriptorSet descriptorSet = wdDescriptorSetPoolVulkan::CreateDescriptorSet(m_LayoutDesc.m_layout);
  {
    wdHybridArray<vk::WriteDescriptorSet, 16> descriptorWrites;

    vk::DescriptorImageInfo sourceInfo;
    sourceInfo.imageLayout = bSourceIsDepth ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : vk::ImageLayout::eShaderReadOnlyOptimal;
    sourceInfo.imageView = sourceView;

    wdArrayPtr<const wdGALShaderVulkan::BindingMapping> bindingMapping = m_PipelineDesc.m_pCurrentShader->GetBindingMapping();
    const wdUInt32 uiCount = bindingMapping.GetCount();
    for (wdUInt32 i = 0; i < uiCount; i++)
    {
      const wdGALShaderVulkan::BindingMapping& mapping = bindingMapping[i];
      vk::WriteDescriptorSet& write = descriptorWrites.ExpandAndGetRef();
      write.dstArrayElement = 0;
      write.descriptorType = mapping.m_descriptorType;
      write.dstBinding = mapping.m_uiTarget;
      write.dstSet = descriptorSet;
      write.descriptorCount = 1;
      switch (mapping.m_type)
      {
        case wdGALShaderVulkan::BindingMapping::ConstantBuffer:
        {
          //#TODO_VULKAN constant buffer for offset in the shader to allow region copy
          //const wdGALBufferVulkan* pBuffer = m_pBoundConstantBuffers[mapping.m_uiSource];
          //write.pBufferInfo = &pBuffer->GetBufferInfo();
        }
        break;
        case wdGALShaderVulkan::BindingMapping::ResourceView:
        {
          write.pImageInfo = &sourceInfo;
        }
      }
    }
    wdDescriptorSetPoolVulkan::UpdateDescriptorSet(descriptorSet, descriptorWrites);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_PipelineDesc.m_layout, 0, 1, &descriptorSet, 0, nullptr);
  }

  // Render
  {
    {
      vk::RenderPassBeginInfo begin;
      begin.renderPass = m_PipelineDesc.m_renderPass;
      begin.framebuffer = frameBuffer;
      begin.renderArea.offset.setX(0).setY(0);
      begin.renderArea.extent.setWidth(extends.x).setHeight(extends.y);
      wdHybridArray<vk::ClearValue, WD_GAL_MAX_RENDERTARGET_COUNT + 1> m_clearValues;
      vk::ClearValue& colorClear = m_clearValues.ExpandAndGetRef();
      wdColor col = wdColor::Pink;
      colorClear.color.setFloat32({col.r, col.g, col.b, col.a});
      begin.clearValueCount = m_clearValues.GetCount();
      begin.pClearValues = m_clearValues.GetData();

      commandBuffer.beginRenderPass(begin, vk::SubpassContents::eInline);
    }

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
    vk::Viewport viewport((float)targetOffset.x, (float)extends.y + (float)targetOffset.y, (float)extends.x, -(float)extends.y, 0, 1.0f);
    commandBuffer.setViewport(0, 1, &viewport);
    vk::Rect2D noScissor({int(viewport.x), int(viewport.y + viewport.height)}, {wdUInt32(viewport.width), wdUInt32(-viewport.height)});
    commandBuffer.setScissor(0, 1, &noScissor);
    commandBuffer.draw(3, targetLayers.layerCount, 0, 0);

    commandBuffer.endRenderPass();
  }
}
