#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ClusteredDataUtils.h>
#include <RendererCore/Lights/SimplifiedDataExtractor.h>
#include <RendererCore/Pipeline/View.h>

//////////////////////////////////////////////////////////////////////////

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdSimplifiedDataCPU, 1, wdRTTINoAllocator)
WD_END_DYNAMIC_REFLECTED_TYPE;

wdSimplifiedDataCPU::wdSimplifiedDataCPU() = default;
wdSimplifiedDataCPU::~wdSimplifiedDataCPU() = default;

//////////////////////////////////////////////////////////////////////////

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdSimplifiedDataExtractor, 1, wdRTTIDefaultAllocator<wdSimplifiedDataExtractor>)
WD_END_DYNAMIC_REFLECTED_TYPE;

wdSimplifiedDataExtractor::wdSimplifiedDataExtractor(const char* szName)
  : wdExtractor(szName)
{
  m_DependsOn.PushBack(wdMakeHashedString("wdVisibleObjectsExtractor"));
}

wdSimplifiedDataExtractor::~wdSimplifiedDataExtractor() = default;

void wdSimplifiedDataExtractor::PostSortAndBatch(
  const wdView& view, const wdDynamicArray<const wdGameObject*>& visibleObjects, wdExtractedRenderData& ref_extractedRenderData)
{
  const wdCamera* pCamera = view.GetCullingCamera();
  const float fAspectRatio = view.GetViewport().width / view.GetViewport().height;

  wdSimplifiedDataCPU* pData = WD_NEW(wdFrameAllocator::GetCurrentAllocator(), wdSimplifiedDataCPU);

  pData->m_uiSkyIrradianceIndex = view.GetWorld()->GetIndex();
  pData->m_cameraUsageHint = view.GetCameraUsageHint();

  ref_extractedRenderData.AddFrameData(pData);
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SimplifiedDataExtractor);
