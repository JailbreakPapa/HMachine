#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdFrameDataProviderBase, 1, wdRTTINoAllocator)
WD_END_DYNAMIC_REFLECTED_TYPE;

wdFrameDataProviderBase::wdFrameDataProviderBase()

  = default;

void* wdFrameDataProviderBase::GetData(const wdRenderViewContext& renderViewContext)
{
  if (m_pData == nullptr || m_uiLastUpdateFrame != wdRenderWorld::GetFrameCounter())
  {
    m_pData = UpdateData(renderViewContext, m_pOwnerPipeline->GetRenderData());

    m_uiLastUpdateFrame = wdRenderWorld::GetFrameCounter();
  }

  return m_pData;
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_FrameDataProvider);
