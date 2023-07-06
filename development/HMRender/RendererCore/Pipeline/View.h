#pragma once

#include <Foundation/Strings/HashedString.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/TagSet.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelineNode.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

class wdFrustum;
class wdWorld;
class wdRenderPipeline;

/// \brief Encapsulates a view on the given world through the given camera
/// and rendered with the specified RenderPipeline into the given render target setup.
class WD_RENDERERCORE_DLL wdView : public wdRenderPipelineNode
{
  WD_ADD_DYNAMIC_REFLECTION(wdView, wdRenderPipelineNode);

private:
  /// \brief Use wdRenderLoop::CreateView to create a view.
  wdView();
  ~wdView();

public:
  wdViewHandle GetHandle() const;

  void SetName(wdStringView sName);
  wdStringView GetName() const;

  void SetWorld(wdWorld* pWorld);
  wdWorld* GetWorld();
  const wdWorld* GetWorld() const;

  /// \brief Sets the swapchain that this view will be rendering into. Can be invalid in case the render target is an off-screen buffer in which case SetRenderTargets needs to be called.
  /// Setting the swap-chain is necessary in order to acquire and present the image to the window.
  /// SetSwapChain and SetRenderTargets are mutually exclusive. Calling this function will reset the render targets.
  void SetSwapChain(wdGALSwapChainHandle hSwapChain);
  wdGALSwapChainHandle GetSwapChain() const;

  /// \brief Sets the off-screen render targets. Use SetSwapChain if rendering to a window.
  /// SetSwapChain and SetRenderTargets are mutually exclusive. Calling this function will reset the swap chain.
  void SetRenderTargets(const wdGALRenderTargets& renderTargets);
  const wdGALRenderTargets& GetRenderTargets() const;

  /// \brief Returns the render targets that were either set via the swapchain or via the manually set render targets.
  const wdGALRenderTargets& GetActiveRenderTargets() const;

  void SetRenderPipelineResource(wdRenderPipelineResourceHandle hPipeline);
  wdRenderPipelineResourceHandle GetRenderPipelineResource() const;

  void SetCamera(wdCamera* pCamera);
  wdCamera* GetCamera();
  const wdCamera* GetCamera() const;

  void SetCullingCamera(const wdCamera* pCamera);
  const wdCamera* GetCullingCamera() const;

  void SetLodCamera(const wdCamera* pCamera);
  const wdCamera* GetLodCamera() const;

  /// \brief Returns the camera usage hint for the view.
  wdEnum<wdCameraUsageHint> GetCameraUsageHint() const;
  /// \brief Sets the camera usage hint for the view. If not 'None', the camera component of the same usage will be auto-connected
  ///   to this view.
  void SetCameraUsageHint(wdEnum<wdCameraUsageHint> val);

  void SetViewRenderMode(wdEnum<wdViewRenderMode> value);
  wdEnum<wdViewRenderMode> GetViewRenderMode() const;

  void SetViewport(const wdRectFloat& viewport);
  const wdRectFloat& GetViewport() const;

  /// \brief Forces the render pipeline to be rebuilt.
  void ForceUpdate();

  const wdViewData& GetData() const;

  bool IsValid() const;

  /// \brief Extracts all relevant data from the world to render the view.
  void ExtractData();

  /// \brief Returns a task implementation that calls ExtractData on this view.
  const wdSharedPtr<wdTask>& GetExtractTask();


  /// \brief Returns the start position and direction (in world space) of the picking ray through the screen position in this view.
  ///
  /// fScreenPosX and fScreenPosY are expected to be in [0; 1] range (normalized pixel coordinates).
  /// If no ray can be computed, WD_FAILURE is returned.
  wdResult ComputePickingRay(float fScreenPosX, float fScreenPosY, wdVec3& out_vRayStartPos, wdVec3& out_vRayDir) const;

  wdResult ComputeScreenSpacePos(const wdVec3& vPoint, wdVec3& out_vScreenPos) const;

  /// \brief Returns the current projection matrix.
  const wdMat4& GetProjectionMatrix(wdCameraEye eye) const;

  /// \brief Returns the current inverse projection matrix.
  const wdMat4& GetInverseProjectionMatrix(wdCameraEye eye) const;

  /// \brief Returns the current view matrix (camera orientation).
  const wdMat4& GetViewMatrix(wdCameraEye eye) const;

  /// \brief Returns the current inverse view matrix (inverse camera orientation).
  const wdMat4& GetInverseViewMatrix(wdCameraEye eye) const;

  /// \brief Returns the current view-projection matrix.
  const wdMat4& GetViewProjectionMatrix(wdCameraEye eye) const;

  /// \brief Returns the current inverse view-projection matrix.
  const wdMat4& GetInverseViewProjectionMatrix(wdCameraEye eye) const;

  /// \brief Returns the frustum that should be used for determine visible objects for this view.
  void ComputeCullingFrustum(wdFrustum& out_frustum) const;

  void SetShaderPermutationVariable(const char* szName, const char* szValue);

  void SetRenderPassProperty(const char* szPassName, const char* szPropertyName, const wdVariant& value);
  void SetExtractorProperty(const char* szPassName, const char* szPropertyName, const wdVariant& value);

  void SetRenderPassReadBackProperty(const char* szPassName, const char* szPropertyName, const wdVariant& value);
  wdVariant GetRenderPassReadBackProperty(const char* szPassName, const char* szPropertyName);
  bool IsRenderPassReadBackPropertyExisting(const char* szPassName, const char* szPropertyName) const;

  /// \brief Pushes the view and camera data into the extracted data of the pipeline.
  ///
  /// Use wdRenderWorld::GetDataIndexForExtraction() to update the data from the extraction thread. Can't be used if this view is currently extracted.
  /// Use wdRenderWorld::GetDataIndexForRendering() to update the data from the render thread.
  void UpdateViewData(wdUInt32 uiDataIndex);

  wdTagSet m_IncludeTags;
  wdTagSet m_ExcludeTags;

private:
  friend class wdRenderWorld;
  friend class wdMemoryUtils;

  wdViewId m_InternalId;
  wdHashedString m_sName;

  wdSharedPtr<wdTask> m_pExtractTask;

  wdWorld* m_pWorld = nullptr;

  wdRenderPipelineResourceHandle m_hRenderPipeline;
  wdUInt32 m_uiRenderPipelineResourceDescriptionCounter = 0;
  wdSharedPtr<wdRenderPipeline> m_pRenderPipeline;
  wdCamera* m_pCamera = nullptr;
  const wdCamera* m_pCullingCamera = nullptr;
  const wdCamera* m_pLodCamera = nullptr;


private:
  wdRenderPipelineNodeInputPin m_PinRenderTarget0;
  wdRenderPipelineNodeInputPin m_PinRenderTarget1;
  wdRenderPipelineNodeInputPin m_PinRenderTarget2;
  wdRenderPipelineNodeInputPin m_PinRenderTarget3;
  wdRenderPipelineNodeInputPin m_PinDepthStencil;

private:
  void UpdateCachedMatrices() const;

  /// \brief Rebuilds pipeline if necessary and pushes double-buffered settings into the pipeline.
  void EnsureUpToDate();

  mutable wdUInt32 m_uiLastCameraSettingsModification = 0;
  mutable wdUInt32 m_uiLastCameraOrientationModification = 0;
  mutable float m_fLastViewportAspectRatio = 1.0f;

  mutable wdViewData m_Data;

  wdInternal::RenderDataCache* m_pRenderDataCache = nullptr;

  wdDynamicArray<wdPermutationVar> m_PermutationVars;
  bool m_bPermutationVarsDirty = false;

  void ApplyPermutationVars();

  struct PropertyValue
  {
    wdString m_sObjectName;
    wdString m_sPropertyName;
    wdVariant m_Value;
    bool m_bIsValid;
    bool m_bIsDirty;
  };

  void SetProperty(wdMap<wdString, PropertyValue>& map, const char* szPassName, const char* szPropertyName, const wdVariant& value);
  void SetReadBackProperty(wdMap<wdString, PropertyValue>& map, const char* szPassName, const char* szPropertyName, const wdVariant& value);

  void ReadBackPassProperties();

  void ResetAllPropertyStates(wdMap<wdString, PropertyValue>& map);

  void ApplyRenderPassProperties();
  void ApplyExtractorProperties();

  void ApplyProperty(wdReflectedClass* pClass, PropertyValue& data, const char* szTypeName);

  wdMap<wdString, PropertyValue> m_PassProperties;
  wdMap<wdString, PropertyValue> m_PassReadBackProperties;
  wdMap<wdString, PropertyValue> m_ExtractorProperties;
};

#include <RendererCore/Pipeline/Implementation/View_inl.h>
