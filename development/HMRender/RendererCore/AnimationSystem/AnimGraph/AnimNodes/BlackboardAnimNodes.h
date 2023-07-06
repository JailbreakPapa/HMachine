#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class WD_RENDERERCORE_DLL wdSetBlackboardValueAnimNode : public wdAnimGraphNode
{
  WD_ADD_DYNAMIC_REFLECTION(wdSetBlackboardValueAnimNode, wdAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // wdAnimGraphNode

protected:
  virtual wdResult SerializeNode(wdStreamWriter& stream) const override;
  virtual wdResult DeserializeNode(wdStreamReader& stream) override;

  virtual void Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // wdSetBlackboardValueAnimNode

public:
  void SetBlackboardEntry(const char* szFile); // [ property ]
  const char* GetBlackboardEntry() const;      // [ property ]

  float m_fOnActivatedValue = 1.0f;   // [ property ]
  float m_fOnHoldValue = 1.0f;        // [ property ]
  float m_fOnDeactivatedValue = 0.0f; // [ property ]
  bool m_bSetOnActivation = true;     // [ property ]
  bool m_bSetOnHold = false;          // [ property ]
  bool m_bSetOnDeactivation = false;  // [ property ]

private:
  wdAnimGraphTriggerInputPin m_ActivePin; // [ property ]
  wdHashedString m_sBlackboardEntry;
  bool m_bLastActiveState = false;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class WD_RENDERERCORE_DLL wdCheckBlackboardValueAnimNode : public wdAnimGraphNode
{
  WD_ADD_DYNAMIC_REFLECTION(wdCheckBlackboardValueAnimNode, wdAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // wdAnimGraphNode

protected:
  virtual wdResult SerializeNode(wdStreamWriter& stream) const override;
  virtual wdResult DeserializeNode(wdStreamReader& stream) override;

  virtual void Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // wdCheckBlackboardValueAnimNode

public:
  void SetBlackboardEntry(const char* szFile); // [ property ]
  const char* GetBlackboardEntry() const;      // [ property ]

  float m_fReferenceValue = 1.0f;            // [ property ]
  wdEnum<wdComparisonOperator> m_Comparison; // [ property ]

private:
  wdAnimGraphTriggerOutputPin m_ActivePin; // [ property ]

  wdHashedString m_sBlackboardEntry;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class WD_RENDERERCORE_DLL wdGetBlackboardNumberAnimNode : public wdAnimGraphNode
{
  WD_ADD_DYNAMIC_REFLECTION(wdGetBlackboardNumberAnimNode, wdAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // wdAnimGraphNode

protected:
  virtual wdResult SerializeNode(wdStreamWriter& stream) const override;
  virtual wdResult DeserializeNode(wdStreamReader& stream) override;

  virtual void Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // wdCheckBlackboardValueAnimNode

public:
  void SetBlackboardEntry(const char* szFile); // [ property ]
  const char* GetBlackboardEntry() const;      // [ property ]

private:
  wdAnimGraphNumberOutputPin m_NumberPin; // [ property ]

  wdHashedString m_sBlackboardEntry;
};
