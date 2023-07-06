#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionPoolData.h>

#include <Core/Graphics/Geometry.h>
#include <RendererCore/Lights/BoxReflectionProbeComponent.h>
#include <RendererCore/Lights/SkyLightComponent.h>
#include <RendererCore/Lights/SphereReflectionProbeComponent.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

//////////////////////////////////////////////////////////////////////////
/// wdReflectionPool::Data

wdReflectionPool::Data* wdReflectionPool::s_pData;

wdReflectionPool::Data::Data()
{
  m_SkyIrradianceStorage.SetCount(64);
}

wdReflectionPool::Data::~Data()
{
  if (!m_hFallbackReflectionSpecularTexture.IsInvalidated())
  {
    wdGALDevice::GetDefaultDevice()->DestroyTexture(m_hFallbackReflectionSpecularTexture);
    m_hFallbackReflectionSpecularTexture.Invalidate();
  }

  wdUInt32 uiWorldReflectionCount = m_WorldReflectionData.GetCount();
  for (wdUInt32 i = 0; i < uiWorldReflectionCount; ++i)
  {
    WorldReflectionData* pData = m_WorldReflectionData[i].Borrow();
    WD_ASSERT_DEV(!pData || pData->m_Probes.IsEmpty(), "Not all probes were deregistered.");
  }
  m_WorldReflectionData.Clear();

  if (!m_hSkyIrradianceTexture.IsInvalidated())
  {
    wdGALDevice::GetDefaultDevice()->DestroyTexture(m_hSkyIrradianceTexture);
    m_hSkyIrradianceTexture.Invalidate();
  }
}

wdReflectionProbeId wdReflectionPool::Data::AddProbe(const wdWorld* pWorld, ProbeData&& probeData)
{
  const wdUInt32 uiWorldIndex = pWorld->GetIndex();

  if (uiWorldIndex >= s_pData->m_WorldReflectionData.GetCount())
    s_pData->m_WorldReflectionData.SetCount(uiWorldIndex + 1);

  if (s_pData->m_WorldReflectionData[uiWorldIndex] == nullptr)
  {
    s_pData->m_WorldReflectionData[uiWorldIndex] = WD_DEFAULT_NEW(WorldReflectionData);
    s_pData->m_WorldReflectionData[uiWorldIndex]->m_mappingSubscriptionId = s_pData->m_WorldReflectionData[uiWorldIndex]->m_mapping.m_Events.AddEventHandler([uiWorldIndex, this](const wdReflectionProbeMappingEvent& e) {
      OnReflectionProbeMappingEvent(uiWorldIndex, e);
    });
  }

  wdReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[uiWorldIndex];

  const wdBitflags<wdProbeFlags> flags = probeData.m_Flags;
  wdReflectionProbeId id = data.m_Probes.Insert(std::move(probeData));

  if (probeData.m_Flags.IsSet(wdProbeFlags::SkyLight))
  {
    data.m_SkyLight = id;
  }
  data.m_mapping.AddProbe(id, flags);

  return id;
}

wdReflectionPool::Data::WorldReflectionData& wdReflectionPool::Data::GetWorldData(const wdWorld* pWorld)
{
  const wdUInt32 uiWorldIndex = pWorld->GetIndex();
  return *s_pData->m_WorldReflectionData[uiWorldIndex];
}

void wdReflectionPool::Data::RemoveProbe(const wdWorld* pWorld, wdReflectionProbeId id)
{
  const wdUInt32 uiWorldIndex = pWorld->GetIndex();
  wdReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[uiWorldIndex];

  data.m_mapping.RemoveProbe(id);

  if (data.m_SkyLight == id)
  {
    data.m_SkyLight.Invalidate();
  }

  data.m_Probes.Remove(id);

  if (data.m_Probes.IsEmpty())
  {
    s_pData->m_WorldReflectionData[uiWorldIndex]->m_mapping.m_Events.RemoveEventHandler(s_pData->m_WorldReflectionData[uiWorldIndex]->m_mappingSubscriptionId);
    s_pData->m_WorldReflectionData[uiWorldIndex].Clear();
  }
}

void wdReflectionPool::Data::UpdateProbeData(ProbeData& ref_probeData, const wdReflectionProbeDesc& desc, const wdReflectionProbeComponentBase* pComponent)
{
  ref_probeData.m_desc = desc;
  ref_probeData.m_GlobalTransform = pComponent->GetOwner()->GetGlobalTransform();

  if (const wdSphereReflectionProbeComponent* pSphere = wdDynamicCast<const wdSphereReflectionProbeComponent*>(pComponent))
  {
    ref_probeData.m_Flags = wdProbeFlags::Sphere;
  }
  else if (const wdBoxReflectionProbeComponent* pBox = wdDynamicCast<const wdBoxReflectionProbeComponent*>(pComponent))
  {
    ref_probeData.m_Flags = wdProbeFlags::Box;
  }

  if (ref_probeData.m_desc.m_Mode == wdReflectionProbeMode::Dynamic)
  {
    ref_probeData.m_Flags |= wdProbeFlags::Dynamic;
  }
  else
  {
    wdStringBuilder sComponentGuid, sCubeMapFile;
    wdConversionUtils::ToString(ref_probeData.m_desc.m_uniqueID, sComponentGuid);

    // this is where the editor will put the file for this probe
    sCubeMapFile.Format(":project/AssetCache/Generated/{0}.wdTexture", sComponentGuid);

    ref_probeData.m_hCubeMap = wdResourceManager::LoadResource<wdTextureCubeResource>(sCubeMapFile);
  }
}

bool wdReflectionPool::Data::UpdateSkyLightData(ProbeData& ref_probeData, const wdReflectionProbeDesc& desc, const wdSkyLightComponent* pComponent)
{
  bool bProbeTypeChanged = false;
  if (ref_probeData.m_desc.m_Mode != desc.m_Mode)
  {
    //#TODO any other reason to unmap a probe.
    bProbeTypeChanged = true;
  }

  ref_probeData.m_desc = desc;
  ref_probeData.m_GlobalTransform = pComponent->GetOwner()->GetGlobalTransform();

  if (auto pSkyLight = wdDynamicCast<const wdSkyLightComponent*>(pComponent))
  {
    ref_probeData.m_Flags = wdProbeFlags::SkyLight;
    ref_probeData.m_hCubeMap = pSkyLight->GetCubeMap();
    if (ref_probeData.m_desc.m_Mode == wdReflectionProbeMode::Dynamic)
    {
      ref_probeData.m_Flags |= wdProbeFlags::Dynamic;
    }
    else
    {
      if (ref_probeData.m_hCubeMap.IsValid())
      {
        ref_probeData.m_Flags |= wdProbeFlags::HasCustomCubeMap;
      }
      else
      {
        wdStringBuilder sComponentGuid, sCubeMapFile;
        wdConversionUtils::ToString(ref_probeData.m_desc.m_uniqueID, sComponentGuid);

        // this is where the editor will put the file for this probe
        sCubeMapFile.Format(":project/AssetCache/Generated/{0}.wdTexture", sComponentGuid);

        ref_probeData.m_hCubeMap = wdResourceManager::LoadResource<wdTextureCubeResource>(sCubeMapFile);
      }
    }
  }
  return bProbeTypeChanged;
}

void wdReflectionPool::Data::OnReflectionProbeMappingEvent(const wdUInt32 uiWorldIndex, const wdReflectionProbeMappingEvent& e)
{
  switch (e.m_Type)
  {
    case wdReflectionProbeMappingEvent::Type::ProbeMapped:
      break;
    case wdReflectionProbeMappingEvent::Type::ProbeUnmapped:
    {
      wdReflectionProbeRef probeUpdate = {uiWorldIndex, e.m_Id};
      if (m_PendingDynamicUpdate.Contains(probeUpdate))
      {
        m_PendingDynamicUpdate.Remove(probeUpdate);
        m_DynamicUpdateQueue.RemoveAndCopy(probeUpdate);
      }

      if (m_ActiveDynamicUpdate.Contains(probeUpdate))
      {
        m_ActiveDynamicUpdate.Remove(probeUpdate);
        m_ReflectionProbeUpdater.CancelUpdate(probeUpdate);
      }
    }
    break;
    case wdReflectionProbeMappingEvent::Type::ProbeUpdateRequested:
    {
      // For now, we just manage a FIFO queue of all dynamic probes that have a high enough priority.
      const wdReflectionProbeRef du = {uiWorldIndex, e.m_Id};
      wdReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[uiWorldIndex];
      if (!m_PendingDynamicUpdate.Contains(du))
      {
        m_PendingDynamicUpdate.Insert(du);
        m_DynamicUpdateQueue.PushBack(du);
      }
    }
    break;
  }
}

//////////////////////////////////////////////////////////////////////////
/// Dynamic Update

void wdReflectionPool::Data::PreExtraction()
{
  WD_LOCK(s_pData->m_Mutex);
  const wdUInt32 uiWorldCount = s_pData->m_WorldReflectionData.GetCount();

  for (wdUInt32 uiWorld = 0; uiWorld < uiWorldCount; uiWorld++)
  {
    if (s_pData->m_WorldReflectionData[uiWorld] == nullptr)
      continue;

    wdReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[uiWorld];
    data.m_mapping.PreExtraction();
  }


  // Schedule new dynamic updates
  {
    wdHybridArray<wdReflectionProbeRef, 4> updatesFinished;
    const wdUInt32 uiCount = wdMath::Min(m_ReflectionProbeUpdater.GetFreeUpdateSlots(updatesFinished), m_DynamicUpdateQueue.GetCount());
    for (const wdReflectionProbeRef& probe : updatesFinished)
    {
      m_ActiveDynamicUpdate.Remove(probe);

      if (s_pData->m_WorldReflectionData[probe.m_uiWorldIndex] == nullptr)
        continue;

      wdReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[probe.m_uiWorldIndex];
      data.m_mapping.ProbeUpdateFinished(probe.m_Id);
    }

    for (wdUInt32 i = 0; i < uiCount; i++)
    {
      wdReflectionProbeRef nextUpdate = m_DynamicUpdateQueue.PeekFront();
      m_DynamicUpdateQueue.PopFront();
      m_PendingDynamicUpdate.Remove(nextUpdate);

      if (s_pData->m_WorldReflectionData[nextUpdate.m_uiWorldIndex] == nullptr)
        continue;

      wdReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[nextUpdate.m_uiWorldIndex];
      ProbeData& probeData = data.m_Probes.GetValueUnchecked(nextUpdate.m_Id.m_InstanceIndex);

      wdReflectionProbeUpdater::TargetSlot target;
      target.m_hSpecularOutputTexture = data.m_mapping.GetTexture();
      target.m_iSpecularOutputIndex = data.m_mapping.GetReflectionIndex(nextUpdate.m_Id);

      if (probeData.m_Flags.IsSet(wdProbeFlags::SkyLight))
      {
        target.m_hIrradianceOutputTexture = m_hSkyIrradianceTexture;
        target.m_iIrradianceOutputIndex = nextUpdate.m_uiWorldIndex;
      }

      if (probeData.m_Flags.IsSet(wdProbeFlags::HasCustomCubeMap))
      {
        WD_ASSERT_DEBUG(probeData.m_hCubeMap.IsValid(), "");
        WD_VERIFY(m_ReflectionProbeUpdater.StartFilterUpdate(nextUpdate, probeData.m_desc, probeData.m_hCubeMap, target).Succeeded(), "GetFreeUpdateSlots returned incorrect result");
      }
      else
      {
        WD_VERIFY(m_ReflectionProbeUpdater.StartDynamicUpdate(nextUpdate, probeData.m_desc, probeData.m_GlobalTransform, target).Succeeded(), "GetFreeUpdateSlots returned incorrect result");
      }
      m_ActiveDynamicUpdate.Insert(nextUpdate);
    }
    m_ReflectionProbeUpdater.GenerateUpdateSteps();
  }
}

void wdReflectionPool::Data::PostExtraction()
{
  WD_LOCK(s_pData->m_Mutex);
  const wdUInt32 uiWorldCount = s_pData->m_WorldReflectionData.GetCount();
  for (wdUInt32 uiWorld = 0; uiWorld < uiWorldCount; uiWorld++)
  {
    if (s_pData->m_WorldReflectionData[uiWorld] == nullptr)
      continue;
    wdReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[uiWorld];
    data.m_mapping.PostExtraction();
  }
}

//////////////////////////////////////////////////////////////////////////
/// Resource Creation

void wdReflectionPool::Data::CreateReflectionViewsAndResources()
{
  if (m_hFallbackReflectionSpecularTexture.IsInvalidated())
  {
    wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

    wdGALTextureCreationDescription desc;
    desc.m_uiWidth = s_uiReflectionCubeMapSize;
    desc.m_uiHeight = s_uiReflectionCubeMapSize;
    desc.m_uiMipLevelCount = GetMipLevels();
    desc.m_uiArraySize = 1;
    desc.m_Format = wdGALResourceFormat::RGBAHalf;
    desc.m_Type = wdGALTextureType::TextureCube;
    desc.m_bCreateRenderTarget = true;
    desc.m_bAllowUAV = true;
    desc.m_ResourceAccess.m_bReadBack = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hFallbackReflectionSpecularTexture = pDevice->CreateTexture(desc);
    if (!m_hFallbackReflectionSpecularTexture.IsInvalidated())
    {
      pDevice->GetTexture(m_hFallbackReflectionSpecularTexture)->SetDebugName("Reflection Fallback Specular Texture");
    }
  }

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  if (!m_hDebugSphere.IsValid())
  {
    wdGeometry geom;
    geom.AddSphere(s_fDebugSphereRadius, 32, 16);

    const char* szBufferResourceName = "ReflectionProbeDebugSphereBuffer";
    wdMeshBufferResourceHandle hMeshBuffer = wdResourceManager::GetExistingResource<wdMeshBufferResource>(szBufferResourceName);
    if (!hMeshBuffer.IsValid())
    {
      wdMeshBufferResourceDescriptor desc;
      desc.AddStream(wdGALVertexAttributeSemantic::Position, wdGALResourceFormat::XYZFloat);
      desc.AddStream(wdGALVertexAttributeSemantic::Normal, wdGALResourceFormat::XYZFloat);
      desc.AllocateStreamsFromGeometry(geom, wdGALPrimitiveTopology::Triangles);

      hMeshBuffer = wdResourceManager::GetOrCreateResource<wdMeshBufferResource>(szBufferResourceName, std::move(desc), szBufferResourceName);
    }

    const char* szMeshResourceName = "ReflectionProbeDebugSphere";
    m_hDebugSphere = wdResourceManager::GetExistingResource<wdMeshResource>(szMeshResourceName);
    if (!m_hDebugSphere.IsValid())
    {
      wdMeshResourceDescriptor desc;
      desc.UseExistingMeshBuffer(hMeshBuffer);
      desc.AddSubMesh(geom.CalculateTriangleCount(), 0, 0);
      desc.ComputeBounds();

      m_hDebugSphere = wdResourceManager::GetOrCreateResource<wdMeshResource>(szMeshResourceName, std::move(desc), szMeshResourceName);
    }
  }

  if (m_hDebugMaterial.IsEmpty())
  {
    const wdUInt32 uiMipLevelCount = GetMipLevels();

    wdMaterialResourceHandle hDebugMaterial = wdResourceManager::LoadResource<wdMaterialResource>(
      "{ 6f8067d0-ece8-44e1-af46-79b49266de41 }"); // ReflectionProbeVisualization.wdMaterialAsset
    wdResourceLock<wdMaterialResource> pMaterial(hDebugMaterial, wdResourceAcquireMode::BlockTillLoaded);
    if (pMaterial->GetLoadingState() != wdResourceState::Loaded)
      return;

    wdMaterialResourceDescriptor desc = pMaterial->GetCurrentDesc();
    wdUInt32 uiMipLevel = desc.m_Parameters.GetCount();
    wdUInt32 uiReflectionProbeIndex = desc.m_Parameters.GetCount();
    wdTempHashedString sMipLevelParam = "MipLevel";
    wdTempHashedString sReflectionProbeIndexParam = "ReflectionProbeIndex";
    for (wdUInt32 i = 0; i < desc.m_Parameters.GetCount(); ++i)
    {
      if (desc.m_Parameters[i].m_Name == sMipLevelParam)
      {
        uiMipLevel = i;
      }
      if (desc.m_Parameters[i].m_Name == sReflectionProbeIndexParam)
      {
        uiReflectionProbeIndex = i;
      }
    }

    if (uiMipLevel >= desc.m_Parameters.GetCount() || uiReflectionProbeIndex >= desc.m_Parameters.GetCount())
      return;

    m_hDebugMaterial.SetCount(uiMipLevelCount * s_uiNumReflectionProbeCubeMaps);
    for (wdUInt32 iReflectionProbeIndex = 0; iReflectionProbeIndex < s_uiNumReflectionProbeCubeMaps; iReflectionProbeIndex++)
    {
      for (wdUInt32 iMipLevel = 0; iMipLevel < uiMipLevelCount; iMipLevel++)
      {
        desc.m_Parameters[uiMipLevel].m_Value = iMipLevel;
        desc.m_Parameters[uiReflectionProbeIndex].m_Value = iReflectionProbeIndex;
        wdStringBuilder sMaterialName;
        sMaterialName.Format("ReflectionProbeVisualization - MipLevel {}, Index {}", iMipLevel, iReflectionProbeIndex);

        wdMaterialResourceDescriptor desc2 = desc;
        m_hDebugMaterial[iReflectionProbeIndex * uiMipLevelCount + iMipLevel] = wdResourceManager::GetOrCreateResource<wdMaterialResource>(sMaterialName, std::move(desc2));
      }
    }
  }
#endif
}

void wdReflectionPool::Data::CreateSkyIrradianceTexture()
{
  if (m_hSkyIrradianceTexture.IsInvalidated())
  {
    wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

    wdGALTextureCreationDescription desc;
    desc.m_uiWidth = 6;
    desc.m_uiHeight = 64;
    desc.m_Format = wdGALResourceFormat::RGBAHalf;
    desc.m_Type = wdGALTextureType::Texture2D;
    desc.m_bCreateRenderTarget = true;
    desc.m_bAllowUAV = true;

    m_hSkyIrradianceTexture = pDevice->CreateTexture(desc);
    pDevice->GetTexture(m_hSkyIrradianceTexture)->SetDebugName("Sky Irradiance Texture");
  }
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ReflectionPoolData);
