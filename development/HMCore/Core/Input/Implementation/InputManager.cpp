#include <Core/CorePCH.h>

#include <Core/Input/InputManager.h>

wdInputManager::wdEventInput wdInputManager::s_InputEvents;
wdInputManager::InternalData* wdInputManager::s_pData = nullptr;
wdUInt32 wdInputManager::s_uiLastCharacter = '\0';
bool wdInputManager::s_bInputSlotResetRequired = true;
wdString wdInputManager::s_sExclusiveInputSet;

wdInputManager::InternalData& wdInputManager::GetInternals()
{
  if (s_pData == nullptr)
    s_pData = WD_DEFAULT_NEW(InternalData);

  return *s_pData;
}

void wdInputManager::DeallocateInternals()
{
  WD_DEFAULT_DELETE(s_pData);
}

wdInputManager::wdInputSlot::wdInputSlot()
{
  m_fValue = 0.0f;
  m_State = wdKeyState::Up;
  m_fDeadZone = 0.0f;
}

void wdInputManager::RegisterInputSlot(const char* szInputSlot, const char* szDefaultDisplayName, wdBitflags<wdInputSlotFlags> SlotFlags)
{
  wdMap<wdString, wdInputSlot>::Iterator it = GetInternals().s_InputSlots.Find(szInputSlot);

  if (it.IsValid())
  {
    if (it.Value().m_SlotFlags != SlotFlags)
    {
      if ((it.Value().m_SlotFlags != wdInputSlotFlags::Default) && (SlotFlags != wdInputSlotFlags::Default))
      {
        wdStringBuilder tmp;
        tmp.Printf("Different devices register Input Slot '%s' with different Slot Flags: %16b vs. %16b", szInputSlot,
          it.Value().m_SlotFlags.GetValue(), SlotFlags.GetValue());
        wdLog::Warning(tmp);
      }

      it.Value().m_SlotFlags |= SlotFlags;
    }

    // If the key already exists, but key and display string are identical, then overwrite the display string with the incoming string
    if (it.Value().m_sDisplayName != it.Key())
      return;
  }

  // wdLog::Debug("Registered Input Slot: '{0}'", szInputSlot);

  wdInputSlot& sm = GetInternals().s_InputSlots[szInputSlot];

  sm.m_sDisplayName = szDefaultDisplayName;
  sm.m_SlotFlags = SlotFlags;

  InputEventData e;
  e.m_EventType = InputEventData::InputSlotChanged;
  e.m_szInputSlot = szInputSlot;

  s_InputEvents.Broadcast(e);
}

wdBitflags<wdInputSlotFlags> wdInputManager::GetInputSlotFlags(const char* szInputSlot)
{
  wdMap<wdString, wdInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(szInputSlot);

  if (it.IsValid())
    return it.Value().m_SlotFlags;

  wdLog::Warning("wdInputManager::GetInputSlotFlags: Input Slot '{0}' does not exist (yet).", szInputSlot);

  return wdInputSlotFlags::Default;
}

void wdInputManager::SetInputSlotDisplayName(const char* szInputSlot, const char* szDefaultDisplayName)
{
  RegisterInputSlot(szInputSlot, szDefaultDisplayName, wdInputSlotFlags::Default);
  GetInternals().s_InputSlots[szInputSlot].m_sDisplayName = szDefaultDisplayName;

  InputEventData e;
  e.m_EventType = InputEventData::InputSlotChanged;
  e.m_szInputSlot = szInputSlot;

  s_InputEvents.Broadcast(e);
}

const char* wdInputManager::GetInputSlotDisplayName(const char* szInputSlot)
{
  wdMap<wdString, wdInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(szInputSlot);

  if (it.IsValid())
    return it.Value().m_sDisplayName.GetData();

  wdLog::Warning("wdInputManager::GetInputSlotDisplayName: Input Slot '{0}' does not exist (yet).", szInputSlot);
  return szInputSlot;
}

const char* wdInputManager::GetInputSlotDisplayName(const char* szInputSet, const char* szAction, wdInt32 iTrigger)
{
  /// \test This is new

  const auto cfg = GetInputActionConfig(szInputSet, szAction);

  if (iTrigger < 0)
  {
    for (iTrigger = 0; iTrigger < wdInputActionConfig::MaxInputSlotAlternatives; ++iTrigger)
    {
      if (!cfg.m_sInputSlotTrigger[iTrigger].IsEmpty())
        break;
    }
  }

  if (iTrigger >= wdInputActionConfig::MaxInputSlotAlternatives)
    return nullptr;

  return GetInputSlotDisplayName(cfg.m_sInputSlotTrigger[iTrigger]);
}

void wdInputManager::SetInputSlotDeadZone(const char* szInputSlot, float fDeadZone)
{
  RegisterInputSlot(szInputSlot, szInputSlot, wdInputSlotFlags::Default);
  GetInternals().s_InputSlots[szInputSlot].m_fDeadZone = wdMath::Max(fDeadZone, 0.0001f);

  InputEventData e;
  e.m_EventType = InputEventData::InputSlotChanged;
  e.m_szInputSlot = szInputSlot;

  s_InputEvents.Broadcast(e);
}

float wdInputManager::GetInputSlotDeadZone(const char* szInputSlot)
{
  wdMap<wdString, wdInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(szInputSlot);

  if (it.IsValid())
    return it.Value().m_fDeadZone;

  wdLog::Warning("wdInputManager::GetInputSlotDeadZone: Input Slot '{0}' does not exist (yet).", szInputSlot);

  wdInputSlot s;
  return s.m_fDeadZone; // return the default value
}

wdKeyState::Enum wdInputManager::GetInputSlotState(const char* szInputSlot, float* pValue)
{
  wdMap<wdString, wdInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(szInputSlot);

  if (it.IsValid())
  {
    if (pValue)
      *pValue = it.Value().m_fValue;

    return it.Value().m_State;
  }

  if (pValue)
    *pValue = 0.0f;

  wdLog::Warning("wdInputManager::GetInputSlotState: Input Slot '{0}' does not exist (yet). To ensure all devices are initialized, call "
                 "wdInputManager::Update before querying device states, or at least call wdInputManager::PollHardware.",
    szInputSlot);
  RegisterInputSlot(szInputSlot, szInputSlot, wdInputSlotFlags::None);

  return wdKeyState::Up;
}

void wdInputManager::PollHardware()
{
  if (s_bInputSlotResetRequired)
  {
    s_bInputSlotResetRequired = false;
    ResetInputSlotValues();
  }

  wdInputDevice::UpdateAllDevices();

  GatherDeviceInputSlotValues();
}

void wdInputManager::Update(wdTime timeDifference)
{
  PollHardware();

  UpdateInputSlotStates();

  s_uiLastCharacter = wdInputDevice::RetrieveLastCharacterFromAllDevices();

  UpdateInputActions(timeDifference);

  wdInputDevice::ResetAllDevices();

  wdInputDevice::UpdateAllHardwareStates(timeDifference);

  s_bInputSlotResetRequired = true;
}

void wdInputManager::ResetInputSlotValues()
{
  // set all input slot values to zero
  // this is crucial for accumulating the new values and for resetting the input state later
  for (wdInputSlotsMap::Iterator it = GetInternals().s_InputSlots.GetIterator(); it.IsValid(); it.Next())
  {
    it.Value().m_fValue = 0.0f;
  }
}

void wdInputManager::GatherDeviceInputSlotValues()
{
  for (wdInputDevice* pDevice = wdInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    pDevice->m_bGeneratedInputRecently = false;

    // iterate over all the input slots that this device provides
    for (auto it = pDevice->m_InputSlotValues.GetIterator(); it.IsValid(); it.Next())
    {
      if (it.Value() > 0.0f)
      {
        wdInputManager::wdInputSlot& Slot = GetInternals().s_InputSlots[it.Key()];

        // do not store a value larger than 0 unless it exceeds the dead-zone threshold
        if (it.Value() > Slot.m_fDeadZone)
        {
          Slot.m_fValue = wdMath::Max(Slot.m_fValue, it.Value()); // 'accumulate' the values for one slot from all the connected devices

          pDevice->m_bGeneratedInputRecently = true;
        }
      }
    }
  }

  wdMap<wdString, float>::Iterator it = GetInternals().s_InjectedInputSlots.GetIterator();

  for (; it.IsValid(); ++it)
  {
    wdInputManager::wdInputSlot& Slot = GetInternals().s_InputSlots[it.Key()];

    // do not store a value larger than 0 unless it exceeds the dead-zone threshold
    if (it.Value() > Slot.m_fDeadZone)
      Slot.m_fValue = wdMath::Max(Slot.m_fValue, it.Value()); // 'accumulate' the values for one slot from all the connected devices
  }

  GetInternals().s_InjectedInputSlots.Clear();
}

void wdInputManager::UpdateInputSlotStates()
{
  for (wdInputSlotsMap::Iterator it = GetInternals().s_InputSlots.GetIterator(); it.IsValid(); it.Next())
  {
    // update the state of the input slot, depending on its current value
    // its value will only be larger than zero, if it is also larger than its dead-zone value
    const wdKeyState::Enum NewState = wdKeyState::GetNewKeyState(it.Value().m_State, it.Value().m_fValue > 0.0f);

    if ((it.Value().m_State != NewState) || (NewState != wdKeyState::Up))
    {
      it.Value().m_State = NewState;

      InputEventData e;
      e.m_EventType = InputEventData::InputSlotChanged;
      e.m_szInputSlot = it.Key().GetData();

      s_InputEvents.Broadcast(e);
    }
  }
}

void wdInputManager::RetrieveAllKnownInputSlots(wdDynamicArray<const char*>& out_inputSlots)
{
  out_inputSlots.Clear();
  out_inputSlots.Reserve(GetInternals().s_InputSlots.GetCount());

  // just copy all slot names into the given array
  for (wdInputSlotsMap::Iterator it = GetInternals().s_InputSlots.GetIterator(); it.IsValid(); it.Next())
  {
    out_inputSlots.PushBack(it.Key().GetData());
  }
}

wdUInt32 wdInputManager::RetrieveLastCharacter(bool bResetCurrent)
{
  if (!bResetCurrent)
    return s_uiLastCharacter;

  wdUInt32 Temp = s_uiLastCharacter;
  s_uiLastCharacter = L'\0';
  return Temp;
}

void wdInputManager::InjectInputSlotValue(const char* szInputSlot, float fValue)
{
  GetInternals().s_InjectedInputSlots[szInputSlot] = wdMath::Max(GetInternals().s_InjectedInputSlots[szInputSlot], fValue);
}

const char* wdInputManager::GetPressedInputSlot(wdInputSlotFlags::Enum mustHaveFlags, wdInputSlotFlags::Enum mustNotHaveFlags)
{
  for (wdInputSlotsMap::Iterator it = GetInternals().s_InputSlots.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_State != wdKeyState::Pressed)
      continue;

    if (it.Value().m_SlotFlags.IsAnySet(mustNotHaveFlags))
      continue;

    if (it.Value().m_SlotFlags.AreAllSet(mustHaveFlags))
      return it.Key().GetData();
  }

  return wdInputSlot_None;
}

const char* wdInputManager::GetInputSlotTouchPoint(unsigned int uiIndex)
{
  switch (uiIndex)
  {
    case 0:
      return wdInputSlot_TouchPoint0;
    case 1:
      return wdInputSlot_TouchPoint1;
    case 2:
      return wdInputSlot_TouchPoint2;
    case 3:
      return wdInputSlot_TouchPoint3;
    case 4:
      return wdInputSlot_TouchPoint4;
    case 5:
      return wdInputSlot_TouchPoint5;
    case 6:
      return wdInputSlot_TouchPoint6;
    case 7:
      return wdInputSlot_TouchPoint7;
    case 8:
      return wdInputSlot_TouchPoint8;
    case 9:
      return wdInputSlot_TouchPoint9;
    default:
      WD_REPORT_FAILURE("Maximum number of supported input touch points is 10");
      return "";
  }
}

const char* wdInputManager::GetInputSlotTouchPointPositionX(unsigned int uiIndex)
{
  switch (uiIndex)
  {
    case 0:
      return wdInputSlot_TouchPoint0_PositionX;
    case 1:
      return wdInputSlot_TouchPoint1_PositionX;
    case 2:
      return wdInputSlot_TouchPoint2_PositionX;
    case 3:
      return wdInputSlot_TouchPoint3_PositionX;
    case 4:
      return wdInputSlot_TouchPoint4_PositionX;
    case 5:
      return wdInputSlot_TouchPoint5_PositionX;
    case 6:
      return wdInputSlot_TouchPoint6_PositionX;
    case 7:
      return wdInputSlot_TouchPoint7_PositionX;
    case 8:
      return wdInputSlot_TouchPoint8_PositionX;
    case 9:
      return wdInputSlot_TouchPoint9_PositionX;
    default:
      WD_REPORT_FAILURE("Maximum number of supported input touch points is 10");
      return "";
  }
}

const char* wdInputManager::GetInputSlotTouchPointPositionY(unsigned int uiIndex)
{
  switch (uiIndex)
  {
    case 0:
      return wdInputSlot_TouchPoint0_PositionY;
    case 1:
      return wdInputSlot_TouchPoint1_PositionY;
    case 2:
      return wdInputSlot_TouchPoint2_PositionY;
    case 3:
      return wdInputSlot_TouchPoint3_PositionY;
    case 4:
      return wdInputSlot_TouchPoint4_PositionY;
    case 5:
      return wdInputSlot_TouchPoint5_PositionY;
    case 6:
      return wdInputSlot_TouchPoint6_PositionY;
    case 7:
      return wdInputSlot_TouchPoint7_PositionY;
    case 8:
      return wdInputSlot_TouchPoint8_PositionY;
    case 9:
      return wdInputSlot_TouchPoint9_PositionY;
    default:
      WD_REPORT_FAILURE("Maximum number of supported input touch points is 10");
      return "";
  }
}

WD_STATICLINK_FILE(Core, Core_Input_Implementation_InputManager);
