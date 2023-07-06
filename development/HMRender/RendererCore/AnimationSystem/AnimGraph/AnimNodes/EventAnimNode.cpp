#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/EventAnimNode.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdEventAnimNode, 1, wdRTTIDefaultAllocator<wdEventAnimNode>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("EventName", GetEventName, SetEventName),

    WD_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new wdHiddenAttribute()),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Events"),
    new wdColorAttribute(wdColorScheme::DarkUI(wdColorScheme::Orange)),
    new wdTitleAttribute("Event: '{EventName}'"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdResult wdEventAnimNode::SerializeNode(wdStreamWriter& stream) const
{
  stream.WriteVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sEventName;

  WD_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));

  return WD_SUCCESS;
}

wdResult wdEventAnimNode::DeserializeNode(wdStreamReader& stream)
{
  stream.ReadVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sEventName;

  WD_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));

  return WD_SUCCESS;
}

void wdEventAnimNode::Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget)
{
  if (!m_ActivePin.IsConnected() || m_sEventName.IsEmpty())
    return;

  if (m_ActivePin.IsTriggered(graph))
  {
    wdMsgGenericEvent msg;
    msg.m_sMessage = m_sEventName;
    msg.m_Value = wdVariant();

    pTarget->SendEventMessage(msg, nullptr);
  }
}


WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_EventAnimNode);
