#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

/// \brief A render pass that renders into a depth target only.
class WD_RENDERERCORE_DLL wdDepthOnlyPass : public wdRenderPipelinePass
{
  WD_ADD_DYNAMIC_REFLECTION(wdDepthOnlyPass, wdRenderPipelinePass);

public:
  wdDepthOnlyPass(const char* szName = "DepthOnlyPass");
  ~wdDepthOnlyPass();

  virtual bool GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs) override;
  virtual void Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) override;

protected:
  wdRenderPipelineNodePassThrougPin m_PinDepthStencil;
};
