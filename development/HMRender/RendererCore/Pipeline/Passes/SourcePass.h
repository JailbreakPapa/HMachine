#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

class WD_RENDERERCORE_DLL wdSourcePass : public wdRenderPipelinePass
{
  WD_ADD_DYNAMIC_REFLECTION(wdSourcePass, wdRenderPipelinePass);

public:
  wdSourcePass(const char* szName = "SourcePass");
  ~wdSourcePass();

  virtual bool GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs) override;
  virtual void Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) override;

protected:
  wdRenderPipelineNodeOutputPin m_PinOutput;

  wdGALResourceFormat::Enum m_Format;
  wdGALMSAASampleCount::Enum m_MsaaMode;
  wdColor m_ClearColor;
  bool m_bClear;
};
