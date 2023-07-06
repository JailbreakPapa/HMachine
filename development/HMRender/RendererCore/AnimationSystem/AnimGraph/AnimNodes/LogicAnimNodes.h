#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class WD_RENDERERCORE_DLL wdLogicAndAnimNode : public wdAnimGraphNode
{
  WD_ADD_DYNAMIC_REFLECTION(wdLogicAndAnimNode, wdAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // wdAnimGraphNode

protected:
  virtual wdResult SerializeNode(wdStreamWriter& stream) const override;
  virtual wdResult DeserializeNode(wdStreamReader& stream) override;

  virtual void Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // wdLogicAndAnimNode

public:
  wdLogicAndAnimNode();
  ~wdLogicAndAnimNode();

  bool m_bNegateResult = false; // [ property ]

private:
  wdAnimGraphTriggerInputPin m_ActivePin;  // [ property ]
  wdAnimGraphTriggerOutputPin m_OutputPin; // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class WD_RENDERERCORE_DLL wdLogicOrAnimNode : public wdAnimGraphNode
{
  WD_ADD_DYNAMIC_REFLECTION(wdLogicOrAnimNode, wdAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // wdAnimGraphNode

protected:
  virtual wdResult SerializeNode(wdStreamWriter& stream) const override;
  virtual wdResult DeserializeNode(wdStreamReader& stream) override;

  virtual void Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // wdLogicOrAnimNode

public:
  wdLogicOrAnimNode();
  ~wdLogicOrAnimNode();

  bool m_bNegateResult = false; // [ property ]

private:
  wdAnimGraphTriggerInputPin m_ActivePin;  // [ property ]
  wdAnimGraphTriggerOutputPin m_OutputPin; // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class WD_RENDERERCORE_DLL wdLogicNotAnimNode : public wdAnimGraphNode
{
  WD_ADD_DYNAMIC_REFLECTION(wdLogicNotAnimNode, wdAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // wdAnimGraphNode

protected:
  virtual wdResult SerializeNode(wdStreamWriter& stream) const override;
  virtual wdResult DeserializeNode(wdStreamReader& stream) override;

  virtual void Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // wdLogicNotAnimNode

public:
  wdLogicNotAnimNode();
  ~wdLogicNotAnimNode();

private:
  wdAnimGraphTriggerInputPin m_ActivePin;  // [ property ]
  wdAnimGraphTriggerOutputPin m_OutputPin; // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class WD_RENDERERCORE_DLL wdCompareNumberAnimNode : public wdAnimGraphNode
{
  WD_ADD_DYNAMIC_REFLECTION(wdCompareNumberAnimNode, wdAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // wdAnimGraphNode

protected:
  virtual wdResult SerializeNode(wdStreamWriter& stream) const override;
  virtual wdResult DeserializeNode(wdStreamReader& stream) override;

  virtual void Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // wdCompareNumberAnimNode

public:
  float m_fReferenceValue = 1.0f;            // [ property ]
  wdEnum<wdComparisonOperator> m_Comparison; // [ property ]

private:
  wdAnimGraphTriggerOutputPin m_ActivePin; // [ property ]
  wdAnimGraphNumberInputPin m_NumberPin;   // [ property ]
};
