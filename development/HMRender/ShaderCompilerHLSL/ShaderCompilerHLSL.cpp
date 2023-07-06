#include <ShaderCompilerHLSL/ShaderCompilerHLSL.h>
#include <d3dcompiler.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdShaderCompilerHLSL, 1, wdRTTIDefaultAllocator<wdShaderCompilerHLSL>)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdResult CompileDXShader(const char* szFile, const char* szSource, bool bDebug, const char* szProfile, const char* szEntryPoint, wdDynamicArray<wdUInt8>& out_byteCode)
{
  out_byteCode.Clear();

  ID3DBlob* pResultBlob = nullptr;
  ID3DBlob* pErrorBlob = nullptr;

  const char* szCompileSource = szSource;
  wdStringBuilder sDebugSource;
  UINT flags1 = 0;
  if (bDebug)
  {
    flags1 = D3DCOMPILE_DEBUG | D3DCOMPILE_PREFER_FLOW_CONTROL | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_ENABLE_STRICTNESS;
    // In debug mode we need to remove '#line' as any shader debugger won't work with them.
    sDebugSource = szSource;
    sDebugSource.ReplaceAll("#line ", "//ine ");
    szCompileSource = sDebugSource;
  }

  if (FAILED(D3DCompile(szCompileSource, strlen(szCompileSource), szFile, nullptr, nullptr, szEntryPoint, szProfile, flags1, 0, &pResultBlob, &pErrorBlob)))
  {
    if (bDebug)
    {
      // Try again with '#line' intact to get correct error messages with file and line info.
      pErrorBlob->Release();
      pErrorBlob = nullptr;
      WD_VERIFY(FAILED(D3DCompile(szSource, strlen(szSource), szFile, nullptr, nullptr, szEntryPoint, szProfile, flags1, 0, &pResultBlob, &pErrorBlob)), "Debug compilation with commented out '#line' failed but original version did not.");
    }

    const char* szError = static_cast<const char*>(pErrorBlob->GetBufferPointer());

    WD_LOG_BLOCK("Shader Compilation Failed", szFile);

    wdLog::Error("Could not compile shader '{0}' for profile '{1}'", szFile, szProfile);
    wdLog::Error("{0}", szError);

    pErrorBlob->Release();
    return WD_FAILURE;
  }

  if (pErrorBlob != nullptr)
  {
    const char* szError = static_cast<const char*>(pErrorBlob->GetBufferPointer());

    WD_LOG_BLOCK("Shader Compilation Error Message", szFile);
    wdLog::Dev("{0}", szError);

    pErrorBlob->Release();
  }

  if (pResultBlob != nullptr)
  {
    out_byteCode.SetCountUninitialized((wdUInt32)pResultBlob->GetBufferSize());
    wdMemoryUtils::Copy(out_byteCode.GetData(), static_cast<wdUInt8*>(pResultBlob->GetBufferPointer()), out_byteCode.GetCount());
    pResultBlob->Release();
  }

  return WD_SUCCESS;
}

void wdShaderCompilerHLSL::ReflectShaderStage(wdShaderProgramData& inout_Data, wdGALShaderStage::Enum Stage)
{
  ID3D11ShaderReflection* pReflector = nullptr;

  auto byteCode = inout_Data.m_StageBinary[Stage].GetByteCode();
  D3DReflect(byteCode.GetData(), byteCode.GetCount(), IID_ID3D11ShaderReflection, (void**)&pReflector);

  D3D11_SHADER_DESC shaderDesc;
  pReflector->GetDesc(&shaderDesc);

  for (wdUInt32 r = 0; r < shaderDesc.BoundResources; ++r)
  {
    D3D11_SHADER_INPUT_BIND_DESC shaderInputBindDesc;
    pReflector->GetResourceBindingDesc(r, &shaderInputBindDesc);

    // wdLog::Info("Bound Resource: '{0}' at slot {1} (Count: {2}, Flags: {3})", sibd.Name, sibd.BindPoint, sibd.BindCount, sibd.uFlags);

    wdShaderResourceBinding shaderResourceBinding;
    shaderResourceBinding.m_Type = wdShaderResourceType::Unknown;
    shaderResourceBinding.m_iSlot = shaderInputBindDesc.BindPoint;
    shaderResourceBinding.m_sName.Assign(shaderInputBindDesc.Name);

    if (shaderInputBindDesc.Type == D3D_SIT_TEXTURE)
    {
      switch (shaderInputBindDesc.Dimension)
      {
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1D:
          shaderResourceBinding.m_Type = wdShaderResourceType::Texture1D;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1DARRAY:
          shaderResourceBinding.m_Type = wdShaderResourceType::Texture1DArray;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2D:
          shaderResourceBinding.m_Type = wdShaderResourceType::Texture2D;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DARRAY:
          shaderResourceBinding.m_Type = wdShaderResourceType::Texture2DArray;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMS:
          shaderResourceBinding.m_Type = wdShaderResourceType::Texture2DMS;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
          shaderResourceBinding.m_Type = wdShaderResourceType::Texture2DMSArray;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE3D:
          shaderResourceBinding.m_Type = wdShaderResourceType::Texture3D;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBE:
          shaderResourceBinding.m_Type = wdShaderResourceType::TextureCube;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
          shaderResourceBinding.m_Type = wdShaderResourceType::TextureCubeArray;
          break;
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFER:
          shaderResourceBinding.m_Type = wdShaderResourceType::GenericBuffer;
          break;

        default:
          WD_ASSERT_NOT_IMPLEMENTED;
          break;
      }
    }

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWTYPED)
    {
      switch (shaderInputBindDesc.Dimension)
      {
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1D:
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE1DARRAY:
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2D:
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_TEXTURE2DARRAY:
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFER:
        case D3D_SRV_DIMENSION::D3D_SRV_DIMENSION_BUFFEREX:
          shaderResourceBinding.m_Type = wdShaderResourceType::UAV;
          break;

        default:
          WD_ASSERT_NOT_IMPLEMENTED;
          break;
      }
    }

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED)
      shaderResourceBinding.m_Type = wdShaderResourceType::UAV;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWBYTEADDRESS)
      shaderResourceBinding.m_Type = wdShaderResourceType::UAV;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_APPEND_STRUCTURED)
      shaderResourceBinding.m_Type = wdShaderResourceType::UAV;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_CONSUME_STRUCTURED)
      shaderResourceBinding.m_Type = wdShaderResourceType::UAV;

    else if (shaderInputBindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER)
      shaderResourceBinding.m_Type = wdShaderResourceType::UAV;

    else if (shaderInputBindDesc.Type == D3D_SIT_CBUFFER)
    {
      shaderResourceBinding.m_Type = wdShaderResourceType::ConstantBuffer;
      shaderResourceBinding.m_pLayout = ReflectConstantBufferLayout(inout_Data.m_StageBinary[Stage], pReflector->GetConstantBufferByName(shaderInputBindDesc.Name));
    }
    else if (shaderInputBindDesc.Type == D3D_SIT_SAMPLER)
    {
      shaderResourceBinding.m_Type = wdShaderResourceType::Sampler;
      if (wdStringUtils::EndsWith(shaderInputBindDesc.Name, "_AutoSampler"))
      {
        wdStringBuilder sb = shaderInputBindDesc.Name;
        sb.Shrink(0, wdStringUtils::GetStringElementCount("_AutoSampler"));
        shaderResourceBinding.m_sName.Assign(sb.GetData());
      }
    }
    else
    {
      shaderResourceBinding.m_Type = wdShaderResourceType::GenericBuffer;
    }

    if (shaderResourceBinding.m_Type != wdShaderResourceType::Unknown)
    {
      inout_Data.m_StageBinary[Stage].AddShaderResourceBinding(shaderResourceBinding);
    }
  }

  pReflector->Release();
}

wdShaderConstantBufferLayout* wdShaderCompilerHLSL::ReflectConstantBufferLayout(wdShaderStageBinary& pStageBinary, ID3D11ShaderReflectionConstantBuffer* pConstantBufferReflection)
{
  D3D11_SHADER_BUFFER_DESC shaderBufferDesc;

  if (FAILED(pConstantBufferReflection->GetDesc(&shaderBufferDesc)))
  {
    return nullptr;
  }

  WD_LOG_BLOCK("Constant Buffer Layout", shaderBufferDesc.Name);
  wdLog::Debug("Constant Buffer has {0} variables, Size is {1}", shaderBufferDesc.Variables, shaderBufferDesc.Size);

  wdShaderConstantBufferLayout* pLayout = pStageBinary.CreateConstantBufferLayout();

  pLayout->m_uiTotalSize = shaderBufferDesc.Size;

  for (wdUInt32 var = 0; var < shaderBufferDesc.Variables; ++var)
  {
    ID3D11ShaderReflectionVariable* pVar = pConstantBufferReflection->GetVariableByIndex(var);

    D3D11_SHADER_VARIABLE_DESC svd;
    pVar->GetDesc(&svd);

    WD_LOG_BLOCK("Constant", svd.Name);

    D3D11_SHADER_TYPE_DESC std;
    pVar->GetType()->GetDesc(&std);

    wdShaderConstantBufferLayout::Constant constant;
    constant.m_uiArrayElements = static_cast<wdUInt8>(wdMath::Max(std.Elements, 1u));
    constant.m_uiOffset = static_cast<wdUInt16>(svd.StartOffset);
    constant.m_sName.Assign(svd.Name);

    if (std.Class == D3D_SVC_SCALAR || std.Class == D3D_SVC_VECTOR)
    {
      switch (std.Type)
      {
        case D3D_SVT_FLOAT:
          constant.m_Type = (wdShaderConstantBufferLayout::Constant::Type::Enum)((wdInt32)wdShaderConstantBufferLayout::Constant::Type::Float1 + std.Columns - 1);
          break;
        case D3D_SVT_INT:
          constant.m_Type = (wdShaderConstantBufferLayout::Constant::Type::Enum)((wdInt32)wdShaderConstantBufferLayout::Constant::Type::Int1 + std.Columns - 1);
          break;
        case D3D_SVT_UINT:
          constant.m_Type = (wdShaderConstantBufferLayout::Constant::Type::Enum)((wdInt32)wdShaderConstantBufferLayout::Constant::Type::UInt1 + std.Columns - 1);
          break;
        case D3D_SVT_BOOL:
          if (std.Columns == 1)
          {
            constant.m_Type = wdShaderConstantBufferLayout::Constant::Type::Bool;
          }
          break;

        default:
          break;
      }
    }
    else if (std.Class == D3D_SVC_MATRIX_COLUMNS)
    {
      if (std.Type != D3D_SVT_FLOAT)
      {
        wdLog::Error("Variable '{0}': Only float matrices are supported", svd.Name);
        continue;
      }

      if (std.Columns == 3 && std.Rows == 3)
      {
        constant.m_Type = wdShaderConstantBufferLayout::Constant::Type::Mat3x3;
      }
      else if (std.Columns == 4 && std.Rows == 4)
      {
        constant.m_Type = wdShaderConstantBufferLayout::Constant::Type::Mat4x4;
      }
      else
      {
        wdLog::Error("Variable '{0}': {1}x{2} matrices are not supported", svd.Name, std.Rows, std.Columns);
        continue;
      }
    }
    else if (std.Class == D3D_SVC_MATRIX_ROWS)
    {
      wdLog::Error("Variable '{0}': Row-Major matrices are not supported", svd.Name);
      continue;
    }
    else if (std.Class == D3D_SVC_STRUCT)
    {
      continue;
    }

    if (constant.m_Type == wdShaderConstantBufferLayout::Constant::Type::Default)
    {
      wdLog::Error("Variable '{0}': Variable type '{1}' is unknown / not supported", svd.Name, std.Class);
      continue;
    }

    pLayout->m_Constants.PushBack(constant);
  }

  return pLayout;
}

const char* GetProfileName(const char* szPlatform, wdGALShaderStage::Enum stage)
{
  if (wdStringUtils::IsEqual(szPlatform, "DX11_SM40_93"))
  {
    switch (stage)
    {
      case wdGALShaderStage::VertexShader:
        return "vs_4_0_level_9_3";
      case wdGALShaderStage::PixelShader:
        return "ps_4_0_level_9_3";
      default:
        break;
    }
  }

  if (wdStringUtils::IsEqual(szPlatform, "DX11_SM40"))
  {
    switch (stage)
    {
      case wdGALShaderStage::VertexShader:
        return "vs_4_0";
      case wdGALShaderStage::GeometryShader:
        return "gs_4_0";
      case wdGALShaderStage::PixelShader:
        return "ps_4_0";
      case wdGALShaderStage::ComputeShader:
        return "cs_4_0";
      default:
        break;
    }
  }

  if (wdStringUtils::IsEqual(szPlatform, "DX11_SM41"))
  {
    switch (stage)
    {
      case wdGALShaderStage::GeometryShader:
        return "gs_4_0";
      case wdGALShaderStage::VertexShader:
        return "vs_4_1";
      case wdGALShaderStage::PixelShader:
        return "ps_4_1";
      case wdGALShaderStage::ComputeShader:
        return "cs_4_1";
      default:
        break;
    }
  }

  if (wdStringUtils::IsEqual(szPlatform, "DX11_SM50"))
  {
    switch (stage)
    {
      case wdGALShaderStage::VertexShader:
        return "vs_5_0";
      case wdGALShaderStage::HullShader:
        return "hs_5_0";
      case wdGALShaderStage::DomainShader:
        return "ds_5_0";
      case wdGALShaderStage::GeometryShader:
        return "gs_5_0";
      case wdGALShaderStage::PixelShader:
        return "ps_5_0";
      case wdGALShaderStage::ComputeShader:
        return "cs_5_0";
      default:
        break;
    }
  }

  WD_REPORT_FAILURE("Unknown Platform '{0}' or Stage {1}", szPlatform, stage);
  return "";
}

wdResult wdShaderCompilerHLSL::Compile(wdShaderProgramData& inout_data, wdLogInterface* pLog)
{
  for (wdUInt32 stage = 0; stage < wdGALShaderStage::ENUM_COUNT; ++stage)
  {
    // shader already compiled
    if (!inout_data.m_StageBinary[stage].GetByteCode().IsEmpty())
    {
      wdLog::Debug("Shader for stage '{0}' is already compiled.", wdGALShaderStage::Names[stage]);
      continue;
    }

    const char* szShaderSource = inout_data.m_szShaderSource[stage];
    const wdUInt32 uiLength = wdStringUtils::GetStringElementCount(szShaderSource);

    if (uiLength > 0 && wdStringUtils::FindSubString(szShaderSource, "main") != nullptr)
    {
      if (CompileDXShader(inout_data.m_szSourceFile, szShaderSource, inout_data.m_Flags.IsSet(wdShaderCompilerFlags::Debug), GetProfileName(inout_data.m_szPlatform, (wdGALShaderStage::Enum)stage), "main", inout_data.m_StageBinary[stage].GetByteCode()).Succeeded())
      {
        ReflectShaderStage(inout_data, (wdGALShaderStage::Enum)stage);
      }
      else
      {
        return WD_FAILURE;
      }
    }
  }

  return WD_SUCCESS;
}
