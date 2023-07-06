#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

/// \brief A very basic render pass that renders into the color target.
///
/// Can either works as passthrough or if no input is present creates
/// output targets matching the view's render target.
/// Needs to be connected to a wdTargetPass to function.
class WD_RENDERERCORE_DLL wdSimpleRenderPass : public wdRenderPipelinePass
{
  WD_ADD_DYNAMIC_REFLECTION(wdSimpleRenderPass, wdRenderPipelinePass);

public:
  wdSimpleRenderPass(const char* szName = "SimpleRenderPass");
  ~wdSimpleRenderPass();

  virtual bool GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs) override;
  virtual void Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) override;

  void SetMessage(const char* szMessage);

protected:
  wdRenderPipelineNodePassThrougPin m_PinColor;
  wdRenderPipelineNodePassThrougPin m_PinDepthStencil;

  wdString m_sMessage;
};
