#include <Core/CorePCH.h>

#include <Core/Scripting/LuaWrapper.h>

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

void wdLuaWrapper::RegisterCFunction(const char* szFunctionName, lua_CFunction function, void* pLightUserData) const
{
  lua_pushlightuserdata(m_pState, pLightUserData);
  lua_pushcclosure(m_pState, function, 1);
  lua_setglobal(m_pState, szFunctionName);
}

void* wdLuaWrapper::GetFunctionLightUserData() const
{
  return lua_touserdata(m_pState, lua_upvalueindex(1));
}

bool wdLuaWrapper::PrepareFunctionCall(const char* szFunctionName)
{
  WD_ASSERT_DEV(m_States.m_iLuaReturnValues == 0,
    "wdLuaWrapper::PrepareFunctionCall: You didn't discard the return-values of the previous script call. {0} Return-values "
    "were expected.",
    m_States.m_iLuaReturnValues);

  m_States.m_iParametersPushed = 0;

  if (m_States.m_iOpenTables == 0)
    lua_getglobal(m_pState, szFunctionName);
  else
  {
    lua_pushstring(m_pState, szFunctionName);
    lua_gettable(m_pState, -2);
  }

  if (lua_isfunction(m_pState, -1) == 0)
  {
    lua_pop(m_pState, 1);
    return false;
  }

  return true;
}

wdResult wdLuaWrapper::CallPreparedFunction(wdUInt32 uiExpectedReturnValues, wdLogInterface* pLogInterface)
{
  m_States.m_iLuaReturnValues = uiExpectedReturnValues;

  // save the current states on a cheap stack
  const wdScriptStates StackedStates = m_States;
  m_States = wdScriptStates();

  if (pLogInterface == nullptr)
    pLogInterface = wdLog::GetThreadLocalLogSystem();

  if (lua_pcall(m_pState, StackedStates.m_iParametersPushed, uiExpectedReturnValues, 0) != 0)
  {
    // restore the states to their previous values
    m_States = StackedStates;

    m_States.m_iLuaReturnValues = 0;

    wdLog::Error(pLogInterface, "Script-function Call: {0}", lua_tostring(m_pState, -1));

    lua_pop(m_pState, 1); /* pop error message from the stack */
    return WD_FAILURE;
  }

  // before resetting the state, make sure the returned state has no stuff left
  WD_ASSERT_DEV((m_States.m_iLuaReturnValues == 0) && (m_States.m_iOpenTables == 0),
    "After wdLuaWrapper::CallPreparedFunction: Return values: {0}, Open Tables: {1}", m_States.m_iLuaReturnValues, m_States.m_iOpenTables);

  m_States = StackedStates;
  return WD_SUCCESS;
}

void wdLuaWrapper::DiscardReturnValues()
{
  if (m_States.m_iLuaReturnValues == 0)
    return;

  lua_pop(m_pState, m_States.m_iLuaReturnValues);
  m_States.m_iLuaReturnValues = 0;
}

bool wdLuaWrapper::IsReturnValueInt(wdUInt32 uiReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) == LUA_TNUMBER);
}

bool wdLuaWrapper::IsReturnValueBool(wdUInt32 uiReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) == LUA_TBOOLEAN);
}

bool wdLuaWrapper::IsReturnValueFloat(wdUInt32 uiReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) == LUA_TNUMBER);
}

bool wdLuaWrapper::IsReturnValueString(wdUInt32 uiReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) == LUA_TSTRING);
}

bool wdLuaWrapper::IsReturnValueNil(wdUInt32 uiReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) == LUA_TNIL);
}

wdInt32 wdLuaWrapper::GetIntReturnValue(wdUInt32 uiReturnValue) const
{
  return ((int)(lua_tointeger(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1)));
}

bool wdLuaWrapper::GetBoolReturnValue(wdUInt32 uiReturnValue) const
{
  return (lua_toboolean(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) != 0);
}

float wdLuaWrapper::GetFloatReturnValue(wdUInt32 uiReturnValue) const
{
  return ((float)(lua_tonumber(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1)));
}

const char* wdLuaWrapper::GetStringReturnValue(wdUInt32 uiReturnValue) const
{
  return (lua_tostring(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1));
}


#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT



WD_STATICLINK_FILE(Core, Core_Scripting_LuaWrapper_CFunctions);
