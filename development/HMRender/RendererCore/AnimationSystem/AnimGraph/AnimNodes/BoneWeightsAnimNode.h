#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class wdSkeletonResource;
class wdStreamWriter;
class wdStreamReader;

class WD_RENDERERCORE_DLL wdBoneWeightsAnimNode : public wdAnimGraphNode
{
  WD_ADD_DYNAMIC_REFLECTION(wdBoneWeightsAnimNode, wdAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // wdAnimGraphNode

protected:
  virtual wdResult SerializeNode(wdStreamWriter& stream) const override;
  virtual wdResult DeserializeNode(wdStreamReader& stream) override;

  virtual void Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // wdBoneWeightsAnimNode

public:
  wdBoneWeightsAnimNode();
  ~wdBoneWeightsAnimNode();

  float m_fWeight = 1.0f; // [ property ]

  wdUInt32 RootBones_GetCount() const;                          // [ property ]
  const char* RootBones_GetValue(wdUInt32 uiIndex) const;       // [ property ]
  void RootBones_SetValue(wdUInt32 uiIndex, const char* value); // [ property ]
  void RootBones_Insert(wdUInt32 uiIndex, const char* value);   // [ property ]
  void RootBones_Remove(wdUInt32 uiIndex);                      // [ property ]

private:
  wdAnimGraphBoneWeightsOutputPin m_WeightsPin;        // [ property ]
  wdAnimGraphBoneWeightsOutputPin m_InverseWeightsPin; // [ property ]

  wdHybridArray<wdHashedString, 2> m_RootBones;

  wdSharedPtr<wdAnimGraphSharedBoneWeights> m_pSharedBoneWeights;
  wdSharedPtr<wdAnimGraphSharedBoneWeights> m_pSharedInverseBoneWeights;
};
