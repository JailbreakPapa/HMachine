#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
WD_BEGIN_ABSTRACT_COMPONENT_TYPE(wdRenderComponent, 1)
{
  WD_BEGIN_MESSAGEHANDLERS
  {
    WD_MESSAGE_HANDLER(wdMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
  WD_END_MESSAGEHANDLERS;
}
WD_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

wdRenderComponent::wdRenderComponent() = default;
wdRenderComponent::~wdRenderComponent() = default;

void wdRenderComponent::Deinitialize()
{
  wdRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());

  SUPER::Deinitialize();
}

void wdRenderComponent::OnActivated()
{
  TriggerLocalBoundsUpdate();
}

void wdRenderComponent::OnDeactivated()
{
  // Can't call TriggerLocalBoundsUpdate because it checks whether we are active, which is not the case anymore.
  GetOwner()->UpdateLocalBounds();
}

void wdRenderComponent::OnUpdateLocalBounds(wdMsgUpdateLocalBounds& msg)
{
  wdBoundingBoxSphere bounds;
  bounds.SetInvalid();

  bool bAlwaysVisible = false;

  if (GetLocalBounds(bounds, bAlwaysVisible, msg).Succeeded())
  {
    wdSpatialData::Category category = GetOwner()->IsDynamic() ? wdDefaultSpatialDataCategories::RenderDynamic : wdDefaultSpatialDataCategories::RenderStatic;

    if (bounds.IsValid())
    {
      msg.AddBounds(bounds, category);
    }

    if (bAlwaysVisible)
    {
      msg.SetAlwaysVisible(category);
    }
  }
}

void wdRenderComponent::InvalidateCachedRenderData()
{
  if (IsActiveAndInitialized())
  {
    wdRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

void wdRenderComponent::TriggerLocalBoundsUpdate()
{
  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

// static
wdUInt32 wdRenderComponent::GetUniqueIdForRendering(const wdComponent* pComponent, wdUInt32 uiInnerIndex /*= 0*/, wdUInt32 uiInnerIndexShift /*= 24*/)
{
  wdUInt32 uniqueId = pComponent->GetUniqueID();
  if (uniqueId == wdInvalidIndex)
  {
    uniqueId = pComponent->GetOwner()->GetHandle().GetInternalID().m_InstanceIndex;
  }
  else
  {
    uniqueId |= (uiInnerIndex << uiInnerIndexShift);
  }

  const wdUInt32 dynamicBit = (1 << 31);
  const wdUInt32 dynamicBitMask = ~dynamicBit;
  return (uniqueId & dynamicBitMask) | (pComponent->GetOwner()->IsDynamic() ? dynamicBit : 0);
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_RenderComponent);
