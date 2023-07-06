#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class WD_RENDERERCORE_DLL wdLocalToModelPoseAnimNode : public wdAnimGraphNode
{
  WD_ADD_DYNAMIC_REFLECTION(wdLocalToModelPoseAnimNode, wdAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // wdAnimGraphNode

protected:
  virtual wdResult SerializeNode(wdStreamWriter& stream) const override;
  virtual wdResult DeserializeNode(wdStreamReader& stream) override;

  virtual void Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // wdLocalToModelPoseAnimNode

public:
  wdLocalToModelPoseAnimNode();
  ~wdLocalToModelPoseAnimNode();

private:
  wdAnimGraphLocalPoseInputPin m_LocalPosePin;  // [ property ]
  wdAnimGraphModelPoseOutputPin m_ModelPosePin; // [ property ]
};
