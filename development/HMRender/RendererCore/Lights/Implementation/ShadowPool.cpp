#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/Pass.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightData.h>

// clang-format off
WD_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, ShadowPool)
BEGIN_SUBSYSTEM_DEPENDENCIES
"Foundation",
"Core",
"RenderWorld"
END_SUBSYSTEM_DEPENDENCIES

ON_HIGHLEVELSYSTEMS_STARTUP
{
  wdShadowPool::OnEngineStartup();
}

ON_HIGHLEVELSYSTEMS_SHUTDOWN
{
  wdShadowPool::OnEngineShutdown();
}

WD_END_SUBSYSTEM_DECLARATION;
  // clang-format on

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
wdCVarBool cvar_RenderingShadowsShowPoolStats("Rendering.Shadows.ShowPoolStats", false, wdCVarFlags::Default, "Display same stats of the shadow pool");
#endif

static wdUInt32 s_uiShadowAtlasTextureWidth = 4096; ///\todo make this configurable
static wdUInt32 s_uiShadowAtlasTextureHeight = 4096;
static wdUInt32 s_uiShadowMapSize = 1024;
static wdUInt32 s_uiMinShadowMapSize = 64;
static float s_fFadeOutScaleStart = (s_uiMinShadowMapSize + 1.0f) / s_uiShadowMapSize;
static float s_fFadeOutScaleEnd = s_fFadeOutScaleStart * 0.5f;

struct ShadowView
{
  wdViewHandle m_hView;
  wdCamera m_Camera;
};

struct ShadowData
{
  wdHybridArray<wdViewHandle, 6> m_Views;
  wdUInt32 m_uiType;
  float m_fShadowMapScale;
  float m_fPenumbraSize;
  float m_fSlopeBias;
  float m_fConstantBias;
  float m_fFadeOutStart;
  float m_fMinRange;
  wdUInt32 m_uiPackedDataOffset; // in 16 bytes steps
};

struct LightAndRefView
{
  WD_DECLARE_POD_TYPE();

  const wdLightComponent* m_pLight;
  const wdView* m_pReferenceView;
};

struct SortedShadowData
{
  WD_DECLARE_POD_TYPE();

  wdUInt32 m_uiIndex;
  float m_fShadowMapScale;

  WD_ALWAYS_INLINE bool operator<(const SortedShadowData& other) const
  {
    if (m_fShadowMapScale > other.m_fShadowMapScale) // we want to sort descending (higher scale first)
      return true;

    return m_uiIndex < other.m_uiIndex;
  }
};

static wdDynamicArray<SortedShadowData> s_SortedShadowData;

struct AtlasCell
{
  WD_DECLARE_POD_TYPE();

  WD_ALWAYS_INLINE AtlasCell()
    : m_Rect(0, 0, 0, 0)
  {
    m_uiChildIndices[0] = m_uiChildIndices[1] = m_uiChildIndices[2] = m_uiChildIndices[3] = 0xFFFF;
    m_uiDataIndex = wdInvalidIndex;
  }

  WD_ALWAYS_INLINE bool IsLeaf() const
  {
    return m_uiChildIndices[0] == 0xFFFF && m_uiChildIndices[1] == 0xFFFF && m_uiChildIndices[2] == 0xFFFF && m_uiChildIndices[3] == 0xFFFF;
  }

  wdRectU32 m_Rect;
  wdUInt16 m_uiChildIndices[4];
  wdUInt32 m_uiDataIndex;
};

static wdDeque<AtlasCell> s_AtlasCells;

static AtlasCell* Insert(AtlasCell* pCell, wdUInt32 uiShadowMapSize, wdUInt32 uiDataIndex)
{
  if (!pCell->IsLeaf())
  {
    for (wdUInt32 i = 0; i < 4; ++i)
    {
      AtlasCell* pChildCell = &s_AtlasCells[pCell->m_uiChildIndices[i]];
      if (AtlasCell* pNewCell = Insert(pChildCell, uiShadowMapSize, uiDataIndex))
      {
        return pNewCell;
      }
    }

    return nullptr;
  }
  else
  {
    if (pCell->m_uiDataIndex != wdInvalidIndex)
      return nullptr;

    if (pCell->m_Rect.width < uiShadowMapSize || pCell->m_Rect.height < uiShadowMapSize)
      return nullptr;

    if (pCell->m_Rect.width == uiShadowMapSize && pCell->m_Rect.height == uiShadowMapSize)
    {
      pCell->m_uiDataIndex = uiDataIndex;
      return pCell;
    }

    // Split
    wdUInt32 x = pCell->m_Rect.x;
    wdUInt32 y = pCell->m_Rect.y;
    wdUInt32 w = pCell->m_Rect.width / 2;
    wdUInt32 h = pCell->m_Rect.height / 2;

    wdUInt32 uiCellIndex = s_AtlasCells.GetCount();
    s_AtlasCells.ExpandAndGetRef().m_Rect = wdRectU32(x, y, w, h);
    s_AtlasCells.ExpandAndGetRef().m_Rect = wdRectU32(x + w, y, w, h);
    s_AtlasCells.ExpandAndGetRef().m_Rect = wdRectU32(x, y + h, w, h);
    s_AtlasCells.ExpandAndGetRef().m_Rect = wdRectU32(x + w, y + h, w, h);

    for (wdUInt32 i = 0; i < 4; ++i)
    {
      pCell->m_uiChildIndices[i] = static_cast<wdUInt16>(uiCellIndex + i);
    }

    AtlasCell* pChildCell = &s_AtlasCells[pCell->m_uiChildIndices[0]];
    return Insert(pChildCell, uiShadowMapSize, uiDataIndex);
  }
}

static wdRectU32 FindAtlasRect(wdUInt32 uiShadowMapSize, wdUInt32 uiDataIndex)
{
  WD_ASSERT_DEBUG(wdMath::IsPowerOf2(uiShadowMapSize), "Size must be power of 2");

  AtlasCell* pCell = Insert(&s_AtlasCells[0], uiShadowMapSize, uiDataIndex);
  if (pCell != nullptr)
  {
    WD_ASSERT_DEBUG(pCell->IsLeaf() && pCell->m_uiDataIndex == uiDataIndex, "Implementation error");
    return pCell->m_Rect;
  }

  wdLog::Warning("Shadow Pool is full. Not enough space for a {0}x{0} shadow map. The light will have no shadow.", uiShadowMapSize);
  return wdRectU32(0, 0, 0, 0);
}

static float AddSafeBorder(wdAngle fov, float fPenumbraSize)
{
  float fHalfHeight = wdMath::Tan(fov * 0.5f);
  float fNewFov = wdMath::ATan(fHalfHeight + fPenumbraSize).GetDegree() * 2.0f;
  return fNewFov;
}

wdTagSet s_ExcludeTagsWhiteList;

static void CopyExcludeTagsOnWhiteList(const wdTagSet& referenceTags, wdTagSet& out_targetTags)
{
  out_targetTags.Clear();
  out_targetTags.SetByName("EditorHidden");

  for (auto& tag : referenceTags)
  {
    if (s_ExcludeTagsWhiteList.IsSet(tag))
    {
      out_targetTags.Set(tag);
    }
  }
}

// must not be in anonymous namespace
template <>
struct wdHashHelper<LightAndRefView>
{
  WD_ALWAYS_INLINE static wdUInt32 Hash(LightAndRefView value) { return wdHashingUtils::xxHash32(&value.m_pLight, sizeof(LightAndRefView)); }

  WD_ALWAYS_INLINE static bool Equal(const LightAndRefView& a, const LightAndRefView& b)
  {
    return a.m_pLight == b.m_pLight && a.m_pReferenceView == b.m_pReferenceView;
  }
};

//////////////////////////////////////////////////////////////////////////

struct wdShadowPool::Data
{
  Data() { Clear(); }

  ~Data()
  {
    for (auto& shadowView : m_ShadowViews)
    {
      wdRenderWorld::DeleteView(shadowView.m_hView);
    }

    wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();
    if (!m_hShadowAtlasTexture.IsInvalidated())
    {
      pDevice->DestroyTexture(m_hShadowAtlasTexture);
      m_hShadowAtlasTexture.Invalidate();
    }

    if (!m_hShadowDataBuffer.IsInvalidated())
    {
      pDevice->DestroyBuffer(m_hShadowDataBuffer);
      m_hShadowDataBuffer.Invalidate();
    }
  }

  enum
  {
    MAX_SHADOW_DATA = 1024
  };

  void CreateShadowAtlasTexture()
  {
    if (m_hShadowAtlasTexture.IsInvalidated())
    {
      wdGALTextureCreationDescription desc;
      desc.SetAsRenderTarget(s_uiShadowAtlasTextureWidth, s_uiShadowAtlasTextureHeight, wdGALResourceFormat::D16);

      m_hShadowAtlasTexture = wdGALDevice::GetDefaultDevice()->CreateTexture(desc);
    }
  }

  void CreateShadowDataBuffer()
  {
    if (m_hShadowDataBuffer.IsInvalidated())
    {
      wdGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(wdVec4);
      desc.m_uiTotalSize = desc.m_uiStructSize * MAX_SHADOW_DATA;
      desc.m_BufferType = wdGALBufferType::Generic;
      desc.m_bUseAsStructuredBuffer = true;
      desc.m_bAllowShaderResourceView = true;
      desc.m_ResourceAccess.m_bImmutable = false;

      m_hShadowDataBuffer = wdGALDevice::GetDefaultDevice()->CreateBuffer(desc);
    }
  }

  wdViewHandle CreateShadowView()
  {
    CreateShadowAtlasTexture();
    CreateShadowDataBuffer();

    wdView* pView = nullptr;
    wdViewHandle hView = wdRenderWorld::CreateView("Unknown", pView);

    pView->SetCameraUsageHint(wdCameraUsageHint::Shadow);

    wdGALRenderTargets renderTargets;
    renderTargets.m_hDSTarget = m_hShadowAtlasTexture;
    pView->SetRenderTargets(renderTargets);

    WD_ASSERT_DEV(m_ShadowViewsMutex.IsLocked(), "m_ShadowViewsMutex must be locked at this point.");
    m_ShadowViewsMutex.Unlock(); // if the resource gets loaded in the call below, his could lead to a deadlock

    // ShadowMapRenderPipeline.wdRenderPipelineAsset
    pView->SetRenderPipelineResource(wdResourceManager::LoadResource<wdRenderPipelineResource>("{ 4f4d9f16-3d47-4c67-b821-a778f11dcaf5 }"));

    m_ShadowViewsMutex.Lock();

    // Set viewport size to something valid, this will be changed to the proper location in the atlas texture in OnEndExtraction before
    // rendering.
    pView->SetViewport(wdRectFloat(0.0f, 0.0f, 1024.0f, 1024.0f));

    const wdTag& tagCastShadows = wdTagRegistry::GetGlobalRegistry().RegisterTag("CastShadow");
    pView->m_IncludeTags.Set(tagCastShadows);

    pView->m_ExcludeTags.SetByName("EditorHidden");

    return hView;
  }

  ShadowView& GetShadowView(wdView*& out_pView)
  {
    WD_LOCK(m_ShadowViewsMutex);

    if (m_uiUsedViews == m_ShadowViews.GetCount())
    {
      m_ShadowViews.ExpandAndGetRef().m_hView = CreateShadowView();
    }

    auto& shadowView = m_ShadowViews[m_uiUsedViews];
    if (wdRenderWorld::TryGetView(shadowView.m_hView, out_pView))
    {
      out_pView->SetCamera(&shadowView.m_Camera);
      out_pView->SetLodCamera(nullptr);
    }

    m_uiUsedViews++;
    return shadowView;
  }

  bool GetDataForExtraction(const wdLightComponent* pLight, const wdView* pReferenceView, float fShadowMapScale, wdUInt32 uiPackedDataSizeInBytes, ShadowData*& out_pData)
  {
    WD_LOCK(m_ShadowDataMutex);

    LightAndRefView key = {pLight, pReferenceView};

    wdUInt32 uiDataIndex = wdInvalidIndex;
    if (m_LightToShadowDataTable.TryGetValue(key, uiDataIndex))
    {
      out_pData = &m_ShadowData[uiDataIndex];
      out_pData->m_fShadowMapScale = wdMath::Max(out_pData->m_fShadowMapScale, fShadowMapScale);
      return true;
    }

    m_ShadowData.EnsureCount(m_uiUsedShadowData + 1);

    out_pData = &m_ShadowData[m_uiUsedShadowData];
    out_pData->m_fShadowMapScale = fShadowMapScale;
    out_pData->m_fPenumbraSize = pLight->GetPenumbraSize();
    out_pData->m_fSlopeBias = pLight->GetSlopeBias() * 100.0f;       // map from user friendly range to real range
    out_pData->m_fConstantBias = pLight->GetConstantBias() / 100.0f; // map from user friendly range to real range
    out_pData->m_fFadeOutStart = 1.0f;
    out_pData->m_fMinRange = 1.0f;
    out_pData->m_uiPackedDataOffset = m_uiUsedPackedShadowData;

    m_LightToShadowDataTable.Insert(key, m_uiUsedShadowData);

    ++m_uiUsedShadowData;
    m_uiUsedPackedShadowData += uiPackedDataSizeInBytes / sizeof(wdVec4);

    return false;
  }

  void Clear()
  {
    m_uiUsedViews = 0;
    m_uiUsedShadowData = 0;

    m_LightToShadowDataTable.Clear();

    m_uiUsedPackedShadowData = 0;
  }

  wdMutex m_ShadowViewsMutex;
  wdDeque<ShadowView> m_ShadowViews;
  wdUInt32 m_uiUsedViews = 0;

  wdMutex m_ShadowDataMutex;
  wdDeque<ShadowData> m_ShadowData;
  wdUInt32 m_uiUsedShadowData = 0;
  wdHashTable<LightAndRefView, wdUInt32> m_LightToShadowDataTable;

  wdDynamicArray<wdVec4, wdAlignedAllocatorWrapper> m_PackedShadowData[2];
  wdUInt32 m_uiUsedPackedShadowData = 0; // in 16 bytes steps (sizeof(wdVec4))

  wdGALTextureHandle m_hShadowAtlasTexture;
  wdGALBufferHandle m_hShadowDataBuffer;
};

//////////////////////////////////////////////////////////////////////////

wdShadowPool::Data* wdShadowPool::s_pData = nullptr;

// static
wdUInt32 wdShadowPool::AddDirectionalLight(const wdDirectionalLightComponent* pDirLight, const wdView* pReferenceView)
{
  WD_ASSERT_DEBUG(pDirLight->GetCastShadows(), "Implementation error");

  // No shadows in orthographic views
  if (pReferenceView->GetCullingCamera()->IsOrthographic())
  {
    return wdInvalidIndex;
  }

  float fMaxReferenceSize = wdMath::Max(pReferenceView->GetViewport().width, pReferenceView->GetViewport().height);
  float fShadowMapScale = fMaxReferenceSize / s_uiShadowMapSize;

  ShadowData* pData = nullptr;
  if (s_pData->GetDataForExtraction(pDirLight, pReferenceView, fShadowMapScale, sizeof(wdDirShadowData), pData))
  {
    return pData->m_uiPackedDataOffset;
  }

  wdUInt32 uiNumCascades = wdMath::Min(pDirLight->GetNumCascades(), 4u);
  const wdCamera* pReferenceCamera = pReferenceView->GetCullingCamera();

  pData->m_uiType = LIGHT_TYPE_DIR;
  pData->m_fFadeOutStart = pDirLight->GetFadeOutStart();
  pData->m_fMinRange = pDirLight->GetMinShadowRange();
  pData->m_Views.SetCount(uiNumCascades);

  // determine cascade ranges
  float fNearPlane = pReferenceCamera->GetNearPlane();
  float fShadowRange = pDirLight->GetMinShadowRange();
  float fSplitModeWeight = pDirLight->GetSplitModeWeight();

  float fCascadeRanges[4];
  for (wdUInt32 i = 0; i < uiNumCascades; ++i)
  {
    float f = float(i + 1) / uiNumCascades;
    float logDistance = fNearPlane * wdMath::Pow(fShadowRange / fNearPlane, f);
    float linearDistance = fNearPlane + (fShadowRange - fNearPlane) * f;
    fCascadeRanges[i] = wdMath::Lerp(linearDistance, logDistance, fSplitModeWeight);
  }

  const char* viewNames[4] = {"DirLightViewC0", "DirLightViewC1", "DirLightViewC2", "DirLightViewC3"};

  const wdGameObject* pOwner = pDirLight->GetOwner();
  wdVec3 vForward = pOwner->GetGlobalDirForwards();
  wdVec3 vUp = pOwner->GetGlobalDirUp();

  float fAspectRatio = pReferenceView->GetViewport().width / pReferenceView->GetViewport().height;

  float fCascadeStart = 0.0f;
  float fCascadeEnd = 0.0f;
  float fTanFovX = wdMath::Tan(pReferenceCamera->GetFovX(fAspectRatio) * 0.5f);
  float fTanFovY = wdMath::Tan(pReferenceCamera->GetFovY(fAspectRatio) * 0.5f);
  wdVec3 corner = wdVec3(fTanFovX, fTanFovY, 1.0f);

  float fNearPlaneOffset = pDirLight->GetNearPlaneOffset();

  for (wdUInt32 i = 0; i < uiNumCascades; ++i)
  {
    wdView* pView = nullptr;
    ShadowView& shadowView = s_pData->GetShadowView(pView);
    pData->m_Views[i] = shadowView.m_hView;

    // Setup view
    {
      pView->SetName(viewNames[i]);
      pView->SetWorld(const_cast<wdWorld*>(pDirLight->GetWorld()));
      pView->SetLodCamera(pReferenceCamera);
      CopyExcludeTagsOnWhiteList(pReferenceView->m_ExcludeTags, pView->m_ExcludeTags);
    }

    // Setup camera
    {
      fCascadeStart = fCascadeEnd;
      fCascadeEnd = fCascadeRanges[i];

      wdVec3 startCorner = corner * fCascadeStart;
      wdVec3 endCorner = corner * fCascadeEnd;

      // Find the enclosing sphere for the frustum:
      // The sphere center must be on the view's center ray and should be equally far away from the corner points.
      // x = distance from camera origin to sphere center
      // d1^2 = sc.x^2 + sc.y^2 + (x - sc.z)^2
      // d2^2 = ec.x^2 + ec.y^2 + (x - ec.z)^2
      // d1 == d2 and solve for x:
      float x = (endCorner.Dot(endCorner) - startCorner.Dot(startCorner)) / (2.0f * (endCorner.z - startCorner.z));
      x = wdMath::Min(x, fCascadeEnd);

      wdVec3 center = pReferenceCamera->GetPosition() + pReferenceCamera->GetDirForwards() * x;

      // prevent too large values
      // sometimes this can happen when imported data is badly scaled and thus way too large
      // then adding dirForwards result in no change and we run into other asserts later
      center.x = wdMath::Clamp(center.x, -1000000.0f, +1000000.0f);
      center.y = wdMath::Clamp(center.y, -1000000.0f, +1000000.0f);
      center.z = wdMath::Clamp(center.z, -1000000.0f, +1000000.0f);

      endCorner.z -= x;
      float radius = endCorner.GetLength();

      if (false)
      {
        wdDebugRenderer::DrawLineSphere(pReferenceView->GetHandle(), wdBoundingSphere(center, radius), wdColor::OrangeRed);
      }

      float fCameraToCenterDistance = radius + fNearPlaneOffset;
      wdVec3 shadowCameraPos = center - vForward * fCameraToCenterDistance;
      float fFarPlane = radius + fCameraToCenterDistance;

      wdCamera& camera = shadowView.m_Camera;
      camera.LookAt(shadowCameraPos, center, vUp);
      camera.SetCameraMode(wdCameraMode::OrthoFixedWidth, radius * 2.0f, 0.0f, fFarPlane);

      // stabilize
      wdMat4 worldToLightMatrix = pView->GetViewMatrix(wdCameraEye::Left);
      wdVec3 offset = worldToLightMatrix.TransformPosition(wdVec3::ZeroVector());
      float texelInWorld = (2.0f * radius) / s_uiShadowMapSize;
      offset.x -= wdMath::Floor(offset.x / texelInWorld) * texelInWorld;
      offset.y -= wdMath::Floor(offset.y / texelInWorld) * texelInWorld;

      camera.MoveLocally(0.0f, offset.x, offset.y);
    }

    wdRenderWorld::AddViewToRender(shadowView.m_hView);
  }

  return pData->m_uiPackedDataOffset;
}

// static
wdUInt32 wdShadowPool::AddPointLight(const wdPointLightComponent* pPointLight, float fScreenSpaceSize, const wdView* pReferenceView)
{
  WD_ASSERT_DEBUG(pPointLight->GetCastShadows(), "Implementation error");

  if (fScreenSpaceSize < s_fFadeOutScaleEnd * 2.0f)
  {
    return wdInvalidIndex;
  }

  ShadowData* pData = nullptr;
  if (s_pData->GetDataForExtraction(pPointLight, nullptr, fScreenSpaceSize, sizeof(wdPointShadowData), pData))
  {
    return pData->m_uiPackedDataOffset;
  }

  pData->m_uiType = LIGHT_TYPE_POINT;
  pData->m_Views.SetCount(6);

  wdVec3 faceDirs[6] = {
    wdVec3(1.0f, 0.0f, 0.0f),
    wdVec3(-1.0f, 0.0f, 0.0f),
    wdVec3(0.0f, 1.0f, 0.0f),
    wdVec3(0.0f, -1.0f, 0.0f),
    wdVec3(0.0f, 0.0f, 1.0f),
    wdVec3(0.0f, 0.0f, -1.0f),
  };

  const char* viewNames[6] = {
    "PointLightView+X",
    "PointLightView-X",
    "PointLightView+Y",
    "PointLightView-Y",
    "PointLightView+Z",
    "PointLightView-Z",
  };

  const wdGameObject* pOwner = pPointLight->GetOwner();
  wdVec3 vPosition = pOwner->GetGlobalPosition();
  wdVec3 vUp = wdVec3(0.0f, 0.0f, 1.0f);

  float fPenumbraSize = wdMath::Max(pPointLight->GetPenumbraSize(), (0.5f / s_uiMinShadowMapSize)); // at least one texel for hardware pcf
  float fFov = AddSafeBorder(wdAngle::Degree(90.0f), fPenumbraSize);

  float fNearPlane = 0.1f; ///\todo expose somewhere
  float fFarPlane = pPointLight->GetEffectiveRange();

  for (wdUInt32 i = 0; i < 6; ++i)
  {
    wdView* pView = nullptr;
    ShadowView& shadowView = s_pData->GetShadowView(pView);
    pData->m_Views[i] = shadowView.m_hView;

    // Setup view
    {
      pView->SetName(viewNames[i]);
      pView->SetWorld(const_cast<wdWorld*>(pPointLight->GetWorld()));
      CopyExcludeTagsOnWhiteList(pReferenceView->m_ExcludeTags, pView->m_ExcludeTags);
    }

    // Setup camera
    {
      wdVec3 vForward = faceDirs[i];

      wdCamera& camera = shadowView.m_Camera;
      camera.LookAt(vPosition, vPosition + vForward, vUp);
      camera.SetCameraMode(wdCameraMode::PerspectiveFixedFovX, fFov, fNearPlane, fFarPlane);
    }

    wdRenderWorld::AddViewToRender(shadowView.m_hView);
  }

  return pData->m_uiPackedDataOffset;
}

// static
wdUInt32 wdShadowPool::AddSpotLight(const wdSpotLightComponent* pSpotLight, float fScreenSpaceSize, const wdView* pReferenceView)
{
  WD_ASSERT_DEBUG(pSpotLight->GetCastShadows(), "Implementation error");

  if (fScreenSpaceSize < s_fFadeOutScaleEnd)
  {
    return wdInvalidIndex;
  }

  ShadowData* pData = nullptr;
  if (s_pData->GetDataForExtraction(pSpotLight, nullptr, fScreenSpaceSize, sizeof(wdSpotShadowData), pData))
  {
    return pData->m_uiPackedDataOffset;
  }

  pData->m_uiType = LIGHT_TYPE_SPOT;
  pData->m_Views.SetCount(1);

  wdView* pView = nullptr;
  ShadowView& shadowView = s_pData->GetShadowView(pView);
  pData->m_Views[0] = shadowView.m_hView;

  // Setup view
  {
    pView->SetName("SpotLightView");
    pView->SetWorld(const_cast<wdWorld*>(pSpotLight->GetWorld()));
    CopyExcludeTagsOnWhiteList(pReferenceView->m_ExcludeTags, pView->m_ExcludeTags);
  }

  // Setup camera
  {
    const wdGameObject* pOwner = pSpotLight->GetOwner();
    wdVec3 vPosition = pOwner->GetGlobalPosition();
    wdVec3 vForward = pOwner->GetGlobalDirForwards();
    wdVec3 vUp = pOwner->GetGlobalDirUp();

    float fFov = AddSafeBorder(pSpotLight->GetOuterSpotAngle(), pSpotLight->GetPenumbraSize());
    float fNearPlane = 0.1f; ///\todo expose somewhere
    float fFarPlane = pSpotLight->GetEffectiveRange();

    wdCamera& camera = shadowView.m_Camera;
    camera.LookAt(vPosition, vPosition + vForward, vUp);
    camera.SetCameraMode(wdCameraMode::PerspectiveFixedFovX, fFov, fNearPlane, fFarPlane);
  }

  wdRenderWorld::AddViewToRender(shadowView.m_hView);

  return pData->m_uiPackedDataOffset;
}

// static
wdGALTextureHandle wdShadowPool::GetShadowAtlasTexture()
{
  return s_pData->m_hShadowAtlasTexture;
}

// static
wdGALBufferHandle wdShadowPool::GetShadowDataBuffer()
{
  return s_pData->m_hShadowDataBuffer;
}

// static
void wdShadowPool::AddExcludeTagToWhiteList(const wdTag& tag)
{
  s_ExcludeTagsWhiteList.Set(tag);
}

// static
void wdShadowPool::OnEngineStartup()
{
  s_pData = WD_DEFAULT_NEW(wdShadowPool::Data);

  wdRenderWorld::GetExtractionEvent().AddEventHandler(OnExtractionEvent);
  wdRenderWorld::GetRenderEvent().AddEventHandler(OnRenderEvent);
}

// static
void wdShadowPool::OnEngineShutdown()
{
  wdRenderWorld::GetExtractionEvent().RemoveEventHandler(OnExtractionEvent);
  wdRenderWorld::GetRenderEvent().RemoveEventHandler(OnRenderEvent);

  WD_DEFAULT_DELETE(s_pData);
}

// static
void wdShadowPool::OnExtractionEvent(const wdRenderWorldExtractionEvent& e)
{
  if (e.m_Type != wdRenderWorldExtractionEvent::Type::EndExtraction)
    return;

  WD_PROFILE_SCOPE("Shadow Pool Update");

  wdUInt32 uiDataIndex = wdRenderWorld::GetDataIndexForExtraction();
  auto& packedShadowData = s_pData->m_PackedShadowData[uiDataIndex];
  packedShadowData.SetCountUninitialized(s_pData->m_uiUsedPackedShadowData);

  if (s_pData->m_uiUsedShadowData == 0)
    return;

  // Sort by shadow map scale
  s_SortedShadowData.Clear();

  for (wdUInt32 uiShadowDataIndex = 0; uiShadowDataIndex < s_pData->m_uiUsedShadowData; ++uiShadowDataIndex)
  {
    auto& shadowData = s_pData->m_ShadowData[uiShadowDataIndex];

    auto& sorted = s_SortedShadowData.ExpandAndGetRef();
    sorted.m_uiIndex = uiShadowDataIndex;
    sorted.m_fShadowMapScale = shadowData.m_uiType == LIGHT_TYPE_DIR ? 100.0f : wdMath::Min(shadowData.m_fShadowMapScale, 10.0f);
  }

  s_SortedShadowData.Sort();

  // Prepare atlas
  s_AtlasCells.Clear();
  s_AtlasCells.ExpandAndGetRef().m_Rect = wdRectU32(0, 0, s_uiShadowAtlasTextureWidth, s_uiShadowAtlasTextureHeight);

  float fAtlasInvWidth = 1.0f / s_uiShadowAtlasTextureWidth;
  float fAtlasInvHeight = 1.0f / s_uiShadowAtlasTextureWidth;

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  wdUInt32 uiTotalAtlasSize = s_uiShadowAtlasTextureWidth * s_uiShadowAtlasTextureHeight;
  wdUInt32 uiUsedAtlasSize = 0;

  wdDebugRendererContext debugContext(wdWorld::GetWorld(0));
  if (const wdView* pView = wdRenderWorld::GetViewByUsageHint(wdCameraUsageHint::MainView, wdCameraUsageHint::EditorView))
  {
    debugContext = wdDebugRendererContext(pView->GetHandle());
  }

  if (cvar_RenderingShadowsShowPoolStats)
  {
    wdDebugRenderer::DrawInfoText(debugContext, wdDebugRenderer::ScreenPlacement::TopLeft, "ShadowPoolStats", "Shadow Pool Stats:", wdColor::LightSteelBlue);
    wdDebugRenderer::DrawInfoText(debugContext, wdDebugRenderer::ScreenPlacement::TopLeft, "ShadowPoolStats", "Details (Name: Size - Atlas Offset)", wdColor::LightSteelBlue);
  }

#endif

  for (auto& sorted : s_SortedShadowData)
  {
    wdUInt32 uiShadowDataIndex = sorted.m_uiIndex;
    auto& shadowData = s_pData->m_ShadowData[uiShadowDataIndex];

    wdUInt32 uiShadowMapSize = s_uiShadowMapSize;
    float fadeOutStart = s_fFadeOutScaleStart;
    float fadeOutEnd = s_fFadeOutScaleEnd;

    // point lights use a lot of atlas space thus we cut the shadow map size in half
    if (shadowData.m_uiType == LIGHT_TYPE_POINT)
    {
      uiShadowMapSize /= 2;
      fadeOutStart *= 2.0f;
      fadeOutEnd *= 2.0f;
    }

    uiShadowMapSize = wdMath::PowerOfTwo_Ceil((wdUInt32)(uiShadowMapSize * wdMath::Clamp(shadowData.m_fShadowMapScale, fadeOutStart, 1.0f)));

    wdHybridArray<wdView*, 8> shadowViews;
    wdHybridArray<wdRectU32, 8> atlasRects;

    // Fill atlas
    for (wdUInt32 uiViewIndex = 0; uiViewIndex < shadowData.m_Views.GetCount(); ++uiViewIndex)
    {
      wdView* pShadowView = nullptr;
      wdRenderWorld::TryGetView(shadowData.m_Views[uiViewIndex], pShadowView);
      shadowViews.PushBack(pShadowView);

      WD_ASSERT_DEV(pShadowView != nullptr, "Implementation error");

      wdRectU32 atlasRect = FindAtlasRect(uiShadowMapSize, uiShadowDataIndex);
      atlasRects.PushBack(atlasRect);

      pShadowView->SetViewport(wdRectFloat((float)atlasRect.x, (float)atlasRect.y, (float)atlasRect.width, (float)atlasRect.height));

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
      if (cvar_RenderingShadowsShowPoolStats)
      {
        wdDebugRenderer::DrawInfoText(debugContext, wdDebugRenderer::ScreenPlacement::TopLeft, "ShadowPoolStats", wdFmt("{0}: {1} - {2}x{3}", pShadowView->GetName(), atlasRect.width, atlasRect.x, atlasRect.y), wdColor::LightSteelBlue);

        uiUsedAtlasSize += atlasRect.width * atlasRect.height;
      }
#endif
    }

    // Fill shadow data
    if (shadowData.m_uiType == LIGHT_TYPE_DIR)
    {
      wdUInt32 uiNumCascades = shadowData.m_Views.GetCount();

      wdUInt32 uiMatrixIndex = GET_WORLD_TO_LIGHT_MATRIX_INDEX(shadowData.m_uiPackedDataOffset, 0);
      wdMat4& worldToLightMatrix = *reinterpret_cast<wdMat4*>(&packedShadowData[uiMatrixIndex]);

      worldToLightMatrix = shadowViews[0]->GetViewProjectionMatrix(wdCameraEye::Left);

      for (wdUInt32 uiViewIndex = 0; uiViewIndex < uiNumCascades; ++uiViewIndex)
      {
        if (uiViewIndex >= 1)
        {
          wdMat4 cascadeToWorldMatrix = shadowViews[uiViewIndex]->GetInverseViewProjectionMatrix(wdCameraEye::Left);
          wdVec3 cascadeCorner = cascadeToWorldMatrix.TransformPosition(wdVec3(0.0f));
          cascadeCorner = worldToLightMatrix.TransformPosition(cascadeCorner);

          wdVec3 otherCorner = cascadeToWorldMatrix.TransformPosition(wdVec3(1.0f));
          otherCorner = worldToLightMatrix.TransformPosition(otherCorner);

          wdUInt32 uiCascadeScaleIndex = GET_CASCADE_SCALE_INDEX(shadowData.m_uiPackedDataOffset, uiViewIndex - 1);
          wdUInt32 uiCascadeOffsetIndex = GET_CASCADE_OFFSET_INDEX(shadowData.m_uiPackedDataOffset, uiViewIndex - 1);

          wdVec4& cascadeScale = packedShadowData[uiCascadeScaleIndex];
          wdVec4& cascadeOffset = packedShadowData[uiCascadeOffsetIndex];

          cascadeScale = wdVec3(1.0f).CompDiv(otherCorner - cascadeCorner).GetAsVec4(1.0f);
          cascadeOffset = cascadeCorner.GetAsVec4(0.0f).CompMul(-cascadeScale);
        }

        wdUInt32 uiAtlasScaleOffsetIndex = GET_ATLAS_SCALE_OFFSET_INDEX(shadowData.m_uiPackedDataOffset, uiViewIndex);
        wdVec4& atlasScaleOffset = packedShadowData[uiAtlasScaleOffsetIndex];

        wdRectU32 atlasRect = atlasRects[uiViewIndex];
        if (atlasRect.HasNonZeroArea())
        {
          wdVec2 scale = wdVec2(atlasRect.width * fAtlasInvWidth, atlasRect.height * fAtlasInvHeight);
          wdVec2 offset = wdVec2(atlasRect.x * fAtlasInvWidth, atlasRect.y * fAtlasInvHeight);

          // combine with tex scale offset
          atlasScaleOffset.x = scale.x * 0.5f;
          atlasScaleOffset.y = scale.y * -0.5f;
          atlasScaleOffset.z = offset.x + scale.x * 0.5f;
          atlasScaleOffset.w = offset.y + scale.y * 0.5f;
        }
        else
        {
          atlasScaleOffset.Set(1.0f, 1.0f, 0.0f, 0.0f);
        }
      }

      const wdCamera* pFirstCascadeCamera = shadowViews[0]->GetCamera();
      const wdCamera* pLastCascadeCamera = shadowViews[uiNumCascades - 1]->GetCamera();

      float cascadeSize = pFirstCascadeCamera->GetFovOrDim();
      float texelSize = 1.0f / uiShadowMapSize;
      float penumbraSize = wdMath::Max(shadowData.m_fPenumbraSize / cascadeSize, texelSize);
      float goodPenumbraSize = 8.0f / uiShadowMapSize;
      float relativeShadowSize = uiShadowMapSize * fAtlasInvHeight;

      // params
      {
        // tweak values to keep the default values consistent with spot and point lights
        float slopeBias = shadowData.m_fSlopeBias * wdMath::Max(penumbraSize, goodPenumbraSize);
        float constantBias = shadowData.m_fConstantBias * 0.2f;
        wdUInt32 uilastCascadeIndex = uiNumCascades - 1;

        wdUInt32 uiParamsIndex = GET_SHADOW_PARAMS_INDEX(shadowData.m_uiPackedDataOffset);
        wdVec4& shadowParams = packedShadowData[uiParamsIndex];
        shadowParams.x = slopeBias;
        shadowParams.y = constantBias;
        shadowParams.z = penumbraSize * relativeShadowSize;
        shadowParams.w = *reinterpret_cast<float*>(&uilastCascadeIndex);
      }

      // params2
      {
        float ditherMultiplier = 0.2f / cascadeSize;
        float zRange = cascadeSize / pFirstCascadeCamera->GetFarPlane();

        float actualPenumbraSize = shadowData.m_fPenumbraSize / pLastCascadeCamera->GetFovOrDim();
        float penumbraSizeIncrement = wdMath::Max(goodPenumbraSize - actualPenumbraSize, 0.0f) / shadowData.m_fMinRange;

        wdUInt32 uiParams2Index = GET_SHADOW_PARAMS2_INDEX(shadowData.m_uiPackedDataOffset);
        wdVec4& shadowParams2 = packedShadowData[uiParams2Index];
        shadowParams2.x = 1.0f - (wdMath::Max(penumbraSize, goodPenumbraSize) + texelSize) * 2.0f;
        shadowParams2.y = ditherMultiplier;
        shadowParams2.z = ditherMultiplier * zRange;
        shadowParams2.w = penumbraSizeIncrement * relativeShadowSize;
      }

      // fadeout
      {
        float fadeOutRange = 1.0f - shadowData.m_fFadeOutStart;
        float xyScale = -1.0f / fadeOutRange;
        float xyOffset = -xyScale;

        float zFadeOutRange = fadeOutRange * pLastCascadeCamera->GetFovOrDim() / pLastCascadeCamera->GetFarPlane();
        float zScale = -1.0f / zFadeOutRange;
        float zOffset = -zScale;

        wdUInt32 uiFadeOutIndex = GET_FADE_OUT_PARAMS_INDEX(shadowData.m_uiPackedDataOffset);
        wdVec4& fadeOutParams = packedShadowData[uiFadeOutIndex];
        fadeOutParams.x = xyScale;
        fadeOutParams.y = xyOffset;
        fadeOutParams.z = zScale;
        fadeOutParams.w = zOffset;
      }
    }
    else // spot or point light
    {
      wdMat4 texMatrix;
      texMatrix.SetIdentity();
      texMatrix.SetDiagonal(wdVec4(0.5f, -0.5f, 1.0f, 1.0f));
      texMatrix.SetTranslationVector(wdVec3(0.5f, 0.5f, 0.0f));

      wdAngle fov;

      for (wdUInt32 uiViewIndex = 0; uiViewIndex < shadowData.m_Views.GetCount(); ++uiViewIndex)
      {
        wdView* pShadowView = shadowViews[uiViewIndex];
        WD_ASSERT_DEV(pShadowView != nullptr, "Implementation error");

        wdUInt32 uiMatrixIndex = GET_WORLD_TO_LIGHT_MATRIX_INDEX(shadowData.m_uiPackedDataOffset, uiViewIndex);
        wdMat4& worldToLightMatrix = *reinterpret_cast<wdMat4*>(&packedShadowData[uiMatrixIndex]);

        wdRectU32 atlasRect = atlasRects[uiViewIndex];
        if (atlasRect.HasNonZeroArea())
        {
          wdVec2 scale = wdVec2(atlasRect.width * fAtlasInvWidth, atlasRect.height * fAtlasInvHeight);
          wdVec2 offset = wdVec2(atlasRect.x * fAtlasInvWidth, atlasRect.y * fAtlasInvHeight);

          wdMat4 atlasMatrix;
          atlasMatrix.SetIdentity();
          atlasMatrix.SetDiagonal(wdVec4(scale.x, scale.y, 1.0f, 1.0f));
          atlasMatrix.SetTranslationVector(offset.GetAsVec3(0.0f));

          fov = pShadowView->GetCamera()->GetFovY(1.0f);
          const wdMat4& viewProjection = pShadowView->GetViewProjectionMatrix(wdCameraEye::Left);

          worldToLightMatrix = atlasMatrix * texMatrix * viewProjection;
        }
        else
        {
          worldToLightMatrix.SetIdentity();
        }
      }

      float screenHeight = wdMath::Tan(fov * 0.5f) * 20.0f; // screen height in worldspace at 10m distance
      float texelSize = 1.0f / uiShadowMapSize;
      float penumbraSize = wdMath::Max(shadowData.m_fPenumbraSize / screenHeight, texelSize);
      float relativeShadowSize = uiShadowMapSize * fAtlasInvHeight;

      float slopeBias = shadowData.m_fSlopeBias * penumbraSize * wdMath::Tan(fov * 0.5f);
      float constantBias = shadowData.m_fConstantBias * s_uiShadowMapSize / uiShadowMapSize;
      float fadeOut = wdMath::Clamp((shadowData.m_fShadowMapScale - fadeOutEnd) / (fadeOutStart - fadeOutEnd), 0.0f, 1.0f);

      wdUInt32 uiParamsIndex = GET_SHADOW_PARAMS_INDEX(shadowData.m_uiPackedDataOffset);
      wdVec4& shadowParams = packedShadowData[uiParamsIndex];
      shadowParams.x = slopeBias;
      shadowParams.y = constantBias;
      shadowParams.z = penumbraSize * relativeShadowSize;
      shadowParams.w = wdMath::Sqrt(fadeOut);
    }
  }

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  if (cvar_RenderingShadowsShowPoolStats)
  {
    wdDebugRenderer::DrawInfoText(debugContext, wdDebugRenderer::ScreenPlacement::TopLeft, "ShadowPoolStats", wdFmt("Atlas Utilization: {0}%%", wdArgF(100.0 * (double)uiUsedAtlasSize / uiTotalAtlasSize, 2)), wdColor::LightSteelBlue);
  }
#endif

  s_pData->Clear();
}

// static
void wdShadowPool::OnRenderEvent(const wdRenderWorldRenderEvent& e)
{
  if (e.m_Type != wdRenderWorldRenderEvent::Type::BeginRender)
    return;

  if (s_pData->m_hShadowAtlasTexture.IsInvalidated() || s_pData->m_hShadowDataBuffer.IsInvalidated())
    return;

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();
  wdGALPass* pGALPass = pDevice->BeginPass("Shadow Atlas");

  wdGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(s_pData->m_hShadowAtlasTexture));
  renderingSetup.m_bClearDepth = true;

  auto pCommandEncoder = pGALPass->BeginRendering(renderingSetup);

  wdUInt32 uiDataIndex = wdRenderWorld::GetDataIndexForRendering();
  auto& packedShadowData = s_pData->m_PackedShadowData[uiDataIndex];
  if (!packedShadowData.IsEmpty())
  {
    WD_PROFILE_SCOPE("Shadow Data Buffer Update");

    pCommandEncoder->UpdateBuffer(s_pData->m_hShadowDataBuffer, 0, packedShadowData.GetByteArrayPtr());
  }

  pGALPass->EndRendering(pCommandEncoder);
  pDevice->EndPass(pGALPass);
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ShadowPool);
