#include <Core/CorePCH.h>

#include <Core/Console/Console.h>
#include <Core/Console/QuakeConsole.h>

void wdCommandInterpreter::FindPossibleCVars(wdStringView sVariable, wdDeque<wdString>& inout_autoCompleteOptions, wdDeque<wdConsoleString>& inout_autoCompleteDescriptions)
{
  wdStringBuilder sText;

  wdCVar* pCVar = wdCVar::GetFirstInstance();
  while (pCVar)
  {
    if (pCVar->GetName().StartsWith_NoCase(sVariable))
    {
      sText.Format("    {0} = {1}", pCVar->GetName(), wdQuakeConsole::GetFullInfoAsString(pCVar));

      wdConsoleString cs;
      cs.m_sText = sText;
      cs.m_Type = wdConsoleString::Type::VarName;
      inout_autoCompleteDescriptions.PushBack(cs);

      inout_autoCompleteOptions.PushBack(pCVar->GetName());
    }

    pCVar = pCVar->GetNextInstance();
  }
}

void wdCommandInterpreter::FindPossibleFunctions(wdStringView sVariable, wdDeque<wdString>& inout_autoCompleteOptions, wdDeque<wdConsoleString>& inout_autoCompleteDescriptions)
{
  wdStringBuilder sText;

  wdConsoleFunctionBase* pFunc = wdConsoleFunctionBase::GetFirstInstance();
  while (pFunc)
  {
    if (pFunc->GetName().StartsWith_NoCase(sVariable))
    {
      sText.Format("    {0} {1}", pFunc->GetName(), pFunc->GetDescription());

      wdConsoleString cs;
      cs.m_sText = sText;
      cs.m_Type = wdConsoleString::Type::FuncName;
      inout_autoCompleteDescriptions.PushBack(cs);

      inout_autoCompleteOptions.PushBack(pFunc->GetName());
    }

    pFunc = pFunc->GetNextInstance();
  }
}


const wdString wdQuakeConsole::GetValueAsString(wdCVar* pCVar)
{
  wdStringBuilder s = "undefined";

  switch (pCVar->GetType())
  {
    case wdCVarType::Int:
    {
      wdCVarInt* pInt = static_cast<wdCVarInt*>(pCVar);
      s.Format("{0}", pInt->GetValue());
    }
    break;

    case wdCVarType::Bool:
    {
      wdCVarBool* pBool = static_cast<wdCVarBool*>(pCVar);
      if (pBool->GetValue() == true)
        s = "true";
      else
        s = "false";
    }
    break;

    case wdCVarType::String:
    {
      wdCVarString* pString = static_cast<wdCVarString*>(pCVar);
      s.Format("\"{0}\"", pString->GetValue());
    }
    break;

    case wdCVarType::Float:
    {
      wdCVarFloat* pFloat = static_cast<wdCVarFloat*>(pCVar);
      s.Format("{0}", wdArgF(pFloat->GetValue(), 3));
    }
    break;

    case wdCVarType::ENUM_COUNT:
      break;
  }

  return s.GetData();
}

wdString wdQuakeConsole::GetFullInfoAsString(wdCVar* pCVar)
{
  wdStringBuilder s = GetValueAsString(pCVar);

  const bool bAnyFlags = pCVar->GetFlags().IsAnySet(wdCVarFlags::RequiresRestart | wdCVarFlags::Save);

  if (bAnyFlags)
    s.Append(" [ ");

  if (pCVar->GetFlags().IsAnySet(wdCVarFlags::Save))
    s.Append("SAVE ");

  if (pCVar->GetFlags().IsAnySet(wdCVarFlags::RequiresRestart))
    s.Append("RESTART ");

  if (bAnyFlags)
    s.Append("]");

  return s;
}

const wdString wdCommandInterpreter::FindCommonString(const wdDeque<wdString>& strings)
{
  wdStringBuilder sCommon;
  wdUInt32 c;

  wdUInt32 uiPos = 0;
  auto it1 = strings[0].GetIteratorFront();
  while (it1.IsValid())
  {
    c = it1.GetCharacter();

    for (int v = 1; v < (int)strings.GetCount(); v++)
    {
      auto it2 = strings[v].GetIteratorFront();

      it2 += uiPos;

      if (it2.GetCharacter() != c)
        return sCommon;
    }

    sCommon.Append(c);

    ++uiPos;
    ++it1;
  }

  return sCommon;
}

void wdCommandInterpreter::AutoComplete(wdCommandInterpreterState& inout_state)
{
  wdString sVarName = inout_state.m_sInput;

  auto it = rbegin(inout_state.m_sInput);

  // dots are allowed in CVar names
  while (it.IsValid() && (it.GetCharacter() == '.' || !wdStringUtils::IsIdentifierDelimiter_C_Code(*it)))
    ++it;

  const char* szLastWordDelimiter = nullptr;
  if (it.IsValid() && wdStringUtils::IsIdentifierDelimiter_C_Code(*it) && it.GetCharacter() != '.')
    szLastWordDelimiter = it.GetData();

  if (szLastWordDelimiter != nullptr)
    sVarName = szLastWordDelimiter + 1;

  wdDeque<wdString> AutoCompleteOptions;
  wdDeque<wdConsoleString> AutoCompleteDescriptions;

  FindPossibleCVars(sVarName.GetData(), AutoCompleteOptions, AutoCompleteDescriptions);
  FindPossibleFunctions(sVarName.GetData(), AutoCompleteOptions, AutoCompleteDescriptions);

  if (AutoCompleteDescriptions.GetCount() > 1)
  {
    AutoCompleteDescriptions.Sort();

    inout_state.AddOutputLine("");

    for (wdUInt32 i = 0; i < AutoCompleteDescriptions.GetCount(); i++)
    {
      inout_state.AddOutputLine(AutoCompleteDescriptions[i].m_sText.GetData(), AutoCompleteDescriptions[i].m_Type);
    }

    inout_state.AddOutputLine("");
  }

  if (AutoCompleteOptions.GetCount() > 0)
  {
    if (szLastWordDelimiter != nullptr)
      inout_state.m_sInput = wdStringView(inout_state.m_sInput.GetData(), szLastWordDelimiter + 1);
    else
      inout_state.m_sInput.Clear();

    inout_state.m_sInput.Append(FindCommonString(AutoCompleteOptions).GetData());
  }
}


WD_STATICLINK_FILE(Core, Core_Console_Implementation_Autocomplete);
