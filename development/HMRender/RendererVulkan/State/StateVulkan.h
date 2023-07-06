
#pragma once

#include <RendererFoundation/State/State.h>

#include <vulkan/vulkan.hpp>

class WD_RENDERERVULKAN_DLL wdGALBlendStateVulkan : public wdGALBlendState
{
public:
  WD_ALWAYS_INLINE const vk::PipelineColorBlendStateCreateInfo* GetBlendState() const;

protected:
  friend class wdGALDeviceVulkan;
  friend class wdMemoryUtils;

  wdGALBlendStateVulkan(const wdGALBlendStateCreationDescription& Description);

  ~wdGALBlendStateVulkan();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) override;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

  vk::PipelineColorBlendStateCreateInfo m_blendState = {};
  vk::PipelineColorBlendAttachmentState m_blendAttachmentState[8] = {};
};

class WD_RENDERERVULKAN_DLL wdGALDepthStencilStateVulkan : public wdGALDepthStencilState
{
public:
  WD_ALWAYS_INLINE const vk::PipelineDepthStencilStateCreateInfo* GetDepthStencilState() const;

protected:
  friend class wdGALDeviceVulkan;
  friend class wdMemoryUtils;

  wdGALDepthStencilStateVulkan(const wdGALDepthStencilStateCreationDescription& Description);

  ~wdGALDepthStencilStateVulkan();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) override;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

  vk::PipelineDepthStencilStateCreateInfo m_depthStencilState = {};
};

class WD_RENDERERVULKAN_DLL wdGALRasterizerStateVulkan : public wdGALRasterizerState
{
public:
  WD_ALWAYS_INLINE const vk::PipelineRasterizationStateCreateInfo* GetRasterizerState() const;

protected:
  friend class wdGALDeviceVulkan;
  friend class wdMemoryUtils;

  wdGALRasterizerStateVulkan(const wdGALRasterizerStateCreationDescription& Description);

  ~wdGALRasterizerStateVulkan();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) override;

  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

  vk::PipelineRasterizationStateCreateInfo m_rasterizerState = {};
};

class WD_RENDERERVULKAN_DLL wdGALSamplerStateVulkan : public wdGALSamplerState
{
public:
  WD_ALWAYS_INLINE const vk::DescriptorImageInfo& GetImageInfo() const;

protected:
  friend class wdGALDeviceVulkan;
  friend class wdMemoryUtils;

  wdGALSamplerStateVulkan(const wdGALSamplerStateCreationDescription& Description);
  ~wdGALSamplerStateVulkan();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) override;
  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

  vk::DescriptorImageInfo m_resourceImageInfo;
};


#include <RendererVulkan/State/Implementation/StateVulkan_inl.h>
