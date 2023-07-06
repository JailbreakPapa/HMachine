#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class WD_RENDERERCORE_DLL wdModelPoseOutputAnimNode : public wdAnimGraphNode
{
  WD_ADD_DYNAMIC_REFLECTION(wdModelPoseOutputAnimNode, wdAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // wdAnimGraphNode

protected:
  virtual wdResult SerializeNode(wdStreamWriter& stream) const override;
  virtual wdResult DeserializeNode(wdStreamReader& stream) override;

  virtual void Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // wdModelPoseOutputAnimNode

public:
  wdModelPoseOutputAnimNode();
  ~wdModelPoseOutputAnimNode();

private:
  wdAnimGraphModelPoseInputPin m_ModelPosePin; // [ property ]
  wdAnimGraphNumberInputPin m_RotateZPin;      // [ property ]
};
