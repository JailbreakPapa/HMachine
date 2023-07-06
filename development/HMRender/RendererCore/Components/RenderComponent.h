#pragma once

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/World.h>
#include <RendererCore/RendererCoreDLL.h>

class WD_RENDERERCORE_DLL wdRenderComponent : public wdComponent
{
  WD_DECLARE_ABSTRACT_COMPONENT_TYPE(wdRenderComponent, wdComponent);

  //////////////////////////////////////////////////////////////////////////
  // wdComponent

protected:
  virtual void Deinitialize() override;

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // wdRenderComponent

public:
  wdRenderComponent();
  ~wdRenderComponent();

  /// \brief Called by wdRenderComponent::OnUpdateLocalBounds().
  /// If WD_SUCCESS is returned, \a bounds and \a bAlwaysVisible will be integrated into the wdMsgUpdateLocalBounds result,
  /// otherwise the out values are simply ignored.
  virtual wdResult GetLocalBounds(wdBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, wdMsgUpdateLocalBounds& ref_msg) = 0;

  void TriggerLocalBoundsUpdate();

  static wdUInt32 GetUniqueIdForRendering(const wdComponent* pComponent, wdUInt32 uiInnerIndex = 0, wdUInt32 uiInnerIndexShift = 24);

  WD_ALWAYS_INLINE wdUInt32 GetUniqueIdForRendering(wdUInt32 uiInnerIndex = 0, wdUInt32 uiInnerIndexShift = 24) const
  {
    return GetUniqueIdForRendering(this, uiInnerIndex, uiInnerIndexShift);
  }

protected:
  void OnUpdateLocalBounds(wdMsgUpdateLocalBounds& msg);
  void InvalidateCachedRenderData();
};
