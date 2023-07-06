#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/CommandEncoder/CommandEncoderImplDX11.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Device/PassDX11.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>
#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>

wdGALPassDX11::wdGALPassDX11(wdGALDevice& device)
  : wdGALPass(device)
{
  m_pCommandEncoderState = WD_DEFAULT_NEW(wdGALCommandEncoderRenderState);
  m_pCommandEncoderImpl = WD_DEFAULT_NEW(wdGALCommandEncoderImplDX11, static_cast<wdGALDeviceDX11&>(device));

  m_pRenderCommandEncoder = WD_DEFAULT_NEW(wdGALRenderCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);
  m_pComputeCommandEncoder = WD_DEFAULT_NEW(wdGALComputeCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);

  m_pCommandEncoderImpl->m_pOwner = m_pRenderCommandEncoder.Borrow();
}

wdGALPassDX11::~wdGALPassDX11() = default;

wdGALRenderCommandEncoder* wdGALPassDX11::BeginRenderingPlatform(const wdGALRenderingSetup& renderingSetup, const char* szName)
{
  m_pCommandEncoderImpl->BeginRendering(renderingSetup);

  return m_pRenderCommandEncoder.Borrow();
}

void wdGALPassDX11::EndRenderingPlatform(wdGALRenderCommandEncoder* pCommandEncoder)
{
  WD_ASSERT_DEV(m_pRenderCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");
}

wdGALComputeCommandEncoder* wdGALPassDX11::BeginComputePlatform(const char* szName)
{
  m_pCommandEncoderImpl->BeginCompute();
  return m_pComputeCommandEncoder.Borrow();
}

void wdGALPassDX11::EndComputePlatform(wdGALComputeCommandEncoder* pCommandEncoder)
{
  WD_ASSERT_DEV(m_pComputeCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");
}

void wdGALPassDX11::BeginPass(const char* szName)
{
}

void wdGALPassDX11::EndPass()
{
}
