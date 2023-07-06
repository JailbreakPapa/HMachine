#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Utilities/EnumerableClass.h>

/// \brief Base class for all types of wdConsoleFunction, represents functions to be exposed to wdConsole.
///
/// Console functions are similar to wdCVar's in that they can be executed from the wdConsole.
/// A console function can wrap many different types of functions with differing number and types of parameters.
/// wdConsoleFunction uses an wdDelegate internally to store the function reference, so even member functions would be possible.
///
/// All console functions are enumerable, as their base class wdConsoleFunctionBase is an wdEnumerable class.
///
/// Console functions can have between zero and six parameters. The LuaInterpreter for wdConsole only supports parameter types
/// (unsigned) int, float/double, bool and string and uses the conversion feature of wdVariant to map the lua input to the final function.
///
/// To make a function available as a console function, create a global variable of type wdConsoleFunction with the proper template
/// arguments to mirror its parameters and return type.
/// Note that although functions with return types are accepted, the return value is currently always ignored.
///
/// \code{.cpp}
///   void MyConsoleFunc1(int a, float b, wdStringView sz) { ... }
///   wdConsoleFunction<void ()> ConFunc_MyConsoleFunc1("MyConsoleFunc1", "()", MyConsoleFunc1);
///
///   int MyConsoleFunc2(int a, float b, wdStringView sz) { ... }
///   wdConsoleFunction<int (int, float, wdString)> ConFunc_MyConsoleFunc2("MyConsoleFunc2", "(int a, float b, string c)", MyConsoleFunc2);
/// \endcode
///
/// Here the global function MyConsoleFunc2 is exposed to the console. The return value type and parameter types are passed as template
/// arguments. ConFunc_MyConsoleFunc2 is now the global variable that represents the function for the console.
/// The first string is the name with which the function is exposed, which is also used for auto-completion.
/// The second string is the description of the function. Here we inserted the parameter list with types, so that the user knows how to
/// use it. Finally the last parameter is the actual function to expose.
class WD_CORE_DLL wdConsoleFunctionBase : public wdEnumerable<wdConsoleFunctionBase>
{
  WD_DECLARE_ENUMERABLE_CLASS(wdConsoleFunctionBase);

public:
  /// \brief The constructor takes the function name and description as it should appear in the console.
  wdConsoleFunctionBase(wdStringView sFunctionName, wdStringView sDescription)
    : m_sFunctionName(sFunctionName)
    , m_sDescription(sDescription)
  {
  }

  /// \brief Returns the name of the function as it should be exposed in the console.
  wdStringView GetName() const { return m_sFunctionName; }

  /// \brief Returns the description of the function as it should appear in the console.
  wdStringView GetDescription() const { return m_sDescription; }

  /// \brief Returns the number of parameters that this function takes.
  virtual wdUInt32 GetNumParameters() const = 0;

  /// \brief Returns the type of the n-th parameter.
  virtual wdVariant::Type::Enum GetParameterType(wdUInt32 uiParam) const = 0;

  /// \brief Calls the function. Each parameter must be put into an wdVariant and all of them are passed along as an array.
  ///
  /// Returns WD_FAILURE, if the number of parameters did not match, or any parameter was not convertible to the actual type that
  /// the function expects.
  virtual wdResult Call(wdArrayPtr<wdVariant> params) = 0;

private:
  wdStringView m_sFunctionName;
  wdStringView m_sDescription;
};


/// \brief Implements the functionality of wdConsoleFunctionBase for functions with different parameter types. See wdConsoleFunctionBase for more
/// details.
template <typename R>
class wdConsoleFunction : public wdConsoleFunctionBase
{
};


#define ARG_COUNT 0
#include <Core/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 1
#include <Core/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 2
#include <Core/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 3
#include <Core/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 4
#include <Core/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 5
#include <Core/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 6
#include <Core/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT
