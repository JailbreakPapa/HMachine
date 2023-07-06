#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/AnimationSystem/Implementation/OzzUtils.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/runtime/skeleton.h>

wdSkeleton::wdSkeleton() = default;
wdSkeleton::~wdSkeleton() = default;

wdSkeleton::wdSkeleton(wdSkeleton&& rhs)
{
  *this = std::move(rhs);
}

void wdSkeleton::operator=(wdSkeleton&& rhs)
{
  m_Joints = std::move(rhs.m_Joints);
  m_pOzzSkeleton = std::move(rhs.m_pOzzSkeleton);
}

wdUInt16 wdSkeleton::FindJointByName(const wdTempHashedString& sJointName) const
{
  const wdUInt16 uiJointCount = static_cast<wdUInt16>(m_Joints.GetCount());
  for (wdUInt16 i = 0; i < uiJointCount; ++i)
  {
    if (m_Joints[i].GetName() == sJointName)
    {
      return i;
    }
  }

  return wdInvalidJointIndex;
}

//bool wdSkeleton::IsCompatibleWith(const wdSkeleton& other) const
//{
//  if (this == &other)
//    return true;
//
//  if (other.GetJointCount() != GetJointCount())
//    return false;
//
//  // TODO: This only checks the joint hierarchy, maybe it should check names or hierarchy based on names
//  const wdUInt16 uiNumJoints = static_cast<wdUInt16>(m_Joints.GetCount());
//  for (wdUInt32 i = 0; i < uiNumJoints; ++i)
//  {
//    if (other.m_Joints[i].GetParentIndex() != m_Joints[i].GetParentIndex())
//    {
//      return false;
//    }
//  }
//
//  return true;
//}

void wdSkeleton::Save(wdStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(5);

  const wdUInt32 uiNumJoints = m_Joints.GetCount();
  inout_stream << uiNumJoints;

  for (wdUInt32 i = 0; i < uiNumJoints; ++i)
  {
    inout_stream << m_Joints[i].m_sName;
    inout_stream << m_Joints[i].m_uiParentIndex;
    inout_stream << m_Joints[i].m_BindPoseLocal;

    inout_stream << m_Joints[i].m_qLocalJointOrientation;
    inout_stream << m_Joints[i].m_HalfSwingLimitZ;
    inout_stream << m_Joints[i].m_HalfSwingLimitY;
    inout_stream << m_Joints[i].m_TwistLimitHalfAngle;
    inout_stream << m_Joints[i].m_TwistLimitCenterAngle;
  }

  inout_stream << m_BoneDirection;
}

void wdSkeleton::Load(wdStreamReader& inout_stream)
{
  const wdTypeVersion version = inout_stream.ReadVersion(5);
  if (version < 3)
    return;

  m_Joints.Clear();

  wdUInt32 uiNumJoints = 0;
  inout_stream >> uiNumJoints;

  m_Joints.Reserve(uiNumJoints);

  for (wdUInt32 i = 0; i < uiNumJoints; ++i)
  {
    wdSkeletonJoint& joint = m_Joints.ExpandAndGetRef();

    inout_stream >> joint.m_sName;
    inout_stream >> joint.m_uiParentIndex;
    inout_stream >> joint.m_BindPoseLocal;

    if (version >= 5)
    {
      inout_stream >> m_Joints[i].m_qLocalJointOrientation;
      inout_stream >> m_Joints[i].m_HalfSwingLimitZ;
      inout_stream >> m_Joints[i].m_HalfSwingLimitY;
      inout_stream >> m_Joints[i].m_TwistLimitHalfAngle;
      inout_stream >> m_Joints[i].m_TwistLimitCenterAngle;
    }
  }

  if (version >= 4)
  {
    inout_stream >> m_BoneDirection;
  }
}

bool wdSkeleton::IsJointDescendantOf(wdUInt16 uiJoint, wdUInt16 uiExpectedParent) const
{
  if (uiExpectedParent == wdInvalidJointIndex)
    return true;

  while (uiJoint != wdInvalidJointIndex)
  {
    if (uiJoint == uiExpectedParent)
      return true;

    uiJoint = m_Joints[uiJoint].m_uiParentIndex;
  }

  return false;
}

static void BuildRawOzzSkeleton(const wdSkeleton& skeleton, wdUInt16 uiExpectedParent, ozz::animation::offline::RawSkeleton::Joint::Children& ref_dstBones)
{
  wdHybridArray<wdUInt16, 6> children;

  for (wdUInt16 i = 0; i < skeleton.GetJointCount(); ++i)
  {
    if (skeleton.GetJointByIndex(i).GetParentIndex() == uiExpectedParent)
    {
      children.PushBack(i);
    }
  }

  ref_dstBones.resize((size_t)children.GetCount());

  for (wdUInt16 i = 0; i < children.GetCount(); ++i)
  {
    const auto& srcJoint = skeleton.GetJointByIndex(children[i]);
    const auto& srcTransform = srcJoint.GetBindPoseLocalTransform();
    auto& dstJoint = ref_dstBones[i];

    dstJoint.name = srcJoint.GetName().GetData();

    dstJoint.transform.translation.x = srcTransform.m_vPosition.x;
    dstJoint.transform.translation.y = srcTransform.m_vPosition.y;
    dstJoint.transform.translation.z = srcTransform.m_vPosition.z;
    dstJoint.transform.rotation.x = srcTransform.m_qRotation.v.x;
    dstJoint.transform.rotation.y = srcTransform.m_qRotation.v.y;
    dstJoint.transform.rotation.z = srcTransform.m_qRotation.v.z;
    dstJoint.transform.rotation.w = srcTransform.m_qRotation.w;
    dstJoint.transform.scale.x = srcTransform.m_vScale.x;
    dstJoint.transform.scale.y = srcTransform.m_vScale.y;
    dstJoint.transform.scale.z = srcTransform.m_vScale.z;

    BuildRawOzzSkeleton(skeleton, children[i], dstJoint.children);
  }
}

const ozz::animation::Skeleton& wdSkeleton::GetOzzSkeleton() const
{
  if (m_pOzzSkeleton)
    return *m_pOzzSkeleton.Borrow();

  // caching the skeleton isn't thread-safe
  static wdMutex cacheSkeletonMutex;
  WD_LOCK(cacheSkeletonMutex);

  // skip this, if the skeleton has been created in the mean-time
  if (m_pOzzSkeleton == nullptr)
  {
    ozz::animation::offline::RawSkeleton rawSkeleton;
    BuildRawOzzSkeleton(*this, wdInvalidJointIndex, rawSkeleton.roots);

    ozz::animation::offline::SkeletonBuilder skeletonBuilder;
    const auto pOzzSkeleton = skeletonBuilder(rawSkeleton);

    auto ozzSkeleton = WD_DEFAULT_NEW(ozz::animation::Skeleton);

    wdOzzUtils::CopySkeleton(ozzSkeleton, pOzzSkeleton.get());

    // since the pointer is read outside the mutex, only assign it, once it is fully ready for use
    m_pOzzSkeleton = ozzSkeleton;
  }

  return *m_pOzzSkeleton.Borrow();
}

wdUInt64 wdSkeleton::GetHeapMemoryUsage() const
{
  return m_Joints.GetHeapMemoryUsage(); // TODO: + ozz skeleton
}

wdAngle wdSkeletonJoint::GetTwistLimitLow() const
{
  return wdMath::Max(wdAngle::Degree(-179), m_TwistLimitCenterAngle - m_TwistLimitHalfAngle);
}

wdAngle wdSkeletonJoint::GetTwistLimitHigh() const
{
  return wdMath::Min(wdAngle::Degree(179), m_TwistLimitCenterAngle + m_TwistLimitHalfAngle);
}

WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_Skeleton);
