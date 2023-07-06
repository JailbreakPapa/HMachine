
WD_ALWAYS_INLINE wdViewHandle wdView::GetHandle() const
{
  return wdViewHandle(m_InternalId);
}

WD_ALWAYS_INLINE wdStringView wdView::GetName() const
{
  return m_sName.GetView();
}

WD_ALWAYS_INLINE wdWorld* wdView::GetWorld()
{
  return m_pWorld;
}

WD_ALWAYS_INLINE const wdWorld* wdView::GetWorld() const
{
  return m_pWorld;
}

WD_ALWAYS_INLINE wdGALSwapChainHandle wdView::GetSwapChain() const
{
  return m_Data.m_hSwapChain;
}

WD_ALWAYS_INLINE const wdGALRenderTargets& wdView::GetRenderTargets() const
{
  return m_Data.m_renderTargets;
}

WD_ALWAYS_INLINE void wdView::SetCamera(wdCamera* pCamera)
{
  m_pCamera = pCamera;
}

WD_ALWAYS_INLINE wdCamera* wdView::GetCamera()
{
  return m_pCamera;
}

WD_ALWAYS_INLINE const wdCamera* wdView::GetCamera() const
{
  return m_pCamera;
}

WD_ALWAYS_INLINE void wdView::SetCullingCamera(const wdCamera* pCamera)
{
  m_pCullingCamera = pCamera;
}

WD_ALWAYS_INLINE const wdCamera* wdView::GetCullingCamera() const
{
  return m_pCullingCamera != nullptr ? m_pCullingCamera : m_pCamera;
}

WD_ALWAYS_INLINE void wdView::SetLodCamera(const wdCamera* pCamera)
{
  m_pLodCamera = pCamera;
}

WD_ALWAYS_INLINE const wdCamera* wdView::GetLodCamera() const
{
  return m_pLodCamera != nullptr ? m_pLodCamera : m_pCamera;
}

WD_ALWAYS_INLINE wdEnum<wdCameraUsageHint> wdView::GetCameraUsageHint() const
{
  return m_Data.m_CameraUsageHint;
}

WD_ALWAYS_INLINE wdEnum<wdViewRenderMode> wdView::GetViewRenderMode() const
{
  return m_Data.m_ViewRenderMode;
}

WD_ALWAYS_INLINE const wdRectFloat& wdView::GetViewport() const
{
  return m_Data.m_ViewPortRect;
}

WD_ALWAYS_INLINE const wdViewData& wdView::GetData() const
{
  UpdateCachedMatrices();
  return m_Data;
}

WD_FORCE_INLINE bool wdView::IsValid() const
{
  return m_pWorld != nullptr && m_pRenderPipeline != nullptr && m_pCamera != nullptr && m_Data.m_ViewPortRect.HasNonZeroArea();
}

WD_ALWAYS_INLINE const wdSharedPtr<wdTask>& wdView::GetExtractTask()
{
  return m_pExtractTask;
}

WD_FORCE_INLINE wdResult wdView::ComputePickingRay(float fScreenPosX, float fScreenPosY, wdVec3& out_vRayStartPos, wdVec3& out_vRayDir) const
{
  UpdateCachedMatrices();
  return m_Data.ComputePickingRay(fScreenPosX, fScreenPosY, out_vRayStartPos, out_vRayDir);
}

WD_FORCE_INLINE wdResult wdView::ComputeScreenSpacePos(const wdVec3& vPoint, wdVec3& out_vScreenPos) const
{
  UpdateCachedMatrices();
  return m_Data.ComputeScreenSpacePos(vPoint, out_vScreenPos);
}

WD_ALWAYS_INLINE const wdMat4& wdView::GetProjectionMatrix(wdCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_ProjectionMatrix[static_cast<int>(eye)];
}

WD_ALWAYS_INLINE const wdMat4& wdView::GetInverseProjectionMatrix(wdCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_InverseProjectionMatrix[static_cast<int>(eye)];
}

WD_ALWAYS_INLINE const wdMat4& wdView::GetViewMatrix(wdCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_ViewMatrix[static_cast<int>(eye)];
}

WD_ALWAYS_INLINE const wdMat4& wdView::GetInverseViewMatrix(wdCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_InverseViewMatrix[static_cast<int>(eye)];
}

WD_ALWAYS_INLINE const wdMat4& wdView::GetViewProjectionMatrix(wdCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_ViewProjectionMatrix[static_cast<int>(eye)];
}

WD_ALWAYS_INLINE const wdMat4& wdView::GetInverseViewProjectionMatrix(wdCameraEye eye) const
{
  UpdateCachedMatrices();
  return m_Data.m_InverseViewProjectionMatrix[static_cast<int>(eye)];
}
