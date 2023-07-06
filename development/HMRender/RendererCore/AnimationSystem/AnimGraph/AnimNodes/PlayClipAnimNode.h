#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

/// \brief Plays a single animation clip, either once or looped
class WD_RENDERERCORE_DLL wdPlayClipAnimNode : public wdAnimGraphNode
{
  WD_ADD_DYNAMIC_REFLECTION(wdPlayClipAnimNode, wdAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // wdAnimGraphNode

protected:
  virtual wdResult SerializeNode(wdStreamWriter& stream) const override;
  virtual wdResult DeserializeNode(wdStreamReader& stream) override;

  virtual void Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // wdPlayClipAnimNode

public:
  wdUInt32 Clips_GetCount() const;                          // [ property ]
  const char* Clips_GetValue(wdUInt32 uiIndex) const;       // [ property ]
  void Clips_SetValue(wdUInt32 uiIndex, const char* value); // [ property ]
  void Clips_Insert(wdUInt32 uiIndex, const char* value);   // [ property ]
  void Clips_Remove(wdUInt32 uiIndex);                      // [ property ]

private:
  wdHybridArray<wdAnimationClipResourceHandle, 1> m_Clips; // [ property ]

  wdAnimGraphTriggerInputPin m_ActivePin;       // [ property ]
  wdAnimGraphBoneWeightsInputPin m_WeightsPin;  // [ property ]
  wdAnimGraphNumberInputPin m_SpeedPin;         // [ property ]
  wdAnimGraphNumberInputPin m_ClipIndexPin;     // [ property ]
  wdAnimGraphLocalPoseOutputPin m_LocalPosePin; // [ property ]
  wdAnimGraphTriggerOutputPin m_OnFadeOutPin;  // [ property ]

  wdAnimState m_State; // [ property ]
  wdUInt8 m_uiClipToPlay = 0xFF;
  wdUInt8 m_uiNextClipToPlay = 0xFF;
  wdTime m_NextClipDuration;
};
