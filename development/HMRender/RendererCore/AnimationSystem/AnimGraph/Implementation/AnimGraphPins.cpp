#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphPins.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdAnimGraphPin, 1, wdRTTIDefaultAllocator<wdAnimGraphPin>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("PinIdx", m_iPinIndex)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("NumConnections", m_uiNumConnections)->AddAttributes(new wdHiddenAttribute()),
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdAnimGraphInputPin, 1, wdRTTIDefaultAllocator<wdAnimGraphInputPin>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdAnimGraphOutputPin, 1, wdRTTIDefaultAllocator<wdAnimGraphOutputPin>)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdResult wdAnimGraphPin::Serialize(wdStreamWriter& inout_stream) const
{
  inout_stream << m_iPinIndex;
  inout_stream << m_uiNumConnections;
  return WD_SUCCESS;
}

wdResult wdAnimGraphPin::Deserialize(wdStreamReader& inout_stream)
{
  inout_stream >> m_iPinIndex;
  inout_stream >> m_uiNumConnections;
  return WD_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdAnimGraphTriggerInputPin, 1, wdRTTIDefaultAllocator<wdAnimGraphTriggerInputPin>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdAnimGraphTriggerOutputPin, 1, wdRTTIDefaultAllocator<wdAnimGraphTriggerOutputPin>)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void wdAnimGraphTriggerOutputPin::SetTriggered(wdAnimGraph& ref_graph, bool bTriggered)
{
  if (m_iPinIndex < 0)
    return;

  if (!bTriggered)
    return;

  const auto& map = ref_graph.m_OutputPinToInputPinMapping[wdAnimGraphPin::Trigger][m_iPinIndex];


  const wdInt8 offset = +1; // bTriggered ? +1 : -1;

  // trigger or reset all input pins that are connected to this output pin
  for (wdUInt16 idx : map)
  {
    ref_graph.m_TriggerInputPinStates[idx] += offset;
  }
}

bool wdAnimGraphTriggerInputPin::IsTriggered(wdAnimGraph& ref_graph) const
{
  if (m_iPinIndex < 0)
    return false;

  return ref_graph.m_TriggerInputPinStates[m_iPinIndex] > 0;
}

bool wdAnimGraphTriggerInputPin::AreAllTriggered(wdAnimGraph& ref_graph) const
{
  return ref_graph.m_TriggerInputPinStates[m_iPinIndex] == m_uiNumConnections;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdAnimGraphNumberInputPin, 1, wdRTTIDefaultAllocator<wdAnimGraphNumberInputPin>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdAnimGraphNumberOutputPin, 1, wdRTTIDefaultAllocator<wdAnimGraphNumberOutputPin>)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

double wdAnimGraphNumberInputPin::GetNumber(wdAnimGraph& ref_graph, double fFallback /*= 0.0*/) const
{
  if (m_iPinIndex < 0)
    return fFallback;

  return ref_graph.m_NumberInputPinStates[m_iPinIndex];
}

void wdAnimGraphNumberOutputPin::SetNumber(wdAnimGraph& ref_graph, double value)
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_OutputPinToInputPinMapping[wdAnimGraphPin::Number][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (wdUInt16 idx : map)
  {
    ref_graph.m_NumberInputPinStates[idx] = value;
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdAnimGraphBoneWeightsInputPin, 1, wdRTTIDefaultAllocator<wdAnimGraphBoneWeightsInputPin>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdAnimGraphBoneWeightsOutputPin, 1, wdRTTIDefaultAllocator<wdAnimGraphBoneWeightsOutputPin>)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdAnimGraphPinDataBoneWeights* wdAnimGraphBoneWeightsInputPin::GetWeights(wdAnimGraph& ref_graph) const
{
  if (m_iPinIndex < 0 || ref_graph.m_BoneWeightInputPinStates[m_iPinIndex] == 0xFFFF)
    return nullptr;

  return &ref_graph.m_PinDataBoneWeights[ref_graph.m_BoneWeightInputPinStates[m_iPinIndex]];
}

void wdAnimGraphBoneWeightsOutputPin::SetWeights(wdAnimGraph& ref_graph, wdAnimGraphPinDataBoneWeights* pWeights)
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_OutputPinToInputPinMapping[wdAnimGraphPin::BoneWeights][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (wdUInt16 idx : map)
  {
    ref_graph.m_BoneWeightInputPinStates[idx] = pWeights->m_uiOwnIndex;
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdAnimGraphLocalPoseInputPin, 1, wdRTTIDefaultAllocator<wdAnimGraphLocalPoseInputPin>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdAnimGraphLocalPoseMultiInputPin, 1, wdRTTIDefaultAllocator<wdAnimGraphLocalPoseMultiInputPin>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdAnimGraphLocalPoseOutputPin, 1, wdRTTIDefaultAllocator<wdAnimGraphLocalPoseOutputPin>)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdAnimGraphPinDataLocalTransforms* wdAnimGraphLocalPoseInputPin::GetPose(wdAnimGraph& ref_graph) const
{
  if (m_iPinIndex < 0)
    return nullptr;

  if (ref_graph.m_LocalPoseInputPinStates[m_iPinIndex].IsEmpty())
    return nullptr;

  return &ref_graph.m_PinDataLocalTransforms[ref_graph.m_LocalPoseInputPinStates[m_iPinIndex][0]];
}

void wdAnimGraphLocalPoseMultiInputPin::GetPoses(wdAnimGraph& ref_graph, wdDynamicArray<wdAnimGraphPinDataLocalTransforms*>& out_poses) const
{
  out_poses.Clear();

  if (m_iPinIndex < 0)
    return;

  out_poses.SetCountUninitialized(ref_graph.m_LocalPoseInputPinStates[m_iPinIndex].GetCount());
  for (wdUInt32 i = 0; i < ref_graph.m_LocalPoseInputPinStates[m_iPinIndex].GetCount(); ++i)
  {
    out_poses[i] = &ref_graph.m_PinDataLocalTransforms[ref_graph.m_LocalPoseInputPinStates[m_iPinIndex][i]];
  }
}

void wdAnimGraphLocalPoseOutputPin::SetPose(wdAnimGraph& ref_graph, wdAnimGraphPinDataLocalTransforms* pPose)
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_OutputPinToInputPinMapping[wdAnimGraphPin::LocalPose][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (wdUInt16 idx : map)
  {
    ref_graph.m_LocalPoseInputPinStates[idx].PushBack(pPose->m_uiOwnIndex);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdAnimGraphModelPoseInputPin, 1, wdRTTIDefaultAllocator<wdAnimGraphModelPoseInputPin>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdAnimGraphModelPoseOutputPin, 1, wdRTTIDefaultAllocator<wdAnimGraphModelPoseOutputPin>)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdAnimGraphPinDataModelTransforms* wdAnimGraphModelPoseInputPin::GetPose(wdAnimGraph& ref_graph) const
{
  if (m_iPinIndex < 0 || ref_graph.m_ModelPoseInputPinStates[m_iPinIndex] == 0xFFFF)
    return nullptr;

  return &ref_graph.m_PinDataModelTransforms[ref_graph.m_ModelPoseInputPinStates[m_iPinIndex]];
}

void wdAnimGraphModelPoseOutputPin::SetPose(wdAnimGraph& ref_graph, wdAnimGraphPinDataModelTransforms* pPose)
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_OutputPinToInputPinMapping[wdAnimGraphPin::ModelPose][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (wdUInt16 idx : map)
  {
    ref_graph.m_ModelPoseInputPinStates[idx] = pPose->m_uiOwnIndex;
  }
}


WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Implementation_AnimGraphPins);
