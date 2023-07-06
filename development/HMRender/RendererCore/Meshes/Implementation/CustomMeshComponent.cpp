#include <RendererCore/RendererCorePCH.h>

#include <../../Data/Base/Shaders/Common/ObjectConstants.h>
#include <Core/Graphics/Geometry.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Meshes/CustomMeshComponent.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/Pipeline/InstanceDataProvider.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
WD_BEGIN_COMPONENT_TYPE(wdCustomMeshComponent, 1, wdComponentMode::Static)
{
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Rendering"),
  }
  WD_END_ATTRIBUTES;
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new wdExposeColorAlphaAttribute()),
    WD_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Material")),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_MESSAGEHANDLERS
  {
    WD_MESSAGE_HANDLER(wdMsgExtractRenderData, OnMsgExtractRenderData),
    WD_MESSAGE_HANDLER(wdMsgSetMeshMaterial, OnMsgSetMeshMaterial),
    WD_MESSAGE_HANDLER(wdMsgSetColor, OnMsgSetColor),
  } WD_END_MESSAGEHANDLERS;
}
WD_END_COMPONENT_TYPE
// clang-format on

wdAtomicInteger32 s_iCustomMeshResources;

wdCustomMeshComponent::wdCustomMeshComponent()
{
  m_Bounds.SetInvalid();
}

wdCustomMeshComponent::~wdCustomMeshComponent() = default;

void wdCustomMeshComponent::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  wdStreamWriter& s = inout_stream.GetStream();

  s << m_Color;
  s << m_hMaterial;

  wdUInt32 uiCategory = m_RenderDataCategory.m_uiValue;
  s << uiCategory;
}

void wdCustomMeshComponent::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const wdUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  wdStreamReader& s = inout_stream.GetStream();

  s >> m_Color;
  s >> m_hMaterial;

  wdUInt32 uiCategory = 0;
  s >> uiCategory;
  m_RenderDataCategory.m_uiValue = static_cast<wdUInt16>(uiCategory);
}

wdResult wdCustomMeshComponent::GetLocalBounds(wdBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, wdMsgUpdateLocalBounds& ref_msg)
{
  if (m_Bounds.IsValid())
  {
    ref_bounds = m_Bounds;
    return WD_SUCCESS;
  }

  return WD_FAILURE;
}

wdDynamicMeshBufferResourceHandle wdCustomMeshComponent::CreateMeshResource(wdGALPrimitiveTopology::Enum topology, wdUInt32 uiMaxVertices, wdUInt32 uiMaxPrimitives, wdGALIndexType::Enum indexType)
{
  wdDynamicMeshBufferResourceDescriptor desc;
  desc.m_Topology = topology;
  desc.m_uiMaxVertices = uiMaxVertices;
  desc.m_uiMaxPrimitives = uiMaxPrimitives;
  desc.m_IndexType = indexType;
  desc.m_bColorStream = true;

  wdStringBuilder sGuid;
  sGuid.Format("CustomMesh_{}", s_iCustomMeshResources.Increment());

  m_hDynamicMesh = wdResourceManager::CreateResource<wdDynamicMeshBufferResource>(sGuid, std::move(desc));

  InvalidateCachedRenderData();

  return m_hDynamicMesh;
}

void wdCustomMeshComponent::SetMeshResource(const wdDynamicMeshBufferResourceHandle& hMesh)
{
  m_hDynamicMesh = hMesh;
  InvalidateCachedRenderData();
}

void wdCustomMeshComponent::SetBounds(const wdBoundingBoxSphere& bounds)
{
  m_Bounds = bounds;
  TriggerLocalBoundsUpdate();
}

void wdCustomMeshComponent::SetMaterial(const wdMaterialResourceHandle& hMaterial)
{
  m_hMaterial = hMaterial;
  InvalidateCachedRenderData();
}

wdMaterialResourceHandle wdCustomMeshComponent::GetMaterial() const
{
  return m_hMaterial;
}

void wdCustomMeshComponent::SetMaterialFile(const char* szMaterial)
{
  wdMaterialResourceHandle hResource;

  if (!wdStringUtils::IsNullOrEmpty(szMaterial))
  {
    hResource = wdResourceManager::LoadResource<wdMaterialResource>(szMaterial);
  }

  m_hMaterial = hResource;
}

const char* wdCustomMeshComponent::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}

void wdCustomMeshComponent::SetColor(const wdColor& color)
{
  m_Color = color;

  InvalidateCachedRenderData();
}

const wdColor& wdCustomMeshComponent::GetColor() const
{
  return m_Color;
}

void wdCustomMeshComponent::OnMsgSetMeshMaterial(wdMsgSetMeshMaterial& ref_msg)
{
  SetMaterial(ref_msg.m_hMaterial);
}

void wdCustomMeshComponent::OnMsgSetColor(wdMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);

  InvalidateCachedRenderData();
}

void wdCustomMeshComponent::SetUsePrimitiveRange(wdUInt32 uiFirstPrimitive /*= 0*/, wdUInt32 uiNumPrimitives /*= wdMath::MaxValue<wdUInt32>()*/)
{
  m_uiFirstPrimitive = uiFirstPrimitive;
  m_uiNumPrimitives = uiNumPrimitives;
}

void wdCustomMeshComponent::OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const
{
  if (!m_hDynamicMesh.IsValid() || !m_hMaterial.IsValid())
    return;

  wdResourceLock<wdDynamicMeshBufferResource> pMesh(m_hDynamicMesh, wdResourceAcquireMode::BlockTillLoaded);

  wdCustomMeshRenderData* pRenderData = wdCreateRenderDataForThisFrame<wdCustomMeshRenderData>(GetOwner());
  {
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
    pRenderData->m_hMesh = m_hDynamicMesh;
    pRenderData->m_hMaterial = m_hMaterial;
    pRenderData->m_Color = m_Color;
    pRenderData->m_uiUniqueID = GetUniqueIdForRendering();
    pRenderData->m_uiFirstPrimitive = wdMath::Min(m_uiFirstPrimitive, pMesh->GetDescriptor().m_uiMaxPrimitives);
    pRenderData->m_uiNumPrimitives = wdMath::Min(m_uiNumPrimitives, pMesh->GetDescriptor().m_uiMaxPrimitives - pRenderData->m_uiFirstPrimitive);

    pRenderData->FillBatchIdAndSortingKey();
  }

  bool bDontCacheYet = false;

  // Determine render data category.
  wdRenderData::Category category = m_RenderDataCategory;
  if (category == wdInvalidRenderDataCategory)
  {
    wdResourceLock<wdMaterialResource> pMaterial(m_hMaterial, wdResourceAcquireMode::AllowLoadingFallback);

    if (pMaterial.GetAcquireResult() == wdResourceAcquireResult::LoadingFallback)
      bDontCacheYet = true;

    wdTempHashedString blendModeValue = pMaterial->GetPermutationValue("BLEND_MODE");
    if (blendModeValue == "BLEND_MODE_OPAQUE" || blendModeValue == "")
    {
      category = wdDefaultRenderDataCategories::LitOpaque;
    }
    else if (blendModeValue == "BLEND_MODE_MASKED")
    {
      category = wdDefaultRenderDataCategories::LitMasked;
    }
    else
    {
      category = wdDefaultRenderDataCategories::LitTransparent;
    }
  }

  msg.AddRenderData(pRenderData, category, bDontCacheYet ? wdRenderData::Caching::Never : wdRenderData::Caching::IfStatic);
}

void wdCustomMeshComponent::OnActivated()
{
  if (false)
  {
    wdGeometry geo;
    geo.AddTorus(1.0f, 1.5f, 32, 16, false);
    geo.TriangulatePolygons();
    geo.ComputeTangents();

    auto hMesh = CreateMeshResource(wdGALPrimitiveTopology::Triangles, geo.GetVertices().GetCount(), geo.GetPolygons().GetCount(), wdGALIndexType::UInt);

    wdResourceLock<wdDynamicMeshBufferResource> pMesh(hMesh, wdResourceAcquireMode::BlockTillLoaded);

    auto verts = pMesh->AccessVertexData();
    auto cols = pMesh->AccessColorData();

    for (wdUInt32 v = 0; v < verts.GetCount(); ++v)
    {
      verts[v].m_vPosition = geo.GetVertices()[v].m_vPosition;
      verts[v].m_vTexCoord.SetZero();
      verts[v].EncodeNormal(geo.GetVertices()[v].m_vNormal);
      verts[v].EncodeTangent(geo.GetVertices()[v].m_vTangent, 1.0f);

      cols[v] = wdColor::CornflowerBlue;
    }

    auto ind = pMesh->AccessIndex32Data();

    for (wdUInt32 i = 0; i < geo.GetPolygons().GetCount(); ++i)
    {
      ind[i * 3 + 0] = geo.GetPolygons()[i].m_Vertices[0];
      ind[i * 3 + 1] = geo.GetPolygons()[i].m_Vertices[1];
      ind[i * 3 + 2] = geo.GetPolygons()[i].m_Vertices[2];
    }

    SetBounds(wdBoundingSphere(wdVec3::ZeroVector(), 1.5f));
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdCustomMeshRenderData, 1, wdRTTIDefaultAllocator<wdCustomMeshRenderData>)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


void wdCustomMeshRenderData::FillBatchIdAndSortingKey()
{
  const wdUInt32 uiAdditionalBatchData = 0;

  m_uiFlipWinding = m_GlobalTransform.ContainsNegativeScale() ? 1 : 0;
  m_uiUniformScale = m_GlobalTransform.ContainsUniformScale() ? 1 : 0;

  const wdUInt32 uiMeshIDHash = wdHashingUtils::StringHashTo32(m_hMesh.GetResourceIDHash());
  const wdUInt32 uiMaterialIDHash = m_hMaterial.IsValid() ? wdHashingUtils::StringHashTo32(m_hMaterial.GetResourceIDHash()) : 0;

  // Generate batch id from mesh, material and part index.
  wdUInt32 data[] = {uiMeshIDHash, uiMaterialIDHash, 0 /*m_uiSubMeshIndex*/, m_uiFlipWinding, uiAdditionalBatchData};
  m_uiBatchId = wdHashingUtils::xxHash32(data, sizeof(data));

  // Sort by material and then by mesh
  m_uiSortingKey = (uiMaterialIDHash << 16) | ((uiMeshIDHash + 0 /*m_uiSubMeshIndex*/) & 0xFFFE) | m_uiFlipWinding;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdCustomMeshRenderer, 1, wdRTTIDefaultAllocator<wdCustomMeshRenderer>)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdCustomMeshRenderer::wdCustomMeshRenderer() = default;
wdCustomMeshRenderer::~wdCustomMeshRenderer() = default;

void wdCustomMeshRenderer::GetSupportedRenderDataCategories(wdHybridArray<wdRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(wdDefaultRenderDataCategories::LitOpaque);
  ref_categories.PushBack(wdDefaultRenderDataCategories::LitMasked);
  ref_categories.PushBack(wdDefaultRenderDataCategories::LitTransparent);
  ref_categories.PushBack(wdDefaultRenderDataCategories::Selection);
}

void wdCustomMeshRenderer::GetSupportedRenderDataTypes(wdHybridArray<const wdRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(wdGetStaticRTTI<wdCustomMeshRenderData>());
}

void wdCustomMeshRenderer::RenderBatch(const wdRenderViewContext& renderViewContext, const wdRenderPipelinePass* pPass, const wdRenderDataBatch& batch) const
{
  wdRenderContext* pRenderContext = renderViewContext.m_pRenderContext;
  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();
  wdGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();

  wdInstanceData* pInstanceData = pPass->GetPipeline()->GetFrameDataProvider<wdInstanceDataProvider>()->GetData(renderViewContext);
  pInstanceData->BindResources(pRenderContext);

  const wdCustomMeshRenderData* pRenderData1st = batch.GetFirstData<wdCustomMeshRenderData>();

  if (pRenderData1st->m_uiFlipWinding)
  {
    pRenderContext->SetShaderPermutationVariable("FLIP_WINDING", "TRUE");
  }
  else
  {
    pRenderContext->SetShaderPermutationVariable("FLIP_WINDING", "FALSE");
  }

  pRenderContext->SetShaderPermutationVariable("VERTEX_SKINNING", "FALSE");

  for (auto it = batch.GetIterator<wdCustomMeshRenderData>(0, batch.GetCount()); it.IsValid(); ++it)
  {
    const wdCustomMeshRenderData* pRenderData = it;

    wdResourceLock<wdDynamicMeshBufferResource> pBuffer(pRenderData->m_hMesh, wdResourceAcquireMode::BlockTillLoaded);

    pRenderContext->BindMaterial(pRenderData->m_hMaterial);

    wdUInt32 uiInstanceDataOffset = 0;
    wdArrayPtr<wdPerInstanceData> instanceData = pInstanceData->GetInstanceData(1, uiInstanceDataOffset);

    instanceData[0].GameObjectID = pRenderData->m_uiUniqueID;
    instanceData[0].Color = pRenderData->m_Color;
    instanceData[0].ObjectToWorld = pRenderData->m_GlobalTransform;

    if (pRenderData->m_uiUniformScale)
    {
      instanceData[0].ObjectToWorldNormal = instanceData[0].ObjectToWorld;
    }
    else
    {
      wdMat4 objectToWorld = pRenderData->m_GlobalTransform.GetAsMat4();

      wdMat3 mInverse = objectToWorld.GetRotationalPart();
      mInverse.Invert(0.0f).IgnoreResult();
      // we explicitly ignore the return value here (success / failure)
      // because when we have a scale of 0 (which happens temporarily during editing) that would be annoying
      instanceData[0].ObjectToWorldNormal = mInverse.GetTranspose();
    }

    pInstanceData->UpdateInstanceData(pRenderContext, 1);

    const auto& desc = pBuffer->GetDescriptor();
    pBuffer->UpdateGpuBuffer(pGALCommandEncoder);

    // redo this after the primitive count has changed
    pRenderContext->BindMeshBuffer(pRenderData->m_hMesh);

    renderViewContext.m_pRenderContext->DrawMeshBuffer(pRenderData->m_uiNumPrimitives, pRenderData->m_uiFirstPrimitive).IgnoreResult();
  }
}


WD_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_CustomMeshComponent);
