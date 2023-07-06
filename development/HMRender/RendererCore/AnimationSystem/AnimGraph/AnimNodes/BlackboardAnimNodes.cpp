#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/BlackboardAnimNodes.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdSetBlackboardValueAnimNode, 1, wdRTTIDefaultAllocator<wdSetBlackboardValueAnimNode>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry),
    WD_MEMBER_PROPERTY("SetOnActivation", m_bSetOnActivation)->AddAttributes(new wdDefaultValueAttribute(true)),
    WD_MEMBER_PROPERTY("ActivationValue", m_fOnActivatedValue)->AddAttributes(new wdDefaultValueAttribute(1.0f)),
    WD_MEMBER_PROPERTY("SetOnHold", m_bSetOnHold)->AddAttributes(new wdDefaultValueAttribute(true)),
    WD_MEMBER_PROPERTY("HoldValue", m_fOnHoldValue)->AddAttributes(new wdDefaultValueAttribute(1.0f)),
    WD_MEMBER_PROPERTY("SetOnDeactivation", m_bSetOnDeactivation)->AddAttributes(new wdDefaultValueAttribute(false)),
    WD_MEMBER_PROPERTY("DeactivationValue", m_fOnDeactivatedValue)->AddAttributes(new wdDefaultValueAttribute(0.0f)),

    WD_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new wdHiddenAttribute()),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdTitleAttribute("Set: '{BlackboardEntry}' '{ActivationValue}''"),
    new wdCategoryAttribute("Blackboard"),
    new wdColorAttribute(wdColorScheme::DarkUI(wdColorScheme::Red)),
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdResult wdSetBlackboardValueAnimNode::SerializeNode(wdStreamWriter& stream) const
{
  stream.WriteVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;
  stream << m_fOnActivatedValue;
  stream << m_fOnHoldValue;
  stream << m_fOnDeactivatedValue;
  stream << m_bSetOnActivation;
  stream << m_bSetOnHold;
  stream << m_bSetOnDeactivation;

  WD_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));

  return WD_SUCCESS;
}

wdResult wdSetBlackboardValueAnimNode::DeserializeNode(wdStreamReader& stream)
{
  stream.ReadVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;
  stream >> m_fOnActivatedValue;
  stream >> m_fOnHoldValue;
  stream >> m_fOnDeactivatedValue;
  stream >> m_bSetOnActivation;
  stream >> m_bSetOnHold;
  stream >> m_bSetOnDeactivation;

  WD_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));

  return WD_SUCCESS;
}

void wdSetBlackboardValueAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* wdSetBlackboardValueAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void wdSetBlackboardValueAnimNode::Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget)
{
  auto pBlackboard = graph.GetBlackboard();
  if (pBlackboard == nullptr)
  {
    wdLog::Warning("No blackboard available for the animation controller graph to use.");
    return;
  }

  const bool bIsActiveNow = m_ActivePin.IsTriggered(graph);

  if (bIsActiveNow != m_bLastActiveState)
  {
    m_bLastActiveState = bIsActiveNow;

    if (bIsActiveNow)
    {
      if (m_bSetOnActivation)
      {
        pBlackboard->RegisterEntry(m_sBlackboardEntry, m_fOnActivatedValue);
      }
    }
    else
    {
      if (m_bSetOnDeactivation)
      {
        pBlackboard->RegisterEntry(m_sBlackboardEntry, m_fOnDeactivatedValue);
      }
    }
  }
  else if (bIsActiveNow && m_bSetOnHold)
  {
    pBlackboard->RegisterEntry(m_sBlackboardEntry, m_fOnHoldValue);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdCheckBlackboardValueAnimNode, 1, wdRTTIDefaultAllocator<wdCheckBlackboardValueAnimNode>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry),
    WD_MEMBER_PROPERTY("ReferenceValue", m_fReferenceValue)->AddAttributes(new wdDefaultValueAttribute(1.0f)),
    WD_ENUM_MEMBER_PROPERTY("Comparison", wdComparisonOperator, m_Comparison),

    WD_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new wdHiddenAttribute()),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Blackboard"),
    new wdTitleAttribute("Check: '{BlackboardEntry}' {Comparison} {ReferenceValue}"),
    new wdColorAttribute(wdColorScheme::DarkUI(wdColorScheme::Lime)),
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdResult wdCheckBlackboardValueAnimNode::SerializeNode(wdStreamWriter& stream) const
{
  stream.WriteVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;
  stream << m_fReferenceValue;
  stream << m_Comparison;

  WD_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));

  return WD_SUCCESS;
}

wdResult wdCheckBlackboardValueAnimNode::DeserializeNode(wdStreamReader& stream)
{
  stream.ReadVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;
  stream >> m_fReferenceValue;
  stream >> m_Comparison;

  WD_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));

  return WD_SUCCESS;
}

void wdCheckBlackboardValueAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* wdCheckBlackboardValueAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void wdCheckBlackboardValueAnimNode::Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget)
{
  auto pBlackboard = graph.GetBlackboard();
  if (pBlackboard == nullptr)
  {
    wdLog::Warning("No blackboard available for the animation controller graph to use.");
    return;
  }

  float fValue = 0.0f;
  if (!m_sBlackboardEntry.IsEmpty())
  {
    wdVariant value = pBlackboard->GetEntryValue(m_sBlackboardEntry);

    if (value.IsValid() && value.IsNumber())
    {
      fValue = value.ConvertTo<float>();
    }
    else
    {
      wdLog::Warning("Blackboard entry '{}' doesn't exist.", m_sBlackboardEntry);
      return;
    }
  }

  if (wdComparisonOperator::Compare(m_Comparison, fValue, m_fReferenceValue))
  {
    m_ActivePin.SetTriggered(graph, true);
  }
  else
  {
    m_ActivePin.SetTriggered(graph, false);
  }
}

//////////////////////////////////////////////////////////////////////////


// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdGetBlackboardNumberAnimNode, 1, wdRTTIDefaultAllocator<wdGetBlackboardNumberAnimNode>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry),

    WD_MEMBER_PROPERTY("Number", m_NumberPin)->AddAttributes(new wdHiddenAttribute()),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Blackboard"),
    new wdTitleAttribute("Get: '{BlackboardEntry}'"),
    new wdColorAttribute(wdColorScheme::DarkUI(wdColorScheme::Lime)),
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdResult wdGetBlackboardNumberAnimNode::SerializeNode(wdStreamWriter& stream) const
{
  stream.WriteVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;

  WD_SUCCEED_OR_RETURN(m_NumberPin.Serialize(stream));

  return WD_SUCCESS;
}

wdResult wdGetBlackboardNumberAnimNode::DeserializeNode(wdStreamReader& stream)
{
  stream.ReadVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;

  WD_SUCCEED_OR_RETURN(m_NumberPin.Deserialize(stream));

  return WD_SUCCESS;
}

void wdGetBlackboardNumberAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* wdGetBlackboardNumberAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void wdGetBlackboardNumberAnimNode::Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget)
{
  auto pBlackboard = graph.GetBlackboard();
  if (pBlackboard == nullptr)
  {
    wdLog::Warning("No blackboard available for the animation controller graph to use.");
    return;
  }

  double fValue = 0.0f;

  if (!m_sBlackboardEntry.IsEmpty())
  {
    wdVariant value = pBlackboard->GetEntryValue(m_sBlackboardEntry);

    if (value.IsValid() && value.IsNumber())
    {
      fValue = value.ConvertTo<double>();
    }
    else
    {
      wdLog::Warning("Blackboard entry '{}' doesn't exist.", m_sBlackboardEntry);
      return;
    }
  }

  m_NumberPin.SetNumber(graph, fValue);
}


WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_BlackboardAnimNodes);
