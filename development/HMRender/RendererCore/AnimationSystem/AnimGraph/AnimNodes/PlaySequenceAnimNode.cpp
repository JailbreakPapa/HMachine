#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/PlaySequenceAnimNode.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdPlaySequenceAnimNode, 1, wdRTTIDefaultAllocator<wdPlaySequenceAnimNode>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("Common", m_State),

    WD_ACCESSOR_PROPERTY("StartClip", GetStartClip, SetStartClip)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),
    WD_ARRAY_ACCESSOR_PROPERTY("MiddleClips", MiddleClips_GetCount, MiddleClips_GetValue, MiddleClips_SetValue, MiddleClips_Insert, MiddleClips_Remove)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),
    WD_ACCESSOR_PROPERTY("EndClip", GetEndClip, SetEndClip)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),

    WD_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("Weights", m_WeightsPin)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("Speed", m_SpeedPin)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("MiddleClipIndex", m_ClipIndexPin)->AddAttributes(new wdHiddenAttribute()),

    WD_MEMBER_PROPERTY("LocalPose", m_LocalPosePin)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("OnNextClip", m_OnNextClipPin)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("PlayingClipIndex", m_PlayingClipIndexPin)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("OnFadeOut", m_OnFadeOutPin)->AddAttributes(new wdHiddenAttribute()),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Animation Sampling"),
    new wdColorAttribute(wdColorScheme::DarkUI(wdColorScheme::Indigo)),
    new wdTitleAttribute("Sequence: '{StartClip}' '{MiddleClips[0]}' '{EndClip}'"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdResult wdPlaySequenceAnimNode::SerializeNode(wdStreamWriter& stream) const
{
  stream.WriteVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  WD_SUCCEED_OR_RETURN(m_State.Serialize(stream));
  stream << m_hStartClip;
  WD_SUCCEED_OR_RETURN(stream.WriteArray(m_hMiddleClips));
  stream << m_hEndClip;

  WD_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_SpeedPin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_ClipIndexPin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_WeightsPin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_LocalPosePin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_OnNextClipPin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_PlayingClipIndexPin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_OnFadeOutPin.Serialize(stream));

  return WD_SUCCESS;
}

wdResult wdPlaySequenceAnimNode::DeserializeNode(wdStreamReader& stream)
{
  stream.ReadVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  WD_SUCCEED_OR_RETURN(m_State.Deserialize(stream));
  stream >> m_hStartClip;
  WD_SUCCEED_OR_RETURN(stream.ReadArray(m_hMiddleClips));
  stream >> m_hEndClip;

  WD_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_SpeedPin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_ClipIndexPin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_WeightsPin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_LocalPosePin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_OnNextClipPin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_PlayingClipIndexPin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_OnFadeOutPin.Deserialize(stream));

  // make sure there are no invalid clips in the middle clip array
  for (wdUInt32 i = m_hMiddleClips.GetCount(); i > 0; i--)
  {
    if (!m_hMiddleClips[i - 1].IsValid())
    {
      m_hMiddleClips.RemoveAtAndSwap(i - 1);
    }
  }

  return WD_SUCCESS;
}

void wdPlaySequenceAnimNode::SetStartClip(const char* szFile)
{
  wdAnimationClipResourceHandle hResource;

  if (!wdStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = wdResourceManager::LoadResource<wdAnimationClipResource>(szFile);
  }

  m_hStartClip = hResource;
}

const char* wdPlaySequenceAnimNode::GetStartClip() const
{
  if (!m_hStartClip.IsValid())
    return "";

  return m_hStartClip.GetResourceID();
}

wdUInt32 wdPlaySequenceAnimNode::MiddleClips_GetCount() const
{
  return m_hMiddleClips.GetCount();
}

const char* wdPlaySequenceAnimNode::MiddleClips_GetValue(wdUInt32 uiIndex) const
{
  const auto& hMat = m_hMiddleClips[uiIndex];

  if (!hMat.IsValid())
    return "";

  return hMat.GetResourceID();
}

void wdPlaySequenceAnimNode::MiddleClips_SetValue(wdUInt32 uiIndex, const char* value)
{
  if (wdStringUtils::IsNullOrEmpty(value))
    m_hMiddleClips[uiIndex] = wdAnimationClipResourceHandle();
  else
    m_hMiddleClips[uiIndex] = wdResourceManager::LoadResource<wdAnimationClipResource>(value);
}

void wdPlaySequenceAnimNode::MiddleClips_Insert(wdUInt32 uiIndex, const char* value)
{
  wdAnimationClipResourceHandle hMat;

  if (!wdStringUtils::IsNullOrEmpty(value))
    hMat = wdResourceManager::LoadResource<wdAnimationClipResource>(value);

  m_hMiddleClips.Insert(hMat, uiIndex);
}

void wdPlaySequenceAnimNode::MiddleClips_Remove(wdUInt32 uiIndex)
{
  m_hMiddleClips.RemoveAtAndCopy(uiIndex);
}

void wdPlaySequenceAnimNode::SetEndClip(const char* szFile)
{
  wdAnimationClipResourceHandle hResource;

  if (!wdStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = wdResourceManager::LoadResource<wdAnimationClipResource>(szFile);
  }

  m_hEndClip = hResource;
}

const char* wdPlaySequenceAnimNode::GetEndClip() const
{
  if (!m_hEndClip.IsValid())
    return "";

  return m_hEndClip.GetResourceID();
}

void wdPlaySequenceAnimNode::Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget)
{
  const bool bActive = m_ActivePin.IsTriggered(graph);

  if (!m_ActivePin.IsConnected() || !m_LocalPosePin.IsConnected() || m_hMiddleClips.IsEmpty() || m_State.WillStateBeOff(bActive))
  {
    m_uiClipToPlay = 0xFF;
    m_uiNextClipToPlay = 0xFF;
    return;
  }

  wdUInt8 uiNextClip = static_cast<wdUInt8>(m_ClipIndexPin.GetNumber(graph, m_uiNextClipToPlay));

  if (uiNextClip >= m_hMiddleClips.GetCount())
  {
    uiNextClip = static_cast<wdUInt8>(pTarget->GetWorld()->GetRandomNumberGenerator().UIntInRange(m_hMiddleClips.GetCount()));
  }

  m_uiNextClipToPlay = uiNextClip;

  if (m_uiClipToPlay >= m_hMiddleClips.GetCount())
  {
    m_uiClipToPlay = uiNextClip;
    m_uiNextClipToPlay = 0xFF; // make sure the next update will pick another random clip
  }

  const bool bWasLooped = m_State.m_bLoop;
  WD_SCOPE_EXIT(m_State.m_bLoop = bWasLooped);

  if (m_Phase == Phase::Off)
  {
    m_Phase = Phase::Start;
  }

  float fPrevPlaybackPos = m_State.GetNormalizedPlaybackPosition();

  m_State.m_fPlaybackSpeedFactor = static_cast<float>(m_SpeedPin.GetNumber(graph, 1.0f));
  m_State.m_DurationOfQueued.SetZero();
  m_State.m_bTriggerActive = m_State.m_bImmediateFadeOut ? bActive : true;

  wdAnimationClipResourceHandle hCurrentClip;
  wdAnimPoseGeneratorCommandID inputCmd = 0xFFFFFFFF;

  if (m_Phase == Phase::Start)
  {
    wdAnimationClipResourceHandle hStartClip = m_hStartClip.IsValid() ? m_hStartClip : m_hMiddleClips[m_uiClipToPlay];
    wdAnimationClipResourceHandle hMiddleClip = m_hMiddleClips[uiNextClip]; // don't use m_uiNextClipToPlay here, it can be 0xFF

    wdResourceLock<wdAnimationClipResource> pClipStart(hStartClip, wdResourceAcquireMode::BlockTillLoaded);

    m_State.m_bLoop = true;
    m_State.m_Duration = pClipStart->GetDescriptor().GetDuration();

    if (m_State.GetCurrentState() == wdAnimState::State::Running)
    {
      wdResourceLock<wdAnimationClipResource> pClipMiddle(hMiddleClip, wdResourceAcquireMode::BlockTillLoaded);

      m_State.m_DurationOfQueued = pClipMiddle->GetDescriptor().GetDuration();
    }

    m_State.UpdateState(tDiff);

    if (m_State.HasTransitioned())
    {
      // guarantee that all animation events from the just finished first clip get evaluated and sent
      {
        auto& cmdE = graph.GetPoseGenerator().AllocCommandSampleEventTrack();
        cmdE.m_hAnimationClip = hStartClip;
        cmdE.m_EventSampling = wdAnimPoseEventTrackSampleMode::OnlyBetween;
        cmdE.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;
        cmdE.m_fNormalizedSamplePos = 1.1f;
        inputCmd = cmdE.GetCommandID();
      }

      m_Phase = Phase::Middle;
      hCurrentClip = hMiddleClip;
      fPrevPlaybackPos = 0.0f;

      m_uiClipToPlay = uiNextClip; // don't use m_uiNextClipToPlay here, it can be 0xFF
      m_uiNextClipToPlay = 0xFF;

      m_OnNextClipPin.SetTriggered(graph, true);
    }
    else
    {
      hCurrentClip = hStartClip;
    }
  }
  else if (m_Phase == Phase::Middle)
  {
    wdAnimationClipResourceHandle hMiddleClip1 = m_hMiddleClips[m_uiClipToPlay];
    wdAnimationClipResourceHandle hMiddleClip2 = (bWasLooped && bActive) ? m_hMiddleClips[uiNextClip] : m_hEndClip; // invalid end clip handled below

    if (!hMiddleClip2.IsValid())
      hMiddleClip2 = hMiddleClip1; // in case end clip doesn't exist

    wdResourceLock<wdAnimationClipResource> pClipMiddle1(hMiddleClip1, wdResourceAcquireMode::BlockTillLoaded);
    wdResourceLock<wdAnimationClipResource> pClipMiddle2(hMiddleClip2, wdResourceAcquireMode::BlockTillLoaded);

    m_State.m_bLoop = true;
    m_State.m_Duration = pClipMiddle1->GetDescriptor().GetDuration();
    m_State.m_DurationOfQueued = pClipMiddle2->GetDescriptor().GetDuration();

    m_State.UpdateState(tDiff);

    hCurrentClip = hMiddleClip1;

    if (m_State.HasTransitioned())
    {
      m_Phase = (bWasLooped && bActive) ? Phase::Middle : Phase::End;

      // guarantee that all animation events from the just finished first clip get evaluated and sent
      {
        auto& cmdE = graph.GetPoseGenerator().AllocCommandSampleEventTrack();
        cmdE.m_hAnimationClip = m_hMiddleClips[m_uiClipToPlay];
        cmdE.m_EventSampling = wdAnimPoseEventTrackSampleMode::OnlyBetween;
        cmdE.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;
        cmdE.m_fNormalizedSamplePos = 1.1f;
        inputCmd = cmdE.GetCommandID();
      }

      fPrevPlaybackPos = 0.0f;
      hCurrentClip = hMiddleClip2;

      m_uiClipToPlay = uiNextClip; // don't use m_uiNextClipToPlay here, it can be 0xFF
      m_uiNextClipToPlay = 0xFF;

      m_OnNextClipPin.SetTriggered(graph, true);
    }
  }
  else if (m_Phase == Phase::End)
  {
    hCurrentClip = m_hEndClip.IsValid() ? m_hEndClip : m_hMiddleClips[m_uiClipToPlay];

    wdResourceLock<wdAnimationClipResource> pClipEnd(hCurrentClip, wdResourceAcquireMode::BlockTillLoaded);

    m_State.m_bTriggerActive = bActive;
    m_State.m_bLoop = false;
    m_State.m_Duration = pClipEnd->GetDescriptor().GetDuration();

    m_State.UpdateState(tDiff);

    if (m_State.GetCurrentState() == wdAnimState::State::StartedRampDown)
    {
      m_OnFadeOutPin.SetTriggered(graph, true);
    }
  }

  if (m_State.GetWeight() <= 0.0f || !hCurrentClip.IsValid())
  {
    m_Phase = Phase::Off;

    m_uiClipToPlay = 0xFF;
    m_uiNextClipToPlay = 0xFF;
    return;
  }

  wdAnimGraphPinDataLocalTransforms* pOutputTransform = graph.AddPinDataLocalTransforms();

  void* pThis = this;
  auto& cmd = graph.GetPoseGenerator().AllocCommandSampleTrack(wdHashingUtils::xxHash32(&pThis, sizeof(pThis)));
  cmd.m_hAnimationClip = hCurrentClip;
  cmd.m_fNormalizedSamplePos = m_State.GetNormalizedPlaybackPosition();
  cmd.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;
  cmd.m_EventSampling = wdAnimPoseEventTrackSampleMode::OnlyBetween; // if there is a loop or transition, we handle that manually

  if (inputCmd != 0xFFFFFFFF)
  {
    cmd.m_Inputs.PushBack(inputCmd);
  }

  switch (m_Phase)
  {
    case wdPlaySequenceAnimNode::Phase::Start:
      m_PlayingClipIndexPin.SetNumber(graph, -1);
      break;
    case wdPlaySequenceAnimNode::Phase::Middle:
      m_PlayingClipIndexPin.SetNumber(graph, m_uiClipToPlay);
      break;
    case wdPlaySequenceAnimNode::Phase::End:
      m_PlayingClipIndexPin.SetNumber(graph, -2);
      break;

      WD_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  {
    wdAnimGraphPinDataLocalTransforms* pLocalTransforms = graph.AddPinDataLocalTransforms();

    pLocalTransforms->m_fOverallWeight = m_State.GetWeight();
    pLocalTransforms->m_pWeights = m_WeightsPin.GetWeights(graph);

    if (m_State.m_bApplyRootMotion)
    {
      pLocalTransforms->m_bUseRootMotion = true;

      wdResourceLock<wdAnimationClipResource> pClip(hCurrentClip, wdResourceAcquireMode::BlockTillLoaded);

      pLocalTransforms->m_vRootMotion = pClip->GetDescriptor().m_vConstantRootMotion * tDiff.AsFloatInSeconds() * m_State.m_fPlaybackSpeed;
    }

    pLocalTransforms->m_CommandID = cmd.GetCommandID();

    m_LocalPosePin.SetPose(graph, pLocalTransforms);
  }
}


WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_PlaySequenceAnimNode);
