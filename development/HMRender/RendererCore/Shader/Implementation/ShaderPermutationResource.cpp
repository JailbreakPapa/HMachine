#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Shader/Shader.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdShaderPermutationResource, 1, wdRTTIDefaultAllocator<wdShaderPermutationResource>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_RESOURCE_IMPLEMENT_COMMON_CODE(wdShaderPermutationResource);
// clang-format on

static wdShaderPermutationResourceLoader g_PermutationResourceLoader;

wdShaderPermutationResource::wdShaderPermutationResource()
  : wdResource(DoUpdate::OnAnyThread, 1)
{
  m_bShaderPermutationValid = false;

  for (wdUInt32 stage = 0; stage < wdGALShaderStage::ENUM_COUNT; ++stage)
  {
    m_pShaderStageBinaries[stage] = nullptr;
  }
}

wdResourceLoadDesc wdShaderPermutationResource::UnloadData(Unload WhatToUnload)
{
  m_bShaderPermutationValid = false;

  auto pDevice = wdGALDevice::GetDefaultDevice();

  if (!m_hShader.IsInvalidated())
  {
    pDevice->DestroyShader(m_hShader);
    m_hShader.Invalidate();
  }

  if (!m_hBlendState.IsInvalidated())
  {
    pDevice->DestroyBlendState(m_hBlendState);
    m_hBlendState.Invalidate();
  }

  if (!m_hDepthStencilState.IsInvalidated())
  {
    pDevice->DestroyDepthStencilState(m_hDepthStencilState);
    m_hDepthStencilState.Invalidate();
  }

  if (!m_hRasterizerState.IsInvalidated())
  {
    pDevice->DestroyRasterizerState(m_hRasterizerState);
    m_hRasterizerState.Invalidate();
  }


  wdResourceLoadDesc res;
  res.m_State = wdResourceState::Unloaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  return res;
}

wdResourceLoadDesc wdShaderPermutationResource::UpdateContent(wdStreamReader* Stream)
{
  wdUInt32 uiGPUMem = 0;
  ModifyMemoryUsage().m_uiMemoryGPU = 0;

  m_bShaderPermutationValid = false;

  wdResourceLoadDesc res;
  res.m_State = wdResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    wdLog::Error("Shader Permutation '{0}': Data is not available", GetResourceID());
    return res;
  }

  wdShaderPermutationBinary PermutationBinary;

  bool bOldVersion = false;
  if (PermutationBinary.Read(*Stream, bOldVersion).Failed())
  {
    wdLog::Error("Shader Permutation '{0}': Could not read shader permutation binary", GetResourceID());
    return res;
  }

  auto pDevice = wdGALDevice::GetDefaultDevice();

  // get the shader render state object
  {
    m_hBlendState = pDevice->CreateBlendState(PermutationBinary.m_StateDescriptor.m_BlendDesc);
    m_hDepthStencilState = pDevice->CreateDepthStencilState(PermutationBinary.m_StateDescriptor.m_DepthStencilDesc);
    m_hRasterizerState = pDevice->CreateRasterizerState(PermutationBinary.m_StateDescriptor.m_RasterizerDesc);
  }

  wdGALShaderCreationDescription ShaderDesc;

  // iterate over all shader stages, add them to the descriptor
  for (wdUInt32 stage = wdGALShaderStage::VertexShader; stage < wdGALShaderStage::ENUM_COUNT; ++stage)
  {
    const wdUInt32 uiStageHash = PermutationBinary.m_uiShaderStageHashes[stage];

    if (uiStageHash == 0) // not used
      continue;

    wdShaderStageBinary* pStageBin = wdShaderStageBinary::LoadStageBinary((wdGALShaderStage::Enum)stage, uiStageHash);

    if (pStageBin == nullptr)
    {
      wdLog::Error("Shader Permutation '{0}': Stage '{1}' could not be loaded", GetResourceID(), wdGALShaderStage::Names[stage]);
      return res;
    }

    // store not only the hash but also the pointer to the stage binary
    // since it contains other useful information (resource bindings), that we need for shader binding
    m_pShaderStageBinaries[stage] = pStageBin;

    WD_ASSERT_DEV(pStageBin->m_Stage == stage, "Invalid shader stage! Expected stage '{0}', but loaded data is for stage '{1}'", wdGALShaderStage::Names[stage], wdGALShaderStage::Names[pStageBin->m_Stage]);

    ShaderDesc.m_ByteCodes[stage] = pStageBin->m_GALByteCode;

    uiGPUMem += pStageBin->m_ByteCode.GetCount();
  }

  m_hShader = pDevice->CreateShader(ShaderDesc);

  if (m_hShader.IsInvalidated())
  {
    wdLog::Error("Shader Permutation '{0}': Shader program creation failed", GetResourceID());
    return res;
  }

  pDevice->GetShader(m_hShader)->SetDebugName(GetResourceID());

  m_PermutationVars = PermutationBinary.m_PermutationVars;

  m_bShaderPermutationValid = true;

  ModifyMemoryUsage().m_uiMemoryGPU = uiGPUMem;

  return res;
}

void wdShaderPermutationResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(wdShaderPermutationResource);
  out_NewMemoryUsage.m_uiMemoryGPU = ModifyMemoryUsage().m_uiMemoryGPU;
}

WD_RESOURCE_IMPLEMENT_CREATEABLE(wdShaderPermutationResource, wdShaderPermutationResourceDescriptor)
{
  wdResourceLoadDesc ret;
  ret.m_State = wdResourceState::Loaded;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable = 0;

  return ret;
}

wdResourceTypeLoader* wdShaderPermutationResource::GetDefaultResourceTypeLoader() const
{
  return &g_PermutationResourceLoader;
}

struct ShaderPermutationResourceLoadData
{
  ShaderPermutationResourceLoadData()
    : m_Reader(&m_Storage)
  {
  }

  wdContiguousMemoryStreamStorage m_Storage;
  wdMemoryStreamReader m_Reader;
};

wdResult wdShaderPermutationResourceLoader::RunCompiler(const wdResource* pResource, wdShaderPermutationBinary& BinaryInfo, bool bForce)
{
  if (wdShaderManager::IsRuntimeCompilationEnabled())
  {
    if (!bForce)
    {
      // check whether any dependent file has changed, and trigger a recompilation if necessary
      if (BinaryInfo.m_DependencyFile.HasAnyFileChanged())
      {
        bForce = true;
      }
    }

    if (!bForce) // no recompilation necessary
      return WD_SUCCESS;

    wdStringBuilder sPermutationFile = pResource->GetResourceID();

    sPermutationFile.ChangeFileExtension("");
    sPermutationFile.Shrink(wdShaderManager::GetCacheDirectory().GetCharacterCount() + wdShaderManager::GetActivePlatform().GetCharacterCount() + 2, 1);

    sPermutationFile.Shrink(0, 9); // remove underscore and the hash at the end
    sPermutationFile.Append(".wdShader");

    wdArrayPtr<const wdPermutationVar> permutationVars = static_cast<const wdShaderPermutationResource*>(pResource)->GetPermutationVars();

    wdShaderCompiler sc;
    return sc.CompileShaderPermutationForPlatforms(sPermutationFile, permutationVars, wdLog::GetThreadLocalLogSystem(), wdShaderManager::GetActivePlatform());
  }
  else
  {
    if (bForce)
    {
      wdLog::Error("Shader was forced to be compiled, but runtime shader compilation is not available");
      return WD_FAILURE;
    }
  }

  return WD_SUCCESS;
}

bool wdShaderPermutationResourceLoader::IsResourceOutdated(const wdResource* pResource) const
{
  // don't try to reload a file that cannot be found
  wdStringBuilder sAbs;
  if (wdFileSystem::ResolvePath(pResource->GetResourceID(), &sAbs, nullptr).Failed())
    return false;

#if WD_ENABLED(WD_SUPPORTS_FILE_STATS)
  if (pResource->GetLoadedFileModificationTime().IsValid())
  {
    wdFileStats stat;
    if (wdFileSystem::GetFileStats(pResource->GetResourceID(), stat).Failed())
      return false;

    if (!stat.m_LastModificationTime.Compare(pResource->GetLoadedFileModificationTime(), wdTimestamp::CompareMode::FileTimeEqual))
      return true;
  }

#endif

  wdDependencyFile dep;
  if (dep.ReadDependencyFile(pResource->GetResourceID()).Failed())
    return true;

  return dep.HasAnyFileChanged();
}

wdResourceLoadData wdShaderPermutationResourceLoader::OpenDataStream(const wdResource* pResource)
{
  wdResourceLoadData res;

  wdShaderPermutationBinary permutationBinary;

  bool bNeedsCompilation = true;
  bool bOldVersion = false;

  {
    wdFileReader File;
    if (File.Open(pResource->GetResourceID().GetData()).Failed())
    {
      wdLog::Debug("Shader Permutation '{0}' does not exist, triggering recompile.", pResource->GetResourceID());

      bNeedsCompilation = false;
      if (RunCompiler(pResource, permutationBinary, true).Failed())
        return res;

      // try again
      if (File.Open(pResource->GetResourceID().GetData()).Failed())
      {
        wdLog::Debug("Shader Permutation '{0}' still does not exist after recompile.", pResource->GetResourceID());
        return res;
      }
    }

    res.m_sResourceDescription = File.GetFilePathRelative().GetData();

#if WD_ENABLED(WD_SUPPORTS_FILE_STATS)
    wdFileStats stat;
    if (wdFileSystem::GetFileStats(pResource->GetResourceID(), stat).Succeeded())
    {
      res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
    }
#endif

    if (permutationBinary.Read(File, bOldVersion).Failed())
    {
      wdLog::Error("Shader Permutation '{0}': Could not read shader permutation binary", pResource->GetResourceID());

      bNeedsCompilation = true;
    }

    if (bOldVersion)
    {
      wdLog::Dev("Shader Permutation Binary version is outdated, recompiling shader.");
      bNeedsCompilation = true;
    }
  }

  if (bNeedsCompilation)
  {
    if (RunCompiler(pResource, permutationBinary, false).Failed())
      return res;

    wdFileReader File;

    if (File.Open(pResource->GetResourceID().GetData()).Failed())
    {
      wdLog::Error("Shader Permutation '{0}': Failed to open the file", pResource->GetResourceID());
      return res;
    }

    if (permutationBinary.Read(File, bOldVersion).Failed())
    {
      wdLog::Error("Shader Permutation '{0}': Binary data could not be read", pResource->GetResourceID());
      return res;
    }

    File.Close();
  }



  ShaderPermutationResourceLoadData* pData = WD_DEFAULT_NEW(ShaderPermutationResourceLoadData);

  wdMemoryStreamWriter w(&pData->m_Storage);

  // preload the files that are referenced in the .wdPermutation file
  {
    // write the permutation file info back to the output stream, so that the resource can read it as well
    permutationBinary.Write(w).IgnoreResult();

    for (wdUInt32 stage = wdGALShaderStage::VertexShader; stage < wdGALShaderStage::ENUM_COUNT; ++stage)
    {
      const wdUInt32 uiStageHash = permutationBinary.m_uiShaderStageHashes[stage];

      if (uiStageHash == 0) // not used
        continue;

      // this is where the preloading happens
      wdShaderStageBinary::LoadStageBinary((wdGALShaderStage::Enum)stage, uiStageHash);
    }
  }

  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  return res;
}

void wdShaderPermutationResourceLoader::CloseDataStream(const wdResource* pResource, const wdResourceLoadData& loaderData)
{
  ShaderPermutationResourceLoadData* pData = static_cast<ShaderPermutationResourceLoadData*>(loaderData.m_pCustomLoaderData);

  WD_DEFAULT_DELETE(pData);
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Shader_Implementation_ShaderPermutationResource);
