#include <Core/CorePCH.h>

#include <Core/Scripting/DuktapeHelper.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Duktape/duktape.h>
#  include <Foundation/IO/FileSystem/FileReader.h>

WD_CHECK_AT_COMPILETIME(wdDuktapeTypeMask::None == DUK_TYPE_MASK_NONE);
WD_CHECK_AT_COMPILETIME(wdDuktapeTypeMask::Undefined == DUK_TYPE_MASK_UNDEFINED);
WD_CHECK_AT_COMPILETIME(wdDuktapeTypeMask::Null == DUK_TYPE_MASK_NULL);
WD_CHECK_AT_COMPILETIME(wdDuktapeTypeMask::Bool == DUK_TYPE_MASK_BOOLEAN);
WD_CHECK_AT_COMPILETIME(wdDuktapeTypeMask::Number == DUK_TYPE_MASK_NUMBER);
WD_CHECK_AT_COMPILETIME(wdDuktapeTypeMask::String == DUK_TYPE_MASK_STRING);
WD_CHECK_AT_COMPILETIME(wdDuktapeTypeMask::Object == DUK_TYPE_MASK_OBJECT);
WD_CHECK_AT_COMPILETIME(wdDuktapeTypeMask::Buffer == DUK_TYPE_MASK_BUFFER);
WD_CHECK_AT_COMPILETIME(wdDuktapeTypeMask::Pointer == DUK_TYPE_MASK_POINTER);

#  if WD_ENABLED(WD_COMPILE_FOR_DEBUG)


void wdDuktapeHelper::EnableStackChangeVerification() const
{
  m_bVerifyStackChange = true;
}

#  endif

wdDuktapeHelper::wdDuktapeHelper(duk_context* pContext)
  : m_pContext(pContext)
{
#  if WD_ENABLED(WD_COMPILE_FOR_DEBUG)
  if (m_pContext)
  {
    m_bVerifyStackChange = false;
    m_iStackTopAtStart = duk_get_top(m_pContext);
  }
#  endif
}

wdDuktapeHelper::wdDuktapeHelper(const wdDuktapeHelper& rhs)
  : wdDuktapeHelper(rhs.GetContext())
{
}

wdDuktapeHelper::~wdDuktapeHelper() = default;

void wdDuktapeHelper::operator=(const wdDuktapeHelper& rhs)
{
  if (this == &rhs)
    return;

  *this = wdDuktapeHelper(rhs.GetContext());
}

#  if WD_ENABLED(WD_COMPILE_FOR_DEBUG)
void wdDuktapeHelper::VerifyExpectedStackChange(wdInt32 iExpectedStackChange, const char* szFile, wdUInt32 uiLine, const char* szFunction) const
{
  if (m_bVerifyStackChange && m_pContext)
  {
    const wdInt32 iCurTop = duk_get_top(m_pContext);
    const wdInt32 iStackChange = iCurTop - m_iStackTopAtStart;

    if (iStackChange != iExpectedStackChange)
    {
      wdLog::Error("{}:{} ({}): Stack change {} != {}", szFile, uiLine, szFunction, iStackChange, iExpectedStackChange);
    }
  }
}
#  endif

void wdDuktapeHelper::Error(const wdFormatString& text)
{
  wdStringBuilder tmp;
  duk_error(m_pContext, DUK_ERR_ERROR, text.GetText(tmp));
}

void wdDuktapeHelper::LogStackTrace(wdInt32 iErrorObjIdx)
{
  if (duk_is_error(m_pContext, iErrorObjIdx))
  {
    WD_LOG_BLOCK("Stack Trace");

    duk_get_prop_string(m_pContext, iErrorObjIdx, "stack");

    const wdStringBuilder stack = duk_safe_to_string(m_pContext, iErrorObjIdx);
    wdHybridArray<wdStringView, 32> lines;
    stack.Split(false, lines, "\n", "\r");

    for (wdStringView line : lines)
    {
      wdLog::Dev("{}", line);
    }

    duk_pop(m_pContext);
  }
}

void wdDuktapeHelper::PopStack(wdUInt32 n /*= 1*/)
{
  duk_pop_n(m_pContext, n);
}

void wdDuktapeHelper::PushGlobalObject()
{
  duk_push_global_object(m_pContext); // [ global ]
}

void wdDuktapeHelper::PushGlobalStash()
{
  duk_push_global_stash(m_pContext); // [ stash ]
}

wdResult wdDuktapeHelper::PushLocalObject(const char* szName, wdInt32 iParentObjectIndex /* = -1*/)
{
  duk_require_top_index(m_pContext);

  if (duk_get_prop_string(m_pContext, iParentObjectIndex, szName) == false) // [ obj/undef ]
  {
    duk_pop(m_pContext); // [ ]
    return WD_FAILURE;
  }

  // [ object ]
  return WD_SUCCESS;
}

bool wdDuktapeHelper::HasProperty(const char* szPropertyName, wdInt32 iParentObjectIndex /*= -1*/) const
{
  return duk_is_object(m_pContext, iParentObjectIndex) && duk_has_prop_string(m_pContext, iParentObjectIndex, szPropertyName);
}

bool wdDuktapeHelper::GetBoolProperty(const char* szPropertyName, bool bFallback, wdInt32 iParentObjectIndex /*= -1*/) const
{
  bool result = bFallback;

  if (duk_is_object(m_pContext, iParentObjectIndex))
  {
    if (duk_get_prop_string(m_pContext, iParentObjectIndex, szPropertyName)) // [ value/undef ]
    {
      result = duk_get_boolean_default(m_pContext, -1, bFallback); // [ value ]
    }

    duk_pop(m_pContext); // [ ]
  }

  return result;
}

wdInt32 wdDuktapeHelper::GetIntProperty(const char* szPropertyName, wdInt32 iFallback, wdInt32 iParentObjectIndex /*= -1*/) const
{
  wdInt32 result = iFallback;

  if (duk_is_object(m_pContext, iParentObjectIndex))
  {
    if (duk_get_prop_string(m_pContext, iParentObjectIndex, szPropertyName)) // [ value/undef ]
    {
      result = duk_get_int_default(m_pContext, -1, iFallback); // [ value ]
    }

    duk_pop(m_pContext); // [ ]
  }

  return result;
}

wdUInt32 wdDuktapeHelper::GetUIntProperty(const char* szPropertyName, wdUInt32 uiFallback, wdInt32 iParentObjectIndex /*= -1*/) const
{
  wdUInt32 result = uiFallback;

  if (duk_is_object(m_pContext, iParentObjectIndex))
  {
    if (duk_get_prop_string(m_pContext, iParentObjectIndex, szPropertyName)) // [ value/undef ]
    {
      result = duk_get_uint_default(m_pContext, -1, uiFallback); // [ value ]
    }

    duk_pop(m_pContext); // [ ]
  }

  return result;
}

float wdDuktapeHelper::GetFloatProperty(const char* szPropertyName, float fFallback, wdInt32 iParentObjectIndex /*= -1*/) const
{
  return static_cast<float>(GetNumberProperty(szPropertyName, fFallback, iParentObjectIndex));
}

double wdDuktapeHelper::GetNumberProperty(const char* szPropertyName, double fFallback, wdInt32 iParentObjectIndex /*= -1*/) const
{
  double result = fFallback;

  if (duk_is_object(m_pContext, iParentObjectIndex))
  {
    if (duk_get_prop_string(m_pContext, iParentObjectIndex, szPropertyName)) // [ value/undef ]
    {
      result = duk_get_number_default(m_pContext, -1, fFallback); // [ value ]
    }

    duk_pop(m_pContext); // [ ]
  }

  return result;
}

const char* wdDuktapeHelper::GetStringProperty(const char* szPropertyName, const char* szFallback, wdInt32 iParentObjectIndex /*= -1*/) const
{
  const char* result = szFallback;

  if (duk_is_object(m_pContext, iParentObjectIndex))
  {
    if (duk_get_prop_string(m_pContext, iParentObjectIndex, szPropertyName)) // [ value/undef ]
    {
      result = duk_get_string_default(m_pContext, -1, szFallback); // [ value ]
    }

    duk_pop(m_pContext); // [ ]
  }

  return result;
}

void wdDuktapeHelper::SetBoolProperty(const char* szPropertyName, bool value, wdInt32 iParentObjectIndex /*= -1*/) const
{
  wdDuktapeHelper duk(m_pContext);

  duk_push_boolean(m_pContext, value); // [ value ]

  if (iParentObjectIndex >= 0)
    duk_put_prop_string(m_pContext, iParentObjectIndex, szPropertyName); // [ ]
  else
    duk_put_prop_string(m_pContext, iParentObjectIndex - 1, szPropertyName); // [ ]

  WD_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void wdDuktapeHelper::SetNumberProperty(const char* szPropertyName, double value, wdInt32 iParentObjectIndex /*= -1*/) const
{
  wdDuktapeHelper duk(m_pContext);

  duk_push_number(m_pContext, value); // [ value ]

  if (iParentObjectIndex >= 0)
    duk_put_prop_string(m_pContext, iParentObjectIndex, szPropertyName); // [ ]
  else
    duk_put_prop_string(m_pContext, iParentObjectIndex - 1, szPropertyName); // [ ]

  WD_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void wdDuktapeHelper::SetStringProperty(const char* szPropertyName, const char* value, wdInt32 iParentObjectIndex /*= -1*/) const
{
  wdDuktapeHelper duk(m_pContext);

  duk_push_string(m_pContext, value); // [ value ]

  if (iParentObjectIndex >= 0)
    duk_put_prop_string(m_pContext, iParentObjectIndex, szPropertyName); // [ ]
  else
    duk_put_prop_string(m_pContext, iParentObjectIndex - 1, szPropertyName); // [ ]

  WD_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void wdDuktapeHelper::SetCustomProperty(const char* szPropertyName, wdInt32 iParentObjectIndex /*= -1*/) const
{
  wdDuktapeHelper duk(m_pContext); // [ value ]

  if (iParentObjectIndex >= 0)
    duk_put_prop_string(m_pContext, iParentObjectIndex, szPropertyName); // [ ]
  else
    duk_put_prop_string(m_pContext, iParentObjectIndex - 1, szPropertyName); // [ ]

  WD_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, -1);
}

void wdDuktapeHelper::StorePointerInStash(const char* szKey, void* pPointer)
{
  duk_push_global_stash(m_pContext);                                                      // [ stash ]
  *reinterpret_cast<void**>(duk_push_fixed_buffer(m_pContext, sizeof(void*))) = pPointer; // [ stash buffer ]
  duk_put_prop_string(m_pContext, -2, szKey);                                             // [ stash ]
  duk_pop(m_pContext);                                                                    // [ ]
}

void* wdDuktapeHelper::RetrievePointerFromStash(const char* szKey) const
{
  void* pPointer = nullptr;

  duk_push_global_stash(m_pContext); // [ stash ]

  if (duk_get_prop_string(m_pContext, -1, szKey)) // [ stash obj/undef ]
  {
    WD_ASSERT_DEBUG(duk_is_buffer(m_pContext, -1), "Object '{}' in stash is not a buffer", szKey);

    pPointer = *reinterpret_cast<void**>(duk_get_buffer(m_pContext, -1, nullptr)); // [ stash obj/undef ]
  }

  duk_pop_2(m_pContext); // [ ]

  return pPointer;
}

void wdDuktapeHelper::StoreStringInStash(const char* szKey, const char* value)
{
  duk_push_global_stash(m_pContext);          // [ stash ]
  duk_push_string(m_pContext, value);         // [ stash value ]
  duk_put_prop_string(m_pContext, -2, szKey); // [ stash ]
  duk_pop(m_pContext);                        // [ ]
}

const char* wdDuktapeHelper::RetrieveStringFromStash(const char* szKey, const char* szFallback /*= nullptr*/) const
{
  duk_push_global_stash(m_pContext); // [ stash ]

  if (!duk_get_prop_string(m_pContext, -1, szKey)) // [ stash string/undef ]
  {
    duk_pop_2(m_pContext); // [ ]
    return szFallback;
  }

  szFallback = duk_get_string_default(m_pContext, -1, szFallback); // [ stash string ]
  duk_pop_2(m_pContext);                                           // [ ]

  return szFallback;
}

bool wdDuktapeHelper::IsOfType(wdBitflags<wdDuktapeTypeMask> mask, wdInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, mask.GetValue());
}

bool wdDuktapeHelper::IsBool(wdInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_BOOLEAN);
}

bool wdDuktapeHelper::IsNumber(wdInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_NUMBER);
}

bool wdDuktapeHelper::IsString(wdInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_STRING);
}

bool wdDuktapeHelper::IsNull(wdInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_NULL);
}

bool wdDuktapeHelper::IsUndefined(wdInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_UNDEFINED);
}

bool wdDuktapeHelper::IsObject(wdInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_OBJECT);
}

bool wdDuktapeHelper::IsBuffer(wdInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_BUFFER);
}

bool wdDuktapeHelper::IsPointer(wdInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_POINTER);
}

bool wdDuktapeHelper::IsNullOrUndefined(wdInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_NULL | DUK_TYPE_MASK_UNDEFINED);
}

void wdDuktapeHelper::RegisterGlobalFunction(
  const char* szFunctionName, duk_c_function function, wdUInt8 uiNumArguments, wdInt16 iMagicValue /*= 0*/)
{
  // TODO: could store iFuncIdx for faster function calls

  duk_push_global_object(m_pContext);                                                 // [ global ]
  /*const int iFuncIdx =*/duk_push_c_function(m_pContext, function, uiNumArguments);  // [ global func ]
  duk_set_magic(m_pContext, -1, iMagicValue);                                         // [ global func ]
  duk_put_prop_string(m_pContext, -2, szFunctionName);                                // [ global ]
  duk_pop(m_pContext);                                                                // [ ]
}

void wdDuktapeHelper::RegisterGlobalFunctionWithVarArgs(const char* szFunctionName, duk_c_function function, wdInt16 iMagicValue /*= 0*/)
{
  // TODO: could store iFuncIdx for faster function calls

  duk_push_global_object(m_pContext);                                              // [ global ]
  /*const int iFuncIdx =*/duk_push_c_function(m_pContext, function, DUK_VARARGS);  // [ global func ]
  duk_set_magic(m_pContext, -1, iMagicValue);                                      // [ global func ]
  duk_put_prop_string(m_pContext, -2, szFunctionName);                             // [ global ]
  duk_pop(m_pContext);                                                             // [ ]
}

void wdDuktapeHelper::RegisterObjectFunction(
  const char* szFunctionName, duk_c_function function, wdUInt8 uiNumArguments, wdInt32 iParentObjectIndex /*= -1*/, wdInt16 iMagicValue /*= 0*/)
{
  /*const int iFuncIdx =*/duk_push_c_function(m_pContext, function, uiNumArguments);  // [ func ]
  duk_set_magic(m_pContext, -1, iMagicValue);                                         // [ func ]

  if (iParentObjectIndex < 0)
  {
    duk_put_prop_string(m_pContext, iParentObjectIndex - 1, szFunctionName); // [ ]
  }
  else
  {
    duk_put_prop_string(m_pContext, iParentObjectIndex, szFunctionName); // [ ]
  }
}

wdResult wdDuktapeHelper::PrepareGlobalFunctionCall(const char* szFunctionName)
{
  if (!duk_get_global_string(m_pContext, szFunctionName)) // [ func/undef ]
    goto failure;

  if (!duk_is_function(m_pContext, -1)) // [ func ]
    goto failure;

  m_iPushedValues = 0;
  return WD_SUCCESS; // [ func ]

failure:
  duk_pop(m_pContext); // [ ]
  return WD_FAILURE;
}

wdResult wdDuktapeHelper::PrepareObjectFunctionCall(const char* szFunctionName, wdInt32 iParentObjectIndex /*= -1*/)
{
  duk_require_top_index(m_pContext);

  if (!duk_get_prop_string(m_pContext, iParentObjectIndex, szFunctionName)) // [ func/undef ]
    goto failure;

  if (!duk_is_function(m_pContext, -1)) // [ func ]
    goto failure;

  m_iPushedValues = 0;
  return WD_SUCCESS; // [ func ]

failure:
  duk_pop(m_pContext); // [ ]
  return WD_FAILURE;
}

wdResult wdDuktapeHelper::CallPreparedFunction()
{
  if (duk_pcall(m_pContext, m_iPushedValues) == DUK_EXEC_SUCCESS) // [ func n-args ] -> [ result/error ]
  {
    return WD_SUCCESS; // [ result ]
  }
  else
  {
    wdLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1));

    LogStackTrace(-1);

    return WD_FAILURE; // [ error ]
  }
}

wdResult wdDuktapeHelper::PrepareMethodCall(const char* szMethodName, wdInt32 iParentObjectIndex /*= -1*/)
{
  if (!duk_get_prop_string(m_pContext, iParentObjectIndex, szMethodName)) // [ func/undef ]
    goto failure;

  if (!duk_is_function(m_pContext, -1)) // [ func ]
    goto failure;

  if (iParentObjectIndex < 0)
  {
    duk_dup(m_pContext, iParentObjectIndex - 1); // [ func this ]
  }
  else
  {
    duk_dup(m_pContext, iParentObjectIndex); // [ func this ]
  }

  m_iPushedValues = 0;
  return WD_SUCCESS; // [ func this ]

failure:
  duk_pop(m_pContext); // [ ]
  return WD_FAILURE;
}

wdResult wdDuktapeHelper::CallPreparedMethod()
{
  if (duk_pcall_method(m_pContext, m_iPushedValues) == DUK_EXEC_SUCCESS) // [ func this n-args ] -> [ result/error ]
  {
    return WD_SUCCESS; // [ result ]
  }
  else
  {
    wdLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1));

    LogStackTrace(-1);

    return WD_FAILURE; // [ error ]
  }
}

void wdDuktapeHelper::PushInt(wdInt32 iParam)
{
  duk_push_int(m_pContext, iParam); // [ value ]
  ++m_iPushedValues;
}

void wdDuktapeHelper::PushUInt(wdUInt32 uiParam)
{
  duk_push_uint(m_pContext, uiParam); // [ value ]
  ++m_iPushedValues;
}

void wdDuktapeHelper::PushBool(bool bParam)
{
  duk_push_boolean(m_pContext, bParam); // [ value ]
  ++m_iPushedValues;
}

void wdDuktapeHelper::PushNumber(double fParam)
{
  duk_push_number(m_pContext, fParam); // [ value ]
  ++m_iPushedValues;
}

void wdDuktapeHelper::PushString(const wdStringView& sParam)
{
  duk_push_lstring(m_pContext, sParam.GetStartPointer(), sParam.GetElementCount()); // [ value ]
  ++m_iPushedValues;
}

void wdDuktapeHelper::PushNull()
{
  duk_push_null(m_pContext); // [ null ]
  ++m_iPushedValues;
}

void wdDuktapeHelper::PushUndefined()
{
  duk_push_undefined(m_pContext); // [ undefined ]
  ++m_iPushedValues;
}

void wdDuktapeHelper::PushCustom(wdUInt32 uiNum)
{
  m_iPushedValues += uiNum;
}

bool wdDuktapeHelper::GetBoolValue(wdInt32 iStackElement, bool bFallback /*= false*/) const
{
  return duk_get_boolean_default(m_pContext, iStackElement, bFallback);
}

wdInt32 wdDuktapeHelper::GetIntValue(wdInt32 iStackElement, wdInt32 iFallback /*= 0*/) const
{
  return duk_get_int_default(m_pContext, iStackElement, iFallback);
}

wdUInt32 wdDuktapeHelper::GetUIntValue(wdInt32 iStackElement, wdUInt32 uiFallback /*= 0*/) const
{
  return duk_get_uint_default(m_pContext, iStackElement, uiFallback);
}

float wdDuktapeHelper::GetFloatValue(wdInt32 iStackElement, float fFallback /*= 0*/) const
{
  return static_cast<float>(duk_get_number_default(m_pContext, iStackElement, fFallback));
}

double wdDuktapeHelper::GetNumberValue(wdInt32 iStackElement, double fFallback /*= 0*/) const
{
  return duk_get_number_default(m_pContext, iStackElement, fFallback);
}

const char* wdDuktapeHelper::GetStringValue(wdInt32 iStackElement, const char* szFallback /*= ""*/) const
{
  return duk_get_string_default(m_pContext, iStackElement, szFallback);
}

wdResult wdDuktapeHelper::ExecuteString(const char* szString, const char* szDebugName /*= "eval"*/)
{
  duk_push_string(m_pContext, szDebugName);                       // [ filename ]
  if (duk_pcompile_string_filename(m_pContext, 0, szString) != 0) // [ function/error ]
  {
    WD_LOG_BLOCK("DukTape::ExecuteString", "Compilation failed");

    wdLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1)); // [ error ]

    LogStackTrace(-1);

    // TODO: print out line by line
    wdLog::Info("[duktape]Source: {0}", szString);

    duk_pop(m_pContext); // [ ]
    return WD_FAILURE;
  }

  // [ function ]

  if (duk_pcall(m_pContext, 0) != DUK_EXEC_SUCCESS) // [ result/error ]
  {
    WD_LOG_BLOCK("DukTape::ExecuteString", "Execution failed");

    wdLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1)); // [ error ]

    LogStackTrace(-1);

    // TODO: print out line by line
    wdLog::Info("[duktape]Source: {0}", szString);

    duk_pop(m_pContext); // [ ]
    return WD_FAILURE;
  }

  duk_pop(m_pContext); // [ ]
  return WD_SUCCESS;
}

wdResult wdDuktapeHelper::ExecuteStream(wdStreamReader& inout_stream, const char* szDebugName)
{
  wdStringBuilder source;
  source.ReadAll(inout_stream);

  return ExecuteString(source, szDebugName);
}

wdResult wdDuktapeHelper::ExecuteFile(const char* szFile)
{
  wdFileReader file;
  WD_SUCCEED_OR_RETURN(file.Open(szFile));

  return ExecuteStream(file, szFile);
}

#endif


WD_STATICLINK_FILE(Core, Core_Scripting_Duktape_DuktapeHelper);
