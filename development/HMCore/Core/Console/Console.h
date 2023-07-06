#pragma once

#include <Core/Console/ConsoleFunction.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>

struct WD_CORE_DLL wdConsoleString
{
  enum class Type : wdUInt8
  {
    Default,
    Error,
    SeriousWarning,
    Warning,
    Note,
    Success,
    Executed,
    VarName,
    FuncName,
    Dev,
    Debug,
  };

  Type m_Type = Type::Default;
  wdString m_sText;
  wdColor GetColor() const;

  bool operator<(const wdConsoleString& rhs) const { return m_sText < rhs.m_sText; }
};

struct WD_CORE_DLL wdCommandInterpreterState
{
  wdStringBuilder m_sInput;
  wdHybridArray<wdConsoleString, 16> m_sOutput;

  void AddOutputLine(const wdFormatString& text, wdConsoleString::Type type = wdConsoleString::Type::Default);
};

class WD_CORE_DLL wdCommandInterpreter : public wdRefCounted
{
public:
  virtual void Interpret(wdCommandInterpreterState& inout_state) = 0;

  virtual void AutoComplete(wdCommandInterpreterState& inout_state);

  /// \brief Iterates over all cvars and finds all that start with the string \a szVariable.
  static void FindPossibleCVars(wdStringView sVariable, wdDeque<wdString>& ref_commonStrings, wdDeque<wdConsoleString>& ref_consoleStrings);

  /// \brief Iterates over all console functions and finds all that start with the string \a szVariable.
  static void FindPossibleFunctions(wdStringView sVariable, wdDeque<wdString>& ref_commonStrings, wdDeque<wdConsoleString>& ref_consoleStrings);

  /// \brief Returns the prefix string that is common to all strings in the \a vStrings array.
  static const wdString FindCommonString(const wdDeque<wdString>& strings);
};

/// \brief The event data that is broadcast by the console
struct wdConsoleEvent
{
  enum class Type : wdInt32
  {
    OutputLineAdded, ///< A string was added to the console
  };

  Type m_Type;

  /// \brief The console string that was just added.
  const wdConsoleString* m_AddedpConsoleString;
};

class WD_CORE_DLL wdConsole
{
public:
  wdConsole();
  virtual ~wdConsole();

  /// \name Events
  /// @{

public:
  /// \brief Grants access to subscribe and unsubscribe from console events.
  const wdEvent<const wdConsoleEvent&>& Events() const { return m_Events; }

protected:
  /// \brief The console event variable, to attach to.
  wdEvent<const wdConsoleEvent&> m_Events;

  /// @}

  /// \name Helpers
  /// @{

public:
  /// \brief Returns the mutex that's used to prevent multi-threaded access
  wdMutex& GetMutex() const { return m_Mutex; }

  static void SetMainConsole(wdConsole* pConsole);
  static wdConsole* GetMainConsole();

protected:
  mutable wdMutex m_Mutex;

private:
  static wdConsole* s_pMainConsole;

  /// @}

  /// \name Command Interpreter
  /// @{

public:
  /// \brief Replaces the current command interpreter.
  ///
  /// This base class doesn't set any default interpreter, but derived classes may do so.
  void SetCommandInterpreter(const wdSharedPtr<wdCommandInterpreter>& pInterpreter) { m_pCommandInterpreter = pInterpreter; }

  /// \brief Returns the currently used command interpreter.
  const wdSharedPtr<wdCommandInterpreter>& GetCommandInterpreter() const { return m_pCommandInterpreter; }

  /// \brief Auto-completes the given text.
  ///
  /// Returns true, if the string was modified in any way.
  /// Adds additional strings to the console output, if there are further auto-completion suggestions.
  virtual bool AutoComplete(wdStringBuilder& ref_sText);

  /// \brief Executes the given input string.
  ///
  /// The command is forwarded to the set command interpreter.
  virtual void ExecuteCommand(wdStringView sInput);

protected:
  wdSharedPtr<wdCommandInterpreter> m_pCommandInterpreter;

  /// @}

  /// \name Console Display
  /// @{

public:
  /// \brief Adds a string to the console.
  ///
  /// The base class only broadcasts an event, but does not store the string anywhere.
  virtual void AddConsoleString(wdStringView sText, wdConsoleString::Type type = wdConsoleString::Type::Default);

  /// @}

  /// \name Input History
  /// @{

public:
  /// \brief Adds an item to the input history.
  void AddToInputHistory(wdStringView sText);

  /// \brief Returns the current input history.
  ///
  /// Make sure to lock the console's mutex while working with the history.
  const wdStaticArray<wdString, 16>& GetInputHistory() const { return m_InputHistory; }

  /// \brief Replaces the input line by the next (or previous) history item.
  void RetrieveInputHistory(wdInt32 iHistoryUp, wdStringBuilder& ref_sResult);

  /// \brief Writes the current input history to a text file.
  wdResult SaveInputHistory(wdStringView sFile);

  /// \brief Reads the text file and appends all lines to the input history.
  void LoadInputHistory(wdStringView sFile);

protected:
  wdInt32 m_iCurrentInputHistoryElement = -1;
  wdStaticArray<wdString, 16> m_InputHistory;

  /// @}
};
