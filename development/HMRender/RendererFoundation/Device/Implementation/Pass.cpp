#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/Pass.h>

wdGALRenderCommandEncoder* wdGALPass::BeginRendering(const wdGALRenderingSetup& renderingSetup, const char* szName /*= ""*/)
{
  WD_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Invalid, "Nested Command Encoder are not allowed");
  m_CurrentCommandEncoderType = CommandEncoderType::Render;

  wdGALRenderCommandEncoder* pCommandEncoder = BeginRenderingPlatform(renderingSetup, szName);

  m_bMarker = !wdStringUtils::IsNullOrEmpty(szName);
  if (m_bMarker)
  {
    pCommandEncoder->PushMarker(szName);
  }

  return pCommandEncoder;
}

void wdGALPass::EndRendering(wdGALRenderCommandEncoder* pCommandEncoder)
{
  WD_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Render, "BeginRendering has not been called");
  m_CurrentCommandEncoderType = CommandEncoderType::Invalid;

  if (m_bMarker)
  {
    pCommandEncoder->PopMarker();
    m_bMarker = false;
  }

  EndRenderingPlatform(pCommandEncoder);
}

wdGALComputeCommandEncoder* wdGALPass::BeginCompute(const char* szName /*= ""*/)
{
  WD_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Invalid, "Nested Command Encoder are not allowed");
  m_CurrentCommandEncoderType = CommandEncoderType::Compute;

  wdGALComputeCommandEncoder* pCommandEncoder = BeginComputePlatform(szName);

  m_bMarker = !wdStringUtils::IsNullOrEmpty(szName);
  if (m_bMarker)
  {
    pCommandEncoder->PushMarker(szName);
  }

  return pCommandEncoder;
}

void wdGALPass::EndCompute(wdGALComputeCommandEncoder* pCommandEncoder)
{
  WD_ASSERT_DEV(m_CurrentCommandEncoderType == CommandEncoderType::Compute, "BeginCompute has not been called");
  m_CurrentCommandEncoderType = CommandEncoderType::Invalid;

  if (m_bMarker)
  {
    pCommandEncoder->PopMarker();
    m_bMarker = false;
  }

  EndComputePlatform(pCommandEncoder);
}

wdGALPass::wdGALPass(wdGALDevice& device)
  : m_Device(device)
{
}

wdGALPass::~wdGALPass() = default;


WD_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_Pass);
