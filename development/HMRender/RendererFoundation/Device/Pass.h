
#pragma once

#include <RendererFoundation/Resources/RenderTargetSetup.h>

class WD_RENDERERFOUNDATION_DLL wdGALPass
{
  WD_DISALLOW_COPY_AND_ASSIGN(wdGALPass);

public:
  wdGALRenderCommandEncoder* BeginRendering(const wdGALRenderingSetup& renderingSetup, const char* szName = "");
  void EndRendering(wdGALRenderCommandEncoder* pCommandEncoder);

  wdGALComputeCommandEncoder* BeginCompute(const char* szName = "");
  void EndCompute(wdGALComputeCommandEncoder* pCommandEncoder);

  // BeginRaytracing() could be here as well (would match Vulkan)

protected:
  virtual wdGALRenderCommandEncoder* BeginRenderingPlatform(const wdGALRenderingSetup& renderingSetup, const char* szName) = 0;
  virtual void EndRenderingPlatform(wdGALRenderCommandEncoder* pCommandEncoder) = 0;

  virtual wdGALComputeCommandEncoder* BeginComputePlatform(const char* szName) = 0;
  virtual void EndComputePlatform(wdGALComputeCommandEncoder* pCommandEncoder) = 0;

  wdGALPass(wdGALDevice& device);
  virtual ~wdGALPass();

  wdGALDevice& m_Device;

  enum class CommandEncoderType
  {
    Invalid,
    Render,
    Compute
  };

  CommandEncoderType m_CurrentCommandEncoderType = CommandEncoderType::Invalid;
  bool m_bMarker = false;
};
