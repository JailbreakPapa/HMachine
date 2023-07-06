#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
WD_BEGIN_COMPONENT_TYPE(wdAmbientLightComponent, 2, wdComponentMode::Static)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("TopColor", GetTopColor, SetTopColor)->AddAttributes(new wdDefaultValueAttribute(wdColorGammaUB(wdColor(0.2f, 0.2f, 0.3f)))),
    WD_ACCESSOR_PROPERTY("BottomColor", GetBottomColor, SetBottomColor)->AddAttributes(new wdDefaultValueAttribute(wdColorGammaUB(wdColor(0.1f, 0.1f, 0.15f)))),
    WD_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new wdClampValueAttribute(0.0f, wdVariant()), new wdDefaultValueAttribute(1.0f))
  }
  WD_END_PROPERTIES;
  WD_BEGIN_MESSAGEHANDLERS
  {
    WD_MESSAGE_HANDLER(wdMsgUpdateLocalBounds, OnUpdateLocalBounds),
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

wdAmbientLightComponent::wdAmbientLightComponent() = default;
wdAmbientLightComponent::~wdAmbientLightComponent() = default;

void wdAmbientLightComponent::Deinitialize()
{
  wdRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());

  SUPER::Deinitialize();
}

void wdAmbientLightComponent::OnActivated()
{
  GetOwner()->UpdateLocalBounds();

  UpdateSkyIrradiance();
}

void wdAmbientLightComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();

  wdReflectionPool::ResetConstantSkyIrradiance(GetWorld());
}

void wdAmbientLightComponent::SetTopColor(wdColorGammaUB color)
{
  m_TopColor = color;

  if (IsActiveAndInitialized())
  {
    UpdateSkyIrradiance();
  }
}

wdColorGammaUB wdAmbientLightComponent::GetTopColor() const
{
  return m_TopColor;
}

void wdAmbientLightComponent::SetBottomColor(wdColorGammaUB color)
{
  m_BottomColor = color;

  if (IsActiveAndInitialized())
  {
    UpdateSkyIrradiance();
  }
}

wdColorGammaUB wdAmbientLightComponent::GetBottomColor() const
{
  return m_BottomColor;
}

void wdAmbientLightComponent::SetIntensity(float fIntensity)
{
  m_fIntensity = fIntensity;

  if (IsActiveAndInitialized())
  {
    UpdateSkyIrradiance();
  }
}

float wdAmbientLightComponent::GetIntensity() const
{
  return m_fIntensity;
}

void wdAmbientLightComponent::OnUpdateLocalBounds(wdMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? wdDefaultSpatialDataCategories::RenderDynamic : wdDefaultSpatialDataCategories::RenderStatic);
}

void wdAmbientLightComponent::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  wdStreamWriter& s = inout_stream.GetStream();

  s << m_TopColor;
  s << m_BottomColor;
  s << m_fIntensity;
}

void wdAmbientLightComponent::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const wdUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  wdStreamReader& s = inout_stream.GetStream();

  s >> m_TopColor;
  s >> m_BottomColor;
  s >> m_fIntensity;
}

void wdAmbientLightComponent::UpdateSkyIrradiance()
{
  wdColor topColor = wdColor(m_TopColor) * m_fIntensity;
  wdColor bottomColor = wdColor(m_BottomColor) * m_fIntensity;
  wdColor midColor = wdMath::Lerp(bottomColor, topColor, 0.5f);

  wdAmbientCube<wdColor> ambientLightIrradiance;
  ambientLightIrradiance.m_Values[wdAmbientCubeBasis::PosX] = midColor;
  ambientLightIrradiance.m_Values[wdAmbientCubeBasis::NegX] = midColor;
  ambientLightIrradiance.m_Values[wdAmbientCubeBasis::PosY] = midColor;
  ambientLightIrradiance.m_Values[wdAmbientCubeBasis::NegY] = midColor;
  ambientLightIrradiance.m_Values[wdAmbientCubeBasis::PosZ] = topColor;
  ambientLightIrradiance.m_Values[wdAmbientCubeBasis::NegZ] = bottomColor;

  wdReflectionPool::SetConstantSkyIrradiance(GetWorld(), ambientLightIrradiance);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class wdAmbientLightComponentPatch_1_2 : public wdGraphPatch
{
public:
  wdAmbientLightComponentPatch_1_2()
    : wdGraphPatch("wdAmbientLightComponent", 2)
  {
  }

  virtual void Patch(wdGraphPatchContext& ref_context, wdAbstractObjectGraph* pGraph, wdAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Top Color", "TopColor");
    pNode->RenameProperty("Bottom Color", "BottomColor");
  }
};

wdAmbientLightComponentPatch_1_2 g_wdAmbientLightComponentPatch_1_2;



WD_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_AmbientLightComponent);
