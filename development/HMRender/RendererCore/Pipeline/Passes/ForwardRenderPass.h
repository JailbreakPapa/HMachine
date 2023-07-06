#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>

struct wdForwardRenderShadingQuality
{
  using StorageType = wdInt8;

  enum Enum
  {
    Normal,
    Simplified,

    Default = Normal,
  };
};

WD_DECLARE_REFLECTABLE_TYPE(WD_NO_LINKAGE, wdForwardRenderShadingQuality);

/// \brief A standard forward render pass that renders into the color target.
class WD_RENDERERCORE_DLL wdForwardRenderPass : public wdRenderPipelinePass
{
  WD_ADD_DYNAMIC_REFLECTION(wdForwardRenderPass, wdRenderPipelinePass);

public:
  wdForwardRenderPass(const char* szName = "ForwardRenderPass");
  ~wdForwardRenderPass();

  virtual bool GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs) override;
  virtual void Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) override;

protected:
  virtual void SetupResources(wdGALPass* pGALPass, const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs);
  virtual void SetupPermutationVars(const wdRenderViewContext& renderViewContext);
  virtual void SetupLighting(const wdRenderViewContext& renderViewContext);

  virtual void RenderObjects(const wdRenderViewContext& renderViewContext) = 0;

  wdRenderPipelineNodePassThrougPin m_PinColor;
  wdRenderPipelineNodePassThrougPin m_PinDepthStencil;

  wdEnum<wdForwardRenderShadingQuality> m_ShadingQuality;
};
