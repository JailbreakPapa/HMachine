#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Meshes/SkinnedMeshComponent.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/Types.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdSkinnedMeshRenderData, 1, wdRTTIDefaultAllocator<wdSkinnedMeshRenderData>)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void wdSkinnedMeshRenderData::FillBatchIdAndSortingKey()
{
  FillBatchIdAndSortingKeyInternal(m_hSkinningTransforms.GetInternalID().m_Data);
}

wdSkinningState::wdSkinningState() = default;

wdSkinningState::~wdSkinningState()
{
  Clear();
}

void wdSkinningState::Clear()
{
  if (!m_hGpuBuffer.IsInvalidated())
  {
    wdGALDevice::GetDefaultDevice()->DestroyBuffer(m_hGpuBuffer);
    m_hGpuBuffer.Invalidate();
  }

  m_bTransformsUpdated[0] = nullptr;
  m_bTransformsUpdated[1] = nullptr;
  m_Transforms.Clear();
}

void wdSkinningState::TransformsChanged()
{
  if (m_hGpuBuffer.IsInvalidated())
  {
    if (m_Transforms.GetCount() == 0)
      return;

    wdGALBufferCreationDescription BufferDesc;
    BufferDesc.m_uiStructSize = sizeof(wdShaderTransform);
    BufferDesc.m_uiTotalSize = BufferDesc.m_uiStructSize * m_Transforms.GetCount();
    BufferDesc.m_bUseAsStructuredBuffer = true;
    BufferDesc.m_bAllowShaderResourceView = true;
    BufferDesc.m_ResourceAccess.m_bImmutable = false;

    m_hGpuBuffer = wdGALDevice::GetDefaultDevice()->CreateBuffer(BufferDesc, m_Transforms.GetArrayPtr().ToByteArray());

    m_bTransformsUpdated[0] = std::make_shared<bool>(true);
    m_bTransformsUpdated[1] = std::make_shared<bool>(true);
  }
  else
  {
    const wdUInt32 uiRenIdx = wdRenderWorld::GetDataIndexForExtraction();
    *m_bTransformsUpdated[uiRenIdx] = false;
  }
}

void wdSkinningState::FillSkinnedMeshRenderData(wdSkinnedMeshRenderData& ref_renderData) const
{
  ref_renderData.m_hSkinningTransforms = m_hGpuBuffer;

  const wdUInt32 uiExIdx = wdRenderWorld::GetDataIndexForExtraction();

  if (m_bTransformsUpdated[uiExIdx] && *m_bTransformsUpdated[uiExIdx] == false)
  {
    auto pSkinningMatrices = WD_NEW_ARRAY(wdFrameAllocator::GetCurrentAllocator(), wdShaderTransform, m_Transforms.GetCount());
    pSkinningMatrices.CopyFrom(m_Transforms);

    ref_renderData.m_pNewSkinningTransformData = pSkinningMatrices.ToByteArray();
    ref_renderData.m_bTransformsUpdated = m_bTransformsUpdated[uiExIdx];
  }
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_SkinnedMeshComponent);
