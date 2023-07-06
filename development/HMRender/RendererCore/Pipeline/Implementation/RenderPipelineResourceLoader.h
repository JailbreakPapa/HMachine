#pragma once

#include <Foundation/Serialization/RttiConverter.h>
#include <RendererCore/RendererCoreDLL.h>

class wdRenderPipeline;
struct wdRenderPipelineResourceDescriptor;

struct WD_RENDERERCORE_DLL wdRenderPipelineResourceLoader
{
  static wdInternal::NewInstance<wdRenderPipeline> CreateRenderPipeline(const wdRenderPipelineResourceDescriptor& desc);
  static void CreateRenderPipelineResourceDescriptor(const wdRenderPipeline* pPipeline, wdRenderPipelineResourceDescriptor& ref_desc);
};

class WD_RENDERERCORE_DLL wdRenderPipelineRttiConverterContext : public wdRttiConverterContext
{
public:
  wdRenderPipelineRttiConverterContext()

    = default;

  virtual void Clear() override;

  virtual wdInternal::NewInstance<void> CreateObject(const wdUuid& guid, const wdRTTI* pRtti) override;
  virtual void DeleteObject(const wdUuid& guid) override;

  wdRenderPipeline* m_pRenderPipeline = nullptr;
};
