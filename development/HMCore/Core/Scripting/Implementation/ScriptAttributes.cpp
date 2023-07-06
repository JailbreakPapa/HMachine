#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdScriptExtensionAttribute, 1, wdRTTIDefaultAllocator<wdScriptExtensionAttribute>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("TypeName", m_sTypeName),
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdScriptExtensionAttribute::wdScriptExtensionAttribute() = default;
wdScriptExtensionAttribute::wdScriptExtensionAttribute(const char* szTypeName)
  : m_sTypeName(szTypeName)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdScriptBaseClassFunctionAttribute, 1, wdRTTIDefaultAllocator<wdScriptBaseClassFunctionAttribute>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("Index", m_uiIndex),
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdScriptBaseClassFunctionAttribute::wdScriptBaseClassFunctionAttribute() = default;
wdScriptBaseClassFunctionAttribute::wdScriptBaseClassFunctionAttribute(wdUInt16 uiIndex)
  : m_uiIndex(uiIndex)
{
}
