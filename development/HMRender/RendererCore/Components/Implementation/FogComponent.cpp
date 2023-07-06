#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/FogComponent.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdFogRenderData, 1, wdRTTIDefaultAllocator<wdFogRenderData>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_BEGIN_COMPONENT_TYPE(wdFogComponent, 2, wdComponentMode::Static)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new wdDefaultValueAttribute(wdColorGammaUB(wdColor(0.2f, 0.2f, 0.3f)))),
    WD_ACCESSOR_PROPERTY("Density", GetDensity, SetDensity)->AddAttributes(new wdClampValueAttribute(0.0f, wdVariant()), new wdDefaultValueAttribute(1.0f)),
    WD_ACCESSOR_PROPERTY("HeightFalloff", GetHeightFalloff, SetHeightFalloff)->AddAttributes(new wdClampValueAttribute(0.0f, wdVariant()), new wdDefaultValueAttribute(10.0f)),
    WD_ACCESSOR_PROPERTY("ModulateWithSkyColor", GetModulateWithSkyColor, SetModulateWithSkyColor),
    WD_ACCESSOR_PROPERTY("SkyDistance", GetSkyDistance, SetSkyDistance)->AddAttributes(new wdClampValueAttribute(0.0f, wdVariant()), new wdDefaultValueAttribute(1000.0f)),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_MESSAGEHANDLERS
  {
    WD_MESSAGE_HANDLER(wdMsgUpdateLocalBounds, OnUpdateLocalBounds),
    WD_MESSAGE_HANDLER(wdMsgExtractRenderData, OnMsgExtractRenderData),
  }
  WD_END_MESSAGEHANDLERS;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Rendering"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_COMPONENT_TYPE
// clang-format on

wdFogComponent::wdFogComponent() = default;
wdFogComponent::~wdFogComponent() = default;

void wdFogComponent::Deinitialize()
{
  wdRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());

  SUPER::Deinitialize();
}

void wdFogComponent::OnActivated()
{
  GetOwner()->UpdateLocalBounds();
}

void wdFogComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();
}

void wdFogComponent::SetColor(wdColor color)
{
  m_Color = color;
  SetModified(WD_BIT(1));
}

wdColor wdFogComponent::GetColor() const
{
  return m_Color;
}

void wdFogComponent::SetDensity(float fDensity)
{
  m_fDensity = wdMath::Max(fDensity, 0.0f);
  SetModified(WD_BIT(2));
}

float wdFogComponent::GetDensity() const
{
  return m_fDensity;
}

void wdFogComponent::SetHeightFalloff(float fHeightFalloff)
{
  m_fHeightFalloff = wdMath::Max(fHeightFalloff, 0.0f);
  SetModified(WD_BIT(3));
}

float wdFogComponent::GetHeightFalloff() const
{
  return m_fHeightFalloff;
}

void wdFogComponent::SetModulateWithSkyColor(bool bModulate)
{
  m_bModulateWithSkyColor = bModulate;
  SetModified(WD_BIT(4));
}

bool wdFogComponent::GetModulateWithSkyColor() const
{
  return m_bModulateWithSkyColor;
}

void wdFogComponent::SetSkyDistance(float fDistance)
{
  m_fSkyDistance = fDistance;
  SetModified(WD_BIT(5));
}

float wdFogComponent::GetSkyDistance() const
{
  return m_fSkyDistance;
}

void wdFogComponent::OnUpdateLocalBounds(wdMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? wdDefaultSpatialDataCategories::RenderDynamic : wdDefaultSpatialDataCategories::RenderStatic);
}

void wdFogComponent::OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const
{
  if (msg.m_OverrideCategory != wdInvalidRenderDataCategory)
    return;

  auto pRenderData = wdCreateRenderDataForThisFrame<wdFogRenderData>(GetOwner());

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_Color = m_Color;
  pRenderData->m_fDensity = m_fDensity / 100.0f;
  pRenderData->m_fHeightFalloff = m_fHeightFalloff;
  pRenderData->m_fInvSkyDistance = m_bModulateWithSkyColor ? 1.0f / m_fSkyDistance : 0.0f;

  msg.AddRenderData(pRenderData, wdDefaultRenderDataCategories::Light, wdRenderData::Caching::IfStatic);
}

void wdFogComponent::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  wdStreamWriter& s = inout_stream.GetStream();

  s << m_Color;
  s << m_fDensity;
  s << m_fHeightFalloff;
  s << m_fSkyDistance;
  s << m_bModulateWithSkyColor;
}

void wdFogComponent::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const wdUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  wdStreamReader& s = inout_stream.GetStream();

  s >> m_Color;
  s >> m_fDensity;
  s >> m_fHeightFalloff;

  if (uiVersion >= 2)
  {
    s >> m_fSkyDistance;
    s >> m_bModulateWithSkyColor;
  }
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_FogComponent);
