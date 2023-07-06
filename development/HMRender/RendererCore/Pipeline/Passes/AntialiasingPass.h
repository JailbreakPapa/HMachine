#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ShaderResource.h>

class WD_RENDERERCORE_DLL wdAntialiasingPass : public wdRenderPipelinePass
{
  WD_ADD_DYNAMIC_REFLECTION(wdAntialiasingPass, wdRenderPipelinePass);

public:
  wdAntialiasingPass();
  ~wdAntialiasingPass();

  virtual bool GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs) override;

  virtual void Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) override;

protected:
  wdRenderPipelineNodeInputPin m_PinInput;
  wdRenderPipelineNodeOutputPin m_PinOutput;

  wdHashedString m_sMsaaSampleCount;
  wdShaderResourceHandle m_hShader;
};
