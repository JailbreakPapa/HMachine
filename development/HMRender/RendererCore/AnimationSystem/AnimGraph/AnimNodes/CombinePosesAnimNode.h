#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class WD_RENDERERCORE_DLL wdCombinePosesAnimNode : public wdAnimGraphNode
{
  WD_ADD_DYNAMIC_REFLECTION(wdCombinePosesAnimNode, wdAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // wdAnimGraphNode

protected:
  virtual wdResult SerializeNode(wdStreamWriter& stream) const override;
  virtual wdResult DeserializeNode(wdStreamReader& stream) override;

  virtual void Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // wdLocalToModelPoseAnimNode

public:
  wdCombinePosesAnimNode();
  ~wdCombinePosesAnimNode();

  wdUInt8 m_uiMaxPoses = 8; // [ property ]

private:
  wdAnimGraphLocalPoseMultiInputPin m_LocalPosesPin; // [ property ]
  wdAnimGraphLocalPoseOutputPin m_LocalPosePin;      // [ property ]

  wdDynamicArray<ozz::math::SimdFloat4, wdAlignedAllocatorWrapper> m_BlendMask;
};
