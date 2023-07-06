#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

/// \brief Blurs input and writes it to an output buffer of the same format.
class WD_RENDERERCORE_DLL wdBlurPass : public wdRenderPipelinePass
{
  WD_ADD_DYNAMIC_REFLECTION(wdBlurPass, wdRenderPipelinePass);

public:
  wdBlurPass();
  ~wdBlurPass();

  virtual bool GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs) override;

  virtual void Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) override;

  void SetRadius(wdInt32 iRadius);
  wdInt32 GetRadius() const;

protected:
  wdRenderPipelineNodeInputPin m_PinInput;
  wdRenderPipelineNodeOutputPin m_PinOutput;

  wdInt32 m_iRadius = 15;
  wdConstantBufferStorageHandle m_hBlurCB;
  wdShaderResourceHandle m_hShader;
};
