#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/LogicAnimNodes.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdLogicAndAnimNode, 1, wdRTTIDefaultAllocator<wdLogicAndAnimNode>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("NegateResult", m_bNegateResult),
    WD_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new wdHiddenAttribute),
    WD_MEMBER_PROPERTY("Output", m_OutputPin)->AddAttributes(new wdHiddenAttribute),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Logic"),
    new wdTitleAttribute("AND"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdLogicAndAnimNode::wdLogicAndAnimNode() = default;
wdLogicAndAnimNode::~wdLogicAndAnimNode() = default;

wdResult wdLogicAndAnimNode::SerializeNode(wdStreamWriter& stream) const
{
  stream.WriteVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_bNegateResult;
  WD_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_OutputPin.Serialize(stream));

  return WD_SUCCESS;
}

wdResult wdLogicAndAnimNode::DeserializeNode(wdStreamReader& stream)
{
  stream.ReadVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_bNegateResult;
  WD_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_OutputPin.Deserialize(stream));

  return WD_SUCCESS;
}

void wdLogicAndAnimNode::Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget)
{
  bool res = m_ActivePin.AreAllTriggered(graph);

  if (m_bNegateResult)
  {
    res = !res;
  }

  m_OutputPin.SetTriggered(graph, res);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdLogicOrAnimNode, 1, wdRTTIDefaultAllocator<wdLogicOrAnimNode>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("NegateResult", m_bNegateResult),
    WD_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new wdHiddenAttribute),
    WD_MEMBER_PROPERTY("Output", m_OutputPin)->AddAttributes(new wdHiddenAttribute),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Logic"),
    new wdTitleAttribute("OR"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdLogicOrAnimNode::wdLogicOrAnimNode() = default;
wdLogicOrAnimNode::~wdLogicOrAnimNode() = default;

wdResult wdLogicOrAnimNode::SerializeNode(wdStreamWriter& stream) const
{
  stream.WriteVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_bNegateResult;
  WD_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_OutputPin.Serialize(stream));

  return WD_SUCCESS;
}

wdResult wdLogicOrAnimNode::DeserializeNode(wdStreamReader& stream)
{
  stream.ReadVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_bNegateResult;
  WD_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_OutputPin.Deserialize(stream));

  return WD_SUCCESS;
}

void wdLogicOrAnimNode::Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget)
{
  bool res = m_ActivePin.IsTriggered(graph);

  if (m_bNegateResult)
  {
    res = !res;
  }

  m_OutputPin.SetTriggered(graph, res);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdLogicNotAnimNode, 1, wdRTTIDefaultAllocator<wdLogicNotAnimNode>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new wdHiddenAttribute),
    WD_MEMBER_PROPERTY("Output", m_OutputPin)->AddAttributes(new wdHiddenAttribute),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Logic"),
    new wdTitleAttribute("NOT"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdLogicNotAnimNode::wdLogicNotAnimNode() = default;
wdLogicNotAnimNode::~wdLogicNotAnimNode() = default;

wdResult wdLogicNotAnimNode::SerializeNode(wdStreamWriter& stream) const
{
  stream.WriteVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  WD_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_OutputPin.Serialize(stream));

  return WD_SUCCESS;
}

wdResult wdLogicNotAnimNode::DeserializeNode(wdStreamReader& stream)
{
  stream.ReadVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  WD_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_OutputPin.Deserialize(stream));

  return WD_SUCCESS;
}

void wdLogicNotAnimNode::Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget)
{
  bool res = !m_ActivePin.IsTriggered(graph);

  m_OutputPin.SetTriggered(graph, res);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdCompareNumberAnimNode, 1, wdRTTIDefaultAllocator<wdCompareNumberAnimNode>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("ReferenceValue", m_fReferenceValue)->AddAttributes(new wdDefaultValueAttribute(1.0f)),
    WD_ENUM_MEMBER_PROPERTY("Comparison", wdComparisonOperator, m_Comparison),

    WD_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("Number", m_NumberPin)->AddAttributes(new wdHiddenAttribute()),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Logic"),
    new wdTitleAttribute("Check: Number {Comparison} {ReferenceValue}"),
    new wdColorAttribute(wdColorScheme::DarkUI(wdColorScheme::Lime)),
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdResult wdCompareNumberAnimNode::SerializeNode(wdStreamWriter& stream) const
{
  stream.WriteVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_fReferenceValue;
  stream << m_Comparison;

  WD_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_NumberPin.Serialize(stream));

  return WD_SUCCESS;
}

wdResult wdCompareNumberAnimNode::DeserializeNode(wdStreamReader& stream)
{
  stream.ReadVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_fReferenceValue;
  stream >> m_Comparison;

  WD_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_NumberPin.Deserialize(stream));

  return WD_SUCCESS;
}

void wdCompareNumberAnimNode::Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget)
{
  if (wdComparisonOperator::Compare<double>(m_Comparison, m_NumberPin.GetNumber(graph), m_fReferenceValue))
  {
    m_ActivePin.SetTriggered(graph, true);
  }
  else
  {
    m_ActivePin.SetTriggered(graph, false);
  }
}


WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_LogicAnimNodes);
