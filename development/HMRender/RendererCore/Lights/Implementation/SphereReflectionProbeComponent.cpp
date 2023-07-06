#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/SphereReflectionProbeComponent.h>

#include <../../Data/Base/Shaders/Common/LightData.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
WD_BEGIN_COMPONENT_TYPE(wdSphereReflectionProbeComponent, 2, wdComponentMode::Static)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new wdClampValueAttribute(0.0f, {}), new wdDefaultValueAttribute(5.0f)),
    WD_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new wdClampValueAttribute(0.0f, 1.0f), new wdDefaultValueAttribute(0.1f)),
    WD_ACCESSOR_PROPERTY("SphereProjection", GetSphereProjection, SetSphereProjection)->AddAttributes(new wdDefaultValueAttribute(true)),
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
    new wdSphereVisualizerAttribute("Radius", wdColorScheme::LightUI(wdColorScheme::Blue)),
    new wdSphereManipulatorAttribute("Radius"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_COMPONENT_TYPE
// clang-format on

wdSphereReflectionProbeComponentManager::wdSphereReflectionProbeComponentManager(wdWorld* pWorld)
  : wdComponentManager<wdSphereReflectionProbeComponent, wdBlockStorageType::Compact>(pWorld)
{
}

//////////////////////////////////////////////////////////////////////////

wdSphereReflectionProbeComponent::wdSphereReflectionProbeComponent() = default;
wdSphereReflectionProbeComponent::~wdSphereReflectionProbeComponent() = default;

void wdSphereReflectionProbeComponent::SetRadius(float fRadius)
{
  m_fRadius = wdMath::Max(fRadius, 0.0f);
  m_bStatesDirty = true;
}

float wdSphereReflectionProbeComponent::GetRadius() const
{
  return m_fRadius;
}

void wdSphereReflectionProbeComponent::SetFalloff(float fFalloff)
{
  m_fFalloff = wdMath::Clamp(fFalloff, wdMath::DefaultEpsilon<float>(), 1.0f);
}

void wdSphereReflectionProbeComponent::SetSphereProjection(bool bSphereProjection)
{
  m_bSphereProjection = bSphereProjection;
}

void wdSphereReflectionProbeComponent::OnActivated()
{
  GetOwner()->EnableStaticTransformChangesNotifications();
  m_Id = wdReflectionPool::RegisterReflectionProbe(GetWorld(), m_Desc, this);
  GetOwner()->UpdateLocalBounds();
}

void wdSphereReflectionProbeComponent::OnDeactivated()
{
  wdReflectionPool::DeregisterReflectionProbe(GetWorld(), m_Id);
  m_Id.Invalidate();

  GetOwner()->UpdateLocalBounds();
}

void wdSphereReflectionProbeComponent::OnObjectCreated(const wdAbstractObjectNode& node)
{
  m_Desc.m_uniqueID = node.GetGuid();
}

void wdSphereReflectionProbeComponent::OnUpdateLocalBounds(wdMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(wdDefaultSpatialDataCategories::RenderDynamic);
}

void wdSphereReflectionProbeComponent::OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const
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
  pRenderData->m_vHalfExtents = wdVec3(m_fRadius);
  pRenderData->m_vInfluenceScale = wdVec3(1.0f);
  pRenderData->m_vInfluenceShift = wdVec3(0.0f);
  pRenderData->m_vPositiveFalloff = wdVec3(m_fFalloff);
  pRenderData->m_vNegativeFalloff = wdVec3(m_fFalloff);
  pRenderData->m_Id = m_Id;
  pRenderData->m_uiIndex = REFLECTION_PROBE_IS_SPHERE;
  if (m_bSphereProjection)
    pRenderData->m_uiIndex |= REFLECTION_PROBE_IS_PROJECTED;

  const wdVec3 vScale = pRenderData->m_GlobalTransform.m_vScale * m_fRadius;
  constexpr float fSphereConstant = (4.0f / 3.0f) * wdMath::Pi<float>();
  const float fEllipsoidVolume = fSphereConstant * wdMath::Abs(vScale.x * vScale.y * vScale.z);

  float fPriority = ComputePriority(msg, pRenderData, fEllipsoidVolume, vScale);
  wdReflectionPool::ExtractReflectionProbe(this, msg, pRenderData, GetWorld(), m_Id, fPriority);
}

void wdSphereReflectionProbeComponent::OnTransformChanged(wdMsgTransformChanged& msg)
{
  m_bStatesDirty = true;
}

void wdSphereReflectionProbeComponent::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  wdStreamWriter& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fFalloff;
  s << m_bSphereProjection;
}

void wdSphereReflectionProbeComponent::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const wdUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  wdStreamReader& s = inout_stream.GetStream();

  s >> m_fRadius;
  s >> m_fFalloff;
  if (uiVersion >= 2)
  {
    s >> m_bSphereProjection;
  }
  else
  {
    m_bSphereProjection = false;
  }
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class wdSphereReflectionProbeComponent_1_2 : public wdGraphPatch
{
public:
  wdSphereReflectionProbeComponent_1_2()
    : wdGraphPatch("wdSphereReflectionProbeComponent", 2)
  {
  }

  virtual void Patch(wdGraphPatchContext& ref_context, wdAbstractObjectGraph* pGraph, wdAbstractObjectNode* pNode) const override
  {
    pNode->AddProperty("SphereProjection", false);
  }
};

wdSphereReflectionProbeComponent_1_2 g_wdSphereReflectionProbeComponent_1_2;

WD_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SphereReflectionProbeComponent);
