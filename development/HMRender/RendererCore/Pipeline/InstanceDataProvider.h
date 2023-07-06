#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>

struct wdPerInstanceData;
class wdInstanceDataProvider;
class wdInstancedMeshComponent;

struct WD_RENDERERCORE_DLL wdInstanceData
{
  WD_DISALLOW_COPY_AND_ASSIGN(wdInstanceData);

public:
  wdInstanceData(wdUInt32 uiMaxInstanceCount = 1024);
  ~wdInstanceData();

  wdGALBufferHandle m_hInstanceDataBuffer;

  wdConstantBufferStorageHandle m_hConstantBuffer;

  void BindResources(wdRenderContext* pRenderContext);

  wdArrayPtr<wdPerInstanceData> GetInstanceData(wdUInt32 uiCount, wdUInt32& out_uiOffset);
  void UpdateInstanceData(wdRenderContext* pRenderContext, wdUInt32 uiCount);

private:
  friend wdInstanceDataProvider;
  friend wdInstancedMeshComponent;

  void CreateBuffer(wdUInt32 uiSize);
  void Reset();

  wdUInt32 m_uiBufferSize = 0;
  wdUInt32 m_uiBufferOffset = 0;
  wdDynamicArray<wdPerInstanceData, wdAlignedAllocatorWrapper> m_PerInstanceData;
};

class WD_RENDERERCORE_DLL wdInstanceDataProvider : public wdFrameDataProvider<wdInstanceData>
{
  WD_ADD_DYNAMIC_REFLECTION(wdInstanceDataProvider, wdFrameDataProviderBase);

public:
  wdInstanceDataProvider();
  ~wdInstanceDataProvider();

private:
  virtual void* UpdateData(const wdRenderViewContext& renderViewContext, const wdExtractedRenderData& extractedData) override;

  wdInstanceData m_Data;
};
