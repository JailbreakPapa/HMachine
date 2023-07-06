#include <RendererCore/RendererCorePCH.h>

#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <RendererCore/BakedProbes/BakedProbesWorldModule.h>
#include <RendererCore/BakedProbes/ProbeTreeSectorResource.h>

// clang-format off
WD_IMPLEMENT_WORLD_MODULE(wdBakedProbesWorldModule);
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdBakedProbesWorldModule, 1, wdRTTINoAllocator)
WD_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

wdBakedProbesWorldModule::wdBakedProbesWorldModule(wdWorld* pWorld)
  : wdWorldModule(pWorld)
{
}

wdBakedProbesWorldModule::~wdBakedProbesWorldModule() = default;

void wdBakedProbesWorldModule::Initialize()
{
}

void wdBakedProbesWorldModule::Deinitialize()
{
}

bool wdBakedProbesWorldModule::HasProbeData() const
{
  return m_hProbeTree.IsValid();
}

wdResult wdBakedProbesWorldModule::GetProbeIndexData(const wdVec3& vGlobalPosition, const wdVec3& vNormal, ProbeIndexData& out_probeIndexData) const
{
  // TODO: optimize

  if (!HasProbeData())
    return WD_FAILURE;

  wdResourceLock<wdProbeTreeSectorResource> pProbeTree(m_hProbeTree, wdResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pProbeTree.GetAcquireResult() != wdResourceAcquireResult::Final)
    return WD_FAILURE;

  wdSimdVec4f gridSpacePos = wdSimdConversion::ToVec3((vGlobalPosition - pProbeTree->GetGridOrigin()).CompDiv(pProbeTree->GetProbeSpacing()));
  gridSpacePos = gridSpacePos.CompMax(wdSimdVec4f::ZeroVector());

  wdSimdVec4f gridSpacePosFloor = gridSpacePos.Floor();
  wdSimdVec4f weights = gridSpacePos - gridSpacePosFloor;

  wdSimdVec4i maxIndices = wdSimdVec4i(pProbeTree->GetProbeCount().x, pProbeTree->GetProbeCount().y, pProbeTree->GetProbeCount().z) - wdSimdVec4i(1);
  wdSimdVec4i pos0 = wdSimdVec4i::Truncate(gridSpacePosFloor).CompMin(maxIndices);
  wdSimdVec4i pos1 = (pos0 + wdSimdVec4i(1)).CompMin(maxIndices);

  wdUInt32 x0 = pos0.x();
  wdUInt32 y0 = pos0.y();
  wdUInt32 z0 = pos0.z();

  wdUInt32 x1 = pos1.x();
  wdUInt32 y1 = pos1.y();
  wdUInt32 z1 = pos1.z();

  wdUInt32 xCount = pProbeTree->GetProbeCount().x;
  wdUInt32 xyCount = xCount * pProbeTree->GetProbeCount().y;

  out_probeIndexData.m_probeIndices[0] = z0 * xyCount + y0 * xCount + x0;
  out_probeIndexData.m_probeIndices[1] = z0 * xyCount + y0 * xCount + x1;
  out_probeIndexData.m_probeIndices[2] = z0 * xyCount + y1 * xCount + x0;
  out_probeIndexData.m_probeIndices[3] = z0 * xyCount + y1 * xCount + x1;
  out_probeIndexData.m_probeIndices[4] = z1 * xyCount + y0 * xCount + x0;
  out_probeIndexData.m_probeIndices[5] = z1 * xyCount + y0 * xCount + x1;
  out_probeIndexData.m_probeIndices[6] = z1 * xyCount + y1 * xCount + x0;
  out_probeIndexData.m_probeIndices[7] = z1 * xyCount + y1 * xCount + x1;

  wdVec3 w1 = wdSimdConversion::ToVec3(weights);
  wdVec3 w0 = wdVec3(1.0f) - w1;

  // TODO: add geometry factor to weight
  out_probeIndexData.m_probeWeights[0] = w0.x * w0.y * w0.z;
  out_probeIndexData.m_probeWeights[1] = w1.x * w0.y * w0.z;
  out_probeIndexData.m_probeWeights[2] = w0.x * w1.y * w0.z;
  out_probeIndexData.m_probeWeights[3] = w1.x * w1.y * w0.z;
  out_probeIndexData.m_probeWeights[4] = w0.x * w0.y * w1.z;
  out_probeIndexData.m_probeWeights[5] = w1.x * w0.y * w1.z;
  out_probeIndexData.m_probeWeights[6] = w0.x * w1.y * w1.z;
  out_probeIndexData.m_probeWeights[7] = w1.x * w1.y * w1.z;

  float weightSum = 0;
  for (wdUInt32 i = 0; i < ProbeIndexData::NumProbes; ++i)
  {
    weightSum += out_probeIndexData.m_probeWeights[i];
  }

  float normalizeFactor = 1.0f / weightSum;
  for (wdUInt32 i = 0; i < ProbeIndexData::NumProbes; ++i)
  {
    out_probeIndexData.m_probeWeights[i] *= normalizeFactor;
  }

  return WD_SUCCESS;
}

wdAmbientCube<float> wdBakedProbesWorldModule::GetSkyVisibility(const ProbeIndexData& indexData) const
{
  // TODO: optimize

  wdAmbientCube<float> result;

  wdResourceLock<wdProbeTreeSectorResource> pProbeTree(m_hProbeTree, wdResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pProbeTree.GetAcquireResult() != wdResourceAcquireResult::Final)
    return result;

  auto compressedSkyVisibility = pProbeTree->GetSkyVisibility();
  wdAmbientCube<float> skyVisibility;

  for (wdUInt32 i = 0; i < ProbeIndexData::NumProbes; ++i)
  {
    wdBakingUtils::DecompressSkyVisibility(compressedSkyVisibility[indexData.m_probeIndices[i]], skyVisibility);

    for (wdUInt32 d = 0; d < wdAmbientCubeBasis::NumDirs; ++d)
    {
      result.m_Values[d] += skyVisibility.m_Values[d] * indexData.m_probeWeights[i];
    }
  }

  return result;
}

void wdBakedProbesWorldModule::SetProbeTreeResourcePrefix(const wdHashedString& prefix)
{
  wdStringBuilder sResourcePath;
  sResourcePath.Format("{}_Global.wdProbeTreeSector", prefix);

  m_hProbeTree = wdResourceManager::LoadResource<wdProbeTreeSectorResource>(sResourcePath);
}


WD_STATICLINK_FILE(RendererCore, RendererCore_BakedProbes_Implementation_BakedProbesWorldModule);
