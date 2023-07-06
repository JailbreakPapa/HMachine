#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Algorithm/HashStream.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>
#include <ShaderCompilerDXC/SpirvMetaData.h>

WD_CHECK_AT_COMPILETIME(wdVulkanDescriptorSetLayoutBinding::ConstantBuffer == wdGALShaderVulkan::BindingMapping::ConstantBuffer);
WD_CHECK_AT_COMPILETIME(wdVulkanDescriptorSetLayoutBinding::ResourceView == wdGALShaderVulkan::BindingMapping::ResourceView);
WD_CHECK_AT_COMPILETIME(wdVulkanDescriptorSetLayoutBinding::UAV == wdGALShaderVulkan::BindingMapping::UAV);
WD_CHECK_AT_COMPILETIME(wdVulkanDescriptorSetLayoutBinding::Sampler == wdGALShaderVulkan::BindingMapping::Sampler);

void wdGALShaderVulkan::DescriptorSetLayoutDesc::ComputeHash()
{
  wdHashStreamWriter32 writer;
  const wdUInt32 uiSize = m_bindings.GetCount();
  for (wdUInt32 i = 0; i < uiSize; i++)
  {
    const auto& binding = m_bindings[i];
    writer << binding.binding;
    writer << wdConversionUtilsVulkan::GetUnderlyingValue(binding.descriptorType);
    writer << binding.descriptorCount;
    writer << wdConversionUtilsVulkan::GetUnderlyingFlagsValue(binding.stageFlags);
    writer << binding.pImmutableSamplers;
  }
  m_uiHash = writer.GetHashValue();
}

wdGALShaderVulkan::wdGALShaderVulkan(const wdGALShaderCreationDescription& Description)
  : wdGALShader(Description)
{
}

wdGALShaderVulkan::~wdGALShaderVulkan() {}

void wdGALShaderVulkan::SetDebugName(const char* szName) const
{
  wdGALDeviceVulkan* pVulkanDevice = static_cast<wdGALDeviceVulkan*>(wdGALDevice::GetDefaultDevice());
  for (wdUInt32 i = 0; i < wdGALShaderStage::ENUM_COUNT; i++)
  {
    pVulkanDevice->SetDebugName(szName, m_Shaders[i]);
  }
}

wdResult wdGALShaderVulkan::InitPlatform(wdGALDevice* pDevice)
{
  wdGALDeviceVulkan* pVulkanDevice = static_cast<wdGALDeviceVulkan*>(pDevice);

  // Extract meta data and shader code.
  wdArrayPtr<const wdUInt8> shaderCode[wdGALShaderStage::ENUM_COUNT];
  wdDynamicArray<wdVulkanDescriptorSetLayout> sets[wdGALShaderStage::ENUM_COUNT];
  wdHybridArray<wdVulkanVertexInputAttribute, 8> vertexInputAttributes;

  for (wdUInt32 i = 0; i < wdGALShaderStage::ENUM_COUNT; i++)
  {
    if (m_Description.HasByteCodeForStage((wdGALShaderStage::Enum)i))
    {
      wdArrayPtr<const wdUInt8> metaData(reinterpret_cast<const wdUInt8*>(m_Description.m_ByteCodes[i]->GetByteCode()), m_Description.m_ByteCodes[i]->GetSize());
      // Only the vertex shader stores vertexInputAttributes, so passing in the array into other shaders is just a no op.
      wdSpirvMetaData::Read(metaData, shaderCode[i], sets[i], vertexInputAttributes);
    }
  }

  // For now the meta data and what the shader exposes is the exact same data but this might change so different types are used.
  for (wdVulkanVertexInputAttribute& via : vertexInputAttributes)
  {
    m_VertexInputAttributes.PushBack({via.m_eSemantic, via.m_uiLocation, via.m_eFormat});
  }

  // Compute remapping.
  // Each shader stage is compiled individually and has its own binding indices.
  // In Vulkan we need to map all stages into one descriptor layout which requires us to remap some shader stages so no binding index conflicts appear.
  struct ShaderRemapping
  {
    const wdVulkanDescriptorSetLayoutBinding* pBinding = 0;
    wdUInt16 m_uiTarget = 0; ///< The new binding target that pBinding needs to be remapped to.
  };
  struct LayoutBinding
  {
    const wdVulkanDescriptorSetLayoutBinding* m_binding = nullptr; ///< The first binding under which this resource was encountered.
    vk::ShaderStageFlags m_stages = {};                            ///< Bitflags of all stages that share this binding. Matching is done by name.
  };
  wdHybridArray<ShaderRemapping, 6> remappings[wdGALShaderStage::ENUM_COUNT]; ///< Remappings for each shader stage.
  wdHybridArray<LayoutBinding, 6> sourceBindings;                             ///< Bindings across all stages. Can have gaps. Array index is the binding index.
  wdMap<wdStringView, wdUInt32> bindingMap;                                   ///< Maps binding name to index in sourceBindings.

  for (wdUInt32 i = 0; i < wdGALShaderStage::ENUM_COUNT; i++)
  {
    const vk::ShaderStageFlags vulkanStage = wdConversionUtilsVulkan::GetShaderStage((wdGALShaderStage::Enum)i);
    if (m_Description.HasByteCodeForStage((wdGALShaderStage::Enum)i))
    {
      WD_ASSERT_DEV(sets[i].GetCount() <= 1, "Only a single descriptor set is currently supported.");

      for (wdUInt32 j = 0; j < sets[i].GetCount(); j++)
      {
        const wdVulkanDescriptorSetLayout& set = sets[i][j];
        WD_ASSERT_DEV(set.m_uiSet == 0, "Only a single descriptor set is currently supported.");
        for (wdUInt32 k = 0; k < set.bindings.GetCount(); k++)
        {
          const wdVulkanDescriptorSetLayoutBinding& binding = set.bindings[k];
          // Does a binding already exist for the resource with the same name?
          if (wdUInt32* pBindingIdx = bindingMap.GetValue(binding.m_sName))
          {
            LayoutBinding& layoutBinding = sourceBindings[*pBindingIdx];
            layoutBinding.m_stages |= vulkanStage;
            const wdVulkanDescriptorSetLayoutBinding* pCurrentBinding = layoutBinding.m_binding;
            WD_ASSERT_DEBUG(pCurrentBinding->m_Type == binding.m_Type, "The descriptor {} was found with different resource type {} and {}", binding.m_sName, pCurrentBinding->m_Type, binding.m_Type);
            WD_ASSERT_DEBUG(pCurrentBinding->m_uiDescriptorType == binding.m_uiDescriptorType, "The descriptor {} was found with different type {} and {}", binding.m_sName, pCurrentBinding->m_uiDescriptorType, binding.m_uiDescriptorType);
            WD_ASSERT_DEBUG(pCurrentBinding->m_uiDescriptorCount == binding.m_uiDescriptorCount, "The descriptor {} was found with different count {} and {}", binding.m_sName, pCurrentBinding->m_uiDescriptorCount, binding.m_uiDescriptorCount);
            // The binding index differs from the one already in the set, remapping is necessary.
            if (binding.m_uiBinding != *pBindingIdx)
            {
              remappings[i].PushBack({&binding, pCurrentBinding->m_uiBinding});
            }
          }
          else
          {
            wdUInt8 uiTargetBinding = binding.m_uiBinding;
            // Doesn't exist yet, find a good place for it.
            if (binding.m_uiBinding >= sourceBindings.GetCount())
              sourceBindings.SetCount(binding.m_uiBinding + 1);

            // If the original binding index doesn't exist yet, use it (No remapping necessary).
            if (sourceBindings[binding.m_uiBinding].m_binding == nullptr)
            {
              sourceBindings[binding.m_uiBinding] = {&binding, vulkanStage};
              bindingMap[binding.m_sName] = uiTargetBinding;
            }
            else
            {
              // Binding index already in use, remapping necessary.
              uiTargetBinding = (wdUInt8)sourceBindings.GetCount();
              sourceBindings.PushBack({&binding, vulkanStage});
              bindingMap[binding.m_sName] = uiTargetBinding;
              remappings[i].PushBack({&binding, uiTargetBinding});
            }

            // The shader reflection used by the high level renderer is per stage and assumes it can map resources to stages.
            // We build this remapping table to map our descriptor binding to the original per-stage resource binding model.
            BindingMapping& bindingMapping = m_BindingMapping.ExpandAndGetRef();
            bindingMapping.m_descriptorType = (vk::DescriptorType)binding.m_uiDescriptorType;
            bindingMapping.m_wdType = binding.m_wdType;
            bindingMapping.m_type = (BindingMapping::Type)binding.m_Type;
            bindingMapping.m_stage = (wdGALShaderStage::Enum)i;
            bindingMapping.m_uiSource = binding.m_uiVirtualBinding;
            bindingMapping.m_uiTarget = uiTargetBinding;
            bindingMapping.m_sName = binding.m_sName;
          }
        }
      }
    }
  }
  m_BindingMapping.Sort([](const BindingMapping& lhs, const BindingMapping& rhs) { return lhs.m_uiTarget < rhs.m_uiTarget; });
  for (wdUInt32 i = 0; i < m_BindingMapping.GetCount(); i++)
  {
    m_BindingMapping[i].m_targetStages = wdConversionUtilsVulkan::GetPipelineStage(sourceBindings[m_BindingMapping[i].m_uiTarget].m_stages);
  }

  // Build Vulkan descriptor set layout
  for (wdUInt32 i = 0; i < sourceBindings.GetCount(); i++)
  {
    const LayoutBinding& sourceBinding = sourceBindings[i];
    if (sourceBinding.m_binding != nullptr)
    {
      vk::DescriptorSetLayoutBinding& binding = m_descriptorSetLayoutDesc.m_bindings.ExpandAndGetRef();
      binding.binding = i;
      binding.descriptorType = (vk::DescriptorType)sourceBinding.m_binding->m_uiDescriptorType;
      binding.descriptorCount = sourceBinding.m_binding->m_uiDescriptorCount;
      binding.stageFlags = sourceBinding.m_stages;
    }
  }
  m_descriptorSetLayoutDesc.m_bindings.Sort([](const vk::DescriptorSetLayoutBinding& lhs, const vk::DescriptorSetLayoutBinding& rhs) { return lhs.binding < rhs.binding; });
  m_descriptorSetLayoutDesc.ComputeHash();

  // Remap and build shaders
  wdUInt32 uiMaxShaderSize = 0;
  for (wdUInt32 i = 0; i < wdGALShaderStage::ENUM_COUNT; i++)
  {
    if (!remappings[i].IsEmpty())
    {
      uiMaxShaderSize = wdMath::Max(uiMaxShaderSize, shaderCode[i].GetCount());
    }
  }

  vk::ShaderModuleCreateInfo createInfo;
  wdDynamicArray<wdUInt8> tempBuffer;
  tempBuffer.Reserve(uiMaxShaderSize);
  for (wdUInt32 i = 0; i < wdGALShaderStage::ENUM_COUNT; i++)
  {
    if (m_Description.HasByteCodeForStage((wdGALShaderStage::Enum)i))
    {
      if (remappings[i].IsEmpty())
      {
        createInfo.codeSize = shaderCode[i].GetCount();
        WD_ASSERT_DEV(createInfo.codeSize % 4 == 0, "Spirv shader code should be a multiple of 4.");
        createInfo.pCode = reinterpret_cast<const wdUInt32*>(shaderCode[i].GetPtr());
        VK_SUCCEED_OR_RETURN_WD_FAILURE(pVulkanDevice->GetVulkanDevice().createShaderModule(&createInfo, nullptr, &m_Shaders[i]));
      }
      else
      {
        tempBuffer = shaderCode[i];
        wdUInt32* pData = reinterpret_cast<wdUInt32*>(tempBuffer.GetData());
        for (const auto& remap : remappings[i])
        {
          WD_ASSERT_DEBUG(pData[remap.pBinding->m_uiWordOffset] == remap.pBinding->m_uiBinding, "Spirv descriptor word offset does not point to descriptor index.");
          pData[remap.pBinding->m_uiWordOffset] = remap.m_uiTarget;
        }
        createInfo.codeSize = tempBuffer.GetCount();
        WD_ASSERT_DEV(createInfo.codeSize % 4 == 0, "Spirv shader code should be a multiple of 4.");
        createInfo.pCode = pData;
        VK_SUCCEED_OR_RETURN_WD_FAILURE(pVulkanDevice->GetVulkanDevice().createShaderModule(&createInfo, nullptr, &m_Shaders[i]));
      }
    }
  }

  return WD_SUCCESS;
}

wdResult wdGALShaderVulkan::DeInitPlatform(wdGALDevice* pDevice)
{
  m_descriptorSetLayoutDesc = {};
  m_BindingMapping.Clear();

  wdGALDeviceVulkan* pVulkanDevice = static_cast<wdGALDeviceVulkan*>(pDevice);
  for (wdUInt32 i = 0; i < wdGALShaderStage::ENUM_COUNT; i++)
  {
    pVulkanDevice->DeleteLater(m_Shaders[i]);
  }
  return WD_SUCCESS;
}

WD_STATICLINK_FILE(RendererVulkan, RendererVulkan_Shader_Implementation_ShaderVulkan);
