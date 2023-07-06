#pragma once

#include <Core/World/World.h>
#include <RendererCore/RendererCoreDLL.h>

struct wdMsgUpdateLocalBounds;

using wdBakedProbesVolumeComponentManager = wdComponentManager<class wdBakedProbesVolumeComponent, wdBlockStorageType::Compact>;

class WD_RENDERERCORE_DLL wdBakedProbesVolumeComponent : public wdComponent
{
  WD_DECLARE_COMPONENT_TYPE(wdBakedProbesVolumeComponent, wdComponent, wdBakedProbesVolumeComponentManager);

public:
  wdBakedProbesVolumeComponent();
  ~wdBakedProbesVolumeComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  const wdVec3& GetExtents() const { return m_vExtents; }
  void SetExtents(const wdVec3& vExtents);

  virtual void SerializeComponent(wdWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(wdWorldReader& inout_stream) override;

  void OnUpdateLocalBounds(wdMsgUpdateLocalBounds& ref_msg) const;

private:
  wdVec3 m_vExtents = wdVec3(10.0f);
};
