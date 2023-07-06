
vk::ShaderModule wdGALShaderVulkan::GetShader(wdGALShaderStage::Enum stage) const
{
  return m_Shaders[stage];
}

const wdGALShaderVulkan::DescriptorSetLayoutDesc& wdGALShaderVulkan::GetDescriptorSetLayout() const
{
  return m_descriptorSetLayoutDesc;
}

const wdArrayPtr<const wdGALShaderVulkan::BindingMapping> wdGALShaderVulkan::GetBindingMapping() const
{
  return m_BindingMapping;
}

const wdArrayPtr<const wdGALShaderVulkan::VertexInputAttribute> wdGALShaderVulkan::GetVertexInputAttributes() const
{
  return m_VertexInputAttributes;
}
