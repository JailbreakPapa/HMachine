#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdShaderResource, 1, wdRTTIDefaultAllocator<wdShaderResource>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_RESOURCE_IMPLEMENT_COMMON_CODE(wdShaderResource);
// clang-format on

wdShaderResource::wdShaderResource()
  : wdResource(DoUpdate::OnAnyThread, 1)
{
  m_bShaderResourceIsValid = false;
}

wdResourceLoadDesc wdShaderResource::UnloadData(Unload WhatToUnload)
{
  m_bShaderResourceIsValid = false;
  m_PermutationVarsUsed.Clear();

  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::Unloaded;

  return res;
}

wdResourceLoadDesc wdShaderResource::UpdateContent(wdStreamReader* stream)
{
  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  m_bShaderResourceIsValid = false;

  if (stream == nullptr)
  {
    res.m_State = wdResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    wdStringBuilder sAbsFilePath;
    (*stream) >> sAbsFilePath;
  }

  wdHybridArray<wdPermutationVar, 16> fixedPermVars; // ignored here
  wdShaderParser::ParsePermutationSection(*stream, m_PermutationVarsUsed, fixedPermVars);

  res.m_State = wdResourceState::Loaded;
  m_bShaderResourceIsValid = true;

  return res;
}

void wdShaderResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(wdShaderResource) + (wdUInt32)m_PermutationVarsUsed.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

WD_RESOURCE_IMPLEMENT_CREATEABLE(wdShaderResource, wdShaderResourceDescriptor)
{
  wdResourceLoadDesc ret;
  ret.m_State = wdResourceState::Loaded;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable = 0;

  m_bShaderResourceIsValid = false;

  return ret;
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Shader_Implementation_ShaderResource);
