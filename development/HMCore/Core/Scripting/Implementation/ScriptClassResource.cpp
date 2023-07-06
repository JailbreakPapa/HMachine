#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClassResource.h>

wdScriptRTTI::wdScriptRTTI(wdStringView sName, const wdRTTI* pParentType, FunctionList&& functions, MessageHandlerList&& messageHandlers)
  : wdRTTI(nullptr, pParentType, 0, 1, wdVariantType::Invalid, wdTypeFlags::Class, nullptr, wdArrayPtr<wdAbstractProperty*>(), wdArrayPtr<wdAbstractFunctionProperty*>(), wdArrayPtr<wdPropertyAttribute*>(), wdArrayPtr<wdAbstractMessageHandler*>(), wdArrayPtr<wdMessageSenderInfo>(), nullptr)
  , m_sTypeNameStorage(sName)
  , m_FunctionStorage(std::move(functions))
  , m_MessageHandlerStorage(std::move(messageHandlers))
{
  m_szTypeName = m_sTypeNameStorage.GetData();

  for (auto& pFunction : m_FunctionStorage)
  {
    if (pFunction != nullptr)
    {
      m_FunctionRawPtrs.PushBack(pFunction.Borrow());
    }
  }

  for (auto& pMessageHandler : m_MessageHandlerStorage)
  {
    if (pMessageHandler != nullptr)
    {
      m_MessageHandlerRawPtrs.PushBack(pMessageHandler.Borrow());
    }
  }

  m_Functions = m_FunctionRawPtrs;
  m_MessageHandlers = m_MessageHandlerRawPtrs;

  RegisterType();

  SetupParentHierarchy();
  GatherDynamicMessageHandlers();
}

wdScriptRTTI::~wdScriptRTTI()
{
  UnregisterType();
  m_szTypeName = nullptr;
}

const wdAbstractFunctionProperty* wdScriptRTTI::GetFunctionByIndex(wdUInt32 uiIndex) const
{
  if (uiIndex < m_FunctionStorage.GetCount())
  {
    return m_FunctionStorage.GetData()[uiIndex].Borrow();
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdScriptClassResource, 1, wdRTTINoAllocator)
WD_END_DYNAMIC_REFLECTED_TYPE;
WD_RESOURCE_IMPLEMENT_COMMON_CODE(wdScriptClassResource);
// clang-format on

wdScriptClassResource::wdScriptClassResource()
  : wdResource(DoUpdate::OnAnyThread, 1)
{
}

wdScriptClassResource::~wdScriptClassResource() = default;

void wdScriptClassResource::CreateScriptType(wdStringView sName, const wdRTTI* pBaseType, wdScriptRTTI::FunctionList&& functions, wdScriptRTTI::MessageHandlerList&& messageHandlers)
{
  wdScriptRTTI::FunctionList sortedFunctions;
  for (auto pFuncProp : pBaseType->GetFunctions())
  {
    auto pBaseClassFuncAttr = pFuncProp->GetAttributeByType<wdScriptBaseClassFunctionAttribute>();
    if (pBaseClassFuncAttr == nullptr)
      continue;

    wdStringView sBaseClassFuncName = pFuncProp->GetPropertyName();
    sBaseClassFuncName.TrimWordStart("Reflection_");

    wdUInt16 uiIndex = pBaseClassFuncAttr->GetIndex();
    sortedFunctions.EnsureCount(uiIndex + 1);

    for (auto& pScriptFuncProp : functions)
    {
      if (pScriptFuncProp == nullptr)
        continue;

      if (sBaseClassFuncName == pScriptFuncProp->GetPropertyName())
      {
        sortedFunctions[uiIndex] = std::move(pScriptFuncProp);
        break;
      }
    }
  }

  m_pType = WD_DEFAULT_NEW(wdScriptRTTI, sName, pBaseType, std::move(sortedFunctions), std::move(messageHandlers));
}

void wdScriptClassResource::DeleteScriptType()
{
  m_pType = nullptr;
}
