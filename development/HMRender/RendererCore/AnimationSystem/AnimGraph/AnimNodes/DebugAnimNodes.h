#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class WD_RENDERERCORE_DLL wdLogAnimNode : public wdAnimGraphNode
{
  WD_ADD_DYNAMIC_REFLECTION(wdLogAnimNode, wdAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // wdAnimGraphNode

protected:
  virtual wdResult SerializeNode(wdStreamWriter& stream) const override;
  virtual wdResult DeserializeNode(wdStreamReader& stream) override;

  virtual void Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // wdLogAnimNode

public:
  wdString m_sText;

private:
  wdAnimGraphTriggerInputPin m_ActivePin; // [ property ]
  wdAnimGraphTriggerInputPin m_Input0;    // [ property ]
  wdAnimGraphTriggerInputPin m_Input1;    // [ property ]
  wdAnimGraphNumberInputPin m_Input2;     // [ property ]
  wdAnimGraphNumberInputPin m_Input3;     // [ property ]
};
