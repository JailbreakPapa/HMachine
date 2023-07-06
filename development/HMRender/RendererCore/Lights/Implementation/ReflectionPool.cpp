#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/Implementation/ReflectionPoolData.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/Pass.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
WD_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, ReflectionPool)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "RenderWorld"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    wdReflectionPool::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    wdReflectionPool::OnEngineShutdown();
  }

WD_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////
/// wdReflectionPool

wdReflectionProbeId wdReflectionPool::RegisterReflectionProbe(const wdWorld* pWorld, const wdReflectionProbeDesc& desc, const wdReflectionProbeComponentBase* pComponent)
{
  WD_LOCK(s_pData->m_Mutex);

  Data::ProbeData probe;
  s_pData->UpdateProbeData(probe, desc, pComponent);
  return s_pData->AddProbe(pWorld, std::move(probe));
}

void wdReflectionPool::DeregisterReflectionProbe(const wdWorld* pWorld, wdReflectionProbeId id)
{
  WD_LOCK(s_pData->m_Mutex);
  s_pData->RemoveProbe(pWorld, id);
}

void wdReflectionPool::UpdateReflectionProbe(const wdWorld* pWorld, wdReflectionProbeId id, const wdReflectionProbeDesc& desc, const wdReflectionProbeComponentBase* pComponent)
{
  WD_LOCK(s_pData->m_Mutex);
  wdReflectionPool::Data::WorldReflectionData& data = s_pData->GetWorldData(pWorld);
  Data::ProbeData& probeData = data.m_Probes.GetValueUnchecked(id.m_InstanceIndex);
  s_pData->UpdateProbeData(probeData, desc, pComponent);
  data.m_mapping.UpdateProbe(id, probeData.m_Flags);
}

void wdReflectionPool::ExtractReflectionProbe(const wdComponent* pComponent, wdMsgExtractRenderData& ref_msg, wdReflectionProbeRenderData* pRenderData0, const wdWorld* pWorld, wdReflectionProbeId id, float fPriority)
{
  WD_LOCK(s_pData->m_Mutex);
  s_pData->m_ReflectionProbeUpdater.ScheduleUpdateSteps();

  const wdUInt32 uiWorldIndex = pWorld->GetIndex();
  wdReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[uiWorldIndex];
  data.m_mapping.AddWeight(id, fPriority);
  const wdInt32 iMappedIndex = data.m_mapping.GetReflectionIndex(id, true);

  Data::ProbeData& probeData = data.m_Probes.GetValueUnchecked(id.m_InstanceIndex);

  if (pComponent->GetOwner()->IsDynamic())
  {
    wdTransform globalTransform = pComponent->GetOwner()->GetGlobalTransform();
    if (!probeData.m_Flags.IsSet(wdProbeFlags::Dynamic) && probeData.m_GlobalTransform != globalTransform)
    {
      data.m_mapping.UpdateProbe(id, probeData.m_Flags);
    }
    probeData.m_GlobalTransform = globalTransform;
  }

  // The sky light is always active and not added to the render data (always passes in nullptr as pRenderData).
  if (pRenderData0 && iMappedIndex > 0)
  {
    // Index and flags are stored in m_uiIndex so we can't just overwrite it.
    pRenderData0->m_uiIndex |= (wdUInt32)iMappedIndex;
    ref_msg.AddRenderData(pRenderData0, wdDefaultRenderDataCategories::ReflectionProbe, wdRenderData::Caching::Never);
  }

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  const wdUInt32 uiMipLevels = GetMipLevels();
  if (probeData.m_desc.m_bShowDebugInfo && s_pData->m_hDebugMaterial.GetCount() == uiMipLevels * s_uiNumReflectionProbeCubeMaps)
  {
    if (ref_msg.m_OverrideCategory == wdInvalidRenderDataCategory)
    {
      wdInt32 activeIndex = 0;
      if (s_pData->m_ActiveDynamicUpdate.Contains(wdReflectionProbeRef{uiWorldIndex, id}))
      {
        activeIndex = 1;
      }

      wdStringBuilder sEnum;
      wdReflectionUtils::BitflagsToString(probeData.m_Flags, sEnum, wdReflectionUtils::EnumConversionMode::ValueNameOnly);
      wdStringBuilder s;
      s.Format("\n RefIdx: {}\nUpdating: {}\nFlags: {}\n", iMappedIndex, activeIndex, sEnum);
      wdDebugRenderer::Draw3DText(pWorld, s, pComponent->GetOwner()->GetGlobalPosition(), wdColorScheme::LightUI(wdColorScheme::Violet));
    }

    // Not mapped in the atlas - cannot render it.
    if (iMappedIndex < 0)
      return;

    const wdGameObject* pOwner = pComponent->GetOwner();
    const wdTransform ownerTransform = pOwner->GetGlobalTransform();

    wdUInt32 uiMipLevelsToRender = probeData.m_desc.m_bShowMipMaps ? uiMipLevels : 1;
    for (wdUInt32 i = 0; i < uiMipLevelsToRender; i++)
    {
      wdMeshRenderData* pRenderData = wdCreateRenderDataForThisFrame<wdMeshRenderData>(pOwner);
      pRenderData->m_GlobalTransform.m_vPosition = ownerTransform * probeData.m_desc.m_vCaptureOffset;
      pRenderData->m_GlobalTransform.m_vScale = wdVec3(1.0f);
      if (!probeData.m_Flags.IsSet(wdProbeFlags::SkyLight))
      {
        pRenderData->m_GlobalTransform.m_qRotation = ownerTransform.m_qRotation;
      }
      pRenderData->m_GlobalTransform.m_vPosition.z += s_fDebugSphereRadius * i * 2;
      pRenderData->m_GlobalBounds = pOwner->GetGlobalBounds();
      pRenderData->m_hMesh = s_pData->m_hDebugSphere;
      pRenderData->m_hMaterial = s_pData->m_hDebugMaterial[iMappedIndex * uiMipLevels + i];
      pRenderData->m_Color = wdColor::White;
      pRenderData->m_uiSubMeshIndex = 0;
      pRenderData->m_uiUniqueID = wdRenderComponent::GetUniqueIdForRendering(pComponent, 0);

      pRenderData->FillBatchIdAndSortingKey();
      ref_msg.AddRenderData(pRenderData, wdDefaultRenderDataCategories::LitOpaque, wdRenderData::Caching::Never);
    }
  }
#endif
}

//////////////////////////////////////////////////////////////////////////
/// SkyLight

wdReflectionProbeId wdReflectionPool::RegisterSkyLight(const wdWorld* pWorld, wdReflectionProbeDesc& ref_desc, const wdSkyLightComponent* pComponent)
{
  WD_LOCK(s_pData->m_Mutex);
  const wdUInt32 uiWorldIndex = pWorld->GetIndex();
  s_pData->m_uiWorldHasSkyLight |= WD_BIT(uiWorldIndex);
  s_pData->m_uiSkyIrradianceChanged |= WD_BIT(uiWorldIndex);

  Data::ProbeData probe;
  s_pData->UpdateSkyLightData(probe, ref_desc, pComponent);

  wdReflectionProbeId id = s_pData->AddProbe(pWorld, std::move(probe));
  return id;
}

void wdReflectionPool::DeregisterSkyLight(const wdWorld* pWorld, wdReflectionProbeId id)
{
  WD_LOCK(s_pData->m_Mutex);

  s_pData->RemoveProbe(pWorld, id);

  const wdUInt32 uiWorldIndex = pWorld->GetIndex();
  s_pData->m_uiWorldHasSkyLight &= ~WD_BIT(uiWorldIndex);
  s_pData->m_uiSkyIrradianceChanged |= WD_BIT(uiWorldIndex);
}

void wdReflectionPool::UpdateSkyLight(const wdWorld* pWorld, wdReflectionProbeId id, const wdReflectionProbeDesc& desc, const wdSkyLightComponent* pComponent)
{
  WD_LOCK(s_pData->m_Mutex);
  wdReflectionPool::Data::WorldReflectionData& data = s_pData->GetWorldData(pWorld);
  Data::ProbeData& probeData = data.m_Probes.GetValueUnchecked(id.m_InstanceIndex);
  if (s_pData->UpdateSkyLightData(probeData, desc, pComponent))
  {
    // s_pData->UnmapProbe(pWorld->GetIndex(), data, id);
  }
  data.m_mapping.UpdateProbe(id, probeData.m_Flags);
}

//////////////////////////////////////////////////////////////////////////
/// Misc

// static
void wdReflectionPool::SetConstantSkyIrradiance(const wdWorld* pWorld, const wdAmbientCube<wdColor>& skyIrradiance)
{
  wdUInt32 uiWorldIndex = pWorld->GetIndex();
  wdAmbientCube<wdColorLinear16f> skyIrradiance16f = skyIrradiance;

  auto& skyIrradianceStorage = s_pData->m_SkyIrradianceStorage;
  if (skyIrradianceStorage[uiWorldIndex] != skyIrradiance16f)
  {
    skyIrradianceStorage[uiWorldIndex] = skyIrradiance16f;

    s_pData->m_uiSkyIrradianceChanged |= WD_BIT(uiWorldIndex);
  }
}

void wdReflectionPool::ResetConstantSkyIrradiance(const wdWorld* pWorld)
{
  wdUInt32 uiWorldIndex = pWorld->GetIndex();

  auto& skyIrradianceStorage = s_pData->m_SkyIrradianceStorage;
  if (skyIrradianceStorage[uiWorldIndex] != wdAmbientCube<wdColorLinear16f>())
  {
    skyIrradianceStorage[uiWorldIndex] = wdAmbientCube<wdColorLinear16f>();

    s_pData->m_uiSkyIrradianceChanged |= WD_BIT(uiWorldIndex);
  }
}

// static
wdUInt32 wdReflectionPool::GetReflectionCubeMapSize()
{
  return s_uiReflectionCubeMapSize;
}

// static
wdGALTextureHandle wdReflectionPool::GetReflectionSpecularTexture(wdUInt32 uiWorldIndex, wdEnum<wdCameraUsageHint> cameraUsageHint)
{
  if (uiWorldIndex < s_pData->m_WorldReflectionData.GetCount() && cameraUsageHint != wdCameraUsageHint::Reflection)
  {
    Data::WorldReflectionData* pData = s_pData->m_WorldReflectionData[uiWorldIndex].Borrow();
    if (pData)
      return pData->m_mapping.GetTexture();
  }
  return s_pData->m_hFallbackReflectionSpecularTexture;
}

// static
wdGALTextureHandle wdReflectionPool::GetSkyIrradianceTexture()
{
  return s_pData->m_hSkyIrradianceTexture;
}

//////////////////////////////////////////////////////////////////////////
/// Private Functions

// static
void wdReflectionPool::OnEngineStartup()
{
  s_pData = WD_DEFAULT_NEW(wdReflectionPool::Data);

  wdRenderWorld::GetExtractionEvent().AddEventHandler(OnExtractionEvent);
  wdRenderWorld::GetRenderEvent().AddEventHandler(OnRenderEvent);
}

// static
void wdReflectionPool::OnEngineShutdown()
{
  wdRenderWorld::GetExtractionEvent().RemoveEventHandler(OnExtractionEvent);
  wdRenderWorld::GetRenderEvent().RemoveEventHandler(OnRenderEvent);

  WD_DEFAULT_DELETE(s_pData);
}

// static
void wdReflectionPool::OnExtractionEvent(const wdRenderWorldExtractionEvent& e)
{
  if (e.m_Type == wdRenderWorldExtractionEvent::Type::BeginExtraction)
  {
    WD_PROFILE_SCOPE("Reflection Pool BeginExtraction");
    s_pData->CreateSkyIrradianceTexture();
    s_pData->CreateReflectionViewsAndResources();
    s_pData->PreExtraction();
  }

  if (e.m_Type == wdRenderWorldExtractionEvent::Type::EndExtraction)
  {
    WD_PROFILE_SCOPE("Reflection Pool EndExtraction");
    s_pData->PostExtraction();
  }
}

// static
void wdReflectionPool::OnRenderEvent(const wdRenderWorldRenderEvent& e)
{
  if (e.m_Type != wdRenderWorldRenderEvent::Type::BeginRender)
    return;

  if (s_pData->m_hSkyIrradianceTexture.IsInvalidated())
    return;

  WD_LOCK(s_pData->m_Mutex);

  wdUInt64 uiWorldHasSkyLight = s_pData->m_uiWorldHasSkyLight;
  wdUInt64 uiSkyIrradianceChanged = s_pData->m_uiSkyIrradianceChanged;
  if ((~uiWorldHasSkyLight & uiSkyIrradianceChanged) == 0)
    return;

  auto& skyIrradianceStorage = s_pData->m_SkyIrradianceStorage;

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  auto pGALPass = pDevice->BeginPass("Sky Irradiance Texture Update");
  wdHybridArray<wdGALTextureHandle, 4> atlasToClear;

  {
    auto pGALCommandEncoder = pGALPass->BeginCompute();
    for (wdUInt32 i = 0; i < skyIrradianceStorage.GetCount(); ++i)
    {
      if ((uiWorldHasSkyLight & WD_BIT(i)) == 0 && (uiSkyIrradianceChanged & WD_BIT(i)) != 0)
      {
        wdBoundingBoxu32 destBox;
        destBox.m_vMin.Set(0, i, 0);
        destBox.m_vMax.Set(6, i + 1, 1);
        wdGALSystemMemoryDescription memDesc;
        memDesc.m_pData = &skyIrradianceStorage[i].m_Values[0];
        memDesc.m_uiRowPitch = sizeof(wdAmbientCube<wdColorLinear16f>);
        pGALCommandEncoder->UpdateTexture(s_pData->m_hSkyIrradianceTexture, wdGALTextureSubresource(), destBox, memDesc);

        uiSkyIrradianceChanged &= ~WD_BIT(i);

        if (i < s_pData->m_WorldReflectionData.GetCount() && s_pData->m_WorldReflectionData[i] != nullptr)
        {
          wdReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[i];
          atlasToClear.PushBack(data.m_mapping.GetTexture());
        }
      }
    }
    pGALPass->EndCompute(pGALCommandEncoder);
  }

  {
    // Clear specular sky reflection to black.
    const wdUInt32 uiNumMipMaps = GetMipLevels();
    for (wdGALTextureHandle atlas : atlasToClear)
    {
      for (wdUInt32 uiMipMapIndex = 0; uiMipMapIndex < uiNumMipMaps; ++uiMipMapIndex)
      {
        for (wdUInt32 uiFaceIndex = 0; uiFaceIndex < 6; ++uiFaceIndex)
        {
          wdGALRenderingSetup renderingSetup;
          wdGALRenderTargetViewCreationDescription desc;
          desc.m_hTexture = atlas;
          desc.m_uiMipLevel = uiMipMapIndex;
          desc.m_uiFirstSlice = uiFaceIndex;
          desc.m_uiSliceCount = 1;
          renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->CreateRenderTargetView(desc));
          renderingSetup.m_ClearColor = wdColor(0, 0, 0, 1);
          renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;

          auto pGALCommandEncoder = pGALPass->BeginRendering(renderingSetup, "ClearSkySpecular");
          pGALCommandEncoder->Clear(wdColor::Black);
          pGALPass->EndRendering(pGALCommandEncoder);
        }
      }
    }
  }

  pDevice->EndPass(pGALPass);
}


WD_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ReflectionPool);
