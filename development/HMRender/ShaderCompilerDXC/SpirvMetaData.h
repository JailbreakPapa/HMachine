#pragma once
#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

struct wdVulkanDescriptorSetLayoutBinding
{
  enum ResourceType : wdUInt8
  {
    ConstantBuffer,
    ResourceView,
    UAV,
    Sampler,
  };

  WD_DECLARE_POD_TYPE();
  wdStringView m_sName;                                                ///< Used to match the same descriptor use across multiple stages.
  wdUInt8 m_uiBinding = 0;                                             ///< Target descriptor binding slot.
  wdUInt8 m_uiVirtualBinding = 0;                                      ///< Virtual binding slot in the high level renderer interface.
  wdShaderResourceType::Enum m_wdType = wdShaderResourceType::Unknown; ///< WD shader resource type, needed to find compatible fallback resources.
  ResourceType m_Type = ResourceType::ConstantBuffer;                  ///< Resource type, used to map to the correct WD resource type.
  wdUInt16 m_uiDescriptorType = 0;                                     ///< Maps to vk::DescriptorType
  wdUInt32 m_uiDescriptorCount = 1;                                    ///< For now, this must be 1 as WD does not support descriptor arrays right now.
  wdUInt32 m_uiWordOffset = 0;                                         ///< Offset of the location in the spirv code where the binding index is located to allow changing it at runtime.
};

struct wdVulkanDescriptorSetLayout
{
  wdUInt32 m_uiSet = 0;
  wdHybridArray<wdVulkanDescriptorSetLayoutBinding, 6> bindings;
};

struct wdVulkanVertexInputAttribute
{
  wdGALVertexAttributeSemantic::Enum m_eSemantic = wdGALVertexAttributeSemantic::Position;
  wdUInt8 m_uiLocation = 0;
  wdGALResourceFormat::Enum m_eFormat = wdGALResourceFormat::XYZFloat;
};

namespace wdSpirvMetaData
{
  constexpr wdUInt32 s_uiSpirvMetaDataMagicNumber = 0x4B565A45; //WDVK

  enum MetaDataVersion
  {
    Version1 = 1,
    Version2 = 2, ///< m_uiVirtualBinding, m_wdType added
    Version3 = 3, ///< Vertex input binding
  };

  void Write(wdStreamWriter& stream, const wdArrayPtr<wdUInt8>& shaderCode, const wdDynamicArray<wdVulkanDescriptorSetLayout>& sets, const wdDynamicArray<wdVulkanVertexInputAttribute>& vertexInputAttributes)
  {
    stream << s_uiSpirvMetaDataMagicNumber;
    stream.WriteVersion(MetaDataVersion::Version3);
    const wdUInt32 uiSize = shaderCode.GetCount();
    stream << uiSize;
    stream.WriteBytes(shaderCode.GetPtr(), uiSize).AssertSuccess();

    const wdUInt8 uiSets = sets.GetCount();
    stream << uiSets;
    for (wdUInt8 i = 0; i < uiSets; i++)
    {
      const wdVulkanDescriptorSetLayout& set = sets[i];
      stream << set.m_uiSet;
      const wdUInt8 uiBindings = set.bindings.GetCount();
      stream << uiBindings;
      for (wdUInt8 j = 0; j < uiBindings; j++)
      {
        const wdVulkanDescriptorSetLayoutBinding& binding = set.bindings[j];
        stream.WriteString(binding.m_sName).AssertSuccess();
        stream << binding.m_uiBinding;
        stream << binding.m_uiVirtualBinding;
        stream << static_cast<wdUInt8>(binding.m_wdType);
        stream << static_cast<wdUInt8>(binding.m_Type);
        stream << binding.m_uiDescriptorType;
        stream << binding.m_uiDescriptorCount;
        stream << binding.m_uiWordOffset;
      }
    }

    const wdUInt8 uiVIA = vertexInputAttributes.GetCount();
    stream << uiVIA;
    for (wdUInt8 i = 0; i < uiVIA; i++)
    {
      const wdVulkanVertexInputAttribute& via = vertexInputAttributes[i];
      stream << static_cast<wdUInt8>(via.m_eSemantic);
      stream << via.m_uiLocation;
      stream << static_cast<wdUInt8>(via.m_eFormat);
    }
  }

  /// \brief Reads Vulkan shader code and meta data from a data buffer. Note that 'data' must be kept alive for the lifetime of the shader as this functions stores views into this memory in its out parameters.
  /// \param data Raw data buffer to read the shader code and meta data from.
  /// \param out_shaderCode Will be filled with a view into data that contains the shader byte code.
  /// \param out_sets Will be filled with shader meta data. Note that this array contains string views into 'data'.
  void Read(const wdArrayPtr<const wdUInt8> data, wdArrayPtr<const wdUInt8>& out_shaderCode, wdDynamicArray<wdVulkanDescriptorSetLayout>& out_sets, wdDynamicArray<wdVulkanVertexInputAttribute>& out_vertexInputAttributes)
  {
    wdRawMemoryStreamReader stream(data.GetPtr(), data.GetCount());

    wdUInt32 uiMagicNumber;
    stream >> uiMagicNumber;
    WD_ASSERT_DEV(uiMagicNumber == s_uiSpirvMetaDataMagicNumber, "Vulkan shader does not start with s_uiSpirvMetaDataMagicNumber");
    wdTypeVersion uiVersion = stream.ReadVersion(MetaDataVersion::Version3);

    wdUInt32 uiSize = 0;
    stream >> uiSize;
    out_shaderCode = wdArrayPtr<const wdUInt8>(&data[(wdUInt32)stream.GetReadPosition()], uiSize);
    stream.SkipBytes(uiSize);

    wdUInt8 uiSets = 0;
    stream >> uiSets;
    out_sets.Reserve(uiSets);

    for (wdUInt8 i = 0; i < uiSets; i++)
    {
      wdVulkanDescriptorSetLayout& set = out_sets.ExpandAndGetRef();
      stream >> set.m_uiSet;
      wdUInt8 uiBindings = 0;
      stream >> uiBindings;
      set.bindings.Reserve(uiBindings);

      for (wdUInt8 j = 0; j < uiBindings; j++)
      {
        wdVulkanDescriptorSetLayoutBinding& binding = set.bindings.ExpandAndGetRef();

        wdUInt32 uiStringElements = 0;
        stream >> uiStringElements;
        binding.m_sName = wdStringView(reinterpret_cast<const char*>(&data[(wdUInt32)stream.GetReadPosition()]), uiStringElements);
        stream.SkipBytes(uiStringElements);
        stream >> binding.m_uiBinding;
        if (uiVersion >= MetaDataVersion::Version2)
        {
          stream >> binding.m_uiVirtualBinding;
          stream >> reinterpret_cast<wdUInt8&>(binding.m_wdType);
        }
        else
        {
          binding.m_uiVirtualBinding = binding.m_uiBinding;
          binding.m_wdType = wdShaderResourceType::Texture2D;
        }
        stream >> reinterpret_cast<wdUInt8&>(binding.m_Type);
        stream >> binding.m_uiDescriptorType;
        stream >> binding.m_uiDescriptorCount;
        stream >> binding.m_uiWordOffset;
      }
    }

    if (uiVersion >= MetaDataVersion::Version3)
    {
      wdUInt8 uiVIA = 0;
      stream >> uiVIA;
      out_vertexInputAttributes.Reserve(uiVIA);
      for (wdUInt8 i = 0; i < uiVIA; i++)
      {
        wdVulkanVertexInputAttribute& via = out_vertexInputAttributes.ExpandAndGetRef();
        stream >> reinterpret_cast<wdUInt8&>(via.m_eSemantic);
        stream >> via.m_uiLocation;
        stream >> reinterpret_cast<wdUInt8&>(via.m_eFormat);
      }
    }
  }
} // namespace wdSpirvMetaData
