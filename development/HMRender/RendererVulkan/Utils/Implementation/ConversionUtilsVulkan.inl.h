#include <RendererFoundation/Resources/ResourceFormats.h>

namespace
{
  bool IsArrayViewInternal(const wdGALTextureCreationDescription& texDesc, const wdGALResourceViewCreationDescription& viewDesc)
  {
    return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstArraySlice > 0;
  }
  bool IsArrayViewInternal(const wdGALTextureCreationDescription& texDesc, const wdGALUnorderedAccessViewCreationDescription& viewDesc)
  {
    return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstArraySlice > 0;
  }
} // namespace

WD_ALWAYS_INLINE vk::SampleCountFlagBits wdConversionUtilsVulkan::GetSamples(wdEnum<wdGALMSAASampleCount> samples)
{
  switch (samples)
  {
    case wdGALMSAASampleCount::None:
      return vk::SampleCountFlagBits::e1;
    case wdGALMSAASampleCount::TwoSamples:
      return vk::SampleCountFlagBits::e2;
    case wdGALMSAASampleCount::FourSamples:
      return vk::SampleCountFlagBits::e4;
    case wdGALMSAASampleCount::EightSamples:
      return vk::SampleCountFlagBits::e8;
    default:
      WD_ASSERT_NOT_IMPLEMENTED;
      return vk::SampleCountFlagBits::e1;
  }
}

WD_ALWAYS_INLINE vk::PresentModeKHR wdConversionUtilsVulkan::GetPresentMode(wdEnum<wdGALPresentMode> presentMode, const wdDynamicArray<vk::PresentModeKHR>& supportedModes)
{
  switch (presentMode)
  {
    case wdGALPresentMode::Immediate:
    {
      if (supportedModes.Contains(vk::PresentModeKHR::eImmediate))
        return vk::PresentModeKHR::eImmediate;
      else if (supportedModes.Contains(vk::PresentModeKHR::eMailbox))
        return vk::PresentModeKHR::eMailbox;
      else
        return vk::PresentModeKHR::eFifo;
    }
    case wdGALPresentMode::VSync:
      return vk::PresentModeKHR::eFifo; // FIFO must be supported according to the standard.
    default:
      WD_ASSERT_NOT_IMPLEMENTED;
      return vk::PresentModeKHR::eFifo;
  }
}

WD_ALWAYS_INLINE vk::ImageSubresourceRange wdConversionUtilsVulkan::GetSubresourceRange(const wdGALTextureCreationDescription& texDesc, const wdGALRenderTargetViewCreationDescription& viewDesc)
{
  vk::ImageSubresourceRange range;
  wdGALResourceFormat::Enum viewFormat = viewDesc.m_OverrideViewFormat == wdGALResourceFormat::Invalid ? texDesc.m_Format : viewDesc.m_OverrideViewFormat;
  range.aspectMask = wdGALResourceFormat::IsDepthFormat(viewFormat) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  range.setBaseMipLevel(viewDesc.m_uiMipLevel).setLevelCount(1).setBaseArrayLayer(viewDesc.m_uiFirstSlice).setLayerCount(viewDesc.m_uiSliceCount);
  return range;
}

WD_ALWAYS_INLINE vk::ImageSubresourceRange wdConversionUtilsVulkan::GetSubresourceRange(const wdGALTextureCreationDescription& texDesc, const wdGALResourceViewCreationDescription& viewDesc)
{
  vk::ImageSubresourceRange range;

  const bool bIsArrayView = IsArrayViewInternal(texDesc, viewDesc);

  wdGALResourceFormat::Enum viewFormat = viewDesc.m_OverrideViewFormat == wdGALResourceFormat::Invalid ? texDesc.m_Format : viewDesc.m_OverrideViewFormat;
  range.aspectMask = wdGALResourceFormat::IsDepthFormat(viewFormat) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  if (viewFormat == wdGALResourceFormat::D24S8)
  {
    range.aspectMask |= vk::ImageAspectFlagBits::eStencil;
  }
  range.baseMipLevel = viewDesc.m_uiMostDetailedMipLevel;
  range.levelCount = wdMath::Min(viewDesc.m_uiMipLevelsToUse, texDesc.m_uiMipLevelCount - range.baseMipLevel);

  switch (texDesc.m_Type)
  {
    case wdGALTextureType::Texture2D:
    case wdGALTextureType::Texture2DProxy:
      range.layerCount = viewDesc.m_uiArraySize;
      range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      break;
    case wdGALTextureType::TextureCube:
      range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      range.layerCount = viewDesc.m_uiArraySize * 6;
      break;
    case wdGALTextureType::Texture3D:
      range.layerCount = 1;
      break;
    default:
      WD_ASSERT_NOT_IMPLEMENTED;
  }
  return range;
}


WD_ALWAYS_INLINE vk::ImageSubresourceRange wdConversionUtilsVulkan::GetSubresourceRange(const wdGALTextureCreationDescription& texDesc, const wdGALUnorderedAccessViewCreationDescription& viewDesc)
{
  vk::ImageSubresourceRange range;

  const bool bIsArrayView = IsArrayViewInternal(texDesc, viewDesc);

  wdGALResourceFormat::Enum viewFormat = viewDesc.m_OverrideViewFormat == wdGALResourceFormat::Invalid ? texDesc.m_Format : viewDesc.m_OverrideViewFormat;
  range.aspectMask = wdGALResourceFormat::IsDepthFormat(viewFormat) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  if (viewFormat == wdGALResourceFormat::D24S8)
  {
    range.aspectMask |= vk::ImageAspectFlagBits::eStencil;
  }

  range.baseMipLevel = viewDesc.m_uiMipLevelToUse;
  range.levelCount = 1;
  range.layerCount = viewDesc.m_uiArraySize;

  switch (texDesc.m_Type)
  {
    case wdGALTextureType::Texture2D:
    case wdGALTextureType::Texture2DProxy:
      range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      break;
    case wdGALTextureType::TextureCube:
      range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      break;
    case wdGALTextureType::Texture3D:
      if (bIsArrayView)
      {
        WD_ASSERT_NOT_IMPLEMENTED;
      }
      else
      {
        range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      }
      break;
    default:
      WD_ASSERT_NOT_IMPLEMENTED;
  }
  return range;
}

WD_ALWAYS_INLINE vk::ImageSubresourceRange wdConversionUtilsVulkan::GetSubresourceRange(
  const vk::ImageSubresourceLayers& layers)
{
  vk::ImageSubresourceRange range;
  range.aspectMask = layers.aspectMask;
  range.baseMipLevel = layers.mipLevel;
  range.levelCount = 1;
  range.baseArrayLayer = layers.baseArrayLayer;
  range.layerCount = layers.layerCount;
  return range;
}

WD_ALWAYS_INLINE vk::ImageViewType wdConversionUtilsVulkan::GetImageViewType(wdEnum<wdGALTextureType> texType, bool bIsArrayView)
{
  switch (texType)
  {
    case wdGALTextureType::Texture2D:
    case wdGALTextureType::Texture2DProxy:
      if (!bIsArrayView)
      {
        return vk::ImageViewType::e2D;
      }
      else
      {
        return vk::ImageViewType::e2DArray;
      }
    case wdGALTextureType::TextureCube:
      if (!bIsArrayView)
      {
        return vk::ImageViewType::eCube;
      }
      else
      {
        return vk::ImageViewType::eCubeArray;
      }
    case wdGALTextureType::Texture3D:
      return vk::ImageViewType::e3D;

    default:
      WD_ASSERT_NOT_IMPLEMENTED;
      return vk::ImageViewType::e1D;
  }
}

WD_ALWAYS_INLINE bool wdConversionUtilsVulkan::IsDepthFormat(vk::Format format)
{
  switch (format)
  {
    case vk::Format::eD16Unorm:
    case vk::Format::eD32Sfloat:
    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD32SfloatS8Uint:
      return true;
    default:
      return false;
  }
}

WD_ALWAYS_INLINE bool wdConversionUtilsVulkan::IsStencilFormat(vk::Format format)
{
  switch (format)
  {
    case vk::Format::eS8Uint:
    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD32SfloatS8Uint:
      return true;
    default:
      return false;
  }
}

WD_ALWAYS_INLINE vk::PrimitiveTopology wdConversionUtilsVulkan::GetPrimitiveTopology(wdEnum<wdGALPrimitiveTopology> topology)
{
  switch (topology)
  {
    case wdGALPrimitiveTopology::Points:
      return vk::PrimitiveTopology::ePointList;
    case wdGALPrimitiveTopology::Lines:
      return vk::PrimitiveTopology::eLineList;
    case wdGALPrimitiveTopology::Triangles:
      return vk::PrimitiveTopology::eTriangleList;
    default:
      WD_ASSERT_NOT_IMPLEMENTED;
      return vk::PrimitiveTopology::ePointList;
  }
}

WD_ALWAYS_INLINE vk::ShaderStageFlagBits wdConversionUtilsVulkan::GetShaderStage(wdGALShaderStage::Enum stage)
{
  switch (stage)
  {
    case wdGALShaderStage::VertexShader:
      return vk::ShaderStageFlagBits::eVertex;
    case wdGALShaderStage::HullShader:
      return vk::ShaderStageFlagBits::eTessellationControl;
    case wdGALShaderStage::DomainShader:
      return vk::ShaderStageFlagBits::eTessellationEvaluation;
    case wdGALShaderStage::GeometryShader:
      return vk::ShaderStageFlagBits::eGeometry;
    case wdGALShaderStage::PixelShader:
      return vk::ShaderStageFlagBits::eFragment;
    default:
      WD_ASSERT_NOT_IMPLEMENTED;
      [[fallthrough]];
    case wdGALShaderStage::ComputeShader:
      return vk::ShaderStageFlagBits::eCompute;
  }
}

WD_ALWAYS_INLINE vk::PipelineStageFlags wdConversionUtilsVulkan::GetPipelineStage(wdGALShaderStage::Enum stage)
{
  switch (stage)
  {
    case wdGALShaderStage::VertexShader:
      return vk::PipelineStageFlagBits::eVertexShader;
    case wdGALShaderStage::HullShader:
      return vk::PipelineStageFlagBits::eTessellationControlShader;
    case wdGALShaderStage::DomainShader:
      return vk::PipelineStageFlagBits::eTessellationEvaluationShader;
    case wdGALShaderStage::GeometryShader:
      return vk::PipelineStageFlagBits::eGeometryShader;
    case wdGALShaderStage::PixelShader:
      return vk::PipelineStageFlagBits::eFragmentShader;
    default:
      WD_ASSERT_NOT_IMPLEMENTED;
      [[fallthrough]];
    case wdGALShaderStage::ComputeShader:
      return vk::PipelineStageFlagBits::eComputeShader;
  }
}

WD_ALWAYS_INLINE vk::PipelineStageFlags wdConversionUtilsVulkan::GetPipelineStage(vk::ShaderStageFlags flags)
{
  vk::PipelineStageFlags res;
  if (flags & vk::ShaderStageFlagBits::eVertex)
    res |= vk::PipelineStageFlagBits::eVertexShader;
  if (flags & vk::ShaderStageFlagBits::eTessellationControl)
    res |= vk::PipelineStageFlagBits::eTessellationControlShader;
  if (flags & vk::ShaderStageFlagBits::eTessellationEvaluation)
    res |= vk::PipelineStageFlagBits::eTessellationEvaluationShader;
  if (flags & vk::ShaderStageFlagBits::eGeometry)
    res |= vk::PipelineStageFlagBits::eGeometryShader;
  if (flags & vk::ShaderStageFlagBits::eFragment)
    res |= vk::PipelineStageFlagBits::eFragmentShader;
  if (flags & vk::ShaderStageFlagBits::eCompute)
    res |= vk::PipelineStageFlagBits::eComputeShader;

  return res;
}
