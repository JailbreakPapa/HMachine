#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Pipeline/Passes/SimpleRenderPass.h>
#include <RendererCore/Pipeline/Passes/SourcePass.h>
#include <RendererCore/Pipeline/Passes/TargetPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>

void wdRenderPipelineResourceDescriptor::CreateFromRenderPipeline(const wdRenderPipeline* pPipeline)
{
  wdRenderPipelineResourceLoader::CreateRenderPipelineResourceDescriptor(pPipeline, *this);
}

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdRenderPipelineResource, 1, wdRTTIDefaultAllocator<wdRenderPipelineResource>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_RESOURCE_IMPLEMENT_COMMON_CODE(wdRenderPipelineResource);
// clang-format on

wdRenderPipelineResource::wdRenderPipelineResource()
  : wdResource(DoUpdate::OnAnyThread, 1)
{
}

wdInternal::NewInstance<wdRenderPipeline> wdRenderPipelineResource::CreateRenderPipeline() const
{
  if (GetLoadingState() != wdResourceState::Loaded)
  {
    wdLog::Error("Can't create render pipeline '{0}', the resource is not loaded!", GetResourceID());
    return wdInternal::NewInstance<wdRenderPipeline>(nullptr, nullptr);
  }

  return wdRenderPipelineResourceLoader::CreateRenderPipeline(m_Desc);
}

// static
wdRenderPipelineResourceHandle wdRenderPipelineResource::CreateMissingPipeline()
{
  wdUniquePtr<wdRenderPipeline> pRenderPipeline = WD_DEFAULT_NEW(wdRenderPipeline);

  wdSourcePass* pColorSourcePass = nullptr;
  {
    wdUniquePtr<wdSourcePass> pPass = WD_DEFAULT_NEW(wdSourcePass, "ColorSource");
    pColorSourcePass = pPass.Borrow();
    pRenderPipeline->AddPass(std::move(pPass));
  }

  wdSimpleRenderPass* pSimplePass = nullptr;
  {
    wdUniquePtr<wdSimpleRenderPass> pPass = WD_DEFAULT_NEW(wdSimpleRenderPass);
    pSimplePass = pPass.Borrow();
    pSimplePass->SetMessage("Render pipeline resource is missing. Ensure that the corresponding asset has been transformed.");
    pRenderPipeline->AddPass(std::move(pPass));
  }

  wdTargetPass* pTargetPass = nullptr;
  {
    wdUniquePtr<wdTargetPass> pPass = WD_DEFAULT_NEW(wdTargetPass);
    pTargetPass = pPass.Borrow();
    pRenderPipeline->AddPass(std::move(pPass));
  }

  WD_VERIFY(pRenderPipeline->Connect(pColorSourcePass, "Output", pSimplePass, "Color"), "Connect failed!");
  WD_VERIFY(pRenderPipeline->Connect(pSimplePass, "Color", pTargetPass, "Color0"), "Connect failed!");

  wdRenderPipelineResourceDescriptor desc;
  wdRenderPipelineResourceLoader::CreateRenderPipelineResourceDescriptor(pRenderPipeline.Borrow(), desc);

  return wdResourceManager::CreateResource<wdRenderPipelineResource>("MissingRenderPipeline", std::move(desc), "MissingRenderPipeline");
}

wdResourceLoadDesc wdRenderPipelineResource::UnloadData(Unload WhatToUnload)
{
  m_Desc.Clear();

  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::Unloaded;

  return res;
}

wdResourceLoadDesc wdRenderPipelineResource::UpdateContent(wdStreamReader* Stream)
{
  m_Desc.Clear();

  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::Loaded;

  if (Stream == nullptr)
  {
    res.m_State = wdResourceState::LoadedResourceMissing;
    return res;
  }

  wdStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  if (sAbsFilePath.HasExtension("wdRenderPipelineBin"))
  {
    wdStringBuilder sTemp, sTemp2;

    wdAssetFileHeader AssetHash;
    AssetHash.Read(*Stream).IgnoreResult();

    wdUInt8 uiVersion = 0;
    (*Stream) >> uiVersion;
    WD_ASSERT_DEV(uiVersion == 1, "Unknown wdRenderPipelineBin version {0}", uiVersion);

    wdUInt32 uiSize = 0;
    (*Stream) >> uiSize;

    m_Desc.m_SerializedPipeline.SetCountUninitialized(uiSize);
    Stream->ReadBytes(m_Desc.m_SerializedPipeline.GetData(), uiSize);

    WD_ASSERT_DEV(uiSize > 0, "RenderPipeline resourse contains no pipeline data!");
  }
  else
  {
    WD_REPORT_FAILURE("The file '{0}' is unsupported, only '.wdRenderPipelineBin' files can be loaded as wdRenderPipelineResource", sAbsFilePath);
  }

  return res;
}

void wdRenderPipelineResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(wdRenderPipelineResource) + (wdUInt32)(m_Desc.m_SerializedPipeline.GetCount());

  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

WD_RESOURCE_IMPLEMENT_CREATEABLE(wdRenderPipelineResource, wdRenderPipelineResourceDescriptor)
{
  m_Desc = descriptor;

  wdResourceLoadDesc res;
  res.m_State = wdResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  return res;
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelineResource);
