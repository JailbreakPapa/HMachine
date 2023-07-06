#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/offline/animation_optimizer.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdAnimationClipResource, 1, wdRTTIDefaultAllocator<wdAnimationClipResource>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_RESOURCE_IMPLEMENT_COMMON_CODE(wdAnimationClipResource);
// clang-format on

wdAnimationClipResource::wdAnimationClipResource()
  : wdResource(DoUpdate::OnAnyThread, 1)
{
}

WD_RESOURCE_IMPLEMENT_CREATEABLE(wdAnimationClipResource, wdAnimationClipResourceDescriptor)
{
  m_pDescriptor = WD_DEFAULT_NEW(wdAnimationClipResourceDescriptor);
  *m_pDescriptor = std::move(descriptor);

  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::Loaded;

  return res;
}

wdResourceLoadDesc wdAnimationClipResource::UnloadData(Unload WhatToUnload)
{
  m_pDescriptor.Clear();

  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::Unloaded;

  return res;
}

wdResourceLoadDesc wdAnimationClipResource::UpdateContent(wdStreamReader* Stream)
{
  WD_LOG_BLOCK("wdAnimationClipResource::UpdateContent", GetResourceDescription().GetData());

  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = wdResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    wdStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  // skip the asset file header at the start of the file
  wdAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_pDescriptor = WD_DEFAULT_NEW(wdAnimationClipResourceDescriptor);
  m_pDescriptor->Deserialize(*Stream).IgnoreResult();

  res.m_State = wdResourceState::Loaded;
  return res;
}

void wdAnimationClipResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(wdAnimationClipResource);

  if (m_pDescriptor)
  {
    out_NewMemoryUsage.m_uiMemoryCPU += static_cast<wdUInt32>(m_pDescriptor->GetHeapMemoryUsage());
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct wdAnimationClipResourceDescriptor::OzzImpl
{
  struct CachedAnim
  {
    wdUInt32 m_uiResourceChangeCounter = 0;
    ozz::unique_ptr<ozz::animation::Animation> m_pAnim;
  };

  wdMap<const wdSkeletonResource*, CachedAnim> m_MappedOzzAnimations;
};

wdAnimationClipResourceDescriptor::wdAnimationClipResourceDescriptor()
{
  m_pOzzImpl = WD_DEFAULT_NEW(OzzImpl);
}

wdAnimationClipResourceDescriptor::wdAnimationClipResourceDescriptor(wdAnimationClipResourceDescriptor&& rhs)
{
  *this = std::move(rhs);
}

wdAnimationClipResourceDescriptor::~wdAnimationClipResourceDescriptor() = default;

void wdAnimationClipResourceDescriptor::operator=(wdAnimationClipResourceDescriptor&& rhs) noexcept
{
  m_pOzzImpl = std::move(rhs.m_pOzzImpl);

  m_JointInfos = std::move(rhs.m_JointInfos);
  m_Transforms = std::move(rhs.m_Transforms);
  m_uiNumTotalPositions = rhs.m_uiNumTotalPositions;
  m_uiNumTotalRotations = rhs.m_uiNumTotalRotations;
  m_uiNumTotalScales = rhs.m_uiNumTotalScales;
  m_Duration = rhs.m_Duration;
}

wdResult wdAnimationClipResourceDescriptor::Serialize(wdStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(9);

  const wdUInt16 uiNumJoints = static_cast<wdUInt16>(m_JointInfos.GetCount());
  inout_stream << uiNumJoints;
  for (wdUInt32 i = 0; i < m_JointInfos.GetCount(); ++i)
  {
    const auto& val = m_JointInfos.GetValue(i);

    inout_stream << m_JointInfos.GetKey(i);
    inout_stream << val.m_uiPositionIdx;
    inout_stream << val.m_uiPositionCount;
    inout_stream << val.m_uiRotationIdx;
    inout_stream << val.m_uiRotationCount;
    inout_stream << val.m_uiScaleIdx;
    inout_stream << val.m_uiScaleCount;
  }

  inout_stream << m_Duration;
  inout_stream << m_uiNumTotalPositions;
  inout_stream << m_uiNumTotalRotations;
  inout_stream << m_uiNumTotalScales;

  WD_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_Transforms));

  inout_stream << m_vConstantRootMotion;

  m_EventTrack.Save(inout_stream);

  inout_stream << m_bAdditive;

  return WD_SUCCESS;
}

wdResult wdAnimationClipResourceDescriptor::Deserialize(wdStreamReader& inout_stream)
{
  const wdTypeVersion uiVersion = inout_stream.ReadVersion(9);

  if (uiVersion < 6)
    return WD_FAILURE;

  wdUInt16 uiNumJoints = 0;
  inout_stream >> uiNumJoints;

  m_JointInfos.Reserve(uiNumJoints);

  wdHashedString hs;

  for (wdUInt16 i = 0; i < uiNumJoints; ++i)
  {
    inout_stream >> hs;

    JointInfo ji;
    inout_stream >> ji.m_uiPositionIdx;
    inout_stream >> ji.m_uiPositionCount;
    inout_stream >> ji.m_uiRotationIdx;
    inout_stream >> ji.m_uiRotationCount;
    inout_stream >> ji.m_uiScaleIdx;
    inout_stream >> ji.m_uiScaleCount;

    m_JointInfos.Insert(hs, ji);
  }

  m_JointInfos.Sort();

  inout_stream >> m_Duration;
  inout_stream >> m_uiNumTotalPositions;
  inout_stream >> m_uiNumTotalRotations;
  inout_stream >> m_uiNumTotalScales;

  WD_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Transforms));

  if (uiVersion >= 7)
  {
    inout_stream >> m_vConstantRootMotion;
  }

  if (uiVersion >= 8)
  {
    m_EventTrack.Load(inout_stream);
  }

  if (uiVersion >= 9)
  {
    inout_stream >> m_bAdditive;
  }

  return WD_SUCCESS;
}

wdUInt64 wdAnimationClipResourceDescriptor::GetHeapMemoryUsage() const
{
  return m_Transforms.GetHeapMemoryUsage() + m_JointInfos.GetHeapMemoryUsage() + m_pOzzImpl->m_MappedOzzAnimations.GetHeapMemoryUsage();
}

wdUInt16 wdAnimationClipResourceDescriptor::GetNumJoints() const
{
  return static_cast<wdUInt16>(m_JointInfos.GetCount());
}

wdTime wdAnimationClipResourceDescriptor::GetDuration() const
{
  return m_Duration;
}

void wdAnimationClipResourceDescriptor::SetDuration(wdTime duration)
{
  m_Duration = duration;
}

WD_FORCE_INLINE void wd2ozz(const wdVec3& vIn, ozz::math::Float3& ref_out)
{
  ref_out.x = vIn.x;
  ref_out.y = vIn.y;
  ref_out.z = vIn.z;
}

WD_FORCE_INLINE void wd2ozz(const wdQuat& qIn, ozz::math::Quaternion& ref_out)
{
  ref_out.x = qIn.v.x;
  ref_out.y = qIn.v.y;
  ref_out.z = qIn.v.z;
  ref_out.w = qIn.w;
}

const ozz::animation::Animation& wdAnimationClipResourceDescriptor::GetMappedOzzAnimation(const wdSkeletonResource& skeleton) const
{
  auto it = m_pOzzImpl->m_MappedOzzAnimations.Find(&skeleton);
  if (it.IsValid())
  {
    if (it.Value().m_uiResourceChangeCounter == skeleton.GetCurrentResourceChangeCounter())
    {
      return *it.Value().m_pAnim.get();
    }
  }

  auto pOzzSkeleton = &skeleton.GetDescriptor().m_Skeleton.GetOzzSkeleton();
  const wdUInt32 uiNumJoints = pOzzSkeleton->num_joints();

  ozz::animation::offline::RawAnimation rawAnim;
  rawAnim.duration = wdMath::Max(1.0f / 60.0f, m_Duration.AsFloatInSeconds());
  rawAnim.tracks.resize(uiNumJoints);

  for (wdUInt32 j = 0; j < uiNumJoints; ++j)
  {
    auto& dstTrack = rawAnim.tracks[j];

    const wdTempHashedString sJointName = wdTempHashedString(pOzzSkeleton->joint_names()[j]);

    const JointInfo* pJointInfo = GetJointInfo(sJointName);

    if (pJointInfo == nullptr)
    {
      dstTrack.translations.resize(1);
      dstTrack.rotations.resize(1);
      dstTrack.scales.resize(1);

      const wdUInt16 uiFallbackIdx = skeleton.GetDescriptor().m_Skeleton.FindJointByName(sJointName);

      WD_ASSERT_DEV(uiFallbackIdx != wdInvalidJointIndex, "");

      const auto& fallbackJoint = skeleton.GetDescriptor().m_Skeleton.GetJointByIndex(uiFallbackIdx);

      const wdTransform& fallbackTransform = fallbackJoint.GetBindPoseLocalTransform();

      auto& dstT = dstTrack.translations[0];
      auto& dstR = dstTrack.rotations[0];
      auto& dstS = dstTrack.scales[0];

      dstT.time = 0.0f;
      dstR.time = 0.0f;
      dstS.time = 0.0f;

      wd2ozz(fallbackTransform.m_vPosition, dstT.value);
      wd2ozz(fallbackTransform.m_qRotation, dstR.value);
      wd2ozz(fallbackTransform.m_vScale, dstS.value);
    }
    else
    {
      // positions
      {
        dstTrack.translations.resize(pJointInfo->m_uiPositionCount);
        const wdArrayPtr<const KeyframeVec3> keyframes = GetPositionKeyframes(*pJointInfo);

        for (wdUInt32 i = 0; i < pJointInfo->m_uiPositionCount; ++i)
        {
          auto& dst = dstTrack.translations[i];

          dst.time = keyframes[i].m_fTimeInSec;
          wd2ozz(keyframes[i].m_Value, dst.value);
        }
      }

      // rotations
      {
        dstTrack.rotations.resize(pJointInfo->m_uiRotationCount);
        const wdArrayPtr<const KeyframeQuat> keyframes = GetRotationKeyframes(*pJointInfo);

        for (wdUInt32 i = 0; i < pJointInfo->m_uiRotationCount; ++i)
        {
          auto& dst = dstTrack.rotations[i];

          dst.time = keyframes[i].m_fTimeInSec;
          wd2ozz(keyframes[i].m_Value, dst.value);
        }
      }

      // scales
      {
        dstTrack.scales.resize(pJointInfo->m_uiScaleCount);
        const wdArrayPtr<const KeyframeVec3> keyframes = GetScaleKeyframes(*pJointInfo);

        for (wdUInt32 i = 0; i < pJointInfo->m_uiScaleCount; ++i)
        {
          auto& dst = dstTrack.scales[i];

          dst.time = keyframes[i].m_fTimeInSec;
          wd2ozz(keyframes[i].m_Value, dst.value);
        }
      }
    }
  }

  ozz::animation::offline::AnimationBuilder animBuilder;

  WD_ASSERT_DEBUG(rawAnim.Validate(), "Invalid animation data");

  auto& cached = m_pOzzImpl->m_MappedOzzAnimations[&skeleton];
  cached.m_pAnim = std::move(animBuilder(rawAnim));
  cached.m_uiResourceChangeCounter = skeleton.GetCurrentResourceChangeCounter();

  return *cached.m_pAnim.get();
}

wdAnimationClipResourceDescriptor::JointInfo wdAnimationClipResourceDescriptor::CreateJoint(const wdHashedString& sJointName, wdUInt16 uiNumPositions, wdUInt16 uiNumRotations, wdUInt16 uiNumScales)
{
  JointInfo ji;
  ji.m_uiPositionIdx = m_uiNumTotalPositions;
  ji.m_uiRotationIdx = m_uiNumTotalRotations;
  ji.m_uiScaleIdx = m_uiNumTotalScales;

  ji.m_uiPositionCount = uiNumPositions;
  ji.m_uiRotationCount = uiNumRotations;
  ji.m_uiScaleCount = uiNumScales;

  m_uiNumTotalPositions += uiNumPositions;
  m_uiNumTotalRotations += uiNumRotations;
  m_uiNumTotalScales += uiNumScales;

  m_JointInfos.Insert(sJointName, ji);

  return ji;
}

const wdAnimationClipResourceDescriptor::JointInfo* wdAnimationClipResourceDescriptor::GetJointInfo(const wdTempHashedString& sJointName) const
{
  wdUInt32 uiIndex = m_JointInfos.Find(sJointName);

  if (uiIndex == wdInvalidIndex)
    return nullptr;

  return &m_JointInfos.GetValue(uiIndex);
}

void wdAnimationClipResourceDescriptor::AllocateJointTransforms()
{
  const wdUInt32 uiNumBytes = m_uiNumTotalPositions * sizeof(KeyframeVec3) + m_uiNumTotalRotations * sizeof(KeyframeQuat) + m_uiNumTotalScales * sizeof(KeyframeVec3);

  m_Transforms.SetCountUninitialized(uiNumBytes);
}

wdArrayPtr<wdAnimationClipResourceDescriptor::KeyframeVec3> wdAnimationClipResourceDescriptor::GetPositionKeyframes(const JointInfo& jointInfo)
{
  WD_ASSERT_DEBUG(!m_Transforms.IsEmpty(), "Joint transforms have not been allocated yet.");

  wdUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * jointInfo.m_uiPositionIdx;

  return wdArrayPtr<KeyframeVec3>(reinterpret_cast<KeyframeVec3*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiPositionCount);
}

wdArrayPtr<wdAnimationClipResourceDescriptor::KeyframeQuat> wdAnimationClipResourceDescriptor::GetRotationKeyframes(const JointInfo& jointInfo)
{
  WD_ASSERT_DEBUG(!m_Transforms.IsEmpty(), "Joint transforms have not been allocated yet.");

  wdUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * m_uiNumTotalPositions;
  uiByteOffsetStart += sizeof(KeyframeQuat) * jointInfo.m_uiRotationIdx;

  return wdArrayPtr<KeyframeQuat>(reinterpret_cast<KeyframeQuat*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiRotationCount);
}

wdArrayPtr<wdAnimationClipResourceDescriptor::KeyframeVec3> wdAnimationClipResourceDescriptor::GetScaleKeyframes(const JointInfo& jointInfo)
{
  WD_ASSERT_DEBUG(!m_Transforms.IsEmpty(), "Joint transforms have not been allocated yet.");

  wdUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * m_uiNumTotalPositions;
  uiByteOffsetStart += sizeof(KeyframeQuat) * m_uiNumTotalRotations;
  uiByteOffsetStart += sizeof(KeyframeVec3) * jointInfo.m_uiScaleIdx;

  return wdArrayPtr<KeyframeVec3>(reinterpret_cast<KeyframeVec3*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiScaleCount);
}

wdArrayPtr<const wdAnimationClipResourceDescriptor::KeyframeVec3> wdAnimationClipResourceDescriptor::GetPositionKeyframes(const JointInfo& jointInfo) const
{
  wdUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * jointInfo.m_uiPositionIdx;

  return wdArrayPtr<const KeyframeVec3>(reinterpret_cast<const KeyframeVec3*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiPositionCount);
}

wdArrayPtr<const wdAnimationClipResourceDescriptor::KeyframeQuat> wdAnimationClipResourceDescriptor::GetRotationKeyframes(const JointInfo& jointInfo) const
{
  wdUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * m_uiNumTotalPositions;
  uiByteOffsetStart += sizeof(KeyframeQuat) * jointInfo.m_uiRotationIdx;

  return wdArrayPtr<const KeyframeQuat>(reinterpret_cast<const KeyframeQuat*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiRotationCount);
}

wdArrayPtr<const wdAnimationClipResourceDescriptor::KeyframeVec3> wdAnimationClipResourceDescriptor::GetScaleKeyframes(const JointInfo& jointInfo) const
{
  wdUInt32 uiByteOffsetStart = 0;
  uiByteOffsetStart += sizeof(KeyframeVec3) * m_uiNumTotalPositions;
  uiByteOffsetStart += sizeof(KeyframeQuat) * m_uiNumTotalRotations;
  uiByteOffsetStart += sizeof(KeyframeVec3) * jointInfo.m_uiScaleIdx;

  return wdArrayPtr<const KeyframeVec3>(reinterpret_cast<const KeyframeVec3*>(m_Transforms.GetData() + uiByteOffsetStart), jointInfo.m_uiScaleCount);
}

// bool wdAnimationClipResourceDescriptor::HasRootMotion() const
//{
//  return m_JointNameToIndex.Contains(wdTempHashedString("wdRootMotionTransform"));
//}
//
// wdUInt16 wdAnimationClipResourceDescriptor::GetRootMotionJoint() const
//{
//  wdUInt16 jointIdx = 0;
//
//#if WD_ENABLED(WD_COMPILE_FOR_DEBUG)
//
//  const wdUInt32 idx = m_JointNameToIndex.Find(wdTempHashedString("wdRootMotionTransform"));
//  WD_ASSERT_DEBUG(idx != wdInvalidIndex, "Animation Clip has no root motion transforms");
//
//  jointIdx = m_JointNameToIndex.GetValue(idx);
//  WD_ASSERT_DEBUG(jointIdx == 0, "The root motion joint should always be at index 0");
//#endif
//
//  return jointIdx;
//}

WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_AnimationClipResource);
