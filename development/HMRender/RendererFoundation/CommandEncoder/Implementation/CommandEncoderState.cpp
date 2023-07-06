#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>

void wdGALCommandEncoderState::InvalidateState()
{
  m_hShader = wdGALShaderHandle();

  for (wdUInt32 i = 0; i < WD_ARRAY_SIZE(m_hConstantBuffers); ++i)
  {
    m_hConstantBuffers[i].Invalidate();
  }

  for (wdUInt32 i = 0; i < wdGALShaderStage::ENUM_COUNT; ++i)
  {
    m_hResourceViews[i].Clear();
    m_pResourcesForResourceViews[i].Clear();
  }

  m_hUnorderedAccessViews.Clear();
  m_pResourcesForUnorderedAccessViews.Clear();

  for (wdUInt32 i = 0; i < wdGALShaderStage::ENUM_COUNT; ++i)
  {
    for (wdUInt32 j = 0; j < WD_GAL_MAX_SAMPLER_COUNT; j++)
    {
      m_hSamplerStates[i][j].Invalidate();
    }
  }
}

void wdGALCommandEncoderRenderState::InvalidateState()
{
  wdGALCommandEncoderState::InvalidateState();

  for (wdUInt32 i = 0; i < WD_ARRAY_SIZE(m_hVertexBuffers); ++i)
  {
    m_hVertexBuffers[i].Invalidate();
  }
  m_hIndexBuffer.Invalidate();

  m_hVertexDeclaration.Invalidate();
  m_Topology = wdGALPrimitiveTopology::ENUM_COUNT;

  m_hBlendState.Invalidate();
  m_BlendFactor = wdColor(0, 0, 0, 0);
  m_uiSampleMask = 0;

  m_hDepthStencilState.Invalidate();
  m_uiStencilRefValue = 0;

  m_hRasterizerState.Invalidate();

  m_ScissorRect = wdRectU32(0xFFFFFFFF, 0xFFFFFFFF, 0, 0);
  m_ViewPortRect = wdRectFloat(wdMath::MaxValue<float>(), wdMath::MaxValue<float>(), 0.0f, 0.0f);
  m_fViewPortMinDepth = wdMath::MaxValue<float>();
  m_fViewPortMaxDepth = -wdMath::MaxValue<float>();
}


WD_STATICLINK_FILE(RendererFoundation, RendererFoundation_CommandEncoder_Implementation_CommandEncoderState);
