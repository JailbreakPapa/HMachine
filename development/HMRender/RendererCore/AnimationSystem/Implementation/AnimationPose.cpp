#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/Shader/Types.h>

// clang-format off
WD_IMPLEMENT_MESSAGE_TYPE(wdMsgAnimationPosePreparing);
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdMsgAnimationPosePreparing, 1, wdRTTIDefaultAllocator<wdMsgAnimationPosePreparing>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_IMPLEMENT_MESSAGE_TYPE(wdMsgAnimationPoseUpdated);
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdMsgAnimationPoseUpdated, 1, wdRTTIDefaultAllocator<wdMsgAnimationPoseUpdated>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_IMPLEMENT_MESSAGE_TYPE(wdMsgAnimationPoseProposal);
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdMsgAnimationPoseProposal, 1, wdRTTIDefaultAllocator<wdMsgAnimationPoseProposal>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_IMPLEMENT_MESSAGE_TYPE(wdMsgRopePoseUpdated);
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdMsgRopePoseUpdated, 1, wdRTTIDefaultAllocator<wdMsgRopePoseUpdated>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_IMPLEMENT_MESSAGE_TYPE(wdMsgQueryAnimationSkeleton);
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdMsgQueryAnimationSkeleton, 1, wdRTTIDefaultAllocator<wdMsgQueryAnimationSkeleton>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_IMPLEMENT_MESSAGE_TYPE(wdMsgApplyRootMotion);
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdMsgApplyRootMotion, 1, wdRTTIDefaultAllocator<wdMsgApplyRootMotion>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("Translation", m_vTranslation),
    WD_MEMBER_PROPERTY("RotationX", m_RotationX),
    WD_MEMBER_PROPERTY("RotationY", m_RotationY),
    WD_MEMBER_PROPERTY("RotationZ", m_RotationZ),
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_IMPLEMENT_MESSAGE_TYPE(wdMsgRetrieveBoneState);
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdMsgRetrieveBoneState, 1, wdRTTIDefaultAllocator<wdMsgRetrieveBoneState>)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void wdMsgAnimationPoseUpdated::ComputeFullBoneTransform(wdUInt32 uiJointIndex, wdMat4& ref_mFullTransform) const
{
  ref_mFullTransform = m_pRootTransform->GetAsMat4() * m_ModelTransforms[uiJointIndex];
}

void wdMsgAnimationPoseUpdated::ComputeFullBoneTransform(const wdMat4& mRootTransform, const wdMat4& mModelTransform, wdMat4& ref_mFullTransform, wdQuat& ref_qRotationOnly)
{
  ref_mFullTransform = mRootTransform * mModelTransform;

  // the bone might contain (non-uniform) scaling and mirroring, which the quaternion can't represent
  // so reconstruct a representable rotation matrix
  ref_qRotationOnly.ReconstructFromMat4(ref_mFullTransform);
}

void wdMsgAnimationPoseUpdated::ComputeFullBoneTransform(wdUInt32 uiJointIndex, wdMat4& ref_mFullTransform, wdQuat& ref_qRotationOnly) const
{
  ComputeFullBoneTransform(m_pRootTransform->GetAsMat4(), m_ModelTransforms[uiJointIndex], ref_mFullTransform, ref_qRotationOnly);
}

WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_AnimationPose);
