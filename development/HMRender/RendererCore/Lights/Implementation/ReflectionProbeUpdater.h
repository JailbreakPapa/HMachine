#pragma once

#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

WD_DECLARE_FLAGS(wdUInt8, wdReflectionProbeUpdaterFlags, SkyLight, HasCustomCubeMap);

/// \brief Renders reflection probes and stores filtered mipmap chains into an atlas texture as well as computing sky irradiance
/// Rendering sky irradiance is optional and only done if m_iIrradianceOutputIndex != -1.
class wdReflectionProbeUpdater
{
public:
  /// \brief Defines the target specular reflection probe atlas and index as well as the sky irradiance atlas and index in case the rendered cube map is a sky light.
  struct TargetSlot
  {
    wdGALTextureHandle m_hSpecularOutputTexture;   ///< Must be a valid cube map texture array handle.
    wdGALTextureHandle m_hIrradianceOutputTexture; ///< Optional. Must be set if m_iIrradianceOutputIndex != -1.
    wdInt32 m_iSpecularOutputIndex = -1;           ///< Must be a valid index into the atlas texture.
    wdInt32 m_iIrradianceOutputIndex = -1;         ///< If -1, no irradiance is computed.
  };

public:
  wdReflectionProbeUpdater();
  ~wdReflectionProbeUpdater();

  /// \brief Returns how many new probes can be started this frame.
  /// \param out_updatesFinished Contains the probes that finished last frame.
  /// \return The number of new probes can be started this frame.
  wdUInt32 GetFreeUpdateSlots(wdDynamicArray<wdReflectionProbeRef>& out_updatesFinished);

  /// \brief Starts rendering a new reflection probe.
  /// \param probe The world and probe index to be rendered. Used as an identifier.
  /// \param desc Probe render settings.
  /// \param globalTransform World position to be rendered.
  /// \param target Where the probe should be rendered into.
  /// \return Returns WD_FAILURE if no more free slots are available.
  wdResult StartDynamicUpdate(const wdReflectionProbeRef& probe, const wdReflectionProbeDesc& desc, const wdTransform& globalTransform, const TargetSlot& target);

  /// \brief Starts filtering an existing cube map into a new reflection probe.
  /// \param probe The world and probe index to be rendered. Used as an identifier.
  /// \param desc Probe render settings.
  /// \param sourceTexture Cube map that should be filtered into a reflection probe.
  /// \param target Where the probe should be rendered into.
  /// \return Returns WD_FAILURE if no more free slots are available.
  wdResult StartFilterUpdate(const wdReflectionProbeRef& probe, const wdReflectionProbeDesc& desc, wdTextureCubeResourceHandle hSourceTexture, const TargetSlot& target);

  /// \brief Cancel a previously started update.
  void CancelUpdate(const wdReflectionProbeRef& probe);

  /// \brief Generates update steps. Should be called in PreExtraction phase.
  void GenerateUpdateSteps();

  /// \brief Schedules probe rendering views. Should be called at some point during the extraction phase. Can be called multiple times. It will only do work on the first call after GenerateUpdateSteps.
  void ScheduleUpdateSteps();

private:
  struct ReflectionView
  {
    wdViewHandle m_hView;
    wdCamera m_Camera;
  };

  struct UpdateStep
  {
    using StorageType = wdUInt8;

    enum Enum
    {
      RenderFace0,
      RenderFace1,
      RenderFace2,
      RenderFace3,
      RenderFace4,
      RenderFace5,
      Filter,

      ENUM_COUNT,

      Default = Filter
    };

    static bool IsRenderStep(Enum value) { return value >= UpdateStep::RenderFace0 && value <= UpdateStep::RenderFace5; }
    static Enum NextStep(Enum value) { return static_cast<UpdateStep::Enum>((value + 1) % UpdateStep::ENUM_COUNT); }
  };

  struct ProbeUpdateInfo
  {
    ProbeUpdateInfo();
    ~ProbeUpdateInfo();

    wdBitflags<wdReflectionProbeUpdaterFlags> m_flags;
    wdReflectionProbeRef m_probe;
    wdReflectionProbeDesc m_desc;
    wdTransform m_globalTransform;
    wdTextureCubeResourceHandle m_sourceTexture;
    TargetSlot m_TargetSlot;

    struct Step
    {
      WD_DECLARE_POD_TYPE();

      wdUInt8 m_uiViewIndex;
      wdEnum<UpdateStep> m_UpdateStep;
    };

    bool m_bInUse = false;
    wdEnum<UpdateStep> m_LastUpdateStep;

    wdHybridArray<Step, 8> m_UpdateSteps;

    wdGALTextureHandle m_hCubemap;
    wdGALTextureHandle m_hCubemapProxies[6];
  };

private:
  static void CreateViews(
    wdDynamicArray<ReflectionView>& views, wdUInt32 uiMaxRenderViews, const char* szNameSuffix, const char* szRenderPipelineResource);
  void CreateReflectionViewsAndResources();

  void ResetProbeUpdateInfo(wdUInt32 uiInfo);
  void AddViewToRender(const ProbeUpdateInfo::Step& step, ProbeUpdateInfo& updateInfo);

  bool m_bUpdateStepsFlushed = true;

  wdDynamicArray<ReflectionView> m_RenderViews;
  wdDynamicArray<ReflectionView> m_FilterViews;

  // Active Dynamic Updates
  wdDynamicArray<wdUniquePtr<ProbeUpdateInfo>> m_DynamicUpdates;
  wdHybridArray<wdReflectionProbeRef, 4> m_FinishedLastFrame;
};
