#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/BakedProbes/BakingInterface.h>
#include <RendererCore/Declarations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct wdMsgUpdateLocalBounds;
struct wdMsgExtractRenderData;
struct wdRenderWorldRenderEvent;
class wdAbstractObjectNode;

class WD_RENDERERCORE_DLL wdBakedProbesComponentManager : public wdSettingsComponentManager<class wdBakedProbesComponent>
{
public:
  wdBakedProbesComponentManager(wdWorld* pWorld);
  ~wdBakedProbesComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  wdMeshResourceHandle m_hDebugSphere;
  wdMaterialResourceHandle m_hDebugMaterial;

private:
  void RenderDebug(const wdWorldModule::UpdateContext& updateContext);
  void OnRenderEvent(const wdRenderWorldRenderEvent& e);
  void CreateDebugResources();
};

class WD_RENDERERCORE_DLL wdBakedProbesComponent : public wdSettingsComponent
{
  WD_DECLARE_COMPONENT_TYPE(wdBakedProbesComponent, wdSettingsComponent, wdBakedProbesComponentManager);

public:
  wdBakedProbesComponent();
  ~wdBakedProbesComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  wdBakingSettings m_Settings; // [ property ]

  void SetShowDebugOverlay(bool bShow);                            // [ property ]
  bool GetShowDebugOverlay() const { return m_bShowDebugOverlay; } // [ property ]

  void SetShowDebugProbes(bool bShow);                           // [ property ]
  bool GetShowDebugProbes() const { return m_bShowDebugProbes; } // [ property ]

  void SetUseTestPosition(bool bUse);                            // [ property ]
  bool GetUseTestPosition() const { return m_bUseTestPosition; } // [ property ]

  void SetTestPosition(const wdVec3& vPos);                         // [ property ]
  const wdVec3& GetTestPosition() const { return m_vTestPosition; } // [ property ]

  void OnUpdateLocalBounds(wdMsgUpdateLocalBounds& ref_msg);
  void OnExtractRenderData(wdMsgExtractRenderData& ref_msg) const;

  virtual void SerializeComponent(wdWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(wdWorldReader& inout_stream) override;

private:
  void RenderDebugOverlay();
  void OnObjectCreated(const wdAbstractObjectNode& node);

  wdHashedString m_sProbeTreeResourcePrefix;

  bool m_bShowDebugOverlay = false;
  bool m_bShowDebugProbes = false;
  bool m_bUseTestPosition = false;
  wdVec3 m_vTestPosition = wdVec3::ZeroVector();

  struct RenderDebugViewTask;
  wdSharedPtr<RenderDebugViewTask> m_pRenderDebugViewTask;

  wdGALTextureHandle m_hDebugViewTexture;
};
