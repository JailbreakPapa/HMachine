#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

#  pragma once

inline lua_State* wdLuaWrapper::GetLuaState()
{
  return m_pState;
}

inline wdInt32 wdLuaWrapper::ReturnToScript() const
{
  return (m_States.m_iParametersPushed);
}

inline wdUInt32 wdLuaWrapper::GetNumberOfFunctionParameters() const
{
  return ((int)lua_gettop(m_pState));
}

inline bool wdLuaWrapper::IsParameterBool(wdUInt32 uiParameter) const
{
  return (lua_type(m_pState, uiParameter + s_iParamOffset) == LUA_TBOOLEAN);
}

inline bool wdLuaWrapper::IsParameterFloat(wdUInt32 uiParameter) const
{
  return (lua_type(m_pState, uiParameter + s_iParamOffset) == LUA_TNUMBER);
}

inline bool wdLuaWrapper::IsParameterInt(wdUInt32 uiParameter) const
{
  return (lua_type(m_pState, uiParameter + s_iParamOffset) == LUA_TNUMBER);
}

inline bool wdLuaWrapper::IsParameterString(wdUInt32 uiParameter) const
{
  return (lua_type(m_pState, uiParameter + s_iParamOffset) == LUA_TSTRING);
}

inline bool wdLuaWrapper::IsParameterNil(wdUInt32 uiParameter) const
{
  return (lua_type(m_pState, uiParameter + s_iParamOffset) == LUA_TNIL);
}

inline bool wdLuaWrapper::IsParameterTable(wdUInt32 uiParameter) const
{
  return (lua_type(m_pState, uiParameter + s_iParamOffset) == LUA_TTABLE);
}

inline void wdLuaWrapper::PushParameter(wdInt32 iParameter)
{
  lua_pushinteger(m_pState, iParameter);
  m_States.m_iParametersPushed++;
}

inline void wdLuaWrapper::PushParameter(bool bParameter)
{
  lua_pushboolean(m_pState, bParameter);
  m_States.m_iParametersPushed++;
}

inline void wdLuaWrapper::PushParameter(float fParameter)
{
  lua_pushnumber(m_pState, fParameter);
  m_States.m_iParametersPushed++;
}

inline void wdLuaWrapper::PushParameter(const char* szParameter)
{
  lua_pushstring(m_pState, szParameter);
  m_States.m_iParametersPushed++;
}

inline void wdLuaWrapper::PushParameter(const char* szParameter, wdUInt32 uiLength)
{
  lua_pushlstring(m_pState, szParameter, uiLength);
  m_States.m_iParametersPushed++;
}

inline void wdLuaWrapper::PushParameterNil()
{
  lua_pushnil(m_pState);
  m_States.m_iParametersPushed++;
}

inline void wdLuaWrapper::PushReturnValue(wdInt32 iParameter)
{
  lua_pushinteger(m_pState, iParameter);
  m_States.m_iParametersPushed++;
}

inline void wdLuaWrapper::PushReturnValue(bool bParameter)
{
  lua_pushboolean(m_pState, bParameter);
  m_States.m_iParametersPushed++;
}

inline void wdLuaWrapper::PushReturnValue(float fParameter)
{
  lua_pushnumber(m_pState, fParameter);
  m_States.m_iParametersPushed++;
}

inline void wdLuaWrapper::PushReturnValue(const char* szParameter)
{
  lua_pushstring(m_pState, szParameter);
  m_States.m_iParametersPushed++;
}

inline void wdLuaWrapper::PushReturnValue(const char* szParameter, wdUInt32 uiLength)
{
  lua_pushlstring(m_pState, szParameter, uiLength);
  m_States.m_iParametersPushed++;
}

inline void wdLuaWrapper::PushReturnValueNil()
{
  lua_pushnil(m_pState);
  m_States.m_iParametersPushed++;
}

inline void wdLuaWrapper::SetVariableNil(const char* szName) const
{
  lua_pushnil(m_pState);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void wdLuaWrapper::SetVariable(const char* szName, wdInt32 iValue) const
{
  lua_pushinteger(m_pState, iValue);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void wdLuaWrapper::SetVariable(const char* szName, float fValue) const
{
  lua_pushnumber(m_pState, fValue);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void wdLuaWrapper::SetVariable(const char* szName, bool bValue) const
{
  lua_pushboolean(m_pState, bValue);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void wdLuaWrapper::SetVariable(const char* szName, const char* szValue) const
{
  lua_pushstring(m_pState, szValue);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void wdLuaWrapper::SetVariable(const char* szName, const char* szValue, wdUInt32 uiLen) const
{
  lua_pushlstring(m_pState, szValue, uiLen);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void wdLuaWrapper::PushTable(const char* szTableName, bool bGlobalTable)
{
  if (bGlobalTable || m_States.m_iOpenTables == 0)
    lua_getglobal(m_pState, szTableName);
  else
  {
    lua_pushstring(m_pState, szTableName);
    lua_gettable(m_pState, -2);
  }

  m_States.m_iParametersPushed++;
}

inline int wdLuaWrapper::GetIntParameter(wdUInt32 uiParameter) const
{
  return ((int)(lua_tointeger(m_pState, uiParameter + s_iParamOffset)));
}

inline bool wdLuaWrapper::GetBoolParameter(wdUInt32 uiParameter) const
{
  return (lua_toboolean(m_pState, uiParameter + s_iParamOffset) != 0);
}

inline float wdLuaWrapper::GetFloatParameter(wdUInt32 uiParameter) const
{
  return ((float)(lua_tonumber(m_pState, uiParameter + s_iParamOffset)));
}

inline const char* wdLuaWrapper::GetStringParameter(wdUInt32 uiParameter) const
{
  return (lua_tostring(m_pState, uiParameter + s_iParamOffset));
}

#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT
