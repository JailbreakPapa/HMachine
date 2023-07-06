#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class WD_RENDERERCORE_DLL wdBloomPass : public wdRenderPipelinePass
{
  WD_ADD_DYNAMIC_REFLECTION(wdBloomPass, wdRenderPipelinePass);

public:
  wdBloomPass();
  ~wdBloomPass();

  virtual bool GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs) override;

  virtual void Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) override;
  virtual void ExecuteInactive(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) override;

protected:
  void UpdateConstantBuffer(wdVec2 pixelSize, const wdColor& tintColor);

  wdRenderPipelineNodeInputPin m_PinInput;
  wdRenderPipelineNodeOutputPin m_PinOutput;

  float m_fRadius = 0.2f;
  float m_fThreshold = 1.0f;
  float m_fIntensity = 0.3f;
  wdColorGammaUB m_InnerTintColor = wdColor::White;
  wdColorGammaUB m_MidTintColor = wdColor::White;
  wdColorGammaUB m_OuterTintColor = wdColor::White;
  wdConstantBufferStorageHandle m_hConstantBuffer;
  wdShaderResourceHandle m_hShader;
};
