#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/BakedProbes/BakingInterface.h>

// clang-format off
WD_BEGIN_STATIC_REFLECTED_TYPE(wdBakingSettings, wdNoBase, 1, wdRTTIDefaultAllocator<wdBakingSettings>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("ProbeSpacing", m_vProbeSpacing)->AddAttributes(new wdDefaultValueAttribute(wdVec3(4)), new wdClampValueAttribute(wdVec3(0.1f), wdVariant())),
    WD_MEMBER_PROPERTY("NumSamplesPerProbe", m_uiNumSamplesPerProbe)->AddAttributes(new wdDefaultValueAttribute(128), new wdClampValueAttribute(32, 1024)),
    WD_MEMBER_PROPERTY("MaxRayDistance", m_fMaxRayDistance)->AddAttributes(new wdDefaultValueAttribute(1000), new wdClampValueAttribute(1, wdVariant())),
  }
  WD_END_PROPERTIES;
}
WD_END_STATIC_REFLECTED_TYPE;
// clang-format on

static wdTypeVersion s_BakingSettingsVersion = 1;
wdResult wdBakingSettings::Serialize(wdStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_BakingSettingsVersion);

  inout_stream << m_vProbeSpacing;
  inout_stream << m_uiNumSamplesPerProbe;
  inout_stream << m_fMaxRayDistance;

  return WD_SUCCESS;
}

wdResult wdBakingSettings::Deserialize(wdStreamReader& inout_stream)
{
  const wdTypeVersion version = inout_stream.ReadVersion(s_BakingSettingsVersion);

  inout_stream >> m_vProbeSpacing;
  inout_stream >> m_uiNumSamplesPerProbe;
  inout_stream >> m_fMaxRayDistance;

  return WD_SUCCESS;
}


WD_STATICLINK_FILE(RendererCore, RendererCore_BakedProbes_Implementation_BakingInterface);
