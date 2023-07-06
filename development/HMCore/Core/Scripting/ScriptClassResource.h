#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/ArrayMap.h>

class wdWorld;
using wdScriptClassResourceHandle = wdTypedResourceHandle<class wdScriptClassResource>;

class WD_CORE_DLL wdScriptInstance
{
public:
  virtual ~wdScriptInstance() = default;
  virtual void ApplyParameters(const wdArrayMap<wdHashedString, wdVariant>& parameters) = 0;
};

class WD_CORE_DLL wdScriptRTTI : public wdRTTI, public wdRefCountingImpl
{
  WD_DISALLOW_COPY_AND_ASSIGN(wdScriptRTTI);

public:
  enum
  {
    NumInplaceFunctions = 7
  };

  using FunctionList = wdSmallArray<wdUniquePtr<wdAbstractFunctionProperty>, NumInplaceFunctions>;
  using MessageHandlerList = wdSmallArray<wdUniquePtr<wdAbstractMessageHandler>, NumInplaceFunctions>;

  wdScriptRTTI(wdStringView sName, const wdRTTI* pParentType, FunctionList&& functions, MessageHandlerList&& messageHandlers);
  ~wdScriptRTTI();

  const wdAbstractFunctionProperty* GetFunctionByIndex(wdUInt32 uiIndex) const;

private:
  wdString m_sTypeNameStorage;
  FunctionList m_FunctionStorage;
  MessageHandlerList m_MessageHandlerStorage;
  wdSmallArray<wdAbstractFunctionProperty*, NumInplaceFunctions> m_FunctionRawPtrs;
  wdSmallArray<wdAbstractMessageHandler*, NumInplaceFunctions> m_MessageHandlerRawPtrs;
};

class WD_CORE_DLL wdScriptClassResource : public wdResource
{
  WD_ADD_DYNAMIC_REFLECTION(wdScriptClassResource, wdResource);
  WD_RESOURCE_DECLARE_COMMON_CODE(wdScriptClassResource);

public:
  wdScriptClassResource();
  ~wdScriptClassResource();

  const wdSharedPtr<wdScriptRTTI>& GetType() const { return m_pType; }

  virtual wdUniquePtr<wdScriptInstance> Instantiate(wdReflectedClass& inout_owner, wdWorld* pWorld) const = 0;

protected:
  void CreateScriptType(wdStringView sName, const wdRTTI* pBaseType, wdScriptRTTI::FunctionList&& functions, wdScriptRTTI::MessageHandlerList&& messageHandlers);
  void DeleteScriptType();

  wdSharedPtr<wdScriptRTTI> m_pType;
};
