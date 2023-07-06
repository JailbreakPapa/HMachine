#include <Core/CorePCH.h>

#include <Core/Scripting/LuaWrapper.h>

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

wdResult wdLuaWrapper::OpenTable(const char* szName)
{
  if (m_States.m_iOpenTables == 0)
    lua_getglobal(m_pState, szName);
  else
  {
    lua_pushstring(m_pState, szName);
    lua_gettable(m_pState, -2);
  }

  // failed, it's no table
  if (lua_istable(m_pState, -1) == 0)
  {
    lua_pop(m_pState, 1);
    return WD_FAILURE;
  }

  m_States.m_iOpenTables++;
  return WD_SUCCESS;
}

wdResult wdLuaWrapper::OpenTableFromParameter(wdUInt32 uiFunctionParameter)
{
  lua_pushvalue(m_pState, uiFunctionParameter + s_iParamOffset);

  // failed, it's no table
  if (lua_istable(m_pState, -1) == 0)
  {
    lua_pop(m_pState, 1);
    return WD_FAILURE;
  }

  m_States.m_iOpenTables++;
  return WD_SUCCESS;
}

void wdLuaWrapper::CloseTable()
{
  DiscardReturnValues();

  if (m_States.m_iOpenTables == 0)
    return;

  m_States.m_iOpenTables--;

  lua_pop(m_pState, 1);
}

void wdLuaWrapper::CloseAllTables()
{
  DiscardReturnValues();

  if (m_States.m_iOpenTables == 0)
    return;

  lua_pop(m_pState, m_States.m_iOpenTables);
  m_States.m_iOpenTables = 0;
}

#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT


WD_STATICLINK_FILE(Core, Core_Scripting_LuaWrapper_Tables);
