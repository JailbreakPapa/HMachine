
#pragma once

#include <Foundation/Math/Rect.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>

class WD_RENDERERFOUNDATION_DLL wdGALRenderCommandEncoder : public wdGALCommandEncoder
{
public:
  wdGALRenderCommandEncoder(wdGALDevice& ref_device, wdGALCommandEncoderRenderState& ref_renderState, wdGALCommandEncoderCommonPlatformInterface& ref_commonImpl, wdGALCommandEncoderRenderPlatformInterface& ref_renderImpl);
  virtual ~wdGALRenderCommandEncoder();

  // Draw functions

  /// \brief Clears active rendertargets.
  ///
  /// \param uiRenderTargetClearMask
  ///   Each bit represents a bound color target. If all bits are set, all bound color targets will be cleared.
  void Clear(const wdColor& clearColor, wdUInt32 uiRenderTargetClearMask = 0xFFFFFFFFu, bool bClearDepth = true, bool bClearStencil = true, float fDepthClear = 1.0f, wdUInt8 uiStencilClear = 0x0u);

  void Draw(wdUInt32 uiVertexCount, wdUInt32 uiStartVertex);
  void DrawIndexed(wdUInt32 uiIndexCount, wdUInt32 uiStartIndex);
  void DrawIndexedInstanced(wdUInt32 uiIndexCountPerInstance, wdUInt32 uiInstanceCount, wdUInt32 uiStartIndex);
  void DrawIndexedInstancedIndirect(wdGALBufferHandle hIndirectArgumentBuffer, wdUInt32 uiArgumentOffsetInBytes);
  void DrawInstanced(wdUInt32 uiVertexCountPerInstance, wdUInt32 uiInstanceCount, wdUInt32 uiStartVertex);
  void DrawInstancedIndirect(wdGALBufferHandle hIndirectArgumentBuffer, wdUInt32 uiArgumentOffsetInBytes);
  void DrawAuto();

  void BeginStreamOut();
  void EndStreamOut();

  // State functions

  void SetIndexBuffer(wdGALBufferHandle hIndexBuffer);
  void SetVertexBuffer(wdUInt32 uiSlot, wdGALBufferHandle hVertexBuffer);
  void SetVertexDeclaration(wdGALVertexDeclarationHandle hVertexDeclaration);

  wdGALPrimitiveTopology::Enum GetPrimitiveTopology() const { return m_RenderState.m_Topology; }
  void SetPrimitiveTopology(wdGALPrimitiveTopology::Enum topology);

  void SetBlendState(wdGALBlendStateHandle hBlendState, const wdColor& blendFactor = wdColor::White, wdUInt32 uiSampleMask = 0xFFFFFFFFu);
  void SetDepthStencilState(wdGALDepthStencilStateHandle hDepthStencilState, wdUInt8 uiStencilRefValue = 0xFFu);
  void SetRasterizerState(wdGALRasterizerStateHandle hRasterizerState);

  void SetViewport(const wdRectFloat& rect, float fMinDepth = 0.0f, float fMaxDepth = 1.0f);
  void SetScissorRect(const wdRectU32& rect);

  void SetStreamOutBuffer(wdUInt32 uiSlot, wdGALBufferHandle hBuffer, wdUInt32 uiOffset);

  virtual void ClearStatisticsCounters() override;

private:
  void CountDrawCall() { m_uiDrawCalls++; }

  // Statistic variables
  wdUInt32 m_uiDrawCalls = 0;

  wdGALCommandEncoderRenderState& m_RenderState;

  wdGALCommandEncoderRenderPlatformInterface& m_RenderImpl;
};
