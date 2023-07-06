#include <Core/CorePCH.h>

#include <Core/Console/QuakeConsole.h>
#include <Foundation/Configuration/CVar.h>

void wdQuakeConsole::ExecuteCommand(wdStringView sInput)
{
  const bool bBind = sInput.StartsWith_NoCase("bind ");
  const bool bUnbind = sInput.StartsWith_NoCase("unbind ");

  if (bBind || bUnbind)
  {
    wdStringBuilder tmp;
    const char* szAfterCmd = wdStringUtils::FindWordEnd(sInput.GetData(tmp), wdStringUtils::IsWhiteSpace); // skip the word 'bind' or 'unbind'

    const char* szKeyNameStart = wdStringUtils::SkipCharacters(szAfterCmd, wdStringUtils::IsWhiteSpace);                // go to the next word
    const char* szKeyNameEnd = wdStringUtils::FindWordEnd(szKeyNameStart, wdStringUtils::IsIdentifierDelimiter_C_Code); // find its end

    wdStringView sKey(szKeyNameStart, szKeyNameEnd);
    tmp = sKey; // copy the word into a zero terminated string

    const char* szCommandToBind = wdStringUtils::SkipCharacters(szKeyNameEnd, wdStringUtils::IsWhiteSpace);

    if (bUnbind || wdStringUtils::IsNullOrEmpty(szCommandToBind))
    {
      UnbindKey(tmp);
      return;
    }

    BindKey(tmp, szCommandToBind);
    return;
  }

  wdConsole::ExecuteCommand(sInput);
}

void wdQuakeConsole::BindKey(const char* szKey, const char* szCommand)
{
  wdStringBuilder s;
  s.Format("Binding key '{0}' to command '{1}'", szKey, szCommand);
  AddConsoleString(s, wdConsoleString::Type::Success);

  m_BoundKeys[szKey] = szCommand;
}

void wdQuakeConsole::UnbindKey(const char* szKey)
{
  wdStringBuilder s;
  s.Format("Unbinding key '{0}'", szKey);
  AddConsoleString(s, wdConsoleString::Type::Success);

  m_BoundKeys.Remove(szKey);
}

void wdQuakeConsole::ExecuteBoundKey(const char* szKey)
{
  auto it = m_BoundKeys.Find(szKey);

  if (it.IsValid())
  {
    ExecuteCommand(it.Value().GetData());
  }
}



WD_STATICLINK_FILE(Core, Core_Console_Implementation_Commands);
