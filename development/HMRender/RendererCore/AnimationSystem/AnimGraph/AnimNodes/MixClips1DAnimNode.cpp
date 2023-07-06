#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/MixClips1DAnimNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
WD_BEGIN_STATIC_REFLECTED_TYPE(wdAnimClip1D, wdNoBase, 1, wdRTTIDefaultAllocator<wdAnimClip1D>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("Clip", GetAnimationFile, SetAnimationFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),
    WD_MEMBER_PROPERTY("Position", m_fPosition),
  }
  WD_END_PROPERTIES;
}
WD_END_STATIC_REFLECTED_TYPE;

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdMixClips1DAnimNode, 1, wdRTTIDefaultAllocator<wdMixClips1DAnimNode>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("Common", m_State),

    WD_ARRAY_MEMBER_PROPERTY("Clips", m_Clips),

    WD_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("Weights", m_WeightsPin)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("Speed", m_SpeedPin)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("Lerp", m_LerpPin)->AddAttributes(new wdHiddenAttribute()),

    WD_MEMBER_PROPERTY("LocalPose", m_LocalPosePin)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("OnFadeOut", m_OnFadeOutPin)->AddAttributes(new wdHiddenAttribute()),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Animation Sampling"),
    new wdColorAttribute(wdColorScheme::DarkUI(wdColorScheme::Blue)),
    new wdTitleAttribute("Mix1D: '{AnimationClip0}' '{AnimationClip1}' '{AnimationClip2}' '{AnimationClip3}'"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void wdAnimClip1D::SetAnimationFile(const char* szSz)
{
  wdAnimationClipResourceHandle hResource;

  if (!wdStringUtils::IsNullOrEmpty(szSz))
  {
    hResource = wdResourceManager::LoadResource<wdAnimationClipResource>(szSz);
  }

  m_hAnimation = hResource;
}

const char* wdAnimClip1D::GetAnimationFile() const
{
  if (m_hAnimation.IsValid())
  {
    return m_hAnimation.GetResourceID();
  }

  return "";
}

wdResult wdMixClips1DAnimNode::SerializeNode(wdStreamWriter& stream) const
{
  stream.WriteVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  WD_SUCCEED_OR_RETURN(m_State.Serialize(stream));

  stream << m_Clips.GetCount();
  for (wdUInt32 i = 0; i < m_Clips.GetCount(); ++i)
  {
    stream << m_Clips[i].m_hAnimation;
    stream << m_Clips[i].m_fPosition;
  }

  WD_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_WeightsPin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_SpeedPin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_LerpPin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_LocalPosePin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_OnFadeOutPin.Serialize(stream));

  return WD_SUCCESS;
}

wdResult wdMixClips1DAnimNode::DeserializeNode(wdStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  WD_SUCCEED_OR_RETURN(m_State.Deserialize(stream));

  wdUInt32 num = 0;
  stream >> num;
  m_Clips.SetCount(num);
  for (wdUInt32 i = 0; i < m_Clips.GetCount(); ++i)
  {
    stream >> m_Clips[i].m_hAnimation;
    stream >> m_Clips[i].m_fPosition;
  }

  WD_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_WeightsPin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_SpeedPin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_LerpPin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_LocalPosePin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_OnFadeOutPin.Deserialize(stream));

  return WD_SUCCESS;
}

void wdMixClips1DAnimNode::Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget)
{
  if (!m_LocalPosePin.IsConnected() || !m_LerpPin.IsConnected() || m_Clips.IsEmpty())
    return;

  if (m_State.WillStateBeOff(m_ActivePin.IsTriggered(graph)))
    return;

  wdUInt32 uiClip1 = 0;
  wdUInt32 uiClip2 = 0;

  const float fLerpPos = (float)m_LerpPin.GetNumber(graph);

  if (m_Clips.GetCount() > 1)
  {
    float fDist1 = 1000000.0f;
    float fDist2 = 1000000.0f;

    for (wdUInt32 i = 0; i < m_Clips.GetCount(); ++i)
    {
      const float dist = wdMath::Abs(m_Clips[i].m_fPosition - fLerpPos);

      if (dist < fDist1)
      {
        fDist2 = fDist1;
        uiClip2 = uiClip1;

        fDist1 = dist;
        uiClip1 = i;
      }
      else if (dist < fDist2)
      {
        fDist2 = dist;
        uiClip2 = i;
      }
    }

    if (wdMath::IsZero(fDist1, wdMath::SmallEpsilon<float>()))
    {
      uiClip2 = uiClip1;
    }
  }

  if (!m_Clips[uiClip1].m_hAnimation.IsValid() || !m_Clips[uiClip2].m_hAnimation.IsValid())
    return;

  float fLerpFactor = 0.0f;

  if (uiClip1 != uiClip2)
  {
    const float len = m_Clips[uiClip2].m_fPosition - m_Clips[uiClip1].m_fPosition;
    fLerpFactor = (fLerpPos - m_Clips[uiClip1].m_fPosition) / len;

    // clamp and reduce to single sample when possible
    if (fLerpFactor <= 0.0f)
    {
      fLerpFactor = 0.0f;
      uiClip2 = uiClip1;
    }
    else if (fLerpFactor >= 1.0f)
    {
      fLerpFactor = 1.0f;
      uiClip1 = uiClip2;
    }
  }

  wdResourceLock<wdAnimationClipResource> pAnimClip1(m_Clips[uiClip1].m_hAnimation, wdResourceAcquireMode::BlockTillLoaded);
  wdResourceLock<wdAnimationClipResource> pAnimClip2(m_Clips[uiClip2].m_hAnimation, wdResourceAcquireMode::BlockTillLoaded);

  if (pAnimClip1.GetAcquireResult() != wdResourceAcquireResult::Final || pAnimClip2.GetAcquireResult() != wdResourceAcquireResult::Final)
    return;

  const auto& animDesc1 = pAnimClip1->GetDescriptor();
  const auto& animDesc2 = pAnimClip2->GetDescriptor();

  const wdTime avgDuration = wdMath::Lerp(animDesc1.GetDuration(), animDesc2.GetDuration(), fLerpFactor);

  const float fPrevPlaybackPos = m_State.GetNormalizedPlaybackPosition();

  m_State.m_bTriggerActive = m_ActivePin.IsTriggered(graph);
  m_State.m_fPlaybackSpeedFactor = static_cast<float>(m_SpeedPin.GetNumber(graph, 1.0));
  m_State.m_Duration = avgDuration;

  m_State.UpdateState(tDiff);

  if (m_State.GetCurrentState() == wdAnimState::State::StartedRampDown)
  {
    m_OnFadeOutPin.SetTriggered(graph, true);
  }

  if (m_State.GetWeight() <= 0.0f)
    return;

  wdAnimGraphPinDataLocalTransforms* pOutputTransform = graph.AddPinDataLocalTransforms();

  wdAnimPoseEventTrackSampleMode eventSampling = wdAnimPoseEventTrackSampleMode::OnlyBetween;

  if (m_State.HasLoopedStart())
    eventSampling = wdAnimPoseEventTrackSampleMode::LoopAtStart;
  else if (m_State.HasLoopedEnd())
    eventSampling = wdAnimPoseEventTrackSampleMode::LoopAtEnd;

  auto& poseGen = graph.GetPoseGenerator();

  if (uiClip1 == uiClip2)
  {
    void* pThis = this;
    auto& cmd = poseGen.AllocCommandSampleTrack(wdHashingUtils::xxHash32(&pThis, sizeof(pThis), 0));
    cmd.m_hAnimationClip = m_Clips[uiClip1].m_hAnimation;
    cmd.m_fNormalizedSamplePos = m_State.GetNormalizedPlaybackPosition();
    cmd.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;
    cmd.m_EventSampling = eventSampling;

    pOutputTransform->m_CommandID = cmd.GetCommandID();
  }
  else
  {
    auto& cmdCmb = poseGen.AllocCommandCombinePoses();
    pOutputTransform->m_CommandID = cmdCmb.GetCommandID();

    // sample animation 1
    {
      void* pThis = this;
      auto& cmd = poseGen.AllocCommandSampleTrack(wdHashingUtils::xxHash32(&pThis, sizeof(pThis), 0));
      cmd.m_hAnimationClip = m_Clips[uiClip1].m_hAnimation;
      cmd.m_fNormalizedSamplePos = m_State.GetNormalizedPlaybackPosition();
      cmd.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;
      cmd.m_EventSampling = fLerpFactor <= 0.5f ? eventSampling : wdAnimPoseEventTrackSampleMode::None; // only the stronger influence will trigger events

      cmdCmb.m_Inputs.PushBack(cmd.GetCommandID());
      cmdCmb.m_InputWeights.PushBack(1.0f - fLerpFactor);
    }

    // sample animation 2
    {
      void* pThis = this;
      auto& cmd = poseGen.AllocCommandSampleTrack(wdHashingUtils::xxHash32(&pThis, sizeof(pThis), 1));
      cmd.m_hAnimationClip = m_Clips[uiClip2].m_hAnimation;
      cmd.m_fNormalizedSamplePos = m_State.GetNormalizedPlaybackPosition();
      cmd.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;
      cmd.m_EventSampling = fLerpFactor > 0.5f ? eventSampling : wdAnimPoseEventTrackSampleMode::None; // only the stronger influence will trigger events

      cmdCmb.m_Inputs.PushBack(cmd.GetCommandID());
      cmdCmb.m_InputWeights.PushBack(fLerpFactor);
    }
  }

  // send to output
  {
    pOutputTransform->m_fOverallWeight = m_State.GetWeight();
    pOutputTransform->m_pWeights = m_WeightsPin.GetWeights(graph);

    if (m_State.m_bApplyRootMotion)
    {
      pOutputTransform->m_bUseRootMotion = true;

      pOutputTransform->m_vRootMotion = wdMath::Lerp(animDesc1.m_vConstantRootMotion, animDesc2.m_vConstantRootMotion, fLerpFactor) * tDiff.AsFloatInSeconds() * m_State.m_fPlaybackSpeed;
    }

    m_LocalPosePin.SetPose(graph, pOutputTransform);
  }
}


WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_MixClips1DAnimNode);
