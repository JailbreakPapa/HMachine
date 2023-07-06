#include <RendererCore/RendererCorePCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>
#include <RendererCore/Shader/Implementation/Helper.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>

bool wdShaderManager::s_bEnableRuntimeCompilation = false;
wdString wdShaderManager::s_sPlatform;
wdString wdShaderManager::s_sPermVarSubDir;
wdString wdShaderManager::s_sShaderCacheDirectory;

namespace
{
  struct PermutationVarConfig
  {
    wdHashedString m_sName;
    wdVariant m_DefaultValue;
    wdDynamicArray<wdShaderParser::EnumValue, wdStaticAllocatorWrapper> m_EnumValues;
  };

  static wdDeque<PermutationVarConfig, wdStaticAllocatorWrapper> s_PermutationVarConfigsStorage;
  static wdHashTable<wdHashedString, PermutationVarConfig*> s_PermutationVarConfigs;
  static wdMutex s_PermutationVarConfigsMutex;

  const PermutationVarConfig* FindConfig(const char* szName, const wdTempHashedString& sHashedName)
  {
    WD_LOCK(s_PermutationVarConfigsMutex);

    PermutationVarConfig* pConfig = nullptr;
    if (!s_PermutationVarConfigs.TryGetValue(sHashedName, pConfig))
    {
      wdShaderManager::ReloadPermutationVarConfig(szName, sHashedName);
      s_PermutationVarConfigs.TryGetValue(sHashedName, pConfig);
    }

    return pConfig;
  }

  const PermutationVarConfig* FindConfig(const wdHashedString& sName)
  {
    WD_LOCK(s_PermutationVarConfigsMutex);

    PermutationVarConfig* pConfig = nullptr;
    if (!s_PermutationVarConfigs.TryGetValue(sName, pConfig))
    {
      wdShaderManager::ReloadPermutationVarConfig(sName.GetData(), sName);
      s_PermutationVarConfigs.TryGetValue(sName, pConfig);
    }

    return pConfig;
  }

  static wdHashedString s_sTrue = wdMakeHashedString("TRUE");
  static wdHashedString s_sFalse = wdMakeHashedString("FALSE");

  bool IsValueAllowed(const PermutationVarConfig& config, const wdTempHashedString& sValue, wdHashedString& out_sValue)
  {
    if (config.m_DefaultValue.IsA<bool>())
    {
      if (sValue == s_sTrue)
      {
        out_sValue = s_sTrue;
        return true;
      }

      if (sValue == s_sFalse)
      {
        out_sValue = s_sFalse;
        return true;
      }
    }
    else
    {
      for (auto& enumValue : config.m_EnumValues)
      {
        if (enumValue.m_sValueName == sValue)
        {
          out_sValue = enumValue.m_sValueName;
          return true;
        }
      }
    }

    return false;
  }

  bool IsValueAllowed(const PermutationVarConfig& config, const wdTempHashedString& sValue)
  {
    if (config.m_DefaultValue.IsA<bool>())
    {
      return sValue == s_sTrue || sValue == s_sFalse;
    }
    else
    {
      for (auto& enumValue : config.m_EnumValues)
      {
        if (enumValue.m_sValueName == sValue)
          return true;
      }
    }

    return false;
  }

  static wdHashTable<wdUInt64, wdString> s_PermutationPaths;
} // namespace

//////////////////////////////////////////////////////////////////////////

void wdShaderManager::Configure(const char* szActivePlatform, bool bEnableRuntimeCompilation, const char* szShaderCacheDirectory, const char* szPermVarSubDirectory)
{
  s_sShaderCacheDirectory = szShaderCacheDirectory;
  s_sPermVarSubDir = szPermVarSubDirectory;

  wdStringBuilder s = szActivePlatform;
  s.ToUpper();

  s_bEnableRuntimeCompilation = bEnableRuntimeCompilation;
  s_sPlatform = s;
}

void wdShaderManager::ReloadPermutationVarConfig(const char* szName, const wdTempHashedString& sHashedName)
{
  // clear earlier data
  {
    WD_LOCK(s_PermutationVarConfigsMutex);

    s_PermutationVarConfigs.Remove(sHashedName);
  }

  wdStringBuilder sPath;
  sPath.Format("{0}/{1}.wdPermVar", s_sPermVarSubDir, szName);

  wdStringBuilder sTemp = s_sPlatform;
  sTemp.Append(" 1");

  wdPreprocessor pp;
  pp.SetLogInterface(wdLog::GetThreadLocalLogSystem());
  pp.SetPassThroughLine(false);
  pp.SetPassThroughPragma(false);
  pp.AddCustomDefine(sTemp.GetData()).IgnoreResult();

  if (pp.Process(sPath, sTemp, false).Failed())
  {
    wdLog::Error("Could not read shader permutation variable '{0}' from file '{1}'", szName, sPath);
  }

  wdVariant defaultValue;
  wdShaderParser::EnumDefinition enumDef;

  wdShaderParser::ParsePermutationVarConfig(sTemp, defaultValue, enumDef);
  if (defaultValue.IsValid())
  {
    WD_LOCK(s_PermutationVarConfigsMutex);

    auto pConfig = &s_PermutationVarConfigsStorage.ExpandAndGetRef();
    pConfig->m_sName.Assign(szName);
    pConfig->m_DefaultValue = defaultValue;
    pConfig->m_EnumValues = enumDef.m_Values;

    s_PermutationVarConfigs.Insert(pConfig->m_sName, pConfig);
  }
}

bool wdShaderManager::IsPermutationValueAllowed(const char* szName, const wdTempHashedString& sHashedName, const wdTempHashedString& sValue, wdHashedString& out_sName, wdHashedString& out_sValue)
{
  const PermutationVarConfig* pConfig = FindConfig(szName, sHashedName);
  if (pConfig == nullptr)
  {
    wdLog::Error("Permutation variable '{0}' does not exist", szName);
    return false;
  }

  out_sName = pConfig->m_sName;

  if (!IsValueAllowed(*pConfig, sValue, out_sValue))
  {
    if (!s_bEnableRuntimeCompilation)
    {
      return false;
    }

    wdLog::Debug("Invalid Shader Permutation: '{0}' cannot be set to value '{1}' -> reloading config for variable", szName, sValue.GetHash());
    ReloadPermutationVarConfig(szName, sHashedName);

    if (!IsValueAllowed(*pConfig, sValue, out_sValue))
    {
      wdLog::Error("Invalid Shader Permutation: '{0}' cannot be set to value '{1}'", szName, sValue.GetHash());
      return false;
    }
  }

  return true;
}

bool wdShaderManager::IsPermutationValueAllowed(const wdHashedString& sName, const wdHashedString& sValue)
{
  const PermutationVarConfig* pConfig = FindConfig(sName);
  if (pConfig == nullptr)
  {
    wdLog::Error("Permutation variable '{0}' does not exist", sName);
    return false;
  }

  if (!IsValueAllowed(*pConfig, sValue))
  {
    if (!s_bEnableRuntimeCompilation)
    {
      return false;
    }

    wdLog::Debug("Invalid Shader Permutation: '{0}' cannot be set to value '{1}' -> reloading config for variable", sName, sValue);
    ReloadPermutationVarConfig(sName, sName);

    if (!IsValueAllowed(*pConfig, sValue))
    {
      wdLog::Error("Invalid Shader Permutation: '{0}' cannot be set to value '{1}'", sName, sValue);
      return false;
    }
  }

  return true;
}

void wdShaderManager::GetPermutationValues(const wdHashedString& sName, wdDynamicArray<wdHashedString>& out_values)
{
  out_values.Clear();

  const PermutationVarConfig* pConfig = FindConfig(sName);
  if (pConfig == nullptr)
    return;

  if (pConfig->m_DefaultValue.IsA<bool>())
  {
    out_values.PushBack(s_sTrue);
    out_values.PushBack(s_sFalse);
  }
  else
  {
    for (const auto& val : pConfig->m_EnumValues)
    {
      out_values.PushBack(val.m_sValueName);
    }
  }
}

wdArrayPtr<const wdShaderParser::EnumValue> wdShaderManager::GetPermutationEnumValues(const wdHashedString& sName)
{
  const PermutationVarConfig* pConfig = FindConfig(sName);
  if (pConfig != nullptr)
  {
    return pConfig->m_EnumValues;
  }

  return {};
}

void wdShaderManager::PreloadPermutations(wdShaderResourceHandle hShader, const wdHashTable<wdHashedString, wdHashedString>& permVars, wdTime shouldBeAvailableIn)
{
  WD_ASSERT_NOT_IMPLEMENTED;
#if 0
  wdResourceLock<wdShaderResource> pShader(hShader, wdResourceAcquireMode::BlockTillLoaded);

  if (!pShader->IsShaderValid())
    return;

  /*wdUInt32 uiPermutationHash = */ FilterPermutationVars(pShader->GetUsedPermutationVars(), permVars);

  generator.RemoveUnusedPermutations(pShader->GetUsedPermutationVars());

  wdHybridArray<wdPermutationVar, 16> usedPermVars;

  const wdUInt32 uiPermutationCount = generator.GetPermutationCount();
  for (wdUInt32 uiPermutation = 0; uiPermutation < uiPermutationCount; ++uiPermutation)
  {
    generator.GetPermutation(uiPermutation, usedPermVars);

    PreloadSingleShaderPermutation(hShader, usedPermVars, tShouldBeAvailableIn);
  }
#endif
}

wdShaderPermutationResourceHandle wdShaderManager::PreloadSinglePermutation(wdShaderResourceHandle hShader, const wdHashTable<wdHashedString, wdHashedString>& permVars, bool bAllowFallback)
{
  wdResourceLock<wdShaderResource> pShader(hShader, bAllowFallback ? wdResourceAcquireMode::AllowLoadingFallback : wdResourceAcquireMode::BlockTillLoaded);

  if (!pShader->IsShaderValid())
    return wdShaderPermutationResourceHandle();

  wdHybridArray<wdPermutationVar, 64> filteredPermutationVariables(wdFrameAllocator::GetCurrentAllocator());
  wdUInt32 uiPermutationHash = FilterPermutationVars(pShader->GetUsedPermutationVars(), permVars, filteredPermutationVariables);

  return PreloadSinglePermutationInternal(pShader->GetResourceID(), pShader->GetResourceIDHash(), uiPermutationHash, filteredPermutationVariables);
}


wdUInt32 wdShaderManager::FilterPermutationVars(wdArrayPtr<const wdHashedString> usedVars, const wdHashTable<wdHashedString, wdHashedString>& permVars, wdDynamicArray<wdPermutationVar>& out_FilteredPermutationVariables)
{
  for (auto& sName : usedVars)
  {
    auto& var = out_FilteredPermutationVariables.ExpandAndGetRef();
    var.m_sName = sName;

    if (!permVars.TryGetValue(sName, var.m_sValue))
    {
      const PermutationVarConfig* pConfig = FindConfig(sName);
      if (pConfig == nullptr)
        continue;

      const wdVariant& defaultValue = pConfig->m_DefaultValue;
      if (defaultValue.IsA<bool>())
      {
        var.m_sValue = defaultValue.Get<bool>() ? s_sTrue : s_sFalse;
      }
      else
      {
        wdUInt32 uiDefaultValue = defaultValue.Get<wdUInt32>();
        var.m_sValue = pConfig->m_EnumValues[uiDefaultValue].m_sValueName;
      }
    }
  }

  return wdShaderHelper::CalculateHash(out_FilteredPermutationVariables);
}



wdShaderPermutationResourceHandle wdShaderManager::PreloadSinglePermutationInternal(const char* szResourceId, wdUInt64 uiResourceIdHash, wdUInt32 uiPermutationHash, wdArrayPtr<wdPermutationVar> filteredPermutationVariables)
{
  const wdUInt64 uiPermutationKey = (wdUInt64)wdHashingUtils::StringHashTo32(uiResourceIdHash) << 32 | uiPermutationHash;

  wdString* pPermutationPath = &s_PermutationPaths[uiPermutationKey];
  if (pPermutationPath->IsEmpty())
  {
    wdStringBuilder sShaderFile = GetCacheDirectory();
    sShaderFile.AppendPath(GetActivePlatform().GetData());
    sShaderFile.AppendPath(szResourceId);
    sShaderFile.ChangeFileExtension("");
    if (sShaderFile.EndsWith("."))
      sShaderFile.Shrink(0, 1);
    sShaderFile.AppendFormat("_{0}.wdPermutation", wdArgU(uiPermutationHash, 8, true, 16, true));

    *pPermutationPath = sShaderFile;
  }

  wdShaderPermutationResourceHandle hShaderPermutation = wdResourceManager::LoadResource<wdShaderPermutationResource>(pPermutationPath->GetData());

  {
    wdResourceLock<wdShaderPermutationResource> pShaderPermutation(hShaderPermutation, wdResourceAcquireMode::PointerOnly);
    if (!pShaderPermutation->IsShaderValid())
    {
      pShaderPermutation->m_PermutationVars = filteredPermutationVariables;
    }
  }

  wdResourceManager::PreloadResource(hShaderPermutation);

  return hShaderPermutation;
}

WD_STATICLINK_FILE(RendererCore, RendererCore_ShaderCompiler_Implementation_ShaderManager);
