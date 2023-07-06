#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Shader/ShaderPermutationBinary.h>

struct wdShaderStateVersion
{
  enum Enum : wdUInt32
  {
    Version0 = 0,
    Version1,
    Version2,
    Version3,

    ENUM_COUNT,
    Current = ENUM_COUNT - 1
  };
};

void wdShaderStateResourceDescriptor::Save(wdStreamWriter& inout_stream) const
{
  inout_stream << (wdUInt32)wdShaderStateVersion::Current;

  // Blend State
  {
    inout_stream << m_BlendDesc.m_bAlphaToCoverage;
    inout_stream << m_BlendDesc.m_bIndependentBlend;

    const wdUInt8 iBlends = m_BlendDesc.m_bIndependentBlend ? WD_GAL_MAX_RENDERTARGET_COUNT : 1;
    inout_stream << iBlends; // in case WD_GAL_MAX_RENDERTARGET_COUNT ever changes

    for (wdUInt32 b = 0; b < iBlends; ++b)
    {
      inout_stream << m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_bBlendingEnabled;
      inout_stream << (wdUInt8)m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_BlendOp;
      inout_stream << (wdUInt8)m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_BlendOpAlpha;
      inout_stream << (wdUInt8)m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_DestBlend;
      inout_stream << (wdUInt8)m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_DestBlendAlpha;
      inout_stream << (wdUInt8)m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_SourceBlend;
      inout_stream << (wdUInt8)m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_SourceBlendAlpha;
      inout_stream << m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_uiWriteMask;
    }
  }

  // Depth Stencil State
  {
    inout_stream << (wdUInt8)m_DepthStencilDesc.m_DepthTestFunc;
    inout_stream << m_DepthStencilDesc.m_bDepthTest;
    inout_stream << m_DepthStencilDesc.m_bDepthWrite;
    inout_stream << m_DepthStencilDesc.m_bSeparateFrontAndBack;
    inout_stream << m_DepthStencilDesc.m_bStencilTest;
    inout_stream << m_DepthStencilDesc.m_uiStencilReadMask;
    inout_stream << m_DepthStencilDesc.m_uiStencilWriteMask;
    inout_stream << (wdUInt8)m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp;
    inout_stream << (wdUInt8)m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp;
    inout_stream << (wdUInt8)m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp;
    inout_stream << (wdUInt8)m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc;
    inout_stream << (wdUInt8)m_DepthStencilDesc.m_BackFaceStencilOp.m_DepthFailOp;
    inout_stream << (wdUInt8)m_DepthStencilDesc.m_BackFaceStencilOp.m_FailOp;
    inout_stream << (wdUInt8)m_DepthStencilDesc.m_BackFaceStencilOp.m_PassOp;
    inout_stream << (wdUInt8)m_DepthStencilDesc.m_BackFaceStencilOp.m_StencilFunc;
  }

  // Rasterizer State
  {
    inout_stream << m_RasterizerDesc.m_bFrontCounterClockwise;
    inout_stream << m_RasterizerDesc.m_bScissorTest;
    inout_stream << m_RasterizerDesc.m_bWireFrame;
    inout_stream << (wdUInt8)m_RasterizerDesc.m_CullMode;
    inout_stream << m_RasterizerDesc.m_fDepthBiasClamp;
    inout_stream << m_RasterizerDesc.m_fSlopeScaledDepthBias;
    inout_stream << m_RasterizerDesc.m_iDepthBias;
    inout_stream << m_RasterizerDesc.m_bConservativeRasterization;
  }
}

void wdShaderStateResourceDescriptor::Load(wdStreamReader& inout_stream)
{
  wdUInt32 uiVersion = 0;
  inout_stream >> uiVersion;

  WD_ASSERT_DEV(uiVersion >= wdShaderStateVersion::Version1 && uiVersion <= wdShaderStateVersion::Current, "Invalid version {0}", uiVersion);

  // Blend State
  {
    inout_stream >> m_BlendDesc.m_bAlphaToCoverage;
    inout_stream >> m_BlendDesc.m_bIndependentBlend;

    wdUInt8 iBlends = 0;
    inout_stream >> iBlends; // in case WD_GAL_MAX_RENDERTARGET_COUNT ever changes

    for (wdUInt32 b = 0; b < iBlends; ++b)
    {
      wdUInt8 uiTemp;
      inout_stream >> m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_bBlendingEnabled;
      inout_stream >> uiTemp;
      m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_BlendOp = (wdGALBlendOp::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_BlendOpAlpha = (wdGALBlendOp::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_DestBlend = (wdGALBlend::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_DestBlendAlpha = (wdGALBlend::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_SourceBlend = (wdGALBlend::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_SourceBlendAlpha = (wdGALBlend::Enum)uiTemp;
      inout_stream >> m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_uiWriteMask;
    }
  }

  // Depth Stencil State
  {
    wdUInt8 uiTemp = 0;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_DepthTestFunc = (wdGALCompareFunc::Enum)uiTemp;
    inout_stream >> m_DepthStencilDesc.m_bDepthTest;
    inout_stream >> m_DepthStencilDesc.m_bDepthWrite;
    inout_stream >> m_DepthStencilDesc.m_bSeparateFrontAndBack;
    inout_stream >> m_DepthStencilDesc.m_bStencilTest;
    inout_stream >> m_DepthStencilDesc.m_uiStencilReadMask;
    inout_stream >> m_DepthStencilDesc.m_uiStencilWriteMask;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp = (wdGALStencilOp::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp = (wdGALStencilOp::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp = (wdGALStencilOp::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc = (wdGALCompareFunc::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_BackFaceStencilOp.m_DepthFailOp = (wdGALStencilOp::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_BackFaceStencilOp.m_FailOp = (wdGALStencilOp::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_BackFaceStencilOp.m_PassOp = (wdGALStencilOp::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_BackFaceStencilOp.m_StencilFunc = (wdGALCompareFunc::Enum)uiTemp;
  }

  // Rasterizer State
  {
    wdUInt8 uiTemp = 0;

    if (uiVersion < wdShaderStateVersion::Version2)
    {
      bool dummy;
      inout_stream >> dummy;
    }

    inout_stream >> m_RasterizerDesc.m_bFrontCounterClockwise;

    if (uiVersion < wdShaderStateVersion::Version2)
    {
      bool dummy;
      inout_stream >> dummy;
      inout_stream >> dummy;
    }

    inout_stream >> m_RasterizerDesc.m_bScissorTest;
    inout_stream >> m_RasterizerDesc.m_bWireFrame;
    inout_stream >> uiTemp;
    m_RasterizerDesc.m_CullMode = (wdGALCullMode::Enum)uiTemp;
    inout_stream >> m_RasterizerDesc.m_fDepthBiasClamp;
    inout_stream >> m_RasterizerDesc.m_fSlopeScaledDepthBias;
    inout_stream >> m_RasterizerDesc.m_iDepthBias;

    if (uiVersion >= wdShaderStateVersion::Version3)
    {
      inout_stream >> m_RasterizerDesc.m_bConservativeRasterization;
    }
  }
}

wdUInt32 wdShaderStateResourceDescriptor::CalculateHash() const
{
  return m_BlendDesc.CalculateHash() + m_RasterizerDesc.CalculateHash() + m_DepthStencilDesc.CalculateHash();
}

static const char* InsertNumber(const char* szString, wdUInt32 uiNumber, wdStringBuilder& ref_sTemp)
{
  ref_sTemp.Format(szString, uiNumber);
  return ref_sTemp.GetData();
}

#if WD_ENABLED(WD_COMPILE_FOR_DEBUG)
static wdSet<wdString> s_AllAllowedVariables;
#endif

static bool GetBoolStateVariable(const wdMap<wdString, wdString>& variables, const char* szVariable, bool bDefValue)
{
#if WD_ENABLED(WD_COMPILE_FOR_DEBUG)
  s_AllAllowedVariables.Insert(szVariable);
#endif

  auto it = variables.Find(szVariable);

  if (!it.IsValid())
    return bDefValue;

  if (it.Value() == "true")
    return true;
  if (it.Value() == "false")
    return false;

  wdLog::Error("Shader state variable '{0}' is set to invalid value '{1}'. Should be 'true' or 'false'", szVariable, it.Value());
  return bDefValue;
}

static wdInt32 GetEnumStateVariable(
  const wdMap<wdString, wdString>& variables, const wdMap<wdString, wdInt32>& values, const char* szVariable, wdInt32 iDefValue)
{
#if WD_ENABLED(WD_COMPILE_FOR_DEBUG)
  s_AllAllowedVariables.Insert(szVariable);
#endif

  auto it = variables.Find(szVariable);

  if (!it.IsValid())
    return iDefValue;

  auto itVal = values.Find(it.Value());
  if (!itVal.IsValid())
  {
    wdStringBuilder valid;
    for (auto vv = values.GetIterator(); vv.IsValid(); ++vv)
    {
      valid.Append(" ", vv.Key());
    }

    wdLog::Error("Shader state variable '{0}' is set to invalid value '{1}'. Valid values are:{2}", szVariable, it.Value(), valid);
    return iDefValue;
  }

  return itVal.Value();
}

static float GetFloatStateVariable(const wdMap<wdString, wdString>& variables, const char* szVariable, float fDefValue)
{
#if WD_ENABLED(WD_COMPILE_FOR_DEBUG)
  s_AllAllowedVariables.Insert(szVariable);
#endif

  auto it = variables.Find(szVariable);

  if (!it.IsValid())
    return fDefValue;

  double result = 0;
  if (wdConversionUtils::StringToFloat(it.Value(), result).Failed())
  {
    wdLog::Error("Shader state variable '{0}' is not a valid float value: '{1}'.", szVariable, it.Value());
    return fDefValue;
  }

  return (float)result;
}

static wdInt32 GetIntStateVariable(const wdMap<wdString, wdString>& variables, const char* szVariable, wdInt32 iDefValue)
{
#if WD_ENABLED(WD_COMPILE_FOR_DEBUG)
  s_AllAllowedVariables.Insert(szVariable);
#endif

  auto it = variables.Find(szVariable);

  if (!it.IsValid())
    return iDefValue;

  wdInt32 result = 0;
  if (wdConversionUtils::StringToInt(it.Value(), result).Failed())
  {
    wdLog::Error("Shader state variable '{0}' is not a valid int value: '{1}'.", szVariable, it.Value());
    return iDefValue;
  }

  return result;
}

// Global variables don't use memory tracking, so these won't reported as memory leaks.
static wdMap<wdString, wdInt32> StateValuesBlend;
static wdMap<wdString, wdInt32> StateValuesBlendOp;
static wdMap<wdString, wdInt32> StateValuesCullMode;
static wdMap<wdString, wdInt32> StateValuesCompareFunc;
static wdMap<wdString, wdInt32> StateValuesStencilOp;

wdResult wdShaderStateResourceDescriptor::Load(const char* szSource)
{
  wdMap<wdString, wdString> VariableValues;

  // extract all state assignments
  {
    wdStringBuilder sSource = szSource;

    wdHybridArray<wdStringView, 32> allAssignments;
    wdHybridArray<wdStringView, 4> components;
    sSource.Split(false, allAssignments, "\n", ";", "\r");

    wdStringBuilder temp1, temp2;
    for (const wdStringView& ass : allAssignments)
    {
      temp1 = ass;
      temp1.Trim(" \t\r\n;");
      if (temp1.IsEmpty())
        continue;

      temp1.Split(false, components, " ", "\t", "=", "\r");

      if (components.GetCount() != 2)
      {
        wdLog::Error("Malformed shader state assignment: '{0}'", temp1);
        continue;
      }

      temp1 = components[0];
      temp2 = components[1];

      VariableValues[temp1] = temp2;
    }
  }

  if (StateValuesBlend.IsEmpty())
  {
    // wdGALBlend
    {
      StateValuesBlend["Blend_Zero"] = wdGALBlend::Zero;
      StateValuesBlend["Blend_One"] = wdGALBlend::One;
      StateValuesBlend["Blend_SrcColor"] = wdGALBlend::SrcColor;
      StateValuesBlend["Blend_InvSrcColor"] = wdGALBlend::InvSrcColor;
      StateValuesBlend["Blend_SrcAlpha"] = wdGALBlend::SrcAlpha;
      StateValuesBlend["Blend_InvSrcAlpha"] = wdGALBlend::InvSrcAlpha;
      StateValuesBlend["Blend_DestAlpha"] = wdGALBlend::DestAlpha;
      StateValuesBlend["Blend_InvDestAlpha"] = wdGALBlend::InvDestAlpha;
      StateValuesBlend["Blend_DestColor"] = wdGALBlend::DestColor;
      StateValuesBlend["Blend_InvDestColor"] = wdGALBlend::InvDestColor;
      StateValuesBlend["Blend_SrcAlphaSaturated"] = wdGALBlend::SrcAlphaSaturated;
      StateValuesBlend["Blend_BlendFactor"] = wdGALBlend::BlendFactor;
      StateValuesBlend["Blend_InvBlendFactor"] = wdGALBlend::InvBlendFactor;
    }

    // wdGALBlendOp
    {
      StateValuesBlendOp["BlendOp_Add"] = wdGALBlendOp::Add;
      StateValuesBlendOp["BlendOp_Subtract"] = wdGALBlendOp::Subtract;
      StateValuesBlendOp["BlendOp_RevSubtract"] = wdGALBlendOp::RevSubtract;
      StateValuesBlendOp["BlendOp_Min"] = wdGALBlendOp::Min;
      StateValuesBlendOp["BlendOp_Max"] = wdGALBlendOp::Max;
    }

    // wdGALCullMode
    {
      StateValuesCullMode["CullMode_None"] = wdGALCullMode::None;
      StateValuesCullMode["CullMode_Front"] = wdGALCullMode::Front;
      StateValuesCullMode["CullMode_Back"] = wdGALCullMode::Back;
    }

    // wdGALCompareFunc
    {
      StateValuesCompareFunc["CompareFunc_Never"] = wdGALCompareFunc::Never;
      StateValuesCompareFunc["CompareFunc_Less"] = wdGALCompareFunc::Less;
      StateValuesCompareFunc["CompareFunc_Equal"] = wdGALCompareFunc::Equal;
      StateValuesCompareFunc["CompareFunc_LessEqual"] = wdGALCompareFunc::LessEqual;
      StateValuesCompareFunc["CompareFunc_Greater"] = wdGALCompareFunc::Greater;
      StateValuesCompareFunc["CompareFunc_NotEqual"] = wdGALCompareFunc::NotEqual;
      StateValuesCompareFunc["CompareFunc_GreaterEqual"] = wdGALCompareFunc::GreaterEqual;
      StateValuesCompareFunc["CompareFunc_Always"] = wdGALCompareFunc::Always;
    }

    // wdGALStencilOp
    {
      StateValuesStencilOp["StencilOp_Keep"] = wdGALStencilOp::Keep;
      StateValuesStencilOp["StencilOp_Zero"] = wdGALStencilOp::Zero;
      StateValuesStencilOp["StencilOp_Replace"] = wdGALStencilOp::Replace;
      StateValuesStencilOp["StencilOp_IncrementSaturated"] = wdGALStencilOp::IncrementSaturated;
      StateValuesStencilOp["StencilOp_DecrementSaturated"] = wdGALStencilOp::DecrementSaturated;
      StateValuesStencilOp["StencilOp_Invert"] = wdGALStencilOp::Invert;
      StateValuesStencilOp["StencilOp_Increment"] = wdGALStencilOp::Increment;
      StateValuesStencilOp["StencilOp_Decrement"] = wdGALStencilOp::Decrement;
    }
  }

  // Retrieve Blend State
  {
    m_BlendDesc.m_bAlphaToCoverage = GetBoolStateVariable(VariableValues, "AlphaToCoverage", m_BlendDesc.m_bAlphaToCoverage);
    m_BlendDesc.m_bIndependentBlend = GetBoolStateVariable(VariableValues, "IndependentBlend", m_BlendDesc.m_bIndependentBlend);

    wdStringBuilder s;

    for (wdUInt32 i = 0; i < 8; ++i)
    {
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_bBlendingEnabled = GetBoolStateVariable(
        VariableValues, InsertNumber("BlendingEnabled{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_bBlendingEnabled);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_BlendOp = (wdGALBlendOp::Enum)GetEnumStateVariable(
        VariableValues, StateValuesBlendOp, InsertNumber("BlendOp{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_BlendOp);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_BlendOpAlpha = (wdGALBlendOp::Enum)GetEnumStateVariable(
        VariableValues, StateValuesBlendOp, InsertNumber("BlendOpAlpha{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_BlendOpAlpha);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_DestBlend = (wdGALBlend::Enum)GetEnumStateVariable(
        VariableValues, StateValuesBlend, InsertNumber("DestBlend{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_DestBlend);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_DestBlendAlpha = (wdGALBlend::Enum)GetEnumStateVariable(
        VariableValues, StateValuesBlend, InsertNumber("DestBlendAlpha{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_DestBlendAlpha);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_SourceBlend = (wdGALBlend::Enum)GetEnumStateVariable(
        VariableValues, StateValuesBlend, InsertNumber("SourceBlend{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_SourceBlend);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_SourceBlendAlpha = (wdGALBlend::Enum)GetEnumStateVariable(VariableValues, StateValuesBlend,
        InsertNumber("SourceBlendAlpha{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_SourceBlendAlpha);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_uiWriteMask = static_cast<wdUInt8>(GetIntStateVariable(VariableValues, InsertNumber("WriteMask{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_uiWriteMask));
    }
  }

  // Retrieve Rasterizer State
  {
    m_RasterizerDesc.m_bFrontCounterClockwise =
      GetBoolStateVariable(VariableValues, "FrontCounterClockwise", m_RasterizerDesc.m_bFrontCounterClockwise);
    m_RasterizerDesc.m_bScissorTest = GetBoolStateVariable(VariableValues, "ScissorTest", m_RasterizerDesc.m_bScissorTest);
    m_RasterizerDesc.m_bConservativeRasterization =
      GetBoolStateVariable(VariableValues, "ConservativeRasterization", m_RasterizerDesc.m_bConservativeRasterization);
    m_RasterizerDesc.m_bWireFrame = GetBoolStateVariable(VariableValues, "WireFrame", m_RasterizerDesc.m_bWireFrame);
    m_RasterizerDesc.m_CullMode =
      (wdGALCullMode::Enum)GetEnumStateVariable(VariableValues, StateValuesCullMode, "CullMode", m_RasterizerDesc.m_CullMode);
    m_RasterizerDesc.m_fDepthBiasClamp = GetFloatStateVariable(VariableValues, "DepthBiasClamp", m_RasterizerDesc.m_fDepthBiasClamp);
    m_RasterizerDesc.m_fSlopeScaledDepthBias =
      GetFloatStateVariable(VariableValues, "SlopeScaledDepthBias", m_RasterizerDesc.m_fSlopeScaledDepthBias);
    m_RasterizerDesc.m_iDepthBias = GetIntStateVariable(VariableValues, "DepthBias", m_RasterizerDesc.m_iDepthBias);
  }

  // Retrieve Depth-Stencil State
  {
    m_DepthStencilDesc.m_BackFaceStencilOp.m_DepthFailOp = (wdGALStencilOp::Enum)GetEnumStateVariable(
      VariableValues, StateValuesStencilOp, "BackFaceDepthFailOp", m_DepthStencilDesc.m_BackFaceStencilOp.m_DepthFailOp);
    m_DepthStencilDesc.m_BackFaceStencilOp.m_FailOp = (wdGALStencilOp::Enum)GetEnumStateVariable(
      VariableValues, StateValuesStencilOp, "BackFaceFailOp", m_DepthStencilDesc.m_BackFaceStencilOp.m_FailOp);
    m_DepthStencilDesc.m_BackFaceStencilOp.m_PassOp = (wdGALStencilOp::Enum)GetEnumStateVariable(
      VariableValues, StateValuesStencilOp, "BackFacePassOp", m_DepthStencilDesc.m_BackFaceStencilOp.m_PassOp);
    m_DepthStencilDesc.m_BackFaceStencilOp.m_StencilFunc = (wdGALCompareFunc::Enum)GetEnumStateVariable(
      VariableValues, StateValuesCompareFunc, "BackFaceStencilFunc", m_DepthStencilDesc.m_BackFaceStencilOp.m_StencilFunc);

    m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp = (wdGALStencilOp::Enum)GetEnumStateVariable(
      VariableValues, StateValuesStencilOp, "FrontFaceDepthFailOp", m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp);
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp = (wdGALStencilOp::Enum)GetEnumStateVariable(
      VariableValues, StateValuesStencilOp, "FrontFaceFailOp", m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp);
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp = (wdGALStencilOp::Enum)GetEnumStateVariable(
      VariableValues, StateValuesStencilOp, "FrontFacePassOp", m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp);
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc = (wdGALCompareFunc::Enum)GetEnumStateVariable(
      VariableValues, StateValuesCompareFunc, "FrontFaceStencilFunc", m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc);

    m_DepthStencilDesc.m_bDepthTest = GetBoolStateVariable(VariableValues, "DepthTest", m_DepthStencilDesc.m_bDepthTest);
    m_DepthStencilDesc.m_bDepthWrite = GetBoolStateVariable(VariableValues, "DepthWrite", m_DepthStencilDesc.m_bDepthWrite);
    m_DepthStencilDesc.m_bSeparateFrontAndBack =
      GetBoolStateVariable(VariableValues, "SeparateFrontAndBack", m_DepthStencilDesc.m_bSeparateFrontAndBack);
    m_DepthStencilDesc.m_bStencilTest = GetBoolStateVariable(VariableValues, "StencilTest", m_DepthStencilDesc.m_bStencilTest);
    m_DepthStencilDesc.m_DepthTestFunc =
      (wdGALCompareFunc::Enum)GetEnumStateVariable(VariableValues, StateValuesCompareFunc, "DepthTestFunc", m_DepthStencilDesc.m_DepthTestFunc);
    m_DepthStencilDesc.m_uiStencilReadMask = static_cast<wdUInt8>(GetIntStateVariable(VariableValues, "StencilReadMask", m_DepthStencilDesc.m_uiStencilReadMask));
    m_DepthStencilDesc.m_uiStencilWriteMask = static_cast<wdUInt8>(GetIntStateVariable(VariableValues, "StencilWriteMask", m_DepthStencilDesc.m_uiStencilWriteMask));
  }

#if WD_ENABLED(WD_COMPILE_FOR_DEBUG)
  // check for invalid variable names
  {
    for (auto it = VariableValues.GetIterator(); it.IsValid(); ++it)
    {
      if (!s_AllAllowedVariables.Contains(it.Key()))
      {
        wdLog::Error("The shader state variable '{0}' does not exist.", it.Key());
      }
    }
  }
#endif


  return WD_SUCCESS;
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Shader_Implementation_ShaderStateDescriptor);
