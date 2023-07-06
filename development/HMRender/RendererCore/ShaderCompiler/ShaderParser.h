#pragma once

#include <Foundation/Strings/String.h>
#include <RendererCore/Declarations.h>

class wdPropertyAttribute;

class WD_RENDERERCORE_DLL wdShaderParser
{
public:
  struct AttributeDefinition
  {
    wdString m_sType;
    wdHybridArray<wdVariant, 8> m_Values;
  };

  struct ParameterDefinition
  {
    const wdRTTI* m_pType = nullptr;
    wdString m_sType;
    wdString m_sName;

    wdHybridArray<AttributeDefinition, 4> m_Attributes;
  };

  struct EnumValue
  {
    wdHashedString m_sValueName;
    wdInt32 m_iValueValue = 0;
  };

  struct EnumDefinition
  {
    wdString m_sName;
    wdUInt32 m_uiDefaultValue = 0;
    wdHybridArray<EnumValue, 16> m_Values;
  };

  static void ParseMaterialParameterSection(
    wdStreamReader& inout_stream, wdHybridArray<ParameterDefinition, 16>& out_parameter, wdHybridArray<EnumDefinition, 4>& out_enumDefinitions);

  static void ParsePermutationSection(
    wdStreamReader& inout_stream, wdHybridArray<wdHashedString, 16>& out_permVars, wdHybridArray<wdPermutationVar, 16>& out_fixedPermVars);
  static void ParsePermutationSection(
    wdStringView sPermutationSection, wdHybridArray<wdHashedString, 16>& out_permVars, wdHybridArray<wdPermutationVar, 16>& out_fixedPermVars);

  static void ParsePermutationVarConfig(wdStringView sPermutationVarConfig, wdVariant& out_defaultValue, EnumDefinition& out_enumDefinition);
};
