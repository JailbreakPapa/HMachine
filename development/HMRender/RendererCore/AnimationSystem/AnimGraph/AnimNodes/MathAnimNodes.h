#pragma once

#include <Foundation/CodeUtils/MathExpression.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class WD_RENDERERCORE_DLL wdMathExpressionAnimNode : public wdAnimGraphNode
{
  WD_ADD_DYNAMIC_REFLECTION(wdMathExpressionAnimNode, wdAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // wdAnimGraphNode

protected:
  virtual wdResult SerializeNode(wdStreamWriter& stream) const override;
  virtual wdResult DeserializeNode(wdStreamReader& stream) override;

  virtual void Initialize(wdAnimGraph& graph, const wdSkeletonResource* pSkeleton) override;
  virtual void Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // wdLogicAndAnimNode

public:
  wdMathExpressionAnimNode();
  ~wdMathExpressionAnimNode();

  void SetExpression(const char* szSz);
  const char* GetExpression() const;

private:
  wdAnimGraphNumberInputPin m_ValueAPin;  // [ property ]
  wdAnimGraphNumberInputPin m_ValueBPin;  // [ property ]
  wdAnimGraphNumberInputPin m_ValueCPin;  // [ property ]
  wdAnimGraphNumberInputPin m_ValueDPin;  // [ property ]
  wdAnimGraphNumberOutputPin m_ResultPin; // [ property ]

  wdMathExpression m_mExpression;
};
