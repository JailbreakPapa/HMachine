#include <Core/CorePCH.h>

#include <Core/Console/LuaInterpreter.h>
#include <Core/Console/QuakeConsole.h>
#include <Core/Scripting/LuaWrapper.h>

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

static void AllowScriptCVarAccess(wdLuaWrapper& ref_script);

static const wdString GetNextWord(wdStringView& ref_sString)
{
  const char* szStartWord = wdStringUtils::SkipCharacters(ref_sString.GetStartPointer(), wdStringUtils::IsWhiteSpace, false);
  const char* szEndWord = wdStringUtils::FindWordEnd(szStartWord, wdStringUtils::IsIdentifierDelimiter_C_Code, true);

  ref_sString = wdStringView(szEndWord);

  return wdStringView(szStartWord, szEndWord);
}

static wdString GetRestWords(wdStringView sString)
{
  return wdStringUtils::SkipCharacters(sString.GetStartPointer(), wdStringUtils::IsWhiteSpace, false);
}

static int LUAFUNC_ConsoleFunc(lua_State* pState)
{
  wdLuaWrapper s(pState);

  wdConsoleFunctionBase* pFunc = (wdConsoleFunctionBase*)s.GetFunctionLightUserData();

  if (pFunc->GetNumParameters() != s.GetNumberOfFunctionParameters())
  {
    wdLog::Error("Function '{0}' expects {1} parameters, {2} were provided.", pFunc->GetName(), pFunc->GetNumParameters(), s.GetNumberOfFunctionParameters());
    return s.ReturnToScript();
  }

  wdHybridArray<wdVariant, 8> m_Params;
  m_Params.SetCount(pFunc->GetNumParameters());

  for (wdUInt32 p = 0; p < pFunc->GetNumParameters(); ++p)
  {
    switch (pFunc->GetParameterType(p))
    {
      case wdVariant::Type::Bool:
        m_Params[p] = s.GetBoolParameter(p);
        break;
      case wdVariant::Type::Int8:
      case wdVariant::Type::Int16:
      case wdVariant::Type::Int32:
      case wdVariant::Type::Int64:
      case wdVariant::Type::UInt8:
      case wdVariant::Type::UInt16:
      case wdVariant::Type::UInt32:
      case wdVariant::Type::UInt64:
        m_Params[p] = s.GetIntParameter(p);
        break;
      case wdVariant::Type::Float:
      case wdVariant::Type::Double:
        m_Params[p] = s.GetFloatParameter(p);
        break;
      case wdVariant::Type::String:
        m_Params[p] = s.GetStringParameter(p);
        break;
      default:
        wdLog::Error("Function '{0}': Type of parameter {1} is not supported by the Lua interpreter.", pFunc->GetName(), p);
        return s.ReturnToScript();
    }
  }

  if (!m_Params.IsEmpty())
    pFunc->Call(wdArrayPtr<wdVariant>(&m_Params[0], m_Params.GetCount())).IgnoreResult();
  else
    pFunc->Call(wdArrayPtr<wdVariant>()).IgnoreResult();

  return s.ReturnToScript();
}

static void SanitizeCVarNames(wdStringBuilder& ref_sCommand)
{
  wdStringBuilder sanitizedCVarName;

  for (const wdCVar* pCVar = wdCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    sanitizedCVarName = pCVar->GetName();
    sanitizedCVarName.ReplaceAll(".", "_");

    ref_sCommand.ReplaceAll(pCVar->GetName(), sanitizedCVarName);
  }
}

static void UnSanitizeCVarName(wdStringBuilder& ref_sCvarName)
{
  wdStringBuilder sanitizedCVarName;

  for (const wdCVar* pCVar = wdCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    sanitizedCVarName = pCVar->GetName();
    sanitizedCVarName.ReplaceAll(".", "_");

    if (ref_sCvarName == sanitizedCVarName)
    {
      ref_sCvarName = pCVar->GetName();
      return;
    }
  }
}

void wdCommandInterpreterLua::Interpret(wdCommandInterpreterState& inout_state)
{
  inout_state.m_sOutput.Clear();

  wdStringBuilder sRealCommand = inout_state.m_sInput;

  if (sRealCommand.IsEmpty())
  {
    inout_state.AddOutputLine("");
    return;
  }

  sRealCommand.Trim(" \t\n\r");
  wdStringBuilder sSanitizedCommand = sRealCommand;
  SanitizeCVarNames(sSanitizedCommand);

  wdStringView sCommandIt = sSanitizedCommand;

  const wdString sSanitizedVarName = GetNextWord(sCommandIt);
  wdStringBuilder sRealVarName = sSanitizedVarName;
  UnSanitizeCVarName(sRealVarName);

  while (wdStringUtils::IsWhiteSpace(sCommandIt.GetCharacter()))
  {
    sCommandIt.Shrink(1, 0);
  }

  const bool bSetValue = sCommandIt.StartsWith("=");

  if (bSetValue)
  {
    sCommandIt.Shrink(1, 0);
  }

  wdStringBuilder sValue = GetRestWords(sCommandIt);
  bool bValueEmpty = sValue.IsEmpty();

  wdStringBuilder sTemp;

  wdLuaWrapper Script;
  AllowScriptCVarAccess(Script);

  // Register all ConsoleFunctions
  {
    wdConsoleFunctionBase* pFunc = wdConsoleFunctionBase::GetFirstInstance();
    while (pFunc)
    {
      Script.RegisterCFunction(pFunc->GetName().GetData(sTemp), LUAFUNC_ConsoleFunc, pFunc);

      pFunc = pFunc->GetNextInstance();
    }
  }

  sTemp = "> ";
  sTemp.Append(sRealCommand);
  inout_state.AddOutputLine(sTemp, wdConsoleString::Type::Executed);

  wdCVar* pCVAR = wdCVar::FindCVarByName(sRealVarName.GetData());
  if (pCVAR != nullptr)
  {
    if ((bSetValue) && (sValue == "") && (pCVAR->GetType() == wdCVarType::Bool))
    {
      // someone typed "myvar =" -> on bools this is the short form for "myvar = not myvar" (toggle), so insert the rest here

      bValueEmpty = false;

      sSanitizedCommand.AppendFormat(" not {0}", sSanitizedVarName);
    }

    if (bSetValue && !bValueEmpty)
    {
      wdMuteLog muteLog;

      if (Script.ExecuteString(sSanitizedCommand, "console", &muteLog).Failed())
      {
        inout_state.AddOutputLine("  Error Executing Command.", wdConsoleString::Type::Error);
        return;
      }
      else
      {
        if (pCVAR->GetFlags().IsAnySet(wdCVarFlags::RequiresRestart))
        {
          inout_state.AddOutputLine("  This change takes only effect after a restart.", wdConsoleString::Type::Note);
        }

        sTemp.Format("  {0} = {1}", sRealVarName, wdQuakeConsole::GetFullInfoAsString(pCVAR));
        inout_state.AddOutputLine(sTemp, wdConsoleString::Type::Success);
      }
    }
    else
    {
      sTemp.Format("{0} = {1}", sRealVarName, wdQuakeConsole::GetFullInfoAsString(pCVAR));
      inout_state.AddOutputLine(sTemp);

      if (!pCVAR->GetDescription().IsEmpty())
      {
        sTemp.Format("  Description: {0}", pCVAR->GetDescription());
        inout_state.AddOutputLine(sTemp, wdConsoleString::Type::Success);
      }
      else
        inout_state.AddOutputLine("  No Description available.", wdConsoleString::Type::Success);
    }

    return;
  }
  else
  {
    wdMuteLog muteLog;

    if (Script.ExecuteString(sSanitizedCommand, "console", &muteLog).Failed())
    {
      inout_state.AddOutputLine("  Error Executing Command.", wdConsoleString::Type::Error);
      return;
    }
  }
}

static int LUAFUNC_ReadCVAR(lua_State* pState)
{
  wdLuaWrapper s(pState);

  wdStringBuilder cvarName = s.GetStringParameter(0);
  UnSanitizeCVarName(cvarName);

  wdCVar* pCVar = wdCVar::FindCVarByName(cvarName);

  if (pCVar == nullptr)
  {
    s.PushReturnValueNil();
    return s.ReturnToScript();
  }

  switch (pCVar->GetType())
  {
    case wdCVarType::Int:
    {
      wdCVarInt* pVar = (wdCVarInt*)pCVar;
      s.PushReturnValue(pVar->GetValue());
    }
    break;
    case wdCVarType::Bool:
    {
      wdCVarBool* pVar = (wdCVarBool*)pCVar;
      s.PushReturnValue(pVar->GetValue());
    }
    break;
    case wdCVarType::Float:
    {
      wdCVarFloat* pVar = (wdCVarFloat*)pCVar;
      s.PushReturnValue(pVar->GetValue());
    }
    break;
    case wdCVarType::String:
    {
      wdCVarString* pVar = (wdCVarString*)pCVar;
      s.PushReturnValue(pVar->GetValue().GetData());
    }
    break;
    case wdCVarType::ENUM_COUNT:
      break;
  }

  return s.ReturnToScript();
}


static int LUAFUNC_WriteCVAR(lua_State* pState)
{
  wdLuaWrapper s(pState);

  wdStringBuilder cvarName = s.GetStringParameter(0);
  UnSanitizeCVarName(cvarName);

  wdCVar* pCVar = wdCVar::FindCVarByName(cvarName);

  if (pCVar == nullptr)
  {
    s.PushReturnValue(false);
    return s.ReturnToScript();
  }

  s.PushReturnValue(true);

  switch (pCVar->GetType())
  {
    case wdCVarType::Int:
    {
      wdCVarInt* pVar = (wdCVarInt*)pCVar;
      *pVar = s.GetIntParameter(1);
    }
    break;
    case wdCVarType::Bool:
    {
      wdCVarBool* pVar = (wdCVarBool*)pCVar;
      *pVar = s.GetBoolParameter(1);
    }
    break;
    case wdCVarType::Float:
    {
      wdCVarFloat* pVar = (wdCVarFloat*)pCVar;
      *pVar = s.GetFloatParameter(1);
    }
    break;
    case wdCVarType::String:
    {
      wdCVarString* pVar = (wdCVarString*)pCVar;
      *pVar = s.GetStringParameter(1);
    }
    break;
    case wdCVarType::ENUM_COUNT:
      break;
  }

  return s.ReturnToScript();
}

static void AllowScriptCVarAccess(wdLuaWrapper& ref_script)
{
  ref_script.RegisterCFunction("ReadCVar", LUAFUNC_ReadCVAR);
  ref_script.RegisterCFunction("WriteCVar", LUAFUNC_WriteCVAR);

  wdStringBuilder sInit = "\
function readcvar (t, key)\n\
return (ReadCVar (key))\n\
end\n\
\n\
function writecvar (t, key, value)\n\
if not WriteCVar (key, value) then\n\
rawset (t, key, value or false)\n\
end\n\
end\n\
\n\
setmetatable (_G, {\n\
__newindex = writecvar,\n\
__index = readcvar,\n\
__metatable = \"Access Denied\",\n\
})";

  ref_script.ExecuteString(sInit.GetData()).IgnoreResult();
}

#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT


WD_STATICLINK_FILE(Core, Core_Console_Implementation_LuaInterpreter);
