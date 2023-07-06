#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Tracks/EventTrack.h>

class wdSkeletonResource;

namespace ozz::animation
{
  class Animation;
}

struct WD_RENDERERCORE_DLL wdAnimationClipResourceDescriptor
{
public:
  wdAnimationClipResourceDescriptor();
  wdAnimationClipResourceDescriptor(wdAnimationClipResourceDescriptor&& rhs);
  ~wdAnimationClipResourceDescriptor();

  void operator=(wdAnimationClipResourceDescriptor&& rhs) noexcept;

  wdResult Serialize(wdStreamWriter& inout_stream) const;
  wdResult Deserialize(wdStreamReader& inout_stream);

  wdUInt64 GetHeapMemoryUsage() const;

  wdUInt16 GetNumJoints() const;
  wdTime GetDuration() const;
  void SetDuration(wdTime duration);

  const ozz::animation::Animation& GetMappedOzzAnimation(const wdSkeletonResource& skeleton) const;

  struct JointInfo
  {
    wdUInt32 m_uiPositionIdx = 0;
    wdUInt32 m_uiRotationIdx = 0;
    wdUInt32 m_uiScaleIdx = 0;
    wdUInt16 m_uiPositionCount = 0;
    wdUInt16 m_uiRotationCount = 0;
    wdUInt16 m_uiScaleCount = 0;
  };

  struct KeyframeVec3
  {
    float m_fTimeInSec;
    wdVec3 m_Value;
  };

  struct KeyframeQuat
  {
    float m_fTimeInSec;
    wdQuat m_Value;
  };

  JointInfo CreateJoint(const wdHashedString& sJointName, wdUInt16 uiNumPositions, wdUInt16 uiNumRotations, wdUInt16 uiNumScales);
  const JointInfo* GetJointInfo(const wdTempHashedString& sJointName) const;
  void AllocateJointTransforms();

  wdArrayPtr<KeyframeVec3> GetPositionKeyframes(const JointInfo& jointInfo);
  wdArrayPtr<KeyframeQuat> GetRotationKeyframes(const JointInfo& jointInfo);
  wdArrayPtr<KeyframeVec3> GetScaleKeyframes(const JointInfo& jointInfo);

  wdArrayPtr<const KeyframeVec3> GetPositionKeyframes(const JointInfo& jointInfo) const;
  wdArrayPtr<const KeyframeQuat> GetRotationKeyframes(const JointInfo& jointInfo) const;
  wdArrayPtr<const KeyframeVec3> GetScaleKeyframes(const JointInfo& jointInfo) const;

  wdVec3 m_vConstantRootMotion = wdVec3::ZeroVector();

  wdEventTrack m_EventTrack;

  bool m_bAdditive = false;

private:
  wdArrayMap<wdHashedString, JointInfo> m_JointInfos;
  wdDataBuffer m_Transforms;
  wdUInt32 m_uiNumTotalPositions = 0;
  wdUInt32 m_uiNumTotalRotations = 0;
  wdUInt32 m_uiNumTotalScales = 0;
  wdTime m_Duration;

  struct OzzImpl;
  wdUniquePtr<OzzImpl> m_pOzzImpl;
};

//////////////////////////////////////////////////////////////////////////

using wdAnimationClipResourceHandle = wdTypedResourceHandle<class wdAnimationClipResource>;

class WD_RENDERERCORE_DLL wdAnimationClipResource : public wdResource
{
  WD_ADD_DYNAMIC_REFLECTION(wdAnimationClipResource, wdResource);
  WD_RESOURCE_DECLARE_COMMON_CODE(wdAnimationClipResource);
  WD_RESOURCE_DECLARE_CREATEABLE(wdAnimationClipResource, wdAnimationClipResourceDescriptor);

public:
  wdAnimationClipResource();

  const wdAnimationClipResourceDescriptor& GetDescriptor() const { return *m_pDescriptor; }

private:
  virtual wdResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual wdResourceLoadDesc UpdateContent(wdStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  wdUniquePtr<wdAnimationClipResourceDescriptor> m_pDescriptor;
};
