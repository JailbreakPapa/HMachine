#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <RendererCore/RendererCoreDLL.h>

struct wdMsgTransformChanged;
struct wdMsgUpdateLocalBounds;
struct wdMsgExtractOccluderData;

class WD_RENDERERCORE_DLL wdOccluderComponentManager final : public wdComponentManager<class wdOccluderComponent, wdBlockStorageType::FreeList>
{
public:
  wdOccluderComponentManager(wdWorld* pWorld);
};

class WD_RENDERERCORE_DLL wdOccluderComponent : public wdComponent
{
  WD_DECLARE_COMPONENT_TYPE(wdOccluderComponent, wdComponent, wdOccluderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // wdComponent

public:
  virtual void SerializeComponent(wdWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(wdWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // wdBoxReflectionProbeComponent

public:
  wdOccluderComponent();
  ~wdOccluderComponent();

  const wdVec3& GetExtents() const
  {
    return m_vExtents;
  }

  void SetExtents(const wdVec3& vExtents);

private:
  wdVec3 m_vExtents = wdVec3(5.0f);

  mutable wdSharedPtr<const wdRasterizerObject> m_pOccluderObject;

  void OnUpdateLocalBounds(wdMsgUpdateLocalBounds& msg);
  void OnMsgExtractOccluderData(wdMsgExtractOccluderData& msg) const;
};
