#include <Core/CorePCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

// clang-format off
WD_BEGIN_STATIC_REFLECTED_ENUM(wdSurfaceInteractionAlignment, 2)
  WD_ENUM_CONSTANTS(wdSurfaceInteractionAlignment::SurfaceNormal, wdSurfaceInteractionAlignment::IncidentDirection, wdSurfaceInteractionAlignment::ReflectedDirection)
  WD_ENUM_CONSTANTS(wdSurfaceInteractionAlignment::ReverseSurfaceNormal, wdSurfaceInteractionAlignment::ReverseIncidentDirection, wdSurfaceInteractionAlignment::ReverseReflectedDirection)
WD_END_STATIC_REFLECTED_ENUM;

WD_BEGIN_STATIC_REFLECTED_TYPE(wdSurfaceInteraction, wdNoBase, 1, wdRTTIDefaultAllocator<wdSurfaceInteraction>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("Type", m_sInteractionType)->AddAttributes(new wdDynamicStringEnumAttribute("SurfaceInteractionTypeEnum")),
    WD_ACCESSOR_PROPERTY("Prefab", GetPrefab, SetPrefab)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Prefab", wdDependencyFlags::Package)),
    WD_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new wdExposedParametersAttribute("Prefab")),
    WD_ENUM_MEMBER_PROPERTY("Alignment", wdSurfaceInteractionAlignment, m_Alignment),
    WD_MEMBER_PROPERTY("Deviation", m_Deviation)->AddAttributes(new wdClampValueAttribute(wdVariant(wdAngle::Degree(0.0f)), wdVariant(wdAngle::Degree(90.0f)))),
    WD_MEMBER_PROPERTY("ImpulseThreshold", m_fImpulseThreshold),
    WD_MEMBER_PROPERTY("ImpulseScale", m_fImpulseScale)->AddAttributes(new wdDefaultValueAttribute(1.0f)),
  }
  WD_END_PROPERTIES;
}
WD_END_STATIC_REFLECTED_TYPE;

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdSurfaceResourceDescriptor, 2, wdRTTIDefaultAllocator<wdSurfaceResourceDescriptor>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("BaseSurface", GetBaseSurfaceFile, SetBaseSurfaceFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Surface")),// package+thumbnail so that it forbids circular dependencies
    WD_MEMBER_PROPERTY("Restitution", m_fPhysicsRestitution)->AddAttributes(new wdDefaultValueAttribute(0.25f)),
    WD_MEMBER_PROPERTY("StaticFriction", m_fPhysicsFrictionStatic)->AddAttributes(new wdDefaultValueAttribute(0.6f)),
    WD_MEMBER_PROPERTY("DynamicFriction", m_fPhysicsFrictionDynamic)->AddAttributes(new wdDefaultValueAttribute(0.4f)),
    WD_ACCESSOR_PROPERTY("OnCollideInteraction", GetCollisionInteraction, SetCollisionInteraction)->AddAttributes(new wdDynamicStringEnumAttribute("SurfaceInteractionTypeEnum")),
    WD_ACCESSOR_PROPERTY("SlideReaction", GetSlideReactionPrefabFile, SetSlideReactionPrefabFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Prefab", wdDependencyFlags::Package)),
    WD_ACCESSOR_PROPERTY("RollReaction", GetRollReactionPrefabFile, SetRollReactionPrefabFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Prefab", wdDependencyFlags::Package)),
    WD_ARRAY_MEMBER_PROPERTY("Interactions", m_Interactions),
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void wdSurfaceInteraction::SetPrefab(const char* szPrefab)
{
  wdPrefabResourceHandle hPrefab;

  if (!wdStringUtils::IsNullOrEmpty(szPrefab))
  {
    hPrefab = wdResourceManager::LoadResource<wdPrefabResource>(szPrefab);
  }

  m_hPrefab = hPrefab;
}

const char* wdSurfaceInteraction::GetPrefab() const
{
  if (!m_hPrefab.IsValid())
    return "";

  return m_hPrefab.GetResourceID();
}

const wdRangeView<const char*, wdUInt32> wdSurfaceInteraction::GetParameters() const
{
  return wdRangeView<const char*, wdUInt32>([]() -> wdUInt32 { return 0; },
    [this]() -> wdUInt32 { return m_Parameters.GetCount(); },
    [](wdUInt32& ref_uiIt) { ++ref_uiIt; },
    [this](const wdUInt32& uiIt) -> const char* { return m_Parameters.GetKey(uiIt).GetString().GetData(); });
}

void wdSurfaceInteraction::SetParameter(const char* szKey, const wdVariant& value)
{
  wdHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != wdInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;
}

void wdSurfaceInteraction::RemoveParameter(const char* szKey)
{
  m_Parameters.RemoveAndCopy(wdTempHashedString(szKey));
}

bool wdSurfaceInteraction::GetParameter(const char* szKey, wdVariant& out_value) const
{
  wdUInt32 it = m_Parameters.Find(szKey);

  if (it == wdInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}

void wdSurfaceResourceDescriptor::Load(wdStreamReader& inout_stream)
{
  wdUInt8 uiVersion = 0;

  inout_stream >> uiVersion;
  WD_ASSERT_DEV(uiVersion <= 7, "Invalid version {0} for surface resource", uiVersion);

  inout_stream >> m_fPhysicsRestitution;
  inout_stream >> m_fPhysicsFrictionStatic;
  inout_stream >> m_fPhysicsFrictionDynamic;
  inout_stream >> m_hBaseSurface;

  if (uiVersion >= 4)
  {
    inout_stream >> m_sOnCollideInteraction;
  }

  if (uiVersion >= 7)
  {
    inout_stream >> m_sSlideInteractionPrefab;
    inout_stream >> m_sRollInteractionPrefab;
  }

  if (uiVersion > 2)
  {
    wdUInt32 count = 0;
    inout_stream >> count;
    m_Interactions.SetCount(count);

    wdStringBuilder sTemp;
    for (wdUInt32 i = 0; i < count; ++i)
    {
      auto& ia = m_Interactions[i];

      inout_stream >> sTemp;
      ia.m_sInteractionType = sTemp;

      inout_stream >> ia.m_hPrefab;
      inout_stream >> ia.m_Alignment;
      inout_stream >> ia.m_Deviation;

      if (uiVersion >= 4)
      {
        inout_stream >> ia.m_fImpulseThreshold;
      }

      if (uiVersion >= 5)
      {
        inout_stream >> ia.m_fImpulseScale;
      }

      if (uiVersion >= 6)
      {
        wdUInt8 uiNumParams;
        inout_stream >> uiNumParams;

        ia.m_Parameters.Clear();
        ia.m_Parameters.Reserve(uiNumParams);

        wdHashedString key;
        wdVariant value;

        for (wdUInt32 i2 = 0; i2 < uiNumParams; ++i2)
        {
          inout_stream >> key;
          inout_stream >> value;

          ia.m_Parameters.Insert(key, value);
        }
      }
    }
  }
}

void wdSurfaceResourceDescriptor::Save(wdStreamWriter& inout_stream) const
{
  const wdUInt8 uiVersion = 7;

  inout_stream << uiVersion;
  inout_stream << m_fPhysicsRestitution;
  inout_stream << m_fPhysicsFrictionStatic;
  inout_stream << m_fPhysicsFrictionDynamic;
  inout_stream << m_hBaseSurface;

  // version 4
  inout_stream << m_sOnCollideInteraction;

  // version 7
  inout_stream << m_sSlideInteractionPrefab;
  inout_stream << m_sRollInteractionPrefab;

  inout_stream << m_Interactions.GetCount();
  for (const auto& ia : m_Interactions)
  {
    inout_stream << ia.m_sInteractionType;
    inout_stream << ia.m_hPrefab;
    inout_stream << ia.m_Alignment;
    inout_stream << ia.m_Deviation;

    // version 4
    inout_stream << ia.m_fImpulseThreshold;

    // version 5
    inout_stream << ia.m_fImpulseScale;

    // version 6
    const wdUInt8 uiNumParams = static_cast<wdUInt8>(ia.m_Parameters.GetCount());
    inout_stream << uiNumParams;
    for (wdUInt32 i = 0; i < uiNumParams; ++i)
    {
      inout_stream << ia.m_Parameters.GetKey(i);
      inout_stream << ia.m_Parameters.GetValue(i);
    }
  }
}

void wdSurfaceResourceDescriptor::SetBaseSurfaceFile(const char* szFile)
{
  wdSurfaceResourceHandle hResource;

  if (!wdStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = wdResourceManager::LoadResource<wdSurfaceResource>(szFile);
  }

  m_hBaseSurface = hResource;
}

const char* wdSurfaceResourceDescriptor::GetBaseSurfaceFile() const
{
  if (!m_hBaseSurface.IsValid())
    return "";

  return m_hBaseSurface.GetResourceID();
}

void wdSurfaceResourceDescriptor::SetCollisionInteraction(const char* szName)
{
  m_sOnCollideInteraction.Assign(szName);
}

const char* wdSurfaceResourceDescriptor::GetCollisionInteraction() const
{
  return m_sOnCollideInteraction.GetData();
}

void wdSurfaceResourceDescriptor::SetSlideReactionPrefabFile(const char* szFile)
{
  m_sSlideInteractionPrefab.Assign(szFile);
}

const char* wdSurfaceResourceDescriptor::GetSlideReactionPrefabFile() const
{
  return m_sSlideInteractionPrefab.GetData();
}

void wdSurfaceResourceDescriptor::SetRollReactionPrefabFile(const char* szFile)
{
  m_sRollInteractionPrefab.Assign(szFile);
}

const char* wdSurfaceResourceDescriptor::GetRollReactionPrefabFile() const
{
  return m_sRollInteractionPrefab.GetData();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class wdSurfaceResourceDescriptorPatch_1_2 : public wdGraphPatch
{
public:
  wdSurfaceResourceDescriptorPatch_1_2()
    : wdGraphPatch("wdSurfaceResourceDescriptor", 2)
  {
  }

  virtual void Patch(wdGraphPatchContext& ref_context, wdAbstractObjectGraph* pGraph, wdAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Base Surface", "BaseSurface");
    pNode->RenameProperty("Static Friction", "StaticFriction");
    pNode->RenameProperty("Dynamic Friction", "DynamicFriction");
  }
};

wdSurfaceResourceDescriptorPatch_1_2 g_wdSurfaceResourceDescriptorPatch_1_2;


WD_STATICLINK_FILE(Core, Core_Physics_Implementation_SurfaceResourceDescriptor);
