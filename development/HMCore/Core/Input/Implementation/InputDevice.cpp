#include <Core/CorePCH.h>

#include <Core/Input/InputManager.h>

WD_ENUMERABLE_CLASS_IMPLEMENTATION(wdInputDevice);

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdInputDevice, 1, wdRTTINoAllocator)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdKeyState::Enum wdKeyState::GetNewKeyState(wdKeyState::Enum prevState, bool bKeyDown)
{
  switch (prevState)
  {
    case wdKeyState::Down:
    case wdKeyState::Pressed:
      return bKeyDown ? wdKeyState::Down : wdKeyState::Released;
    case wdKeyState::Released:
    case wdKeyState::Up:
      return bKeyDown ? wdKeyState::Pressed : wdKeyState::Up;
  }

  return wdKeyState::Up;
}

wdInputDevice::wdInputDevice()
{
  m_bInitialized = false;
  m_uiLastCharacter = '\0';
}

void wdInputDevice::RegisterInputSlot(const char* szName, const char* szDefaultDisplayName, wdBitflags<wdInputSlotFlags> SlotFlags)
{
  wdInputManager::RegisterInputSlot(szName, szDefaultDisplayName, SlotFlags);
}

void wdInputDevice::Initialize()
{
  if (m_bInitialized)
    return;

  WD_LOG_BLOCK("Initializing Input Device", GetDynamicRTTI()->GetTypeName());

  wdLog::Dev("Input Device Type: {0}, Device Name: {1}", GetDynamicRTTI()->GetParentType()->GetTypeName(), GetDynamicRTTI()->GetTypeName());

  m_bInitialized = true;

  RegisterInputSlots();
  InitializeDevice();
}


void wdInputDevice::UpdateAllHardwareStates(wdTime tTimeDifference)
{
  // tell each device to update its hardware
  for (wdInputDevice* pDevice = wdInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    pDevice->UpdateHardwareState(tTimeDifference);
  }
}

void wdInputDevice::UpdateAllDevices()
{
  // tell each device to update its current input slot values
  for (wdInputDevice* pDevice = wdInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    pDevice->Initialize();
    pDevice->UpdateInputSlotValues();
  }
}

void wdInputDevice::ResetAllDevices()
{
  // tell all devices that the input update is through and they might need to reset some values now
  // this is especially important for device types that will get input messages at some undefined time after this call
  // but not during 'UpdateInputSlotValues'
  for (wdInputDevice* pDevice = wdInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    pDevice->ResetInputSlotValues();
  }
}

wdUInt32 wdInputDevice::RetrieveLastCharacter()
{
  wdUInt32 Temp = m_uiLastCharacter;
  m_uiLastCharacter = L'\0';
  return Temp;
}

wdUInt32 wdInputDevice::RetrieveLastCharacterFromAllDevices()
{
  for (wdInputDevice* pDevice = wdInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    const wdUInt32 Char = pDevice->RetrieveLastCharacter();

    if (Char != L'\0')
      return Char;
  }

  return '\0';
}

float wdInputDevice::GetInputSlotState(const char* szSlot) const
{
  return m_InputSlotValues.GetValueOrDefault(szSlot, 0.f);
}

bool wdInputDevice::HasDeviceBeenUsedLastFrame() const
{
  return m_bGeneratedInputRecently;
}

WD_STATICLINK_FILE(Core, Core_Input_Implementation_InputDevice);
