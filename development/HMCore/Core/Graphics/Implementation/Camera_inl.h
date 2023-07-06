#pragma once

inline wdVec3 wdCamera::GetCenterPosition() const
{
  if (m_Mode == wdCameraMode::Stereo)
    return (GetPosition(wdCameraEye::Left) + GetPosition(wdCameraEye::Right)) * 0.5f;
  else
    return GetPosition();
}

inline wdVec3 wdCamera::GetCenterDirForwards() const
{
  if (m_Mode == wdCameraMode::Stereo)
    return (GetDirForwards(wdCameraEye::Left) + GetDirForwards(wdCameraEye::Right)).GetNormalized();
  else
    return GetDirForwards();
}

inline wdVec3 wdCamera::GetCenterDirUp() const
{
  if (m_Mode == wdCameraMode::Stereo)
    return (GetDirUp(wdCameraEye::Left) + GetDirUp(wdCameraEye::Right)).GetNormalized();
  else
    return GetDirUp();
}

inline wdVec3 wdCamera::GetCenterDirRight() const
{
  if (m_Mode == wdCameraMode::Stereo)
    return (GetDirRight(wdCameraEye::Left) + GetDirRight(wdCameraEye::Right)).GetNormalized();
  else
    return GetDirRight();
}

WD_ALWAYS_INLINE float wdCamera::GetNearPlane() const
{
  return m_fNearPlane;
}

WD_ALWAYS_INLINE float wdCamera::GetFarPlane() const
{
  return m_fFarPlane;
}

WD_ALWAYS_INLINE float wdCamera::GetFovOrDim() const
{
  return m_fFovOrDim;
}

WD_ALWAYS_INLINE wdCameraMode::Enum wdCamera::GetCameraMode() const
{
  return m_Mode;
}

WD_ALWAYS_INLINE bool wdCamera::IsPerspective() const
{
  return m_Mode == wdCameraMode::PerspectiveFixedFovX || m_Mode == wdCameraMode::PerspectiveFixedFovY ||
         m_Mode == wdCameraMode::Stereo; // All HMD stereo cameras are perspective!
}

WD_ALWAYS_INLINE bool wdCamera::IsOrthographic() const
{
  return m_Mode == wdCameraMode::OrthoFixedWidth || m_Mode == wdCameraMode::OrthoFixedHeight;
}

WD_ALWAYS_INLINE bool wdCamera::IsStereoscopic() const
{
  return m_Mode == wdCameraMode::Stereo;
}

WD_ALWAYS_INLINE float wdCamera::GetExposure() const
{
  return m_fExposure;
}

WD_ALWAYS_INLINE void wdCamera::SetExposure(float fExposure)
{
  m_fExposure = fExposure;
}

WD_ALWAYS_INLINE const wdMat4& wdCamera::GetViewMatrix(wdCameraEye eye) const
{
  return m_mViewMatrix[static_cast<int>(eye)];
}
