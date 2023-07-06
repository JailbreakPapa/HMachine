#include <Core/CorePCH.h>

#include <Core/Scripting/LuaWrapper.h>

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

wdLuaWrapper::wdLuaWrapper()
{
  m_bReleaseOnExit = true;
  m_pState = nullptr;

  Clear();
}

wdLuaWrapper::wdLuaWrapper(lua_State* s)
{
  m_pState = s;
  m_bReleaseOnExit = false;
}

wdLuaWrapper::~wdLuaWrapper()
{
  if (m_bReleaseOnExit)
    lua_close(m_pState);
}

void wdLuaWrapper::Clear()
{
  WD_ASSERT_DEV(m_bReleaseOnExit, "Cannot clear a script that did not create the Lua state itself.");

  if (m_pState)
    lua_close(m_pState);

  m_pState = lua_newstate(lua_allocator, nullptr);

  luaL_openlibs(m_pState);
}

wdResult wdLuaWrapper::ExecuteString(const char* szString, const char* szDebugChunkName, wdLogInterface* pLogInterface) const
{
  WD_ASSERT_DEV(m_States.m_iLuaReturnValues == 0,
    "wdLuaWrapper::ExecuteString: You didn't discard the return-values of the previous script call. {0} Return-values were expected.",
    m_States.m_iLuaReturnValues);

  if (!pLogInterface)
    pLogInterface = wdLog::GetThreadLocalLogSystem();

  int error = luaL_loadbuffer(m_pState, szString, wdStringUtils::GetStringElementCount(szString), szDebugChunkName);

  if (error != LUA_OK)
  {
    WD_LOG_BLOCK("wdLuaWrapper::ExecuteString");

    wdLog::Error(pLogInterface, "[lua]Lua compile error: {0}", lua_tostring(m_pState, -1));
    wdLog::Info(pLogInterface, "[luascript]Script: {0}", szString);

    return WD_FAILURE;
  }

  error = lua_pcall(m_pState, 0, 0, 0);

  if (error != LUA_OK)
  {
    WD_LOG_BLOCK("wdLuaWrapper::ExecuteString");

    wdLog::Error(pLogInterface, "[lua]Lua error: {0}", lua_tostring(m_pState, -1));
    wdLog::Info(pLogInterface, "[luascript]Script: {0}", szString);

    return WD_FAILURE;
  }

  return WD_SUCCESS;
}

void* wdLuaWrapper::lua_allocator(void* ud, void* ptr, size_t osize, size_t nsize)
{
  /// \todo Create optimized allocator.

  if (nsize == 0)
  {
    delete[](wdUInt8*) ptr;
    return (nullptr);
  }

  wdUInt8* ucPtr = new wdUInt8[nsize];

  if (ptr != nullptr)
  {
    wdMemoryUtils::Copy(ucPtr, (wdUInt8*)ptr, wdUInt32(osize < nsize ? osize : nsize));

    delete[](wdUInt8*) ptr;
  }

  return ((void*)ucPtr);
}


#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT


WD_STATICLINK_FILE(Core, Core_Scripting_LuaWrapper_Initialize);
