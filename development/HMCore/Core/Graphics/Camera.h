#pragma once

#include <Core/CoreDLL.h>
#include <Core/World/CoordinateSystem.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>

/// \brief Specifies in which mode this camera is configured.
struct WD_CORE_DLL wdCameraMode
{
  using StorageType = wdInt8;

  enum Enum
  {
    None,                 ///< Not initialized
    PerspectiveFixedFovX, ///< Perspective camera, the fov for X is fixed, Y depends on the aspect ratio
    PerspectiveFixedFovY, ///< Perspective camera, the fov for Y is fixed, X depends on the aspect ratio
    OrthoFixedWidth,      ///< Orthographic camera, the width is fixed, the height depends on the aspect ratio
    OrthoFixedHeight,     ///< Orthographic camera, the height is fixed, the width depends on the aspect ratio
    Stereo,               ///< A stereo camera with view/projection matrices provided by an HMD.
    Default = PerspectiveFixedFovY
  };
};

WD_DECLARE_REFLECTABLE_TYPE(WD_CORE_DLL, wdCameraMode);

/// \brief Determines left or right eye of a stereo camera.
///
/// As a general rule, this parameter does not matter for mono-scopic cameras and will always return the same value.
enum class wdCameraEye
{
  Left,
  Right,
  // Two eyes should be enough for everyone.
};

/// \brief A camera class that stores the orientation and some basic camera settings.
class WD_CORE_DLL wdCamera
{
public:
  wdCamera();

  /// \brief Allows to specify a different coordinate system in which the camera input and output coordinates are given.
  ///
  /// The default in z is forward = PositiveX, right = PositiveY, Up = PositiveZ.
  void SetCoordinateSystem(wdBasisAxis::Enum forwardAxis, wdBasisAxis::Enum rightAxis, wdBasisAxis::Enum axis);

  /// \brief Allows to specify a full wdCoordinateSystemProvider to determine forward/right/up vectors for camera movement
  void SetCoordinateSystem(const wdSharedPtr<wdCoordinateSystemProvider>& pProvider);

  /// \brief Returns the position of the camera that should be used for rendering etc.
  wdVec3 GetPosition(wdCameraEye eye = wdCameraEye::Left) const;

  /// \brief Returns the forwards vector that should be used for rendering etc.
  wdVec3 GetDirForwards(wdCameraEye eye = wdCameraEye::Left) const;

  /// \brief Returns the up vector that should be used for rendering etc.
  wdVec3 GetDirUp(wdCameraEye eye = wdCameraEye::Left) const;

  /// \brief Returns the right vector that should be used for rendering etc.
  wdVec3 GetDirRight(wdCameraEye eye = wdCameraEye::Left) const;

  /// \brief Returns the horizontal FOV.
  ///
  /// Works only with wdCameraMode::PerspectiveFixedFovX and wdCameraMode::PerspectiveFixedFovY
  wdAngle GetFovX(float fAspectRatioWidthDivHeight) const;

  /// \brief Returns the vertical FOV.
  ///
  /// Works only with wdCameraMode::PerspectiveFixedFovX and wdCameraMode::PerspectiveFixedFovY
  wdAngle GetFovY(float fAspectRatioWidthDivHeight) const;

  /// \brief Returns the horizontal dimension for an orthographic view.
  ///
  /// Works only with wdCameraMode::OrthoFixedWidth and wdCameraMode::OrthoFixedWidth
  float GetDimensionX(float fAspectRatioWidthDivHeight) const;

  /// \brief Returns the vertical dimension for an orthographic view.
  ///
  /// Works only with wdCameraMode::OrthoFixedWidth and wdCameraMode::OrthoFixedWidth
  float GetDimensionY(float fAspectRatioWidthDivHeight) const;

  /// \brief Returns the average camera position.
  ///
  /// For all cameras execpt Stereo cameras this is identical to GetPosition()
  wdVec3 GetCenterPosition() const;

  /// \brief Returns the average forwards vector.
  ///
  /// For all cameras execpt Stereo cameras this is identical to GetDirForwards()
  wdVec3 GetCenterDirForwards() const;

  /// \brief Returns the average up vector.
  ///
  /// For all cameras execpt Stereo cameras this is identical to GetDirUp()
  wdVec3 GetCenterDirUp() const;

  /// \brief Returns the average right vector.
  ///
  /// For all cameras execpt Stereo cameras this is identical to GetDirRight()
  wdVec3 GetCenterDirRight() const;

  /// \brief Returns the near plane distance that was passed to SetCameraProjectionAndMode().
  float GetNearPlane() const;

  /// \brief Returns the far plane distance that was passed to SetCameraProjectionAndMode().
  float GetFarPlane() const;

  /// \brief Specifies the mode and the projection settings that this camera uses.
  ///
  /// \param fFovOrDim
  ///   Fov X/Y in degree or width/height (depending on Mode).
  void SetCameraMode(wdCameraMode::Enum mode, float fFovOrDim, float fNearPlane, float fFarPlane);

  /// Sets the camera mode to stereo and specifies projection matrices directly.
  ///
  /// \param fAspectRatio
  ///   These stereo projection matrices will only be returned by getProjectionMatrix for the given aspectRatio.
  void SetStereoProjection(const wdMat4& mProjectionLeftEye, const wdMat4& mProjectionRightEye, float fAspectRatioWidthDivHeight);

  /// \brief Returns the fFovOrDim parameter that was passed to SetCameraProjectionAndMode().
  float GetFovOrDim() const;

  /// \brief Returns the current camera mode.
  wdCameraMode::Enum GetCameraMode() const;

  bool IsPerspective() const;

  bool IsOrthographic() const;

  /// \brief Whether this is a stereoscopic camera.
  bool IsStereoscopic() const;

  /// \brief Sets the view matrix directly.
  ///
  /// Works with all camera types. Position- and direction- getter/setter will work as usual.
  void SetViewMatrix(const wdMat4& mLookAtMatrix, wdCameraEye eye = wdCameraEye::Left);

  /// \brief Repositions the camera such that it looks at the given target position.
  ///
  /// Not supported for stereo cameras.
  void LookAt(const wdVec3& vCameraPos, const wdVec3& vTargetPos, const wdVec3& vUp);

  /// \brief Moves the camera in its local space along the forward/right/up directions of the coordinate system.
  ///
  /// Not supported for stereo cameras.
  void MoveLocally(float fForward, float fRight, float fUp);

  /// \brief Moves the camera in global space along the forward/right/up directions of the coordinate system.
  ///
  /// Not supported for stereo cameras.
  void MoveGlobally(float fForward, float fRight, float fUp);

  /// \brief Rotates the camera around the forward, right and up axis in its own local space.
  ///
  /// Rotate around \a rightAxis for looking up/down. \forwardAxis is roll. For turning left/right use RotateGlobally().
  /// Not supported for stereo cameras.
  void RotateLocally(wdAngle forwardAxis, wdAngle rightAxis, wdAngle axis);

  /// \brief Rotates the camera around the forward, right and up axis of the coordinate system in global space.
  ///
  /// Rotate around Z for turning the camera left/right.
  /// Not supported for stereo cameras.
  void RotateGlobally(wdAngle forwardAxis, wdAngle rightAxis, wdAngle axis);

  /// \brief Returns the view matrix for the given eye.
  ///
  /// \note The view matrix is given in OpenGL convention.
  const wdMat4& GetViewMatrix(wdCameraEye eye = wdCameraEye::Left) const;

  /// \brief Calculates the projection matrix from the current camera properties and stores it in out_projectionMatrix.
  ///
  /// If the camera is stereo and the given aspect ratio is close to the aspect ratio passed in SetStereoProjection,
  /// the matrix set in SetStereoProjection will be used.
  void GetProjectionMatrix(float fAspectRatioWidthDivHeight, wdMat4& out_mProjectionMatrix, wdCameraEye eye = wdCameraEye::Left,
    wdClipSpaceDepthRange::Enum depthRange = wdClipSpaceDepthRange::Default) const;

  float GetExposure() const;

  void SetExposure(float fExposure);

  /// \brief Returns a counter that is increased every time the camera settings are modified.
  ///
  /// The camera settings are used to compute the projection matrix. This counter can be used to determine whether the projection matrix
  /// has changed and thus whether cached values need to be updated.
  wdUInt32 GetSettingsModificationCounter() const { return m_uiSettingsModificationCounter; }

  /// \brief Returns a counter that is increased every time the camera orientation is modified.
  ///
  /// The camera orientation is used to compute the view matrix. This counter can be used to determine whether the view matrix
  /// has changed and thus whether cached values need to be updated.
  wdUInt32 GetOrientationModificationCounter() const { return m_uiOrientationModificationCounter; }

private:
  /// \brief This function is called whenever the camera position or rotation changed.
  void CameraOrientationChanged(bool bPosition, bool bRotation) { ++m_uiOrientationModificationCounter; }

  /// \brief This function is called when the camera mode or projection changes (e.g. SetCameraProjectionAndMode was called).
  void CameraSettingsChanged();

  /// \brief This function is called by RotateLocally() and RotateGlobally() BEFORE the values are applied,
  /// and allows to adjust them (e.g. for limiting how far the camera can rotate).
  void ClampRotationAngles(bool bLocalSpace, wdAngle& forwardAxis, wdAngle& rightAxis, wdAngle& upAxis);

  wdVec3 InternalGetPosition(wdCameraEye eye = wdCameraEye::Left) const;
  wdVec3 InternalGetDirForwards(wdCameraEye eye = wdCameraEye::Left) const;
  wdVec3 InternalGetDirUp(wdCameraEye eye = wdCameraEye::Left) const;
  wdVec3 InternalGetDirRight(wdCameraEye eye = wdCameraEye::Left) const;

  float m_fNearPlane = 0.1f;
  float m_fFarPlane = 1000.0f;

  wdCameraMode::Enum m_Mode = wdCameraMode::None;

  float m_fFovOrDim = 90.0f;

  float m_fExposure = 1.0f;

  wdVec3 m_vCameraPosition[2];
  wdMat4 m_mViewMatrix[2];

  /// If the camera mode is stereo and the aspect ratio given in getProjectio is close to this value, one of the stereo projection matrices
  /// is returned.
  float m_fAspectOfPrecomputedStereoProjection = -1.0;
  wdMat4 m_mStereoProjectionMatrix[2];

  wdUInt32 m_uiSettingsModificationCounter = 0;
  wdUInt32 m_uiOrientationModificationCounter = 0;

  wdSharedPtr<wdCoordinateSystemProvider> m_pCoordinateSystem;

  wdVec3 MapExternalToInternal(const wdVec3& v) const;
  wdVec3 MapInternalToExternal(const wdVec3& v) const;
};


#include <Core/Graphics/Implementation/Camera_inl.h>
