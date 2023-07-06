#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/OccluderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

// clang-format off
WD_BEGIN_COMPONENT_TYPE(wdOccluderComponent, 1, wdComponentMode::Static)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new wdClampValueAttribute(wdVec3(0.0f), {}), new wdDefaultValueAttribute(wdVec3(1.0f))),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_MESSAGEHANDLERS
  {
    WD_MESSAGE_HANDLER(wdMsgUpdateLocalBounds, OnUpdateLocalBounds),
    WD_MESSAGE_HANDLER(wdMsgExtractOccluderData, OnMsgExtractOccluderData),
  }
  WD_END_MESSAGEHANDLERS;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Rendering"),
    new wdBoxVisualizerAttribute("Extents", 1.0f, wdColorScheme::LightUI(wdColorScheme::Blue)),
    new wdBoxManipulatorAttribute("Extents", 1.0f, true),
  }
  WD_END_ATTRIBUTES;
}
WD_END_COMPONENT_TYPE
// clang-format on

wdOccluderComponentManager::wdOccluderComponentManager(wdWorld* pWorld)
  : wdComponentManager<wdOccluderComponent, wdBlockStorageType::FreeList>(pWorld)
{
}

//////////////////////////////////////////////////////////////////////////

wdOccluderComponent::wdOccluderComponent() = default;
wdOccluderComponent::~wdOccluderComponent() = default;

void wdOccluderComponent::SetExtents(const wdVec3& vExtents)
{
  m_vExtents = vExtents;
  m_pOccluderObject.Clear();

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void wdOccluderComponent::OnUpdateLocalBounds(wdMsgUpdateLocalBounds& msg)
{
  if (GetOwner()->IsStatic())
    msg.AddBounds(wdBoundingBox(-m_vExtents * 0.5f, m_vExtents * 0.5f), wdDefaultSpatialDataCategories::OcclusionStatic);
  else
    msg.AddBounds(wdBoundingBox(-m_vExtents * 0.5f, m_vExtents * 0.5f), wdDefaultSpatialDataCategories::OcclusionDynamic);
}

void wdOccluderComponent::OnMsgExtractOccluderData(wdMsgExtractOccluderData& msg) const
{
  if (IsActiveAndInitialized())
  {
    if (m_pOccluderObject == nullptr)
    {
      m_pOccluderObject = wdRasterizerObject::CreateBox(m_vExtents);
    }

    msg.AddOccluder(m_pOccluderObject.Borrow(), GetOwner()->GetGlobalTransform());
  }
}

void wdOccluderComponent::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  wdStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
}

void wdOccluderComponent::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const wdUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  wdStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;
}

void wdOccluderComponent::OnActivated()
{
  m_pOccluderObject.Clear();
  GetOwner()->UpdateLocalBounds();
}

void wdOccluderComponent::OnDeactivated()
{
  m_pOccluderObject.Clear();
}


WD_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_OccluderComponent);
