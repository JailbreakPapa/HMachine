#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/GameState/ForwardEventsToGameStateComponent.h>
#include <Core/GameState/GameStateBase.h>

// clang-format off
WD_BEGIN_COMPONENT_TYPE(wdForwardEventsToGameStateComponent, 1 /* version */, wdComponentMode::Static)
{
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Gameplay/Logic"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_COMPONENT_TYPE
// clang-format on

wdForwardEventsToGameStateComponent::wdForwardEventsToGameStateComponent() = default;
wdForwardEventsToGameStateComponent::~wdForwardEventsToGameStateComponent() = default;

bool wdForwardEventsToGameStateComponent::HandlesMessage(const wdMessage& msg) const
{
  // check whether there is any active game state
  // if so, test whether it would handle this type of message
  if (wdGameStateBase* pGameState = wdGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    return pGameState->GetDynamicRTTI()->CanHandleMessage(msg.GetId());
  }

  return false;
}

bool wdForwardEventsToGameStateComponent::OnUnhandledMessage(wdMessage& msg, bool bWasPostedMsg)
{
  // if we have an active game state, forward the message to it
  if (wdGameStateBase* pGameState = wdGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    return pGameState->GetDynamicRTTI()->DispatchMessage(pGameState, msg);
  }

  return false;
}

bool wdForwardEventsToGameStateComponent::OnUnhandledMessage(wdMessage& msg, bool bWasPostedMsg) const
{
  // if we have an active game state, forward the message to it
  if (const wdGameStateBase* pGameState = wdGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    return pGameState->GetDynamicRTTI()->DispatchMessage(pGameState, msg);
  }

  return false;
}

void wdForwardEventsToGameStateComponent::Initialize()
{
  SUPER::Initialize();

  EnableUnhandledMessageHandler(true);
}


WD_STATICLINK_FILE(Core, Core_GameState_Implementation_ForwardEventsToGameStateComponent);
