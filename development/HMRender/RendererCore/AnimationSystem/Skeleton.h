#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Math/Mat3.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/Declarations.h>

class wdStreamWriter;
class wdStreamReader;
class wdSkeletonBuilder;
class wdSkeleton;

namespace ozz::animation
{
  class Skeleton;
}

/// \brief Describes a single joint.
/// The transforms of the joints are in their local space and thus need to be correctly multiplied with their parent transforms to get the
/// final transform.
class WD_RENDERERCORE_DLL wdSkeletonJoint
{
public:
  const wdTransform& GetBindPoseLocalTransform() const { return m_BindPoseLocal; }

  /// \brief Returns wdInvalidJointIndex if no parent
  wdUInt16 GetParentIndex() const { return m_uiParentIndex; }

  bool IsRootJoint() const { return m_uiParentIndex == wdInvalidJointIndex; }
  const wdHashedString& GetName() const { return m_sName; }

  wdAngle GetHalfSwingLimitY() const { return m_HalfSwingLimitY; }
  wdAngle GetHalfSwingLimitZ() const { return m_HalfSwingLimitZ; }
  wdAngle GetTwistLimitHalfAngle() const { return m_TwistLimitHalfAngle; }
  wdAngle GetTwistLimitCenterAngle() const { return m_TwistLimitCenterAngle; }
  wdAngle GetTwistLimitLow() const;
  wdAngle GetTwistLimitHigh() const;

  wdQuat GetLocalOrientation() const { return m_qLocalJointOrientation; }
  wdUInt8 GetCollisionLayer() const { return m_uiCollisionLayer; }
  wdSurfaceResourceHandle GetSurface() { return m_hSurface;}

  protected:
  friend wdSkeleton;
  friend wdSkeletonBuilder;

  wdTransform m_BindPoseLocal;
  wdUInt16 m_uiParentIndex = wdInvalidJointIndex;
  wdHashedString m_sName;

  wdSurfaceResourceHandle m_hSurface;
  wdUInt8 m_uiCollisionLayer = 0;

  wdQuat m_qLocalJointOrientation = wdQuat::IdentityQuaternion();
  wdAngle m_HalfSwingLimitY;
  wdAngle m_HalfSwingLimitZ;
  wdAngle m_TwistLimitHalfAngle;
  wdAngle m_TwistLimitCenterAngle;
};

/// \brief The skeleton class encapsulates the information about the joint structure for a model.
class WD_RENDERERCORE_DLL wdSkeleton
{
  WD_DISALLOW_COPY_AND_ASSIGN(wdSkeleton);

public:
  wdSkeleton();
  wdSkeleton(wdSkeleton&& rhs);
  ~wdSkeleton();

  void operator=(wdSkeleton&& rhs);

  /// \brief Returns the number of joints in the skeleton.
  wdUInt16 GetJointCount() const { return static_cast<wdUInt16>(m_Joints.GetCount()); }

  /// \brief Returns the nth joint.
  const wdSkeletonJoint& GetJointByIndex(wdUInt16 uiIndex) const { return m_Joints[uiIndex]; }

  /// \brief Allows to find a specific joint in the skeleton by name. Returns wdInvalidJointIndex if not found
  wdUInt16 FindJointByName(const wdTempHashedString& sName) const;

  /// \brief Checks if two skeletons are compatible (same joint count and hierarchy)
  // bool IsCompatibleWith(const wdSkeleton& other) const;

  /// \brief Saves the skeleton in a given stream.
  void Save(wdStreamWriter& inout_stream) const;

  /// \brief Loads the skeleton from the given stream.
  void Load(wdStreamReader& inout_stream);

  bool IsJointDescendantOf(wdUInt16 uiJoint, wdUInt16 uiExpectedParent) const;

  const ozz::animation::Skeleton& GetOzzSkeleton() const;

  wdUInt64 GetHeapMemoryUsage() const;

  /// \brief The direction in which the bones shall point for visualization
  wdEnum<wdBasisAxis> m_BoneDirection;

protected:
  friend wdSkeletonBuilder;

  wdDynamicArray<wdSkeletonJoint> m_Joints;
  mutable wdUniquePtr<ozz::animation::Skeleton> m_pOzzSkeleton;
};
