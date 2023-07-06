
WD_ALWAYS_INLINE const vk::PipelineColorBlendStateCreateInfo* wdGALBlendStateVulkan::GetBlendState() const
{
  return &m_blendState;
}

WD_ALWAYS_INLINE const vk::PipelineDepthStencilStateCreateInfo* wdGALDepthStencilStateVulkan::GetDepthStencilState() const
{
  return &m_depthStencilState;
}

WD_ALWAYS_INLINE const vk::PipelineRasterizationStateCreateInfo* wdGALRasterizerStateVulkan::GetRasterizerState() const
{
  return &m_rasterizerState;
}

WD_ALWAYS_INLINE const vk::DescriptorImageInfo& wdGALSamplerStateVulkan::GetImageInfo() const
{
  return m_resourceImageInfo;
}
