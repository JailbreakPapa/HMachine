#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptClassResource.h>
#include <Core/Scripting/ScriptWorldModule.h>

// clang-format off
WD_IMPLEMENT_WORLD_MODULE(wdScriptWorldModule);
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdScriptWorldModule, 1, wdRTTINoAllocator)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdScriptWorldModule::wdScriptWorldModule(wdWorld* pWorld)
  : wdWorldModule(pWorld)
{
  wdResourceManager::GetResourceEvents().AddEventHandler(wdMakeDelegate(&wdScriptWorldModule::ResourceEventHandler, this));
}

wdScriptWorldModule::~wdScriptWorldModule()
{
  wdResourceManager::GetResourceEvents().RemoveEventHandler(wdMakeDelegate(&wdScriptWorldModule::ResourceEventHandler, this));
}

void wdScriptWorldModule::Initialize()
{
  SUPER::Initialize();

  {
    auto updateDesc = WD_CREATE_MODULE_UPDATE_FUNCTION_DESC(wdScriptWorldModule::CallUpdateFunctions, this);
    updateDesc.m_Phase = wdWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    updateDesc.m_bOnlyUpdateWhenSimulating = true;

    RegisterUpdateFunction(updateDesc);
  }

  {
    auto updateDesc = WD_CREATE_MODULE_UPDATE_FUNCTION_DESC(wdScriptWorldModule::ReloadScripts, this);
    updateDesc.m_Phase = wdWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    updateDesc.m_fPriority = 10000.0f;

    RegisterUpdateFunction(updateDesc);
  }
}

void wdScriptWorldModule::AddUpdateFunctionToSchedule(const wdAbstractFunctionProperty* pFunction, void* pInstance, wdTime updateInterval)
{
  FunctionContext context;
  context.m_pFunction = pFunction;
  context.m_pInstance = pInstance;

  m_Scheduler.AddOrUpdateWork(context, updateInterval);
}

void wdScriptWorldModule::RemoveUpdateFunctionToSchedule(const wdAbstractFunctionProperty* pFunction, void* pInstance)
{
  FunctionContext context;
  context.m_pFunction = pFunction;
  context.m_pInstance = pInstance;

  m_Scheduler.RemoveWork(context);
}

void wdScriptWorldModule::AddScriptReloadFunction(wdScriptClassResourceHandle hScript, ReloadFunction function)
{
  if (hScript.IsValid() == false)
    return;

  m_ReloadFunctions[hScript].PushBack(function);
}

void wdScriptWorldModule::RemoveScriptReloadFunction(wdScriptClassResourceHandle hScript, void* pInstance)
{
  ReloadFunctionList* pReloadFunctions = nullptr;
  if (m_ReloadFunctions.TryGetValue(hScript, pReloadFunctions))
  {
    for (wdUInt32 i = 0; i < pReloadFunctions->GetCount(); ++i)
    {
      if ((*pReloadFunctions)[i].GetClassInstance() == pInstance)
      {
        pReloadFunctions->RemoveAtAndSwap(i);
        break;
      }
    }
  }
}

void wdScriptWorldModule::CallUpdateFunctions(const wdWorldModule::UpdateContext& context)
{
  const wdTime deltaTime = GetWorld()->GetClock().GetTimeDiff();
  m_Scheduler.Update(deltaTime, [this](const FunctionContext& context, wdTime deltaTime) {
    wdVariant returnValue;
    context.m_pFunction->Execute(context.m_pInstance, wdArrayPtr<wdVariant>(), returnValue); });
}

void wdScriptWorldModule::ReloadScripts(const wdWorldModule::UpdateContext& context)
{
  for (auto hScript : m_NeedReload)
  {
    if (m_ReloadFunctions.TryGetValue(hScript, m_TempReloadFunctions))
    {
      for (auto& reloadFunction : m_TempReloadFunctions)
      {
        reloadFunction();
      }
    }
  }

  m_NeedReload.Clear();
}

void wdScriptWorldModule::ResourceEventHandler(const wdResourceEvent& e)
{
  if (e.m_Type != wdResourceEvent::Type::ResourceContentUnloading)
    return;

  if (auto pResource = wdDynamicCast<const wdScriptClassResource*>(e.m_pResource))
  {
    wdScriptClassResourceHandle hScript = pResource->GetResourceHandle();
    m_NeedReload.Insert(hScript);
  }
}
