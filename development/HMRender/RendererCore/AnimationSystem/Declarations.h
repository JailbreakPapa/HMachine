#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Communication/Message.h>
#include <RendererCore/RendererCoreDLL.h>
#include <ozz/base/maths/soa_transform.h>

class wdSkeleton;
class wdAnimationPose;
struct wdSkeletonResourceDescriptor;
class wdEditableSkeletonJoint;
struct wdAnimationClipResourceDescriptor;

using wdSkeletonResourceHandle = wdTypedResourceHandle<class wdSkeletonResource>;

#define wdInvalidJointIndex static_cast<wdUInt16>(0xFFFFu)

namespace ozz::animation
{
  class Skeleton;
}

struct wdSkeletonJointGeometryType
{
  using StorageType = wdUInt8;

  enum Enum
  {
    None = 0,
    Capsule,
    Sphere,
    Box,

    Default = None
  };
};

/// \brief Used by components that skin a mesh to inform children whenever a new pose is being prepared.
///
/// The pose matrices are still in local space and in the ozz internal structure-of-arrays format.
/// At this point individual bones can still be modified, to propagate the effect to the child bones.
struct WD_RENDERERCORE_DLL wdMsgAnimationPosePreparing : public wdMessage
{
  WD_DECLARE_MESSAGE_TYPE(wdMsgAnimationPosePreparing, wdMessage);

  const wdSkeleton* m_pSkeleton = nullptr;
  wdArrayPtr<ozz::math::SoaTransform> m_LocalTransforms;
};

/// \brief Used by components that skin a mesh to inform children whenever a new pose has been computed.
///
/// This can be used by child nodes/components to synchronize their state to the new animation pose.
/// The message is sent while the pose is in object space.
/// Both skeleton and pose pointer are always valid.
struct WD_RENDERERCORE_DLL wdMsgAnimationPoseUpdated : public wdMessage
{
  WD_DECLARE_MESSAGE_TYPE(wdMsgAnimationPoseUpdated, wdMessage);

  static void ComputeFullBoneTransform(const wdMat4& mRootTransform, const wdMat4& mModelTransform, wdMat4& ref_mFullTransform, wdQuat& ref_qRotationOnly);
  void ComputeFullBoneTransform(wdUInt32 uiJointIndex, wdMat4& ref_mFullTransform) const;
  void ComputeFullBoneTransform(wdUInt32 uiJointIndex, wdMat4& ref_mFullTransform, wdQuat& ref_qRotationOnly) const;

  const wdTransform* m_pRootTransform = nullptr;
  const wdSkeleton* m_pSkeleton = nullptr;
  wdArrayPtr<const wdMat4> m_ModelTransforms;
  bool m_bContinueAnimating = true;
};

struct WD_RENDERERCORE_DLL wdMsgAnimationPoseProposal : public wdMessage
{
  WD_DECLARE_MESSAGE_TYPE(wdMsgAnimationPoseProposal, wdMessage);

  const wdTransform* m_pRootTransform = nullptr;
  const wdSkeleton* m_pSkeleton = nullptr;
  wdArrayPtr<const wdMat4> m_ModelTransforms;
  bool m_bContinueAnimating = true;
};

/// \brief Used by components that do rope simulation and rendering.
///
/// The rope simulation component sends this message to components attached to the same game object,
/// every time there is a new rope pose. There is no skeleton information, since all joints/bones are
/// connected as one long string.
///
/// For a rope with N segments, N+1 poses are sent. The last pose may use the same rotation as the one before.
struct WD_RENDERERCORE_DLL wdMsgRopePoseUpdated : public wdMessage
{
  WD_DECLARE_MESSAGE_TYPE(wdMsgRopePoseUpdated, wdMessage);

  wdArrayPtr<const wdTransform> m_LinkTransforms;
};
struct wdSkeletonJointType
{
  using StorageType = wdUInt8;

  enum Enum
  {
    None,
    Fixed,
    //  Hinge,
    SwingTwist,

    Default = None,
  };
};
/// \brief The animated mesh component listens to this message and 'answers' by filling out the skeleton resource handle.
///
/// This can be used by components that require a skeleton, to ask the nearby components to provide it to them.
struct WD_RENDERERCORE_DLL wdMsgQueryAnimationSkeleton : public wdMessage
{
  WD_DECLARE_MESSAGE_TYPE(wdMsgQueryAnimationSkeleton, wdMessage);

  wdSkeletonResourceHandle m_hSkeleton;
};

/// \brief This message is sent when animation root motion data is available.
///
/// Listening components can use this to move a character.
struct WD_RENDERERCORE_DLL wdMsgApplyRootMotion : public wdMessage
{
  WD_DECLARE_MESSAGE_TYPE(wdMsgApplyRootMotion, wdMessage);

  wdVec3 m_vTranslation;
  wdAngle m_RotationX;
  wdAngle m_RotationY;
  wdAngle m_RotationZ;
};

/// \brief Queries the local transforms of each bone in an object with a skeleton
///
/// Used to retrieve the pose of a ragdoll after simulation.
struct WD_RENDERERCORE_DLL wdMsgRetrieveBoneState : public wdMessage
{
  WD_DECLARE_MESSAGE_TYPE(wdMsgRetrieveBoneState, wdMessage);

  // maps from bone name to its local transform
  wdMap<wdString, wdTransform> m_BoneTransforms;
};
