#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionPoolData.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeMapping.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

wdReflectionProbeMapping::wdReflectionProbeMapping(wdUInt32 uiAtlasSize)
  : m_uiAtlasSize(uiAtlasSize)
{
  m_MappedCubes.SetCount(m_uiAtlasSize);
  m_ActiveProbes.Reserve(m_uiAtlasSize);
  m_UnusedProbeSlots.Reserve(m_uiAtlasSize);
  m_AddProbes.Reserve(m_uiAtlasSize);

  WD_ASSERT_DEV(m_hReflectionSpecularTexture.IsInvalidated(), "World data already created.");
  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();
  wdGALTextureCreationDescription desc;
  desc.m_uiWidth = s_uiReflectionCubeMapSize;
  desc.m_uiHeight = s_uiReflectionCubeMapSize;
  desc.m_uiMipLevelCount = GetMipLevels();
  desc.m_uiArraySize = s_uiNumReflectionProbeCubeMaps;
  desc.m_Format = wdGALResourceFormat::RGBAHalf;
  desc.m_Type = wdGALTextureType::TextureCube;
  desc.m_bCreateRenderTarget = true;
  desc.m_bAllowUAV = true;
  desc.m_ResourceAccess.m_bReadBack = true;
  desc.m_ResourceAccess.m_bImmutable = false;

  m_hReflectionSpecularTexture = pDevice->CreateTexture(desc);
  pDevice->GetTexture(m_hReflectionSpecularTexture)->SetDebugName("Reflection Specular Texture");
}

wdReflectionProbeMapping::~wdReflectionProbeMapping()
{
  WD_ASSERT_DEV(!m_hReflectionSpecularTexture.IsInvalidated(), "World data not created.");
  wdGALDevice::GetDefaultDevice()->DestroyTexture(m_hReflectionSpecularTexture);
  m_hReflectionSpecularTexture.Invalidate();
}

void wdReflectionProbeMapping::AddProbe(wdReflectionProbeId probe, wdBitflags<wdProbeFlags> flags)
{
  m_RegisteredProbes.EnsureCount(probe.m_InstanceIndex + 1);
  ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_InstanceIndex];
  WD_ASSERT_DEBUG(probeData.m_Flags == 0, "");
  probeData.m_id = probe;
  probeData.m_Flags.SetValue(flags.GetValue());
  probeData.m_Flags.Add(wdProbeMappingFlags::Dirty);
  if (probeData.m_Flags.IsSet(wdProbeMappingFlags::SkyLight))
  {
    m_SkyLight = probe;
    MapProbe(probe, 0);
  }
}

void wdReflectionProbeMapping::UpdateProbe(wdReflectionProbeId probe, wdBitflags<wdProbeFlags> flags)
{
  ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_InstanceIndex];
  if (!probeData.m_Flags.IsSet(wdProbeMappingFlags::SkyLight) && probeData.m_Flags.IsSet(wdProbeMappingFlags::Dynamic) != flags.IsSet(wdProbeFlags::Dynamic))
  {
    UnmapProbe(probe);
  }
  wdBitflags<wdProbeMappingFlags> preserveFlags = probeData.m_Flags & wdProbeMappingFlags::Usable;
  probeData.m_Flags.SetValue(flags.GetValue());
  probeData.m_Flags.Add(preserveFlags | wdProbeMappingFlags::Dirty);
}

void wdReflectionProbeMapping::ProbeUpdateFinished(wdReflectionProbeId probe)
{
  ProbeDataInternal& probeData0 = m_RegisteredProbes[probe.m_InstanceIndex];
  if (m_SkyLight == probe && probeData0.m_Flags.IsSet(wdProbeMappingFlags::Dirty))
  {
    // If the sky irradiance changed all other probes are no longer valid and need to be marked as dirty.
    for (ProbeDataInternal& probeData : m_RegisteredProbes)
    {
      if (!probeData.m_id.IsInvalidated() && probeData.m_id != probe)
      {
        probeData.m_Flags.Add(wdProbeMappingFlags::Dirty);
      }
    }
  }
  probeData0.m_Flags.Add(wdProbeMappingFlags::Usable);
  probeData0.m_Flags.Remove(wdProbeMappingFlags::Dirty);
}

void wdReflectionProbeMapping::RemoveProbe(wdReflectionProbeId probe)
{
  if (m_SkyLight == probe)
  {
    m_SkyLight.Invalidate();
    // If the sky irradiance changed all other probes are no longer valid and need to be marked as dirty.
    for (ProbeDataInternal& probeData : m_RegisteredProbes)
    {
      if (!probeData.m_id.IsInvalidated() && probeData.m_id != probe)
      {
        probeData.m_Flags.Add(wdProbeMappingFlags::Dirty);
      }
    }
  }
  UnmapProbe(probe);
  ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_InstanceIndex];
  probeData = {};
}

wdInt32 wdReflectionProbeMapping::GetReflectionIndex(wdReflectionProbeId probe, bool bForExtraction) const
{
  const ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_InstanceIndex];
  if (bForExtraction && !probeData.m_Flags.IsSet(wdProbeMappingFlags::Usable))
  {
    return -1;
  }
  return probeData.m_uiReflectionIndex;
}

void wdReflectionProbeMapping::PreExtraction()
{
  // Reset priorities
  for (ProbeDataInternal& probeData : m_RegisteredProbes)
  {
    probeData.m_fPriority = 0.0f;
  }
  if (!m_SkyLight.IsInvalidated())
  {
    ProbeDataInternal& probeData = m_RegisteredProbes[m_SkyLight.m_InstanceIndex];
    probeData.m_fPriority = wdMath::MaxValue<float>();
  }

  m_SortedProbes.Clear();
  m_ActiveProbes.Clear();
  m_UnusedProbeSlots.Clear();
  m_AddProbes.Clear();
}

void wdReflectionProbeMapping::AddWeight(wdReflectionProbeId probe, float fPriority)
{
  ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_InstanceIndex];
  probeData.m_fPriority = wdMath::Max(probeData.m_fPriority, fPriority);
}

void wdReflectionProbeMapping::PostExtraction()
{
  {
    // Sort all active non-skylight probes so we can find the best candidates to evict from the atlas.
    for (wdUInt32 i = 1; i < s_uiNumReflectionProbeCubeMaps; i++)
    {
      auto id = m_MappedCubes[i];
      if (!id.IsInvalidated())
      {
        m_ActiveProbes.PushBack({id, m_RegisteredProbes[id.m_InstanceIndex].m_fPriority});
      }
      else
      {
        m_UnusedProbeSlots.PushBack(i);
      }
    }
    m_ActiveProbes.Sort();
  }

  {
    // Sort all exiting probes by priority.
    m_SortedProbes.Reserve(m_RegisteredProbes.GetCount());
    for (const ProbeDataInternal& probeData : m_RegisteredProbes)
    {
      if (!probeData.m_id.IsInvalidated())
      {
        m_SortedProbes.PushBack({probeData.m_id, probeData.m_fPriority});
      }
    }
    m_SortedProbes.Sort();
  }

  {
    // Look at the first N best probes that would ideally be mapped in the atlas and find unmapped ones.
    const wdUInt32 uiMaxCount = wdMath::Min(m_uiAtlasSize, m_SortedProbes.GetCount());
    for (wdUInt32 i = 0; i < uiMaxCount; i++)
    {
      const SortedProbes& probe = m_SortedProbes[i];
      const ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_uiIndex.m_InstanceIndex];

      if (probeData.m_uiReflectionIndex < 0)
      {
        // We found a better probe to be mapped to the atlas.
        m_AddProbes.PushBack(probe);
      }
    }
  }

  {
    // Trigger resource loading of static or updates of dynamic probes.
    const wdUInt32 uiMaxCount = m_AddProbes.GetCount();
    for (wdUInt32 i = 0; i < uiMaxCount; i++)
    {
      const SortedProbes& probe = m_AddProbes[i];
      const ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_uiIndex.m_InstanceIndex];
      //#TODO static probe resource loading
    }
  }

  // Unmap probes in case we need free slots using results from last frame
  {
    // Only unmap one probe per frame
    //#TODO better heuristic to decide how many if any should be unmapped.
    if (m_UnusedProbeSlots.GetCount() == 0 && m_AddProbes.GetCount() > 0)
    {
      const SortedProbes probe = m_ActiveProbes.PeekBack();
      UnmapProbe(probe.m_uiIndex);
    }
  }

  // Map probes with higher priority
  {
    const wdUInt32 uiMaxCount = wdMath::Min(m_AddProbes.GetCount(), m_UnusedProbeSlots.GetCount());
    for (wdUInt32 i = 0; i < uiMaxCount; i++)
    {
      wdInt32 iReflectionIndex = m_UnusedProbeSlots[i];
      const SortedProbes probe = m_AddProbes[i];
      MapProbe(probe.m_uiIndex, iReflectionIndex);
    }
  }

  // Enqueue dynamic probe updates
  {
    // We add the skylight again as we want to consider it for dynamic updates.
    if (!m_SkyLight.IsInvalidated())
    {
      m_ActiveProbes.PushBack({m_SkyLight, wdMath::MaxValue<float>()});
    }
    const wdUInt32 uiMaxCount = m_ActiveProbes.GetCount();
    for (wdUInt32 i = 0; i < uiMaxCount; i++)
    {
      const SortedProbes probe = m_ActiveProbes[i];
      const ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_uiIndex.m_InstanceIndex];
      if (probeData.m_Flags.IsSet(wdProbeMappingFlags::Dynamic))
      {
        wdReflectionProbeMappingEvent e = {probeData.m_id, wdReflectionProbeMappingEvent::Type::ProbeUpdateRequested};
        m_Events.Broadcast(e);
      }
      else
      {

        //#TODO Add static probes once resources are loaded.
        if (probeData.m_Flags.IsSet(wdProbeMappingFlags::Dirty))
        {
          wdReflectionProbeMappingEvent e = {probeData.m_id, wdReflectionProbeMappingEvent::Type::ProbeUpdateRequested};
          m_Events.Broadcast(e);
        }
      }
    }
  }
}

void wdReflectionProbeMapping::MapProbe(wdReflectionProbeId id, wdInt32 iReflectionIndex)
{
  ProbeDataInternal& probeData = m_RegisteredProbes[id.m_InstanceIndex];

  probeData.m_uiReflectionIndex = iReflectionIndex;
  m_MappedCubes[probeData.m_uiReflectionIndex] = id;
  m_ActiveProbes.PushBack({id, 0.0f});

  wdReflectionProbeMappingEvent e = {id, wdReflectionProbeMappingEvent::Type::ProbeMapped};
  m_Events.Broadcast(e);
}

void wdReflectionProbeMapping::UnmapProbe(wdReflectionProbeId id)
{
  ProbeDataInternal& probeData = m_RegisteredProbes[id.m_InstanceIndex];
  if (probeData.m_uiReflectionIndex != -1)
  {
    m_MappedCubes[probeData.m_uiReflectionIndex].Invalidate();
    probeData.m_uiReflectionIndex = -1;

    wdReflectionProbeMappingEvent e = {id, wdReflectionProbeMappingEvent::Type::ProbeUnmapped};
    m_Events.Broadcast(e);
  }
}


WD_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ReflectionProbeMapping);
