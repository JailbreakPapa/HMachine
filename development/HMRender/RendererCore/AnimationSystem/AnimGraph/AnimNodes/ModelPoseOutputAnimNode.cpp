#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/ModelPoseOutputAnimNode.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdModelPoseOutputAnimNode, 1, wdRTTIDefaultAllocator<wdModelPoseOutputAnimNode>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("ModelPose", m_ModelPosePin)->AddAttributes(new wdHiddenAttribute),
    WD_MEMBER_PROPERTY("RotateZ", m_RotateZPin)->AddAttributes(new wdHiddenAttribute),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Output"),
    new wdColorAttribute(wdColorScheme::DarkUI(wdColorScheme::Grape)),
    new wdTitleAttribute("Output"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdModelPoseOutputAnimNode::wdModelPoseOutputAnimNode() = default;
wdModelPoseOutputAnimNode::~wdModelPoseOutputAnimNode() = default;

wdResult wdModelPoseOutputAnimNode::SerializeNode(wdStreamWriter& stream) const
{
  stream.WriteVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  WD_SUCCEED_OR_RETURN(m_ModelPosePin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_RotateZPin.Serialize(stream));

  return WD_SUCCESS;
}

wdResult wdModelPoseOutputAnimNode::DeserializeNode(wdStreamReader& stream)
{
  stream.ReadVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  WD_SUCCEED_OR_RETURN(m_ModelPosePin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_RotateZPin.Deserialize(stream));

  return WD_SUCCESS;
}

void wdModelPoseOutputAnimNode::Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget)
{
  wdVec3 rootMotion = wdVec3::ZeroVector();
  wdAngle rootRotationX;
  wdAngle rootRotationY;
  wdAngle rootRotationZ;

  if (m_ModelPosePin.IsConnected())
  {
    if (auto pCurrentModelTransforms = m_ModelPosePin.GetPose(graph))
    {
      if (pCurrentModelTransforms->m_CommandID != wdInvalidIndex)
      {
        auto& cmd = graph.GetPoseGenerator().AllocCommandModelPoseToOutput();
        cmd.m_Inputs.PushBack(m_ModelPosePin.GetPose(graph)->m_CommandID);
      }

      if (pCurrentModelTransforms->m_bUseRootMotion)
      {
        rootMotion = pCurrentModelTransforms->m_vRootMotion;
        rootRotationX = pCurrentModelTransforms->m_RootRotationX;
        rootRotationY = pCurrentModelTransforms->m_RootRotationY;
        rootRotationZ = pCurrentModelTransforms->m_RootRotationZ;
      }

      graph.SetOutputModelTransform(pCurrentModelTransforms);
    }
  }

  if (m_RotateZPin.IsConnected())
  {
    const float rotZ = static_cast<float>(m_RotateZPin.GetNumber(graph));
    rootRotationZ += wdAngle::Degree(rotZ);
  }

  graph.SetRootMotion(rootMotion, rootRotationX, rootRotationY, rootRotationZ);
}


WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_ModelPoseOutputAnimNode);
