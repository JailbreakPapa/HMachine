#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/SortingFunctions.h>

namespace
{
  WD_ALWAYS_INLINE wdUInt32 CalculateTypeHash(const wdRenderData* pRenderData)
  {
    wdUInt32 uiTypeHash = wdHashingUtils::StringHashTo32(pRenderData->GetDynamicRTTI()->GetTypeNameHash());
    return (uiTypeHash >> 16) ^ (uiTypeHash & 0xFFFF);
  }

  WD_FORCE_INLINE wdUInt32 CalculateDistance(const wdRenderData* pRenderData, const wdCamera& camera)
  {
    ///\todo far-plane is not enough to normalize distance
    const float fDistance = (camera.GetPosition() - pRenderData->m_GlobalTransform.m_vPosition).GetLength();
    const float fNormalizedDistance = wdMath::Clamp(fDistance / camera.GetFarPlane(), 0.0f, 1.0f);
    return static_cast<wdUInt32>(fNormalizedDistance * 65535.0f);
  }
} // namespace

// static
wdUInt64 wdRenderSortingFunctions::ByRenderDataThenFrontToBack(
  const wdRenderData* pRenderData, wdUInt32 uiRenderDataSortingKey, const wdCamera& camera)
{
  const wdUInt64 uiTypeHash = CalculateTypeHash(pRenderData);
  const wdUInt64 uiRenderDataSortingKey64 = uiRenderDataSortingKey;
  const wdUInt64 uiDistance = CalculateDistance(pRenderData, camera);

  const wdUInt64 uiSortingKey = (uiTypeHash << 48) | (uiRenderDataSortingKey64 << 16) | uiDistance;
  return uiSortingKey;
}

// static
wdUInt64 wdRenderSortingFunctions::BackToFrontThenByRenderData(
  const wdRenderData* pRenderData, wdUInt32 uiRenderDataSortingKey, const wdCamera& camera)
{
  const wdUInt64 uiTypeHash = CalculateTypeHash(pRenderData);
  const wdUInt64 uiRenderDataSortingKey64 = uiRenderDataSortingKey;
  const wdUInt64 uiInvDistance = 0xFFFF - CalculateDistance(pRenderData, camera);

  const wdUInt64 uiSortingKey = (uiInvDistance << 48) | (uiTypeHash << 32) | uiRenderDataSortingKey64;
  return uiSortingKey;
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_SortingFunctions);
