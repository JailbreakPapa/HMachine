#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

/// \brief Depth aware blur on input and writes it to an output buffer of the same format.
///
/// In theory it is mathematical nonsense to separate a bilateral blur, but it is common praxis and works good enough.
/// (Thus the name "separated" in contrast to "separable")
class WD_RENDERERCORE_DLL wdSeparatedBilateralBlurPass : public wdRenderPipelinePass
{
  WD_ADD_DYNAMIC_REFLECTION(wdSeparatedBilateralBlurPass, wdRenderPipelinePass);

public:
  wdSeparatedBilateralBlurPass();
  ~wdSeparatedBilateralBlurPass();

  virtual bool GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs) override;

  virtual void Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) override;

  void SetRadius(wdUInt32 uiRadius);
  wdUInt32 GetRadius() const;

  void SetGaussianSigma(float fSigma);
  float GetGaussianSigma() const;

  void SetSharpness(float fSharpness);
  float GetSharpness() const;

protected:
  wdRenderPipelineNodeInputPin m_PinBlurSourceInput;
  wdRenderPipelineNodeInputPin m_PinDepthInput;
  wdRenderPipelineNodeOutputPin m_PinOutput;

  wdUInt32 m_uiRadius = 7;
  float m_fGaussianSigma = 3.5f;
  float m_fSharpness = 120.0f;
  wdConstantBufferStorageHandle m_hBilateralBlurCB;
  wdShaderResourceHandle m_hShader;
};
