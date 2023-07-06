#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/MathAnimNodes.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdMathExpressionAnimNode, 1, wdRTTIDefaultAllocator<wdMathExpressionAnimNode>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("Expression", GetExpression, SetExpression)->AddAttributes(new wdDefaultValueAttribute("a*a + (b-c) / abs(d)")),
    WD_MEMBER_PROPERTY("a", m_ValueAPin)->AddAttributes(new wdHiddenAttribute),
    WD_MEMBER_PROPERTY("b", m_ValueBPin)->AddAttributes(new wdHiddenAttribute),
    WD_MEMBER_PROPERTY("c", m_ValueCPin)->AddAttributes(new wdHiddenAttribute),
    WD_MEMBER_PROPERTY("d", m_ValueDPin)->AddAttributes(new wdHiddenAttribute),
    WD_MEMBER_PROPERTY("Result", m_ResultPin)->AddAttributes(new wdHiddenAttribute),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Math"),
    new wdColorAttribute(wdColorScheme::DarkUI(wdColorScheme::Lime)),
    new wdTitleAttribute("= {Expression}"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdMathExpressionAnimNode::wdMathExpressionAnimNode() = default;
wdMathExpressionAnimNode::~wdMathExpressionAnimNode() = default;

void wdMathExpressionAnimNode::SetExpression(const char* szSz)
{
  m_mExpression.Reset(szSz);
}

const char* wdMathExpressionAnimNode::GetExpression() const
{
  return m_mExpression.GetExpressionString();
}

wdResult wdMathExpressionAnimNode::SerializeNode(wdStreamWriter& stream) const
{
  stream.WriteVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_mExpression.GetExpressionString();
  WD_SUCCEED_OR_RETURN(m_ValueAPin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_ValueBPin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_ValueCPin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_ValueDPin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_ResultPin.Serialize(stream));

  return WD_SUCCESS;
}

wdResult wdMathExpressionAnimNode::DeserializeNode(wdStreamReader& stream)
{
  stream.ReadVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  wdStringBuilder tmp;
  stream >> tmp;
  m_mExpression.Reset(tmp);
  WD_SUCCEED_OR_RETURN(m_ValueAPin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_ValueBPin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_ValueCPin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_ValueDPin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_ResultPin.Deserialize(stream));

  return WD_SUCCESS;
}

void wdMathExpressionAnimNode::Initialize(wdAnimGraph& graph, const wdSkeletonResource* pSkeleton)
{
  if (!m_mExpression.IsValid() && m_ResultPin.IsConnected())
  {
    wdLog::Error("Math expression '{}' is invalid.", m_mExpression.GetExpressionString());
  }
}

static wdHashedString s_sA = wdMakeHashedString("a");
static wdHashedString s_sB = wdMakeHashedString("b");
static wdHashedString s_sC = wdMakeHashedString("c");
static wdHashedString s_sD = wdMakeHashedString("d");

void wdMathExpressionAnimNode::Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget)
{
  if (!m_mExpression.IsValid())
  {
    m_ResultPin.SetNumber(graph, 0);
    return;
  }

  wdMathExpression::Input inputs[] =
    {
      {s_sA, static_cast<float>(m_ValueAPin.GetNumber(graph))},
      {s_sB, static_cast<float>(m_ValueBPin.GetNumber(graph))},
      {s_sC, static_cast<float>(m_ValueCPin.GetNumber(graph))},
      {s_sD, static_cast<float>(m_ValueDPin.GetNumber(graph))},
    };

  float result = m_mExpression.Evaluate(inputs);
  m_ResultPin.SetNumber(graph, result);
}


WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_MathAnimNodes);
