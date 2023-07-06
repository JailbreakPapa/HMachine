#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/SkyRenderPass.h>

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdSkyRenderPass, 1, wdRTTIDefaultAllocator<wdSkyRenderPass>)
WD_END_DYNAMIC_REFLECTED_TYPE;

wdSkyRenderPass::wdSkyRenderPass(const char* szName)
  : wdForwardRenderPass(szName)
{
}

wdSkyRenderPass::~wdSkyRenderPass() = default;

void wdSkyRenderPass::RenderObjects(const wdRenderViewContext& renderViewContext)
{
  RenderDataWithCategory(renderViewContext, wdDefaultRenderDataCategories::Sky);
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SkyRenderPass);
