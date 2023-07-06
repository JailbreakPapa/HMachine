#include <Core/CorePCH.h>

#include <Core/Console/QuakeConsole.h>
#include <Core/Input/InputManager.h>

bool wdQuakeConsole::ProcessInputCharacter(wdUInt32 uiChar)
{
  switch (uiChar)
  {
    case 27: // Escape
      ClearInputLine();
      return false;

    case '\b': // backspace
    {
      if (!m_sInputLine.IsEmpty() && m_iCaretPosition > 0)
      {
        RemoveCharacter(m_iCaretPosition - 1);
        MoveCaret(-1);
      }
    }
      return false;

    case '\t':
      if (AutoComplete(m_sInputLine))
      {
        MoveCaret(500);
      }
      return false;

    case 13: // Enter
      AddToInputHistory(m_sInputLine);
      ExecuteCommand(m_sInputLine);
      ClearInputLine();
      return false;
  }

  return true;
}

bool wdQuakeConsole::FilterInputCharacter(wdUInt32 uiChar)
{
  // filter out not only all non-ASCII characters, but also all the non-printable ASCII characters
  // if you want to support full Unicode characters in the console, override this function and change this restriction
  if (uiChar < 32 || uiChar > 126)
    return false;

  return true;
}

void wdQuakeConsole::ClampCaretPosition()
{
  m_iCaretPosition = wdMath::Clamp<wdInt32>(m_iCaretPosition, 0, m_sInputLine.GetCharacterCount());
}

void wdQuakeConsole::MoveCaret(wdInt32 iMoveOffset)
{
  m_iCaretPosition += iMoveOffset;

  ClampCaretPosition();
}

void wdQuakeConsole::Scroll(wdInt32 iLines)
{
  if (m_bUseFilteredStrings)
    m_iScrollPosition = wdMath::Clamp<wdInt32>(m_iScrollPosition + iLines, 0, wdMath::Max<wdInt32>(m_FilteredConsoleStrings.GetCount() - 10, 0));
  else
    m_iScrollPosition = wdMath::Clamp<wdInt32>(m_iScrollPosition + iLines, 0, wdMath::Max<wdInt32>(m_ConsoleStrings.GetCount() - 10, 0));
}

void wdQuakeConsole::ClearInputLine()
{
  m_sInputLine.Clear();
  m_iCaretPosition = 0;
  m_iScrollPosition = 0;
  m_iCurrentInputHistoryElement = -1;

  m_FilteredConsoleStrings.Clear();
  m_bUseFilteredStrings = false;

  InputStringChanged();
}

void wdQuakeConsole::ClearConsoleStrings()
{
  m_ConsoleStrings.Clear();
  m_FilteredConsoleStrings.Clear();
  m_bUseFilteredStrings = false;
  m_iScrollPosition = 0;
}

void wdQuakeConsole::DeleteNextCharacter()
{
  RemoveCharacter(m_iCaretPosition);
}

void wdQuakeConsole::RemoveCharacter(wdUInt32 uiInputLinePosition)
{
  if (uiInputLinePosition >= m_sInputLine.GetCharacterCount())
    return;

  auto it = m_sInputLine.GetIteratorFront();
  it += uiInputLinePosition;

  auto itNext = it;
  ++itNext;

  m_sInputLine.Remove(it.GetData(), itNext.GetData());

  InputStringChanged();
}

void wdQuakeConsole::AddInputCharacter(wdUInt32 uiChar)
{
  if (uiChar == '\0')
    return;

  if (!ProcessInputCharacter(uiChar))
    return;

  if (!FilterInputCharacter(uiChar))
    return;

  ClampCaretPosition();

  auto it = m_sInputLine.GetIteratorFront();
  it += m_iCaretPosition;

  wdUInt32 uiString[2] = {uiChar, 0};

  m_sInputLine.Insert(it.GetData(), wdStringUtf8(uiString).GetData());

  MoveCaret(1);

  InputStringChanged();
}

void wdQuakeConsole::DoDefaultInputHandling(bool bConsoleOpen)
{
  if (!m_bDefaultInputHandlingInitialized)
  {
    m_bDefaultInputHandlingInitialized = true;

    wdInputActionConfig cfg;
    cfg.m_bApplyTimeScaling = true;

    cfg.m_sInputSlotTrigger[0] = wdInputSlot_KeyLeft;
    wdInputManager::SetInputActionConfig("Console", "MoveCaretLeft", cfg, true);

    cfg.m_sInputSlotTrigger[0] = wdInputSlot_KeyRight;
    wdInputManager::SetInputActionConfig("Console", "MoveCaretRight", cfg, true);

    cfg.m_sInputSlotTrigger[0] = wdInputSlot_KeyHome;
    wdInputManager::SetInputActionConfig("Console", "MoveCaretStart", cfg, true);

    cfg.m_sInputSlotTrigger[0] = wdInputSlot_KeyEnd;
    wdInputManager::SetInputActionConfig("Console", "MoveCaretEnd", cfg, true);

    cfg.m_sInputSlotTrigger[0] = wdInputSlot_KeyDelete;
    wdInputManager::SetInputActionConfig("Console", "DeleteCharacter", cfg, true);

    cfg.m_sInputSlotTrigger[0] = wdInputSlot_KeyPageUp;
    wdInputManager::SetInputActionConfig("Console", "ScrollUp", cfg, true);

    cfg.m_sInputSlotTrigger[0] = wdInputSlot_KeyPageDown;
    wdInputManager::SetInputActionConfig("Console", "ScrollDown", cfg, true);

    cfg.m_sInputSlotTrigger[0] = wdInputSlot_KeyUp;
    wdInputManager::SetInputActionConfig("Console", "HistoryUp", cfg, true);

    cfg.m_sInputSlotTrigger[0] = wdInputSlot_KeyDown;
    wdInputManager::SetInputActionConfig("Console", "HistoryDown", cfg, true);

    cfg.m_sInputSlotTrigger[0] = wdInputSlot_KeyF2;
    wdInputManager::SetInputActionConfig("Console", "RepeatLast", cfg, true);

    cfg.m_sInputSlotTrigger[0] = wdInputSlot_KeyF3;
    wdInputManager::SetInputActionConfig("Console", "RepeatSecondLast", cfg, true);

    return;
  }

  if (bConsoleOpen)
  {
    if (wdInputManager::GetInputActionState("Console", "MoveCaretLeft") == wdKeyState::Pressed)
      MoveCaret(-1);
    if (wdInputManager::GetInputActionState("Console", "MoveCaretRight") == wdKeyState::Pressed)
      MoveCaret(1);
    if (wdInputManager::GetInputActionState("Console", "MoveCaretStart") == wdKeyState::Pressed)
      MoveCaret(-1000);
    if (wdInputManager::GetInputActionState("Console", "MoveCaretEnd") == wdKeyState::Pressed)
      MoveCaret(1000);
    if (wdInputManager::GetInputActionState("Console", "DeleteCharacter") == wdKeyState::Pressed)
      DeleteNextCharacter();
    if (wdInputManager::GetInputActionState("Console", "ScrollUp") == wdKeyState::Pressed)
      Scroll(10);
    if (wdInputManager::GetInputActionState("Console", "ScrollDown") == wdKeyState::Pressed)
      Scroll(-10);
    if (wdInputManager::GetInputActionState("Console", "HistoryUp") == wdKeyState::Pressed)
    {
      RetrieveInputHistory(1, m_sInputLine);
      m_iCaretPosition = m_sInputLine.GetCharacterCount();
    }
    if (wdInputManager::GetInputActionState("Console", "HistoryDown") == wdKeyState::Pressed)
    {
      RetrieveInputHistory(-1, m_sInputLine);
      m_iCaretPosition = m_sInputLine.GetCharacterCount();
    }

    const wdUInt32 uiChar = wdInputManager::RetrieveLastCharacter();

    if (uiChar != '\0')
      AddInputCharacter(uiChar);
  }
  else
  {
    const wdUInt32 uiChar = wdInputManager::RetrieveLastCharacter(false);

    char szCmd[16] = "";
    char* szIterator = szCmd;
    wdUnicodeUtils::EncodeUtf32ToUtf8(uiChar, szIterator);
    *szIterator = '\0';
    ExecuteBoundKey(szCmd);
  }

  if (wdInputManager::GetInputActionState("Console", "RepeatLast") == wdKeyState::Pressed)
  {
    if (GetInputHistory().GetCount() >= 1)
      ExecuteCommand(GetInputHistory()[0]);
  }

  if (wdInputManager::GetInputActionState("Console", "RepeatSecondLast") == wdKeyState::Pressed)
  {
    if (GetInputHistory().GetCount() >= 2)
      ExecuteCommand(GetInputHistory()[1]);
  }
}



WD_STATICLINK_FILE(Core, Core_Console_Implementation_Input);
