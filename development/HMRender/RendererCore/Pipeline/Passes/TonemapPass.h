#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture3DResource.h>

class WD_RENDERERCORE_DLL wdTonemapPass : public wdRenderPipelinePass
{
  WD_ADD_DYNAMIC_REFLECTION(wdTonemapPass, wdRenderPipelinePass);

public:
  wdTonemapPass();
  ~wdTonemapPass();

  virtual bool GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs) override;

  virtual void Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) override;

protected:
  wdRenderPipelineNodeInputPin m_PinColorInput;
  wdRenderPipelineNodeInputPin m_PinBloomInput;
  wdRenderPipelineNodeOutputPin m_PinOutput;

  void SetVignettingTextureFile(const char* szFile);
  const char* GetVignettingTextureFile() const;

  void SetLUT1TextureFile(const char* szFile);
  const char* GetLUT1TextureFile() const;

  void SetLUT2TextureFile(const char* szFile);
  const char* GetLUT2TextureFile() const;

  wdTexture2DResourceHandle m_hVignettingTexture;
  wdTexture2DResourceHandle m_hNoiseTexture;
  wdTexture3DResourceHandle m_hLUT1;
  wdTexture3DResourceHandle m_hLUT2;

  wdColor m_MoodColor;
  float m_fMoodStrength;
  float m_fSaturation;
  float m_fContrast;
  float m_fLut1Strength;
  float m_fLut2Strength;

  wdConstantBufferStorageHandle m_hConstantBuffer;
  wdShaderResourceHandle m_hShader;
};
