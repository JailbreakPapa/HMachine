#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Foundation/Types/RangeView.h>

using wdScriptComponentManager = wdComponentManager<class wdScriptComponent, wdBlockStorageType::FreeList>;

class WD_CORE_DLL wdScriptComponent : public wdEventMessageHandlerComponent
{
  WD_DECLARE_COMPONENT_TYPE(wdScriptComponent, wdEventMessageHandlerComponent, wdScriptComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // wdComponent

protected:
  virtual void SerializeComponent(wdWorldWriter& stream) const override;
  virtual void DeserializeComponent(wdWorldReader& stream) override;
  virtual void Initialize() override;
  virtual void Deinitialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  /*virtual bool OnUnhandledMessage(wdMessage& msg, bool bWasPostedMsg) override;
  virtual bool OnUnhandledMessage(wdMessage& msg, bool bWasPostedMsg) const override;

  bool HandleUnhandledMessage(wdMessage& msg, bool bWasPostedMsg);*/

  //////////////////////////////////////////////////////////////////////////
  // wdEventMessageHandlerComponent

protected:
  // virtual bool HandlesEventMessage(const wdEventMessage& msg) const override;

  //////////////////////////////////////////////////////////////////////////
  // wdScriptComponent
public:
  wdScriptComponent();
  ~wdScriptComponent();

  void BroadcastEventMsg(wdEventMessage& msg);

  void SetScriptClass(const wdScriptClassResourceHandle& hScript);
  const wdScriptClassResourceHandle& GetScriptClass() const { return m_hScriptClass; }

  void SetScriptClassFile(const char* szFile); // [ property ]
  const char* GetScriptClassFile() const;      // [ property ]

  void SetUpdateInterval(wdTime interval); // [ property ]
  wdTime GetUpdateInterval() const;        // [ property ]

  //////////////////////////////////////////////////////////////////////////
  // Exposed Parameters
  const wdRangeView<const char*, wdUInt32> GetParameters() const;
  void SetParameter(const char* szKey, const wdVariant& value);
  void RemoveParameter(const char* szKey);
  bool GetParameter(const char* szKey, wdVariant& out_value) const;

private:
  void InstantiateScript(bool bActivate);
  void ClearInstance(bool bDeactivate);
  void UpdateScheduling();

  const wdAbstractFunctionProperty* GetScriptFunction(wdUInt32 uiFunctionIndex);
  void CallScriptFunction(wdUInt32 uiFunctionIndex);

  struct EventSender
  {
    const wdRTTI* m_pMsgType = nullptr;
    wdEventMessageSender<wdEventMessage> m_Sender;
  };

  wdHybridArray<EventSender, 2> m_EventSenders;

  wdArrayMap<wdHashedString, wdVariant> m_Parameters;

  wdScriptClassResourceHandle m_hScriptClass;
  wdTime m_UpdateInterval = wdTime::Zero();

  wdSharedPtr<wdScriptRTTI> m_pScriptType;
  wdUniquePtr<wdScriptInstance> m_pInstance;
};
