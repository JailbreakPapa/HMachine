#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <RendererCore/Rasterizer/RasterizerView.h>
#include <RendererCore/Rasterizer/Thirdparty/Occluder.h>
#include <RendererCore/Rasterizer/Thirdparty/Rasterizer.h>

wdCVarInt cvar_SpatialCullingOcclusionMaxResolution("Spatial.Occlusion.MaxResolution", 512, wdCVarFlags::Default, "Max resolution for occlusion buffers.");
wdCVarInt cvar_SpatialCullingOcclusionMaxOccluders("Spatial.Occlusion.MaxOccluders", 64, wdCVarFlags::Default, "Max number of occluders to rasterize per frame.");

wdRasterizerView::wdRasterizerView() = default;
wdRasterizerView::~wdRasterizerView() = default;

void wdRasterizerView::SetResolution(wdUInt32 uiWidth, wdUInt32 uiHeight, float fAspectRatio)
{
  if (m_uiResolutionX != uiWidth || m_uiResolutionY != uiHeight)
  {
    m_uiResolutionX = uiWidth;
    m_uiResolutionY = uiHeight;

    m_pRasterizer = WD_DEFAULT_NEW(Rasterizer, uiWidth, uiHeight);
  }

  if (fAspectRatio == 0.0f)
    m_fAspectRation = float(m_uiResolutionX) / float(m_uiResolutionY);
  else
    m_fAspectRation = fAspectRatio;
}

void wdRasterizerView::BeginScene()
{
  WD_ASSERT_DEV(m_pRasterizer != nullptr, "Call SetResolution() first.");

  WD_PROFILE_SCOPE("Occlusion::Clear");

  m_pRasterizer->clear();
  m_bAnyOccludersRasterized = false;
}

void wdRasterizerView::ReadBackFrame(wdArrayPtr<wdColorLinearUB> targetBuffer) const
{
  WD_PROFILE_SCOPE("Occlusion::ReadFrame");

  WD_ASSERT_DEV(m_pRasterizer != nullptr, "Call SetResolution() first.");
  WD_ASSERT_DEV(targetBuffer.GetCount() >= m_uiResolutionX * m_uiResolutionY, "Target buffer is too small.");

  m_pRasterizer->readBackDepth(targetBuffer.GetPtr());
}

void wdRasterizerView::EndScene()
{
  if (m_Instances.IsEmpty())
    return;

  WD_PROFILE_SCOPE("Occlusion::RasterizeScene");

  SortObjectsFrontToBack();

  UpdateViewProjectionMatrix();

  // only rasterize a limited number of the closest objects
  RasterizeObjects(cvar_SpatialCullingOcclusionMaxOccluders);

  m_Instances.Clear();

  m_pRasterizer->setModelViewProjection(m_mViewProjection.m_fElementsCM);
}

void wdRasterizerView::RasterizeObjects(wdUInt32 uiMaxObjects)
{
#if WD_ENABLED(WD_RASTERIZER_SUPPORTED)

  WD_PROFILE_SCOPE("Occlusion::RasterizeObjects");

  for (const Instance& inst : m_Instances)
  {
    ApplyModelViewProjectionMatrix(inst.m_Transform);

    bool bNeedsClipping;
    const Occluder& occluder = inst.m_pObject->m_Occluder;

    if (m_pRasterizer->queryVisibility(occluder.m_boundsMin, occluder.m_boundsMax, bNeedsClipping))
    {
      m_bAnyOccludersRasterized = true;

      if (bNeedsClipping)
      {
        m_pRasterizer->rasterize<true>(occluder);
      }
      else
      {
        m_pRasterizer->rasterize<false>(occluder);
      }

      if (--uiMaxObjects == 0)
        return;
    }
  }
#endif
}

void wdRasterizerView::UpdateViewProjectionMatrix()
{
  wdMat4 mProjection;
  m_pCamera->GetProjectionMatrix(m_fAspectRation, mProjection, wdCameraEye::Left, wdClipSpaceDepthRange::ZeroToOne);

  m_mViewProjection = mProjection * m_pCamera->GetViewMatrix();
}

void wdRasterizerView::ApplyModelViewProjectionMatrix(const wdTransform& modelTransform)
{
  const wdMat4 mModel = modelTransform.GetAsMat4();
  const wdMat4 mMVP = m_mViewProjection * mModel;

  m_pRasterizer->setModelViewProjection(mMVP.m_fElementsCM);
}

void wdRasterizerView::SortObjectsFrontToBack()
{
#if WD_ENABLED(WD_RASTERIZER_SUPPORTED)
  WD_PROFILE_SCOPE("Occlusion::SortObjects");

  const wdVec3 camPos = m_pCamera->GetCenterPosition();

  m_Instances.Sort([&](const Instance& i1, const Instance& i2) {
      const float d1 = (i1.m_Transform.m_vPosition - camPos).GetLengthSquared();
      const float d2 = (i2.m_Transform.m_vPosition - camPos).GetLengthSquared();

      return d1 < d2; });
#endif
}

bool wdRasterizerView::IsVisible(const wdSimdBBox& aabb) const
{
#if WD_ENABLED(WD_RASTERIZER_SUPPORTED)
  if (!m_bAnyOccludersRasterized)
    return true; // assume that people already do frustum culling anyway

  WD_PROFILE_SCOPE("Occlusion::IsVisible");

  wdSimdVec4f vmin = aabb.m_Min;
  wdSimdVec4f vmax = aabb.m_Max;

  // wdSimdBBox makes no guarantees what's in the W component
  // but the SW rasterizer requires them to be 1
  vmin.SetW(1);
  vmax.SetW(1);

  bool needsClipping = false;
  return m_pRasterizer->queryVisibility(vmin.m_v, vmax.m_v, needsClipping);
#else
  return true;
#endif
}

wdRasterizerView* wdRasterizerViewPool::GetRasterizerView(wdUInt32 uiWidth, wdUInt32 uiHeight, float fAspectRatio)
{
  WD_PROFILE_SCOPE("Occlusion::GetViewFromPool");

  WD_LOCK(m_Mutex);

  const float divX = (float)uiWidth / (float)cvar_SpatialCullingOcclusionMaxResolution;
  const float divY = (float)uiHeight / (float)cvar_SpatialCullingOcclusionMaxResolution;
  const float div = wdMath::Max(divX, divY);

  if (div > 1.0)
  {
    uiWidth = (wdUInt32)(uiWidth / div);
    uiHeight = (wdUInt32)(uiHeight / div);
  }

  uiWidth = wdMath::RoundDown(uiWidth, 8);
  uiHeight = wdMath::RoundDown(uiHeight, 8);

  uiWidth = wdMath::Clamp<wdUInt32>(uiWidth, 32u, cvar_SpatialCullingOcclusionMaxResolution);
  uiHeight = wdMath::Clamp<wdUInt32>(uiHeight, 32u, cvar_SpatialCullingOcclusionMaxResolution);

  for (PoolEntry& entry : m_Entries)
  {
    if (entry.m_bInUse)
      continue;

    if (entry.m_RasterizerView.GetResolutionX() == uiWidth && entry.m_RasterizerView.GetResolutionY() == uiHeight)
    {
      entry.m_bInUse = true;
      entry.m_RasterizerView.SetResolution(uiWidth, uiHeight, fAspectRatio);
      return &entry.m_RasterizerView;
    }
  }

  auto& ne = m_Entries.ExpandAndGetRef();
  ne.m_RasterizerView.SetResolution(uiWidth, uiHeight, fAspectRatio);
  ne.m_bInUse = true;

  return &ne.m_RasterizerView;
}

void wdRasterizerViewPool::ReturnRasterizerView(wdRasterizerView* pView)
{
  if (pView == nullptr)
    return;

  WD_PROFILE_SCOPE("Occlusion::ReturnViewToPool");

  pView->SetCamera(nullptr);

  WD_LOCK(m_Mutex);

  for (PoolEntry& entry : m_Entries)
  {
    if (&entry.m_RasterizerView == pView)
    {
      entry.m_bInUse = false;
      return;
    }
  }

  WD_ASSERT_NOT_IMPLEMENTED;
}


WD_STATICLINK_FILE(RendererCore, RendererCore_Rasterizer_Implementation_RasterizerView);
