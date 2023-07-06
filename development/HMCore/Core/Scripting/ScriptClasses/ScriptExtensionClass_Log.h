#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class WD_CORE_DLL wdScriptExtensionClass_Log
{
public:
  static void Info(const char* szText, const wdVariantArray& params);
  static void Warning(const char* szText, const wdVariantArray& params);
  static void Error(const char* szText, const wdVariantArray& params);
};

WD_DECLARE_REFLECTABLE_TYPE(WD_CORE_DLL, wdScriptExtensionClass_Log);
