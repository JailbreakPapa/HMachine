
#pragma once

#include <RendererFoundation/Resources/RenderTargetView.h>

struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11UnorderedAccessView;

class wdGALRenderTargetViewVulkan : public wdGALRenderTargetView
{
public:
  vk::ImageView GetImageView() const;
  bool IsFullRange() const;
  vk::ImageSubresourceRange GetRange() const;

protected:
  friend class wdGALDeviceVulkan;
  friend class wdMemoryUtils;

  wdGALRenderTargetViewVulkan(wdGALTexture* pTexture, const wdGALRenderTargetViewCreationDescription& Description);
  virtual ~wdGALRenderTargetViewVulkan();

  virtual wdResult InitPlatform(wdGALDevice* pDevice) override;
  virtual wdResult DeInitPlatform(wdGALDevice* pDevice) override;

  vk::ImageView m_imageView;
  bool m_bfullRange = false;
  vk::ImageSubresourceRange m_range;
};

#include <RendererVulkan/Resources/Implementation/RenderTargetViewVulkan_inl.h>
