#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/String.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

struct duk_hthread;
using duk_context = duk_hthread;
using duk_c_function = int (*)(duk_context*);

struct wdDuktapeTypeMask
{
  using StorageType = wdUInt32;

  enum Enum
  {
    None = WD_BIT(0),      ///< no value, e.g. invalid index
    Undefined = WD_BIT(1), ///< ECMAScript undefined
    Null = WD_BIT(2),      ///< ECMAScript null
    Bool = WD_BIT(3),      ///< boolean, true or false
    Number = WD_BIT(4),    ///< any number, stored as a double
    String = WD_BIT(5),    ///< ECMAScript string: CESU-8 / extended UTF-8 encoded
    Object = WD_BIT(6),    ///< ECMAScript object: includes objects, arrays, functions, threads
    Buffer = WD_BIT(7),    ///< fixed or dynamic, garbage collected byte buffer
    Pointer = WD_BIT(8)    ///< raw void pointer

  };

  struct Bits
  {
    StorageType None : 1;
    StorageType Undefined : 1;
    StorageType Null : 1;
    StorageType Bool : 1;
    StorageType Number : 1;
    StorageType String : 1;
    StorageType Object : 1;
  };
};

WD_DECLARE_FLAGS_OPERATORS(wdDuktapeTypeMask);

#  if WD_ENABLED(WD_COMPILE_FOR_DEBUG)

#    define WD_DUK_VERIFY_STACK(duk, ExpectedStackChange) \
      duk.EnableStackChangeVerification();                \
      duk.VerifyExpectedStackChange(ExpectedStackChange, WD_SOURCE_FILE, WD_SOURCE_LINE, WD_SOURCE_FUNCTION);

#    define WD_DUK_RETURN_AND_VERIFY_STACK(duk, ReturnCode, ExpectedStackChange) \
      {                                                                          \
        auto ret = ReturnCode;                                                   \
        WD_DUK_VERIFY_STACK(duk, ExpectedStackChange);                           \
        return ret;                                                              \
      }

#    define WD_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, ExpectedStackChange) \
      WD_DUK_VERIFY_STACK(duk, ExpectedStackChange);                      \
      return;


#  else

#    define WD_DUK_VERIFY_STACK(duk, ExpectedStackChange)

#    define WD_DUK_RETURN_AND_VERIFY_STACK(duk, ReturnCode, ExpectedStackChange) return ReturnCode;

#    define WD_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, ExpectedStackChange) return;

#  endif

class WD_CORE_DLL wdDuktapeHelper
{
public:
  wdDuktapeHelper(duk_context* pContext);
  wdDuktapeHelper(const wdDuktapeHelper& rhs);
  ~wdDuktapeHelper();
  void operator=(const wdDuktapeHelper& rhs);

  /// \name Basics
  ///@{

  /// \brief Returns the raw Duktape context for custom operations.
  WD_ALWAYS_INLINE duk_context* GetContext() const { return m_pContext; }

  /// \brief Implicit conversion to duk_context*
  WD_ALWAYS_INLINE operator duk_context*() const { return m_pContext; }

#  if WD_ENABLED(WD_COMPILE_FOR_DEBUG)
  void VerifyExpectedStackChange(wdInt32 iExpectedStackChange, const char* szFile, wdUInt32 uiLine, const char* szFunction) const;
#  endif

  ///@}
  /// \name Error Handling
  ///@{

  void Error(const wdFormatString& text);

  void LogStackTrace(wdInt32 iErrorObjIdx);


  ///@}
  /// \name Objects / Stash
  ///@{

  void PopStack(wdUInt32 n = 1);

  void PushGlobalObject();

  void PushGlobalStash();

  wdResult PushLocalObject(const char* szName, wdInt32 iParentObjectIndex = -1);

  ///@}
  /// \name Object Properties
  ///@{

  bool HasProperty(const char* szPropertyName, wdInt32 iParentObjectIndex = -1) const;

  bool GetBoolProperty(const char* szPropertyName, bool bFallback, wdInt32 iParentObjectIndex = -1) const;
  wdInt32 GetIntProperty(const char* szPropertyName, wdInt32 iFallback, wdInt32 iParentObjectIndex = -1) const;
  wdUInt32 GetUIntProperty(const char* szPropertyName, wdUInt32 uiFallback, wdInt32 iParentObjectIndex = -1) const;
  float GetFloatProperty(const char* szPropertyName, float fFallback, wdInt32 iParentObjectIndex = -1) const;
  double GetNumberProperty(const char* szPropertyName, double fFallback, wdInt32 iParentObjectIndex = -1) const;
  const char* GetStringProperty(const char* szPropertyName, const char* szFallback, wdInt32 iParentObjectIndex = -1) const;

  void SetBoolProperty(const char* szPropertyName, bool value, wdInt32 iParentObjectIndex = -1) const;
  void SetNumberProperty(const char* szPropertyName, double value, wdInt32 iParentObjectIndex = -1) const;
  void SetStringProperty(const char* szPropertyName, const char* value, wdInt32 iParentObjectIndex = -1) const;

  /// \note If a negative parent index is given, the parent object taken is actually ParentIdx - 1 (obj at idx -1 is the custom object to use)
  void SetCustomProperty(const char* szPropertyName, wdInt32 iParentObjectIndex = -1) const;


  ///@}
  /// \name Global State
  ///@{

  void StorePointerInStash(const char* szKey, void* pPointer);
  void* RetrievePointerFromStash(const char* szKey) const;

  void StoreStringInStash(const char* szKey, const char* value);
  const char* RetrieveStringFromStash(const char* szKey, const char* szFallback = nullptr) const;

  ///@}
  /// \name Type Checks
  ///@{

  bool IsOfType(wdBitflags<wdDuktapeTypeMask> mask, wdInt32 iStackElement = -1) const;
  bool IsBool(wdInt32 iStackElement = -1) const;
  bool IsNumber(wdInt32 iStackElement = -1) const;
  bool IsString(wdInt32 iStackElement = -1) const;
  bool IsNull(wdInt32 iStackElement = -1) const;
  bool IsUndefined(wdInt32 iStackElement = -1) const;
  bool IsObject(wdInt32 iStackElement = -1) const;
  bool IsBuffer(wdInt32 iStackElement = -1) const;
  bool IsPointer(wdInt32 iStackElement = -1) const;
  bool IsNullOrUndefined(wdInt32 iStackElement = -1) const;

  ///@}
  /// \name C Functions
  ///@{

  void RegisterGlobalFunction(const char* szFunctionName, duk_c_function function, wdUInt8 uiNumArguments, wdInt16 iMagicValue = 0);
  void RegisterGlobalFunctionWithVarArgs(const char* szFunctionName, duk_c_function function, wdInt16 iMagicValue = 0);

  void RegisterObjectFunction(
    const char* szFunctionName, duk_c_function function, wdUInt8 uiNumArguments, wdInt32 iParentObjectIndex = -1, wdInt16 iMagicValue = 0);

  wdResult PrepareGlobalFunctionCall(const char* szFunctionName);
  wdResult PrepareObjectFunctionCall(const char* szFunctionName, wdInt32 iParentObjectIndex = -1);
  wdResult CallPreparedFunction();

  wdResult PrepareMethodCall(const char* szMethodName, wdInt32 iParentObjectIndex = -1);
  wdResult CallPreparedMethod();


  ///@}
  /// \name Values / Parameters
  ///@{

  void PushInt(wdInt32 iParam);
  void PushUInt(wdUInt32 uiParam);
  void PushBool(bool bParam);
  void PushNumber(double fParam);
  void PushString(const wdStringView& sParam);
  void PushNull();
  void PushUndefined();
  void PushCustom(wdUInt32 uiNum = 1);

  bool GetBoolValue(wdInt32 iStackElement, bool bFallback = false) const;
  wdInt32 GetIntValue(wdInt32 iStackElement, wdInt32 iFallback = 0) const;
  wdUInt32 GetUIntValue(wdInt32 iStackElement, wdUInt32 uiFallback = 0) const;
  float GetFloatValue(wdInt32 iStackElement, float fFallback = 0) const;
  double GetNumberValue(wdInt32 iStackElement, double fFallback = 0) const;
  const char* GetStringValue(wdInt32 iStackElement, const char* szFallback = "") const;

  ///@}
  /// \name Executing Scripts
  ///@{

  wdResult ExecuteString(const char* szString, const char* szDebugName = "eval");

  wdResult ExecuteStream(wdStreamReader& inout_stream, const char* szDebugName);

  wdResult ExecuteFile(const char* szFile);

  ///@}

public:
#  if WD_ENABLED(WD_COMPILE_FOR_DEBUG)
  void EnableStackChangeVerification() const;
#  endif


protected:
  duk_context* m_pContext = nullptr;
  wdInt32 m_iPushedValues = 0;

#  if WD_ENABLED(WD_COMPILE_FOR_DEBUG)
  wdInt32 m_iStackTopAtStart = -1000;
  mutable bool m_bVerifyStackChange = false;

#  endif
};

#endif
