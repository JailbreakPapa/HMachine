#pragma once

#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Textures/Texture2DResource.h>

struct wdMsgExtractRenderData;

using wdRenderTargetComponentManager = wdComponentManager<class wdRenderTargetActivatorComponent, wdBlockStorageType::Compact>;

class WD_RENDERERCORE_DLL wdRenderTargetActivatorComponent : public wdRenderComponent
{
  WD_DECLARE_COMPONENT_TYPE(wdRenderTargetActivatorComponent, wdRenderComponent, wdRenderTargetComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // wdComponent
public:
  virtual void SerializeComponent(wdWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(wdWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // wdRenderComponent

public:
  virtual wdResult GetLocalBounds(wdBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, wdMsgUpdateLocalBounds& ref_msg) override;


  //////////////////////////////////////////////////////////////////////////
  // wdRenderTargetActivatorComponent

public:
  wdRenderTargetActivatorComponent();
  ~wdRenderTargetActivatorComponent();

  void SetRenderTargetFile(const char* szFile); // [ property ]
  const char* GetRenderTargetFile() const;      // [ property ]

  void SetRenderTarget(const wdRenderToTexture2DResourceHandle& hResource);
  wdRenderToTexture2DResourceHandle GetRenderTarget() const { return m_hRenderTarget; }

private:
  void OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const;

  wdRenderToTexture2DResourceHandle m_hRenderTarget;
};
