#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/BakedProbes/BakedProbesVolumeComponent.h>

// clang-format off
WD_BEGIN_COMPONENT_TYPE(wdBakedProbesVolumeComponent, 1, wdComponentMode::Static)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new wdDefaultValueAttribute(wdVec3(10.0f)), new wdClampValueAttribute(wdVec3(0), wdVariant())),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_MESSAGEHANDLERS
  {
    WD_MESSAGE_HANDLER(wdMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  WD_END_MESSAGEHANDLERS;
  WD_BEGIN_ATTRIBUTES
  {
    new wdInDevelopmentAttribute(wdInDevelopmentAttribute::Phase::Beta),
    new wdCategoryAttribute("Rendering/Baking"),
    new wdBoxManipulatorAttribute("Extents", 1.0f, true),
    new wdBoxVisualizerAttribute("Extents", 1.0f, wdColor::OrangeRed),
  }
  WD_END_ATTRIBUTES;
}
WD_END_COMPONENT_TYPE
// clang-format on

wdBakedProbesVolumeComponent::wdBakedProbesVolumeComponent() = default;
wdBakedProbesVolumeComponent::~wdBakedProbesVolumeComponent() = default;

void wdBakedProbesVolumeComponent::OnActivated()
{
  GetOwner()->UpdateLocalBounds();
}

void wdBakedProbesVolumeComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();
}

void wdBakedProbesVolumeComponent::SetExtents(const wdVec3& vExtents)
{
  if (m_vExtents != vExtents)
  {
    m_vExtents = vExtents;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }
  }
}

void wdBakedProbesVolumeComponent::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  wdStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
}

void wdBakedProbesVolumeComponent::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const wdUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  wdStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;
}

void wdBakedProbesVolumeComponent::OnUpdateLocalBounds(wdMsgUpdateLocalBounds& ref_msg) const
{
  ref_msg.AddBounds(wdBoundingBox(-m_vExtents * 0.5f, m_vExtents * 0.5f), wdInvalidSpatialDataCategory);
}


WD_STATICLINK_FILE(RendererCore, RendererCore_BakedProbes_Implementation_BakedProbesVolumeComponent);
