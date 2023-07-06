#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

struct WD_RENDERERCORE_DLL wdAnimClip1D
{
  wdAnimationClipResourceHandle m_hAnimation;
  float m_fPosition;

  void SetAnimationFile(const char* szSz);
  const char* GetAnimationFile() const;
};

WD_DECLARE_REFLECTABLE_TYPE(WD_RENDERERCORE_DLL, wdAnimClip1D);

class WD_RENDERERCORE_DLL wdMixClips1DAnimNode : public wdAnimGraphNode
{
  WD_ADD_DYNAMIC_REFLECTION(wdMixClips1DAnimNode, wdAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // wdAnimGraphNode

protected:
  virtual wdResult SerializeNode(wdStreamWriter& stream) const override;
  virtual wdResult DeserializeNode(wdStreamReader& stream) override;

  virtual void Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // wdMixClips1DAnimNode

public:
  wdHybridArray<wdAnimClip1D, 3> m_Clips; // [ property ]

private:
  wdAnimGraphTriggerInputPin m_ActivePin;       // [ property ]
  wdAnimGraphBoneWeightsInputPin m_WeightsPin;  // [ property ]
  wdAnimGraphNumberInputPin m_SpeedPin;         // [ property ]
  wdAnimGraphNumberInputPin m_LerpPin;          // [ property ]
  wdAnimGraphLocalPoseOutputPin m_LocalPosePin; // [ property ]
  wdAnimGraphTriggerOutputPin m_OnFadeOutPin;  // [ property ]

  wdAnimState m_State; // [ property ]
};
