#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/BoxReflectionProbeComponent.h>

#include <../../Data/Base/Shaders/Common/LightData.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
WD_BEGIN_COMPONENT_TYPE(wdBoxReflectionProbeComponent, 2, wdComponentMode::Static)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new wdClampValueAttribute(wdVec3(0.0f), {}), new wdDefaultValueAttribute(wdVec3(5.0f))),
    WD_ACCESSOR_PROPERTY("InfluenceScale", GetInfluenceScale, SetInfluenceScale)->AddAttributes(new wdClampValueAttribute(wdVec3(0.0f), wdVec3(1.0f)), new wdDefaultValueAttribute(wdVec3(1.0f))),
    WD_ACCESSOR_PROPERTY("InfluenceShift", GetInfluenceShift, SetInfluenceShift)->AddAttributes(new wdClampValueAttribute(wdVec3(-1.0f), wdVec3(1.0f)), new wdDefaultValueAttribute(wdVec3(0.0f))),
    WD_ACCESSOR_PROPERTY("PositiveFalloff", GetPositiveFalloff, SetPositiveFalloff)->AddAttributes(new wdClampValueAttribute(wdVec3(0.0f), wdVec3(1.0f)), new wdDefaultValueAttribute(wdVec3(0.1f, 0.1f, 0.0f))),
    WD_ACCESSOR_PROPERTY("NegativeFalloff", GetNegativeFalloff, SetNegativeFalloff)->AddAttributes(new wdClampValueAttribute(wdVec3(0.0f), wdVec3(1.0f)), new wdDefaultValueAttribute(wdVec3(0.1f, 0.1f, 0.0f))),
    WD_ACCESSOR_PROPERTY("BoxProjection", GetBoxProjection, SetBoxProjection)->AddAttributes(new wdDefaultValueAttribute(true)),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_FUNCTIONS
  {
    WD_FUNCTION_PROPERTY(OnObjectCreated),
  }
  WD_END_FUNCTIONS;
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
    new wdBoxVisualizerAttribute("Extents", 1.0f, wdColorScheme::LightUI(wdColorScheme::Blue)),
    new wdBoxManipulatorAttribute("Extents", 1.0f, true),
    new wdBoxReflectionProbeVisualizerAttribute("Extents", "InfluenceScale", "InfluenceShift"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_COMPONENT_TYPE

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdBoxReflectionProbeVisualizerAttribute, 1, wdRTTIDefaultAllocator<wdBoxReflectionProbeVisualizerAttribute>)
{
  WD_BEGIN_FUNCTIONS
  {
    WD_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*),
  }
  WD_END_FUNCTIONS;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdBoxReflectionProbeComponentManager::wdBoxReflectionProbeComponentManager(wdWorld* pWorld)
  : wdComponentManager<wdBoxReflectionProbeComponent, wdBlockStorageType::Compact>(pWorld)
{
}

//////////////////////////////////////////////////////////////////////////

wdBoxReflectionProbeComponent::wdBoxReflectionProbeComponent() = default;
wdBoxReflectionProbeComponent::~wdBoxReflectionProbeComponent() = default;

void wdBoxReflectionProbeComponent::SetExtents(const wdVec3& vExtents)
{
  m_vExtents = vExtents;
}

const wdVec3& wdBoxReflectionProbeComponent::GetInfluenceScale() const
{
  return m_vInfluenceScale;
}

void wdBoxReflectionProbeComponent::SetInfluenceScale(const wdVec3& vInfluenceScale)
{
  m_vInfluenceScale = vInfluenceScale;
}

const wdVec3& wdBoxReflectionProbeComponent::GetInfluenceShift() const
{
  return m_vInfluenceShift;
}

void wdBoxReflectionProbeComponent::SetInfluenceShift(const wdVec3& vInfluenceShift)
{
  m_vInfluenceShift = vInfluenceShift;
}

void wdBoxReflectionProbeComponent::SetPositiveFalloff(const wdVec3& vFalloff)
{
  // Does not affect cube generation so m_bStatesDirty is not set.
  m_vPositiveFalloff = vFalloff.CompClamp(wdVec3(wdMath::DefaultEpsilon<float>()), wdVec3(1.0f));
}

void wdBoxReflectionProbeComponent::SetNegativeFalloff(const wdVec3& vFalloff)
{
  // Does not affect cube generation so m_bStatesDirty is not set.
  m_vNegativeFalloff = vFalloff.CompClamp(wdVec3(wdMath::DefaultEpsilon<float>()), wdVec3(1.0f));
}

void wdBoxReflectionProbeComponent::SetBoxProjection(bool bBoxProjection)
{
  m_bBoxProjection = bBoxProjection;
}

const wdVec3& wdBoxReflectionProbeComponent::GetExtents() const
{
  return m_vExtents;
}

void wdBoxReflectionProbeComponent::OnActivated()
{
  GetOwner()->EnableStaticTransformChangesNotifications();
  m_Id = wdReflectionPool::RegisterReflectionProbe(GetWorld(), m_Desc, this);
  GetOwner()->UpdateLocalBounds();
}

void wdBoxReflectionProbeComponent::OnDeactivated()
{
  wdReflectionPool::DeregisterReflectionProbe(GetWorld(), m_Id);
  m_Id.Invalidate();

  GetOwner()->UpdateLocalBounds();
}

void wdBoxReflectionProbeComponent::OnObjectCreated(const wdAbstractObjectNode& node)
{
  m_Desc.m_uniqueID = node.GetGuid();
}

void wdBoxReflectionProbeComponent::OnUpdateLocalBounds(wdMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(wdDefaultSpatialDataCategories::RenderDynamic);
}

void wdBoxReflectionProbeComponent::OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const
{
  // Don't trigger reflection rendering in shadow or other reflection views.
  if (msg.m_pView->GetCameraUsageHint() == wdCameraUsageHint::Shadow || msg.m_pView->GetCameraUsageHint() == wdCameraUsageHint::Reflection)
    return;

  if (m_bStatesDirty)
  {
    m_bStatesDirty = false;
    wdReflectionPool::UpdateReflectionProbe(GetWorld(), m_Id, m_Desc, this);
  }

  auto pRenderData = wdCreateRenderDataForThisFrame<wdReflectionProbeRenderData>(GetOwner());
  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_vProbePosition = pRenderData->m_GlobalTransform * m_Desc.m_vCaptureOffset;
  pRenderData->m_vHalfExtents = m_vExtents / 2.0f;
  pRenderData->m_vInfluenceScale = m_vInfluenceScale;
  pRenderData->m_vInfluenceShift = m_vInfluenceShift;
  pRenderData->m_vPositiveFalloff = m_vPositiveFalloff;
  pRenderData->m_vNegativeFalloff = m_vNegativeFalloff;
  pRenderData->m_Id = m_Id;
  pRenderData->m_uiIndex = 0;
  if (m_bBoxProjection)
    pRenderData->m_uiIndex |= REFLECTION_PROBE_IS_PROJECTED;

  const wdVec3 vScale = pRenderData->m_GlobalTransform.m_vScale.CompMul(m_vExtents);
  const float fVolume = wdMath::Abs(vScale.x * vScale.y * vScale.z);

  float fPriority = ComputePriority(msg, pRenderData, fVolume, vScale);
  wdReflectionPool::ExtractReflectionProbe(this, msg, pRenderData, GetWorld(), m_Id, fPriority);
}

void wdBoxReflectionProbeComponent::OnTransformChanged(wdMsgTransformChanged& msg)
{
  m_bStatesDirty = true;
}

void wdBoxReflectionProbeComponent::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  wdStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
  s << m_vInfluenceScale;
  s << m_vInfluenceShift;
  s << m_vPositiveFalloff;
  s << m_vNegativeFalloff;
  s << m_bBoxProjection;
}

void wdBoxReflectionProbeComponent::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const wdUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  wdStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;
  s >> m_vInfluenceScale;
  s >> m_vInfluenceShift;
  s >> m_vPositiveFalloff;
  s >> m_vNegativeFalloff;
  if (uiVersion >= 2)
  {
    s >> m_bBoxProjection;
  }
}

//////////////////////////////////////////////////////////////////////////

wdBoxReflectionProbeVisualizerAttribute::wdBoxReflectionProbeVisualizerAttribute()
  : wdVisualizerAttribute(nullptr)
{
}

wdBoxReflectionProbeVisualizerAttribute::wdBoxReflectionProbeVisualizerAttribute(const char* szExtentsProperty, const char* szInfluenceScaleProperty, const char* szInfluenceShiftProperty)
  : wdVisualizerAttribute(szExtentsProperty, szInfluenceScaleProperty, szInfluenceShiftProperty)
{
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_BoxReflectionProbeComponent);
