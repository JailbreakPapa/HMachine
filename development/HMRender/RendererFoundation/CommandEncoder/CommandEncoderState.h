
#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Rect.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct WD_RENDERERFOUNDATION_DLL wdGALCommandEncoderState
{
  virtual void InvalidateState();

  wdGALShaderHandle m_hShader;

  wdGALBufferHandle m_hConstantBuffers[WD_GAL_MAX_CONSTANT_BUFFER_COUNT];

  wdHybridArray<wdGALResourceViewHandle, 16> m_hResourceViews[wdGALShaderStage::ENUM_COUNT];
  wdHybridArray<const wdGALResourceBase*, 16> m_pResourcesForResourceViews[wdGALShaderStage::ENUM_COUNT];

  wdHybridArray<wdGALUnorderedAccessViewHandle, 16> m_hUnorderedAccessViews;
  wdHybridArray<const wdGALResourceBase*, 16> m_pResourcesForUnorderedAccessViews;

  wdGALSamplerStateHandle m_hSamplerStates[wdGALShaderStage::ENUM_COUNT][WD_GAL_MAX_SAMPLER_COUNT];
};

struct WD_RENDERERFOUNDATION_DLL wdGALCommandEncoderRenderState : public wdGALCommandEncoderState
{
  virtual void InvalidateState() override;

  wdGALBufferHandle m_hVertexBuffers[WD_GAL_MAX_VERTEX_BUFFER_COUNT];
  wdGALBufferHandle m_hIndexBuffer;

  wdGALVertexDeclarationHandle m_hVertexDeclaration;
  wdGALPrimitiveTopology::Enum m_Topology = wdGALPrimitiveTopology::ENUM_COUNT;

  wdGALBlendStateHandle m_hBlendState;
  wdColor m_BlendFactor = wdColor(0, 0, 0, 0);
  wdUInt32 m_uiSampleMask = 0;

  wdGALDepthStencilStateHandle m_hDepthStencilState;
  wdUInt8 m_uiStencilRefValue = 0;

  wdGALRasterizerStateHandle m_hRasterizerState;

  wdRectU32 m_ScissorRect = wdRectU32(0xFFFFFFFF, 0xFFFFFFFF, 0, 0);
  wdRectFloat m_ViewPortRect = wdRectFloat(wdMath::MaxValue<float>(), wdMath::MaxValue<float>(), 0.0f, 0.0f);
  float m_fViewPortMinDepth = wdMath::MaxValue<float>();
  float m_fViewPortMaxDepth = -wdMath::MaxValue<float>();
};
