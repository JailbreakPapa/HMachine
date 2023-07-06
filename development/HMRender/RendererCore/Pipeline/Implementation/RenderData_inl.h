#include <Core/World/GameObject.h>

WD_ALWAYS_INLINE wdRenderData::Category::Category()
  : m_uiValue(0xFFFF)
{
}

WD_ALWAYS_INLINE wdRenderData::Category::Category(wdUInt16 uiValue)
  : m_uiValue(uiValue)
{
}

WD_ALWAYS_INLINE bool wdRenderData::Category::operator==(const Category& other) const
{
  return m_uiValue == other.m_uiValue;
}

WD_ALWAYS_INLINE bool wdRenderData::Category::operator!=(const Category& other) const
{
  return m_uiValue != other.m_uiValue;
}

//////////////////////////////////////////////////////////////////////////

// static
WD_FORCE_INLINE const wdRenderer* wdRenderData::GetCategoryRenderer(Category category, const wdRTTI* pRenderDataType)
{
  if (s_bRendererInstancesDirty)
  {
    CreateRendererInstances();
  }

  auto& categoryData = s_CategoryData[category.m_uiValue];

  wdUInt32 uiIndex = 0;
  if (categoryData.m_TypeToRendererIndex.TryGetValue(pRenderDataType, uiIndex))
  {
    return s_RendererInstances[uiIndex].Borrow();
  }

  return nullptr;
}

// static
WD_FORCE_INLINE const char* wdRenderData::GetCategoryName(Category category)
{
  return s_CategoryData[category.m_uiValue].m_sName.GetString();
}

WD_FORCE_INLINE wdUInt64 wdRenderData::GetCategorySortingKey(Category category, const wdCamera& camera) const
{
  return s_CategoryData[category.m_uiValue].m_sortingKeyFunc(this, m_uiSortingKey, camera);
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
static T* wdCreateRenderDataForThisFrame(const wdGameObject* pOwner)
{
  WD_CHECK_AT_COMPILETIME(WD_IS_DERIVED_FROM_STATIC(wdRenderData, T));

  T* pRenderData = WD_NEW(wdFrameAllocator::GetCurrentAllocator(), T);

  if (pOwner != nullptr)
  {
    pRenderData->m_hOwner = pOwner->GetHandle();
  }

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  pRenderData->m_pOwner = pOwner;
#endif

  return pRenderData;
}
