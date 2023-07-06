#pragma once

#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>

/// \brief A forward render pass that renders all sky objects into the color target.
class WD_RENDERERCORE_DLL wdSkyRenderPass : public wdForwardRenderPass
{
  WD_ADD_DYNAMIC_REFLECTION(wdSkyRenderPass, wdForwardRenderPass);

public:
  wdSkyRenderPass(const char* szName = "SkyRenderPass");
  ~wdSkyRenderPass();

protected:
  virtual void RenderObjects(const wdRenderViewContext& renderViewContext) override;
};
