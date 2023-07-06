#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/DebugAnimNodes.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdLogAnimNode, 1, wdRTTIDefaultAllocator<wdLogAnimNode>)
  {
    WD_BEGIN_PROPERTIES
    {
      WD_MEMBER_PROPERTY("Text", m_sText),

      WD_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new wdHiddenAttribute()),
      WD_MEMBER_PROPERTY("Input0", m_Input0)->AddAttributes(new wdHiddenAttribute()),
      WD_MEMBER_PROPERTY("Input1", m_Input1)->AddAttributes(new wdHiddenAttribute()),
      WD_MEMBER_PROPERTY("Input2", m_Input2)->AddAttributes(new wdHiddenAttribute()),
      WD_MEMBER_PROPERTY("Input3", m_Input3)->AddAttributes(new wdHiddenAttribute()),
    }
    WD_END_PROPERTIES;
    WD_BEGIN_ATTRIBUTES
    {
      new wdCategoryAttribute("Debug"),
      new wdColorAttribute(wdColorScheme::DarkUI(wdColorScheme::Pink)),
      new wdTitleAttribute("Log: '{Text}'"),
    }
    WD_END_ATTRIBUTES;
  }
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdResult wdLogAnimNode::SerializeNode(wdStreamWriter& stream) const
{
  stream.WriteVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sText;

  WD_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_Input0.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_Input1.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_Input2.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_Input3.Serialize(stream));

  return WD_SUCCESS;
}

wdResult wdLogAnimNode::DeserializeNode(wdStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sText;

  WD_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_Input0.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_Input1.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_Input2.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_Input3.Deserialize(stream));

  return WD_SUCCESS;
}

void wdLogAnimNode::Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget)
{
  if (!m_ActivePin.IsTriggered(graph))
    return;

  wdLog::Dev(m_sText, m_Input0.IsTriggered(graph), m_Input1.IsTriggered(graph), m_Input2.GetNumber(graph), m_Input3.GetNumber(graph));
}


WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_DebugAnimNodes);
