#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/Renderer.h>
#include <RendererCore/Pipeline/SortingFunctions.h>

// clang-format off
WD_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, RenderData)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    wdRenderData::UpdateRendererTypes();

    wdPlugin::Events().AddEventHandler(wdRenderData::PluginEventHandler);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    wdPlugin::Events().RemoveEventHandler(wdRenderData::PluginEventHandler);

    wdRenderData::ClearRendererInstances();
  }

WD_END_SUBSYSTEM_DECLARATION;
// clang-format on

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdRenderData, 1, wdRTTINoAllocator)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdRenderer, 1, wdRTTINoAllocator)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_IMPLEMENT_MESSAGE_TYPE(wdMsgExtractRenderData);
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdMsgExtractRenderData, 1, wdRTTIDefaultAllocator<wdMsgExtractRenderData>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_IMPLEMENT_MESSAGE_TYPE(wdMsgExtractOccluderData);
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdMsgExtractOccluderData, 1, wdRTTIDefaultAllocator<wdMsgExtractOccluderData>)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdHybridArray<wdRenderData::CategoryData, 32> wdRenderData::s_CategoryData;

wdHybridArray<const wdRTTI*, 16> wdRenderData::s_RendererTypes;
wdDynamicArray<wdUniquePtr<wdRenderer>> wdRenderData::s_RendererInstances;
bool wdRenderData::s_bRendererInstancesDirty = false;

// static
wdRenderData::Category wdRenderData::RegisterCategory(const char* szCategoryName, SortingKeyFunc sortingKeyFunc)
{
  Category oldCategory = FindCategory(szCategoryName);
  if (oldCategory != wdInvalidRenderDataCategory)
    return oldCategory;

  Category newCategory = Category(static_cast<wdUInt16>(s_CategoryData.GetCount()));

  auto& data = s_CategoryData.ExpandAndGetRef();
  data.m_sName.Assign(szCategoryName);
  data.m_sortingKeyFunc = sortingKeyFunc;

  return newCategory;
}

// static
wdRenderData::Category wdRenderData::FindCategory(const char* szCategoryName)
{
  wdTempHashedString categoryName(szCategoryName);

  for (wdUInt32 uiCategoryIndex = 0; uiCategoryIndex < s_CategoryData.GetCount(); ++uiCategoryIndex)
  {
    if (s_CategoryData[uiCategoryIndex].m_sName == categoryName)
      return Category(static_cast<wdUInt16>(uiCategoryIndex));
  }

  return wdInvalidRenderDataCategory;
}

// static
void wdRenderData::PluginEventHandler(const wdPluginEvent& e)
{
  switch (e.m_EventType)
  {
    case wdPluginEvent::AfterPluginChanges:
      UpdateRendererTypes();
      break;

    default:
      break;
  }
}

// static
void wdRenderData::UpdateRendererTypes()
{
  s_RendererTypes.Clear();

  for (const wdRTTI* pRtti = wdRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    if (!pRtti->IsDerivedFrom<wdRenderer>() || pRtti->GetTypeFlags().IsAnySet(wdTypeFlags::Abstract) || !pRtti->GetAllocator()->CanAllocate())
      continue;

    s_RendererTypes.PushBack(pRtti);
  }

  s_bRendererInstancesDirty = true;
}

// static
void wdRenderData::CreateRendererInstances()
{
  ClearRendererInstances();

  for (auto pRendererType : s_RendererTypes)
  {
    WD_ASSERT_DEV(pRendererType->IsDerivedFrom(wdGetStaticRTTI<wdRenderer>()), "Renderer type '{}' must be derived from wdRenderer",
      pRendererType->GetTypeName());

    auto pRenderer = pRendererType->GetAllocator()->Allocate<wdRenderer>();

    wdUInt32 uiIndex = s_RendererInstances.GetCount();
    s_RendererInstances.PushBack(pRenderer);

    wdHybridArray<Category, 8> supportedCategories;
    pRenderer->GetSupportedRenderDataCategories(supportedCategories);

    wdHybridArray<const wdRTTI*, 8> supportedTypes;
    pRenderer->GetSupportedRenderDataTypes(supportedTypes);

    for (Category category : supportedCategories)
    {
      auto& categoryData = s_CategoryData[category.m_uiValue];

      for (wdUInt32 i = 0; i < supportedTypes.GetCount(); ++i)
      {
        categoryData.m_TypeToRendererIndex.Insert(supportedTypes[i], uiIndex);
      }
    }
  }

  s_bRendererInstancesDirty = false;
}

// static
void wdRenderData::ClearRendererInstances()
{
  s_RendererInstances.Clear();

  for (auto& categoryData : s_CategoryData)
  {
    categoryData.m_TypeToRendererIndex.Clear();
  }
}

//////////////////////////////////////////////////////////////////////////

wdRenderData::Category wdDefaultRenderDataCategories::Light = wdRenderData::RegisterCategory("Light", &wdRenderSortingFunctions::ByRenderDataThenFrontToBack);
wdRenderData::Category wdDefaultRenderDataCategories::Decal = wdRenderData::RegisterCategory("Decal", &wdRenderSortingFunctions::ByRenderDataThenFrontToBack);
wdRenderData::Category wdDefaultRenderDataCategories::ReflectionProbe = wdRenderData::RegisterCategory("ReflectionProbe", &wdRenderSortingFunctions::ByRenderDataThenFrontToBack);
wdRenderData::Category wdDefaultRenderDataCategories::Sky = wdRenderData::RegisterCategory("Sky", &wdRenderSortingFunctions::ByRenderDataThenFrontToBack);
wdRenderData::Category wdDefaultRenderDataCategories::LitOpaque = wdRenderData::RegisterCategory("LitOpaque", &wdRenderSortingFunctions::ByRenderDataThenFrontToBack);
wdRenderData::Category wdDefaultRenderDataCategories::LitMasked = wdRenderData::RegisterCategory("LitMasked", &wdRenderSortingFunctions::ByRenderDataThenFrontToBack);
wdRenderData::Category wdDefaultRenderDataCategories::LitTransparent = wdRenderData::RegisterCategory("LitTransparent", &wdRenderSortingFunctions::BackToFrontThenByRenderData);
wdRenderData::Category wdDefaultRenderDataCategories::LitForeground = wdRenderData::RegisterCategory("LitForeground", &wdRenderSortingFunctions::ByRenderDataThenFrontToBack);
wdRenderData::Category wdDefaultRenderDataCategories::SimpleOpaque = wdRenderData::RegisterCategory("SimpleOpaque", &wdRenderSortingFunctions::ByRenderDataThenFrontToBack);
wdRenderData::Category wdDefaultRenderDataCategories::SimpleTransparent = wdRenderData::RegisterCategory("SimpleTransparent", &wdRenderSortingFunctions::BackToFrontThenByRenderData);
wdRenderData::Category wdDefaultRenderDataCategories::SimpleForeground = wdRenderData::RegisterCategory("SimpleForeground", &wdRenderSortingFunctions::ByRenderDataThenFrontToBack);
wdRenderData::Category wdDefaultRenderDataCategories::Selection = wdRenderData::RegisterCategory("Selection", &wdRenderSortingFunctions::ByRenderDataThenFrontToBack);
wdRenderData::Category wdDefaultRenderDataCategories::GUI = wdRenderData::RegisterCategory("GUI", &wdRenderSortingFunctions::BackToFrontThenByRenderData);

//////////////////////////////////////////////////////////////////////////

void wdMsgExtractRenderData::AddRenderData(
  const wdRenderData* pRenderData, wdRenderData::Category category, wdRenderData::Caching::Enum cachingBehavior)
{
  auto& cached = m_ExtractedRenderData.ExpandAndGetRef();
  cached.m_pRenderData = pRenderData;
  cached.m_uiCategory = category.m_uiValue;

  if (cachingBehavior == wdRenderData::Caching::IfStatic)
  {
    ++m_uiNumCacheIfStatic;
  }
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderData);
