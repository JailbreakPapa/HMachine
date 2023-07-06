#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>

struct WD_RENDERERCORE_DLL wdAnimClip2D
{
  wdAnimationClipResourceHandle m_hAnimation;
  wdVec2 m_vPosition;

  void SetAnimationFile(const char* szSz);
  const char* GetAnimationFile() const;
};

WD_DECLARE_REFLECTABLE_TYPE(WD_RENDERERCORE_DLL, wdAnimClip2D);

class WD_RENDERERCORE_DLL wdMixClips2DAnimNode : public wdAnimGraphNode
{
  WD_ADD_DYNAMIC_REFLECTION(wdMixClips2DAnimNode, wdAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // wdAnimGraphNode

protected:
  virtual wdResult SerializeNode(wdStreamWriter& stream) const override;
  virtual wdResult DeserializeNode(wdStreamReader& stream) override;

  virtual void Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // wdMixClips2DAnimNode

public:
  void SetCenterClipFile(const char* szSz);
  const char* GetCenterClipFile() const;

  wdAnimationClipResourceHandle m_hCenterClip; // [ property ]
  wdHybridArray<wdAnimClip2D, 8> m_Clips;      // [ property ]
  wdTime m_InputResponse;                      // [ property ]

private:
  wdAnimGraphTriggerInputPin m_ActivePin;       // [ property ]
  wdAnimGraphBoneWeightsInputPin m_WeightsPin;  // [ property ]
  wdAnimGraphNumberInputPin m_SpeedPin;         // [ property ]
  wdAnimGraphNumberInputPin m_XCoordPin;        // [ property ]
  wdAnimGraphNumberInputPin m_YCoordPin;        // [ property ]
  wdAnimGraphLocalPoseOutputPin m_LocalPosePin; // [ property ]
  wdAnimGraphTriggerOutputPin m_OnFadeOutPin;   // [ property ]

  struct ClipToPlay
  {
    WD_DECLARE_POD_TYPE();

    wdUInt32 m_uiIndex;
    float m_fWeight = 1.0f;
  };

  void UpdateCenterClipPlaybackTime(wdAnimGraph& graph, wdTime tDiff, wdAnimPoseEventTrackSampleMode& out_eventSamplingCenter);
  void PlayClips(wdAnimGraph& graph, wdTime tDiff, wdArrayPtr<ClipToPlay> clips, wdUInt32 uiMaxWeightClip);
  void ComputeClipsAndWeights(const wdVec2& p, wdDynamicArray<ClipToPlay>& out_Clips, wdUInt32& out_uiMaxWeightClip);

  wdTime m_CenterPlaybackTime;
  wdAnimState m_State; // [ property ]

  float m_fLastValueX = 0.0f;
  float m_fLastValueY = 0.0f;
};
