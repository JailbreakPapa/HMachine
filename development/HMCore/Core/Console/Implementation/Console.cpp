#include <Core/CorePCH.h>

#include <Core/Console/LuaInterpreter.h>
#include <Core/Console/QuakeConsole.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

WD_ENUMERABLE_CLASS_IMPLEMENTATION(wdConsoleFunctionBase);

wdQuakeConsole::wdQuakeConsole()
{
  ClearInputLine();

  m_bLogOutputEnabled = false;
  m_bDefaultInputHandlingInitialized = false;
  m_uiMaxConsoleStrings = 1000;

  EnableLogOutput(true);

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT
  SetCommandInterpreter(WD_DEFAULT_NEW(wdCommandInterpreterLua));
#endif
}

wdQuakeConsole::~wdQuakeConsole()
{
  EnableLogOutput(false);
}

void wdQuakeConsole::AddConsoleString(wdStringView sText, wdConsoleString::Type type)
{
  WD_LOCK(m_Mutex);

  m_ConsoleStrings.PushFront();

  wdConsoleString& cs = m_ConsoleStrings.PeekFront();
  cs.m_sText = sText;
  cs.m_Type = type;

  if (m_ConsoleStrings.GetCount() > m_uiMaxConsoleStrings)
    m_ConsoleStrings.PopBack(m_ConsoleStrings.GetCount() - m_uiMaxConsoleStrings);

  wdConsole::AddConsoleString(sText, type);
}

const wdDeque<wdConsoleString>& wdQuakeConsole::GetConsoleStrings() const
{
  if (m_bUseFilteredStrings)
  {
    return m_FilteredConsoleStrings;
  }

  return m_ConsoleStrings;
}

void wdQuakeConsole::LogHandler(const wdLoggingEventData& data)
{
  wdConsoleString::Type type = wdConsoleString::Type::Default;

  switch (data.m_EventType)
  {
    case wdLogMsgType::GlobalDefault:
    case wdLogMsgType::Flush:
    case wdLogMsgType::BeginGroup:
    case wdLogMsgType::EndGroup:
    case wdLogMsgType::None:
    case wdLogMsgType::ENUM_COUNT:
    case wdLogMsgType::All:
      return;

    case wdLogMsgType::ErrorMsg:
      type = wdConsoleString::Type::Error;
      break;

    case wdLogMsgType::SeriousWarningMsg:
      type = wdConsoleString::Type::SeriousWarning;
      break;

    case wdLogMsgType::WarningMsg:
      type = wdConsoleString::Type::Warning;
      break;

    case wdLogMsgType::SuccessMsg:
      type = wdConsoleString::Type::Success;
      break;

    case wdLogMsgType::InfoMsg:
      break;

    case wdLogMsgType::DevMsg:
      type = wdConsoleString::Type::Dev;
      break;

    case wdLogMsgType::DebugMsg:
      type = wdConsoleString::Type::Debug;
      break;
  }

  wdStringBuilder sFormat;
  sFormat.Printf("%*s", data.m_uiIndentation, "");
  sFormat.Append(data.m_sText);

  AddConsoleString(sFormat.GetData(), type);
}

void wdQuakeConsole::InputStringChanged()
{
  m_bUseFilteredStrings = false;
  m_FilteredConsoleStrings.Clear();

  if (m_sInputLine.StartsWith("*"))
  {
    wdStringBuilder input = m_sInputLine;

    input.Shrink(1, 0);
    input.Trim(" ");

    if (input.IsEmpty())
      return;

    m_FilteredConsoleStrings.Clear();
    m_bUseFilteredStrings = true;

    for (const auto& e : m_ConsoleStrings)
    {
      if (e.m_sText.FindSubString_NoCase(input))
      {
        m_FilteredConsoleStrings.PushBack(e);
      }
    }

    Scroll(0); // clamp scroll position
  }
}

void wdQuakeConsole::EnableLogOutput(bool bEnable)
{
  if (m_bLogOutputEnabled == bEnable)
    return;

  m_bLogOutputEnabled = bEnable;

  if (bEnable)
  {
    wdGlobalLog::AddLogWriter(wdMakeDelegate(&wdQuakeConsole::LogHandler, this));
  }
  else
  {
    wdGlobalLog::RemoveLogWriter(wdMakeDelegate(&wdQuakeConsole::LogHandler, this));
  }
}

void wdQuakeConsole::SaveState(wdStreamWriter& inout_stream) const
{
  WD_LOCK(m_Mutex);

  const wdUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_InputHistory.GetCount();
  for (wdUInt32 i = 0; i < m_InputHistory.GetCount(); ++i)
  {
    inout_stream << m_InputHistory[i];
  }

  inout_stream << m_BoundKeys.GetCount();
  for (auto it = m_BoundKeys.GetIterator(); it.IsValid(); ++it)
  {
    inout_stream << it.Key();
    inout_stream << it.Value();
  }
}

void wdQuakeConsole::LoadState(wdStreamReader& inout_stream)
{
  WD_LOCK(m_Mutex);

  wdUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  if (uiVersion == 1)
  {
    wdUInt32 count = 0;
    inout_stream >> count;
    m_InputHistory.SetCount(count);

    for (wdUInt32 i = 0; i < m_InputHistory.GetCount(); ++i)
    {
      inout_stream >> m_InputHistory[i];
    }

    inout_stream >> count;

    wdString sKey;
    wdString sValue;

    for (wdUInt32 i = 0; i < count; ++i)
    {
      inout_stream >> sKey;
      inout_stream >> sValue;

      m_BoundKeys[sKey] = sValue;
    }
  }
}

void wdCommandInterpreterState::AddOutputLine(const wdFormatString& text, wdConsoleString::Type type /*= wdCommandOutputLine::Type::Default*/)
{
  auto& line = m_sOutput.ExpandAndGetRef();
  line.m_Type = type;

  wdStringBuilder tmp;
  line.m_sText = text.GetText(tmp);
}

wdColor wdConsoleString::GetColor() const
{
  switch (m_Type)
  {
    case wdConsoleString::Type::Default:
      return wdColor::White;

    case wdConsoleString::Type::Error:
      return wdColor(1.0f, 0.2f, 0.2f);

    case wdConsoleString::Type::SeriousWarning:
      return wdColor(1.0f, 0.4f, 0.1f);

    case wdConsoleString::Type::Warning:
      return wdColor(1.0f, 0.6f, 0.1f);

    case wdConsoleString::Type::Note:
      return wdColor(1, 200.0f / 255.0f, 0);

    case wdConsoleString::Type::Success:
      return wdColor(0.1f, 1.0f, 0.1f);

    case wdConsoleString::Type::Executed:
      return wdColor(1.0f, 0.5f, 0.0f);

    case wdConsoleString::Type::VarName:
      return wdColorGammaUB(255, 210, 0);

    case wdConsoleString::Type::FuncName:
      return wdColorGammaUB(100, 255, 100);

    case wdConsoleString::Type::Dev:
      return wdColor(0.6f, 0.6f, 0.6f);

    case wdConsoleString::Type::Debug:
      return wdColor(0.4f, 0.6f, 0.8f);

      WD_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return wdColor::White;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

wdConsole::wdConsole() = default;

wdConsole::~wdConsole()
{
  if (s_pMainConsole == this)
  {
    s_pMainConsole = nullptr;
  }
}

void wdConsole::SetMainConsole(wdConsole* pConsole)
{
  s_pMainConsole = pConsole;
}

wdConsole* wdConsole::GetMainConsole()
{
  return s_pMainConsole;
}

wdConsole* wdConsole::s_pMainConsole = nullptr;

bool wdConsole::AutoComplete(wdStringBuilder& ref_sText)
{
  WD_LOCK(m_Mutex);

  if (m_pCommandInterpreter)
  {
    wdCommandInterpreterState s;
    s.m_sInput = ref_sText;

    m_pCommandInterpreter->AutoComplete(s);

    for (auto& l : s.m_sOutput)
    {
      AddConsoleString(l.m_sText, l.m_Type);
    }

    if (ref_sText != s.m_sInput)
    {
      ref_sText = s.m_sInput;
      return true;
    }
  }

  return false;
}

void wdConsole::ExecuteCommand(wdStringView sInput)
{
  if (sInput.IsEmpty())
    return;

  WD_LOCK(m_Mutex);

  if (m_pCommandInterpreter)
  {
    wdCommandInterpreterState s;
    s.m_sInput = sInput;
    m_pCommandInterpreter->Interpret(s);

    for (auto& l : s.m_sOutput)
    {
      AddConsoleString(l.m_sText, l.m_Type);
    }
  }
  else
  {
    AddConsoleString(sInput);
  }
}

void wdConsole::AddConsoleString(wdStringView sText, wdConsoleString::Type type /*= wdConsoleString::Type::Default*/)
{
  wdConsoleString cs;
  cs.m_sText = sText;
  cs.m_Type = type;

  // Broadcast that we have added a string to the console
  wdConsoleEvent e;
  e.m_Type = wdConsoleEvent::Type::OutputLineAdded;
  e.m_AddedpConsoleString = &cs;

  m_Events.Broadcast(e);
}

void wdConsole::AddToInputHistory(wdStringView sText)
{
  WD_LOCK(m_Mutex);

  m_iCurrentInputHistoryElement = -1;

  if (sText.IsEmpty())
    return;

  for (wdInt32 i = 0; i < (wdInt32)m_InputHistory.GetCount(); i++)
  {
    if (m_InputHistory[i] == sText) // already in the History
    {
      // just move it to the front

      for (wdInt32 j = i - 1; j >= 0; j--)
        m_InputHistory[j + 1] = m_InputHistory[j];

      m_InputHistory[0] = sText;
      return;
    }
  }

  m_InputHistory.SetCount(wdMath::Min<wdUInt32>(m_InputHistory.GetCount() + 1, m_InputHistory.GetCapacity()));

  for (wdUInt32 i = m_InputHistory.GetCount() - 1; i > 0; i--)
    m_InputHistory[i] = m_InputHistory[i - 1];

  m_InputHistory[0] = sText;
}

void wdConsole::RetrieveInputHistory(wdInt32 iHistoryUp, wdStringBuilder& ref_sResult)
{
  WD_LOCK(m_Mutex);

  if (m_InputHistory.IsEmpty())
    return;

  m_iCurrentInputHistoryElement = wdMath::Clamp<wdInt32>(m_iCurrentInputHistoryElement + iHistoryUp, 0, m_InputHistory.GetCount() - 1);

  if (!m_InputHistory[m_iCurrentInputHistoryElement].IsEmpty())
  {
    ref_sResult = m_InputHistory[m_iCurrentInputHistoryElement];
  }
}

wdResult wdConsole::SaveInputHistory(wdStringView sFile)
{
  wdFileWriter file;
  WD_SUCCEED_OR_RETURN(file.Open(sFile));

  wdStringBuilder str;

  for (const wdString& line : m_InputHistory)
  {
    if (line.IsEmpty())
      continue;

    str.Set(line, "\n");

    WD_SUCCEED_OR_RETURN(file.WriteBytes(str.GetData(), str.GetElementCount()));
  }

  return WD_SUCCESS;
}

void wdConsole::LoadInputHistory(wdStringView sFile)
{
  wdFileReader file;
  if (file.Open(sFile).Failed())
    return;

  wdStringBuilder str;
  str.ReadAll(file);

  wdHybridArray<wdStringView, 32> lines;
  str.Split(false, lines, "\n", "\r");

  for (wdUInt32 i = 0; i < lines.GetCount(); ++i)
  {
    AddToInputHistory(lines[lines.GetCount() - 1 - i]);
  }
}

WD_STATICLINK_FILE(Core, Core_Console_Implementation_Console);
