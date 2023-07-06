#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>

wdExtractedRenderData::wdExtractedRenderData() = default;

void wdExtractedRenderData::AddRenderData(const wdRenderData* pRenderData, wdRenderData::Category category)
{
  m_DataPerCategory.EnsureCount(category.m_uiValue + 1);

  auto& sortableRenderData = m_DataPerCategory[category.m_uiValue].m_SortableRenderData.ExpandAndGetRef();
  sortableRenderData.m_pRenderData = pRenderData;
  sortableRenderData.m_uiSortingKey = pRenderData->GetCategorySortingKey(category, m_Camera);
}

void wdExtractedRenderData::AddFrameData(const wdRenderData* pFrameData)
{
  m_FrameData.PushBack(pFrameData);
}

void wdExtractedRenderData::SortAndBatch()
{
  WD_PROFILE_SCOPE("SortAndBatch");

  struct RenderDataComparer
  {
    WD_FORCE_INLINE bool Less(const wdRenderDataBatch::SortableRenderData& a, const wdRenderDataBatch::SortableRenderData& b) const
    {
      if (a.m_uiSortingKey == b.m_uiSortingKey)
      {
        return a.m_pRenderData->m_uiBatchId < b.m_pRenderData->m_uiBatchId;
      }

      return a.m_uiSortingKey < b.m_uiSortingKey;
    }
  };

  for (auto& dataPerCategory : m_DataPerCategory)
  {
    if (dataPerCategory.m_SortableRenderData.IsEmpty())
      continue;

    auto& data = dataPerCategory.m_SortableRenderData;

    // Sort
    data.Sort(RenderDataComparer());

    // Find batches
    wdUInt32 uiCurrentBatchId = data[0].m_pRenderData->m_uiBatchId;
    wdUInt32 uiCurrentBatchStartIndex = 0;
    const wdRTTI* pCurrentBatchType = data[0].m_pRenderData->GetDynamicRTTI();

    for (wdUInt32 i = 1; i < data.GetCount(); ++i)
    {
      auto pRenderData = data[i].m_pRenderData;

      if (pRenderData->m_uiBatchId != uiCurrentBatchId || pRenderData->GetDynamicRTTI() != pCurrentBatchType)
      {
        dataPerCategory.m_Batches.ExpandAndGetRef().m_Data = wdMakeArrayPtr(&data[uiCurrentBatchStartIndex], i - uiCurrentBatchStartIndex);

        uiCurrentBatchId = pRenderData->m_uiBatchId;
        uiCurrentBatchStartIndex = i;
        pCurrentBatchType = pRenderData->GetDynamicRTTI();
      }
    }

    dataPerCategory.m_Batches.ExpandAndGetRef().m_Data = wdMakeArrayPtr(&data[uiCurrentBatchStartIndex], data.GetCount() - uiCurrentBatchStartIndex);
  }
}

void wdExtractedRenderData::Clear()
{
  for (auto& dataPerCategory : m_DataPerCategory)
  {
    dataPerCategory.m_Batches.Clear();
    dataPerCategory.m_SortableRenderData.Clear();
  }

  m_FrameData.Clear();

  // TODO: intelligent compact
}

wdRenderDataBatchList wdExtractedRenderData::GetRenderDataBatchesWithCategory(wdRenderData::Category category, wdRenderDataBatch::Filter filter) const
{
  if (category.m_uiValue < m_DataPerCategory.GetCount())
  {
    wdRenderDataBatchList list;
    list.m_Batches = m_DataPerCategory[category.m_uiValue].m_Batches;
    list.m_Filter = filter;

    return list;
  }

  return wdRenderDataBatchList();
}

const wdRenderData* wdExtractedRenderData::GetFrameData(const wdRTTI* pRtti) const
{
  for (auto pData : m_FrameData)
  {
    if (pData->IsInstanceOf(pRtti))
    {
      return pData;
    }
  }

  return nullptr;
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_ExtractedRenderData);
