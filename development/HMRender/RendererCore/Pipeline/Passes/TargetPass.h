#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

struct wdGALRenderTargets;

class WD_RENDERERCORE_DLL wdTargetPass : public wdRenderPipelinePass
{
  WD_ADD_DYNAMIC_REFLECTION(wdTargetPass, wdRenderPipelinePass);

public:
  wdTargetPass(const char* szName = "TargetPass");
  ~wdTargetPass();

  const wdGALTextureHandle* GetTextureHandle(const wdGALRenderTargets& renderTargets, const wdRenderPipelineNodePin* pPin);

  virtual bool GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs) override;
  virtual void Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) override;

private:
  bool VerifyInput(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, const char* szPinName);

protected:
  wdRenderPipelineNodeInputPin m_PinColor0;
  wdRenderPipelineNodeInputPin m_PinColor1;
  wdRenderPipelineNodeInputPin m_PinColor2;
  wdRenderPipelineNodeInputPin m_PinColor3;
  wdRenderPipelineNodeInputPin m_PinColor4;
  wdRenderPipelineNodeInputPin m_PinColor5;
  wdRenderPipelineNodeInputPin m_PinColor6;
  wdRenderPipelineNodeInputPin m_PinColor7;
  wdRenderPipelineNodeInputPin m_PinDepthStencil;
};
