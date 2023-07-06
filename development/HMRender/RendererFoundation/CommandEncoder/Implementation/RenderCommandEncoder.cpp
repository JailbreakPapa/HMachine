#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Query.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/ResourceView.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Resources/UnorderedAccesView.h>

wdGALRenderCommandEncoder::wdGALRenderCommandEncoder(wdGALDevice& ref_device, wdGALCommandEncoderRenderState& ref_renderState, wdGALCommandEncoderCommonPlatformInterface& ref_commonImpl, wdGALCommandEncoderRenderPlatformInterface& ref_renderImpl)
  : wdGALCommandEncoder(ref_device, ref_renderState, ref_commonImpl)
  , m_RenderState(ref_renderState)
  , m_RenderImpl(ref_renderImpl)
{
}

wdGALRenderCommandEncoder::~wdGALRenderCommandEncoder() = default;

void wdGALRenderCommandEncoder::Clear(const wdColor& clearColor, wdUInt32 uiRenderTargetClearMask /*= 0xFFFFFFFFu*/, bool bClearDepth /*= true*/, bool bClearStencil /*= true*/, float fDepthClear /*= 1.0f*/, wdUInt8 uiStencilClear /*= 0x0u*/)
{
  AssertRenderingThread();

  m_RenderImpl.ClearPlatform(clearColor, uiRenderTargetClearMask, bClearDepth, bClearStencil, fDepthClear, uiStencilClear);
}

void wdGALRenderCommandEncoder::Draw(wdUInt32 uiVertexCount, wdUInt32 uiStartVertex)
{
  AssertRenderingThread();

  /// \todo If platform indicates that non-indexed rendering is not possible bind a helper index buffer which contains continuous indices
  /// (0, 1, 2, ..)

  m_RenderImpl.DrawPlatform(uiVertexCount, uiStartVertex);

  CountDrawCall();
}

void wdGALRenderCommandEncoder::DrawIndexed(wdUInt32 uiIndexCount, wdUInt32 uiStartIndex)
{
  AssertRenderingThread();

  m_RenderImpl.DrawIndexedPlatform(uiIndexCount, uiStartIndex);

  CountDrawCall();
}

void wdGALRenderCommandEncoder::DrawIndexedInstanced(wdUInt32 uiIndexCountPerInstance, wdUInt32 uiInstanceCount, wdUInt32 uiStartIndex)
{
  AssertRenderingThread();
  /// \todo Assert for instancing

  m_RenderImpl.DrawIndexedInstancedPlatform(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex);

  CountDrawCall();
}

void wdGALRenderCommandEncoder::DrawIndexedInstancedIndirect(wdGALBufferHandle hIndirectArgumentBuffer, wdUInt32 uiArgumentOffsetInBytes)
{
  AssertRenderingThread();
  /// \todo Assert for instancing
  /// \todo Assert for indirect draw
  /// \todo Assert offset < buffer size

  const wdGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  WD_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");

  /// \todo Assert that the buffer can be used for indirect arguments (flag in desc)
  m_RenderImpl.DrawIndexedInstancedIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);

  CountDrawCall();
}

void wdGALRenderCommandEncoder::DrawInstanced(wdUInt32 uiVertexCountPerInstance, wdUInt32 uiInstanceCount, wdUInt32 uiStartVertex)
{
  AssertRenderingThread();
  /// \todo Assert for instancing

  /// \todo If platform indicates that non-indexed rendering is not possible bind a helper index buffer which contains continuous indices
  /// (0, 1, 2, ..)

  m_RenderImpl.DrawInstancedPlatform(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex);

  CountDrawCall();
}

void wdGALRenderCommandEncoder::DrawInstancedIndirect(wdGALBufferHandle hIndirectArgumentBuffer, wdUInt32 uiArgumentOffsetInBytes)
{
  AssertRenderingThread();
  /// \todo Assert for instancing
  /// \todo Assert for indirect draw
  /// \todo Assert offset < buffer size

  const wdGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  WD_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");

  /// \todo Assert that the buffer can be used for indirect arguments (flag in desc)
  m_RenderImpl.DrawInstancedIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);

  CountDrawCall();
}

void wdGALRenderCommandEncoder::DrawAuto()
{
  AssertRenderingThread();
  /// \todo Assert for draw auto support

  m_RenderImpl.DrawAutoPlatform();

  CountDrawCall();
}

void wdGALRenderCommandEncoder::BeginStreamOut()
{
  AssertRenderingThread();
  /// \todo Assert for streamout support

  m_RenderImpl.BeginStreamOutPlatform();
}

void wdGALRenderCommandEncoder::EndStreamOut()
{
  AssertRenderingThread();

  m_RenderImpl.EndStreamOutPlatform();
}

void wdGALRenderCommandEncoder::SetIndexBuffer(wdGALBufferHandle hIndexBuffer)
{
  if (m_RenderState.m_hIndexBuffer == hIndexBuffer)
  {
    CountRedundantStateChange();
    return;
  }

  const wdGALBuffer* pBuffer = GetDevice().GetBuffer(hIndexBuffer);
  /// \todo Assert on index buffer type (if non nullptr)
  // Note that GL4 can bind arbitrary buffer to arbitrary binding points (index/vertex/transform-feedback/indirect-draw/...)

  m_RenderImpl.SetIndexBufferPlatform(pBuffer);

  m_RenderState.m_hIndexBuffer = hIndexBuffer;
  CountStateChange();
}

void wdGALRenderCommandEncoder::SetVertexBuffer(wdUInt32 uiSlot, wdGALBufferHandle hVertexBuffer)
{
  if (m_RenderState.m_hVertexBuffers[uiSlot] == hVertexBuffer)
  {
    CountRedundantStateChange();
    return;
  }

  const wdGALBuffer* pBuffer = GetDevice().GetBuffer(hVertexBuffer);
  // Assert on vertex buffer type (if non-zero)
  // Note that GL4 can bind arbitrary buffer to arbitrary binding points (index/vertex/transform-feedback/indirect-draw/...)

  m_RenderImpl.SetVertexBufferPlatform(uiSlot, pBuffer);

  m_RenderState.m_hVertexBuffers[uiSlot] = hVertexBuffer;
  CountStateChange();
}

void wdGALRenderCommandEncoder::SetPrimitiveTopology(wdGALPrimitiveTopology::Enum topology)
{
  AssertRenderingThread();

  if (m_RenderState.m_Topology == topology)
  {
    CountRedundantStateChange();
    return;
  }

  m_RenderImpl.SetPrimitiveTopologyPlatform(topology);

  m_RenderState.m_Topology = topology;

  CountStateChange();
}

void wdGALRenderCommandEncoder::SetVertexDeclaration(wdGALVertexDeclarationHandle hVertexDeclaration)
{
  AssertRenderingThread();

  if (m_RenderState.m_hVertexDeclaration == hVertexDeclaration)
  {
    CountRedundantStateChange();
    return;
  }

  const wdGALVertexDeclaration* pVertexDeclaration = GetDevice().GetVertexDeclaration(hVertexDeclaration);
  // Assert on vertex buffer type (if non-zero)

  m_RenderImpl.SetVertexDeclarationPlatform(pVertexDeclaration);

  m_RenderState.m_hVertexDeclaration = hVertexDeclaration;

  CountStateChange();
}

void wdGALRenderCommandEncoder::SetBlendState(wdGALBlendStateHandle hBlendState, const wdColor& blendFactor, wdUInt32 uiSampleMask)
{
  AssertRenderingThread();

  if (m_RenderState.m_hBlendState == hBlendState && m_RenderState.m_BlendFactor.IsEqualRGBA(blendFactor, 0.001f) && m_RenderState.m_uiSampleMask == uiSampleMask)
  {
    CountRedundantStateChange();
    return;
  }

  const wdGALBlendState* pBlendState = GetDevice().GetBlendState(hBlendState);

  m_RenderImpl.SetBlendStatePlatform(pBlendState, blendFactor, uiSampleMask);

  m_RenderState.m_hBlendState = hBlendState;
  m_RenderState.m_BlendFactor = blendFactor;
  m_RenderState.m_uiSampleMask = uiSampleMask;

  CountStateChange();
}

void wdGALRenderCommandEncoder::SetDepthStencilState(wdGALDepthStencilStateHandle hDepthStencilState, wdUInt8 uiStencilRefValue /*= 0xFFu*/)
{
  AssertRenderingThread();

  if (m_RenderState.m_hDepthStencilState == hDepthStencilState && m_RenderState.m_uiStencilRefValue == uiStencilRefValue)
  {
    CountRedundantStateChange();
    return;
  }

  const wdGALDepthStencilState* pDepthStencilState = GetDevice().GetDepthStencilState(hDepthStencilState);

  m_RenderImpl.SetDepthStencilStatePlatform(pDepthStencilState, uiStencilRefValue);

  m_RenderState.m_hDepthStencilState = hDepthStencilState;
  m_RenderState.m_uiStencilRefValue = uiStencilRefValue;

  CountStateChange();
}

void wdGALRenderCommandEncoder::SetRasterizerState(wdGALRasterizerStateHandle hRasterizerState)
{
  AssertRenderingThread();

  if (m_RenderState.m_hRasterizerState == hRasterizerState)
  {
    CountRedundantStateChange();
    return;
  }

  const wdGALRasterizerState* pRasterizerState = GetDevice().GetRasterizerState(hRasterizerState);

  m_RenderImpl.SetRasterizerStatePlatform(pRasterizerState);

  m_RenderState.m_hRasterizerState = hRasterizerState;

  CountStateChange();
}

void wdGALRenderCommandEncoder::SetViewport(const wdRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  AssertRenderingThread();

  if (m_RenderState.m_ViewPortRect == rect && m_RenderState.m_fViewPortMinDepth == fMinDepth && m_RenderState.m_fViewPortMaxDepth == fMaxDepth)
  {
    CountRedundantStateChange();
    return;
  }

  m_RenderImpl.SetViewportPlatform(rect, fMinDepth, fMaxDepth);

  m_RenderState.m_ViewPortRect = rect;
  m_RenderState.m_fViewPortMinDepth = fMinDepth;
  m_RenderState.m_fViewPortMaxDepth = fMaxDepth;

  CountStateChange();
}

void wdGALRenderCommandEncoder::SetScissorRect(const wdRectU32& rect)
{
  AssertRenderingThread();

  if (m_RenderState.m_ScissorRect == rect)
  {
    CountRedundantStateChange();
    return;
  }

  m_RenderImpl.SetScissorRectPlatform(rect);

  m_RenderState.m_ScissorRect = rect;

  CountStateChange();
}

void wdGALRenderCommandEncoder::SetStreamOutBuffer(wdUInt32 uiSlot, wdGALBufferHandle hBuffer, wdUInt32 uiOffset)
{
  WD_ASSERT_NOT_IMPLEMENTED;

  CountStateChange();
}

void wdGALRenderCommandEncoder::ClearStatisticsCounters()
{
  wdGALCommandEncoder::ClearStatisticsCounters();

  m_uiDrawCalls = 0;
}


WD_STATICLINK_FILE(RendererFoundation, RendererFoundation_CommandEncoder_Implementation_RenderCommandEncoder);
