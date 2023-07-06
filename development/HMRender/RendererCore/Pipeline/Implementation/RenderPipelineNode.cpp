#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/RenderPipelineNode.h>

// WD_CHECK_AT_COMPILETIME(sizeof(wdRenderPipelineNodePin) == 4);

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdRenderPipelineNode, 1, wdRTTINoAllocator)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_BEGIN_STATIC_REFLECTED_TYPE(wdRenderPipelineNodePin, wdNoBase, 1, wdRTTINoAllocator)
{
  WD_BEGIN_ATTRIBUTES
  {
   new wdHiddenAttribute(),
  }
  WD_END_ATTRIBUTES;
}
WD_END_STATIC_REFLECTED_TYPE;

WD_BEGIN_STATIC_REFLECTED_TYPE(wdRenderPipelineNodeInputPin, wdRenderPipelineNodePin, 1, wdRTTINoAllocator)
WD_END_STATIC_REFLECTED_TYPE;

WD_BEGIN_STATIC_REFLECTED_TYPE(wdRenderPipelineNodeOutputPin, wdRenderPipelineNodePin, 1, wdRTTINoAllocator)
WD_END_STATIC_REFLECTED_TYPE;

WD_BEGIN_STATIC_REFLECTED_TYPE(wdRenderPipelineNodePassThrougPin, wdRenderPipelineNodePin, 1, wdRTTINoAllocator)
WD_END_STATIC_REFLECTED_TYPE;
// clang-format on

void wdRenderPipelineNode::InitializePins()
{
  m_InputPins.Clear();
  m_OutputPins.Clear();
  m_NameToPin.Clear();

  const wdRTTI* pType = GetDynamicRTTI();

  wdHybridArray<wdAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  for (auto pProp : properties)
  {
    if (pProp->GetCategory() != wdPropertyCategory::Member || !pProp->GetSpecificType()->IsDerivedFrom(wdGetStaticRTTI<wdRenderPipelineNodePin>()))
      continue;

    auto pPinProp = static_cast<wdAbstractMemberProperty*>(pProp);
    wdRenderPipelineNodePin* pPin = static_cast<wdRenderPipelineNodePin*>(pPinProp->GetPropertyPointer(this));

    pPin->m_pParent = this;
    if (pPin->m_Type == wdRenderPipelineNodePin::Type::Unknown)
    {
      WD_REPORT_FAILURE("Pin '{0}' has an invalid type. Do not use wdRenderPipelineNodePin directly as member but one of its derived types", pProp->GetPropertyName());
      continue;
    }

    if (pPin->m_Type == wdRenderPipelineNodePin::Type::Input || pPin->m_Type == wdRenderPipelineNodePin::Type::PassThrough)
    {
      pPin->m_uiInputIndex = static_cast<wdUInt8>(m_InputPins.GetCount());
      m_InputPins.PushBack(pPin);
    }
    if (pPin->m_Type == wdRenderPipelineNodePin::Type::Output || pPin->m_Type == wdRenderPipelineNodePin::Type::PassThrough)
    {
      pPin->m_uiOutputIndex = static_cast<wdUInt8>(m_OutputPins.GetCount());
      m_OutputPins.PushBack(pPin);
    }

    wdHashedString sHashedName;
    sHashedName.Assign(pProp->GetPropertyName());
    m_NameToPin.Insert(sHashedName, pPin);
  }
}

wdHashedString wdRenderPipelineNode::GetPinName(const wdRenderPipelineNodePin* pPin) const
{
  for (auto it = m_NameToPin.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value() == pPin)
    {
      return it.Key();
    }
  }
  return wdHashedString();
}

const wdRenderPipelineNodePin* wdRenderPipelineNode::GetPinByName(const char* szName) const
{
  wdHashedString sHashedName;
  sHashedName.Assign(szName);
  return GetPinByName(sHashedName);
}

const wdRenderPipelineNodePin* wdRenderPipelineNode::GetPinByName(wdHashedString sName) const
{
  const wdRenderPipelineNodePin* pin;
  if (m_NameToPin.TryGetValue(sName, pin))
  {
    return pin;
  }

  return nullptr;
}


WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelineNode);
