#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>
#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererVulkan/CommandEncoder/CommandEncoderImplVulkan.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/PassVulkan.h>

wdGALPassVulkan::wdGALPassVulkan(wdGALDevice& device)
  : wdGALPass(device)
{
  m_pCommandEncoderState = WD_DEFAULT_NEW(wdGALCommandEncoderRenderState);
  m_pCommandEncoderImpl = WD_DEFAULT_NEW(wdGALCommandEncoderImplVulkan, static_cast<wdGALDeviceVulkan&>(device));

  m_pRenderCommandEncoder = WD_DEFAULT_NEW(wdGALRenderCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);
  m_pComputeCommandEncoder = WD_DEFAULT_NEW(wdGALComputeCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);
}

wdGALPassVulkan::~wdGALPassVulkan() = default;

void wdGALPassVulkan::Reset()
{
  m_pCommandEncoderImpl->Reset();
  m_pRenderCommandEncoder->InvalidateState();
  m_pComputeCommandEncoder->InvalidateState();
}

void wdGALPassVulkan::MarkDirty()
{
  m_pCommandEncoderImpl->MarkDirty();
}

void wdGALPassVulkan::SetCurrentCommandBuffer(vk::CommandBuffer* commandBuffer, wdPipelineBarrierVulkan* pipelineBarrier)
{
  m_pCommandEncoderImpl->SetCurrentCommandBuffer(commandBuffer, pipelineBarrier);
}

wdGALRenderCommandEncoder* wdGALPassVulkan::BeginRenderingPlatform(const wdGALRenderingSetup& renderingSetup, const char* szName)
{
  auto& deviceVulkan = static_cast<wdGALDeviceVulkan&>(m_Device);
  deviceVulkan.GetCurrentCommandBuffer();

  m_pCommandEncoderImpl->BeginRendering(renderingSetup);

  return m_pRenderCommandEncoder.Borrow();
}

void wdGALPassVulkan::EndRenderingPlatform(wdGALRenderCommandEncoder* pCommandEncoder)
{
  WD_ASSERT_DEV(m_pRenderCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");

  m_pCommandEncoderImpl->EndRendering();
}

wdGALComputeCommandEncoder* wdGALPassVulkan::BeginComputePlatform(const char* szName)
{
  auto& deviceVulkan = static_cast<wdGALDeviceVulkan&>(m_Device);
  deviceVulkan.GetCurrentCommandBuffer();

  m_pCommandEncoderImpl->BeginCompute();

  return m_pComputeCommandEncoder.Borrow();
}

void wdGALPassVulkan::EndComputePlatform(wdGALComputeCommandEncoder* pCommandEncoder)
{
  WD_ASSERT_DEV(m_pComputeCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");

  m_pCommandEncoderImpl->EndCompute();
}
