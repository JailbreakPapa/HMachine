#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/SkeletonBuilder.h>

wdSkeletonBuilder::wdSkeletonBuilder() = default;
wdSkeletonBuilder::~wdSkeletonBuilder() = default;

wdUInt16 wdSkeletonBuilder::AddJoint(const char* szName, const wdTransform& localBindPose, wdUInt16 uiParentIndex /*= wdInvalidJointIndex*/)
{
  WD_ASSERT_DEV(uiParentIndex == wdInvalidJointIndex || uiParentIndex < m_Joints.GetCount(), "Invalid parent index for joint");

  auto& joint = m_Joints.ExpandAndGetRef();

  joint.m_BindPoseLocal = localBindPose;
  joint.m_BindPoseGlobal = localBindPose;
  joint.m_sName.Assign(szName);
  joint.m_uiParentIndex = uiParentIndex;

  if (uiParentIndex != wdInvalidJointIndex)
  {
    joint.m_BindPoseGlobal = m_Joints[joint.m_uiParentIndex].m_BindPoseGlobal * joint.m_BindPoseLocal;
  }

  joint.m_InverseBindPoseGlobal = joint.m_BindPoseGlobal.GetInverse();

  return static_cast<wdUInt16>(m_Joints.GetCount() - 1);
}

void wdSkeletonBuilder::SetJointLimit(wdUInt16 uiJointIndex, const wdQuat& qLocalOrientation, bool bLimitSwing, wdAngle halfSwingLimitY, wdAngle halfSwingLimitZ, bool bLimitTwist, wdAngle twistLimitHalfAngle, wdAngle twistLimitCenterAngle)
{
  auto& j = m_Joints[uiJointIndex];
  j.m_qLocalJointOrientation = qLocalOrientation;
  j.m_HalfSwingLimitY = halfSwingLimitY;
  j.m_HalfSwingLimitZ = halfSwingLimitZ;
  j.m_TwistLimitHalfAngle = twistLimitHalfAngle;
  j.m_TwistLimitCenterAngle = twistLimitCenterAngle;
  j.m_bLimitSwing = bLimitSwing;
  j.m_bLimitTwist = bLimitTwist;
}

void wdSkeletonBuilder::BuildSkeleton(wdSkeleton& ref_skeleton) const
{
  // WD_ASSERT_DEV(HasJoints(), "Can't build a skeleton with no joints!");

  const wdUInt32 numJoints = m_Joints.GetCount();

  // Copy joints to skeleton
  ref_skeleton.m_Joints.SetCount(numJoints);

  for (wdUInt32 i = 0; i < numJoints; ++i)
  {
    ref_skeleton.m_Joints[i].m_sName = m_Joints[i].m_sName;
    ref_skeleton.m_Joints[i].m_uiParentIndex = m_Joints[i].m_uiParentIndex;
    ref_skeleton.m_Joints[i].m_BindPoseLocal = m_Joints[i].m_BindPoseLocal;

    ref_skeleton.m_Joints[i].m_qLocalJointOrientation = m_Joints[i].m_qLocalJointOrientation;
    ref_skeleton.m_Joints[i].m_HalfSwingLimitY = m_Joints[i].m_bLimitSwing ? m_Joints[i].m_HalfSwingLimitY : wdAngle();
    ref_skeleton.m_Joints[i].m_HalfSwingLimitZ = m_Joints[i].m_bLimitSwing ? m_Joints[i].m_HalfSwingLimitZ : wdAngle();
    ref_skeleton.m_Joints[i].m_TwistLimitHalfAngle = m_Joints[i].m_bLimitTwist ? m_Joints[i].m_TwistLimitHalfAngle : wdAngle();
    ref_skeleton.m_Joints[i].m_TwistLimitCenterAngle = m_Joints[i].m_bLimitTwist ? m_Joints[i].m_TwistLimitCenterAngle : wdAngle();
  }
}

bool wdSkeletonBuilder::HasJoints() const
{
  return !m_Joints.IsEmpty();
}



WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_SkeletonBuilder);
