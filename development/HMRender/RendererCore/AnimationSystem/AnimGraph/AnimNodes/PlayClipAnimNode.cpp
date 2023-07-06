#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/PlayClipAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdPlayClipAnimNode, 1, wdRTTIDefaultAllocator<wdPlayClipAnimNode>)
  {
    WD_BEGIN_PROPERTIES
    {
      WD_MEMBER_PROPERTY("Common", m_State),
      WD_ARRAY_ACCESSOR_PROPERTY("Clips", Clips_GetCount, Clips_GetValue, Clips_SetValue, Clips_Insert, Clips_Remove)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),

      WD_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new wdHiddenAttribute()),
      WD_MEMBER_PROPERTY("Weights", m_WeightsPin)->AddAttributes(new wdHiddenAttribute()),
      WD_MEMBER_PROPERTY("Speed", m_SpeedPin)->AddAttributes(new wdHiddenAttribute()),
      WD_MEMBER_PROPERTY("ClipIndex", m_ClipIndexPin)->AddAttributes(new wdHiddenAttribute()),

      WD_MEMBER_PROPERTY("LocalPose", m_LocalPosePin)->AddAttributes(new wdHiddenAttribute()),
      WD_MEMBER_PROPERTY("OnFadeOut", m_OnFadeOutPin)->AddAttributes(new wdHiddenAttribute()),
    }
    WD_END_PROPERTIES;
    WD_BEGIN_ATTRIBUTES
    {
      new wdCategoryAttribute("Animation Sampling"),
      new wdColorAttribute(wdColorScheme::DarkUI(wdColorScheme::Blue)),
      new wdTitleAttribute("Play: '{Clips[0]}' '{Clips[1]}' '{Clips[2]}'"),
    }
    WD_END_ATTRIBUTES;
  }
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdResult wdPlayClipAnimNode::SerializeNode(wdStreamWriter& stream) const
{
  stream.WriteVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  WD_SUCCEED_OR_RETURN(m_State.Serialize(stream));
  WD_SUCCEED_OR_RETURN(stream.WriteArray(m_Clips));

  WD_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_SpeedPin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_ClipIndexPin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_WeightsPin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_LocalPosePin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_OnFadeOutPin.Serialize(stream));

  return WD_SUCCESS;
}

wdResult wdPlayClipAnimNode::DeserializeNode(wdStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  WD_SUCCEED_OR_RETURN(m_State.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(stream.ReadArray(m_Clips));

  WD_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_SpeedPin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_ClipIndexPin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_WeightsPin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_LocalPosePin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_OnFadeOutPin.Deserialize(stream));

  // make sure there are no invalid clips in the middle clip array
  for (wdUInt32 i = m_Clips.GetCount(); i > 0; i--)
  {
    if (!m_Clips[i - 1].IsValid())
    {
      m_Clips.RemoveAtAndSwap(i - 1);
    }
  }

  return WD_SUCCESS;
}

void wdPlayClipAnimNode::Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget)
{
  if (m_Clips.IsEmpty() || !m_LocalPosePin.IsConnected() || m_State.WillStateBeOff(m_ActivePin.IsTriggered(graph)))
  {
    m_uiClipToPlay = 0xFF;
    m_uiNextClipToPlay = 0xFF;
    return;
  }

  wdUInt8 uiNextClip = static_cast<wdUInt8>(m_ClipIndexPin.GetNumber(graph, m_uiNextClipToPlay));

  if (uiNextClip >= m_Clips.GetCount())
  {
    uiNextClip = static_cast<wdUInt8>(pTarget->GetWorld()->GetRandomNumberGenerator().UIntInRange(m_Clips.GetCount()));
  }

  if (m_uiNextClipToPlay != uiNextClip)
  {
    wdResourceLock<wdAnimationClipResource> pNextClip(m_Clips[uiNextClip], wdResourceAcquireMode::BlockTillLoaded);
    if (pNextClip.GetAcquireResult() != wdResourceAcquireResult::Final)
      return;

    m_uiNextClipToPlay = uiNextClip;
    m_NextClipDuration = pNextClip->GetDescriptor().GetDuration();
  }

  if (m_uiClipToPlay >= m_Clips.GetCount())
  {
    m_uiClipToPlay = uiNextClip;
    m_uiNextClipToPlay = 0xFF; // make sure the next update will pick another random clip
  }

  wdResourceLock<wdAnimationClipResource> pAnimClip(m_Clips[m_uiClipToPlay], wdResourceAcquireMode::BlockTillLoaded);
  if (pAnimClip.GetAcquireResult() != wdResourceAcquireResult::Final)
    return;

  float fPrevPlaybackPos = m_State.GetNormalizedPlaybackPosition();

  m_State.m_bTriggerActive = m_ActivePin.IsTriggered(graph);
  m_State.m_Duration = pAnimClip->GetDescriptor().GetDuration();
  m_State.m_DurationOfQueued = m_State.m_bLoop ? m_NextClipDuration : wdTime::Zero();
  m_State.m_fPlaybackSpeedFactor = static_cast<float>(m_SpeedPin.GetNumber(graph, 1.0));

  m_State.UpdateState(tDiff);

  void* pThis = this;
  auto& cmd = graph.GetPoseGenerator().AllocCommandSampleTrack(wdHashingUtils::xxHash32(&pThis, sizeof(pThis)));

  if (m_Clips.GetCount() > 1 && m_State.HasTransitioned())
  {
    // guarantee that all animation events from the just finished first clip get evaluated and sent
    {
      auto& cmdE = graph.GetPoseGenerator().AllocCommandSampleEventTrack();
      cmdE.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;
      cmdE.m_fNormalizedSamplePos = m_State.GetFinalSpeed() > 0 ? 1.1f : -0.1f;
      cmdE.m_EventSampling = wdAnimPoseEventTrackSampleMode::OnlyBetween;
      cmdE.m_hAnimationClip = m_Clips[m_uiClipToPlay];

      cmd.m_Inputs.PushBack(cmdE.GetCommandID());
    }

    m_uiClipToPlay = uiNextClip; // don't use m_uiNextClipToPlay here, it can be 0xFF
    m_uiNextClipToPlay = 0xFF;
    m_NextClipDuration.SetZero();

    fPrevPlaybackPos = 0.0f;
  }

  if (m_State.GetCurrentState() == wdAnimState::State::StartedRampDown)
  {
    m_OnFadeOutPin.SetTriggered(graph, true);
  }

  cmd.m_hAnimationClip = m_Clips[m_uiClipToPlay];
  cmd.m_fNormalizedSamplePos = m_State.GetNormalizedPlaybackPosition();
  cmd.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;

  if (m_State.HasLoopedStart())
    cmd.m_EventSampling = wdAnimPoseEventTrackSampleMode::LoopAtStart;
  else if (m_State.HasLoopedEnd())
    cmd.m_EventSampling = wdAnimPoseEventTrackSampleMode::LoopAtEnd;
  else
    cmd.m_EventSampling = wdAnimPoseEventTrackSampleMode::OnlyBetween;

  {
    wdAnimGraphPinDataLocalTransforms* pLocalTransforms = graph.AddPinDataLocalTransforms();

    pLocalTransforms->m_fOverallWeight = m_State.GetWeight();
    pLocalTransforms->m_pWeights = m_WeightsPin.GetWeights(graph);

    if (m_State.m_bApplyRootMotion)
    {
      pLocalTransforms->m_bUseRootMotion = true;

      pLocalTransforms->m_vRootMotion = pAnimClip->GetDescriptor().m_vConstantRootMotion * tDiff.AsFloatInSeconds() * m_State.m_fPlaybackSpeed;
    }

    pLocalTransforms->m_CommandID = cmd.GetCommandID();

    m_LocalPosePin.SetPose(graph, pLocalTransforms);
  }
}

wdUInt32 wdPlayClipAnimNode::Clips_GetCount() const
{
  return m_Clips.GetCount();
}

const char* wdPlayClipAnimNode::Clips_GetValue(wdUInt32 uiIndex) const
{
  const auto& hMat = m_Clips[uiIndex];

  if (!hMat.IsValid())
    return "";

  return hMat.GetResourceID();
}

void wdPlayClipAnimNode::Clips_SetValue(wdUInt32 uiIndex, const char* value)
{
  if (wdStringUtils::IsNullOrEmpty(value))
    m_Clips[uiIndex] = wdAnimationClipResourceHandle();
  else
  {
    m_Clips[uiIndex] = wdResourceManager::LoadResource<wdAnimationClipResource>(value);
  }
}

void wdPlayClipAnimNode::Clips_Insert(wdUInt32 uiIndex, const char* value)
{
  wdAnimationClipResourceHandle hMat;

  if (!wdStringUtils::IsNullOrEmpty(value))
    hMat = wdResourceManager::LoadResource<wdAnimationClipResource>(value);

  m_Clips.Insert(hMat, uiIndex);
}

void wdPlayClipAnimNode::Clips_Remove(wdUInt32 uiIndex)
{
  m_Clips.RemoveAtAndCopy(uiIndex);
}


WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_PlayClipAnimNode);
