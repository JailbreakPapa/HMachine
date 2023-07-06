#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/MixClips2DAnimNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
WD_BEGIN_STATIC_REFLECTED_TYPE(wdAnimClip2D, wdNoBase, 1, wdRTTIDefaultAllocator<wdAnimClip2D>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("Clip", GetAnimationFile, SetAnimationFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),
    WD_MEMBER_PROPERTY("Position", m_vPosition),
  }
  WD_END_PROPERTIES;
}
WD_END_STATIC_REFLECTED_TYPE;

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdMixClips2DAnimNode, 1, wdRTTIDefaultAllocator<wdMixClips2DAnimNode>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("Common", m_State),

    WD_MEMBER_PROPERTY("InputResponse", m_InputResponse),
    WD_ACCESSOR_PROPERTY("CenterClip", GetCenterClipFile, SetCenterClipFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),
    WD_ARRAY_MEMBER_PROPERTY("Clips", m_Clips),

    WD_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("Weights", m_WeightsPin)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("Speed", m_SpeedPin)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("X", m_XCoordPin)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("Y", m_YCoordPin)->AddAttributes(new wdHiddenAttribute()),

    WD_MEMBER_PROPERTY("LocalPose", m_LocalPosePin)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("OnFadeOut", m_OnFadeOutPin)->AddAttributes(new wdHiddenAttribute()),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Animation Sampling"),
    new wdColorAttribute(wdColorScheme::DarkUI(wdColorScheme::Blue)),
    new wdTitleAttribute("Mix2D '{CenterClip}'"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void wdAnimClip2D::SetAnimationFile(const char* szSz)
{
  wdAnimationClipResourceHandle hResource;

  if (!wdStringUtils::IsNullOrEmpty(szSz))
  {
    hResource = wdResourceManager::LoadResource<wdAnimationClipResource>(szSz);
  }

  m_hAnimation = hResource;
}

const char* wdAnimClip2D::GetAnimationFile() const
{
  if (m_hAnimation.IsValid())
  {
    return m_hAnimation.GetResourceID();
  }

  return "";
}

wdResult wdMixClips2DAnimNode::SerializeNode(wdStreamWriter& stream) const
{
  stream.WriteVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  WD_SUCCEED_OR_RETURN(m_State.Serialize(stream));

  stream << m_hCenterClip;

  stream << m_Clips.GetCount();
  for (wdUInt32 i = 0; i < m_Clips.GetCount(); ++i)
  {
    stream << m_Clips[i].m_hAnimation;
    stream << m_Clips[i].m_vPosition;
  }

  WD_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_WeightsPin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_SpeedPin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_XCoordPin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_YCoordPin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_LocalPosePin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_OnFadeOutPin.Serialize(stream));

  stream << m_InputResponse;

  return WD_SUCCESS;
}

wdResult wdMixClips2DAnimNode::DeserializeNode(wdStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  WD_SUCCEED_OR_RETURN(m_State.Deserialize(stream));

  stream >> m_hCenterClip;

  wdUInt32 num = 0;
  stream >> num;
  m_Clips.SetCount(num);
  for (wdUInt32 i = 0; i < m_Clips.GetCount(); ++i)
  {
    stream >> m_Clips[i].m_hAnimation;
    stream >> m_Clips[i].m_vPosition;
  }

  WD_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_WeightsPin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_SpeedPin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_XCoordPin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_YCoordPin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_LocalPosePin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_OnFadeOutPin.Deserialize(stream));

  stream >> m_InputResponse;

  return WD_SUCCESS;
}

void wdMixClips2DAnimNode::Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget)
{
  if (!m_LocalPosePin.IsConnected() || (!m_XCoordPin.IsConnected() && !m_YCoordPin.IsConnected()) || m_Clips.IsEmpty())
    return;

  if (m_State.WillStateBeOff(m_ActivePin.IsTriggered(graph)))
    return;

  m_State.m_bTriggerActive = m_ActivePin.IsTriggered(graph);
  m_State.m_fPlaybackSpeedFactor = static_cast<float>(m_SpeedPin.GetNumber(graph, 1.0));

  const float x = static_cast<float>(m_XCoordPin.GetNumber(graph));
  const float y = static_cast<float>(m_YCoordPin.GetNumber(graph));

  if (m_InputResponse.IsZeroOrNegative())
  {
    m_fLastValueX = x;
    m_fLastValueY = y;
  }
  else
  {
    const float lerp = static_cast<float>(wdMath::Min(1.0, tDiff.GetSeconds() * (1.0 / m_InputResponse.GetSeconds())));
    m_fLastValueX = wdMath::Lerp(m_fLastValueX, x, lerp);
    m_fLastValueY = wdMath::Lerp(m_fLastValueY, y, lerp);
  }

  wdUInt32 uiMaxWeightClip = 0;
  wdHybridArray<ClipToPlay, 8> clips;
  ComputeClipsAndWeights(wdVec2(m_fLastValueX, m_fLastValueY), clips, uiMaxWeightClip);

  PlayClips(graph, tDiff, clips, uiMaxWeightClip);

  if (m_State.GetCurrentState() == wdAnimState::State::StartedRampDown)
  {
    m_OnFadeOutPin.SetTriggered(graph, true);
  }
}

void wdMixClips2DAnimNode::ComputeClipsAndWeights(const wdVec2& p, wdDynamicArray<ClipToPlay>& clips, wdUInt32& out_uiMaxWeightClip)
{
  out_uiMaxWeightClip = 0;
  float fMaxWeight = -1.0f;

  if (m_Clips.GetCount() == 1 && !m_hCenterClip.IsValid())
  {
    clips.ExpandAndGetRef().m_uiIndex = 0;
  }
  else
  {
    // this algorithm is taken from http://runevision.com/thesis chapter 6.3 "Gradient Band Interpolation"
    // also see http://answers.unity.com/answers/1208837/view.html

    float fWeightNormalization = 0.0f;

    for (wdUInt32 i = 0; i < m_Clips.GetCount(); ++i)
    {
      const wdVec2 pi = m_Clips[i].m_vPosition;
      float fMinWeight = 1.0f;

      for (wdUInt32 j = 0; j < m_Clips.GetCount(); ++j)
      {
        const wdVec2 pj = m_Clips[j].m_vPosition;

        const float fLenSqr = (pi - pj).GetLengthSquared();
        const float fProjLenSqr = (pi - p).Dot(pi - pj);

        // filters out both (i == j) and cases where another clip is in the same place and would result in division by zero
        if (fLenSqr <= 0.0f)
          continue;

        const float fWeight = 1.0f - (fProjLenSqr / fLenSqr);
        fMinWeight = wdMath::Min(fMinWeight, fWeight);
      }

      // also check against center clip
      if (m_hCenterClip.IsValid())
      {
        const float fLenSqr = pi.GetLengthSquared();
        const float fProjLenSqr = (pi - p).Dot(pi);

        // filters out both (i == j) and cases where another clip is in the same place and would result in division by zero
        if (fLenSqr <= 0.0f)
          continue;

        const float fWeight = 1.0f - (fProjLenSqr / fLenSqr);
        fMinWeight = wdMath::Min(fMinWeight, fWeight);
      }

      if (fMinWeight > 0.0f)
      {
        auto& c = clips.ExpandAndGetRef();
        c.m_uiIndex = i;
        c.m_fWeight = fMinWeight;

        fWeightNormalization += fMinWeight;
      }
    }

    // also compute weight for center clip
    if (m_hCenterClip.IsValid())
    {
      float fMinWeight = 1.0f;

      for (wdUInt32 j = 0; j < m_Clips.GetCount(); ++j)
      {
        const wdVec2 pj = m_Clips[j].m_vPosition;

        const float fLenSqr = pj.GetLengthSquared();
        const float fProjLenSqr = (-p).Dot(-pj);

        // filters out both (i == j) and cases where another clip is in the same place and would result in division by zero
        if (fLenSqr <= 0.0f)
          continue;

        const float fWeight = 1.0f - (fProjLenSqr / fLenSqr);
        fMinWeight = wdMath::Min(fMinWeight, fWeight);
      }

      if (fMinWeight > 0.0f)
      {
        auto& c = clips.ExpandAndGetRef();
        c.m_uiIndex = 0xFFFFFFFF;
        c.m_fWeight = fMinWeight;

        fWeightNormalization += fMinWeight;
      }
    }

    fWeightNormalization = 1.0f / fWeightNormalization;

    for (wdUInt32 i = 0; i < clips.GetCount(); ++i)
    {
      auto& c = clips[i];

      c.m_fWeight *= fWeightNormalization;

      if (c.m_fWeight > fMaxWeight)
      {
        fMaxWeight = c.m_fWeight;
        out_uiMaxWeightClip = i;
      }
    }
  }
}

void wdMixClips2DAnimNode::SetCenterClipFile(const char* szSz)
{
  wdAnimationClipResourceHandle hResource;

  if (!wdStringUtils::IsNullOrEmpty(szSz))
  {
    hResource = wdResourceManager::LoadResource<wdAnimationClipResource>(szSz);
  }

  m_hCenterClip = hResource;
}

const char* wdMixClips2DAnimNode::GetCenterClipFile() const
{
  if (!m_hCenterClip.IsValid())
    return nullptr;

  return m_hCenterClip.GetResourceID();
}

void wdMixClips2DAnimNode::UpdateCenterClipPlaybackTime(wdAnimGraph& graph, wdTime tDiff, wdAnimPoseEventTrackSampleMode& out_eventSamplingCenter)
{
  const float fSpeed = m_State.m_fPlaybackSpeed * static_cast<float>(m_SpeedPin.GetNumber(graph, 1.0));

  if (m_hCenterClip.IsValid())
  {
    wdResourceLock<wdAnimationClipResource> pClip(m_hCenterClip, wdResourceAcquireMode::BlockTillLoaded);

    const wdTime tDur = pClip->GetDescriptor().GetDuration();

    m_CenterPlaybackTime += tDiff * fSpeed;

    // always loop the center clip
    while (m_CenterPlaybackTime > tDur)
    {
      m_CenterPlaybackTime -= tDur;
      out_eventSamplingCenter = wdAnimPoseEventTrackSampleMode::LoopAtEnd;
    }
    while (m_CenterPlaybackTime < wdTime::Zero())
    {
      m_CenterPlaybackTime += tDur;
      out_eventSamplingCenter = wdAnimPoseEventTrackSampleMode::LoopAtStart;
    }
  }
}

void wdMixClips2DAnimNode::PlayClips(wdAnimGraph& graph, wdTime tDiff, wdArrayPtr<ClipToPlay> clips, wdUInt32 uiMaxWeightClip)
{
  wdTime tAvgDuration = wdTime::Zero();

  wdHybridArray<wdAnimPoseGeneratorCommandSampleTrack*, 8> pSampleTrack;
  pSampleTrack.SetCountUninitialized(clips.GetCount());

  wdVec3 vRootMotion = wdVec3::ZeroVector();
  wdUInt32 uiNumAvgClips = 0;

  for (wdUInt32 i = 0; i < clips.GetCount(); ++i)
  {
    const auto& c = clips[i];

    const wdAnimationClipResourceHandle& hClip = c.m_uiIndex >= 0xFF ? m_hCenterClip : m_Clips[c.m_uiIndex].m_hAnimation;

    wdResourceLock<wdAnimationClipResource> pClip(hClip, wdResourceAcquireMode::BlockTillLoaded);

    if (c.m_uiIndex < 0xFF) // center clip should not contribute to the average time
    {
      ++uiNumAvgClips;
      tAvgDuration += pClip->GetDescriptor().GetDuration();
    }

    void* pThis = this;
    auto& cmd = graph.GetPoseGenerator().AllocCommandSampleTrack(wdHashingUtils::xxHash32(&pThis, sizeof(pThis), i));
    cmd.m_hAnimationClip = hClip;
    cmd.m_fNormalizedSamplePos = pClip->GetDescriptor().GetDuration().AsFloatInSeconds(); // will be combined with actual pos below

    pSampleTrack[i] = &cmd;
    vRootMotion += pClip->GetDescriptor().m_vConstantRootMotion * c.m_fWeight;
  }

  if (uiNumAvgClips > 0)
  {
    tAvgDuration = tAvgDuration / uiNumAvgClips;
  }

  const wdTime fPrevCenterPlaybackPos = m_CenterPlaybackTime;
  const float fPrevPlaybackPos = m_State.GetNormalizedPlaybackPosition();

  // now that we know the duration, we can finally update the playback state
  m_State.m_Duration = wdMath::Max(tAvgDuration, wdTime::Milliseconds(16));
  m_State.UpdateState(tDiff);

  if (m_State.GetWeight() <= 0.0f)
    return;

  wdAnimPoseEventTrackSampleMode eventSamplingCenter = wdAnimPoseEventTrackSampleMode::OnlyBetween;
  wdAnimPoseEventTrackSampleMode eventSampling = wdAnimPoseEventTrackSampleMode::OnlyBetween;

  UpdateCenterClipPlaybackTime(graph, tDiff, eventSamplingCenter);

  if (m_State.HasLoopedStart())
    eventSampling = wdAnimPoseEventTrackSampleMode::LoopAtStart;
  else if (m_State.HasLoopedEnd())
    eventSampling = wdAnimPoseEventTrackSampleMode::LoopAtEnd;

  for (wdUInt32 i = 0; i < clips.GetCount(); ++i)
  {

    if (pSampleTrack[i]->m_hAnimationClip == m_hCenterClip)
    {
      pSampleTrack[i]->m_fPreviousNormalizedSamplePos = fPrevCenterPlaybackPos.AsFloatInSeconds() / pSampleTrack[i]->m_fNormalizedSamplePos;
      pSampleTrack[i]->m_fNormalizedSamplePos = m_CenterPlaybackTime.AsFloatInSeconds() / pSampleTrack[i]->m_fNormalizedSamplePos;
      pSampleTrack[i]->m_EventSampling = uiMaxWeightClip == i ? eventSamplingCenter : wdAnimPoseEventTrackSampleMode::None;
    }
    else
    {
      pSampleTrack[i]->m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;
      pSampleTrack[i]->m_fNormalizedSamplePos = m_State.GetNormalizedPlaybackPosition();
      pSampleTrack[i]->m_EventSampling = uiMaxWeightClip == i ? eventSampling : wdAnimPoseEventTrackSampleMode::None;
    }
  }

  wdAnimGraphPinDataLocalTransforms* pOutputTransform = graph.AddPinDataLocalTransforms();
  pOutputTransform->m_fOverallWeight = m_State.GetWeight();
  pOutputTransform->m_pWeights = m_WeightsPin.GetWeights(graph);

  if (m_State.m_bApplyRootMotion)
  {
    pOutputTransform->m_bUseRootMotion = true;

    const float fSpeed = m_State.m_fPlaybackSpeed * static_cast<float>(m_SpeedPin.GetNumber(graph, 1.0));

    pOutputTransform->m_vRootMotion = tDiff.AsFloatInSeconds() * vRootMotion * fSpeed;
  }

  if (clips.GetCount() == 1)
  {
    pOutputTransform->m_CommandID = pSampleTrack[0]->GetCommandID();
  }
  else
  {
    auto& cmdCmb = graph.GetPoseGenerator().AllocCommandCombinePoses();
    pOutputTransform->m_CommandID = cmdCmb.GetCommandID();

    cmdCmb.m_InputWeights.SetCountUninitialized(clips.GetCount());
    cmdCmb.m_Inputs.SetCountUninitialized(clips.GetCount());

    for (wdUInt32 i = 0; i < clips.GetCount(); ++i)
    {
      cmdCmb.m_InputWeights[i] = clips[i].m_fWeight;
      cmdCmb.m_Inputs[i] = pSampleTrack[i]->GetCommandID();
    }
  }

  m_LocalPosePin.SetPose(graph, pOutputTransform);
}


WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_MixClips2DAnimNode);
