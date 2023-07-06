#include <Core/CorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/World/CoordinateSystem.h>
#include <Foundation/Utilities/GraphicsUtils.h>

class RemapCoordinateSystemProvider : public wdCoordinateSystemProvider
{
public:
  RemapCoordinateSystemProvider()
    : wdCoordinateSystemProvider(nullptr)
  {
  }

  virtual void GetCoordinateSystem(const wdVec3& vGlobalPosition, wdCoordinateSystem& out_coordinateSystem) const override
  {
    out_coordinateSystem.m_vForwardDir = wdBasisAxis::GetBasisVector(m_ForwardAxis);
    out_coordinateSystem.m_vRightDir = wdBasisAxis::GetBasisVector(m_RightAxis);
    out_coordinateSystem.m_vUpDir = wdBasisAxis::GetBasisVector(m_UpAxis);
  }

  wdBasisAxis::Enum m_ForwardAxis = wdBasisAxis::PositiveX;
  wdBasisAxis::Enum m_RightAxis = wdBasisAxis::PositiveY;
  wdBasisAxis::Enum m_UpAxis = wdBasisAxis::PositiveZ;
};

// clang-format off
WD_BEGIN_STATIC_REFLECTED_ENUM(wdCameraMode, 1)
  WD_ENUM_CONSTANT(wdCameraMode::PerspectiveFixedFovX),
  WD_ENUM_CONSTANT(wdCameraMode::PerspectiveFixedFovY),
  WD_ENUM_CONSTANT(wdCameraMode::OrthoFixedWidth),
  WD_ENUM_CONSTANT(wdCameraMode::OrthoFixedHeight),
WD_END_STATIC_REFLECTED_ENUM;
// clang-format on

wdCamera::wdCamera()
{
  m_vCameraPosition[0].SetZero();
  m_vCameraPosition[1].SetZero();
  m_mViewMatrix[0].SetIdentity();
  m_mViewMatrix[1].SetIdentity();
  m_mStereoProjectionMatrix[0].SetIdentity();
  m_mStereoProjectionMatrix[1].SetIdentity();

  SetCoordinateSystem(wdBasisAxis::PositiveX, wdBasisAxis::PositiveY, wdBasisAxis::PositiveZ);
}

void wdCamera::SetCoordinateSystem(wdBasisAxis::Enum forwardAxis, wdBasisAxis::Enum rightAxis, wdBasisAxis::Enum axis)
{
  auto provider = WD_DEFAULT_NEW(RemapCoordinateSystemProvider);
  provider->m_ForwardAxis = forwardAxis;
  provider->m_RightAxis = rightAxis;
  provider->m_UpAxis = axis;

  m_pCoordinateSystem = provider;
}

void wdCamera::SetCoordinateSystem(const wdSharedPtr<wdCoordinateSystemProvider>& pProvider)
{
  m_pCoordinateSystem = pProvider;
}

wdVec3 wdCamera::GetPosition(wdCameraEye eye) const
{
  return MapInternalToExternal(m_vCameraPosition[static_cast<int>(eye)]);
}

wdVec3 wdCamera::GetDirForwards(wdCameraEye eye) const
{
  wdVec3 decFwd, decRight, decUp, decPos;
  wdGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], wdHandedness::LeftHanded);

  return MapInternalToExternal(decFwd);
}

wdVec3 wdCamera::GetDirUp(wdCameraEye eye) const
{
  wdVec3 decFwd, decRight, decUp, decPos;
  wdGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], wdHandedness::LeftHanded);

  return MapInternalToExternal(decUp);
}

wdVec3 wdCamera::GetDirRight(wdCameraEye eye) const
{
  wdVec3 decFwd, decRight, decUp, decPos;
  wdGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], wdHandedness::LeftHanded);

  return MapInternalToExternal(decRight);
}

wdVec3 wdCamera::InternalGetPosition(wdCameraEye eye) const
{
  return m_vCameraPosition[static_cast<int>(eye)];
}

wdVec3 wdCamera::InternalGetDirForwards(wdCameraEye eye) const
{
  wdVec3 decFwd, decRight, decUp, decPos;
  wdGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], wdHandedness::LeftHanded);

  return decFwd;
}

wdVec3 wdCamera::InternalGetDirUp(wdCameraEye eye) const
{
  wdVec3 decFwd, decRight, decUp, decPos;
  wdGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], wdHandedness::LeftHanded);

  return decUp;
}

wdVec3 wdCamera::InternalGetDirRight(wdCameraEye eye) const
{
  wdVec3 decFwd, decRight, decUp, decPos;
  wdGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], wdHandedness::LeftHanded);

  return -decRight;
}

wdVec3 wdCamera::MapExternalToInternal(const wdVec3& v) const
{
  if (m_pCoordinateSystem)
  {
    wdCoordinateSystem system;
    m_pCoordinateSystem->GetCoordinateSystem(m_vCameraPosition[0], system);

    wdMat3 m;
    m.SetRow(0, system.m_vForwardDir);
    m.SetRow(1, system.m_vRightDir);
    m.SetRow(2, system.m_vUpDir);

    return m * v;
  }

  return v;
}

wdVec3 wdCamera::MapInternalToExternal(const wdVec3& v) const
{
  if (m_pCoordinateSystem)
  {
    wdCoordinateSystem system;
    m_pCoordinateSystem->GetCoordinateSystem(m_vCameraPosition[0], system);

    wdMat3 m;
    m.SetColumn(0, system.m_vForwardDir);
    m.SetColumn(1, system.m_vRightDir);
    m.SetColumn(2, system.m_vUpDir);

    return m * v;
  }

  return v;
}

wdAngle wdCamera::GetFovX(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == wdCameraMode::PerspectiveFixedFovX)
    return wdAngle::Degree(m_fFovOrDim);

  if (m_Mode == wdCameraMode::PerspectiveFixedFovY)
    return wdMath::ATan(wdMath::Tan(wdAngle::Degree(m_fFovOrDim) * 0.5f) * fAspectRatioWidthDivHeight) * 2.0f;

  // TODO: HACK
  if (m_Mode == wdCameraMode::Stereo)
    return wdAngle::Degree(90);

  WD_REPORT_FAILURE("You cannot get the camera FOV when it is not a perspective camera.");
  return wdAngle();
}

wdAngle wdCamera::GetFovY(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == wdCameraMode::PerspectiveFixedFovX)
    return wdMath::ATan(wdMath::Tan(wdAngle::Degree(m_fFovOrDim) * 0.5f) / fAspectRatioWidthDivHeight) * 2.0f;

  if (m_Mode == wdCameraMode::PerspectiveFixedFovY)
    return wdAngle::Degree(m_fFovOrDim);

  // TODO: HACK
  if (m_Mode == wdCameraMode::Stereo)
    return wdAngle::Degree(90);

  WD_REPORT_FAILURE("You cannot get the camera FOV when it is not a perspective camera.");
  return wdAngle();
}


float wdCamera::GetDimensionX(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == wdCameraMode::OrthoFixedWidth)
    return m_fFovOrDim;

  if (m_Mode == wdCameraMode::OrthoFixedHeight)
    return m_fFovOrDim * fAspectRatioWidthDivHeight;

  WD_REPORT_FAILURE("You cannot get the camera dimensions when it is not an orthographic camera.");
  return 0;
}


float wdCamera::GetDimensionY(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == wdCameraMode::OrthoFixedWidth)
    return m_fFovOrDim / fAspectRatioWidthDivHeight;

  if (m_Mode == wdCameraMode::OrthoFixedHeight)
    return m_fFovOrDim;

  WD_REPORT_FAILURE("You cannot get the camera dimensions when it is not an orthographic camera.");
  return 0;
}

void wdCamera::SetCameraMode(wdCameraMode::Enum mode, float fFovOrDim, float fNearPlane, float fFarPlane)
{
  // early out if no change
  if (m_Mode == mode && m_fFovOrDim == fFovOrDim && m_fNearPlane == fNearPlane && m_fFarPlane == fFarPlane)
  {
    return;
  }

  m_Mode = mode;
  m_fFovOrDim = fFovOrDim;
  m_fNearPlane = fNearPlane;
  m_fFarPlane = fFarPlane;

  m_fAspectOfPrecomputedStereoProjection = -1.0f;

  CameraSettingsChanged();
}

void wdCamera::SetStereoProjection(const wdMat4& mProjectionLeftEye, const wdMat4& mProjectionRightEye, float fAspectRatioWidthDivHeight)
{
  m_mStereoProjectionMatrix[static_cast<int>(wdCameraEye::Left)] = mProjectionLeftEye;
  m_mStereoProjectionMatrix[static_cast<int>(wdCameraEye::Right)] = mProjectionRightEye;
  m_fAspectOfPrecomputedStereoProjection = fAspectRatioWidthDivHeight;

  CameraSettingsChanged();
}

void wdCamera::LookAt(const wdVec3& vCameraPos0, const wdVec3& vTargetPos0, const wdVec3& vUp0)
{
  const wdVec3 vCameraPos = MapExternalToInternal(vCameraPos0);
  const wdVec3 vTargetPos = MapExternalToInternal(vTargetPos0);
  const wdVec3 vUp = MapExternalToInternal(vUp0);

  if (m_Mode == wdCameraMode::Stereo)
  {
    WD_REPORT_FAILURE("wdCamera::LookAt is not possible for stereo cameras.");
    return;
  }

  m_mViewMatrix[0] = wdGraphicsUtils::CreateLookAtViewMatrix(vCameraPos, vTargetPos, vUp, wdHandedness::LeftHanded);
  m_mViewMatrix[1] = m_mViewMatrix[0];
  m_vCameraPosition[1] = m_vCameraPosition[0] = vCameraPos;

  CameraOrientationChanged(true, true);
}

void wdCamera::SetViewMatrix(const wdMat4& mLookAtMatrix, wdCameraEye eye)
{
  const int iEyeIdx = static_cast<int>(eye);

  m_mViewMatrix[iEyeIdx] = mLookAtMatrix;

  wdVec3 decFwd, decRight, decUp;
  wdGraphicsUtils::DecomposeViewMatrix(
    m_vCameraPosition[iEyeIdx], decFwd, decRight, decUp, m_mViewMatrix[static_cast<int>(eye)], wdHandedness::LeftHanded);

  if (m_Mode != wdCameraMode::Stereo)
  {
    m_mViewMatrix[1 - iEyeIdx] = m_mViewMatrix[iEyeIdx];
    m_vCameraPosition[1 - iEyeIdx] = m_vCameraPosition[iEyeIdx];
  }

  CameraOrientationChanged(true, true);
}

void wdCamera::GetProjectionMatrix(float fAspectRatioWidthDivHeight, wdMat4& out_mProjectionMatrix, wdCameraEye eye, wdClipSpaceDepthRange::Enum depthRange) const
{
  switch (m_Mode)
  {
    case wdCameraMode::PerspectiveFixedFovX:
      out_mProjectionMatrix = wdGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(wdAngle::Degree(m_fFovOrDim), fAspectRatioWidthDivHeight,
        m_fNearPlane, m_fFarPlane, depthRange, wdClipSpaceYMode::Regular, wdHandedness::LeftHanded);
      break;

    case wdCameraMode::PerspectiveFixedFovY:
      out_mProjectionMatrix = wdGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(wdAngle::Degree(m_fFovOrDim), fAspectRatioWidthDivHeight,
        m_fNearPlane, m_fFarPlane, depthRange, wdClipSpaceYMode::Regular, wdHandedness::LeftHanded);
      break;

    case wdCameraMode::OrthoFixedWidth:
      out_mProjectionMatrix = wdGraphicsUtils::CreateOrthographicProjectionMatrix(m_fFovOrDim, m_fFovOrDim / fAspectRatioWidthDivHeight, m_fNearPlane,
        m_fFarPlane, depthRange, wdClipSpaceYMode::Regular, wdHandedness::LeftHanded);
      break;

    case wdCameraMode::OrthoFixedHeight:
      out_mProjectionMatrix = wdGraphicsUtils::CreateOrthographicProjectionMatrix(m_fFovOrDim * fAspectRatioWidthDivHeight, m_fFovOrDim, m_fNearPlane,
        m_fFarPlane, depthRange, wdClipSpaceYMode::Regular, wdHandedness::LeftHanded);
      break;

    case wdCameraMode::Stereo:
      if (wdMath::IsEqual(m_fAspectOfPrecomputedStereoProjection, fAspectRatioWidthDivHeight, wdMath::LargeEpsilon<float>()))
        out_mProjectionMatrix = m_mStereoProjectionMatrix[static_cast<int>(eye)];
      else
      {
        // Evade to FixedFovY
        out_mProjectionMatrix = wdGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(wdAngle::Degree(m_fFovOrDim), fAspectRatioWidthDivHeight,
          m_fNearPlane, m_fFarPlane, depthRange, wdClipSpaceYMode::Regular, wdHandedness::LeftHanded);
      }
      break;

    default:
      WD_REPORT_FAILURE("Invalid Camera Mode {0}", (int)m_Mode);
  }
}

void wdCamera::CameraSettingsChanged()
{
  WD_ASSERT_DEV(m_Mode != wdCameraMode::None, "Invalid Camera Mode.");
  WD_ASSERT_DEV(m_fNearPlane < m_fFarPlane, "Near and Far Plane are invalid.");
  WD_ASSERT_DEV(m_fFovOrDim > 0.0f, "FOV or Camera Dimension is invalid.");

  ++m_uiSettingsModificationCounter;
}

void wdCamera::MoveLocally(float fForward, float fRight, float fUp)
{
  m_mViewMatrix[0].SetTranslationVector(m_mViewMatrix[0].GetTranslationVector() - wdVec3(fRight, fUp, fForward));
  m_mViewMatrix[1].SetTranslationVector(m_mViewMatrix[0].GetTranslationVector());

  wdVec3 decFwd, decRight, decUp, decPos;
  wdGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[0], wdHandedness::LeftHanded);

  m_vCameraPosition[0] = m_vCameraPosition[1] = decPos;

  CameraOrientationChanged(true, false);
}

void wdCamera::MoveGlobally(float fForward, float fRight, float fUp)
{
  wdVec3 vMove(fForward, fRight, fUp);

  wdVec3 decFwd, decRight, decUp, decPos;
  wdGraphicsUtils::DecomposeViewMatrix(decPos, decFwd, decRight, decUp, m_mViewMatrix[0], wdHandedness::LeftHanded);

  m_vCameraPosition[0] += vMove;
  m_vCameraPosition[1] = m_vCameraPosition[0];

  m_mViewMatrix[0] = wdGraphicsUtils::CreateViewMatrix(m_vCameraPosition[0], decFwd, decRight, decUp, wdHandedness::LeftHanded);

  m_mViewMatrix[1].SetTranslationVector(m_mViewMatrix[0].GetTranslationVector());

  CameraOrientationChanged(true, false);
}

void wdCamera::ClampRotationAngles(bool bLocalSpace, wdAngle& forwardAxis, wdAngle& rightAxis, wdAngle& upAxis)
{
  if (bLocalSpace)
  {
    if (rightAxis.GetRadian() != 0.0f)
    {
      // Limit how much the camera can look up and down, to prevent it from overturning

      const float fDot = InternalGetDirForwards().Dot(wdVec3(0, 0, -1));
      const wdAngle fCurAngle = wdMath::ACos(fDot) - wdAngle::Degree(90.0f);
      const wdAngle fNewAngle = fCurAngle + rightAxis;

      const wdAngle fAllowedAngle = wdMath::Clamp(fNewAngle, wdAngle::Degree(-85.0f), wdAngle::Degree(85.0f));

      rightAxis = fAllowedAngle - fCurAngle;
    }
  }
}

void wdCamera::RotateLocally(wdAngle forwardAxis, wdAngle rightAxis, wdAngle axis)
{
  ClampRotationAngles(true, forwardAxis, rightAxis, axis);

  wdVec3 vDirForwards = InternalGetDirForwards();
  wdVec3 vDirUp = InternalGetDirUp();
  wdVec3 vDirRight = InternalGetDirRight();

  if (forwardAxis.GetRadian() != 0.0f)
  {
    wdMat3 m;
    m.SetRotationMatrix(vDirForwards, forwardAxis);

    vDirUp = m * vDirUp;
    vDirRight = m * vDirRight;
  }

  if (rightAxis.GetRadian() != 0.0f)
  {
    wdMat3 m;
    m.SetRotationMatrix(vDirRight, rightAxis);

    vDirUp = m * vDirUp;
    vDirForwards = m * vDirForwards;
  }

  if (axis.GetRadian() != 0.0f)
  {
    wdMat3 m;
    m.SetRotationMatrix(vDirUp, axis);

    vDirRight = m * vDirRight;
    vDirForwards = m * vDirForwards;
  }

  // Using wdGraphicsUtils::CreateLookAtViewMatrix is not only easier, it also has the advantage that we end up always with orthonormal
  // vectors.
  auto vPos = InternalGetPosition();
  m_mViewMatrix[0] = wdGraphicsUtils::CreateLookAtViewMatrix(vPos, vPos + vDirForwards, vDirUp, wdHandedness::LeftHanded);
  m_mViewMatrix[1] = m_mViewMatrix[0];

  CameraOrientationChanged(false, true);
}

void wdCamera::RotateGlobally(wdAngle forwardAxis, wdAngle rightAxis, wdAngle axis)
{
  ClampRotationAngles(false, forwardAxis, rightAxis, axis);

  wdVec3 vDirForwards = InternalGetDirForwards();
  wdVec3 vDirUp = InternalGetDirUp();

  if (forwardAxis.GetRadian() != 0.0f)
  {
    wdMat3 m;
    m.SetRotationMatrixX(forwardAxis);

    vDirUp = m * vDirUp;
    vDirForwards = m * vDirForwards;
  }

  if (rightAxis.GetRadian() != 0.0f)
  {
    wdMat3 m;
    m.SetRotationMatrixY(rightAxis);

    vDirUp = m * vDirUp;
    vDirForwards = m * vDirForwards;
  }

  if (axis.GetRadian() != 0.0f)
  {
    wdMat3 m;
    m.SetRotationMatrixZ(axis);

    vDirUp = m * vDirUp;
    vDirForwards = m * vDirForwards;
  }

  // Using wdGraphicsUtils::CreateLookAtViewMatrix is not only easier, it also has the advantage that we end up always with orthonormal
  // vectors.
  auto vPos = InternalGetPosition();
  m_mViewMatrix[0] = wdGraphicsUtils::CreateLookAtViewMatrix(vPos, vPos + vDirForwards, vDirUp, wdHandedness::LeftHanded);
  m_mViewMatrix[1] = m_mViewMatrix[0];

  CameraOrientationChanged(false, true);
}



WD_STATICLINK_FILE(Core, Core_Graphics_Implementation_Camera);
