#include <RendererCore/RendererCorePCH.h>

#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <RendererCore/Shader/Implementation/Helper.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>

using namespace wdTokenParseUtils;

namespace
{
  static wdHashTable<wdStringView, const wdRTTI*> s_NameToTypeTable;

  void InitializeTables()
  {
    if (!s_NameToTypeTable.IsEmpty())
      return;

    s_NameToTypeTable.Insert("float", wdGetStaticRTTI<float>());
    s_NameToTypeTable.Insert("float2", wdGetStaticRTTI<wdVec2>());
    s_NameToTypeTable.Insert("float3", wdGetStaticRTTI<wdVec3>());
    s_NameToTypeTable.Insert("float4", wdGetStaticRTTI<wdVec4>());
    s_NameToTypeTable.Insert("int", wdGetStaticRTTI<int>());
    s_NameToTypeTable.Insert("int2", wdGetStaticRTTI<wdVec2I32>());
    s_NameToTypeTable.Insert("int3", wdGetStaticRTTI<wdVec3I32>());
    s_NameToTypeTable.Insert("int4", wdGetStaticRTTI<wdVec4I32>());
    s_NameToTypeTable.Insert("uint", wdGetStaticRTTI<wdUInt32>());
    s_NameToTypeTable.Insert("uint2", wdGetStaticRTTI<wdVec2U32>());
    s_NameToTypeTable.Insert("uint3", wdGetStaticRTTI<wdVec3U32>());
    s_NameToTypeTable.Insert("uint4", wdGetStaticRTTI<wdVec4U32>());
    s_NameToTypeTable.Insert("bool", wdGetStaticRTTI<bool>());
    s_NameToTypeTable.Insert("Color", wdGetStaticRTTI<wdColor>());
    /// \todo Are we going to support linear UB colors ?
    s_NameToTypeTable.Insert("Texture2D", wdGetStaticRTTI<wdString>());
    s_NameToTypeTable.Insert("Texture3D", wdGetStaticRTTI<wdString>());
    s_NameToTypeTable.Insert("TextureCube", wdGetStaticRTTI<wdString>());
  }

  const wdRTTI* GetType(wdStringView sType)
  {
    InitializeTables();

    const wdRTTI* pType = nullptr;
    s_NameToTypeTable.TryGetValue(sType, pType);
    return pType;
  }

  wdVariant ParseValue(const TokenStream& tokens, wdUInt32& ref_uiCurToken)
  {
    wdUInt32 uiValueToken = ref_uiCurToken;

    if (Accept(tokens, ref_uiCurToken, wdTokenType::String1, &uiValueToken) || Accept(tokens, ref_uiCurToken, wdTokenType::String2, &uiValueToken))
    {
      wdStringBuilder sValue = tokens[uiValueToken]->m_DataView;
      sValue.Trim("\"'");

      return wdVariant(sValue.GetData());
    }

    if (Accept(tokens, ref_uiCurToken, wdTokenType::Integer, &uiValueToken))
    {
      wdString sValue = tokens[uiValueToken]->m_DataView;

      wdInt64 iValue = 0;
      if (sValue.StartsWith_NoCase("0x"))
      {
        wdUInt32 uiValue32 = 0;
        wdConversionUtils::ConvertHexStringToUInt32(sValue, uiValue32).IgnoreResult();

        iValue = uiValue32;
      }
      else
      {
        wdConversionUtils::StringToInt64(sValue, iValue).IgnoreResult();
      }

      return wdVariant(iValue);
    }

    if (Accept(tokens, ref_uiCurToken, wdTokenType::Float, &uiValueToken))
    {
      wdString sValue = tokens[uiValueToken]->m_DataView;

      double fValue = 0;
      wdConversionUtils::StringToFloat(sValue, fValue).IgnoreResult();

      return wdVariant(fValue);
    }

    if (Accept(tokens, ref_uiCurToken, "true", &uiValueToken) || Accept(tokens, ref_uiCurToken, "false", &uiValueToken))
    {
      bool bValue = tokens[uiValueToken]->m_DataView == "true";
      return wdVariant(bValue);
    }

    auto& dataView = tokens[ref_uiCurToken]->m_DataView;
    if (tokens[ref_uiCurToken]->m_iType == wdTokenType::Identifier && wdStringUtils::IsValidIdentifierName(dataView.GetStartPointer(), dataView.GetEndPointer()))
    {
      // complex type constructor
      const wdRTTI* pType = nullptr;
      if (!s_NameToTypeTable.TryGetValue(dataView, pType))
      {
        wdLog::Error("Invalid type name '{}'", dataView);
        return wdVariant();
      }

      ++ref_uiCurToken;
      Accept(tokens, ref_uiCurToken, "(");

      wdHybridArray<wdVariant, 8> constructorArgs;

      while (!Accept(tokens, ref_uiCurToken, ")"))
      {
        wdVariant value = ParseValue(tokens, ref_uiCurToken);
        if (value.IsValid())
        {
          constructorArgs.PushBack(value);
        }
        else
        {
          wdLog::Error("Invalid arguments for constructor '{}'", pType->GetTypeName());
          return WD_FAILURE;
        }

        Accept(tokens, ref_uiCurToken, ",");
      }

      // find matching constructor
      auto& functions = pType->GetFunctions();
      for (auto pFunc : functions)
      {
        if (pFunc->GetFunctionType() == wdFunctionType::Constructor && pFunc->GetArgumentCount() == constructorArgs.GetCount())
        {
          wdHybridArray<wdVariant, 8> convertedArgs;
          bool bAllArgsValid = true;

          for (wdUInt32 uiArg = 0; uiArg < pFunc->GetArgumentCount(); ++uiArg)
          {
            const wdRTTI* pArgType = pFunc->GetArgumentType(uiArg);
            wdResult conversionResult = WD_FAILURE;
            convertedArgs.PushBack(constructorArgs[uiArg].ConvertTo(pArgType->GetVariantType(), &conversionResult));
            if (conversionResult.Failed())
            {
              bAllArgsValid = false;
              break;
            }
          }

          if (bAllArgsValid)
          {
            wdVariant result;
            pFunc->Execute(nullptr, convertedArgs, result);

            if (result.IsValid())
            {
              return result;
            }
          }
        }
      }
    }

    return wdVariant();
  }

  wdResult ParseAttribute(const TokenStream& tokens, wdUInt32& ref_uiCurToken, wdShaderParser::ParameterDefinition& out_parameterDefinition)
  {
    if (!Accept(tokens, ref_uiCurToken, "@"))
    {
      return WD_FAILURE;
    }

    wdUInt32 uiTypeToken = ref_uiCurToken;
    if (!Accept(tokens, ref_uiCurToken, wdTokenType::Identifier, &uiTypeToken))
    {
      return WD_FAILURE;
    }

    wdShaderParser::AttributeDefinition& attributeDef = out_parameterDefinition.m_Attributes.ExpandAndGetRef();
    attributeDef.m_sType = tokens[uiTypeToken]->m_DataView;

    Accept(tokens, ref_uiCurToken, "(");

    while (!Accept(tokens, ref_uiCurToken, ")"))
    {
      wdVariant value = ParseValue(tokens, ref_uiCurToken);
      if (value.IsValid())
      {
        attributeDef.m_Values.PushBack(value);
      }
      else
      {
        wdLog::Error("Invalid arguments for attribute '{}'", attributeDef.m_sType);
        return WD_FAILURE;
      }

      Accept(tokens, ref_uiCurToken, ",");
    }

    return WD_SUCCESS;
  }

  wdResult ParseParameter(const TokenStream& tokens, wdUInt32& ref_uiCurToken, wdShaderParser::ParameterDefinition& out_parameterDefinition)
  {
    wdUInt32 uiTypeToken = ref_uiCurToken;
    if (!Accept(tokens, ref_uiCurToken, wdTokenType::Identifier, &uiTypeToken))
    {
      return WD_FAILURE;
    }

    out_parameterDefinition.m_sType = tokens[uiTypeToken]->m_DataView;
    out_parameterDefinition.m_pType = GetType(out_parameterDefinition.m_sType);

    wdUInt32 uiNameToken = ref_uiCurToken;
    if (!Accept(tokens, ref_uiCurToken, wdTokenType::Identifier, &uiNameToken))
    {
      return WD_FAILURE;
    }

    out_parameterDefinition.m_sName = tokens[uiNameToken]->m_DataView;

    while (!Accept(tokens, ref_uiCurToken, ";"))
    {
      if (ParseAttribute(tokens, ref_uiCurToken, out_parameterDefinition).Failed())
      {
        return WD_FAILURE;
      }
    }

    return WD_SUCCESS;
  }

  wdResult ParseEnum(const TokenStream& tokens, wdUInt32& ref_uiCurToken, wdShaderParser::EnumDefinition& out_enumDefinition, bool bCheckPrefix)
  {
    if (!Accept(tokens, ref_uiCurToken, "enum"))
    {
      return WD_FAILURE;
    }

    wdUInt32 uiNameToken = ref_uiCurToken;
    if (!Accept(tokens, ref_uiCurToken, wdTokenType::Identifier, &uiNameToken))
    {
      return WD_FAILURE;
    }

    out_enumDefinition.m_sName = tokens[uiNameToken]->m_DataView;
    wdStringBuilder sEnumPrefix(out_enumDefinition.m_sName, "_");

    if (!Accept(tokens, ref_uiCurToken, "{"))
    {
      wdLog::Error("Opening bracket expected for enum definition.");
      return WD_FAILURE;
    }

    wdUInt32 uiDefaultValue = 0;
    wdUInt32 uiCurrentValue = 0;

    while (true)
    {
      wdUInt32 uiValueNameToken = ref_uiCurToken;
      if (!Accept(tokens, ref_uiCurToken, wdTokenType::Identifier, &uiValueNameToken))
      {
        return WD_FAILURE;
      }

      wdStringView sValueName = tokens[uiValueNameToken]->m_DataView;

      if (Accept(tokens, ref_uiCurToken, "="))
      {
        wdUInt32 uiValueToken = ref_uiCurToken;
        Accept(tokens, ref_uiCurToken, wdTokenType::Integer, &uiValueToken);

        wdInt32 iValue = 0;
        if (wdConversionUtils::StringToInt(tokens[uiValueToken]->m_DataView.GetStartPointer(), iValue).Succeeded() && iValue >= 0)
        {
          uiCurrentValue = iValue;
        }
        else
        {
          wdLog::Error("Invalid enum value '{0}'. Only positive numbers are allowed.", tokens[uiValueToken]->m_DataView);
        }
      }

      if (sValueName.IsEqual_NoCase("default"))
      {
        uiDefaultValue = uiCurrentValue;
      }
      else
      {
        if (bCheckPrefix && !sValueName.StartsWith(sEnumPrefix))
        {
          wdLog::Error("Enum value does not start with the expected enum name as prefix: '{0}'", sEnumPrefix);
        }

        auto& ev = out_enumDefinition.m_Values.ExpandAndGetRef();

        const wdStringBuilder sFinalName = sValueName;
        ev.m_sValueName.Assign(sFinalName.GetData());
        ev.m_iValueValue = static_cast<wdInt32>(uiCurrentValue);
      }

      if (Accept(tokens, ref_uiCurToken, ","))
      {
        ++uiCurrentValue;
      }
      else
      {
        break;
      }

      if (Accept(tokens, ref_uiCurToken, "}"))
        goto after_braces;
    }

    if (!Accept(tokens, ref_uiCurToken, "}"))
    {
      wdLog::Error("Closing bracket expected for enum definition.");
      return WD_FAILURE;
    }

  after_braces:

    out_enumDefinition.m_uiDefaultValue = uiDefaultValue;

    Accept(tokens, ref_uiCurToken, ";");

    return WD_SUCCESS;
  }

  void SkipWhitespace(wdStringView& s)
  {
    while (s.IsValid() && wdStringUtils::IsWhiteSpace(s.GetCharacter()))
    {
      ++s;
    }
  }
} // namespace

// static
void wdShaderParser::ParseMaterialParameterSection(wdStreamReader& inout_stream, wdHybridArray<ParameterDefinition, 16>& out_parameter, wdHybridArray<EnumDefinition, 4>& out_enumDefinitions)
{
  wdString sContent;
  sContent.ReadAll(inout_stream);

  wdShaderHelper::wdTextSectionizer Sections;
  wdShaderHelper::GetShaderSections(sContent.GetData(), Sections);

  wdUInt32 uiFirstLine = 0;
  wdStringView s = Sections.GetSectionContent(wdShaderHelper::wdShaderSections::MATERIALPARAMETER, uiFirstLine);

  wdTokenizer tokenizer;
  tokenizer.Tokenize(wdArrayPtr<const wdUInt8>((const wdUInt8*)s.GetStartPointer(), s.GetElementCount()), wdLog::GetThreadLocalLogSystem());

  TokenStream tokens;
  tokenizer.GetAllLines(tokens);

  wdUInt32 uiCurToken = 0;

  while (!Accept(tokens, uiCurToken, wdTokenType::EndOfFile))
  {
    EnumDefinition enumDef;
    if (ParseEnum(tokens, uiCurToken, enumDef, false).Succeeded())
    {
      WD_ASSERT_DEV(!enumDef.m_sName.IsEmpty(), "");

      out_enumDefinitions.PushBack(std::move(enumDef));
      continue;
    }

    ParameterDefinition paramDef;
    if (ParseParameter(tokens, uiCurToken, paramDef).Succeeded())
    {
      out_parameter.PushBack(std::move(paramDef));
      continue;
    }

    wdLog::Error("Invalid token in material parameter section '{}'", tokens[uiCurToken]->m_DataView);
    break;
  }
}

// static
void wdShaderParser::ParsePermutationSection(wdStreamReader& inout_stream, wdHybridArray<wdHashedString, 16>& out_permVars, wdHybridArray<wdPermutationVar, 16>& out_fixedPermVars)
{
  wdString sContent;
  sContent.ReadAll(inout_stream);

  wdShaderHelper::wdTextSectionizer Sections;
  wdShaderHelper::GetShaderSections(sContent.GetData(), Sections);

  wdUInt32 uiFirstLine = 0;
  wdStringView sPermutations = Sections.GetSectionContent(wdShaderHelper::wdShaderSections::PERMUTATIONS, uiFirstLine);
  ParsePermutationSection(sPermutations, out_permVars, out_fixedPermVars);
}

// static
void wdShaderParser::ParsePermutationSection(wdStringView s, wdHybridArray<wdHashedString, 16>& out_permVars, wdHybridArray<wdPermutationVar, 16>& out_fixedPermVars)
{
  out_permVars.Clear();
  out_fixedPermVars.Clear();

  wdTokenizer tokenizer;
  tokenizer.Tokenize(wdArrayPtr<const wdUInt8>((const wdUInt8*)s.GetStartPointer(), s.GetElementCount()), wdLog::GetThreadLocalLogSystem());

  enum class State
  {
    Idle,
    HasName,
    HasEqual,
    HasValue
  };

  State state = State::Idle;
  wdStringBuilder sToken, sVarName;

  for (const auto& token : tokenizer.GetTokens())
  {
    if (token.m_iType == wdTokenType::Whitespace || token.m_iType == wdTokenType::BlockComment || token.m_iType == wdTokenType::LineComment)
      continue;

    if (token.m_iType == wdTokenType::String1 || token.m_iType == wdTokenType::String2)
    {
      sToken = token.m_DataView;
      wdLog::Error("Strings are not allowed in the permutation section: '{0}'", sToken);
      return;
    }

    if (token.m_iType == wdTokenType::Newline || token.m_iType == wdTokenType::EndOfFile)
    {
      if (state == State::HasEqual)
      {
        wdLog::Error("Missing assignment value in permutation section");
        return;
      }

      if (state == State::HasName)
      {
        out_permVars.ExpandAndGetRef().Assign(sVarName.GetData());
      }

      state = State::Idle;
      continue;
    }

    sToken = token.m_DataView;

    if (token.m_iType == wdTokenType::NonIdentifier)
    {
      if (sToken == "=" && state == State::HasName)
      {
        state = State::HasEqual;
        continue;
      }
    }
    else if (token.m_iType == wdTokenType::Identifier)
    {
      if (state == State::Idle)
      {
        sVarName = sToken;
        state = State::HasName;
        continue;
      }

      if (state == State::HasEqual)
      {
        auto& res = out_fixedPermVars.ExpandAndGetRef();
        res.m_sName.Assign(sVarName.GetData());
        res.m_sValue.Assign(sToken.GetData());
        state = State::HasValue;
        continue;
      }
    }

    wdLog::Error("Invalid permutation section at token '{0}'", sToken);
  }
}

// static
void wdShaderParser::ParsePermutationVarConfig(wdStringView s, wdVariant& out_defaultValue, EnumDefinition& out_enumDefinition)
{
  SkipWhitespace(s);

  wdStringBuilder name;

  if (s.StartsWith("bool"))
  {
    bool bDefaultValue = false;

    const char* szDefaultValue = s.FindSubString("=");
    if (szDefaultValue != nullptr)
    {
      name.SetSubString_FromTo(s.GetStartPointer() + 4, szDefaultValue);

      ++szDefaultValue;
      wdConversionUtils::StringToBool(szDefaultValue, bDefaultValue).IgnoreResult();
    }
    else
    {
      name.SetSubString_FromTo(s.GetStartPointer() + 4, s.GetEndPointer());
    }

    name.Trim(" \t\r\n");
    out_enumDefinition.m_sName = name;
    out_defaultValue = bDefaultValue;
  }
  else if (s.StartsWith("enum"))
  {
    wdTokenizer tokenizer;
    tokenizer.Tokenize(wdArrayPtr<const wdUInt8>((const wdUInt8*)s.GetStartPointer(), s.GetElementCount()), wdLog::GetThreadLocalLogSystem());

    TokenStream tokens;
    tokenizer.GetAllLines(tokens);

    wdUInt32 uiCurToken = 0;
    if (ParseEnum(tokens, uiCurToken, out_enumDefinition, true).Failed())
    {
      wdLog::Error("Invalid enum PermutationVar definition.");
    }
    else
    {
      WD_ASSERT_DEV(!out_enumDefinition.m_sName.IsEmpty(), "");

      out_defaultValue = out_enumDefinition.m_uiDefaultValue;
    }
  }
  else
  {
    wdLog::Error("Unknown permutation var type");
  }
}



WD_STATICLINK_FILE(RendererCore, RendererCore_ShaderCompiler_Implementation_ShaderParser);
