#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <RendererCore/Shader/ShaderStageBinary.h>
#include <RendererCore/Shader/Types.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>

wdUInt32 wdShaderConstantBufferLayout::Constant::s_TypeSize[(wdUInt32)Type::ENUM_COUNT] = {0, sizeof(float) * 1, sizeof(float) * 2, sizeof(float) * 3, sizeof(float) * 4, sizeof(int) * 1, sizeof(int) * 2, sizeof(int) * 3, sizeof(int) * 4, sizeof(wdUInt32) * 1, sizeof(wdUInt32) * 2,
  sizeof(wdUInt32) * 3, sizeof(wdUInt32) * 4, sizeof(wdShaderMat3), sizeof(wdMat4), sizeof(wdShaderTransform), sizeof(wdShaderBool)};

void wdShaderConstantBufferLayout::Constant::CopyDataFormVariant(wdUInt8* pDest, wdVariant* pValue) const
{
  WD_ASSERT_DEV(m_uiArrayElements == 1, "Array constants are not supported");

  wdResult conversionResult = WD_FAILURE;

  if (pValue != nullptr)
  {
    switch (m_Type)
    {
      case Type::Float1:
        *reinterpret_cast<float*>(pDest) = pValue->ConvertTo<float>(&conversionResult);
        break;
      case Type::Float2:
        *reinterpret_cast<wdVec2*>(pDest) = pValue->Get<wdVec2>();
        return;
      case Type::Float3:
        *reinterpret_cast<wdVec3*>(pDest) = pValue->Get<wdVec3>();
        return;
      case Type::Float4:
        if (pValue->GetType() == wdVariant::Type::Color || pValue->GetType() == wdVariant::Type::ColorGamma)
        {
          const wdColor tmp = pValue->ConvertTo<wdColor>();
          *reinterpret_cast<wdVec4*>(pDest) = *reinterpret_cast<const wdVec4*>(&tmp);
        }
        else
        {
          *reinterpret_cast<wdVec4*>(pDest) = pValue->Get<wdVec4>();
        }
        return;

      case Type::Int1:
        *reinterpret_cast<wdInt32*>(pDest) = pValue->ConvertTo<wdInt32>(&conversionResult);
        break;
      case Type::Int2:
        *reinterpret_cast<wdVec2I32*>(pDest) = pValue->Get<wdVec2I32>();
        return;
      case Type::Int3:
        *reinterpret_cast<wdVec3I32*>(pDest) = pValue->Get<wdVec3I32>();
        return;
      case Type::Int4:
        *reinterpret_cast<wdVec4I32*>(pDest) = pValue->Get<wdVec4I32>();
        return;

      case Type::UInt1:
        *reinterpret_cast<wdUInt32*>(pDest) = pValue->ConvertTo<wdUInt32>(&conversionResult);
        break;
      case Type::UInt2:
        *reinterpret_cast<wdVec2U32*>(pDest) = pValue->Get<wdVec2U32>();
        return;
      case Type::UInt3:
        *reinterpret_cast<wdVec3U32*>(pDest) = pValue->Get<wdVec3U32>();
        return;
      case Type::UInt4:
        *reinterpret_cast<wdVec4U32*>(pDest) = pValue->Get<wdVec4U32>();
        return;

      case Type::Mat3x3:
        *reinterpret_cast<wdShaderMat3*>(pDest) = pValue->Get<wdMat3>();
        return;
      case Type::Mat4x4:
        *reinterpret_cast<wdMat4*>(pDest) = pValue->Get<wdMat4>();
        return;
      case Type::Transform:
        *reinterpret_cast<wdShaderTransform*>(pDest) = pValue->Get<wdTransform>();
        return;

      case Type::Bool:
        *reinterpret_cast<wdShaderBool*>(pDest) = pValue->ConvertTo<bool>(&conversionResult);
        break;

      default:
        WD_ASSERT_NOT_IMPLEMENTED;
    }
  }

  if (conversionResult.Succeeded())
  {
    return;
  }

  // wdLog::Error("Constant '{0}' is not set, invalid or couldn't be converted to target type and will be set to zero.", m_sName);
  const wdUInt32 uiSize = s_TypeSize[m_Type];
  wdMemoryUtils::ZeroFill(pDest, uiSize);
}

wdShaderConstantBufferLayout::wdShaderConstantBufferLayout()
{
  m_uiTotalSize = 0;
}

wdShaderConstantBufferLayout::~wdShaderConstantBufferLayout() = default;

wdResult wdShaderConstantBufferLayout::Write(wdStreamWriter& inout_stream) const
{
  inout_stream << m_uiTotalSize;

  wdUInt16 uiConstants = static_cast<wdUInt16>(m_Constants.GetCount());
  inout_stream << uiConstants;

  for (auto& constant : m_Constants)
  {
    inout_stream << constant.m_sName;
    inout_stream << constant.m_Type;
    inout_stream << constant.m_uiArrayElements;
    inout_stream << constant.m_uiOffset;
  }

  return WD_SUCCESS;
}

wdResult wdShaderConstantBufferLayout::Read(wdStreamReader& inout_stream)
{
  inout_stream >> m_uiTotalSize;

  wdUInt16 uiConstants = 0;
  inout_stream >> uiConstants;

  m_Constants.SetCount(uiConstants);

  for (auto& constant : m_Constants)
  {
    inout_stream >> constant.m_sName;
    inout_stream >> constant.m_Type;
    inout_stream >> constant.m_uiArrayElements;
    inout_stream >> constant.m_uiOffset;
  }

  return WD_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

wdShaderResourceBinding::wdShaderResourceBinding()
{
  m_Type = wdShaderResourceType::Unknown;
  m_iSlot = -1;
  m_pLayout = nullptr;
}

wdShaderResourceBinding::~wdShaderResourceBinding() = default;

//////////////////////////////////////////////////////////////////////////

wdMap<wdUInt32, wdShaderStageBinary> wdShaderStageBinary::s_ShaderStageBinaries[wdGALShaderStage::ENUM_COUNT];

wdShaderStageBinary::wdShaderStageBinary() = default;

wdShaderStageBinary::~wdShaderStageBinary()
{
  if (m_GALByteCode)
  {
    wdGALShaderByteCode* pByteCode = m_GALByteCode;
    m_GALByteCode = nullptr;

    if (pByteCode->GetRefCount() == 0)
      WD_DEFAULT_DELETE(pByteCode);
  }

  for (auto& binding : m_ShaderResourceBindings)
  {
    if (binding.m_pLayout != nullptr)
    {
      wdShaderConstantBufferLayout* pLayout = binding.m_pLayout;
      binding.m_pLayout = nullptr;

      if (pLayout->GetRefCount() == 0)
        WD_DEFAULT_DELETE(pLayout);
    }
  }
}

wdResult wdShaderStageBinary::Write(wdStreamWriter& inout_stream) const
{
  const wdUInt8 uiVersion = wdShaderStageBinary::VersionCurrent;

  if (inout_stream.WriteBytes(&uiVersion, sizeof(wdUInt8)).Failed())
    return WD_FAILURE;

  if (inout_stream.WriteDWordValue(&m_uiSourceHash).Failed())
    return WD_FAILURE;

  const wdUInt8 uiStage = (wdUInt8)m_Stage;

  if (inout_stream.WriteBytes(&uiStage, sizeof(wdUInt8)).Failed())
    return WD_FAILURE;

  const wdUInt32 uiByteCodeSize = m_ByteCode.GetCount();

  if (inout_stream.WriteDWordValue(&uiByteCodeSize).Failed())
    return WD_FAILURE;

  if (!m_ByteCode.IsEmpty() && inout_stream.WriteBytes(&m_ByteCode[0], uiByteCodeSize).Failed())
    return WD_FAILURE;

  wdUInt16 uiResources = static_cast<wdUInt16>(m_ShaderResourceBindings.GetCount());
  inout_stream << uiResources;

  for (const auto& r : m_ShaderResourceBindings)
  {
    inout_stream << r.m_sName.GetData();
    inout_stream << r.m_iSlot;
    inout_stream << (wdUInt8)r.m_Type;

    if (r.m_Type == wdShaderResourceType::ConstantBuffer)
    {
      WD_SUCCEED_OR_RETURN(r.m_pLayout->Write(inout_stream));
    }
  }

  inout_stream << m_bWasCompiledWithDebug;

  return WD_SUCCESS;
}

wdResult wdShaderStageBinary::Read(wdStreamReader& inout_stream)
{
  wdUInt8 uiVersion = 0;

  if (inout_stream.ReadBytes(&uiVersion, sizeof(wdUInt8)) != sizeof(wdUInt8))
    return WD_FAILURE;

  WD_ASSERT_DEV(uiVersion <= wdShaderStageBinary::VersionCurrent, "Wrong Version {0}", uiVersion);

  if (inout_stream.ReadDWordValue(&m_uiSourceHash).Failed())
    return WD_FAILURE;

  wdUInt8 uiStage = wdGALShaderStage::ENUM_COUNT;

  if (inout_stream.ReadBytes(&uiStage, sizeof(wdUInt8)) != sizeof(wdUInt8))
    return WD_FAILURE;

  m_Stage = (wdGALShaderStage::Enum)uiStage;

  wdUInt32 uiByteCodeSize = 0;

  if (inout_stream.ReadDWordValue(&uiByteCodeSize).Failed())
    return WD_FAILURE;

  m_ByteCode.SetCountUninitialized(uiByteCodeSize);

  if (!m_ByteCode.IsEmpty() && inout_stream.ReadBytes(&m_ByteCode[0], uiByteCodeSize) != uiByteCodeSize)
    return WD_FAILURE;

  if (uiVersion >= wdShaderStageBinary::Version2)
  {
    wdUInt16 uiResources = 0;
    inout_stream >> uiResources;

    m_ShaderResourceBindings.SetCount(uiResources);

    wdString sTemp;

    for (auto& r : m_ShaderResourceBindings)
    {
      inout_stream >> sTemp;
      r.m_sName.Assign(sTemp.GetData());
      inout_stream >> r.m_iSlot;

      wdUInt8 uiType = 0;
      inout_stream >> uiType;
      r.m_Type = (wdShaderResourceType::Enum)uiType;

      if (r.m_Type == wdShaderResourceType::ConstantBuffer && uiVersion >= wdShaderStageBinary::Version4)
      {
        auto pLayout = WD_DEFAULT_NEW(wdShaderConstantBufferLayout);
        WD_SUCCEED_OR_RETURN(pLayout->Read(inout_stream));

        r.m_pLayout = pLayout;
      }
    }
  }

  if (uiVersion >= wdShaderStageBinary::Version5)
  {
    inout_stream >> m_bWasCompiledWithDebug;
  }

  return WD_SUCCESS;
}


wdDynamicArray<wdUInt8>& wdShaderStageBinary::GetByteCode()
{
  return m_ByteCode;
}

void wdShaderStageBinary::AddShaderResourceBinding(const wdShaderResourceBinding& binding)
{
  m_ShaderResourceBindings.PushBack(binding);
}


wdArrayPtr<const wdShaderResourceBinding> wdShaderStageBinary::GetShaderResourceBindings() const
{
  return m_ShaderResourceBindings;
}

const wdShaderResourceBinding* wdShaderStageBinary::GetShaderResourceBinding(const wdTempHashedString& sName) const
{
  for (auto& binding : m_ShaderResourceBindings)
  {
    if (binding.m_sName == sName)
    {
      return &binding;
    }
  }

  return nullptr;
}

wdShaderConstantBufferLayout* wdShaderStageBinary::CreateConstantBufferLayout() const
{
  return WD_DEFAULT_NEW(wdShaderConstantBufferLayout);
}

wdResult wdShaderStageBinary::WriteStageBinary(wdLogInterface* pLog) const
{
  wdStringBuilder sShaderStageFile = wdShaderManager::GetCacheDirectory();

  sShaderStageFile.AppendPath(wdShaderManager::GetActivePlatform().GetData());
  sShaderStageFile.AppendFormat("/{0}_{1}.wdShaderStage", wdGALShaderStage::Names[m_Stage], wdArgU(m_uiSourceHash, 8, true, 16, true));

  wdFileWriter StageFileOut;
  if (StageFileOut.Open(sShaderStageFile.GetData()).Failed())
  {
    wdLog::Error(pLog, "Could not open shader stage file '{0}' for writing", sShaderStageFile);
    return WD_FAILURE;
  }

  if (Write(StageFileOut).Failed())
  {
    wdLog::Error(pLog, "Could not write shader stage file '{0}'", sShaderStageFile);
    return WD_FAILURE;
  }

  return WD_SUCCESS;
}

// static
wdShaderStageBinary* wdShaderStageBinary::LoadStageBinary(wdGALShaderStage::Enum Stage, wdUInt32 uiHash)
{
  auto itStage = s_ShaderStageBinaries[Stage].Find(uiHash);

  if (!itStage.IsValid())
  {
    wdStringBuilder sShaderStageFile = wdShaderManager::GetCacheDirectory();

    sShaderStageFile.AppendPath(wdShaderManager::GetActivePlatform().GetData());
    sShaderStageFile.AppendFormat("/{0}_{1}.wdShaderStage", wdGALShaderStage::Names[Stage], wdArgU(uiHash, 8, true, 16, true));

    wdFileReader StageFileIn;
    if (StageFileIn.Open(sShaderStageFile.GetData()).Failed())
    {
      wdLog::Debug("Could not open shader stage file '{0}' for reading", sShaderStageFile);
      return nullptr;
    }

    wdShaderStageBinary shaderStageBinary;
    if (shaderStageBinary.Read(StageFileIn).Failed())
    {
      wdLog::Error("Could not read shader stage file '{0}'", sShaderStageFile);
      return nullptr;
    }

    itStage = wdShaderStageBinary::s_ShaderStageBinaries[Stage].Insert(uiHash, shaderStageBinary);
  }

  if (!itStage.IsValid())
  {
    return nullptr;
  }

  wdShaderStageBinary* pShaderStageBinary = &itStage.Value();

  if (pShaderStageBinary->m_GALByteCode == nullptr && !pShaderStageBinary->m_ByteCode.IsEmpty())
  {
    pShaderStageBinary->m_GALByteCode = WD_DEFAULT_NEW(wdGALShaderByteCode, pShaderStageBinary->m_ByteCode);
  }

  return pShaderStageBinary;
}

// static
void wdShaderStageBinary::OnEngineShutdown()
{
  for (wdUInt32 stage = 0; stage < wdGALShaderStage::ENUM_COUNT; ++stage)
  {
    s_ShaderStageBinaries[stage].Clear();
  }
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Shader_Implementation_ShaderStageBinary);
