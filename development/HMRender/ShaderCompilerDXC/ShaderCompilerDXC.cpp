#include <ShaderCompilerDXC/ShaderCompilerDXC.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Strings/StringConversion.h>

#include <ShaderCompilerDXC/SpirvMetaData.h>
#include <spirv_reflect.h>

#if WD_ENABLED(WD_PLATFORM_WINDOWS)
#  include <d3dcompiler.h>
#endif

#include <dxc/dxcapi.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdShaderCompilerDXC, 1, wdRTTIDefaultAllocator<wdShaderCompilerDXC>)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

template <typename T>
struct wdComPtr
{
public:
  wdComPtr() {}
  ~wdComPtr()
  {
    if (m_ptr != nullptr)
    {
      m_ptr->Release();
      m_ptr = nullptr;
    }
  }

  wdComPtr(const wdComPtr& other)
    : m_ptr(other.m_ptr)
  {
    if (m_ptr)
    {
      m_ptr->AddRef();
    }
  }

  T* operator->() { return m_ptr; }
  T* const operator->() const { return m_ptr; }

  T** put()
  {
    WD_ASSERT_DEV(m_ptr == nullptr, "Can only put into an empty wdComPtr");
    return &m_ptr;
  }

  bool operator==(nullptr_t)
  {
    return m_ptr == nullptr;
  }

  bool operator!=(nullptr_t)
  {
    return m_ptr != nullptr;
  }

private:
  T* m_ptr = nullptr;
};

wdComPtr<IDxcUtils> s_pDxcUtils;
wdComPtr<IDxcCompiler3> s_pDxcCompiler;

static wdResult CompileVulkanShader(const char* szFile, const char* szSource, bool bDebug, const char* szProfile, const char* szEntryPoint, wdDynamicArray<wdUInt8>& out_ByteCode);

static const char* GetProfileName(const char* szPlatform, wdGALShaderStage::Enum Stage)
{
  if (wdStringUtils::IsEqual(szPlatform, "VULKAN"))
  {
    switch (Stage)
    {
      case wdGALShaderStage::VertexShader:
        return "vs_6_0";
      case wdGALShaderStage::HullShader:
        return "hs_6_0";
      case wdGALShaderStage::DomainShader:
        return "ds_6_0";
      case wdGALShaderStage::GeometryShader:
        return "gs_6_0";
      case wdGALShaderStage::PixelShader:
        return "ps_6_0";
      case wdGALShaderStage::ComputeShader:
        return "cs_6_0";
      default:
        break;
    }
  }

  WD_REPORT_FAILURE("Unknown Platform '{}' or Stage {}", szPlatform, Stage);
  return "";
}

wdResult wdShaderCompilerDXC::Initialize()
{
  if (m_VertexInputMapping.IsEmpty())
  {
    m_VertexInputMapping["in.var.POSITION"] = wdGALVertexAttributeSemantic::Position;
    m_VertexInputMapping["in.var.NORMAL"] = wdGALVertexAttributeSemantic::Normal;
    m_VertexInputMapping["in.var.TANGENT"] = wdGALVertexAttributeSemantic::Tangent;

    m_VertexInputMapping["in.var.COLOR0"] = wdGALVertexAttributeSemantic::Color0;
    m_VertexInputMapping["in.var.COLOR1"] = wdGALVertexAttributeSemantic::Color1;
    m_VertexInputMapping["in.var.COLOR2"] = wdGALVertexAttributeSemantic::Color2;
    m_VertexInputMapping["in.var.COLOR3"] = wdGALVertexAttributeSemantic::Color3;
    m_VertexInputMapping["in.var.COLOR4"] = wdGALVertexAttributeSemantic::Color4;
    m_VertexInputMapping["in.var.COLOR5"] = wdGALVertexAttributeSemantic::Color5;
    m_VertexInputMapping["in.var.COLOR6"] = wdGALVertexAttributeSemantic::Color6;
    m_VertexInputMapping["in.var.COLOR7"] = wdGALVertexAttributeSemantic::Color7;

    m_VertexInputMapping["in.var.TEXCOORD0"] = wdGALVertexAttributeSemantic::TexCoord0;
    m_VertexInputMapping["in.var.TEXCOORD1"] = wdGALVertexAttributeSemantic::TexCoord1;
    m_VertexInputMapping["in.var.TEXCOORD2"] = wdGALVertexAttributeSemantic::TexCoord2;
    m_VertexInputMapping["in.var.TEXCOORD3"] = wdGALVertexAttributeSemantic::TexCoord3;
    m_VertexInputMapping["in.var.TEXCOORD4"] = wdGALVertexAttributeSemantic::TexCoord4;
    m_VertexInputMapping["in.var.TEXCOORD5"] = wdGALVertexAttributeSemantic::TexCoord5;
    m_VertexInputMapping["in.var.TEXCOORD6"] = wdGALVertexAttributeSemantic::TexCoord6;
    m_VertexInputMapping["in.var.TEXCOORD7"] = wdGALVertexAttributeSemantic::TexCoord7;
    m_VertexInputMapping["in.var.TEXCOORD8"] = wdGALVertexAttributeSemantic::TexCoord8;
    m_VertexInputMapping["in.var.TEXCOORD9"] = wdGALVertexAttributeSemantic::TexCoord9;

    m_VertexInputMapping["in.var.BITANGENT"] = wdGALVertexAttributeSemantic::BiTangent;
    m_VertexInputMapping["in.var.BONEINDICES0"] = wdGALVertexAttributeSemantic::BoneIndices0;
    m_VertexInputMapping["in.var.BONEINDICES1"] = wdGALVertexAttributeSemantic::BoneIndices1;
    m_VertexInputMapping["in.var.BONEWEIGHTS0"] = wdGALVertexAttributeSemantic::BoneWeights0;
    m_VertexInputMapping["in.var.BONEWEIGHTS1"] = wdGALVertexAttributeSemantic::BoneWeights1;
  }

  if (s_pDxcUtils != nullptr)
    return WD_SUCCESS;

  DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(s_pDxcUtils.put()));
  DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(s_pDxcCompiler.put()));

  return WD_SUCCESS;
}

wdResult wdShaderCompilerDXC::Compile(wdShaderProgramData& inout_Data, wdLogInterface* pLog)
{
  WD_SUCCEED_OR_RETURN(Initialize());

  for (wdUInt32 stage = 0; stage < wdGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (!inout_Data.m_StageBinary[stage].GetByteCode().IsEmpty())
    {
      wdLog::Debug("Shader for stage '{}' is already compiled.", wdGALShaderStage::Names[stage]);
      continue;
    }

    const char* szShaderSource = inout_Data.m_szShaderSource[stage];
    const wdUInt32 uiLength = wdStringUtils::GetStringElementCount(szShaderSource);

    if (uiLength > 0 && wdStringUtils::FindSubString(szShaderSource, "main") != nullptr)
    {
      if (CompileVulkanShader(inout_Data.m_szSourceFile, szShaderSource, inout_Data.m_Flags.IsSet(wdShaderCompilerFlags::Debug), GetProfileName(inout_Data.m_szPlatform, (wdGALShaderStage::Enum)stage), "main", inout_Data.m_StageBinary[stage].GetByteCode()).Succeeded())
      {
        WD_SUCCEED_OR_RETURN(ReflectShaderStage(inout_Data, (wdGALShaderStage::Enum)stage));
      }
      else
      {
        return WD_FAILURE;
      }
    }
  }

  return WD_SUCCESS;
}

wdResult CompileVulkanShader(const char* szFile, const char* szSource, bool bDebug, const char* szProfile, const char* szEntryPoint, wdDynamicArray<wdUInt8>& out_ByteCode)
{
  out_ByteCode.Clear();

  const char* szCompileSource = szSource;
  wdStringBuilder sDebugSource;

  wdDynamicArray<wdStringWChar> args;
  args.PushBack(wdStringWChar(szFile));
  args.PushBack(L"-E");
  args.PushBack(wdStringWChar(szEntryPoint));
  args.PushBack(L"-T");
  args.PushBack(wdStringWChar(szProfile));
  args.PushBack(L"-spirv");
  args.PushBack(L"-fvk-use-dx-position-w");
  args.PushBack(L"-fspv-target-env=vulkan1.1");

  if (bDebug)
  {
    // In debug mode we need to remove '#line' as any shader debugger won't work with them.
    sDebugSource = szSource;
    sDebugSource.ReplaceAll("#line ", "//ine ");
    szCompileSource = sDebugSource;

    //wdLog::Warning("Vulkan DEBUG shader support not really implemented.");

    args.PushBack(L"-Zi"); // Enable debug information.
    // args.PushBack(L"-Fo"); // Optional. Stored in the pdb.
    // args.PushBack(L"myshader.bin");
    // args.PushBack(L"-Fd"); // The file name of the pdb.
    // args.PushBack(L"myshader.pdb");
  }

  wdComPtr<IDxcBlobEncoding> pSource;
  s_pDxcUtils->CreateBlob(szCompileSource, (UINT32)strlen(szCompileSource), DXC_CP_UTF8, pSource.put());

  DxcBuffer Source;
  Source.Ptr = pSource->GetBufferPointer();
  Source.Size = pSource->GetBufferSize();
  Source.Encoding = DXC_CP_UTF8;

  wdHybridArray<LPCWSTR, 16> pszArgs;
  pszArgs.SetCount(args.GetCount());
  for (wdUInt32 i = 0; i < args.GetCount(); ++i)
  {
    pszArgs[i] = args[i].GetData();
  }

  wdComPtr<IDxcResult> pResults;
  s_pDxcCompiler->Compile(&Source, pszArgs.GetData(), pszArgs.GetCount(), nullptr, IID_PPV_ARGS(pResults.put()));

  wdComPtr<IDxcBlobUtf8> pErrors;
  pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pErrors.put()), nullptr);

  HRESULT hrStatus;
  pResults->GetStatus(&hrStatus);
  if (FAILED(hrStatus))
  {
    wdLog::Error("Vulkan shader compilation failed.");

    if (pErrors != nullptr && pErrors->GetStringLength() != 0)
    {
      wdLog::Error("{}", wdStringUtf8(pErrors->GetStringPointer()).GetData());
    }

    return WD_FAILURE;
  }
  else
  {
    if (pErrors != nullptr && pErrors->GetStringLength() != 0)
    {
      wdLog::Warning("{}", wdStringUtf8(pErrors->GetStringPointer()).GetData());
    }
  }

  wdComPtr<IDxcBlob> pShader;
  wdComPtr<IDxcBlobWide> pShaderName;
  pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(pShader.put()), pShaderName.put());

  if (pShader == nullptr)
  {
    wdLog::Error("No Vulkan bytecode was generated.");
    return WD_FAILURE;
  }

  out_ByteCode.SetCountUninitialized(static_cast<wdUInt32>(pShader->GetBufferSize()));

  wdMemoryUtils::Copy(out_ByteCode.GetData(), reinterpret_cast<wdUInt8*>(pShader->GetBufferPointer()), out_ByteCode.GetCount());

  return WD_SUCCESS;
}

wdResult wdShaderCompilerDXC::FillResourceBinding(wdShaderStageBinary& shaderBinary, wdShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info)
{
  if (info.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SRV)
  {
    return FillSRVResourceBinding(shaderBinary, binding, info);
  }

  if (info.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_UAV)
  {
    return FillUAVResourceBinding(shaderBinary, binding, info);
  }

  if (info.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_CBV)
  {
    binding.m_Type = wdShaderResourceType::ConstantBuffer;
    binding.m_pLayout = ReflectConstantBufferLayout(shaderBinary, info);

    return WD_SUCCESS;
  }

  if (info.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SAMPLER)
  {
    binding.m_Type = wdShaderResourceType::Sampler;

    // TODO: not sure how this will map to Vulkan
    if (binding.m_sName.GetString().EndsWith("_AutoSampler"))
    {
      wdStringBuilder sb = binding.m_sName.GetString();
      sb.TrimWordEnd("_AutoSampler");
      binding.m_sName.Assign(sb);
    }

    return WD_SUCCESS;
  }

  wdLog::Error("Resource '{}': Unsupported resource type.", info.name);
  return WD_FAILURE;
}

wdResult wdShaderCompilerDXC::FillSRVResourceBinding(wdShaderStageBinary& shaderBinary, wdShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info)
{
  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER)
  {
    if (info.type_description->op == SpvOp::SpvOpTypeStruct)
    {
      binding.m_Type = wdShaderResourceType::GenericBuffer;
      return WD_SUCCESS;
    }
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
  {
    switch (info.image.dim)
    {
      case SpvDim::SpvDim1D:
      {
        if (info.image.ms == 0)
        {
          if (info.image.arrayed > 0)
          {
            binding.m_Type = wdShaderResourceType::Texture1DArray;
            return WD_SUCCESS;
          }
          else
          {
            binding.m_Type = wdShaderResourceType::Texture1D;
            return WD_SUCCESS;
          }
        }

        break;
      }

      case SpvDim::SpvDim2D:
      {
        if (info.image.ms == 0)
        {
          if (info.image.arrayed > 0)
          {
            binding.m_Type = wdShaderResourceType::Texture2DArray;
            return WD_SUCCESS;
          }
          else
          {
            binding.m_Type = wdShaderResourceType::Texture2D;
            return WD_SUCCESS;
          }
        }
        else
        {
          if (info.image.arrayed > 0)
          {
            binding.m_Type = wdShaderResourceType::Texture2DMSArray;
            return WD_SUCCESS;
          }
          else
          {
            binding.m_Type = wdShaderResourceType::Texture2DMS;
            return WD_SUCCESS;
          }
        }

        break;
      }

      case SpvDim::SpvDim3D:
      {
        if (info.image.ms == 0 && info.image.arrayed == 0)
        {
          binding.m_Type = wdShaderResourceType::Texture3D;
          return WD_SUCCESS;
        }

        break;
      }

      case SpvDim::SpvDimCube:
      {
        if (info.image.ms == 0)
        {
          if (info.image.arrayed == 0)
          {
            binding.m_Type = wdShaderResourceType::TextureCube;
            return WD_SUCCESS;
          }
          else
          {
            binding.m_Type = wdShaderResourceType::TextureCubeArray;
            return WD_SUCCESS;
          }
        }

        break;
      }

      case SpvDim::SpvDimBuffer:
        binding.m_Type = wdShaderResourceType::GenericBuffer;
        return WD_SUCCESS;

      case SpvDim::SpvDimRect:
        WD_ASSERT_NOT_IMPLEMENTED;
        return WD_FAILURE;

      case SpvDim::SpvDimSubpassData:
        WD_ASSERT_NOT_IMPLEMENTED;
        return WD_FAILURE;

      case SpvDim::SpvDimMax:
        WD_ASSERT_DEV(false, "Invalid enum value");
        break;
    }

    if (info.image.ms > 0)
    {
      wdLog::Error("Resource '{}': Multi-sampled textures of this type are not supported.", info.name);
      return WD_FAILURE;
    }

    if (info.image.arrayed > 0)
    {
      wdLog::Error("Resource '{}': Array-textures of this type are not supported.", info.name);
      return WD_FAILURE;
    }
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER)
  {
    if (info.image.dim == SpvDim::SpvDimBuffer)
    {
      binding.m_Type = wdShaderResourceType::GenericBuffer;
      return WD_SUCCESS;
    }

    wdLog::Error("Resource '{}': Unsupported texel buffer SRV type.", info.name);
    return WD_FAILURE;
  }

  wdLog::Error("Resource '{}': Unsupported SRV type.", info.name);
  return WD_FAILURE;
}

wdResult wdShaderCompilerDXC::FillUAVResourceBinding(wdShaderStageBinary& shaderBinary, wdShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info)
{
  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE)
  {
    binding.m_Type = wdShaderResourceType::UAV;
    return WD_SUCCESS;
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
  {
    if (info.image.dim == SpvDim::SpvDimBuffer)
    {
      binding.m_Type = wdShaderResourceType::UAV;
      return WD_SUCCESS;
    }

    wdLog::Error("Resource '{}': Unsupported texel buffer UAV type.", info.name);
    return WD_FAILURE;
  }

  wdLog::Error("Resource '{}': Unsupported UAV type.", info.name);
  return WD_FAILURE;
}

wdGALResourceFormat::Enum GetWDFormat(SpvReflectFormat format)
{
  switch (format)
  {
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32_UINT:
      return wdGALResourceFormat::RUInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32_SINT:
      return wdGALResourceFormat::RInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32_SFLOAT:
      return wdGALResourceFormat::RFloat;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32_UINT:
      return wdGALResourceFormat::RGUInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32_SINT:
      return wdGALResourceFormat::RGInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32_SFLOAT:
      return wdGALResourceFormat::RGFloat;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32_UINT:
      return wdGALResourceFormat::RGBUInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32_SINT:
      return wdGALResourceFormat::RGBInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
      return wdGALResourceFormat::RGBFloat;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32A32_UINT:
      return wdGALResourceFormat::RGBAUInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32A32_SINT:
      return wdGALResourceFormat::RGBAInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
      return wdGALResourceFormat::RGBAFloat;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_UNDEFINED:
    default:
      return wdGALResourceFormat::Invalid;
  }
}

wdResult wdShaderCompilerDXC::ReflectShaderStage(wdShaderProgramData& inout_Data, wdGALShaderStage::Enum Stage)
{
  WD_LOG_BLOCK("ReflectShaderStage", inout_Data.m_szSourceFile);

  auto& bytecode = inout_Data.m_StageBinary[Stage].GetByteCode();

  SpvReflectShaderModule module;

  if (spvReflectCreateShaderModule(bytecode.GetCount(), bytecode.GetData(), &module) != SPV_REFLECT_RESULT_SUCCESS)
  {
    wdLog::Error("Extracting shader reflection information failed.");
    return WD_FAILURE;
  }

  WD_SCOPE_EXIT(spvReflectDestroyShaderModule(&module));

  //
  wdHybridArray<wdVulkanVertexInputAttribute, 8> vertexInputAttributes;
  if (Stage == wdGALShaderStage::VertexShader)
  {
    wdUInt32 uiNumVars = 0;
    if (spvReflectEnumerateInputVariables(&module, &uiNumVars, nullptr) != SPV_REFLECT_RESULT_SUCCESS)
    {
      wdLog::Error("Failed to retrieve number of input variables.");
      return WD_FAILURE;
    }
    wdDynamicArray<SpvReflectInterfaceVariable*> vars;
    vars.SetCount(uiNumVars);

    if (spvReflectEnumerateInputVariables(&module, &uiNumVars, vars.GetData()) != SPV_REFLECT_RESULT_SUCCESS)
    {
      wdLog::Error("Failed to retrieve input variables.");
      return WD_FAILURE;
    }

    vertexInputAttributes.Reserve(vars.GetCount());

    for (wdUInt32 i = 0; i < vars.GetCount(); ++i)
    {
      SpvReflectInterfaceVariable* pVar = vars[i];
      if (pVar->name != nullptr)
      {
        wdVulkanVertexInputAttribute& attr = vertexInputAttributes.ExpandAndGetRef();
        attr.m_uiLocation = static_cast<wdUInt8>(pVar->location);

        wdGALVertexAttributeSemantic::Enum* pVAS = m_VertexInputMapping.GetValue(pVar->name);
        WD_ASSERT_DEV(pVAS != nullptr, "Unknown vertex input sematic found: {}", pVar->name);
        attr.m_eSemantic = *pVAS;
        attr.m_eFormat = GetWDFormat(pVar->format);
        WD_ASSERT_DEV(pVAS != nullptr, "Unknown vertex input format found: {}", pVar->format);
      }
    }
  }


  // descriptor bindings
  {
    wdUInt32 uiNumVars = 0;
    if (spvReflectEnumerateDescriptorBindings(&module, &uiNumVars, nullptr) != SPV_REFLECT_RESULT_SUCCESS)
    {
      wdLog::Error("Failed to retrieve number of descriptor bindings.");
      return WD_FAILURE;
    }

    wdDynamicArray<SpvReflectDescriptorBinding*> vars;
    vars.SetCount(uiNumVars);

    if (spvReflectEnumerateDescriptorBindings(&module, &uiNumVars, vars.GetData()) != SPV_REFLECT_RESULT_SUCCESS)
    {
      wdLog::Error("Failed to retrieve descriptor bindings.");
      return WD_FAILURE;
    }

    wdMap<wdUInt32, wdUInt32> descriptorToEzBinding;
    wdUInt32 uiVirtualResourceView = 0;
    wdUInt32 uiVirtualSampler = 0;
    for (wdUInt32 i = 0; i < vars.GetCount(); ++i)
    {
      auto& info = *vars[i];

      wdLog::Info("Bound Resource: '{}' at slot {} (Count: {})", info.name, info.binding, info.count);

      wdShaderResourceBinding shaderResourceBinding;
      shaderResourceBinding.m_Type = wdShaderResourceType::Unknown;
      shaderResourceBinding.m_iSlot = info.binding;
      shaderResourceBinding.m_sName.Assign(info.name);

      if (FillResourceBinding(inout_Data.m_StageBinary[Stage], shaderResourceBinding, info).Failed())
        continue;

      // We pretend SRVs and Samplers are mapped per stage and nicely packed so we fit into the DX11-based high level render interface.
      if (info.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SRV)
      {
        shaderResourceBinding.m_iSlot = uiVirtualResourceView;
        uiVirtualResourceView++;
      }
      if (info.resource_type == SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SAMPLER)
      {
        shaderResourceBinding.m_iSlot = uiVirtualSampler;
        uiVirtualSampler++;
      }


      WD_ASSERT_DEV(shaderResourceBinding.m_Type != wdShaderResourceType::Unknown, "FillResourceBinding should have failed.");

      descriptorToEzBinding[i] = inout_Data.m_StageBinary[Stage].GetShaderResourceBindings().GetCount();
      inout_Data.m_StageBinary[Stage].AddShaderResourceBinding(shaderResourceBinding);
    }

    {
      wdArrayPtr<const wdShaderResourceBinding> wdBindings = inout_Data.m_StageBinary[Stage].GetShaderResourceBindings();
      // Modify meta data
      wdDefaultMemoryStreamStorage storage;
      wdMemoryStreamWriter stream(&storage);

      const wdUInt32 uiCount = vars.GetCount();

      //#TODO_VULKAN Currently hard coded to a single DescriptorSetLayout.
      wdHybridArray<wdVulkanDescriptorSetLayout, 3> sets;
      wdVulkanDescriptorSetLayout& set = sets.ExpandAndGetRef();

      for (wdUInt32 i = 0; i < uiCount; ++i)
      {
        auto& info = *vars[i];
        WD_ASSERT_DEV(info.set == 0, "Only a single descriptor set is currently supported.");
        wdVulkanDescriptorSetLayoutBinding& binding = set.bindings.ExpandAndGetRef();
        binding.m_sName = info.name;
        binding.m_uiBinding = static_cast<wdUInt8>(info.binding);
        binding.m_uiVirtualBinding = wdBindings[descriptorToEzBinding[i]].m_iSlot;
        binding.m_wdType = wdBindings[descriptorToEzBinding[i]].m_Type;
        switch (info.resource_type)
        {
          case SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SAMPLER:
            binding.m_Type = wdVulkanDescriptorSetLayoutBinding::ResourceType::Sampler;
            break;
          case SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_CBV:
            binding.m_Type = wdVulkanDescriptorSetLayoutBinding::ResourceType::ConstantBuffer;
            break;
          case SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SRV:
            binding.m_Type = wdVulkanDescriptorSetLayoutBinding::ResourceType::ResourceView;
            break;
          default:
          case SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_UAV:
            binding.m_Type = wdVulkanDescriptorSetLayoutBinding::ResourceType::UAV;
            break;
        }
        binding.m_uiDescriptorType = static_cast<wdUInt32>(info.descriptor_type);
        binding.m_uiDescriptorCount = 1;
        for (wdUInt32 uiDim = 0; uiDim < info.array.dims_count; ++uiDim)
        {
          binding.m_uiDescriptorCount *= info.array.dims[uiDim];
        }
        binding.m_uiWordOffset = info.word_offset.binding;
      }
      set.bindings.Sort([](const wdVulkanDescriptorSetLayoutBinding& lhs, const wdVulkanDescriptorSetLayoutBinding& rhs) { return lhs.m_uiBinding < rhs.m_uiBinding; });

      wdSpirvMetaData::Write(stream, bytecode, sets, vertexInputAttributes);

      // Replaced compiled Spirv code with custom wdSpirvMetaData format.
      wdUInt64 uiBytesLeft = storage.GetStorageSize64();
      wdUInt64 uiReadPosition = 0;
      bytecode.Clear();
      bytecode.Reserve((wdUInt32)uiBytesLeft);
      while (uiBytesLeft > 0)
      {
        wdArrayPtr<const wdUInt8> data = storage.GetContiguousMemoryRange(uiReadPosition);
        bytecode.PushBackRange(data);
        uiReadPosition += data.GetCount();
        uiBytesLeft -= data.GetCount();
      }
    }
  }
  return WD_SUCCESS;
}

wdShaderConstantBufferLayout* wdShaderCompilerDXC::ReflectConstantBufferLayout(wdShaderStageBinary& pStageBinary, const SpvReflectDescriptorBinding& constantBufferReflection)
{
  const auto& block = constantBufferReflection.block;

  WD_LOG_BLOCK("Constant Buffer Layout", constantBufferReflection.name);
  wdLog::Debug("Constant Buffer has {} variables, Size is {}", block.member_count, block.padded_size);

  wdShaderConstantBufferLayout* pLayout = pStageBinary.CreateConstantBufferLayout();

  pLayout->m_uiTotalSize = block.padded_size;

  for (wdUInt32 var = 0; var < block.member_count; ++var)
  {
    const auto& svd = block.members[var];

    wdShaderConstantBufferLayout::Constant constant;
    constant.m_sName.Assign(svd.name);
    constant.m_uiOffset = svd.offset; // TODO: or svd.absolute_offset ??
    constant.m_uiArrayElements = 1;

    wdUInt32 uiFlags = svd.type_description->type_flags;

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_ARRAY)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_ARRAY;

      if (svd.array.dims_count != 1)
      {
        wdLog::Error("Variable '{}': Multi-dimensional arrays are not supported.", constant.m_sName);
        continue;
      }

      constant.m_uiArrayElements = svd.array.dims[0];
    }

    wdUInt32 uiComponents = 0;

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_VECTOR)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_VECTOR;

      uiComponents = svd.numeric.vector.component_count;
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_BOOL)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_BOOL;

      // TODO: unfortunately this never seems to be set, 'bool' types are always exposed as 'int'
      WD_ASSERT_NOT_IMPLEMENTED;

      switch (uiComponents)
      {
        case 0:
        case 1:
          constant.m_Type = wdShaderConstantBufferLayout::Constant::Type::Bool;
          break;

        default:
          wdLog::Error("Variable '{}': Multi-component bools are not supported.", constant.m_sName);
          continue;
      }
    }
    else if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_INT)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_INT;

      // TODO: there doesn't seem to be a way to detect 'unsigned' types

      switch (uiComponents)
      {
        case 0:
        case 1:
          constant.m_Type = wdShaderConstantBufferLayout::Constant::Type::Int1;
          break;
        case 2:
          constant.m_Type = wdShaderConstantBufferLayout::Constant::Type::Int2;
          break;
        case 3:
          constant.m_Type = wdShaderConstantBufferLayout::Constant::Type::Int3;
          break;
        case 4:
          constant.m_Type = wdShaderConstantBufferLayout::Constant::Type::Int4;
          break;
      }
    }
    else if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_FLOAT)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_FLOAT;

      switch (uiComponents)
      {
        case 0:
        case 1:
          constant.m_Type = wdShaderConstantBufferLayout::Constant::Type::Float1;
          break;
        case 2:
          constant.m_Type = wdShaderConstantBufferLayout::Constant::Type::Float2;
          break;
        case 3:
          constant.m_Type = wdShaderConstantBufferLayout::Constant::Type::Float3;
          break;
        case 4:
          constant.m_Type = wdShaderConstantBufferLayout::Constant::Type::Float4;
          break;
      }
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_MATRIX)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_MATRIX;

      constant.m_Type = wdShaderConstantBufferLayout::Constant::Type::Default;

      const wdUInt32 rows = svd.type_description->traits.numeric.matrix.row_count;
      const wdUInt32 columns = svd.type_description->traits.numeric.matrix.column_count;

      if ((svd.type_description->type_flags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_FLOAT) == 0)
      {
        wdLog::Error("Variable '{}': Only float matrices are supported", constant.m_sName);
        continue;
      }

      if (columns == 3 && rows == 3)
      {
        constant.m_Type = wdShaderConstantBufferLayout::Constant::Type::Mat3x3;
      }
      else if (columns == 4 && rows == 4)
      {
        constant.m_Type = wdShaderConstantBufferLayout::Constant::Type::Mat4x4;
      }
      else
      {
        wdLog::Error("Variable '{}': {}x{} matrices are not supported", constant.m_sName, rows, columns);
        continue;
      }
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_STRUCT)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_STRUCT;
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_EXTERNAL_BLOCK;

      constant.m_Type = wdShaderConstantBufferLayout::Constant::Type::Struct;
    }

    if (uiFlags != 0)
    {
      wdLog::Error("Variable '{}': Unknown additional type flags '{}'", constant.m_sName, uiFlags);
    }

    if (constant.m_Type == wdShaderConstantBufferLayout::Constant::Type::Default)
    {
      wdLog::Error("Variable '{}': Variable type is unknown / not supported", constant.m_sName);
      continue;
    }

    const char* typeNames[] = {
      "Default",
      "Float1",
      "Float2",
      "Float3",
      "Float4",
      "Int1",
      "Int2",
      "Int3",
      "Int4",
      "UInt1",
      "UInt2",
      "UInt3",
      "UInt4",
      "Mat3x3",
      "Mat4x4",
      "Transform",
      "Bool",
      "Struct",
    };

    if (constant.m_uiArrayElements > 1)
    {
      wdLog::Info("{1} {3}[{2}] {0}", constant.m_sName, wdArgU(constant.m_uiOffset, 3, true), constant.m_uiArrayElements, typeNames[constant.m_Type]);
    }
    else
    {
      wdLog::Info("{1} {3} {0}", constant.m_sName, wdArgU(constant.m_uiOffset, 3, true), constant.m_uiArrayElements, typeNames[constant.m_Type]);
    }

    pLayout->m_Constants.PushBack(constant);
  }

  return pLayout;
}
