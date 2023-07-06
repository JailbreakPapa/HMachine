#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class WD_RENDERERCORE_DLL wdPlaySequenceAnimNode : public wdAnimGraphNode
{
  WD_ADD_DYNAMIC_REFLECTION(wdPlaySequenceAnimNode, wdAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // wdAnimGraphNode

protected:
  virtual wdResult SerializeNode(wdStreamWriter& stream) const override;
  virtual wdResult DeserializeNode(wdStreamReader& stream) override;

  virtual void Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // wdPlaySequenceAnimNode

public:
  void SetStartClip(const char* szFile); // [ property ]
  const char* GetStartClip() const;      // [ property ]

  wdUInt32 MiddleClips_GetCount() const;                          // [ property ]
  const char* MiddleClips_GetValue(wdUInt32 uiIndex) const;       // [ property ]
  void MiddleClips_SetValue(wdUInt32 uiIndex, const char* value); // [ property ]
  void MiddleClips_Insert(wdUInt32 uiIndex, const char* value);   // [ property ]
  void MiddleClips_Remove(wdUInt32 uiIndex);                      // [ property ]

  void SetEndClip(const char* szFile); // [ property ]
  const char* GetEndClip() const;      // [ property ]

  wdAnimationClipResourceHandle m_hStartClip;
  wdHybridArray<wdAnimationClipResourceHandle, 2> m_hMiddleClips;
  wdAnimationClipResourceHandle m_hEndClip;

private:
  wdAnimGraphTriggerInputPin m_ActivePin;           // [ property ]
  wdAnimGraphBoneWeightsInputPin m_WeightsPin;      // [ property ]
  wdAnimGraphNumberInputPin m_SpeedPin;             // [ property ]
  wdAnimGraphNumberInputPin m_ClipIndexPin;         // [ property ]
  wdAnimGraphLocalPoseOutputPin m_LocalPosePin;     // [ property ]
  wdAnimGraphTriggerOutputPin m_OnNextClipPin;      // [ property ]
  wdAnimGraphNumberOutputPin m_PlayingClipIndexPin; // [ property ]
  wdAnimGraphTriggerOutputPin m_OnFadeOutPin;       // [ property ]

  wdAnimState m_State;

  enum class Phase : wdUInt8
  {
    Off,
    Start,
    Middle,
    End,
  };

  Phase m_Phase = Phase::Off;
  wdUInt8 m_uiClipToPlay = 0xFF;
  wdUInt8 m_uiNextClipToPlay = 0xFF;
};
