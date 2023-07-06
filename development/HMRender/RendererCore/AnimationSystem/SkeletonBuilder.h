#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/RendererCoreDLL.h>

/// \brief The skeleton builder class provides the means to build skeleton instances from scratch.
/// This class is not necessary to use skeletons, usually they should be deserialized from data created by the tools.
class WD_RENDERERCORE_DLL wdSkeletonBuilder
{

public:
  wdSkeletonBuilder();
  ~wdSkeletonBuilder();

  /// \brief Adds a joint to the skeleton
  /// Since the only way to add a joint with a parent is through this method the order of joints in the array is guaranteed
  /// so that child joints always come after their parent joints
  wdUInt16 AddJoint(const char* szName, const wdTransform& localBindPose, wdUInt16 uiParentIndex = wdInvalidJointIndex);

  void SetJointLimit(wdUInt16 uiJointIndex, const wdQuat& qLocalOrientation, bool bLimitSwing, wdAngle halfSwingLimitY, wdAngle halfSwingLimitZ, bool bLimitTwist, wdAngle twistLimitHalfAngle, wdAngle twistLimitCenterAngle);

  /// \brief Creates a skeleton from the accumulated data.
  void BuildSkeleton(wdSkeleton& ref_skeleton) const;

  /// \brief Returns true if there any joints have been added to the skeleton builder
  bool HasJoints() const;

protected:
  struct BuilderJoint
  {
    wdTransform m_BindPoseLocal;
    wdTransform m_BindPoseGlobal; // this one is temporary and not stored in the final wdSkeleton
    wdTransform m_InverseBindPoseGlobal;
    wdUInt16 m_uiParentIndex = wdInvalidJointIndex;
    wdHashedString m_sName;
    wdQuat m_qLocalJointOrientation = wdQuat::IdentityQuaternion();
    wdAngle m_HalfSwingLimitZ;
    wdAngle m_HalfSwingLimitY;
    wdAngle m_TwistLimitHalfAngle;
    wdAngle m_TwistLimitCenterAngle;
    bool m_bLimitTwist = false;
    bool m_bLimitSwing = false;
  };

  wdDeque<BuilderJoint> m_Joints;
};
