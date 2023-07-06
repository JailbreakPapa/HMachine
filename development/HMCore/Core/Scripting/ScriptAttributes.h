#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

/// \brief Add this attribute to a class to add script functions to the szTypeName class.
/// This might be necessary if the specified class is not reflected or to separate script functions from the specified class.
class WD_CORE_DLL wdScriptExtensionAttribute : public wdPropertyAttribute
{
  WD_ADD_DYNAMIC_REFLECTION(wdScriptExtensionAttribute, wdPropertyAttribute);

public:
  wdScriptExtensionAttribute();
  wdScriptExtensionAttribute(const char* szTypeName);

  const char* GetTypeName() const { return m_sTypeName; }

private:
  wdUntrackedString m_sTypeName;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Add this attribute to a script function to mark it as a base class function.
/// These are functions that can be entry points to visual scripts or over-writable functions in script languages like e.g. typescript.
class WD_CORE_DLL wdScriptBaseClassFunctionAttribute : public wdPropertyAttribute
{
  WD_ADD_DYNAMIC_REFLECTION(wdScriptBaseClassFunctionAttribute, wdPropertyAttribute);

public:
  wdScriptBaseClassFunctionAttribute();
  wdScriptBaseClassFunctionAttribute(wdUInt16 uiIndex);

  wdUInt16 GetIndex() const { return m_uiIndex; }

private:
  wdUInt16 m_uiIndex;
};
