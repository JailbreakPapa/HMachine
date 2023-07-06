#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class WD_RENDERERCORE_DLL wdReflectionFilterPass : public wdRenderPipelinePass
{
  WD_ADD_DYNAMIC_REFLECTION(wdReflectionFilterPass, wdRenderPipelinePass);

public:
  wdReflectionFilterPass();
  ~wdReflectionFilterPass();

  virtual bool GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs) override;

  virtual void Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) override;

  wdUInt32 GetInputCubemap() const;
  void SetInputCubemap(wdUInt32 uiCubemapHandle);

protected:
  void UpdateFilteredSpecularConstantBuffer(wdUInt32 uiMipMapIndex, wdUInt32 uiNumMipMaps);
  void UpdateIrradianceConstantBuffer();

  wdRenderPipelineNodeOutputPin m_PinFilteredSpecular;
  wdRenderPipelineNodeOutputPin m_PinAvgLuminance;
  wdRenderPipelineNodeOutputPin m_PinIrradianceData;

  float m_fIntensity = 1.0f;
  float m_fSaturation = 1.0f;
  wdUInt32 m_uiSpecularOutputIndex = 0;
  wdUInt32 m_uiIrradianceOutputIndex = 0;

  wdGALTextureHandle m_hInputCubemap;

  wdConstantBufferStorageHandle m_hFilteredSpecularConstantBuffer;
  wdShaderResourceHandle m_hFilteredSpecularShader;

  wdConstantBufferStorageHandle m_hIrradianceConstantBuffer;
  wdShaderResourceHandle m_hIrradianceShader;
};
