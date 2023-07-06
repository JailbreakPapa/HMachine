#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererFoundation/Shader/Shader.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>

wdGALVertexDeclarationVulkan::wdGALVertexDeclarationVulkan(const wdGALVertexDeclarationCreationDescription& Description)
  : wdGALVertexDeclaration(Description)
{
}

wdGALVertexDeclarationVulkan::~wdGALVertexDeclarationVulkan() = default;

wdResult wdGALVertexDeclarationVulkan::InitPlatform(wdGALDevice* pDevice)
{
  wdGALDeviceVulkan* pVulkanDevice = static_cast<wdGALDeviceVulkan*>(pDevice);

  const wdGALShaderVulkan* pShader = static_cast<const wdGALShaderVulkan*>(pDevice->GetShader(m_Description.m_hShader));

  if (pShader == nullptr || !pShader->GetDescription().HasByteCodeForStage(wdGALShaderStage::VertexShader))
  {
    return WD_FAILURE;
  }

  wdHybridArray<wdGALShaderVulkan::VertexInputAttribute, 8> vias(pShader->GetVertexInputAttributes());
  auto FindLocation = [&](wdGALVertexAttributeSemantic::Enum sematic, wdGALResourceFormat::Enum format) -> wdUInt32 {
    for (wdUInt32 i = 0; i < vias.GetCount(); i++)
    {
      if (vias[i].m_eSemantic == sematic)
      {
        //WD_ASSERT_DEBUG(vias[i].m_eFormat == format, "Found matching sematic {} but format differs: {} : {}", sematic, format, vias[i].m_eFormat);
        wdUInt32 uiLocation = vias[i].m_uiLocation;
        vias.RemoveAtAndSwap(i);
        return uiLocation;
      }
    }
    return wdMath::MaxValue<wdUInt32>();
  };

  // Copy attribute descriptions
  wdUInt32 usedBindings = 0;
  for (wdUInt32 i = 0; i < m_Description.m_VertexAttributes.GetCount(); i++)
  {
    const wdGALVertexAttribute& Current = m_Description.m_VertexAttributes[i];

    const wdUInt32 uiLocation = FindLocation(Current.m_eSemantic, Current.m_eFormat);
    if (uiLocation == wdMath::MaxValue<wdUInt32>())
    {
      wdLog::Warning("Vertex buffer semantic {} not used by shader", Current.m_eSemantic);
      continue;
    }
    vk::VertexInputAttributeDescription& attrib = m_attributes.ExpandAndGetRef();
    attrib.binding = Current.m_uiVertexBufferSlot;
    attrib.location = uiLocation;
    attrib.format = pVulkanDevice->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_eVertexAttributeType;
    attrib.offset = Current.m_uiOffset;

    if (attrib.format == vk::Format::eUndefined)
    {
      wdLog::Error("Vertex attribute format {0} of attribute at index {1} is undefined!", Current.m_eFormat, i);
      return WD_FAILURE;
    }

    usedBindings |= WD_BIT(Current.m_uiVertexBufferSlot);
    if (Current.m_uiVertexBufferSlot >= m_bindings.GetCount())
    {
      m_bindings.SetCount(Current.m_uiVertexBufferSlot + 1);
    }
    vk::VertexInputBindingDescription& binding = m_bindings[Current.m_uiVertexBufferSlot];
    binding.binding = Current.m_uiVertexBufferSlot;
    binding.stride = 0;
    binding.inputRate = Current.m_bInstanceData ? vk::VertexInputRate::eInstance : vk::VertexInputRate::eVertex;
  }
  for (wdInt32 i = (wdInt32)m_bindings.GetCount() - 1; i >= 0; --i)
  {
    if ((usedBindings & WD_BIT(i)) == 0)
    {
      m_bindings.RemoveAtAndCopy(i);
    }
  }

  if (!vias.IsEmpty())
  {
    wdLog::Error("Vertex buffers do not cover all vertex attributes defined in the shader!");
    return WD_FAILURE;
  }
  return WD_SUCCESS;
}

wdResult wdGALVertexDeclarationVulkan::DeInitPlatform(wdGALDevice* pDevice)
{
  return WD_SUCCESS;
}



WD_STATICLINK_FILE(RendererVulkan, RendererVulkan_Shader_Implementation_VertexDeclarationVulkan);
