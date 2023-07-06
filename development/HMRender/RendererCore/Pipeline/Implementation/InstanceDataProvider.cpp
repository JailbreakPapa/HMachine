#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/InstanceDataProvider.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>

wdInstanceData::wdInstanceData(wdUInt32 uiMaxInstanceCount /*= 1024*/)

{
  CreateBuffer(uiMaxInstanceCount);

  m_hConstantBuffer = wdRenderContext::CreateConstantBufferStorage<wdObjectConstants>();
}

wdInstanceData::~wdInstanceData()
{
  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  pDevice->DestroyBuffer(m_hInstanceDataBuffer);

  wdRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void wdInstanceData::BindResources(wdRenderContext* pRenderContext)
{
  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  pRenderContext->BindBuffer("perInstanceData", pDevice->GetDefaultResourceView(m_hInstanceDataBuffer));
  pRenderContext->BindConstantBuffer("wdObjectConstants", m_hConstantBuffer);
}

wdArrayPtr<wdPerInstanceData> wdInstanceData::GetInstanceData(wdUInt32 uiCount, wdUInt32& out_uiOffset)
{
  uiCount = wdMath::Min(uiCount, m_uiBufferSize);
  if (m_uiBufferOffset + uiCount > m_uiBufferSize)
  {
    m_uiBufferOffset = 0;
  }

  out_uiOffset = m_uiBufferOffset;
  return m_PerInstanceData.GetArrayPtr().GetSubArray(m_uiBufferOffset, uiCount);
}

void wdInstanceData::UpdateInstanceData(wdRenderContext* pRenderContext, wdUInt32 uiCount)
{
  WD_ASSERT_DEV(m_uiBufferOffset + uiCount <= m_uiBufferSize, "Implementation error");

  wdGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();

  wdUInt32 uiDestOffset = m_uiBufferOffset * sizeof(wdPerInstanceData);
  auto pSourceData = m_PerInstanceData.GetArrayPtr().GetSubArray(m_uiBufferOffset, uiCount);
  wdGALUpdateMode::Enum updateMode = (m_uiBufferOffset == 0) ? wdGALUpdateMode::Discard : wdGALUpdateMode::NoOverwrite;

  pGALCommandEncoder->UpdateBuffer(m_hInstanceDataBuffer, uiDestOffset, pSourceData.ToByteArray(), updateMode);


  wdObjectConstants* pConstants = pRenderContext->GetConstantBufferData<wdObjectConstants>(m_hConstantBuffer);
  pConstants->InstanceDataOffset = m_uiBufferOffset;

  m_uiBufferOffset += uiCount;
}

void wdInstanceData::CreateBuffer(wdUInt32 uiSize)
{
  m_uiBufferSize = uiSize;
  m_PerInstanceData.SetCountUninitialized(m_uiBufferSize);

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  wdGALBufferCreationDescription desc;
  desc.m_uiStructSize = sizeof(wdPerInstanceData);
  desc.m_uiTotalSize = desc.m_uiStructSize * uiSize;
  desc.m_BufferType = wdGALBufferType::Generic;
  desc.m_bUseAsStructuredBuffer = true;
  desc.m_bAllowShaderResourceView = true;
  desc.m_ResourceAccess.m_bImmutable = false;

  m_hInstanceDataBuffer = pDevice->CreateBuffer(desc);
}

void wdInstanceData::Reset()
{
  m_uiBufferOffset = 0;
}

//////////////////////////////////////////////////////////////////////////

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdInstanceDataProvider, 1, wdRTTIDefaultAllocator<wdInstanceDataProvider>)
  {
  }
WD_END_DYNAMIC_REFLECTED_TYPE;

wdInstanceDataProvider::wdInstanceDataProvider() = default;

wdInstanceDataProvider::~wdInstanceDataProvider() = default;

void* wdInstanceDataProvider::UpdateData(const wdRenderViewContext& renderViewContext, const wdExtractedRenderData& extractedData)
{
  m_Data.Reset();

  return &m_Data;
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_InstanceDataProvider);
