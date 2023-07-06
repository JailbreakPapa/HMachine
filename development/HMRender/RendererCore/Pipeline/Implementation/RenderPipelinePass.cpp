#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/Renderer.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Profiling/Profiling.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdRenderPipelinePass, 1, wdRTTINoAllocator)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new wdDefaultValueAttribute(true)),
    WD_ACCESSOR_PROPERTY("Name", GetName, SetName),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdColorAttribute(wdColorScheme::DarkUI(wdColorScheme::Grape))
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdRenderPipelinePass::wdRenderPipelinePass(const char* szName, bool bIsStereoAware)
  : m_bIsStereoAware(bIsStereoAware)

{
  m_sName.Assign(szName);
}

wdRenderPipelinePass::~wdRenderPipelinePass() = default;

void wdRenderPipelinePass::SetName(const char* szName)
{
  if (!wdStringUtils::IsNullOrEmpty(szName))
  {
    m_sName.Assign(szName);
  }
}

const char* wdRenderPipelinePass::GetName() const
{
  return m_sName.GetData();
}

void wdRenderPipelinePass::InitRenderPipelinePass(const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) {}

void wdRenderPipelinePass::ExecuteInactive(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) {}

void wdRenderPipelinePass::ReadBackProperties(wdView* pView) {}

void wdRenderPipelinePass::RenderDataWithCategory(const wdRenderViewContext& renderViewContext, wdRenderData::Category category, wdRenderDataBatch::Filter filter)
{
  WD_PROFILE_AND_MARKER(renderViewContext.m_pRenderContext->GetCommandEncoder(), wdRenderData::GetCategoryName(category));

  auto batchList = m_pPipeline->GetRenderDataBatchesWithCategory(category, filter);
  const wdUInt32 uiBatchCount = batchList.GetBatchCount();
  for (wdUInt32 i = 0; i < uiBatchCount; ++i)
  {
    const wdRenderDataBatch& batch = batchList.GetBatch(i);

    if (const wdRenderData* pRenderData = batch.GetFirstData<wdRenderData>())
    {
      const wdRTTI* pType = pRenderData->GetDynamicRTTI();

      if (const wdRenderer* pRenderer = wdRenderData::GetCategoryRenderer(category, pType))
      {
        pRenderer->RenderBatch(renderViewContext, this, batch);
      }
    }
  }
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelinePass);
