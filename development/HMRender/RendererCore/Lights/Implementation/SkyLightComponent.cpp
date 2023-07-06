#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/SkyLightComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/TextureCubeResource.h>

namespace
{
  static wdVariantArray GetDefaultTags()
  {
    wdVariantArray value(wdStaticAllocatorWrapper::GetAllocator());
    value.PushBack(wdStringView("SkyLight"));
    return value;
  }
} // namespace

// clang-format off
WD_BEGIN_COMPONENT_TYPE(wdSkyLightComponent, 3, wdComponentMode::Static)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ENUM_ACCESSOR_PROPERTY("ReflectionProbeMode", wdReflectionProbeMode, GetReflectionProbeMode, SetReflectionProbeMode)->AddAttributes(new wdDefaultValueAttribute(wdReflectionProbeMode::Dynamic), new wdGroupAttribute("Capture Description")),
    WD_ACCESSOR_PROPERTY("CubeMap", GetCubeMapFile, SetCubeMapFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Texture_Cube")),
    WD_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new wdClampValueAttribute(0.0f, wdVariant()), new wdDefaultValueAttribute(1.0f)),
    WD_ACCESSOR_PROPERTY("Saturation", GetSaturation, SetSaturation)->AddAttributes(new wdClampValueAttribute(0.0f, wdVariant()), new wdDefaultValueAttribute(1.0f)),
    WD_SET_ACCESSOR_PROPERTY("IncludeTags", GetIncludeTags, InsertIncludeTag, RemoveIncludeTag)->AddAttributes(new wdTagSetWidgetAttribute("Default"), new wdDefaultValueAttribute(GetDefaultTags())),
    WD_SET_ACCESSOR_PROPERTY("ExcludeTags", GetExcludeTags, InsertExcludeTag, RemoveExcludeTag)->AddAttributes(new wdTagSetWidgetAttribute("Default")),
    WD_ACCESSOR_PROPERTY("NearPlane", GetNearPlane, SetNearPlane)->AddAttributes(new wdDefaultValueAttribute(0.0f), new wdClampValueAttribute(0.0f, {}), new wdMinValueTextAttribute("Auto")),
    WD_ACCESSOR_PROPERTY("FarPlane", GetFarPlane, SetFarPlane)->AddAttributes(new wdDefaultValueAttribute(100.0f), new wdClampValueAttribute(0.01f, 10000.0f)),
    WD_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    WD_ACCESSOR_PROPERTY("ShowMipMaps", GetShowMipMaps, SetShowMipMaps),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_MESSAGEHANDLERS
  {
    WD_MESSAGE_HANDLER(wdMsgUpdateLocalBounds, OnUpdateLocalBounds),
    WD_MESSAGE_HANDLER(wdMsgExtractRenderData, OnMsgExtractRenderData),
    WD_MESSAGE_HANDLER(wdMsgTransformChanged, OnTransformChanged),
  }
  WD_END_MESSAGEHANDLERS;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Rendering/Lighting"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_COMPONENT_TYPE
// clang-format on

wdSkyLightComponent::wdSkyLightComponent()
{
  m_Desc.m_uniqueID.CreateNewUuid();
}

wdSkyLightComponent::~wdSkyLightComponent() = default;

void wdSkyLightComponent::OnActivated()
{
  GetOwner()->EnableStaticTransformChangesNotifications();
  m_Id = wdReflectionPool::RegisterSkyLight(GetWorld(), m_Desc, this);

  GetOwner()->UpdateLocalBounds();
}

void wdSkyLightComponent::OnDeactivated()
{
  wdReflectionPool::DeregisterSkyLight(GetWorld(), m_Id);
  m_Id.Invalidate();

  GetOwner()->UpdateLocalBounds();
}

void wdSkyLightComponent::SetReflectionProbeMode(wdEnum<wdReflectionProbeMode> mode)
{
  m_Desc.m_Mode = mode;
  m_bStatesDirty = true;
}

wdEnum<wdReflectionProbeMode> wdSkyLightComponent::GetReflectionProbeMode() const
{
  return m_Desc.m_Mode;
}

void wdSkyLightComponent::SetIntensity(float fIntensity)
{
  m_Desc.m_fIntensity = fIntensity;
  m_bStatesDirty = true;
}

float wdSkyLightComponent::GetIntensity() const
{
  return m_Desc.m_fIntensity;
}

void wdSkyLightComponent::SetSaturation(float fSaturation)
{
  m_Desc.m_fSaturation = fSaturation;
  m_bStatesDirty = true;
}

float wdSkyLightComponent::GetSaturation() const
{
  return m_Desc.m_fSaturation;
}

const wdTagSet& wdSkyLightComponent::GetIncludeTags() const
{
  return m_Desc.m_IncludeTags;
}

void wdSkyLightComponent::InsertIncludeTag(const char* szTag)
{
  m_Desc.m_IncludeTags.SetByName(szTag);
  m_bStatesDirty = true;
}

void wdSkyLightComponent::RemoveIncludeTag(const char* szTag)
{
  m_Desc.m_IncludeTags.RemoveByName(szTag);
  m_bStatesDirty = true;
}

const wdTagSet& wdSkyLightComponent::GetExcludeTags() const
{
  return m_Desc.m_ExcludeTags;
}

void wdSkyLightComponent::InsertExcludeTag(const char* szTag)
{
  m_Desc.m_ExcludeTags.SetByName(szTag);
  m_bStatesDirty = true;
}

void wdSkyLightComponent::RemoveExcludeTag(const char* szTag)
{
  m_Desc.m_ExcludeTags.RemoveByName(szTag);
  m_bStatesDirty = true;
}

void wdSkyLightComponent::SetShowDebugInfo(bool bShowDebugInfo)
{
  m_Desc.m_bShowDebugInfo = bShowDebugInfo;
  m_bStatesDirty = true;
}

bool wdSkyLightComponent::GetShowDebugInfo() const
{
  return m_Desc.m_bShowDebugInfo;
}

void wdSkyLightComponent::SetShowMipMaps(bool bShowMipMaps)
{
  m_Desc.m_bShowMipMaps = bShowMipMaps;
  m_bStatesDirty = true;
}

bool wdSkyLightComponent::GetShowMipMaps() const
{
  return m_Desc.m_bShowMipMaps;
}


void wdSkyLightComponent::SetCubeMapFile(const char* szFile)
{
  wdTextureCubeResourceHandle hCubeMap;
  if (!wdStringUtils::IsNullOrEmpty(szFile))
  {
    hCubeMap = wdResourceManager::LoadResource<wdTextureCubeResource>(szFile);
  }
  m_hCubeMap = hCubeMap;
  m_bStatesDirty = true;
}

const char* wdSkyLightComponent::GetCubeMapFile() const
{
  return m_hCubeMap.IsValid() ? m_hCubeMap.GetResourceID().GetData() : "";
}

void wdSkyLightComponent::SetNearPlane(float fNearPlane)
{
  m_Desc.m_fNearPlane = fNearPlane;
  m_bStatesDirty = true;
}

void wdSkyLightComponent::SetFarPlane(float fFarPlane)
{
  m_Desc.m_fFarPlane = fFarPlane;
  m_bStatesDirty = true;
}

void wdSkyLightComponent::OnUpdateLocalBounds(wdMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? wdDefaultSpatialDataCategories::RenderDynamic : wdDefaultSpatialDataCategories::RenderStatic);
}

void wdSkyLightComponent::OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const
{
  // Don't trigger reflection rendering in shadow or other reflection views.
  if (msg.m_pView->GetCameraUsageHint() == wdCameraUsageHint::Shadow || msg.m_pView->GetCameraUsageHint() == wdCameraUsageHint::Reflection)
    return;

  if (m_bStatesDirty)
  {
    m_bStatesDirty = false;
    wdReflectionPool::UpdateSkyLight(GetWorld(), m_Id, m_Desc, this);
  }

  wdReflectionPool::ExtractReflectionProbe(this, msg, nullptr, GetWorld(), m_Id, wdMath::MaxValue<float>());
}

void wdSkyLightComponent::OnTransformChanged(wdMsgTransformChanged& msg)
{
  m_bStatesDirty = true;
}

void wdSkyLightComponent::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  wdStreamWriter& s = inout_stream.GetStream();

  m_Desc.m_IncludeTags.Save(s);
  m_Desc.m_ExcludeTags.Save(s);
  s << m_Desc.m_Mode;
  s << m_Desc.m_bShowDebugInfo;
  s << m_Desc.m_fIntensity;
  s << m_Desc.m_fSaturation;
  s << m_hCubeMap;
  s << m_Desc.m_fNearPlane;
  s << m_Desc.m_fFarPlane;
}

void wdSkyLightComponent::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const wdUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  wdStreamReader& s = inout_stream.GetStream();

  m_Desc.m_IncludeTags.Load(s, wdTagRegistry::GetGlobalRegistry());
  m_Desc.m_ExcludeTags.Load(s, wdTagRegistry::GetGlobalRegistry());
  s >> m_Desc.m_Mode;
  s >> m_Desc.m_bShowDebugInfo;
  s >> m_Desc.m_fIntensity;
  s >> m_Desc.m_fSaturation;
  if (uiVersion >= 2)
  {
    s >> m_hCubeMap;
  }
  if (uiVersion >= 3)
  {
    s >> m_Desc.m_fNearPlane;
    s >> m_Desc.m_fFarPlane;
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class wdSkyLightComponentPatch_2_3 : public wdGraphPatch
{
public:
  wdSkyLightComponentPatch_2_3()
    : wdGraphPatch("wdSkyLightComponent", 3)
  {
  }

  virtual void Patch(wdGraphPatchContext& ref_context, wdAbstractObjectGraph* pGraph, wdAbstractObjectNode* pNode) const override
  {
    // Inline ReflectionData sub-object into the sky light itself.
    if (const wdAbstractObjectNode::Property* pProp0 = pNode->FindProperty("ReflectionData"))
    {
      if (pProp0->m_Value.IsA<wdUuid>())
      {
        if (wdAbstractObjectNode* pSubNode = pGraph->GetNode(pProp0->m_Value.Get<wdUuid>()))
        {
          for (auto pProp : pSubNode->GetProperties())
          {
            pNode->AddProperty(pProp.m_szPropertyName, pProp.m_Value);
          }
        }
      }
    }
  }
};

wdSkyLightComponentPatch_2_3 g_wdSkyLightComponentPatch_2_3;

WD_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SkyLightComponent);
