

wdArrayPtr<const vk::VertexInputAttributeDescription> wdGALVertexDeclarationVulkan::GetAttributes() const
{
  return m_attributes.GetArrayPtr();
}

wdArrayPtr<const vk::VertexInputBindingDescription> wdGALVertexDeclarationVulkan::GetBindings() const
{
  return m_bindings.GetArrayPtr();
}
