#include <Core/CorePCH.h>

#include <Core/GameState/GameStateBase.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdGameStateBase, 1, wdRTTINoAllocator)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdGameStateBase::wdGameStateBase() = default;
wdGameStateBase::~wdGameStateBase() = default;

void wdGameStateBase::ProcessInput() {}

void wdGameStateBase::BeforeWorldUpdate() {}

void wdGameStateBase::AfterWorldUpdate() {}

void wdGameStateBase::RequestQuit()
{
  m_bStateWantsToQuit = true;
}

bool wdGameStateBase::WasQuitRequested() const
{
  return m_bStateWantsToQuit;
}



WD_STATICLINK_FILE(Core, Core_GameState_Implementation_GameStateBase);
