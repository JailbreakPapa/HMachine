// NO #pragma once in this file !

template <typename R WD_COMMA_IF(ARG_COUNT) WD_LIST(typename P, ARG_COUNT)>
class wdConsoleFunction<R(WD_LIST(P, ARG_COUNT))> : public wdConsoleFunctionBase
{
public:
  using FUNC = wdDelegate<R(WD_LIST(P, ARG_COUNT))>;

  FUNC m_Func;

  wdConsoleFunction(wdStringView sFunctionName, wdStringView sDescription, FUNC f)
    : wdConsoleFunctionBase(sFunctionName, sDescription)
  {
    m_Func = f;
  }

  wdUInt32 GetNumParameters() const override { return ARG_COUNT; }

  virtual wdVariant::Type::Enum GetParameterType(wdUInt32 uiParam) const override
  {
    WD_ASSERT_DEV(uiParam < GetNumParameters(), "Invalid Parameter Index {0}", uiParam);

#if (ARG_COUNT > 0)

    switch (uiParam)
    {
      case 0:
        return static_cast<wdVariant::Type::Enum>(wdVariant::TypeDeduction<P0>::value);

#  if (ARG_COUNT > 1)
      case 1:
        return static_cast<wdVariant::Type::Enum>(wdVariant::TypeDeduction<P1>::value);
#  endif
#  if (ARG_COUNT > 2)
      case 2:
        return static_cast<wdVariant::Type::Enum>(wdVariant::TypeDeduction<P2>::value);
#  endif
#  if (ARG_COUNT > 3)
      case 3:
        return static_cast<wdVariant::Type::Enum>(wdVariant::TypeDeduction<P3>::value);
#  endif
#  if (ARG_COUNT > 4)
      case 4:
        return static_cast<wdVariant::Type::Enum>(wdVariant::TypeDeduction<P4>::value);
#  endif
#  if (ARG_COUNT > 5)
      case 5:
        return static_cast<wdVariant::Type::Enum>(wdVariant::TypeDeduction<P5>::value);
#  endif
    }

#endif
    return wdVariant::Type::Invalid;
  }

  virtual wdResult Call(wdArrayPtr<wdVariant> params) override
  {
    wdResult r = WD_FAILURE;
    WD_IGNORE_UNUSED(r);

#if (ARG_COUNT > 0)
    P0 param0 = params[0].ConvertTo<P0>(&r);

    if (r.Failed())
      return WD_FAILURE;
#endif

#if (ARG_COUNT > 1)
    P1 param1 = params[1].ConvertTo<P1>(&r);

    if (r.Failed())
      return WD_FAILURE;
#endif

#if (ARG_COUNT > 2)
    P2 param2 = params[2].ConvertTo<P2>(&r);

    if (r.Failed())
      return WD_FAILURE;
#endif

#if (ARG_COUNT > 3)
    P3 param3 = params[3].ConvertTo<P3>(&r);

    if (r.Failed())
      return WD_FAILURE;
#endif

#if (ARG_COUNT > 4)
    P4 param4 = params[4].ConvertTo<P4>(&r);

    if (r.Failed())
      return WD_FAILURE;
#endif

#if (ARG_COUNT > 5)
    P5 param5 = params[5].ConvertTo<P5>(&r);

    if (r.Failed())
      return WD_FAILURE;
#endif

    m_Func(WD_LIST(param, ARG_COUNT));
    return WD_SUCCESS;
  }
};
