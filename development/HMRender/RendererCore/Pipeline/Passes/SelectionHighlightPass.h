#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class WD_RENDERERCORE_DLL wdSelectionHighlightPass : public wdRenderPipelinePass
{
  WD_ADD_DYNAMIC_REFLECTION(wdSelectionHighlightPass, wdRenderPipelinePass);

public:
  wdSelectionHighlightPass(const char* szName = "SelectionHighlightPass");
  ~wdSelectionHighlightPass();

  virtual bool GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs) override;
  virtual void Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) override;

protected:
  wdRenderPipelineNodePassThrougPin m_PinColor;
  wdRenderPipelineNodeInputPin m_PinDepthStencil;

  wdShaderResourceHandle m_hShader;
  wdConstantBufferStorageHandle m_hConstantBuffer;

  wdColor m_HighlightColor = wdColorScheme::LightUI(wdColorScheme::Yellow);
  float m_fOverlayOpacity = 0.1f;
};
