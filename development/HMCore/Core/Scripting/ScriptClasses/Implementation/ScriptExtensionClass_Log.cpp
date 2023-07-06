#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_Log.h>

// clang-format off
WD_BEGIN_STATIC_REFLECTED_TYPE(wdScriptExtensionClass_Log, wdNoBase, 1, wdRTTINoAllocator)
{
  WD_BEGIN_FUNCTIONS
  {
    WD_SCRIPT_FUNCTION_PROPERTY(Info, In, "Text", In, "Params")->AddAttributes(new wdDynamicPinAttribute("Params")),
    WD_SCRIPT_FUNCTION_PROPERTY(Warning, In, "Text", In, "Params")->AddAttributes(new wdDynamicPinAttribute("Params")),
    WD_SCRIPT_FUNCTION_PROPERTY(Error, In, "Text", In, "Params")->AddAttributes(new wdDynamicPinAttribute("Params")),
  }
  WD_END_FUNCTIONS;
  WD_BEGIN_ATTRIBUTES
  {
    new wdScriptExtensionAttribute("Log"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_STATIC_REFLECTED_TYPE;
// clang-format on

const char* BuildFormattedText(const char* szText, const wdVariantArray& params, wdStringBuilder& ref_sStorage)
{
  wdHybridArray<wdString, 12> stringStorage;
  stringStorage.Reserve(params.GetCount());
  for (auto& param : params)
  {
    stringStorage.PushBack(param.ConvertTo<wdString>());
  }

  wdHybridArray<wdStringView, 12> stringViews;
  stringViews.Reserve(stringStorage.GetCount());
  for (auto& s : stringStorage)
  {
    stringViews.PushBack(s);
  }

  wdFormatString fs(szText);
  return fs.BuildFormattedText(ref_sStorage, stringViews.GetData(), stringViews.GetCount());
}

// static
void wdScriptExtensionClass_Log::Info(const char* szText, const wdVariantArray& params)
{
  wdStringBuilder sStorage;
  wdLog::Info(BuildFormattedText(szText, params, sStorage));
}

// static
void wdScriptExtensionClass_Log::Warning(const char* szText, const wdVariantArray& params)
{
  wdStringBuilder sStorage;
  wdLog::Warning(BuildFormattedText(szText, params, sStorage));
}

// static
void wdScriptExtensionClass_Log::Error(const char* szText, const wdVariantArray& params)
{
  wdStringBuilder sStorage;
  wdLog::Error(BuildFormattedText(szText, params, sStorage));
}
