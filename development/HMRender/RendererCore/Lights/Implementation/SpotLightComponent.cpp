#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Pipeline/View.h>

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
wdCVarBool cvar_RenderingLightingVisScreenSpaceSize("Rendering.Lighting.VisScreenSpaceSize", false, wdCVarFlags::Default, "Enables debug visualization of light screen space size calculation");
#endif

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdSpotLightRenderData, 1, wdRTTIDefaultAllocator<wdSpotLightRenderData>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_BEGIN_COMPONENT_TYPE(wdSpotLightComponent, 2, wdComponentMode::Static)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new wdClampValueAttribute(0.0f, wdVariant()), new wdDefaultValueAttribute(0.0f), new wdSuffixAttribute(" m"), new wdMinValueTextAttribute("Auto")),
    WD_ACCESSOR_PROPERTY("InnerSpotAngle", GetInnerSpotAngle, SetInnerSpotAngle)->AddAttributes(new wdClampValueAttribute(wdAngle::Degree(0.0f), wdAngle::Degree(179.0f)), new wdDefaultValueAttribute(wdAngle::Degree(15.0f))),
    WD_ACCESSOR_PROPERTY("OuterSpotAngle", GetOuterSpotAngle, SetOuterSpotAngle)->AddAttributes(new wdClampValueAttribute(wdAngle::Degree(0.0f), wdAngle::Degree(179.0f)), new wdDefaultValueAttribute(wdAngle::Degree(30.0f))),
    //WD_ACCESSOR_PROPERTY("ProjectedTexture", GetProjectedTextureFile, SetProjectedTextureFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_MESSAGEHANDLERS
  {
    WD_MESSAGE_HANDLER(wdMsgExtractRenderData, OnMsgExtractRenderData),
  }
  WD_END_MESSAGEHANDLERS;
  WD_BEGIN_ATTRIBUTES
  {
    new wdSpotLightVisualizerAttribute("OuterSpotAngle", "Range", "Intensity", "LightColor"),
    new wdConeLengthManipulatorAttribute("Range"),
    new wdConeAngleManipulatorAttribute("OuterSpotAngle", 1.5f),
    new wdConeAngleManipulatorAttribute("InnerSpotAngle", 1.5f),
  }
  WD_END_ATTRIBUTES;
}
WD_END_COMPONENT_TYPE
// clang-format on

wdSpotLightComponent::wdSpotLightComponent()
{
  m_fEffectiveRange = CalculateEffectiveRange(m_fRange, m_fIntensity);
}

wdSpotLightComponent::~wdSpotLightComponent() = default;

wdResult wdSpotLightComponent::GetLocalBounds(wdBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, wdMsgUpdateLocalBounds& ref_msg)
{
  m_fEffectiveRange = CalculateEffectiveRange(m_fRange, m_fIntensity);

  ref_bounds = CalculateBoundingSphere(wdTransform::IdentityTransform(), m_fEffectiveRange);
  return WD_SUCCESS;
}

void wdSpotLightComponent::SetRange(float fRange)
{
  m_fRange = fRange;

  TriggerLocalBoundsUpdate();
}

float wdSpotLightComponent::GetRange() const
{
  return m_fRange;
}

float wdSpotLightComponent::GetEffectiveRange() const
{
  return m_fEffectiveRange;
}

void wdSpotLightComponent::SetInnerSpotAngle(wdAngle spotAngle)
{
  m_InnerSpotAngle = wdMath::Clamp(spotAngle, wdAngle::Degree(0.0f), m_OuterSpotAngle);

  InvalidateCachedRenderData();
}

wdAngle wdSpotLightComponent::GetInnerSpotAngle() const
{
  return m_InnerSpotAngle;
}

void wdSpotLightComponent::SetOuterSpotAngle(wdAngle spotAngle)
{
  m_OuterSpotAngle = wdMath::Clamp(spotAngle, m_InnerSpotAngle, wdAngle::Degree(179.0f));

  TriggerLocalBoundsUpdate();
}

wdAngle wdSpotLightComponent::GetOuterSpotAngle() const
{
  return m_OuterSpotAngle;
}

void wdSpotLightComponent::SetProjectedTexture(const wdTexture2DResourceHandle& hProjectedTexture)
{
  m_hProjectedTexture = hProjectedTexture;

  InvalidateCachedRenderData();
}

const wdTexture2DResourceHandle& wdSpotLightComponent::GetProjectedTexture() const
{
  return m_hProjectedTexture;
}

void wdSpotLightComponent::SetProjectedTextureFile(const char* szFile)
{
  wdTexture2DResourceHandle hProjectedTexture;

  if (!wdStringUtils::IsNullOrEmpty(szFile))
  {
    hProjectedTexture = wdResourceManager::LoadResource<wdTexture2DResource>(szFile);
  }

  SetProjectedTexture(hProjectedTexture);
}

const char* wdSpotLightComponent::GetProjectedTextureFile() const
{
  if (!m_hProjectedTexture.IsValid())
    return "";

  return m_hProjectedTexture.GetResourceID();
}

void wdSpotLightComponent::OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const
{
  // Don't extract light render data for selection or in shadow views.
  if (msg.m_OverrideCategory != wdInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == wdCameraUsageHint::Shadow)
    return;

  if (m_fIntensity <= 0.0f || m_fEffectiveRange <= 0.0f || m_OuterSpotAngle.GetRadian() <= 0.0f)
    return;

  wdTransform t = GetOwner()->GetGlobalTransform();
  wdBoundingSphere bs = CalculateBoundingSphere(t, m_fEffectiveRange * 0.5f);

  float fScreenSpaceSize = CalculateScreenSpaceSize(bs, *msg.m_pView->GetCullingCamera());

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  if (cvar_RenderingLightingVisScreenSpaceSize)
  {
    wdStringBuilder sb;
    sb.Format("{0}", fScreenSpaceSize);
    wdDebugRenderer::Draw3DText(msg.m_pView->GetHandle(), sb, t.m_vPosition, wdColor::Olive);
    wdDebugRenderer::DrawLineSphere(msg.m_pView->GetHandle(), bs, wdColor::Olive);
  }
#endif

  auto pRenderData = wdCreateRenderDataForThisFrame<wdSpotLightRenderData>(GetOwner());

  pRenderData->m_GlobalTransform = t;
  pRenderData->m_LightColor = m_LightColor;
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_fRange = m_fEffectiveRange;
  pRenderData->m_InnerSpotAngle = m_InnerSpotAngle;
  pRenderData->m_OuterSpotAngle = m_OuterSpotAngle;
  pRenderData->m_hProjectedTexture = m_hProjectedTexture;
  pRenderData->m_uiShadowDataOffset = m_bCastShadows ? wdShadowPool::AddSpotLight(this, fScreenSpaceSize, msg.m_pView) : wdInvalidIndex;

  pRenderData->FillBatchIdAndSortingKey(fScreenSpaceSize);

  wdRenderData::Caching::Enum caching = m_bCastShadows ? wdRenderData::Caching::Never : wdRenderData::Caching::IfStatic;
  msg.AddRenderData(pRenderData, wdDefaultRenderDataCategories::Light, caching);
}

void wdSpotLightComponent::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  wdStreamWriter& s = inout_stream.GetStream();

  s << m_fRange;
  s << m_InnerSpotAngle;
  s << m_OuterSpotAngle;
  s << GetProjectedTextureFile();
}

void wdSpotLightComponent::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const wdUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  wdStreamReader& s = inout_stream.GetStream();

  s >> m_fRange;
  s >> m_InnerSpotAngle;
  s >> m_OuterSpotAngle;

  wdStringBuilder temp;
  s >> temp;
  SetProjectedTextureFile(temp);
}

wdBoundingSphere wdSpotLightComponent::CalculateBoundingSphere(const wdTransform& t, float fRange) const
{
  wdBoundingSphere res;
  wdAngle halfAngle = m_OuterSpotAngle / 2.0f;
  wdVec3 position = t.m_vPosition;
  wdVec3 forwardDir = t.m_qRotation * wdVec3(1.0f, 0.0f, 0.0f);

  if (halfAngle > wdAngle::Degree(45.0f))
  {
    res.m_vCenter = position + wdMath::Cos(halfAngle) * fRange * forwardDir;
    res.m_fRadius = wdMath::Sin(halfAngle) * fRange;
  }
  else
  {
    res.m_fRadius = fRange / (2.0f * wdMath::Cos(halfAngle));
    res.m_vCenter = position + forwardDir * res.m_fRadius;
  }

  return res;
}

//////////////////////////////////////////////////////////////////////////

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdSpotLightVisualizerAttribute, 1, wdRTTIDefaultAllocator<wdSpotLightVisualizerAttribute>)
WD_END_DYNAMIC_REFLECTED_TYPE;

wdSpotLightVisualizerAttribute::wdSpotLightVisualizerAttribute()
  : wdVisualizerAttribute(nullptr)
{
}

wdSpotLightVisualizerAttribute::wdSpotLightVisualizerAttribute(
  const char* szAngleProperty, const char* szRangeProperty, const char* szIntensityProperty, const char* szColorProperty)
  : wdVisualizerAttribute(szAngleProperty, szRangeProperty, szIntensityProperty, szColorProperty)
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class wdSpotLightComponentPatch_1_2 : public wdGraphPatch
{
public:
  wdSpotLightComponentPatch_1_2()
    : wdGraphPatch("wdSpotLightComponent", 2)
  {
  }

  virtual void Patch(wdGraphPatchContext& ref_context, wdAbstractObjectGraph* pGraph, wdAbstractObjectNode* pNode) const override
  {
    ref_context.PatchBaseClass("wdLightComponent", 2, true);

    pNode->RenameProperty("Inner Spot Angle", "InnerSpotAngle");
    pNode->RenameProperty("Outer Spot Angle", "OuterSpotAngle");
  }
};

wdSpotLightComponentPatch_1_2 g_wdSpotLightComponentPatch_1_2;


WD_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SpotLightComponent);
