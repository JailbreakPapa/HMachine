
#pragma once

#include <RendererFoundation/CommandEncoder/CommandEncoder.h>

class WD_RENDERERFOUNDATION_DLL wdGALComputeCommandEncoder : public wdGALCommandEncoder
{
public:
  wdGALComputeCommandEncoder(wdGALDevice& ref_device, wdGALCommandEncoderState& ref_state, wdGALCommandEncoderCommonPlatformInterface& ref_commonImpl, wdGALCommandEncoderComputePlatformInterface& ref_computeImpl);
  virtual ~wdGALComputeCommandEncoder();

  // Dispatch

  void Dispatch(wdUInt32 uiThreadGroupCountX, wdUInt32 uiThreadGroupCountY, wdUInt32 uiThreadGroupCountZ);
  void DispatchIndirect(wdGALBufferHandle hIndirectArgumentBuffer, wdUInt32 uiArgumentOffsetInBytes);

  virtual void ClearStatisticsCounters() override;

private:
  void CountDispatchCall() { m_uiDispatchCalls++; }

  // Statistic variables
  wdUInt32 m_uiDispatchCalls = 0;

  wdGALCommandEncoderComputePlatformInterface& m_ComputeImpl;
};
