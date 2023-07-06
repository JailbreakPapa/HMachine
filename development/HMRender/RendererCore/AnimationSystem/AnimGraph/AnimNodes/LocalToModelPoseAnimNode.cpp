#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/LocalToModelPoseAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdLocalToModelPoseAnimNode, 1, wdRTTIDefaultAllocator<wdLocalToModelPoseAnimNode>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("LocalPose", m_LocalPosePin)->AddAttributes(new wdHiddenAttribute),
    WD_MEMBER_PROPERTY("ModelPose", m_ModelPosePin)->AddAttributes(new wdHiddenAttribute),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Pose Processing"),
    new wdColorAttribute(wdColorScheme::DarkUI(wdColorScheme::Blue)),
    new wdTitleAttribute("Local To Model Space"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdLocalToModelPoseAnimNode::wdLocalToModelPoseAnimNode() = default;
wdLocalToModelPoseAnimNode::~wdLocalToModelPoseAnimNode() = default;

wdResult wdLocalToModelPoseAnimNode::SerializeNode(wdStreamWriter& stream) const
{
  stream.WriteVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  WD_SUCCEED_OR_RETURN(m_LocalPosePin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_ModelPosePin.Serialize(stream));

  return WD_SUCCESS;
}

wdResult wdLocalToModelPoseAnimNode::DeserializeNode(wdStreamReader& stream)
{
  stream.ReadVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  WD_SUCCEED_OR_RETURN(m_LocalPosePin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_ModelPosePin.Deserialize(stream));

  return WD_SUCCESS;
}

void wdLocalToModelPoseAnimNode::Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget)
{
  if (!m_LocalPosePin.IsConnected() || !m_ModelPosePin.IsConnected())
    return;

  auto pLocalPose = m_LocalPosePin.GetPose(graph);
  if (pLocalPose == nullptr)
    return;

  wdAnimGraphPinDataModelTransforms* pModelTransform = graph.AddPinDataModelTransforms();

  if (pLocalPose->m_bUseRootMotion)
  {
    pModelTransform->m_bUseRootMotion = true;
    pModelTransform->m_vRootMotion = pLocalPose->m_vRootMotion;
  }

  auto& cmd = graph.GetPoseGenerator().AllocCommandLocalToModelPose();
  cmd.m_Inputs.PushBack(m_LocalPosePin.GetPose(graph)->m_CommandID);

  pModelTransform->m_CommandID = cmd.GetCommandID();

  m_ModelPosePin.SetPose(graph, pModelTransform);
}


WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_LocalToModelPoseAnimNode);
