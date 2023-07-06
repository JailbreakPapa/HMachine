
#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/Shader.h>

#include <RendererCore/Shader/ShaderStageBinary.h>
#include <vulkan/vulkan.hpp>

class WD_RENDERERVULKAN_DLL wdGALShaderVulkan : public wdGALShader
{
public:
  /// \brief Used as input to wdResourceCacheVulkan::RequestDescriptorSetLayout to create a vk::DescriptorSetLayout.
  struct DescriptorSetLayoutDesc
  {
    mutable wdUInt32 m_uiHash = 0;
    wdHybridArray<vk::DescriptorSetLayoutBinding, 6> m_bindings;
    void ComputeHash();
  };

  /// \brief Remaps high level resource binding to the descriptor layout used by this shader.
  struct BindingMapping
  {
    enum Type : wdUInt8
    {
      ConstantBuffer,
      ResourceView,
      UAV,
      Sampler,
    };
    vk::DescriptorType m_descriptorType = vk::DescriptorType::eSampler;  ///< Descriptor slot type.
    wdShaderResourceType::Enum m_wdType = wdShaderResourceType::Unknown; ///< WD resource type. We need this to find a compatible fallback resource is a descriptor slot is empty.
    Type m_type = Type::ConstantBuffer;                                  ///< Source resource type in the high level binding model.
    wdGALShaderStage::Enum m_stage = wdGALShaderStage::ENUM_COUNT;       ///< Source stage in the high level resource binding model.
    wdUInt8 m_uiSource = 0;                                              ///< Source binding index in the high level resource binding model.
    wdUInt8 m_uiTarget = 0;                                              ///< Target binding index in the descriptor set layout.
    vk::PipelineStageFlags m_targetStages;                               ///< Target stages that this mapping is used in.
    wdStringView m_sName;
  };

  struct VertexInputAttribute
  {
    wdGALVertexAttributeSemantic::Enum m_eSemantic = wdGALVertexAttributeSemantic::Position;
    wdUInt8 m_uiLocation = 0;
    wdGALResourceFormat::Enum m_eFormat = wdGALResourceFormat::XYZFloat;
  };

  void SetDebugName(const char* szName) const override;

  WD_ALWAYS_INLINE vk::ShaderModule GetShader(wdGALShaderStage::Enum stage) const;
  WD_ALWAYS_INLINE const DescriptorSetLayoutDesc& GetDescriptorSetLayout() const;
  WD_ALWAYS_INLINE const wdArrayPtr<const BindingMapping> GetBindingMapping() const;
  WD_ALWAYS_INLINE const wdArrayPtr<const VertexInputAttribute> GetVertexInputAttributes() const;

protected:
  friend class wdGALDeviceVulkan;
  friend class wdMemoryUtils;

  wdGALShaderVulkan(const wdGALShaderCreationDescription& description);
  virtual ~wdGALShaderVulkan();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) override;
  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

private:
  DescriptorSetLayoutDesc m_descriptorSetLayoutDesc;
  wdHybridArray<BindingMapping, 16> m_BindingMapping;
  wdHybridArray<VertexInputAttribute, 8> m_VertexInputAttributes;
  vk::ShaderModule m_Shaders[wdGALShaderStage::ENUM_COUNT];
};

#include <RendererVulkan/Shader/Implementation/ShaderVulkan_inl.h>
