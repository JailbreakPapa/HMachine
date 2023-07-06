#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class WD_RENDERERCORE_DLL wdControllerInputAnimNode : public wdAnimGraphNode
{
  WD_ADD_DYNAMIC_REFLECTION(wdControllerInputAnimNode, wdAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // wdAnimGraphNode

protected:
  virtual wdResult SerializeNode(wdStreamWriter& stream) const override;
  virtual wdResult DeserializeNode(wdStreamReader& stream) override;

  virtual void Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget) override;

private:
  wdAnimGraphTriggerOutputPin m_ButtonA; // [ property ]
  wdAnimGraphTriggerOutputPin m_ButtonB; // [ property ]
  wdAnimGraphTriggerOutputPin m_ButtonX; // [ property ]
  wdAnimGraphTriggerOutputPin m_ButtonY; // [ property ]

  wdAnimGraphNumberOutputPin m_LeftStickX;  // [ property ]
  wdAnimGraphNumberOutputPin m_LeftStickY;  // [ property ]
  wdAnimGraphNumberOutputPin m_RightStickX; // [ property ]
  wdAnimGraphNumberOutputPin m_RightStickY; // [ property ]

  wdAnimGraphNumberOutputPin m_LeftTrigger;  // [ property ]
  wdAnimGraphNumberOutputPin m_RightTrigger; // [ property ]

  wdAnimGraphTriggerOutputPin m_LeftShoulder;  // [ property ]
  wdAnimGraphTriggerOutputPin m_RightShoulder; // [ property ]

  wdAnimGraphTriggerOutputPin m_PadLeft;  // [ property ]
  wdAnimGraphTriggerOutputPin m_PadRight; // [ property ]
  wdAnimGraphTriggerOutputPin m_PadUp;    // [ property ]
  wdAnimGraphTriggerOutputPin m_PadDown;  // [ property ]
};
