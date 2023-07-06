#pragma once

#include <Core/Graphics/Camera.h>
#include <Core/World/World.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/Declarations.h>

class wdView;
struct wdResourceEvent;

class WD_RENDERERCORE_DLL wdCameraComponentManager : public wdComponentManager<class wdCameraComponent, wdBlockStorageType::Compact>
{
public:
  wdCameraComponentManager(wdWorld* pWorld);
  ~wdCameraComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  void Update(const wdWorldModule::UpdateContext& context);

  void ReinitializeAllRenderTargetCameras();

  const wdCameraComponent* GetCameraByUsageHint(wdCameraUsageHint::Enum usageHint) const;
  wdCameraComponent* GetCameraByUsageHint(wdCameraUsageHint::Enum usageHint);

private:
  friend class wdCameraComponent;

  void AddRenderTargetCamera(wdCameraComponent* pComponent);
  void RemoveRenderTargetCamera(wdCameraComponent* pComponent);

  void OnViewCreated(wdView* pView);
  void OnCameraConfigsChanged(void* dummy);

  wdDynamicArray<wdComponentHandle> m_ModifiedCameras;
  wdDynamicArray<wdComponentHandle> m_RenderTargetCameras;
};


class WD_RENDERERCORE_DLL wdCameraComponent : public wdComponent
{
  WD_DECLARE_COMPONENT_TYPE(wdCameraComponent, wdComponent, wdCameraComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // wdComponent

public:
  virtual void SerializeComponent(wdWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(wdWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // wdCameraComponent

public:
  wdCameraComponent();
  ~wdCameraComponent();

  wdEnum<wdCameraUsageHint> GetUsageHint() const { return m_UsageHint; } // [ property ]
  void SetUsageHint(wdEnum<wdCameraUsageHint> val);                      // [ property ]

  void SetRenderTargetFile(const char* szFile); // [ property ]
  const char* GetRenderTargetFile() const;      // [ property ]

  void SetRenderTargetRectOffset(wdVec2 value);                                 // [ property ]
  wdVec2 GetRenderTargetRectOffset() const { return m_vRenderTargetRectOffset; } // [ property ]

  void SetRenderTargetRectSize(wdVec2 value);                               // [ property ]
  wdVec2 GetRenderTargetRectSize() const { return m_vRenderTargetRectSize; } // [ property ]

  wdEnum<wdCameraMode> GetCameraMode() const { return m_Mode; } // [ property ]
  void SetCameraMode(wdEnum<wdCameraMode> val);                 // [ property ]

  float GetNearPlane() const { return m_fNearPlane; } // [ property ]
  void SetNearPlane(float fVal);                      // [ property ]

  float GetFarPlane() const { return m_fFarPlane; } // [ property ]
  void SetFarPlane(float fVal);                     // [ property ]

  float GetFieldOfView() const { return m_fPerspectiveFieldOfView; } // [ property ]
  void SetFieldOfView(float fVal);                                   // [ property ]

  float GetOrthoDimension() const { return m_fOrthoDimension; } // [ property ]
  void SetOrthoDimension(float fVal);                           // [ property ]

  wdRenderPipelineResourceHandle GetRenderPipeline() const; // [ property ]

  const char* GetRenderPipelineEnum() const;      // [ property ]
  void SetRenderPipelineEnum(const char* szFile); // [ property ]

  float GetAperture() const { return m_fAperture; } // [ property ]
  void SetAperture(float fAperture);                // [ property ]

  wdTime GetShutterTime() const { return m_ShutterTime; } // [ property ]
  void SetShutterTime(wdTime shutterTime);                // [ property ]

  float GetISO() const { return m_fISO; } // [ property ]
  void SetISO(float fISO);                // [ property ]

  float GetExposureCompensation() const { return m_fExposureCompensation; } // [ property ]
  void SetExposureCompensation(float fEC);                                  // [ property ]

  float GetEV100() const;    // [ property ]
  float GetExposure() const; // [ property ]

  wdTagSet m_IncludeTags; // [ property ]
  wdTagSet m_ExcludeTags; // [ property ]

  void ApplySettingsToView(wdView* pView) const;

private:
  void UpdateRenderTargetCamera();
  void ShowStats(wdView* pView);

  void ResourceChangeEventHandler(const wdResourceEvent& e);

  wdEnum<wdCameraUsageHint> m_UsageHint;
  wdEnum<wdCameraMode> m_Mode;
  wdRenderToTexture2DResourceHandle m_hRenderTarget;
  float m_fNearPlane = 0.25f;
  float m_fFarPlane = 1000.0f;
  float m_fPerspectiveFieldOfView = 60.0f;
  float m_fOrthoDimension = 10.0f;
  wdRenderPipelineResourceHandle m_hCachedRenderPipeline;

  float m_fAperture = 1.0f;
  wdTime m_ShutterTime = wdTime::Seconds(1.0f);
  float m_fISO = 100.0f;
  float m_fExposureCompensation = 0.0f;

  void MarkAsModified();
  void MarkAsModified(wdCameraComponentManager* pCameraManager);

  bool m_bIsModified = false;
  bool m_bShowStats = false;
  bool m_bRenderTargetInitialized = false;

  // -1 for none, 0 to 9 for ALT+Number
  wdInt8 m_iEditorShortcut = -1; // [ property ]

  void ActivateRenderToTexture();
  void DeactivateRenderToTexture();

  wdViewHandle m_hRenderTargetView;
  wdVec2 m_vRenderTargetRectOffset = wdVec2(0.0f);
  wdVec2 m_vRenderTargetRectSize = wdVec2(1.0f);
  wdCamera m_RenderTargetCamera;
  wdHashedString m_sRenderPipeline;
};
