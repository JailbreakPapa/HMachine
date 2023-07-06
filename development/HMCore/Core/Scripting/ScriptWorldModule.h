#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/Utils/IntervalScheduler.h>
#include <Core/World/World.h>

using wdScriptClassResourceHandle = wdTypedResourceHandle<class wdScriptClassResource>;

class WD_CORE_DLL wdScriptWorldModule : public wdWorldModule
{
  WD_DECLARE_WORLD_MODULE();
  WD_ADD_DYNAMIC_REFLECTION(wdScriptWorldModule, wdWorldModule);

public:
  wdScriptWorldModule(wdWorld* pWorld);
  ~wdScriptWorldModule();

  virtual void Initialize() override;

  void AddUpdateFunctionToSchedule(const wdAbstractFunctionProperty* pFunction, void* pInstance, wdTime updateInterval);
  void RemoveUpdateFunctionToSchedule(const wdAbstractFunctionProperty* pFunction, void* pInstance);

  using ReloadFunction = wdDelegate<void()>;
  void AddScriptReloadFunction(wdScriptClassResourceHandle hScript, ReloadFunction function);
  void RemoveScriptReloadFunction(wdScriptClassResourceHandle hScript, void* pInstance);

  struct FunctionContext
  {
    const wdAbstractFunctionProperty* m_pFunction = nullptr;
    void* m_pInstance = nullptr;

    bool operator==(const FunctionContext& other) const
    {
      return m_pFunction == other.m_pFunction && m_pInstance == other.m_pInstance;
    }
  };

private:
  void CallUpdateFunctions(const wdWorldModule::UpdateContext& context);
  void ReloadScripts(const wdWorldModule::UpdateContext& context);
  void ResourceEventHandler(const wdResourceEvent& e);

  wdIntervalScheduler<FunctionContext> m_Scheduler;

  using ReloadFunctionList = wdHybridArray<ReloadFunction, 8>;
  wdHashTable<wdScriptClassResourceHandle, ReloadFunctionList> m_ReloadFunctions;
  wdHashSet<wdScriptClassResourceHandle> m_NeedReload;
  ReloadFunctionList m_TempReloadFunctions;
};

//////////////////////////////////////////////////////////////////////////

template <>
struct wdHashHelper<wdScriptWorldModule::FunctionContext>
{
  WD_ALWAYS_INLINE static wdUInt32 Hash(const wdScriptWorldModule::FunctionContext& value)
  {
    wdUInt32 hash = wdHashHelper<const void*>::Hash(value.m_pFunction);
    hash = wdHashingUtils::CombineHashValues32(hash, wdHashHelper<void*>::Hash(value.m_pInstance));
    return hash;
  }

  WD_ALWAYS_INLINE static bool Equal(const wdScriptWorldModule::FunctionContext& a, const wdScriptWorldModule::FunctionContext& b) { return a == b; }
};
