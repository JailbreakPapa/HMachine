#include <Core/CorePCH.h>

#include <Core/Scripting/DuktapeFunction.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Duktape/duk_module_duktape.h>
#  include <Duktape/duktape.h>

wdDuktapeFunction::wdDuktapeFunction(duk_context* pExistingContext)
  : wdDuktapeHelper(pExistingContext)
{
}

wdDuktapeFunction::~wdDuktapeFunction()
{
#  if WD_ENABLED(WD_COMPILE_FOR_DEBUG)
  if (m_bVerifyStackChange && !m_bDidReturnValue)
  {
    wdLog::Error("You need to call one wdDuktapeFunction::ReturnXY() and return its result from your C function.");
  }
#  endif
}

wdUInt32 wdDuktapeFunction::GetNumVarArgFunctionParameters() const
{
  return duk_get_top(GetContext());
}

wdInt16 wdDuktapeFunction::GetFunctionMagicValue() const
{
  return static_cast<wdInt16>(duk_get_current_magic(GetContext()));
}

wdInt32 wdDuktapeFunction::ReturnVoid()
{
  WD_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  return 0;
}

wdInt32 wdDuktapeFunction::ReturnNull()
{
  WD_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_null(GetContext());
  return 1;
}

wdInt32 wdDuktapeFunction::ReturnUndefined()
{
  WD_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_undefined(GetContext());
  return 1;
}

wdInt32 wdDuktapeFunction::ReturnBool(bool value)
{
  WD_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_boolean(GetContext(), value);
  return 1;
}

wdInt32 wdDuktapeFunction::ReturnInt(wdInt32 value)
{
  WD_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_int(GetContext(), value);
  return 1;
}

wdInt32 wdDuktapeFunction::ReturnUInt(wdUInt32 value)
{
  WD_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_uint(GetContext(), value);
  return 1;
}

wdInt32 wdDuktapeFunction::ReturnFloat(float value)
{
  WD_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_number(GetContext(), value);
  return 1;
}

wdInt32 wdDuktapeFunction::ReturnNumber(double value)
{
  WD_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_number(GetContext(), value);
  return 1;
}

wdInt32 wdDuktapeFunction::ReturnString(wdStringView value)
{
  WD_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_lstring(GetContext(), value.GetStartPointer(), value.GetElementCount());
  return 1;
}

wdInt32 wdDuktapeFunction::ReturnCustom()
{
  WD_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  // push nothing, the user calls this because he pushed something custom already
  return 1;
}

#endif


WD_STATICLINK_FILE(Core, Core_Scripting_Duktape_DuktapeFunction);
