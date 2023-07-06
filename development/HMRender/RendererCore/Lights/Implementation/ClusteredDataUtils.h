#pragma once

#include <RendererCore/Decals/DecalComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightData.h>
WD_DEFINE_AS_POD_TYPE(wdPerLightData);
WD_DEFINE_AS_POD_TYPE(wdPerDecalData);
WD_DEFINE_AS_POD_TYPE(wdPerReflectionProbeData);
WD_DEFINE_AS_POD_TYPE(wdPerClusterData);

#include <Core/Graphics/Camera.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <Foundation/Utilities/GraphicsUtils.h>

namespace
{
  ///\todo Make this configurable.
  static float s_fMinLightDistance = 5.0f;
  static float s_fMaxLightDistance = 500.0f;

  static float s_fDepthSliceScale = (NUM_CLUSTERS_Z - 1) / (wdMath::Log2(s_fMaxLightDistance) - wdMath::Log2(s_fMinLightDistance));
  static float s_fDepthSliceBias = -s_fDepthSliceScale * wdMath::Log2(s_fMinLightDistance) + 1.0f;

  WD_ALWAYS_INLINE float GetDepthFromSliceIndex(wdUInt32 uiSliceIndex)
  {
    return wdMath::Pow(2.0f, (uiSliceIndex - s_fDepthSliceBias + 1.0f) / s_fDepthSliceScale);
  }

  WD_ALWAYS_INLINE wdUInt32 GetSliceIndexFromDepth(float fLinearDepth)
  {
    return wdMath::Clamp((wdInt32)(wdMath::Log2(fLinearDepth) * s_fDepthSliceScale + s_fDepthSliceBias), 0, NUM_CLUSTERS_Z - 1);
  }

  WD_ALWAYS_INLINE wdUInt32 GetClusterIndexFromCoord(wdUInt32 x, wdUInt32 y, wdUInt32 z) { return z * NUM_CLUSTERS_XY + y * NUM_CLUSTERS_X + x; }

  // in order: tlf, trf, blf, brf, tln, trn, bln, brn
  WD_FORCE_INLINE void GetClusterCornerPoints(
    const wdCamera& camera, float fZf, float fZn, float fTanLeft, float fTanRight, float fTanBottom, float fTanTop, wdInt32 x, wdInt32 y, wdInt32 z, wdVec3* out_pCorners)
  {
    const wdVec3& pos = camera.GetPosition();
    const wdVec3& dirForward = camera.GetDirForwards();
    const wdVec3& dirRight = camera.GetDirRight();
    const wdVec3& dirUp = camera.GetDirUp();

    const float fStartXf = fZf * fTanLeft;
    const float fStartYf = fZf * fTanBottom;
    const float fEndXf = fZf * fTanRight;
    const float fEndYf = fZf * fTanTop;

    float fStepXf = (fEndXf - fStartXf) / NUM_CLUSTERS_X;
    float fStepYf = (fEndYf - fStartYf) / NUM_CLUSTERS_Y;

    float fXf = fStartXf + x * fStepXf;
    float fYf = fStartYf + y * fStepYf;

    out_pCorners[0] = pos + dirForward * fZf + dirRight * fXf - dirUp * fYf;
    out_pCorners[1] = out_pCorners[0] + dirRight * fStepXf;
    out_pCorners[2] = out_pCorners[0] - dirUp * fStepYf;
    out_pCorners[3] = out_pCorners[2] + dirRight * fStepXf;

    const float fStartXn = fZn * fTanLeft;
    const float fStartYn = fZn * fTanBottom;
    const float fEndXn = fZn * fTanRight;
    const float fEndYn = fZn * fTanTop;

    float fStepXn = (fEndXn - fStartXn) / NUM_CLUSTERS_X;
    float fStepYn = (fEndYn - fStartYn) / NUM_CLUSTERS_Y;
    float fXn = fStartXn + x * fStepXn;
    float fYn = fStartYn + y * fStepYn;

    out_pCorners[4] = pos + dirForward * fZn + dirRight * fXn - dirUp * fYn;
    out_pCorners[5] = out_pCorners[4] + dirRight * fStepXn;
    out_pCorners[6] = out_pCorners[4] - dirUp * fStepYn;
    out_pCorners[7] = out_pCorners[6] + dirRight * fStepXn;
  }

  void FillClusterBoundingSpheres(const wdCamera& camera, float fAspectRatio, wdArrayPtr<wdSimdBSphere> clusterBoundingSpheres)
  {
    WD_PROFILE_SCOPE("FillClusterBoundingSpheres");

    ///\todo proper implementation for orthographic views
    if (camera.IsOrthographic())
      return;

    wdMat4 mProj;
    camera.GetProjectionMatrix(fAspectRatio, mProj);

    wdSimdVec4f stepScale;
    wdSimdVec4f tanLBLB;
    {
      wdAngle fFovLeft;
      wdAngle fFovRight;
      wdAngle fFovBottom;
      wdAngle fFovTop;
      wdGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fFovLeft, fFovRight, fFovBottom, fFovTop);

      const float fTanLeft = wdMath::Tan(fFovLeft);
      const float fTanRight = wdMath::Tan(fFovRight);
      const float fTanBottom = wdMath::Tan(fFovBottom);
      const float fTanTop = wdMath::Tan(fFovTop);

      float fStepXf = (fTanRight - fTanLeft) / NUM_CLUSTERS_X;
      float fStepYf = (fTanTop - fTanBottom) / NUM_CLUSTERS_Y;

      stepScale = wdSimdVec4f(fStepXf, fStepYf, fStepXf, fStepYf);
      tanLBLB = wdSimdVec4f(fTanLeft, fTanBottom, fTanLeft, fTanBottom);
    }

    wdSimdVec4f pos = wdSimdConversion::ToVec3(camera.GetPosition());
    wdSimdVec4f dirForward = wdSimdConversion::ToVec3(camera.GetDirForwards());
    wdSimdVec4f dirRight = wdSimdConversion::ToVec3(camera.GetDirRight());
    wdSimdVec4f dirUp = wdSimdConversion::ToVec3(camera.GetDirUp());


    wdSimdVec4f fZn = wdSimdVec4f::ZeroVector();
    wdSimdVec4f cc[8];

    for (wdInt32 z = 0; z < NUM_CLUSTERS_Z; z++)
    {
      wdSimdVec4f fZf = wdSimdVec4f(GetDepthFromSliceIndex(z));
      wdSimdVec4f zff_znn = fZf.GetCombined<wdSwizzle::XXXX>(fZn);
      wdSimdVec4f steps = zff_znn.CompMul(stepScale);

      wdSimdVec4f depthF = pos + dirForward * fZf.x();
      wdSimdVec4f depthN = pos + dirForward * fZn.x();

      wdSimdVec4f startLBLB = zff_znn.CompMul(tanLBLB);

      for (wdInt32 y = 0; y < NUM_CLUSTERS_Y; y++)
      {
        for (wdInt32 x = 0; x < NUM_CLUSTERS_X; x++)
        {
          wdSimdVec4f xyxy = wdSimdVec4i(x, y, x, y).ToFloat();
          wdSimdVec4f xfyf = startLBLB + (xyxy).CompMul(steps);

          cc[0] = depthF + dirRight * xfyf.x() - dirUp * xfyf.y();
          cc[1] = cc[0] + dirRight * steps.x();
          cc[2] = cc[0] - dirUp * steps.y();
          cc[3] = cc[2] + dirRight * steps.x();

          cc[4] = depthN + dirRight * xfyf.z() - dirUp * xfyf.w();
          cc[5] = cc[4] + dirRight * steps.z();
          cc[6] = cc[4] - dirUp * steps.w();
          cc[7] = cc[6] + dirRight * steps.z();

          wdSimdBSphere s;
          s.SetFromPoints(cc, 8);

          clusterBoundingSpheres[GetClusterIndexFromCoord(x, y, z)] = s;
        }
      }

      fZn = fZf;
    }
  }

  WD_ALWAYS_INLINE void FillLightData(wdPerLightData& ref_perLightData, const wdLightRenderData* pLightRenderData, wdUInt8 uiType)
  {
    wdMemoryUtils::ZeroFill(&ref_perLightData, 1);

    wdColorLinearUB lightColor = pLightRenderData->m_LightColor;
    lightColor.a = uiType;

    ref_perLightData.colorAndType = *reinterpret_cast<wdUInt32*>(&lightColor.r);
    ref_perLightData.intensity = pLightRenderData->m_fIntensity;
    ref_perLightData.shadowDataOffset = pLightRenderData->m_uiShadowDataOffset;
  }

  void FillPointLightData(wdPerLightData& ref_perLightData, const wdPointLightRenderData* pPointLightRenderData)
  {
    FillLightData(ref_perLightData, pPointLightRenderData, LIGHT_TYPE_POINT);

    ref_perLightData.position = pPointLightRenderData->m_GlobalTransform.m_vPosition;
    ref_perLightData.invSqrAttRadius = 1.0f / (pPointLightRenderData->m_fRange * pPointLightRenderData->m_fRange);
  }

  void FillSpotLightData(wdPerLightData& ref_perLightData, const wdSpotLightRenderData* pSpotLightRenderData)
  {
    FillLightData(ref_perLightData, pSpotLightRenderData, LIGHT_TYPE_SPOT);

    ref_perLightData.direction = wdShaderUtils::Float3ToRGB10(pSpotLightRenderData->m_GlobalTransform.m_qRotation * wdVec3(-1, 0, 0));
    ref_perLightData.position = pSpotLightRenderData->m_GlobalTransform.m_vPosition;
    ref_perLightData.invSqrAttRadius = 1.0f / (pSpotLightRenderData->m_fRange * pSpotLightRenderData->m_fRange);

    const float fCosInner = wdMath::Cos(pSpotLightRenderData->m_InnerSpotAngle * 0.5f);
    const float fCosOuter = wdMath::Cos(pSpotLightRenderData->m_OuterSpotAngle * 0.5f);
    const float fSpotParamScale = 1.0f / wdMath::Max(0.001f, (fCosInner - fCosOuter));
    const float fSpotParamOffset = -fCosOuter * fSpotParamScale;
    ref_perLightData.spotParams = wdShaderUtils::Float2ToRG16F(wdVec2(fSpotParamScale, fSpotParamOffset));
  }

  void FillDirLightData(wdPerLightData& ref_perLightData, const wdDirectionalLightRenderData* pDirLightRenderData)
  {
    FillLightData(ref_perLightData, pDirLightRenderData, LIGHT_TYPE_DIR);

    ref_perLightData.direction = wdShaderUtils::Float3ToRGB10(pDirLightRenderData->m_GlobalTransform.m_qRotation * wdVec3(-1, 0, 0));
  }

  void FillDecalData(wdPerDecalData& ref_perDecalData, const wdDecalRenderData* pDecalRenderData)
  {
    wdVec3 position = pDecalRenderData->m_GlobalTransform.m_vPosition;
    wdVec3 dirForwards = pDecalRenderData->m_GlobalTransform.m_qRotation * wdVec3(1.0f, 0.0, 0.0f);
    wdVec3 dirUp = pDecalRenderData->m_GlobalTransform.m_qRotation * wdVec3(0.0f, 0.0, 1.0f);
    wdVec3 scale = pDecalRenderData->m_GlobalTransform.m_vScale;

    // the CompMax prevents division by zero (thus inf, thus NaN later, then crash)
    // if negative scaling should be allowed, this would need to be changed
    scale = wdVec3(1.0f).CompDiv(scale.CompMax(wdVec3(0.00001f)));

    const wdMat4 lookAt = wdGraphicsUtils::CreateLookAtViewMatrix(position, position + dirForwards, dirUp);
    wdMat4 scaleMat;
    scaleMat.SetScalingMatrix(wdVec3(scale.y, -scale.z, scale.x));

    ref_perDecalData.worldToDecalMatrix = scaleMat * lookAt;
    ref_perDecalData.applyOnlyToId = pDecalRenderData->m_uiApplyOnlyToId;
    ref_perDecalData.decalFlags = pDecalRenderData->m_uiFlags;
    ref_perDecalData.angleFadeParams = pDecalRenderData->m_uiAngleFadeParams;
    ref_perDecalData.baseColor = *reinterpret_cast<const wdUInt32*>(&pDecalRenderData->m_BaseColor.r);
    ref_perDecalData.emissiveColorRG = wdShaderUtils::PackFloat16intoUint(pDecalRenderData->m_EmissiveColor.r, pDecalRenderData->m_EmissiveColor.g);
    ref_perDecalData.emissiveColorBA = wdShaderUtils::PackFloat16intoUint(pDecalRenderData->m_EmissiveColor.b, pDecalRenderData->m_EmissiveColor.a);
    ref_perDecalData.baseColorAtlasScale = pDecalRenderData->m_uiBaseColorAtlasScale;
    ref_perDecalData.baseColorAtlasOffset = pDecalRenderData->m_uiBaseColorAtlasOffset;
    ref_perDecalData.normalAtlasScale = pDecalRenderData->m_uiNormalAtlasScale;
    ref_perDecalData.normalAtlasOffset = pDecalRenderData->m_uiNormalAtlasOffset;
    ref_perDecalData.ormAtlasScale = pDecalRenderData->m_uiORMAtlasScale;
    ref_perDecalData.ormAtlasOffset = pDecalRenderData->m_uiORMAtlasOffset;
  }

  void FillReflectionProbeData(wdPerReflectionProbeData& ref_perReflectionProbeData, const wdReflectionProbeRenderData* pReflectionProbeRenderData)
  {
    wdVec3 position = pReflectionProbeRenderData->m_GlobalTransform.m_vPosition;
    wdVec3 scale = pReflectionProbeRenderData->m_GlobalTransform.m_vScale.CompMul(pReflectionProbeRenderData->m_vHalfExtents);

    // We store scale separately so we easily transform into probe projection space (with scale), influence space (scale + offset) and cube map space (no scale).
    auto trans = pReflectionProbeRenderData->m_GlobalTransform;
    trans.m_vScale = wdVec3(1.0f, 1.0f, 1.0f);
    auto inverse = trans.GetAsMat4().GetInverse();

    // the CompMax prevents division by zero (thus inf, thus NaN later, then crash)
    // if negative scaling should be allowed, this would need to be changed
    scale = wdVec3(1.0f).CompDiv(scale.CompMax(wdVec3(0.00001f)));
    ref_perReflectionProbeData.WorldToProbeProjectionMatrix = inverse;

    ref_perReflectionProbeData.ProbePosition = pReflectionProbeRenderData->m_vProbePosition.GetAsVec4(1.0f); // W isn't used.
    ref_perReflectionProbeData.Scale = scale.GetAsVec4(0.0f);                                                // W isn't used.

    ref_perReflectionProbeData.InfluenceScale = pReflectionProbeRenderData->m_vInfluenceScale.GetAsVec4(0.0f);
    ref_perReflectionProbeData.InfluenceShift = pReflectionProbeRenderData->m_vInfluenceShift.CompMul(wdVec3(1.0f) - pReflectionProbeRenderData->m_vInfluenceScale).GetAsVec4(0.0f);

    ref_perReflectionProbeData.PositiveFalloff = pReflectionProbeRenderData->m_vPositiveFalloff.GetAsVec4(0.0f);
    ref_perReflectionProbeData.NegativeFalloff = pReflectionProbeRenderData->m_vNegativeFalloff.GetAsVec4(0.0f);
    ref_perReflectionProbeData.Index = pReflectionProbeRenderData->m_uiIndex;
  }


  WD_FORCE_INLINE wdSimdBBox GetScreenSpaceBounds(const wdSimdBSphere& sphere, const wdSimdMat4f& mViewMatrix, const wdSimdMat4f& mProjectionMatrix)
  {
    wdSimdVec4f viewSpaceCenter = mViewMatrix.TransformPosition(sphere.GetCenter());
    wdSimdFloat depth = viewSpaceCenter.z();
    wdSimdFloat radius = sphere.GetRadius();

    wdSimdVec4f mi;
    wdSimdVec4f ma;

    if (viewSpaceCenter.GetLength<3>() > radius && depth > radius)
    {
      wdSimdVec4f one = wdSimdVec4f(1.0f);
      wdSimdVec4f oneNegOne = wdSimdVec4f(1.0f, -1.0f, 1.0f, -1.0f);

      wdSimdVec4f pRadius = wdSimdVec4f(radius / depth);
      wdSimdVec4f pRadius2 = pRadius.CompMul(pRadius);

      wdSimdVec4f xy = viewSpaceCenter / depth;
      wdSimdVec4f xxyy = xy.Get<wdSwizzle::XXYY>();
      wdSimdVec4f nom = (pRadius2.CompMul(xxyy.CompMul(xxyy) - pRadius2 + one)).GetSqrt() - xxyy.CompMul(oneNegOne);
      wdSimdVec4f denom = pRadius2 - one;

      wdSimdVec4f projection = mProjectionMatrix.m_col0.GetCombined<wdSwizzle::XXYY>(mProjectionMatrix.m_col1);
      wdSimdVec4f minXmaxX_minYmaxY = nom.CompDiv(denom).CompMul(oneNegOne).CompMul(projection);

      mi = minXmaxX_minYmaxY.Get<wdSwizzle::XZXX>();
      ma = minXmaxX_minYmaxY.Get<wdSwizzle::YWYY>();
    }
    else
    {
      mi = wdSimdVec4f(-1.0f);
      ma = wdSimdVec4f(1.0f);
    }

    mi.SetZ(depth - radius);
    ma.SetZ(depth + radius);

    return wdSimdBBox(mi, ma);
  }

  template <typename Cluster, typename IntersectionFunc>
  WD_FORCE_INLINE void FillCluster(
    const wdSimdBBox& screenSpaceBounds, wdUInt32 uiBlockIndex, wdUInt32 uiMask, Cluster* pClusters, IntersectionFunc func)
  {
    wdSimdVec4f scale = wdSimdVec4f(0.5f * NUM_CLUSTERS_X, -0.5f * NUM_CLUSTERS_Y, 1.0f, 1.0f);
    wdSimdVec4f bias = wdSimdVec4f(0.5f * NUM_CLUSTERS_X, 0.5f * NUM_CLUSTERS_Y, 0.0f, 0.0f);

    wdSimdVec4f mi = wdSimdVec4f::MulAdd(screenSpaceBounds.m_Min, scale, bias);
    wdSimdVec4f ma = wdSimdVec4f::MulAdd(screenSpaceBounds.m_Max, scale, bias);

    wdSimdVec4i minXY_maxXY = wdSimdVec4i::Truncate(mi.GetCombined<wdSwizzle::XYXY>(ma));

    wdSimdVec4i maxClusterIndex = wdSimdVec4i(NUM_CLUSTERS_X, NUM_CLUSTERS_Y, NUM_CLUSTERS_X, NUM_CLUSTERS_Y);
    minXY_maxXY = minXY_maxXY.CompMin(maxClusterIndex - wdSimdVec4i(1));
    minXY_maxXY = minXY_maxXY.CompMax(wdSimdVec4i::ZeroVector());

    wdUInt32 xMin = minXY_maxXY.x();
    wdUInt32 yMin = minXY_maxXY.w();

    wdUInt32 xMax = minXY_maxXY.z();
    wdUInt32 yMax = minXY_maxXY.y();

    wdUInt32 zMin = GetSliceIndexFromDepth(screenSpaceBounds.m_Min.z());
    wdUInt32 zMax = GetSliceIndexFromDepth(screenSpaceBounds.m_Max.z());

    for (wdUInt32 z = zMin; z <= zMax; ++z)
    {
      for (wdUInt32 y = yMin; y <= yMax; ++y)
      {
        for (wdUInt32 x = xMin; x <= xMax; ++x)
        {
          wdUInt32 uiClusterIndex = GetClusterIndexFromCoord(x, y, z);
          if (func(uiClusterIndex))
          {
            pClusters[uiClusterIndex].m_BitMask[uiBlockIndex] |= uiMask;
          }
        }
      }
    }
  }

  template <typename Cluster>
  void RasterizeSphere(const wdSimdBSphere& pointLightSphere, wdUInt32 uiLightIndex, const wdSimdMat4f& mViewMatrix,
    const wdSimdMat4f& mProjectionMatrix, Cluster* pClusters, wdSimdBSphere* pClusterBoundingSpheres)
  {
    wdSimdBBox screenSpaceBounds = GetScreenSpaceBounds(pointLightSphere, mViewMatrix, mProjectionMatrix);

    const wdUInt32 uiBlockIndex = uiLightIndex / 32;
    const wdUInt32 uiMask = 1 << (uiLightIndex - uiBlockIndex * 32);

    FillCluster(screenSpaceBounds, uiBlockIndex, uiMask, pClusters,
      [&](wdUInt32 uiClusterIndex) { return pointLightSphere.Overlaps(pClusterBoundingSpheres[uiClusterIndex]); });
  }

  struct BoundingCone
  {
    wdSimdBSphere m_BoundingSphere;
    wdSimdVec4f m_PositionAndRange;
    wdSimdVec4f m_ForwardDir;
    wdSimdVec4f m_SinCosAngle;
  };

  template <typename Cluster>
  void RasterizeSpotLight(const BoundingCone& spotLightCone, wdUInt32 uiLightIndex, const wdSimdMat4f& mViewMatrix,
    const wdSimdMat4f& mProjectionMatrix, Cluster* pClusters, wdSimdBSphere* pClusterBoundingSpheres)
  {
    wdSimdVec4f position = spotLightCone.m_PositionAndRange;
    wdSimdFloat range = spotLightCone.m_PositionAndRange.w();
    wdSimdVec4f forwardDir = spotLightCone.m_ForwardDir;
    wdSimdFloat sinAngle = spotLightCone.m_SinCosAngle.x();
    wdSimdFloat cosAngle = spotLightCone.m_SinCosAngle.y();

    // First calculate a bounding sphere around the cone to get min and max bounds
    wdSimdVec4f bSphereCenter;
    wdSimdFloat bSphereRadius;
    if (sinAngle > 0.707107f) // sin(45)
    {
      bSphereCenter = position + forwardDir * cosAngle * range;
      bSphereRadius = sinAngle * range;
    }
    else
    {
      bSphereRadius = range / (cosAngle + cosAngle);
      bSphereCenter = position + forwardDir * bSphereRadius;
    }

    wdSimdBSphere spotLightSphere(bSphereCenter, bSphereRadius);
    wdSimdBBox screenSpaceBounds = GetScreenSpaceBounds(spotLightSphere, mViewMatrix, mProjectionMatrix);

    const wdUInt32 uiBlockIndex = uiLightIndex / 32;
    const wdUInt32 uiMask = 1 << (uiLightIndex - uiBlockIndex * 32);

    FillCluster(screenSpaceBounds, uiBlockIndex, uiMask, pClusters, [&](wdUInt32 uiClusterIndex) {
      wdSimdBSphere clusterSphere = pClusterBoundingSpheres[uiClusterIndex];
      wdSimdFloat clusterRadius = clusterSphere.GetRadius();

      wdSimdVec4f toConePos = clusterSphere.m_CenterAndRadius - position;
      wdSimdFloat projected = forwardDir.Dot<3>(toConePos);
      wdSimdFloat distToConeSq = toConePos.Dot<3>(toConePos);
      wdSimdFloat distClosestP = cosAngle * (distToConeSq - projected * projected).GetSqrt() - projected * sinAngle;

      bool angleCull = distClosestP > clusterRadius;
      bool frontCull = projected > clusterRadius + range;
      bool backCull = projected < -clusterRadius;

      return !(angleCull || frontCull || backCull);
    });
  }

  template <typename Cluster>
  void RasterizeDirLight(const wdDirectionalLightRenderData* pDirLightRenderData, wdUInt32 uiLightIndex, wdArrayPtr<Cluster> clusters)
  {
    const wdUInt32 uiBlockIndex = uiLightIndex / 32;
    const wdUInt32 uiMask = 1 << (uiLightIndex - uiBlockIndex * 32);

    for (wdUInt32 i = 0; i < clusters.GetCount(); ++i)
    {
      clusters[i].m_BitMask[uiBlockIndex] |= uiMask;
    }
  }

  template <typename Cluster>
  void RasterizeBox(const wdTransform& transform, wdUInt32 uiDecalIndex, const wdSimdMat4f& mViewProjectionMatrix, Cluster* pClusters,
    wdSimdBSphere* pClusterBoundingSpheres)
  {
    wdSimdMat4f decalToWorld = wdSimdConversion::ToTransform(transform).GetAsMat4();
    wdSimdMat4f worldToDecal = decalToWorld.GetInverse();

    wdVec3 corners[8];
    wdBoundingBox(wdVec3(-1), wdVec3(1)).GetCorners(corners);

    wdSimdMat4f decalToScreen = mViewProjectionMatrix * decalToWorld;
    wdSimdBBox screenSpaceBounds;
    screenSpaceBounds.SetInvalid();
    bool bInsideBox = false;
    for (wdUInt32 i = 0; i < 8; ++i)
    {
      wdSimdVec4f corner = wdSimdConversion::ToVec3(corners[i]);
      wdSimdVec4f screenSpaceCorner = decalToScreen.TransformPosition(corner);
      wdSimdFloat depth = screenSpaceCorner.w();
      bInsideBox |= depth < wdSimdFloat::Zero();

      screenSpaceCorner /= depth;
      screenSpaceCorner = screenSpaceCorner.GetCombined<wdSwizzle::XYZW>(wdSimdVec4f(depth));

      screenSpaceBounds.m_Min = screenSpaceBounds.m_Min.CompMin(screenSpaceCorner);
      screenSpaceBounds.m_Max = screenSpaceBounds.m_Max.CompMax(screenSpaceCorner);
    }

    if (bInsideBox)
    {
      screenSpaceBounds.m_Min = wdSimdVec4f(-1.0f).GetCombined<wdSwizzle::XYZW>(screenSpaceBounds.m_Min);
      screenSpaceBounds.m_Max = wdSimdVec4f(1.0f).GetCombined<wdSwizzle::XYZW>(screenSpaceBounds.m_Max);
    }

    wdSimdVec4f decalHalfExtents = wdSimdVec4f(1.0f);
    wdSimdBBox localDecalBounds = wdSimdBBox(-decalHalfExtents, decalHalfExtents);

    const wdUInt32 uiBlockIndex = uiDecalIndex / 32;
    const wdUInt32 uiMask = 1 << (uiDecalIndex - uiBlockIndex * 32);

    FillCluster(screenSpaceBounds, uiBlockIndex, uiMask, pClusters, [&](wdUInt32 uiClusterIndex) {
      wdSimdBSphere clusterSphere = pClusterBoundingSpheres[uiClusterIndex];
      clusterSphere.Transform(worldToDecal);

      return localDecalBounds.Overlaps(clusterSphere);
    });
  }
} // namespace
