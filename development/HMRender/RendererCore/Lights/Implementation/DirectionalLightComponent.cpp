#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdDirectionalLightRenderData, 1, wdRTTIDefaultAllocator<wdDirectionalLightRenderData>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_BEGIN_COMPONENT_TYPE(wdDirectionalLightComponent, 3, wdComponentMode::Static)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("NumCascades", GetNumCascades, SetNumCascades)->AddAttributes(new wdClampValueAttribute(1, 4), new wdDefaultValueAttribute(2)),
    WD_ACCESSOR_PROPERTY("MinShadowRange", GetMinShadowRange, SetMinShadowRange)->AddAttributes(new wdClampValueAttribute(0.1f, wdVariant()), new wdDefaultValueAttribute(30.0f), new wdSuffixAttribute(" m")),
    WD_ACCESSOR_PROPERTY("FadeOutStart", GetFadeOutStart, SetFadeOutStart)->AddAttributes(new wdClampValueAttribute(0.6f, 1.0f), new wdDefaultValueAttribute(0.8f)),
    WD_ACCESSOR_PROPERTY("SplitModeWeight", GetSplitModeWeight, SetSplitModeWeight)->AddAttributes(new wdClampValueAttribute(0.0f, 1.0f), new wdDefaultValueAttribute(0.7f)),
    WD_ACCESSOR_PROPERTY("NearPlaneOffset", GetNearPlaneOffset, SetNearPlaneOffset)->AddAttributes(new wdClampValueAttribute(0.0f, wdVariant()), new wdDefaultValueAttribute(100.0f), new wdSuffixAttribute(" m")),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_MESSAGEHANDLERS
  {
    WD_MESSAGE_HANDLER(wdMsgExtractRenderData, OnMsgExtractRenderData),
  }
  WD_END_MESSAGEHANDLERS;
  WD_BEGIN_ATTRIBUTES
  {
    new wdDirectionVisualizerAttribute(wdBasisAxis::PositiveX, 1.0f, wdColor::White, "LightColor"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_COMPONENT_TYPE
// clang-format on

wdDirectionalLightComponent::wdDirectionalLightComponent() = default;
wdDirectionalLightComponent::~wdDirectionalLightComponent() = default;

wdResult wdDirectionalLightComponent::GetLocalBounds(wdBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, wdMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return WD_SUCCESS;
}

void wdDirectionalLightComponent::SetNumCascades(wdUInt32 uiNumCascades)
{
  m_uiNumCascades = wdMath::Clamp(uiNumCascades, 1u, 4u);

  InvalidateCachedRenderData();
}

wdUInt32 wdDirectionalLightComponent::GetNumCascades() const
{
  return m_uiNumCascades;
}

void wdDirectionalLightComponent::SetMinShadowRange(float fMinShadowRange)
{
  m_fMinShadowRange = wdMath::Max(fMinShadowRange, 0.0f);

  InvalidateCachedRenderData();
}

float wdDirectionalLightComponent::GetMinShadowRange() const
{
  return m_fMinShadowRange;
}

void wdDirectionalLightComponent::SetFadeOutStart(float fFadeOutStart)
{
  m_fFadeOutStart = wdMath::Clamp(fFadeOutStart, 0.0f, 1.0f);

  InvalidateCachedRenderData();
}

float wdDirectionalLightComponent::GetFadeOutStart() const
{
  return m_fFadeOutStart;
}

void wdDirectionalLightComponent::SetSplitModeWeight(float fSplitModeWeight)
{
  m_fSplitModeWeight = wdMath::Clamp(fSplitModeWeight, 0.0f, 1.0f);

  InvalidateCachedRenderData();
}

float wdDirectionalLightComponent::GetSplitModeWeight() const
{
  return m_fSplitModeWeight;
}

void wdDirectionalLightComponent::SetNearPlaneOffset(float fNearPlaneOffset)
{
  m_fNearPlaneOffset = wdMath::Max(fNearPlaneOffset, 0.0f);

  InvalidateCachedRenderData();
}

float wdDirectionalLightComponent::GetNearPlaneOffset() const
{
  return m_fNearPlaneOffset;
}

void wdDirectionalLightComponent::OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const
{
  // Don't extract light render data for selection or in shadow views.
  if (msg.m_OverrideCategory != wdInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == wdCameraUsageHint::Shadow)
    return;

  if (m_fIntensity <= 0.0f)
    return;

  auto pRenderData = wdCreateRenderDataForThisFrame<wdDirectionalLightRenderData>(GetOwner());

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_LightColor = m_LightColor;
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_uiShadowDataOffset = m_bCastShadows ? wdShadowPool::AddDirectionalLight(this, msg.m_pView) : wdInvalidIndex;

  pRenderData->FillBatchIdAndSortingKey(1.0f);

  wdRenderData::Caching::Enum caching = m_bCastShadows ? wdRenderData::Caching::Never : wdRenderData::Caching::IfStatic;
  msg.AddRenderData(pRenderData, wdDefaultRenderDataCategories::Light, caching);
}

void wdDirectionalLightComponent::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  wdStreamWriter& s = inout_stream.GetStream();

  s << m_uiNumCascades;
  s << m_fMinShadowRange;
  s << m_fFadeOutStart;
  s << m_fSplitModeWeight;
  s << m_fNearPlaneOffset;
}

void wdDirectionalLightComponent::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const wdUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  wdStreamReader& s = inout_stream.GetStream();

  if (uiVersion >= 3)
  {
    s >> m_uiNumCascades;
    s >> m_fMinShadowRange;
    s >> m_fFadeOutStart;
    s >> m_fSplitModeWeight;
    s >> m_fNearPlaneOffset;
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class wdDirectionalLightComponentPatch_1_2 : public wdGraphPatch
{
public:
  wdDirectionalLightComponentPatch_1_2()
    : wdGraphPatch("wdDirectionalLightComponent", 2)
  {
  }

  virtual void Patch(wdGraphPatchContext& ref_context, wdAbstractObjectGraph* pGraph, wdAbstractObjectNode* pNode) const override
  {
    ref_context.PatchBaseClass("wdLightComponent", 2, true);
  }
};

wdDirectionalLightComponentPatch_1_2 g_wdDirectionalLightComponentPatch_1_2;



WD_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_DirectionalLightComponent);
