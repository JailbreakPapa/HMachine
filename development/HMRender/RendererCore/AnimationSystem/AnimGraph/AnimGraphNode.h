#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Time/Time.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphPins.h>

class wdSkeletonResource;
class wdGameObject;
class wdAnimGraph;
class wdStreamWriter;
class wdStreamReader;
struct wdAnimGraphPinDataLocalTransforms;
struct wdAnimGraphPinDataBoneWeights;
class wdAnimationClipResource;
using wdAnimationClipResourceHandle = wdTypedResourceHandle<class wdAnimationClipResource>;

namespace ozz
{
  namespace animation
  {
    class Animation;
  }
} // namespace ozz

/// \brief Base class for all nodes in an wdAnimGraph
///
/// These nodes are used to configure which skeletal animations can be played on an object,
/// and how they would be played back exactly.
/// The nodes implement different functionality. For example logic nodes are used to figure out how to play an animation,
/// other nodes then sample and combining animation poses, and yet other nodes can inform the user about events
/// or they write state back to the animation graph's blackboard.
class WD_RENDERERCORE_DLL wdAnimGraphNode : public wdReflectedClass
{
  WD_ADD_DYNAMIC_REFLECTION(wdAnimGraphNode, wdReflectedClass);

public:
  wdAnimGraphNode();
  virtual ~wdAnimGraphNode();

  //////////////////////////////////////////////////////////////////////////
  // wdAnimGraphNode

  const char* GetCustomNodeTitle() const { return m_sCustomNodeTitle.GetString(); }
  void SetCustomNodeTitle(const char* szSz) { m_sCustomNodeTitle.Assign(szSz); }

protected:
  friend class wdAnimGraph;

  wdHashedString m_sCustomNodeTitle;

  virtual wdResult SerializeNode(wdStreamWriter& stream) const = 0;
  virtual wdResult DeserializeNode(wdStreamReader& stream) = 0;

  virtual void Initialize(wdAnimGraph& graph, const wdSkeletonResource* pSkeleton) {}
  virtual void Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget) = 0;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct WD_RENDERERCORE_DLL wdAnimState
{
  enum class State
  {
    Off,
    StartedRampUp,
    RampingUp,
    Running,
    StartedRampDown,
    RampingDown,
    Finished,
  };

  // Properties:
  wdTime m_FadeIn;                  // [ property ]
  wdTime m_FadeOut;                 // [ property ]
  bool m_bImmediateFadeIn = false;  // [ property ]
  bool m_bImmediateFadeOut = false; // [ property ]
  bool m_bLoop = false;             // [ property ]
  float m_fPlaybackSpeed = 1.0f;    // [ property ]
  bool m_bApplyRootMotion = false;  // [ property ]

  // Inputs:
  bool m_bTriggerActive = false;
  float m_fPlaybackSpeedFactor = 1.0f;
  wdTime m_Duration;
  wdTime m_DurationOfQueued;

  bool WillStateBeOff(bool bTriggerActive) const;
  void UpdateState(wdTime diff);
  State GetCurrentState() const { return m_State; }
  float GetWeight() const { return m_fCurWeight; }
  float GetNormalizedPlaybackPosition() const { return m_fNormalizedPlaybackPosition; }
  bool HasTransitioned() const { return m_bHasTransitioned; }
  bool HasLoopedStart() const { return m_bHasLoopedStart; }
  bool HasLoopedEnd() const { return m_bHasLoopedEnd; }
  float GetFinalSpeed() const { return m_fPlaybackSpeed * m_fPlaybackSpeedFactor; }

  wdResult Serialize(wdStreamWriter& inout_stream) const;
  wdResult Deserialize(wdStreamReader& inout_stream);

private:
  void RampWeightUpOrDown(float& inout_fWeight, float fTargetWeight, wdTime tDiff) const;

  State m_State = State::Off;
  float m_fNormalizedPlaybackPosition = 0.0f;
  bool m_bRequireLoopForRampDown = true;
  bool m_bHasTransitioned = false;
  bool m_bHasLoopedStart = false;
  bool m_bHasLoopedEnd = false;
  float m_fCurWeight = 0.0f;
};

WD_DECLARE_REFLECTABLE_TYPE(WD_RENDERERCORE_DLL, wdAnimState);
