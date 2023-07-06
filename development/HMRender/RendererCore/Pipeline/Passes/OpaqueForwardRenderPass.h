#pragma once

#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>

/// \brief A forward render pass that renders all opaque objects into the color target.
class WD_RENDERERCORE_DLL wdOpaqueForwardRenderPass : public wdForwardRenderPass
{
  WD_ADD_DYNAMIC_REFLECTION(wdOpaqueForwardRenderPass, wdForwardRenderPass);

public:
  wdOpaqueForwardRenderPass(const char* szName = "OpaqueForwardRenderPass");
  ~wdOpaqueForwardRenderPass();

  virtual bool GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs) override;

protected:
  virtual void SetupResources(wdGALPass* pGALPass, const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) override;
  virtual void SetupPermutationVars(const wdRenderViewContext& renderViewContext) override;

  virtual void RenderObjects(const wdRenderViewContext& renderViewContext) override;

  wdRenderPipelineNodeInputPin m_PinSSAO;
  // wdRenderPipelineNodeOutputPin m_PinNormal;
  // wdRenderPipelineNodeOutputPin m_PinSpecularColorRoughness;

  bool m_bWriteDepth = true;

  wdTexture2DResourceHandle m_hWhiteTexture;
};
