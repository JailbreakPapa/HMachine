#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/AlwaysVisibleComponent.h>

// clang-format off
WD_BEGIN_COMPONENT_TYPE(wdAlwaysVisibleComponent, 1, wdComponentMode::Static)
{
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Rendering"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_COMPONENT_TYPE;
// clang-format on

wdAlwaysVisibleComponent::wdAlwaysVisibleComponent() = default;
wdAlwaysVisibleComponent::~wdAlwaysVisibleComponent() = default;

wdResult wdAlwaysVisibleComponent::GetLocalBounds(wdBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, wdMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return WD_SUCCESS;
}


WD_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_AlwaysVisibleComponent);
