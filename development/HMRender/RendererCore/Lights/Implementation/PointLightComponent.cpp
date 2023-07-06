#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdPointLightRenderData, 1, wdRTTIDefaultAllocator<wdPointLightRenderData>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_BEGIN_COMPONENT_TYPE(wdPointLightComponent, 2, wdComponentMode::Static)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new wdClampValueAttribute(0.0f, wdVariant()), new wdDefaultValueAttribute(0.0f), new wdSuffixAttribute(" m"), new wdMinValueTextAttribute("Auto")),
    //WD_ACCESSOR_PROPERTY("ProjectedTexture", GetProjectedTextureFile, SetProjectedTextureFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Texture_Cube")),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_MESSAGEHANDLERS
  {
    WD_MESSAGE_HANDLER(wdMsgExtractRenderData, OnMsgExtractRenderData),
  }
  WD_END_MESSAGEHANDLERS;
  WD_BEGIN_ATTRIBUTES
  {
    new wdSphereManipulatorAttribute("Range"),
    new wdPointLightVisualizerAttribute("Range", "Intensity", "LightColor"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_COMPONENT_TYPE
// clang-format on

wdPointLightComponent::wdPointLightComponent()
{
  m_fEffectiveRange = CalculateEffectiveRange(m_fRange, m_fIntensity);
}

wdPointLightComponent::~wdPointLightComponent() = default;

wdResult wdPointLightComponent::GetLocalBounds(wdBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, wdMsgUpdateLocalBounds& ref_msg)
{
  m_fEffectiveRange = CalculateEffectiveRange(m_fRange, m_fIntensity);

  ref_bounds = wdBoundingSphere(wdVec3::ZeroVector(), m_fEffectiveRange);
  return WD_SUCCESS;
}

void wdPointLightComponent::SetRange(float fRange)
{
  m_fRange = fRange;

  TriggerLocalBoundsUpdate();
}

float wdPointLightComponent::GetRange() const
{
  return m_fRange;
}

float wdPointLightComponent::GetEffectiveRange() const
{
  return m_fEffectiveRange;
}

void wdPointLightComponent::SetProjectedTexture(const wdTextureCubeResourceHandle& hProjectedTexture)
{
  m_hProjectedTexture = hProjectedTexture;

  InvalidateCachedRenderData();
}

const wdTextureCubeResourceHandle& wdPointLightComponent::GetProjectedTexture() const
{
  return m_hProjectedTexture;
}

void wdPointLightComponent::SetProjectedTextureFile(const char* szFile)
{
  wdTextureCubeResourceHandle hProjectedTexture;

  if (!wdStringUtils::IsNullOrEmpty(szFile))
  {
    hProjectedTexture = wdResourceManager::LoadResource<wdTextureCubeResource>(szFile);
  }

  SetProjectedTexture(hProjectedTexture);
}

const char* wdPointLightComponent::GetProjectedTextureFile() const
{
  if (!m_hProjectedTexture.IsValid())
    return "";

  return m_hProjectedTexture.GetResourceID();
}

void wdPointLightComponent::OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const
{
  // Don't extract light render data for selection or in shadow views.
  if (msg.m_OverrideCategory != wdInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == wdCameraUsageHint::Shadow)
    return;

  if (m_fIntensity <= 0.0f || m_fEffectiveRange <= 0.0f)
    return;

  wdTransform t = GetOwner()->GetGlobalTransform();

  float fScreenSpaceSize = CalculateScreenSpaceSize(wdBoundingSphere(t.m_vPosition, m_fEffectiveRange * 0.5f), *msg.m_pView->GetCullingCamera());

  auto pRenderData = wdCreateRenderDataForThisFrame<wdPointLightRenderData>(GetOwner());

  pRenderData->m_GlobalTransform = t;
  pRenderData->m_LightColor = m_LightColor;
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_fRange = m_fEffectiveRange;
  pRenderData->m_hProjectedTexture = m_hProjectedTexture;
  pRenderData->m_uiShadowDataOffset = m_bCastShadows ? wdShadowPool::AddPointLight(this, fScreenSpaceSize, msg.m_pView) : wdInvalidIndex;

  pRenderData->FillBatchIdAndSortingKey(fScreenSpaceSize);

  wdRenderData::Caching::Enum caching = m_bCastShadows ? wdRenderData::Caching::Never : wdRenderData::Caching::IfStatic;
  msg.AddRenderData(pRenderData, wdDefaultRenderDataCategories::Light, caching);
}

void wdPointLightComponent::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  wdStreamWriter& s = inout_stream.GetStream();

  s << m_fRange;
  s << m_hProjectedTexture;
}

void wdPointLightComponent::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const wdUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  wdStreamReader& s = inout_stream.GetStream();

  s >> m_fRange;
  s >> m_hProjectedTexture;
}

//////////////////////////////////////////////////////////////////////////

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdPointLightVisualizerAttribute, 1, wdRTTIDefaultAllocator<wdPointLightVisualizerAttribute>)
WD_END_DYNAMIC_REFLECTED_TYPE;

wdPointLightVisualizerAttribute::wdPointLightVisualizerAttribute()
  : wdVisualizerAttribute(nullptr)
{
}

wdPointLightVisualizerAttribute::wdPointLightVisualizerAttribute(
  const char* szRangeProperty, const char* szIntensityProperty, const char* szColorProperty)
  : wdVisualizerAttribute(szRangeProperty, szIntensityProperty, szColorProperty)
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class wdPointLightComponentPatch_1_2 : public wdGraphPatch
{
public:
  wdPointLightComponentPatch_1_2()
    : wdGraphPatch("wdPointLightComponent", 2)
  {
  }

  virtual void Patch(wdGraphPatchContext& ref_context, wdAbstractObjectGraph* pGraph, wdAbstractObjectNode* pNode) const override
  {
    ref_context.PatchBaseClass("wdLightComponent", 2, true);
  }
};

wdPointLightComponentPatch_1_2 g_wdPointLightComponentPatch_1_2;

WD_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_PointLightComponent);
