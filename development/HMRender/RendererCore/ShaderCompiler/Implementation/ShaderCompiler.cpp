#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdShaderProgramCompiler, 1, wdRTTINoAllocator)
WD_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  static bool PlatformEnabled(const wdString& sPlatforms, const char* szPlatform)
  {
    wdStringBuilder sTemp;
    sTemp = szPlatform;

    sTemp.Prepend("!");

    // if it contains '!platform'
    if (sPlatforms.FindWholeWord_NoCase(sTemp.GetData(), wdStringUtils::IsIdentifierDelimiter_C_Code) != nullptr)
      return false;

    sTemp = szPlatform;

    // if it contains 'platform'
    if (sPlatforms.FindWholeWord_NoCase(sTemp.GetData(), wdStringUtils::IsIdentifierDelimiter_C_Code) != nullptr)
      return true;

    // do not enable this when ALL is specified
    if (wdStringUtils::IsEqual(szPlatform, "DEBUG"))
      return false;

    // if it contains 'ALL'
    if (sPlatforms.FindWholeWord_NoCase("ALL", wdStringUtils::IsIdentifierDelimiter_C_Code) != nullptr)
      return true;

    return false;
  }

  static void GenerateDefines(const char* szPlatform, const wdArrayPtr<wdPermutationVar>& permutationVars, wdHybridArray<wdString, 32>& out_defines)
  {
    wdStringBuilder sTemp;

    if (out_defines.IsEmpty())
    {
      out_defines.PushBack("TRUE 1");
      out_defines.PushBack("FALSE 0");

      sTemp = szPlatform;
      sTemp.ToUpper();

      out_defines.PushBack(sTemp.GetData());
    }

    for (const wdPermutationVar& var : permutationVars)
    {
      const char* szValue = var.m_sValue.GetData();
      const bool isBoolVar = wdStringUtils::IsEqual(szValue, "TRUE") || wdStringUtils::IsEqual(szValue, "FALSE");

      if (isBoolVar)
      {
        sTemp.Set(var.m_sName, " ", var.m_sValue);
        out_defines.PushBack(sTemp);
      }
      else
      {
        const char* szName = var.m_sName.GetData();
        auto enumValues = wdShaderManager::GetPermutationEnumValues(var.m_sName);

        for (const auto& ev : enumValues)
        {
          sTemp.Format("{1} {2}", szName, ev.m_sValueName, ev.m_iValueValue);
          out_defines.PushBack(sTemp);
        }

        if (wdStringUtils::StartsWith(szValue, szName))
        {
          sTemp.Set(szName, " ", szValue);
        }
        else
        {
          sTemp.Set(szName, " ", szName, "_", szValue);
        }
        out_defines.PushBack(sTemp);
      }
    }
  }

  static const char* s_szStageDefines[wdGALShaderStage::ENUM_COUNT] = {"VERTEX_SHADER", "HULL_SHADER", "DOMAIN_SHADER", "GEOMETRY_SHADER", "PIXEL_SHADER", "COMPUTE_SHADER"};
} // namespace

wdResult wdShaderCompiler::FileOpen(const char* szAbsoluteFile, wdDynamicArray<wdUInt8>& FileContent, wdTimestamp& out_FileModification)
{
  if (wdStringUtils::IsEqual(szAbsoluteFile, "ShaderRenderState"))
  {
    const wdString& sData = m_ShaderData.m_StateSource;
    const wdUInt32 uiCount = sData.GetElementCount();
    const char* szString = sData.GetData();

    FileContent.SetCountUninitialized(uiCount);

    if (uiCount > 0)
    {
      wdMemoryUtils::Copy<wdUInt8>(FileContent.GetData(), (const wdUInt8*)szString, uiCount);
    }

    return WD_SUCCESS;
  }

  for (wdUInt32 stage = 0; stage < wdGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (m_StageSourceFile[stage] == szAbsoluteFile)
    {
      const wdString& sData = m_ShaderData.m_ShaderStageSource[stage];
      const wdUInt32 uiCount = sData.GetElementCount();
      const char* szString = sData.GetData();

      FileContent.SetCountUninitialized(uiCount);

      if (uiCount > 0)
      {
        wdMemoryUtils::Copy<wdUInt8>(FileContent.GetData(), (const wdUInt8*)szString, uiCount);
      }

      return WD_SUCCESS;
    }
  }

  m_IncludeFiles.Insert(szAbsoluteFile);

  wdFileReader r;
  if (r.Open(szAbsoluteFile).Failed())
  {
    wdLog::Error("Could not find include file '{0}'", szAbsoluteFile);
    return WD_FAILURE;
  }

#if WD_ENABLED(WD_SUPPORTS_FILE_STATS)
  wdFileStats stats;
  if (wdFileSystem::GetFileStats(szAbsoluteFile, stats).Succeeded())
  {
    out_FileModification = stats.m_LastModificationTime;
  }
#endif

  wdUInt8 Temp[4096];

  while (wdUInt64 uiRead = r.ReadBytes(Temp, 4096))
  {
    FileContent.PushBackRange(wdArrayPtr<wdUInt8>(Temp, (wdUInt32)uiRead));
  }

  return WD_SUCCESS;
}

wdResult wdShaderCompiler::CompileShaderPermutationForPlatforms(const char* szFile, const wdArrayPtr<const wdPermutationVar>& permutationVars, wdLogInterface* pLog, const char* szPlatform)
{
  wdStringBuilder sFileContent, sTemp;

  {
    wdFileReader File;
    if (File.Open(szFile).Failed())
      return WD_FAILURE;

    sFileContent.ReadAll(File);
  }

  wdShaderHelper::wdTextSectionizer Sections;
  wdShaderHelper::GetShaderSections(sFileContent.GetData(), Sections);

  wdUInt32 uiFirstLine = 0;
  sTemp = Sections.GetSectionContent(wdShaderHelper::wdShaderSections::PLATFORMS, uiFirstLine);
  sTemp.ToUpper();

  m_ShaderData.m_Platforms = sTemp;

  wdHybridArray<wdHashedString, 16> usedPermutations;
  wdShaderParser::ParsePermutationSection(Sections.GetSectionContent(wdShaderHelper::wdShaderSections::PERMUTATIONS, uiFirstLine), usedPermutations, m_ShaderData.m_FixedPermVars);

  for (const wdHashedString& usedPermutationVar : usedPermutations)
  {
    wdUInt32 uiIndex = wdInvalidIndex;
    for (wdUInt32 i = 0; i < permutationVars.GetCount(); ++i)
    {
      if (permutationVars[i].m_sName == usedPermutationVar)
      {
        uiIndex = i;
        break;
      }
    }

    if (uiIndex != wdInvalidIndex)
    {
      m_ShaderData.m_Permutations.PushBack(permutationVars[uiIndex]);
    }
    else
    {
      wdLog::Error("No value given for permutation var '{0}'. Assuming default value of zero.", usedPermutationVar);

      wdPermutationVar& finalVar = m_ShaderData.m_Permutations.ExpandAndGetRef();
      finalVar.m_sName = usedPermutationVar;
      finalVar.m_sValue.Assign("0");
    }
  }

  m_ShaderData.m_StateSource = Sections.GetSectionContent(wdShaderHelper::wdShaderSections::RENDERSTATE, uiFirstLine);

  wdUInt32 uiFirstShaderLine = 0;
  wdStringView sShaderSource = Sections.GetSectionContent(wdShaderHelper::wdShaderSections::SHADER, uiFirstShaderLine);

  for (wdUInt32 stage = wdGALShaderStage::VertexShader; stage < wdGALShaderStage::ENUM_COUNT; ++stage)
  {
    wdStringView sStageSource = Sections.GetSectionContent(wdShaderHelper::wdShaderSections::VERTEXSHADER + stage, uiFirstLine);

    // later code checks whether the string is empty, to see whether we have any shader source, so this has to be kept empty
    if (!sStageSource.IsEmpty())
    {
      sTemp.Clear();

      // prepend common shader section if there is any
      if (!sShaderSource.IsEmpty())
      {
        sTemp.AppendFormat("#line {0}\n{1}", uiFirstShaderLine, sShaderSource);
      }

      sTemp.AppendFormat("#line {0}\n{1}", uiFirstLine, sStageSource);

      m_ShaderData.m_ShaderStageSource[stage] = sTemp;
    }
    else
    {
      m_ShaderData.m_ShaderStageSource[stage].Clear();
    }
  }

  wdStringBuilder tmp = szFile;
  tmp.MakeCleanPath();

  m_StageSourceFile[wdGALShaderStage::VertexShader] = tmp;
  m_StageSourceFile[wdGALShaderStage::VertexShader].ChangeFileExtension("vs");

  m_StageSourceFile[wdGALShaderStage::HullShader] = tmp;
  m_StageSourceFile[wdGALShaderStage::HullShader].ChangeFileExtension("hs");

  m_StageSourceFile[wdGALShaderStage::DomainShader] = tmp;
  m_StageSourceFile[wdGALShaderStage::DomainShader].ChangeFileExtension("ds");

  m_StageSourceFile[wdGALShaderStage::GeometryShader] = tmp;
  m_StageSourceFile[wdGALShaderStage::GeometryShader].ChangeFileExtension("gs");

  m_StageSourceFile[wdGALShaderStage::PixelShader] = tmp;
  m_StageSourceFile[wdGALShaderStage::PixelShader].ChangeFileExtension("ps");

  m_StageSourceFile[wdGALShaderStage::ComputeShader] = tmp;
  m_StageSourceFile[wdGALShaderStage::ComputeShader].ChangeFileExtension("cs");

  // try out every compiler that we can find
  wdRTTI* pRtti = wdRTTI::GetFirstInstance();
  while (pRtti)
  {
    wdRTTIAllocator* pAllocator = pRtti->GetAllocator();
    if (pRtti->IsDerivedFrom<wdShaderProgramCompiler>() && pAllocator->CanAllocate())
    {
      wdShaderProgramCompiler* pCompiler = pAllocator->Allocate<wdShaderProgramCompiler>();

      const wdResult ret = RunShaderCompiler(szFile, szPlatform, pCompiler, pLog);
      pAllocator->Deallocate(pCompiler);

      if (ret.Failed())
        return ret;
    }

    pRtti = pRtti->GetNextInstance();
  }

  return WD_SUCCESS;
}

wdResult wdShaderCompiler::RunShaderCompiler(const char* szFile, const char* szPlatform, wdShaderProgramCompiler* pCompiler, wdLogInterface* pLog)
{
  WD_LOG_BLOCK(pLog, "Compiling Shader", szFile);

  wdStringBuilder sProcessed[wdGALShaderStage::ENUM_COUNT];

  wdHybridArray<wdString, 4> Platforms;
  pCompiler->GetSupportedPlatforms(Platforms);

  for (wdUInt32 p = 0; p < Platforms.GetCount(); ++p)
  {
    if (!PlatformEnabled(szPlatform, Platforms[p].GetData()))
      continue;

    // if this shader is not tagged for this platform, ignore it
    if (!PlatformEnabled(m_ShaderData.m_Platforms, Platforms[p].GetData()))
      continue;

    WD_LOG_BLOCK(pLog, "Platform", Platforms[p].GetData());

    wdShaderProgramCompiler::wdShaderProgramData spd;
    spd.m_szSourceFile = szFile;
    spd.m_szPlatform = Platforms[p].GetData();

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
    // 'DEBUG' is a platform tag that enables additional compiler flags
    if (PlatformEnabled(m_ShaderData.m_Platforms, "DEBUG"))
    {
      wdLog::Warning("Shader specifies the 'DEBUG' platform, which enables the debug shader compiler flag.");
      spd.m_Flags.Add(wdShaderCompilerFlags::Debug);
    }
#endif

    m_IncludeFiles.Clear();

    wdHybridArray<wdString, 32> defines;
    GenerateDefines(Platforms[p].GetData(), m_ShaderData.m_Permutations, defines);
    GenerateDefines(Platforms[p].GetData(), m_ShaderData.m_FixedPermVars, defines);

    wdShaderPermutationBinary shaderPermutationBinary;

    // Generate Shader State Source
    {
      WD_LOG_BLOCK(pLog, "Preprocessing Shader State Source");

      wdPreprocessor pp;
      pp.SetCustomFileCache(&m_FileCache);
      pp.SetLogInterface(wdLog::GetThreadLocalLogSystem());
      pp.SetFileOpenFunction(wdPreprocessor::FileOpenCB(&wdShaderCompiler::FileOpen, this));
      pp.SetPassThroughPragma(false);
      pp.SetPassThroughLine(false);

      for (auto& define : defines)
      {
        WD_SUCCEED_OR_RETURN(pp.AddCustomDefine(define));
      }

      bool bFoundUndefinedVars = false;
      pp.m_ProcessingEvents.AddEventHandler([&bFoundUndefinedVars](const wdPreprocessor::ProcessingEvent& e) {
        if (e.m_Type == wdPreprocessor::ProcessingEvent::EvaluateUnknown)
        {
          bFoundUndefinedVars = true;

          wdLog::Error("Undefined variable is evaluated: '{0}' (File: '{1}', Line: {2}", e.m_pToken->m_DataView, e.m_pToken->m_File, e.m_pToken->m_uiLine);
        }
      });

      wdStringBuilder sOutput;
      if (pp.Process("ShaderRenderState", sOutput, false).Failed() || bFoundUndefinedVars)
      {
        wdLog::Error(pLog, "Preprocessing the Shader State block failed");
        return WD_FAILURE;
      }
      else
      {
        if (shaderPermutationBinary.m_StateDescriptor.Load(sOutput).Failed())
        {
          wdLog::Error(pLog, "Failed to interpret the shader state block");
          return WD_FAILURE;
        }
      }
    }

    for (wdUInt32 stage = wdGALShaderStage::VertexShader; stage < wdGALShaderStage::ENUM_COUNT; ++stage)
    {
      spd.m_StageBinary[stage].m_Stage = (wdGALShaderStage::Enum)stage;
      spd.m_StageBinary[stage].m_uiSourceHash = 0;

      if (m_ShaderData.m_ShaderStageSource[stage].IsEmpty())
        continue;

      bool bFoundUndefinedVars = false;

      wdPreprocessor pp;
      pp.SetCustomFileCache(&m_FileCache);
      pp.SetLogInterface(wdLog::GetThreadLocalLogSystem());
      pp.SetFileOpenFunction(wdPreprocessor::FileOpenCB(&wdShaderCompiler::FileOpen, this));
      pp.SetPassThroughPragma(true);
      pp.SetPassThroughUnknownCmdsCB(wdMakeDelegate(&wdShaderCompiler::PassThroughUnknownCommandCB, this));
      pp.SetPassThroughLine(false);
      pp.m_ProcessingEvents.AddEventHandler([&bFoundUndefinedVars](const wdPreprocessor::ProcessingEvent& e) {
        if (e.m_Type == wdPreprocessor::ProcessingEvent::EvaluateUnknown)
        {
          bFoundUndefinedVars = true;

          wdLog::Error("Undefined variable is evaluated: '{0}' (File: '{1}', Line: {2}", e.m_pToken->m_DataView, e.m_pToken->m_File, e.m_pToken->m_uiLine);
        }
      });

      WD_SUCCEED_OR_RETURN(pp.AddCustomDefine(s_szStageDefines[stage]));
      for (auto& define : defines)
      {
        WD_SUCCEED_OR_RETURN(pp.AddCustomDefine(define));
      }

      wdUInt32 uiSourceStringLen = 0;
      if (pp.Process(m_StageSourceFile[stage], sProcessed[stage], true, true, true).Failed() || bFoundUndefinedVars)
      {
        sProcessed[stage].Clear();
        spd.m_szShaderSource[stage] = m_StageSourceFile[stage];
        uiSourceStringLen = m_StageSourceFile[stage].GetElementCount();

        wdLog::Error(pLog, "Shader preprocessing failed");
        return WD_FAILURE;
      }
      else
      {
        spd.m_szShaderSource[stage] = sProcessed[stage];
        uiSourceStringLen = sProcessed[stage].GetElementCount();
      }

      spd.m_StageBinary[stage].m_uiSourceHash = wdHashingUtils::xxHash32(spd.m_szShaderSource[stage], uiSourceStringLen);

      if (spd.m_StageBinary[stage].m_uiSourceHash != 0)
      {
        wdShaderStageBinary* pBinary = wdShaderStageBinary::LoadStageBinary((wdGALShaderStage::Enum)stage, spd.m_StageBinary[stage].m_uiSourceHash);

        if (pBinary)
        {
          spd.m_StageBinary[stage] = *pBinary;
          spd.m_bWriteToDisk[stage] = pBinary->GetByteCode().IsEmpty();
        }
      }
    }

    // copy the source hashes
    for (wdUInt32 stage = wdGALShaderStage::VertexShader; stage < wdGALShaderStage::ENUM_COUNT; ++stage)
    {
      shaderPermutationBinary.m_uiShaderStageHashes[stage] = spd.m_StageBinary[stage].m_uiSourceHash;
    }

    // if compilation failed, the stage binary for the source hash will simply not exist and therefore cannot be loaded
    // the .wdPermutation file should be updated, however, to store the new source hash to the broken shader
    if (pCompiler->Compile(spd, wdLog::GetThreadLocalLogSystem()).Failed())
    {
      WriteFailedShaderSource(spd, pLog);
      return WD_FAILURE;
    }

    for (wdUInt32 stage = wdGALShaderStage::VertexShader; stage < wdGALShaderStage::ENUM_COUNT; ++stage)
    {
      if (spd.m_StageBinary[stage].m_uiSourceHash != 0 && spd.m_bWriteToDisk[stage])
      {
        spd.m_StageBinary[stage].m_bWasCompiledWithDebug = spd.m_Flags.IsSet(wdShaderCompilerFlags::Debug);

        if (spd.m_StageBinary[stage].WriteStageBinary(pLog).Failed())
        {
          wdLog::Error(pLog, "Writing stage {0} binary failed", stage);
          return WD_FAILURE;
        }
      }
    }

    wdStringBuilder sTemp = wdShaderManager::GetCacheDirectory();
    sTemp.AppendPath(Platforms[p].GetData());
    sTemp.AppendPath(szFile);
    sTemp.ChangeFileExtension("");
    if (sTemp.EndsWith("."))
      sTemp.Shrink(0, 1);

    const wdUInt32 uiPermutationHash = wdShaderHelper::CalculateHash(m_ShaderData.m_Permutations);
    sTemp.AppendFormat("_{0}.wdPermutation", wdArgU(uiPermutationHash, 8, true, 16, true));

    shaderPermutationBinary.m_DependencyFile.Clear();
    shaderPermutationBinary.m_DependencyFile.AddFileDependency(szFile);

    for (auto it = m_IncludeFiles.GetIterator(); it.IsValid(); ++it)
    {
      shaderPermutationBinary.m_DependencyFile.AddFileDependency(it.Key());
    }

    shaderPermutationBinary.m_PermutationVars = m_ShaderData.m_Permutations;

    wdDeferredFileWriter PermutationFileOut;
    PermutationFileOut.SetOutput(sTemp.GetData());
    WD_SUCCEED_OR_RETURN(shaderPermutationBinary.Write(PermutationFileOut));

    if (PermutationFileOut.Close().Failed())
    {
      wdLog::Error(pLog, "Could not open file for writing: '{0}'", sTemp);
      return WD_FAILURE;
    }
  }

  return WD_SUCCESS;
}


void wdShaderCompiler::WriteFailedShaderSource(wdShaderProgramCompiler::wdShaderProgramData& spd, wdLogInterface* pLog)
{
  for (wdUInt32 stage = wdGALShaderStage::VertexShader; stage < wdGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (spd.m_StageBinary[stage].m_uiSourceHash != 0 && spd.m_bWriteToDisk[stage])
    {
      wdStringBuilder sShaderStageFile = wdShaderManager::GetCacheDirectory();

      sShaderStageFile.AppendPath(wdShaderManager::GetActivePlatform().GetData());
      sShaderStageFile.AppendFormat("/_Failed_{0}_{1}.wdShaderSource", wdGALShaderStage::Names[stage], wdArgU(spd.m_StageBinary[stage].m_uiSourceHash, 8, true, 16, true));

      wdFileWriter StageFileOut;
      if (StageFileOut.Open(sShaderStageFile.GetData()).Succeeded())
      {
        StageFileOut.WriteBytes(spd.m_szShaderSource[stage], wdStringUtils::GetStringElementCount(spd.m_szShaderSource[stage])).IgnoreResult();
        wdLog::Info(pLog, "Failed shader source written to '{0}'", sShaderStageFile);
      }
    }
  }
}

WD_STATICLINK_FILE(RendererCore, RendererCore_ShaderCompiler_Implementation_ShaderCompiler);
