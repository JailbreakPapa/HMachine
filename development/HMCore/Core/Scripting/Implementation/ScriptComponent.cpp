#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptComponent.h>
#include <Core/Scripting/ScriptWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
WD_BEGIN_COMPONENT_TYPE(wdScriptComponent, 1, wdComponentMode::Static)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("UpdateInterval", GetUpdateInterval, SetUpdateInterval)->AddAttributes(new wdClampValueAttribute(wdTime::Zero(), wdVariant())),
    WD_ACCESSOR_PROPERTY("ScriptClass", GetScriptClassFile, SetScriptClassFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_ScriptClass")),
    WD_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new wdExposedParametersAttribute("ScriptClass")),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Scripting"),
    new wdInDevelopmentAttribute(wdInDevelopmentAttribute::Phase::Alpha),
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdScriptComponent::wdScriptComponent() = default;
wdScriptComponent::~wdScriptComponent() = default;

void wdScriptComponent::SerializeComponent(wdWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hScriptClass;
  s << m_UpdateInterval;

  wdUInt16 uiNumParams = static_cast<wdUInt16>(m_Parameters.GetCount());
  s << uiNumParams;

  for (wdUInt32 p = 0; p < uiNumParams; ++p)
  {
    s << m_Parameters.GetKey(p);
    s << m_Parameters.GetValue(p);
  }
}

void wdScriptComponent::DeserializeComponent(wdWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const wdUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_hScriptClass;
  s >> m_UpdateInterval;

  wdUInt16 uiNumParams = 0;
  s >> uiNumParams;
  m_Parameters.Reserve(uiNumParams);

  wdHashedString key;
  wdVariant value;
  for (wdUInt32 p = 0; p < uiNumParams; ++p)
  {
    s >> key;
    s >> value;

    m_Parameters.Insert(key, value);
  }
}

void wdScriptComponent::Initialize()
{
  SUPER::Initialize();

  if (m_hScriptClass.IsValid())
  {
    InstantiateScript(false);
  }
}

void wdScriptComponent::Deinitialize()
{
  SUPER::Deinitialize();

  ClearInstance(false);
}

void wdScriptComponent::OnActivated()
{
  SUPER::OnActivated();

  CallScriptFunction(wdComponent_ScriptBaseClassFunctions::OnActivated);
}

void wdScriptComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  CallScriptFunction(wdComponent_ScriptBaseClassFunctions::OnDeactivated);
}

void wdScriptComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  CallScriptFunction(wdComponent_ScriptBaseClassFunctions::OnSimulationStarted);
}

void wdScriptComponent::BroadcastEventMsg(wdEventMessage& msg)
{
  const wdRTTI* pType = msg.GetDynamicRTTI();
  for (auto& sender : m_EventSenders)
  {
    if (sender.m_pMsgType == pType)
    {
      sender.m_Sender.SendEventMessage(msg, this, GetOwner());
      return;
    }
  }

  auto& sender = m_EventSenders.ExpandAndGetRef();
  sender.m_pMsgType = pType;
  sender.m_Sender.SendEventMessage(msg, this, GetOwner());
}

void wdScriptComponent::SetScriptClass(const wdScriptClassResourceHandle& hScript)
{
  if (m_hScriptClass == hScript)
    return;

  if (IsInitialized())
  {
    ClearInstance(IsActiveAndInitialized());
  }

  m_hScriptClass = hScript;

  if (IsInitialized() && m_hScriptClass.IsValid())
  {
    InstantiateScript(IsActiveAndInitialized());
  }
}

void wdScriptComponent::SetScriptClassFile(const char* szFile)
{
  wdScriptClassResourceHandle hScript;

  if (!wdStringUtils::IsNullOrEmpty(szFile))
  {
    hScript = wdResourceManager::LoadResource<wdScriptClassResource>(szFile);
  }

  SetScriptClass(hScript);
}

const char* wdScriptComponent::GetScriptClassFile() const
{
  return m_hScriptClass.IsValid() ? m_hScriptClass.GetResourceID().GetData() : "";
}

void wdScriptComponent::SetUpdateInterval(wdTime interval)
{
  m_UpdateInterval = interval;

  if (IsActiveAndInitialized())
  {
    UpdateScheduling();
  }
}

wdTime wdScriptComponent::GetUpdateInterval() const
{
  return m_UpdateInterval;
}

const wdRangeView<const char*, wdUInt32> wdScriptComponent::GetParameters() const
{
  return wdRangeView<const char*, wdUInt32>([]() -> wdUInt32 { return 0; },
    [this]() -> wdUInt32 { return m_Parameters.GetCount(); },
    [](wdUInt32& it) { ++it; },
    [this](const wdUInt32& it) -> const char* { return m_Parameters.GetKey(it).GetString().GetData(); });
}

void wdScriptComponent::SetParameter(const char* szKey, const wdVariant& value)
{
  wdHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != wdInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;
}

void wdScriptComponent::RemoveParameter(const char* szKey)
{
  if (m_Parameters.RemoveAndCopy(wdTempHashedString(szKey)))
  {
  }
}

bool wdScriptComponent::GetParameter(const char* szKey, wdVariant& out_value) const
{
  wdUInt32 it = m_Parameters.Find(szKey);

  if (it == wdInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}

void wdScriptComponent::InstantiateScript(bool bActivate)
{
  ClearInstance(IsActiveAndInitialized());

  wdResourceLock<wdScriptClassResource> pScript(m_hScriptClass, wdResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pScript.GetAcquireResult() != wdResourceAcquireResult::Final)
  {
    wdLog::Error("Failed to load script '{}'", GetScriptClassFile());
    return;
  }

  auto pScriptType = pScript->GetType();
  if (pScriptType == nullptr || pScriptType->IsDerivedFrom(wdGetStaticRTTI<wdComponent>()) == false)
  {
    wdLog::Error("Script type '{}' is not a component", pScriptType != nullptr ? pScriptType->GetTypeName() : "NULL");
    return;
  }

  m_pScriptType = pScriptType;
  m_pMessageDispatchType = pScriptType;

  m_pInstance = pScript->Instantiate(*this, GetWorld());
  if (m_pInstance != nullptr)
  {
    m_pInstance->ApplyParameters(m_Parameters);
  }

  UpdateScheduling();

  CallScriptFunction(wdComponent_ScriptBaseClassFunctions::Initialize);
  if (bActivate)
  {
    CallScriptFunction(wdComponent_ScriptBaseClassFunctions::OnActivated);
  }
}

void wdScriptComponent::ClearInstance(bool bDeactivate)
{
  if (bDeactivate)
  {
    CallScriptFunction(wdComponent_ScriptBaseClassFunctions::OnDeactivated);
  }
  CallScriptFunction(wdComponent_ScriptBaseClassFunctions::Deinitialize);

  auto pModule = GetWorld()->GetOrCreateModule<wdScriptWorldModule>();
  if (auto pUpdateFunction = GetScriptFunction(wdComponent_ScriptBaseClassFunctions::Update))
  {
    pModule->RemoveUpdateFunctionToSchedule(pUpdateFunction, m_pInstance.Borrow());
  }

  pModule->RemoveScriptReloadFunction(m_hScriptClass, this);

  m_pInstance = nullptr;
  m_pScriptType = nullptr;

  m_pMessageDispatchType = GetDynamicRTTI();
}

void wdScriptComponent::UpdateScheduling()
{
  auto pModule = GetWorld()->GetOrCreateModule<wdScriptWorldModule>();
  if (auto pUpdateFunction = GetScriptFunction(wdComponent_ScriptBaseClassFunctions::Update))
  {
    pModule->AddUpdateFunctionToSchedule(pUpdateFunction, m_pInstance.Borrow(), m_UpdateInterval);
  }

  pModule->AddScriptReloadFunction(m_hScriptClass,
    [this]() {
      InstantiateScript(IsActiveAndInitialized());
    });
}

const wdAbstractFunctionProperty* wdScriptComponent::GetScriptFunction(wdUInt32 uiFunctionIndex)
{
  if (m_pScriptType != nullptr && m_pInstance != nullptr)
  {
    return m_pScriptType->GetFunctionByIndex(uiFunctionIndex);
  }

  return nullptr;
}

void wdScriptComponent::CallScriptFunction(wdUInt32 uiFunctionIndex)
{
  if (auto pFunction = GetScriptFunction(uiFunctionIndex))
  {
    wdVariant returnValue;
    pFunction->Execute(m_pInstance.Borrow(), wdArrayPtr<wdVariant>(), returnValue);
  }
}
