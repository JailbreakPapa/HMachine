#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Lights/LightComponent.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdLightRenderData, 1, wdRTTINoAllocator)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void wdLightRenderData::FillBatchIdAndSortingKey(float fScreenSpaceSize)
{
  m_uiSortingKey = (m_uiShadowDataOffset != wdInvalidIndex) ? 0 : 1;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_ABSTRACT_COMPONENT_TYPE(wdLightComponent, 4)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("LightColor", GetLightColor, SetLightColor),
    WD_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new wdClampValueAttribute(0.0f, wdVariant()), new wdDefaultValueAttribute(10.0f)),
    WD_ACCESSOR_PROPERTY("CastShadows", GetCastShadows, SetCastShadows),
    WD_ACCESSOR_PROPERTY("PenumbraSize", GetPenumbraSize, SetPenumbraSize)->AddAttributes(new wdClampValueAttribute(0.0f, 0.5f), new wdDefaultValueAttribute(0.1f), new wdSuffixAttribute(" m")),
    WD_ACCESSOR_PROPERTY("SlopeBias", GetSlopeBias, SetSlopeBias)->AddAttributes(new wdClampValueAttribute(0.0f, 10.0f), new wdDefaultValueAttribute(0.25f)),
    WD_ACCESSOR_PROPERTY("ConstantBias", GetConstantBias, SetConstantBias)->AddAttributes(new wdClampValueAttribute(0.0f, 10.0f), new wdDefaultValueAttribute(0.1f))
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Rendering/Lighting"),
  }
  WD_END_ATTRIBUTES;
  WD_BEGIN_MESSAGEHANDLERS
  {
    WD_MESSAGE_HANDLER(wdMsgSetColor, OnMsgSetColor),
  }
  WD_END_MESSAGEHANDLERS;
}
WD_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

wdLightComponent::wdLightComponent() = default;
wdLightComponent::~wdLightComponent() = default;

void wdLightComponent::SetLightColor(wdColorGammaUB lightColor)
{
  m_LightColor = lightColor;

  InvalidateCachedRenderData();
}

wdColorGammaUB wdLightComponent::GetLightColor() const
{
  return m_LightColor;
}

void wdLightComponent::SetIntensity(float fIntensity)
{
  m_fIntensity = wdMath::Max(fIntensity, 0.0f);

  TriggerLocalBoundsUpdate();
}

float wdLightComponent::GetIntensity() const
{
  return m_fIntensity;
}

void wdLightComponent::SetCastShadows(bool bCastShadows)
{
  m_bCastShadows = bCastShadows;

  InvalidateCachedRenderData();
}

bool wdLightComponent::GetCastShadows() const
{
  return m_bCastShadows;
}

void wdLightComponent::SetPenumbraSize(float fPenumbraSize)
{
  m_fPenumbraSize = fPenumbraSize;

  InvalidateCachedRenderData();
}

float wdLightComponent::GetPenumbraSize() const
{
  return m_fPenumbraSize;
}

void wdLightComponent::SetSlopeBias(float fBias)
{
  m_fSlopeBias = fBias;

  InvalidateCachedRenderData();
}

float wdLightComponent::GetSlopeBias() const
{
  return m_fSlopeBias;
}

void wdLightComponent::SetConstantBias(float fBias)
{
  m_fConstantBias = fBias;

  InvalidateCachedRenderData();
}

float wdLightComponent::GetConstantBias() const
{
  return m_fConstantBias;
}

void wdLightComponent::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  wdStreamWriter& s = inout_stream.GetStream();

  s << m_LightColor;
  s << m_fIntensity;
  s << m_fPenumbraSize;
  s << m_fSlopeBias;
  s << m_fConstantBias;
  s << m_bCastShadows;
}

void wdLightComponent::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const wdUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  wdStreamReader& s = inout_stream.GetStream();

  s >> m_LightColor;
  s >> m_fIntensity;

  if (uiVersion >= 3)
  {
    s >> m_fPenumbraSize;
  }

  if (uiVersion >= 4)
  {
    s >> m_fSlopeBias;
    s >> m_fConstantBias;
  }

  s >> m_bCastShadows;
}

void wdLightComponent::OnMsgSetColor(wdMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_LightColor);

  InvalidateCachedRenderData();
}

// static
float wdLightComponent::CalculateEffectiveRange(float fRange, float fIntensity)
{
  const float fThreshold = 0.10f; // aggressive threshold to prevent large lights
  const float fEffectiveRange = wdMath::Sqrt(wdMath::Max(0.0f, fIntensity)) / wdMath::Sqrt(fThreshold);

  WD_ASSERT_DEBUG(!wdMath::IsNaN(fEffectiveRange), "Light range is NaN");

  if (fRange <= 0.0f)
  {
    return fEffectiveRange;
  }

  return wdMath::Min(fRange, fEffectiveRange);
}

// static
float wdLightComponent::CalculateScreenSpaceSize(const wdBoundingSphere& sphere, const wdCamera& camera)
{
  if (camera.IsPerspective())
  {
    float dist = (sphere.m_vCenter - camera.GetPosition()).GetLength();
    float fHalfHeight = wdMath::Tan(camera.GetFovY(1.0f) * 0.5f) * dist;
    return wdMath::Pow(sphere.m_fRadius / fHalfHeight, 0.8f); // tweak factor to make transitions more linear.
  }
  else
  {
    float fHalfHeight = camera.GetDimensionY(1.0f) * 0.5f;
    return sphere.m_fRadius / fHalfHeight;
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class wdLightComponentPatch_1_2 : public wdGraphPatch
{
public:
  wdLightComponentPatch_1_2()
    : wdGraphPatch("wdLightComponent", 2)
  {
  }

  virtual void Patch(wdGraphPatchContext& ref_context, wdAbstractObjectGraph* pGraph, wdAbstractObjectNode* pNode) const override { pNode->RenameProperty("Light Color", "LightColor"); }
};

wdLightComponentPatch_1_2 g_wdLightComponentPatch_1_2;



WD_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_LightComponent);
