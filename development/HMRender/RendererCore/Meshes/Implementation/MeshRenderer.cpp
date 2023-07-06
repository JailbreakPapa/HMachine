#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/Implementation/MeshRendererUtils.h>
#include <RendererCore/Meshes/InstancedMeshComponent.h>
#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/Pipeline/InstanceDataProvider.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdMeshRenderer, 1, wdRTTIDefaultAllocator<wdMeshRenderer>)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdMeshRenderer::wdMeshRenderer() = default;
wdMeshRenderer::~wdMeshRenderer() = default;

void wdMeshRenderer::GetSupportedRenderDataTypes(wdHybridArray<const wdRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(wdGetStaticRTTI<wdMeshRenderData>());
  ref_types.PushBack(wdGetStaticRTTI<wdInstancedMeshRenderData>());
}

void wdMeshRenderer::GetSupportedRenderDataCategories(wdHybridArray<wdRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(wdDefaultRenderDataCategories::Sky);
  ref_categories.PushBack(wdDefaultRenderDataCategories::LitOpaque);
  ref_categories.PushBack(wdDefaultRenderDataCategories::LitMasked);
  ref_categories.PushBack(wdDefaultRenderDataCategories::LitTransparent);
  ref_categories.PushBack(wdDefaultRenderDataCategories::LitForeground);
  ref_categories.PushBack(wdDefaultRenderDataCategories::SimpleOpaque);
  ref_categories.PushBack(wdDefaultRenderDataCategories::SimpleTransparent);
  ref_categories.PushBack(wdDefaultRenderDataCategories::SimpleForeground);
  ref_categories.PushBack(wdDefaultRenderDataCategories::Selection);
  ref_categories.PushBack(wdDefaultRenderDataCategories::GUI);
}

void wdMeshRenderer::RenderBatch(const wdRenderViewContext& renderViewContext, const wdRenderPipelinePass* pPass, const wdRenderDataBatch& batch) const
{
  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();
  wdRenderContext* pContext = renderViewContext.m_pRenderContext;

  const wdMeshRenderData* pRenderData = batch.GetFirstData<wdMeshRenderData>();

  const wdMeshResourceHandle& hMesh = pRenderData->m_hMesh;
  const wdMaterialResourceHandle& hMaterial = pRenderData->m_hMaterial;
  const wdUInt32 uiPartIndex = pRenderData->m_uiSubMeshIndex;
  const bool bHasExplicitInstanceData = pRenderData->IsInstanceOf<wdInstancedMeshRenderData>();

  wdResourceLock<wdMeshResource> pMesh(hMesh, wdResourceAcquireMode::AllowLoadingFallback);

  // This can happen when the resource has been reloaded and now has fewer submeshes.
  const auto& subMeshes = pMesh->GetSubMeshes();
  if (subMeshes.GetCount() <= uiPartIndex)
  {
    return;
  }

  wdInstanceData* pInstanceData = bHasExplicitInstanceData ? static_cast<const wdInstancedMeshRenderData*>(pRenderData)->m_pExplicitInstanceData : pPass->GetPipeline()->GetFrameDataProvider<wdInstanceDataProvider>()->GetData(renderViewContext);

  pInstanceData->BindResources(pContext);

  if (pRenderData->m_uiFlipWinding)
  {
    pContext->SetShaderPermutationVariable("FLIP_WINDING", "TRUE");
  }
  else
  {
    pContext->SetShaderPermutationVariable("FLIP_WINDING", "FALSE");
  }

  pContext->BindMaterial(hMaterial);
  pContext->BindMeshBuffer(pMesh->GetMeshBuffer());

  SetAdditionalData(renderViewContext, pRenderData);

  if (!bHasExplicitInstanceData)
  {
    wdUInt32 uiStartIndex = 0;
    while (uiStartIndex < batch.GetCount())
    {
      const wdUInt32 uiRemainingInstances = batch.GetCount() - uiStartIndex;

      wdUInt32 uiInstanceDataOffset = 0;
      wdArrayPtr<wdPerInstanceData> instanceData = pInstanceData->GetInstanceData(uiRemainingInstances, uiInstanceDataOffset);

      wdUInt32 uiFilteredCount = 0;
      FillPerInstanceData(instanceData, batch, uiStartIndex, uiFilteredCount);

      if (uiFilteredCount > 0) // Instance data might be empty if all render data was filtered.
      {
        pInstanceData->UpdateInstanceData(pContext, uiFilteredCount);

        const wdMeshResourceDescriptor::SubMesh& meshPart = subMeshes[uiPartIndex];

        if (pContext->DrawMeshBuffer(meshPart.m_uiPrimitiveCount, meshPart.m_uiFirstPrimitive, uiFilteredCount).Failed())
        {
          for (auto it = batch.GetIterator<wdMeshRenderData>(uiStartIndex, instanceData.GetCount()); it.IsValid(); ++it)
          {
            pRenderData = it;

            // draw bounding box instead
            if (pRenderData->m_GlobalBounds.IsValid())
            {
              wdDebugRenderer::DrawLineBox(*renderViewContext.m_pViewDebugContext, pRenderData->m_GlobalBounds.GetBox(), wdColor::Magenta);
            }
          }
        }
      }

      uiStartIndex += instanceData.GetCount();
    }
  }
  else
  {
    wdUInt32 uiInstanceCount = static_cast<const wdInstancedMeshRenderData*>(pRenderData)->m_uiExplicitInstanceCount;

    const wdMeshResourceDescriptor::SubMesh& meshPart = subMeshes[uiPartIndex];

    pContext->DrawMeshBuffer(meshPart.m_uiPrimitiveCount, meshPart.m_uiFirstPrimitive, uiInstanceCount).IgnoreResult();
  }
}

void wdMeshRenderer::SetAdditionalData(const wdRenderViewContext& renderViewContext, const wdMeshRenderData* pRenderData) const
{
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");
}

void wdMeshRenderer::FillPerInstanceData(wdArrayPtr<wdPerInstanceData> instanceData, const wdRenderDataBatch& batch, wdUInt32 uiStartIndex, wdUInt32& out_uiFilteredCount) const
{
  wdUInt32 uiCount = wdMath::Min<wdUInt32>(instanceData.GetCount(), batch.GetCount() - uiStartIndex);
  wdUInt32 uiCurrentIndex = 0;

  for (auto it = batch.GetIterator<wdMeshRenderData>(uiStartIndex, uiCount); it.IsValid(); ++it)
  {
    wdInternal::FillPerInstanceData(instanceData[uiCurrentIndex], it);

    ++uiCurrentIndex;
  }

  out_uiFilteredCount = uiCurrentIndex;
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshRenderer);
