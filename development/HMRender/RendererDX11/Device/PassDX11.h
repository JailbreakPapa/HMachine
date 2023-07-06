
#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/Device/Pass.h>

struct wdGALCommandEncoderRenderState;
class wdGALRenderCommandEncoder;
class wdGALComputeCommandEncoder;

class wdGALCommandEncoderImplDX11;

class wdGALPassDX11 : public wdGALPass
{
protected:
  friend class wdGALDeviceDX11;
  friend class wdMemoryUtils;

  wdGALPassDX11(wdGALDevice& device);
  virtual ~wdGALPassDX11();

  virtual wdGALRenderCommandEncoder* BeginRenderingPlatform(const wdGALRenderingSetup& renderingSetup, const char* szName) override;
  virtual void EndRenderingPlatform(wdGALRenderCommandEncoder* pCommandEncoder) override;

  virtual wdGALComputeCommandEncoder* BeginComputePlatform(const char* szName) override;
  virtual void EndComputePlatform(wdGALComputeCommandEncoder* pCommandEncoder) override;

  void BeginPass(const char* szName);
  void EndPass();

private:
  wdUniquePtr<wdGALCommandEncoderRenderState> m_pCommandEncoderState;
  wdUniquePtr<wdGALCommandEncoderImplDX11> m_pCommandEncoderImpl;

  wdUniquePtr<wdGALRenderCommandEncoder> m_pRenderCommandEncoder;
  wdUniquePtr<wdGALComputeCommandEncoder> m_pComputeCommandEncoder;
};
