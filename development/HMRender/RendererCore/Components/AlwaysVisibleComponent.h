#pragma once

#include <RendererCore/Components/RenderComponent.h>

using wdAlwaysVisibleComponentManager = wdComponentManager<class wdAlwaysVisibleComponent, wdBlockStorageType::Compact>;

/// \brief Attaching this component to a game object makes the renderer consider it always visible, ie. disables culling
class WD_RENDERERCORE_DLL wdAlwaysVisibleComponent : public wdRenderComponent
{
  WD_DECLARE_COMPONENT_TYPE(wdAlwaysVisibleComponent, wdRenderComponent, wdAlwaysVisibleComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // wdRenderComponent

public:
  virtual wdResult GetLocalBounds(wdBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, wdMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // wdAlwaysVisibleComponent

public:
  wdAlwaysVisibleComponent();
  ~wdAlwaysVisibleComponent();
};
