#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>

using wdForwardEventsToGameStateComponentManager = wdComponentManager<class wdForwardEventsToGameStateComponent, wdBlockStorageType::Compact>;

/// \brief This event handler component forwards any message that it receives to the active wdGameStateBase.
///
/// Game states can have message handlers just like any other reflected type.
/// However, since they are not part of the wdWorld, messages are not delivered to them.
/// By attaching this component to a game object, all event messages that arrive at that node are
/// forwarded to the active game state. This way, a game state can receive information, such as
/// when a trigger gets activated.
///
/// Multiple of these components can exist in a scene, gathering and forwarding messages from many
/// different game objects, so that the game state can react to many different things.
class WD_CORE_DLL wdForwardEventsToGameStateComponent : public wdEventMessageHandlerComponent
{
  WD_DECLARE_COMPONENT_TYPE(wdForwardEventsToGameStateComponent, wdEventMessageHandlerComponent, wdForwardEventsToGameStateComponentManager);

public:
  //////////////////////////////////////////////////////////////////////////
  // wdForwardEventsToGameStateComponent

public:
  wdForwardEventsToGameStateComponent();
  ~wdForwardEventsToGameStateComponent();

protected:
  virtual bool HandlesMessage(const wdMessage& msg) const override;
  virtual bool OnUnhandledMessage(wdMessage& msg, bool bWasPostedMsg) override;
  virtual bool OnUnhandledMessage(wdMessage& msg, bool bWasPostedMsg) const override;

  virtual void Initialize() override;
};
