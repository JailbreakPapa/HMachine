#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>

wdGALComputeCommandEncoder::wdGALComputeCommandEncoder(wdGALDevice& ref_device, wdGALCommandEncoderState& ref_state, wdGALCommandEncoderCommonPlatformInterface& ref_commonImpl, wdGALCommandEncoderComputePlatformInterface& ref_computeImpl)
  : wdGALCommandEncoder(ref_device, ref_state, ref_commonImpl)
  , m_ComputeImpl(ref_computeImpl)
{
}

wdGALComputeCommandEncoder::~wdGALComputeCommandEncoder() = default;

void wdGALComputeCommandEncoder::Dispatch(wdUInt32 uiThreadGroupCountX, wdUInt32 uiThreadGroupCountY, wdUInt32 uiThreadGroupCountZ)
{
  AssertRenderingThread();

  WD_ASSERT_DEBUG(uiThreadGroupCountX > 0 && uiThreadGroupCountY > 0 && uiThreadGroupCountZ > 0, "Thread group counts of zero are not meaningful. Did you mean 1?");

  /// \todo Assert for compute

  m_ComputeImpl.DispatchPlatform(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);

  CountDispatchCall();
}

void wdGALComputeCommandEncoder::DispatchIndirect(wdGALBufferHandle hIndirectArgumentBuffer, wdUInt32 uiArgumentOffsetInBytes)
{
  AssertRenderingThread();
  /// \todo Assert for compute
  /// \todo Assert for indirect dispatch
  /// \todo Assert offset < buffer size

  const wdGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  WD_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");

  /// \todo Assert that the buffer can be used for indirect arguments (flag in desc)
  m_ComputeImpl.DispatchIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);

  CountDispatchCall();
}

void wdGALComputeCommandEncoder::ClearStatisticsCounters()
{
  wdGALCommandEncoder::ClearStatisticsCounters();

  m_uiDispatchCalls = 0;
}


WD_STATICLINK_FILE(RendererFoundation, RendererFoundation_CommandEncoder_Implementation_ComputeCommandEncoder);
