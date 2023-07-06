#include <Core/CorePCH.h>

#include <Core/Input/InputManager.h>

wdInputActionConfig::wdInputActionConfig()
{
  m_bApplyTimeScaling = true;

  m_fFilterXMinValue = 0.0f;
  m_fFilterXMaxValue = 1.0f;
  m_fFilterYMinValue = 0.0f;
  m_fFilterYMaxValue = 1.0f;

  m_fFilteredPriority = -10000.0f;

  m_OnLeaveArea = LoseFocus;
  m_OnEnterArea = ActivateImmediately;

  for (wdInt32 i = 0; i < MaxInputSlotAlternatives; ++i)
  {
    m_fInputSlotScale[i] = 1.0f;
    m_sInputSlotTrigger[i] = wdInputSlot_None;
    m_sFilterByInputSlotX[i] = wdInputSlot_None;
    m_sFilterByInputSlotY[i] = wdInputSlot_None;
  }
}

wdInputManager::wdActionData::wdActionData()
{
  m_fValue = 0.0f;
  m_State = wdKeyState::Up;
  m_iTriggeredViaAlternative = -1;
}

void wdInputManager::ClearInputMapping(const char* szInputSet, const char* szInputSlot)
{
  wdActionMap& Actions = GetInternals().s_ActionMapping[szInputSet];

  // iterate over all existing actions
  for (wdActionMap::Iterator it = Actions.GetIterator(); it.IsValid(); ++it)
  {
    // iterate over all input slots in the existing action
    for (wdUInt32 i1 = 0; i1 < wdInputActionConfig::MaxInputSlotAlternatives; ++i1)
    {
      // if that action is triggered by the given input slot, remove that trigger from the action

      if (it.Value().m_Config.m_sInputSlotTrigger[i1] == szInputSlot)
        it.Value().m_Config.m_sInputSlotTrigger[i1].Clear();
    }
  }
}

void wdInputManager::SetInputActionConfig(
  const char* szInputSet, const char* szAction, const wdInputActionConfig& config, bool bClearPreviousInputMappings)
{
  WD_ASSERT_DEV(!wdStringUtils::IsNullOrEmpty(szInputSet), "The InputSet name must not be empty.");
  WD_ASSERT_DEV(!wdStringUtils::IsNullOrEmpty(szAction), "No input action to map to was given.");

  if (bClearPreviousInputMappings)
  {
    for (wdUInt32 i1 = 0; i1 < wdInputActionConfig::MaxInputSlotAlternatives; ++i1)
      ClearInputMapping(szInputSet, config.m_sInputSlotTrigger[i1].GetData());
  }

  // store the new action mapping
  wdInputManager::wdActionData& ad = GetInternals().s_ActionMapping[szInputSet][szAction];
  ad.m_Config = config;

  InputEventData e;
  e.m_EventType = InputEventData::InputActionChanged;
  e.m_szInputSet = szInputSet;
  e.m_szInputAction = szAction;

  s_InputEvents.Broadcast(e);
}

wdInputActionConfig wdInputManager::GetInputActionConfig(const char* szInputSet, const char* szAction)
{
  const wdInputSetMap::ConstIterator ItSet = GetInternals().s_ActionMapping.Find(szInputSet);

  if (!ItSet.IsValid())
    return wdInputActionConfig();

  const wdActionMap::ConstIterator ItAction = ItSet.Value().Find(szAction);

  if (!ItAction.IsValid())
    return wdInputActionConfig();

  return ItAction.Value().m_Config;
}

void wdInputManager::RemoveInputAction(const char* szInputSet, const char* szAction)
{
  GetInternals().s_ActionMapping[szInputSet].Remove(szAction);
}

wdKeyState::Enum wdInputManager::GetInputActionState(const char* szInputSet, const char* szAction, float* pValue, wdInt8* pTriggeredSlot)
{
  if (pValue)
    *pValue = 0.0f;

  if (pTriggeredSlot)
    *pTriggeredSlot = -1;

  if (!s_sExclusiveInputSet.IsEmpty() && s_sExclusiveInputSet != szInputSet)
    return wdKeyState::Up;

  const wdInputSetMap::ConstIterator ItSet = GetInternals().s_ActionMapping.Find(szInputSet);

  if (!ItSet.IsValid())
    return wdKeyState::Up;

  const wdActionMap::ConstIterator ItAction = ItSet.Value().Find(szAction);

  if (!ItAction.IsValid())
    return wdKeyState::Up;

  if (pValue)
    *pValue = ItAction.Value().m_fValue;

  if (pTriggeredSlot)
    *pTriggeredSlot = ItAction.Value().m_iTriggeredViaAlternative;

  return ItAction.Value().m_State;
}

wdInputManager::wdActionMap::Iterator wdInputManager::GetBestAction(wdActionMap& Actions, const wdString& sSlot, const wdActionMap::Iterator& itFirst)
{
  // this function determines which input action should be triggered by the given input slot
  // it will prefer actions with higher priority
  // it will check that all conditions of the action are met (ie. filters like that a mouse cursor is inside a rectangle)
  // if some action had focus before and shall keep it until some key is released, that action will always be preferred

  wdActionMap::Iterator ItAction = itFirst;

  wdActionMap::Iterator itBestAction;
  float fBestPriority = -1000000;

  if (ItAction.IsValid())
  {
    // take the priority of the last returned value as the basis to compare all other actions against
    fBestPriority = ItAction.Value().m_Config.m_fFilteredPriority;

    // and make sure to skip the last returned action, of course
    ++ItAction;
  }
  else
  {
    // if an invalid iterator is passed in, this is the first call to this function, start searching at the beginning
    ItAction = Actions.GetIterator();
  }

  // check all actions from the given array
  for (; ItAction.IsValid(); ++ItAction)
  {
    wdActionData& ThisAction = ItAction.Value();

    wdInt8 AltSlot = ThisAction.m_iTriggeredViaAlternative;

    if (AltSlot >= 0)
    {
      if (ThisAction.m_Config.m_sInputSlotTrigger[AltSlot] == sSlot)
        goto hell;
    }
    else
    {
      // if the given slot triggers this action (or any of its alternative slots), continue
      for (AltSlot = 0; AltSlot < wdInputActionConfig::MaxInputSlotAlternatives; ++AltSlot)
      {
        if (ThisAction.m_Config.m_sInputSlotTrigger[AltSlot] == sSlot)
          goto hell;
      }
    }

    // if the action is not triggered by this slot, skip it
    continue;

  hell:

    WD_ASSERT_DEV(AltSlot >= 0 && AltSlot < wdInputActionConfig::MaxInputSlotAlternatives, "Alternate Slot out of bounds.");

    // if the action had input in the last update AND wants to keep the focus, it will ALWAYS get the input, until the input slot gets
    // inactive (key up) independent from priority, overlap of areas etc.
    if (ThisAction.m_State != wdKeyState::Up && ThisAction.m_Config.m_OnLeaveArea == wdInputActionConfig::KeepFocus)
    {
      // just return this result immediately
      return ItAction;
    }

    // if this action has lower priority than what we already found, ignore it
    if (ThisAction.m_Config.m_fFilteredPriority < fBestPriority)
      continue;

    // if it has the same priority but we already found one, also ignore it
    // if it has the same priority but we did not yet find a 'best action' take this one
    if (ThisAction.m_Config.m_fFilteredPriority == fBestPriority && itBestAction.IsValid())
      continue;

    // this is the "mouse cursor filter" for the x-axis
    // if any filter is set, check that it is in range
    if (!ThisAction.m_Config.m_sFilterByInputSlotX[AltSlot].IsEmpty())
    {
      const float fVal = GetInternals().s_InputSlots[ThisAction.m_Config.m_sFilterByInputSlotX[AltSlot]].m_fValue;
      if (fVal < ThisAction.m_Config.m_fFilterXMinValue || fVal > ThisAction.m_Config.m_fFilterXMaxValue)
        continue;
    }

    // this is the "mouse cursor filter" for the y-axis
    // if any filter is set, check that it is in range
    if (!ThisAction.m_Config.m_sFilterByInputSlotY[AltSlot].IsEmpty())
    {
      const float fVal = GetInternals().s_InputSlots[ThisAction.m_Config.m_sFilterByInputSlotY[AltSlot]].m_fValue;
      if (fVal < ThisAction.m_Config.m_fFilterYMinValue || fVal > ThisAction.m_Config.m_fFilterYMaxValue)
        continue;
    }

    // we found something!
    fBestPriority = ThisAction.m_Config.m_fFilteredPriority;
    itBestAction = ItAction;

    ThisAction.m_iTriggeredViaAlternative = AltSlot;
  }

  return itBestAction;
}

void wdInputManager::UpdateInputActions(wdTime tTimeDifference)
{
  // update each input set
  // all input sets are disjunct from each other, so one key press can have different effects in each input set
  for (wdInputSetMap::Iterator ItSets = GetInternals().s_ActionMapping.GetIterator(); ItSets.IsValid(); ++ItSets)
  {
    UpdateInputActions(ItSets.Key().GetData(), ItSets.Value(), tTimeDifference);
  }
}

void wdInputManager::UpdateInputActions(const char* szInputSet, wdActionMap& Actions, wdTime tTimeDifference)
{
  // reset all action values to zero
  for (wdActionMap::Iterator ItActions = Actions.GetIterator(); ItActions.IsValid(); ++ItActions)
    ItActions.Value().m_fValue = 0.0f;

  // iterate over all input slots and check how their values affect the actions from the current input set
  for (wdInputSlotsMap::Iterator ItSlots = GetInternals().s_InputSlots.GetIterator(); ItSlots.IsValid(); ++ItSlots)
  {
    // if this input slot is not active, ignore it; we will reset all actions later
    if (ItSlots.Value().m_fValue == 0.0f)
      continue;

    // If this key got clicked in this frame, it has not been dragged into the active area of the action
    // e.g. the mouse has been clicked while it was inside this area, instead of outside and then moved here
    const bool bFreshClick = ItSlots.Value().m_State == wdKeyState::Pressed;

    // find the action that should be affected by this input slot
    wdActionMap::Iterator itBestAction;

    // we activate all actions with the same priority simultaneously
    while (true)
    {
      // get the (next) best action
      itBestAction = GetBestAction(Actions, ItSlots.Key(), itBestAction);

      // if we found anything, update its input
      if (!itBestAction.IsValid())
        break;

      const float fSlotScale = itBestAction.Value().m_Config.m_fInputSlotScale[(wdUInt32)(itBestAction.Value().m_iTriggeredViaAlternative)];

      float fSlotValue = ItSlots.Value().m_fValue;

      if (fSlotScale >= 0.0f)
        fSlotValue *= fSlotScale;
      else
        fSlotValue = wdMath::Pow(fSlotValue, -fSlotScale);

      if ((!ItSlots.Value().m_SlotFlags.IsAnySet(wdInputSlotFlags::NeverTimeScale)) && (itBestAction.Value().m_Config.m_bApplyTimeScaling))
        fSlotValue *= (float)tTimeDifference.GetSeconds();

      const float fNewValue = wdMath::Max(itBestAction.Value().m_fValue, fSlotValue);

      if (itBestAction.Value().m_Config.m_OnEnterArea == wdInputActionConfig::RequireKeyUp)
      {
        // if this action requires that it is only activated by a key press while the mouse is inside it
        // we check whether this is either a fresh click (inside the area) or the action is already active
        // if it is already active, the mouse is most likely held clicked at the moment

        if (bFreshClick || (itBestAction.Value().m_fValue > 0.0f) || (itBestAction.Value().m_State == wdKeyState::Pressed) ||
            (itBestAction.Value().m_State == wdKeyState::Down))
          itBestAction.Value().m_fValue = fNewValue;
      }
      else
        itBestAction.Value().m_fValue = fNewValue;
    }
  }

  // now update all action states, if any one has not gotten any input from any input slot recently, it will be reset to 'Released' or 'Up'
  for (wdActionMap::Iterator ItActions = Actions.GetIterator(); ItActions.IsValid(); ++ItActions)
  {
    const bool bHasInput = ItActions.Value().m_fValue > 0.0f;
    const wdKeyState::Enum NewState = wdKeyState::GetNewKeyState(ItActions.Value().m_State, bHasInput);

    if ((NewState != wdKeyState::Up) || (NewState != ItActions.Value().m_State))
    {
      ItActions.Value().m_State = NewState;

      InputEventData e;
      e.m_EventType = InputEventData::InputActionChanged;
      e.m_szInputSet = szInputSet;
      e.m_szInputAction = ItActions.Key().GetData();

      s_InputEvents.Broadcast(e);
    }

    if (NewState == wdKeyState::Up)
      ItActions.Value().m_iTriggeredViaAlternative = -1;
  }
}

void wdInputManager::SetActionDisplayName(const char* szAction, const char* szDisplayName)
{
  GetInternals().s_ActionDisplayNames[szAction] = szDisplayName;
}

const wdString wdInputManager::GetActionDisplayName(const char* szAction)
{
  return GetInternals().s_ActionDisplayNames.GetValueOrDefault(szAction, szAction);
}

void wdInputManager::GetAllInputSets(wdDynamicArray<wdString>& out_inputSetNames)
{
  out_inputSetNames.Clear();

  for (wdInputSetMap::Iterator it = GetInternals().s_ActionMapping.GetIterator(); it.IsValid(); ++it)
    out_inputSetNames.PushBack(it.Key());
}

void wdInputManager::GetAllInputActions(const char* szInputSetName, wdDynamicArray<wdString>& out_inputActions)
{
  const auto& map = GetInternals().s_ActionMapping[szInputSetName];

  out_inputActions.Clear();
  out_inputActions.Reserve(map.GetCount());

  for (wdActionMap::ConstIterator it = map.GetIterator(); it.IsValid(); ++it)
    out_inputActions.PushBack(it.Key());
}



WD_STATICLINK_FILE(Core, Core_Input_Implementation_Action);
