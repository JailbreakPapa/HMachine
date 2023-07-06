#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

#include <vulkan/vulkan.hpp>

WD_DEFINE_AS_POD_TYPE(vk::PresentModeKHR);

/// \brief Helper functions to convert and extract Vulkan objects from WD objects.
class WD_RENDERERVULKAN_DLL wdConversionUtilsVulkan
{
public:
  /// \brief Helper function to hash vk enums.
  template <typename T, typename R = typename std::underlying_type<T>::type>
  static R GetUnderlyingValue(T value)
  {
    return static_cast<typename std::underlying_type<T>::type>(value);
  }

  /// \brief Helper function to hash vk flags.
  template <typename T>
  static auto GetUnderlyingFlagsValue(T value)
  {
    return static_cast<typename T::MaskType>(value);
  }

  static vk::SampleCountFlagBits GetSamples(wdEnum<wdGALMSAASampleCount> samples);
  static vk::PresentModeKHR GetPresentMode(wdEnum<wdGALPresentMode> presentMode, const wdDynamicArray<vk::PresentModeKHR>& supportedModes);
  static vk::ImageSubresourceRange GetSubresourceRange(const wdGALTextureCreationDescription& texDesc, const wdGALRenderTargetViewCreationDescription& desc);
  static vk::ImageSubresourceRange GetSubresourceRange(const wdGALTextureCreationDescription& texDesc, const wdGALResourceViewCreationDescription& viewDesc);
  static vk::ImageSubresourceRange GetSubresourceRange(const wdGALTextureCreationDescription& texDesc, const wdGALUnorderedAccessViewCreationDescription& viewDesc);
  static vk::ImageSubresourceRange GetSubresourceRange(const vk::ImageSubresourceLayers& layers);
  static vk::ImageViewType GetImageViewType(wdEnum<wdGALTextureType> texType, bool bIsArray);

  static bool IsDepthFormat(vk::Format format);
  static bool IsStencilFormat(vk::Format format);
  static vk::PrimitiveTopology GetPrimitiveTopology(wdEnum<wdGALPrimitiveTopology> topology);
  static vk::ShaderStageFlagBits GetShaderStage(wdGALShaderStage::Enum stage);
  static vk::PipelineStageFlags GetPipelineStage(wdGALShaderStage::Enum stage);
  static vk::PipelineStageFlags GetPipelineStage(vk::ShaderStageFlags flags);
};

#include <RendererVulkan/Utils/Implementation/ConversionUtilsVulkan.inl.h>
