#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ShaderResource.h>

class WD_RENDERERCORE_DLL wdStereoTestPass : public wdRenderPipelinePass
{
  WD_ADD_DYNAMIC_REFLECTION(wdStereoTestPass, wdRenderPipelinePass);

public:
  wdStereoTestPass();
  ~wdStereoTestPass();

  virtual bool GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs) override;

  virtual void Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) override;

protected:
  wdRenderPipelineNodeInputPin m_PinInput;
  wdRenderPipelineNodeOutputPin m_PinOutput;

  wdShaderResourceHandle m_hShader;
};
