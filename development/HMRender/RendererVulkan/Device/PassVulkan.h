
#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/Device/Pass.h>

struct wdGALCommandEncoderRenderState;
class wdGALRenderCommandEncoder;
class wdGALComputeCommandEncoder;
class wdGALCommandEncoderImplVulkan;

class wdGALPassVulkan : public wdGALPass
{
protected:
  friend class wdGALDeviceVulkan;
  friend class wdMemoryUtils;
  void Reset();
  void MarkDirty();
  void SetCurrentCommandBuffer(vk::CommandBuffer* commandBuffer, wdPipelineBarrierVulkan* pipelineBarrier);

  virtual wdGALRenderCommandEncoder* BeginRenderingPlatform(const wdGALRenderingSetup& renderingSetup, const char* szName) override;
  virtual void EndRenderingPlatform(wdGALRenderCommandEncoder* pCommandEncoder) override;

  virtual wdGALComputeCommandEncoder* BeginComputePlatform(const char* szName) override;
  virtual void EndComputePlatform(wdGALComputeCommandEncoder* pCommandEncoder) override;

  wdGALPassVulkan(wdGALDevice& device);
  virtual ~wdGALPassVulkan();

  void BeginPass(const char* szName);
  void EndPass();

private:
  wdUniquePtr<wdGALCommandEncoderRenderState> m_pCommandEncoderState;
  wdUniquePtr<wdGALCommandEncoderImplVulkan> m_pCommandEncoderImpl;

  wdUniquePtr<wdGALRenderCommandEncoder> m_pRenderCommandEncoder;
  wdUniquePtr<wdGALComputeCommandEncoder> m_pComputeCommandEncoder;
};
