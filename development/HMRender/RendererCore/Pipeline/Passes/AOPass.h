#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class WD_RENDERERCORE_DLL wdAOPass : public wdRenderPipelinePass
{
  WD_ADD_DYNAMIC_REFLECTION(wdAOPass, wdRenderPipelinePass);

public:
  wdAOPass();
  ~wdAOPass();

  virtual bool GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs) override;

  virtual void Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) override;
  virtual void ExecuteInactive(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) override;

  void SetFadeOutStart(float fStart);
  float GetFadeOutStart() const;

  void SetFadeOutEnd(float fEnd);
  float GetFadeOutEnd() const;

protected:
  void CreateSamplerState();

  wdRenderPipelineNodeInputPin m_PinDepthInput;
  wdRenderPipelineNodeOutputPin m_PinOutput;

  float m_fRadius = 1.0f;
  float m_fMaxScreenSpaceRadius = 1.0f;
  float m_fContrast = 2.0f;
  float m_fIntensity = 0.7f;

  float m_fFadeOutStart = 80.0f;
  float m_fFadeOutEnd = 100.0f;

  float m_fPositionBias = 5.0f;
  float m_fMipLevelScale = 10.0f;
  float m_fDepthBlurThreshold = 2.0f;

  wdConstantBufferStorageHandle m_hDownscaleConstantBuffer;
  wdConstantBufferStorageHandle m_hSSAOConstantBuffer;

  wdTexture2DResourceHandle m_hNoiseTexture;

  wdGALSamplerStateHandle m_hSSAOSamplerState;

  wdShaderResourceHandle m_hDownscaleShader;
  wdShaderResourceHandle m_hSSAOShader;
  wdShaderResourceHandle m_hBlurShader;
};
