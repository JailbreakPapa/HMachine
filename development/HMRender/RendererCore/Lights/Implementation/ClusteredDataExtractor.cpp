#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/Components/FogComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <RendererCore/Lights/ClusteredDataExtractor.h>
#include <RendererCore/Lights/Implementation/ClusteredDataUtils.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
wdCVarBool cvar_RenderingLightingVisClusterData("Rendering.Lighting.VisClusterData", false, wdCVarFlags::Default, "Enables debug visualization of clustered light data");
wdCVarInt cvar_RenderingLightingVisClusterDepthSlice("Rendering.Lighting.VisClusterDepthSlice", -1, wdCVarFlags::Default, "Show the debug visualization only for the given depth slice");

namespace
{
  void VisualizeClusteredData(const wdView& view, const wdClusteredDataCPU* pData, wdArrayPtr<wdSimdBSphere> boundingSpheres)
  {
    if (!cvar_RenderingLightingVisClusterData)
      return;

    const wdCamera* pCamera = view.GetCullingCamera();

    if (pCamera->IsOrthographic())
      return;

    float fAspectRatio = view.GetViewport().width / view.GetViewport().height;

    wdMat4 mProj;
    pCamera->GetProjectionMatrix(view.GetViewport().width / (float)view.GetViewport().height, mProj);

    wdAngle fFovLeft;
    wdAngle fFovRight;
    wdAngle fFovBottom;
    wdAngle fFovTop;
    wdGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fFovLeft, fFovRight, fFovBottom, fFovTop);

    const float fTanLeft = wdMath::Tan(fFovLeft);
    const float fTanRight = wdMath::Tan(fFovRight);
    const float fTanBottom = wdMath::Tan(fFovBottom);
    const float fTanTop = wdMath::Tan(fFovTop);

    wdColor lineColor = wdColor(1.0f, 1.0f, 1.0f, 0.1f);

    wdInt32 debugSlice = cvar_RenderingLightingVisClusterDepthSlice;
    wdUInt32 maxSlice = debugSlice < 0 ? NUM_CLUSTERS_Z : debugSlice + 1;
    wdUInt32 minSlice = debugSlice < 0 ? 0 : debugSlice;

    bool bDrawBoundingSphere = false;

    for (wdUInt32 z = maxSlice; z-- > minSlice;)
    {
      float fZf = GetDepthFromSliceIndex(z);
      float fZn = (z > 0) ? GetDepthFromSliceIndex(z - 1) : 0.0f;
      for (wdInt32 y = 0; y < NUM_CLUSTERS_Y; ++y)
      {
        for (wdInt32 x = 0; x < NUM_CLUSTERS_X; ++x)
        {
          wdUInt32 clusterIndex = GetClusterIndexFromCoord(x, y, z);
          auto& clusterData = pData->m_ClusterData[clusterIndex];

          if (clusterData.counts > 0)
          {
            if (bDrawBoundingSphere)
            {
              wdBoundingSphere s = wdSimdConversion::ToBSphere(boundingSpheres[clusterIndex]);
              wdDebugRenderer::DrawLineSphere(view.GetHandle(), s, lineColor);
            }

            wdVec3 cc[8];
            GetClusterCornerPoints(*pCamera, fZf, fZn, fTanLeft, fTanRight, fTanBottom, fTanTop, x, y, z, cc);

            float lightCount = (float)GET_LIGHT_INDEX(clusterData.counts);
            float decalCount = (float)GET_DECAL_INDEX(clusterData.counts);
            float r = wdMath::Clamp(lightCount / 16.0f, 0.0f, 1.0f);
            float g = wdMath::Clamp(decalCount / 16.0f, 0.0f, 1.0f);

            wdDebugRenderer::Triangle tris[12];
            // back
            tris[0] = wdDebugRenderer::Triangle(cc[0], cc[2], cc[1]);
            tris[1] = wdDebugRenderer::Triangle(cc[2], cc[3], cc[1]);
            // front
            tris[2] = wdDebugRenderer::Triangle(cc[4], cc[5], cc[6]);
            tris[3] = wdDebugRenderer::Triangle(cc[6], cc[5], cc[7]);
            // top
            tris[4] = wdDebugRenderer::Triangle(cc[4], cc[0], cc[5]);
            tris[5] = wdDebugRenderer::Triangle(cc[0], cc[1], cc[5]);
            // bottom
            tris[6] = wdDebugRenderer::Triangle(cc[6], cc[7], cc[2]);
            tris[7] = wdDebugRenderer::Triangle(cc[2], cc[7], cc[3]);
            // left
            tris[8] = wdDebugRenderer::Triangle(cc[4], cc[6], cc[0]);
            tris[9] = wdDebugRenderer::Triangle(cc[0], cc[6], cc[2]);
            // right
            tris[10] = wdDebugRenderer::Triangle(cc[5], cc[1], cc[7]);
            tris[11] = wdDebugRenderer::Triangle(cc[1], cc[3], cc[7]);

            wdDebugRenderer::DrawSolidTriangles(view.GetHandle(), tris, wdColor(r, g, 0.0f, 0.1f));

            wdDebugRenderer::Line lines[12];
            lines[0] = wdDebugRenderer::Line(cc[4], cc[5]);
            lines[1] = wdDebugRenderer::Line(cc[5], cc[7]);
            lines[2] = wdDebugRenderer::Line(cc[7], cc[6]);
            lines[3] = wdDebugRenderer::Line(cc[6], cc[4]);

            lines[4] = wdDebugRenderer::Line(cc[0], cc[1]);
            lines[5] = wdDebugRenderer::Line(cc[1], cc[3]);
            lines[6] = wdDebugRenderer::Line(cc[3], cc[2]);
            lines[7] = wdDebugRenderer::Line(cc[2], cc[0]);

            lines[8] = wdDebugRenderer::Line(cc[4], cc[0]);
            lines[9] = wdDebugRenderer::Line(cc[5], cc[1]);
            lines[10] = wdDebugRenderer::Line(cc[7], cc[3]);
            lines[11] = wdDebugRenderer::Line(cc[6], cc[2]);

            wdDebugRenderer::DrawLines(view.GetHandle(), lines, wdColor(r, g, 0.0f));
          }
        }
      }

      {
        wdVec3 leftWidth = pCamera->GetDirRight() * fZf * fTanLeft;
        wdVec3 rightWidth = pCamera->GetDirRight() * fZf * fTanRight;
        wdVec3 bottomHeight = pCamera->GetDirUp() * fZf * fTanBottom;
        wdVec3 topHeight = pCamera->GetDirUp() * fZf * fTanTop;

        wdVec3 depthFar = pCamera->GetPosition() + pCamera->GetDirForwards() * fZf;
        wdVec3 p0 = depthFar + rightWidth + topHeight;
        wdVec3 p1 = depthFar + rightWidth + bottomHeight;
        wdVec3 p2 = depthFar + leftWidth + bottomHeight;
        wdVec3 p3 = depthFar + leftWidth + topHeight;

        wdDebugRenderer::Line lines[4];
        lines[0] = wdDebugRenderer::Line(p0, p1);
        lines[1] = wdDebugRenderer::Line(p1, p2);
        lines[2] = wdDebugRenderer::Line(p2, p3);
        lines[3] = wdDebugRenderer::Line(p3, p0);

        wdDebugRenderer::DrawLines(view.GetHandle(), lines, lineColor);
      }
    }
  }
} // namespace
#endif

//////////////////////////////////////////////////////////////////////////

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdClusteredDataCPU, 1, wdRTTINoAllocator)
WD_END_DYNAMIC_REFLECTED_TYPE;

wdClusteredDataCPU::wdClusteredDataCPU() = default;
wdClusteredDataCPU::~wdClusteredDataCPU() = default;

//////////////////////////////////////////////////////////////////////////

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdClusteredDataExtractor, 1, wdRTTIDefaultAllocator<wdClusteredDataExtractor>)
WD_END_DYNAMIC_REFLECTED_TYPE;

wdClusteredDataExtractor::wdClusteredDataExtractor(const char* szName)
  : wdExtractor(szName)
{
  m_DependsOn.PushBack(wdMakeHashedString("wdVisibleObjectsExtractor"));

  m_TempLightsClusters.SetCountUninitialized(NUM_CLUSTERS);
  m_TempDecalsClusters.SetCountUninitialized(NUM_CLUSTERS);
  m_TempReflectionProbeClusters.SetCountUninitialized(NUM_CLUSTERS);
  m_ClusterBoundingSpheres.SetCountUninitialized(NUM_CLUSTERS);
}

wdClusteredDataExtractor::~wdClusteredDataExtractor() = default;

void wdClusteredDataExtractor::PostSortAndBatch(
  const wdView& view, const wdDynamicArray<const wdGameObject*>& visibleObjects, wdExtractedRenderData& ref_extractedRenderData)
{
  WD_PROFILE_SCOPE("PostSortAndBatch");

  const wdCamera* pCamera = view.GetCullingCamera();
  const float fAspectRatio = view.GetViewport().width / view.GetViewport().height;

  FillClusterBoundingSpheres(*pCamera, fAspectRatio, m_ClusterBoundingSpheres);
  wdClusteredDataCPU* pData = WD_NEW(wdFrameAllocator::GetCurrentAllocator(), wdClusteredDataCPU);
  pData->m_ClusterData = WD_NEW_ARRAY(wdFrameAllocator::GetCurrentAllocator(), wdPerClusterData, NUM_CLUSTERS);

  wdMat4 tmp = pCamera->GetViewMatrix();
  wdSimdMat4f viewMatrix = wdSimdConversion::ToMat4(tmp);

  pCamera->GetProjectionMatrix(fAspectRatio, tmp);
  wdSimdMat4f projectionMatrix = wdSimdConversion::ToMat4(tmp);

  wdSimdMat4f viewProjectionMatrix = projectionMatrix * viewMatrix;

  // Lights
  {
    WD_PROFILE_SCOPE("Lights");
    m_TempLightData.Clear();
    wdMemoryUtils::ZeroFill(m_TempLightsClusters.GetData(), NUM_CLUSTERS);

    auto batchList = ref_extractedRenderData.GetRenderDataBatchesWithCategory(wdDefaultRenderDataCategories::Light);
    const wdUInt32 uiBatchCount = batchList.GetBatchCount();
    for (wdUInt32 i = 0; i < uiBatchCount; ++i)
    {
      const wdRenderDataBatch& batch = batchList.GetBatch(i);

      for (auto it = batch.GetIterator<wdRenderData>(); it.IsValid(); ++it)
      {
        const wdUInt32 uiLightIndex = m_TempLightData.GetCount();

        if (uiLightIndex == wdClusteredDataCPU::MAX_LIGHT_DATA)
        {
          wdLog::Warning("Maximum number of lights reached ({0}). Further lights will be discarded.", wdClusteredDataCPU::MAX_LIGHT_DATA);
          break;
        }

        if (auto pPointLightRenderData = wdDynamicCast<const wdPointLightRenderData*>(it))
        {
          FillPointLightData(m_TempLightData.ExpandAndGetRef(), pPointLightRenderData);

          wdSimdBSphere pointLightSphere =
            wdSimdBSphere(wdSimdConversion::ToVec3(pPointLightRenderData->m_GlobalTransform.m_vPosition), pPointLightRenderData->m_fRange);
          RasterizeSphere(
            pointLightSphere, uiLightIndex, viewMatrix, projectionMatrix, m_TempLightsClusters.GetData(), m_ClusterBoundingSpheres.GetData());

          if (false)
          {
            wdSimdBBox ssb = GetScreenSpaceBounds(pointLightSphere, viewMatrix, projectionMatrix);
            float minX = ((float)ssb.m_Min.x() * 0.5f + 0.5f) * view.GetViewport().width;
            float maxX = ((float)ssb.m_Max.x() * 0.5f + 0.5f) * view.GetViewport().width;
            float minY = ((float)ssb.m_Max.y() * -0.5f + 0.5f) * view.GetViewport().height;
            float maxY = ((float)ssb.m_Min.y() * -0.5f + 0.5f) * view.GetViewport().height;

            wdRectFloat rect(minX, minY, maxX - minX, maxY - minY);
            wdDebugRenderer::Draw2DRectangle(view.GetHandle(), rect, 0.0f, wdColor::Blue.WithAlpha(0.3f));
          }
        }
        else if (auto pSpotLightRenderData = wdDynamicCast<const wdSpotLightRenderData*>(it))
        {
          FillSpotLightData(m_TempLightData.ExpandAndGetRef(), pSpotLightRenderData);

          wdAngle halfAngle = pSpotLightRenderData->m_OuterSpotAngle / 2.0f;

          BoundingCone cone;
          cone.m_PositionAndRange = wdSimdConversion::ToVec3(pSpotLightRenderData->m_GlobalTransform.m_vPosition);
          cone.m_PositionAndRange.SetW(pSpotLightRenderData->m_fRange);
          cone.m_ForwardDir = wdSimdConversion::ToVec3(pSpotLightRenderData->m_GlobalTransform.m_qRotation * wdVec3(1.0f, 0.0f, 0.0f));
          cone.m_SinCosAngle = wdSimdVec4f(wdMath::Sin(halfAngle), wdMath::Cos(halfAngle), 0.0f);
          RasterizeSpotLight(cone, uiLightIndex, viewMatrix, projectionMatrix, m_TempLightsClusters.GetData(), m_ClusterBoundingSpheres.GetData());
        }
        else if (auto pDirLightRenderData = wdDynamicCast<const wdDirectionalLightRenderData*>(it))
        {
          FillDirLightData(m_TempLightData.ExpandAndGetRef(), pDirLightRenderData);

          RasterizeDirLight(pDirLightRenderData, uiLightIndex, m_TempLightsClusters.GetArrayPtr());
        }
        else if (auto pFogRenderData = wdDynamicCast<const wdFogRenderData*>(it))
        {
          float fogBaseHeight = pFogRenderData->m_GlobalTransform.m_vPosition.z;
          float fogHeightFalloff = pFogRenderData->m_fHeightFalloff > 0.0f ? wdMath::Ln(0.0001f) / pFogRenderData->m_fHeightFalloff : 0.0f;

          float fogAtCameraPos = fogHeightFalloff * (pCamera->GetPosition().z - fogBaseHeight);
          if (fogAtCameraPos >= 80.0f) // Prevent infs
          {
            fogHeightFalloff = 0.0f;
          }

          pData->m_fFogHeight = -fogHeightFalloff * fogBaseHeight;
          pData->m_fFogHeightFalloff = fogHeightFalloff;
          pData->m_fFogDensityAtCameraPos = wdMath::Exp(wdMath::Clamp(fogAtCameraPos, -80.0f, 80.0f)); // Prevent infs
          pData->m_fFogDensity = pFogRenderData->m_fDensity;
          pData->m_fFogInvSkyDistance = pFogRenderData->m_fInvSkyDistance;

          pData->m_FogColor = pFogRenderData->m_Color;
        }
        else
        {
          WD_ASSERT_NOT_IMPLEMENTED;
        }
      }
    }

    pData->m_LightData = WD_NEW_ARRAY(wdFrameAllocator::GetCurrentAllocator(), wdPerLightData, m_TempLightData.GetCount());
    pData->m_LightData.CopyFrom(m_TempLightData);

    pData->m_uiSkyIrradianceIndex = view.GetWorld()->GetIndex();
    pData->m_cameraUsageHint = view.GetCameraUsageHint();
  }

  // Decals
  {
    WD_PROFILE_SCOPE("Decals");
    m_TempDecalData.Clear();
    wdMemoryUtils::ZeroFill(m_TempDecalsClusters.GetData(), NUM_CLUSTERS);

    auto batchList = ref_extractedRenderData.GetRenderDataBatchesWithCategory(wdDefaultRenderDataCategories::Decal);
    const wdUInt32 uiBatchCount = batchList.GetBatchCount();
    for (wdUInt32 i = 0; i < uiBatchCount; ++i)
    {
      const wdRenderDataBatch& batch = batchList.GetBatch(i);

      for (auto it = batch.GetIterator<wdRenderData>(); it.IsValid(); ++it)
      {
        const wdUInt32 uiDecalIndex = m_TempDecalData.GetCount();

        if (uiDecalIndex == wdClusteredDataCPU::MAX_DECAL_DATA)
        {
          wdLog::Warning("Maximum number of decals reached ({0}). Further decals will be discarded.", wdClusteredDataCPU::MAX_DECAL_DATA);
          break;
        }

        if (auto pDecalRenderData = wdDynamicCast<const wdDecalRenderData*>(it))
        {
          FillDecalData(m_TempDecalData.ExpandAndGetRef(), pDecalRenderData);

          RasterizeBox(pDecalRenderData->m_GlobalTransform, uiDecalIndex, viewProjectionMatrix, m_TempDecalsClusters.GetData(), m_ClusterBoundingSpheres.GetData());
        }
        else
        {
          WD_ASSERT_NOT_IMPLEMENTED;
        }
      }
    }

    pData->m_DecalData = WD_NEW_ARRAY(wdFrameAllocator::GetCurrentAllocator(), wdPerDecalData, m_TempDecalData.GetCount());
    pData->m_DecalData.CopyFrom(m_TempDecalData);
  }

  // Reflection Probes
  {
    WD_PROFILE_SCOPE("Probes");
    m_TempReflectionProbeData.Clear();
    wdMemoryUtils::ZeroFill(m_TempReflectionProbeClusters.GetData(), NUM_CLUSTERS);

    auto batchList = ref_extractedRenderData.GetRenderDataBatchesWithCategory(wdDefaultRenderDataCategories::ReflectionProbe);
    const wdUInt32 uiBatchCount = batchList.GetBatchCount();
    for (wdUInt32 i = 0; i < uiBatchCount; ++i)
    {
      const wdRenderDataBatch& batch = batchList.GetBatch(i);

      for (auto it = batch.GetIterator<wdRenderData>(); it.IsValid(); ++it)
      {
        const wdUInt32 uiProbeIndex = m_TempReflectionProbeData.GetCount();

        if (uiProbeIndex == wdClusteredDataCPU::MAX_REFLECTION_PROBE_DATA)
        {
          wdLog::Warning("Maximum number of reflection probes reached ({0}). Further reflection probes will be discarded.", wdClusteredDataCPU::MAX_REFLECTION_PROBE_DATA);
          break;
        }

        if (auto pReflectionProbeRenderData = wdDynamicCast<const wdReflectionProbeRenderData*>(it))
        {
          auto& probeData = m_TempReflectionProbeData.ExpandAndGetRef();
          FillReflectionProbeData(probeData, pReflectionProbeRenderData);

          const wdVec3 vFullScale = pReflectionProbeRenderData->m_vHalfExtents.CompMul(pReflectionProbeRenderData->m_GlobalTransform.m_vScale);

          bool bRasterizeSphere = false;
          float fMaxRadius = 0.0f;
          if (pReflectionProbeRenderData->m_uiIndex & REFLECTION_PROBE_IS_SPHERE)
          {
            constexpr float fSphereConstant = (4.0f / 3.0f) * wdMath::Pi<float>();
            fMaxRadius = wdMath::Max(wdMath::Max(wdMath::Abs(vFullScale.x), wdMath::Abs(vFullScale.y)), wdMath::Abs(vFullScale.z));
            const float fSphereVolume = fSphereConstant * wdMath::Pow(fMaxRadius, 3.0f);
            const float fBoxVolume = wdMath::Abs(vFullScale.x * vFullScale.y * vFullScale.z * 8);
            if (fSphereVolume < fBoxVolume)
            {
              bRasterizeSphere = true;
            }
          }

          if (bRasterizeSphere)
          {
            wdSimdBSphere pointLightSphere =
              wdSimdBSphere(wdSimdConversion::ToVec3(pReflectionProbeRenderData->m_GlobalTransform.m_vPosition), fMaxRadius);
            RasterizeSphere(
              pointLightSphere, uiProbeIndex, viewMatrix, projectionMatrix, m_TempReflectionProbeClusters.GetData(), m_ClusterBoundingSpheres.GetData());
          }
          else
          {
            wdTransform transform = pReflectionProbeRenderData->m_GlobalTransform;
            transform.m_vScale = vFullScale.CompMul(probeData.InfluenceScale.GetAsVec3());
            transform.m_vPosition += transform.m_qRotation * vFullScale.CompMul(probeData.InfluenceShift.GetAsVec3());

            //const wdBoundingBox aabb(wdVec3(-1.0f), wdVec3(1.0f));
            //wdDebugRenderer::DrawLineBox(view.GetHandle(), aabb, wdColor::DarkBlue, transform);

            RasterizeBox(transform, uiProbeIndex, viewProjectionMatrix, m_TempReflectionProbeClusters.GetData(), m_ClusterBoundingSpheres.GetData());
          }
        }
        else
        {
          WD_ASSERT_NOT_IMPLEMENTED;
        }
      }
    }

    pData->m_ReflectionProbeData = WD_NEW_ARRAY(wdFrameAllocator::GetCurrentAllocator(), wdPerReflectionProbeData, m_TempReflectionProbeData.GetCount());
    pData->m_ReflectionProbeData.CopyFrom(m_TempReflectionProbeData);
  }

  FillItemListAndClusterData(pData);

  ref_extractedRenderData.AddFrameData(pData);

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  VisualizeClusteredData(view, pData, m_ClusterBoundingSpheres);
#endif
}

namespace
{
  wdUInt32 PackIndex(wdUInt32 uiLightIndex, wdUInt32 uiDecalIndex) { return uiDecalIndex << 10 | uiLightIndex; }

  wdUInt32 PackReflectionProbeIndex(wdUInt32 uiData, wdUInt32 uiReflectionProbeIndex) { return uiReflectionProbeIndex << 20 | uiData; }
} // namespace

void wdClusteredDataExtractor::FillItemListAndClusterData(wdClusteredDataCPU* pData)
{
  WD_PROFILE_SCOPE("FillItemListAndClusterData");
  m_TempClusterItemList.Clear();

  const wdUInt32 uiNumLights = m_TempLightData.GetCount();
  const wdUInt32 uiMaxLightBlockIndex = (uiNumLights + 31) / 32;

  const wdUInt32 uiNumDecals = m_TempDecalData.GetCount();
  const wdUInt32 uiMaxDecalBlockIndex = (uiNumDecals + 31) / 32;

  const wdUInt32 uiNumReflectionProbes = m_TempReflectionProbeData.GetCount();
  const wdUInt32 uiMaxReflectionProbeBlockIndex = (uiNumReflectionProbes + 31) / 32;

  const wdUInt32 uiWorstCase = wdMath::Max(uiNumLights, uiNumDecals, uiNumReflectionProbes);
  for (wdUInt32 i = 0; i < NUM_CLUSTERS; ++i)
  {
    const wdUInt32 uiOffset = m_TempClusterItemList.GetCount();
    wdUInt32 uiLightCount = 0;

    // We expand m_TempClusterItemList by the worst case this loop can produce and then cut it down again to the actual size once we have filled the data. This makes sure we do not waste time on boundary checks or potential out of line calls like PushBack or PushBackUnchecked.
    m_TempClusterItemList.SetCountUninitialized(uiOffset + uiWorstCase);
    wdUInt32* pTempClusterItemListRange = m_TempClusterItemList.GetData() + uiOffset;

    // Lights
    {
      auto& tempCluster = m_TempLightsClusters[i];
      for (wdUInt32 uiBlockIndex = 0; uiBlockIndex < uiMaxLightBlockIndex; ++uiBlockIndex)
      {
        wdUInt32 mask = tempCluster.m_BitMask[uiBlockIndex];

        while (mask > 0)
        {
          wdUInt32 uiLightIndex = wdMath::FirstBitLow(mask);
          mask &= ~(1 << uiLightIndex);

          uiLightIndex += uiBlockIndex * 32;
          pTempClusterItemListRange[uiLightCount] = uiLightIndex;
          ++uiLightCount;
        }
      }
    }

    wdUInt32 uiDecalCount = 0;

    // Decals
    {
      auto& tempCluster = m_TempDecalsClusters[i];
      for (wdUInt32 uiBlockIndex = 0; uiBlockIndex < uiMaxDecalBlockIndex; ++uiBlockIndex)
      {
        wdUInt32 mask = tempCluster.m_BitMask[uiBlockIndex];

        while (mask > 0)
        {
          wdUInt32 uiDecalIndex = wdMath::FirstBitLow(mask);
          mask &= ~(1 << uiDecalIndex);

          uiDecalIndex += uiBlockIndex * 32;

          if (uiDecalCount < uiLightCount)
          {
            auto& item = pTempClusterItemListRange[uiDecalCount];
            item = PackIndex(item, uiDecalIndex);
          }
          else
          {
            pTempClusterItemListRange[uiDecalCount] = PackIndex(0, uiDecalIndex);
          }

          ++uiDecalCount;
        }
      }
    }

    wdUInt32 uiReflectionProbeCount = 0;
    const wdUInt32 uiMaxUsed = wdMath::Max(uiLightCount, uiDecalCount);
    // Reflection Probes
    {
      auto& tempCluster = m_TempReflectionProbeClusters[i];
      for (wdUInt32 uiBlockIndex = 0; uiBlockIndex < uiMaxReflectionProbeBlockIndex; ++uiBlockIndex)
      {
        wdUInt32 mask = tempCluster.m_BitMask[uiBlockIndex];

        while (mask > 0)
        {
          wdUInt32 uiReflectionProbeIndex = wdMath::FirstBitLow(mask);
          mask &= ~(1 << uiReflectionProbeIndex);

          uiReflectionProbeIndex += uiBlockIndex * 32;

          if (uiReflectionProbeCount < uiMaxUsed)
          {
            auto& item = pTempClusterItemListRange[uiReflectionProbeCount];
            item = PackReflectionProbeIndex(item, uiReflectionProbeIndex);
          }
          else
          {
            pTempClusterItemListRange[uiReflectionProbeCount] = PackReflectionProbeIndex(0, uiReflectionProbeIndex);
          }

          ++uiReflectionProbeCount;
        }
      }
    }

    // Cut down the array to the actual number of elements we have written.
    const wdUInt32 uiActualCase = wdMath::Max(uiLightCount, uiDecalCount, uiReflectionProbeCount);
    m_TempClusterItemList.SetCountUninitialized(uiOffset + uiActualCase);

    auto& clusterData = pData->m_ClusterData[i];
    clusterData.offset = uiOffset;
    clusterData.counts = PackReflectionProbeIndex(PackIndex(uiLightCount, uiDecalCount), uiReflectionProbeCount);
  }

  pData->m_ClusterItemList = WD_NEW_ARRAY(wdFrameAllocator::GetCurrentAllocator(), wdUInt32, m_TempClusterItemList.GetCount());
  pData->m_ClusterItemList.CopyFrom(m_TempClusterItemList);
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ClusteredDataExtractor);
