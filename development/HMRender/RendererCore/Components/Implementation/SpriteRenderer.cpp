#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Types/ScopeExit.h>
#include <RendererCore/Components/SpriteComponent.h>
#include <RendererCore/Components/SpriteRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

struct alignas(16) SpriteData
{
  wdVec3 m_worldSpacePosition;
  float m_size;
  float m_maxScreenSize;
  float m_aspectRatio;
  wdUInt32 m_colorRG;
  wdUInt32 m_colorBA;
  wdUInt32 m_texCoordScale;
  wdUInt32 m_texCoordOffset;
  wdUInt32 m_gameObjectID;
  wdUInt32 m_reserved;
};

WD_CHECK_AT_COMPILETIME(sizeof(SpriteData) == 48);

namespace
{
  enum
  {
    MAX_SPRITE_DATA_PER_BATCH = 1024
  };
}

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdSpriteRenderer, 1, wdRTTIDefaultAllocator<wdSpriteRenderer>)
WD_END_DYNAMIC_REFLECTED_TYPE;

wdSpriteRenderer::wdSpriteRenderer()
{
  m_hShader = wdResourceManager::LoadResource<wdShaderResource>("Shaders/Materials/SpriteMaterial.wdShader");
}

wdSpriteRenderer::~wdSpriteRenderer() = default;

void wdSpriteRenderer::GetSupportedRenderDataTypes(wdHybridArray<const wdRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(wdGetStaticRTTI<wdSpriteRenderData>());
}

void wdSpriteRenderer::GetSupportedRenderDataCategories(wdHybridArray<wdRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(wdDefaultRenderDataCategories::LitMasked);
  ref_categories.PushBack(wdDefaultRenderDataCategories::LitTransparent);
  ref_categories.PushBack(wdDefaultRenderDataCategories::SimpleOpaque);
  ref_categories.PushBack(wdDefaultRenderDataCategories::SimpleTransparent);
  ref_categories.PushBack(wdDefaultRenderDataCategories::Selection);
}

void wdSpriteRenderer::RenderBatch(const wdRenderViewContext& renderViewContext, const wdRenderPipelinePass* pPass, const wdRenderDataBatch& batch) const
{
  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();
  wdRenderContext* pContext = renderViewContext.m_pRenderContext;

  const wdSpriteRenderData* pRenderData = batch.GetFirstData<wdSpriteRenderData>();

  wdGALBufferHandle hSpriteData = CreateSpriteDataBuffer();
  WD_SCOPE_EXIT(DeleteSpriteDataBuffer(hSpriteData));

  pContext->BindShader(m_hShader);
  pContext->BindBuffer("spriteData", pDevice->GetDefaultResourceView(hSpriteData));
  pContext->BindTexture2D("SpriteTexture", pRenderData->m_hTexture);

  pContext->SetShaderPermutationVariable("BLEND_MODE", wdSpriteBlendMode::GetPermutationValue(pRenderData->m_BlendMode));

  wdUInt32 uiStartIndex = 0;
  while (uiStartIndex < batch.GetCount())
  {
    const wdUInt32 uiCount = wdMath::Min(batch.GetCount() - uiStartIndex, (wdUInt32)MAX_SPRITE_DATA_PER_BATCH);

    FillSpriteData(batch, uiStartIndex, uiCount);
    if (m_SpriteData.GetCount() > 0) // Instance data might be empty if all render data was filtered.
    {
      pContext->GetCommandEncoder()->UpdateBuffer(hSpriteData, 0, m_SpriteData.GetByteArrayPtr());

      pContext->BindMeshBuffer(wdGALBufferHandle(), wdGALBufferHandle(), nullptr, wdGALPrimitiveTopology::Triangles, uiCount * 2);
      pContext->DrawMeshBuffer().IgnoreResult();
    }

    uiStartIndex += uiCount;
  }
}

wdGALBufferHandle wdSpriteRenderer::CreateSpriteDataBuffer() const
{
  wdGALBufferCreationDescription desc;
  desc.m_uiStructSize = sizeof(SpriteData);
  desc.m_uiTotalSize = desc.m_uiStructSize * MAX_SPRITE_DATA_PER_BATCH;
  desc.m_BufferType = wdGALBufferType::Generic;
  desc.m_bUseAsStructuredBuffer = true;
  desc.m_bAllowShaderResourceView = true;
  desc.m_ResourceAccess.m_bImmutable = false;

  return wdGPUResourcePool::GetDefaultInstance()->GetBuffer(desc);
}

void wdSpriteRenderer::DeleteSpriteDataBuffer(wdGALBufferHandle hBuffer) const
{
  wdGPUResourcePool::GetDefaultInstance()->ReturnBuffer(hBuffer);
}

void wdSpriteRenderer::FillSpriteData(const wdRenderDataBatch& batch, wdUInt32 uiStartIndex, wdUInt32 uiCount) const
{
  m_SpriteData.Clear();
  m_SpriteData.Reserve(uiCount);

  for (auto it = batch.GetIterator<wdSpriteRenderData>(uiStartIndex, uiCount); it.IsValid(); ++it)
  {
    const wdSpriteRenderData* pRenderData = it;

    auto& spriteData = m_SpriteData.ExpandAndGetRef();

    spriteData.m_worldSpacePosition = pRenderData->m_GlobalTransform.m_vPosition;
    spriteData.m_size = pRenderData->m_fSize;
    spriteData.m_maxScreenSize = pRenderData->m_fMaxScreenSize;
    spriteData.m_aspectRatio = pRenderData->m_fAspectRatio;
    spriteData.m_colorRG = wdShaderUtils::Float2ToRG16F(wdVec2(pRenderData->m_color.r, pRenderData->m_color.g));
    spriteData.m_colorBA = wdShaderUtils::Float2ToRG16F(wdVec2(pRenderData->m_color.b, pRenderData->m_color.a));
    spriteData.m_texCoordScale = wdShaderUtils::Float2ToRG16F(pRenderData->m_texCoordScale);
    spriteData.m_texCoordOffset = wdShaderUtils::Float2ToRG16F(pRenderData->m_texCoordOffset);
    spriteData.m_gameObjectID = pRenderData->m_uiUniqueID;
    spriteData.m_reserved = 0;
  }
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_SpriteRenderer);
