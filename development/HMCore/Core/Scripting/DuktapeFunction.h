#pragma once

#include <Core/CoreDLL.h>
#include <Core/Scripting/DuktapeHelper.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

class WD_CORE_DLL wdDuktapeFunction final : public wdDuktapeHelper
{
public:
  wdDuktapeFunction(duk_context* pExistingContext);
  ~wdDuktapeFunction();

  /// \name Retrieving function parameters
  ///@{

  /// Returns how many Parameters were passed to the called C-Function.
  wdUInt32 GetNumVarArgFunctionParameters() const;

  wdInt16 GetFunctionMagicValue() const;

  ///@}

  /// \name Returning values from C function
  ///@{

  wdInt32 ReturnVoid();
  wdInt32 ReturnNull();
  wdInt32 ReturnUndefined();
  wdInt32 ReturnBool(bool value);
  wdInt32 ReturnInt(wdInt32 value);
  wdInt32 ReturnUInt(wdUInt32 value);
  wdInt32 ReturnFloat(float value);
  wdInt32 ReturnNumber(double value);
  wdInt32 ReturnString(wdStringView value);
  wdInt32 ReturnCustom();

  ///@}

private:
  bool m_bDidReturnValue = false;
};

#endif
