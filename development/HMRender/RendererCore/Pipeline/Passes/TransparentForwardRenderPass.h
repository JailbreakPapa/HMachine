#pragma once

#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>

/// \brief A forward render pass that renders all transparent objects into the color target.
class WD_RENDERERCORE_DLL wdTransparentForwardRenderPass : public wdForwardRenderPass
{
  WD_ADD_DYNAMIC_REFLECTION(wdTransparentForwardRenderPass, wdForwardRenderPass);

public:
  wdTransparentForwardRenderPass(const char* szName = "TransparentForwardRenderPass");
  ~wdTransparentForwardRenderPass();

  virtual void Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) override;

protected:
  virtual void SetupResources(wdGALPass* pGALPass, const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) override;
  virtual void RenderObjects(const wdRenderViewContext& renderViewContext) override;

  void UpdateSceneColorTexture(const wdRenderViewContext& renderViewContext, wdGALTextureHandle hSceneColorTexture, wdGALTextureHandle hCurrentColorTexture);
  void CreateSamplerState();

  wdRenderPipelineNodeInputPin m_PinResolvedDepth;

  wdGALSamplerStateHandle m_hSceneColorSamplerState;
};
