#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Textures/TextureCubeResource.h>

// clang-format off
WD_BEGIN_COMPONENT_TYPE(wdSkyBoxComponent, 4, wdComponentMode::Static)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("CubeMap", GetCubeMapFile, SetCubeMapFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Texture_Cube")),
    WD_ACCESSOR_PROPERTY("ExposureBias", GetExposureBias, SetExposureBias)->AddAttributes(new wdClampValueAttribute(-32.0f, 32.0f)),
    WD_ACCESSOR_PROPERTY("InverseTonemap", GetInverseTonemap, SetInverseTonemap),
    WD_ACCESSOR_PROPERTY("UseFog", GetUseFog, SetUseFog)->AddAttributes(new wdDefaultValueAttribute(true)),
    WD_ACCESSOR_PROPERTY("VirtualDistance", GetVirtualDistance, SetVirtualDistance)->AddAttributes(new wdClampValueAttribute(0.0f, wdVariant()), new wdDefaultValueAttribute(1000.0f)),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Rendering"),
  }
  WD_END_ATTRIBUTES;
  WD_BEGIN_MESSAGEHANDLERS
  {
    WD_MESSAGE_HANDLER(wdMsgExtractRenderData, OnMsgExtractRenderData),
  }
  WD_END_MESSAGEHANDLERS;
}
WD_END_COMPONENT_TYPE;
// clang-format on

wdSkyBoxComponent::wdSkyBoxComponent() = default;
wdSkyBoxComponent::~wdSkyBoxComponent() = default;

void wdSkyBoxComponent::Initialize()
{
  SUPER::Initialize();

  const char* szBufferResourceName = "SkyBoxBuffer";
  wdMeshBufferResourceHandle hMeshBuffer = wdResourceManager::GetExistingResource<wdMeshBufferResource>(szBufferResourceName);
  if (!hMeshBuffer.IsValid())
  {
    wdGeometry geom;
    geom.AddRectXY(wdVec2(2.0f));

    wdMeshBufferResourceDescriptor desc;
    desc.AddStream(wdGALVertexAttributeSemantic::Position, wdGALResourceFormat::XYZFloat);
    desc.AllocateStreamsFromGeometry(geom, wdGALPrimitiveTopology::Triangles);

    hMeshBuffer = wdResourceManager::GetOrCreateResource<wdMeshBufferResource>(szBufferResourceName, std::move(desc), szBufferResourceName);
  }

  const char* szMeshResourceName = "SkyBoxMesh";
  m_hMesh = wdResourceManager::GetExistingResource<wdMeshResource>(szMeshResourceName);
  if (!m_hMesh.IsValid())
  {
    wdMeshResourceDescriptor desc;
    desc.UseExistingMeshBuffer(hMeshBuffer);
    desc.AddSubMesh(2, 0, 0);
    desc.ComputeBounds();

    m_hMesh = wdResourceManager::GetOrCreateResource<wdMeshResource>(szMeshResourceName, std::move(desc), szMeshResourceName);
  }

  wdStringBuilder cubeMapMaterialName = "SkyBoxMaterial_CubeMap";
  cubeMapMaterialName.AppendFormat("_{0}", wdArgP(GetWorld())); // make the resource unique for each world

  m_hCubeMapMaterial = wdResourceManager::GetExistingResource<wdMaterialResource>(cubeMapMaterialName);
  if (!m_hCubeMapMaterial.IsValid())
  {
    wdMaterialResourceDescriptor desc;
    desc.m_hBaseMaterial = wdResourceManager::LoadResource<wdMaterialResource>("{ b4b75b1c-c2c8-4a0e-8076-780bdd46d18b }"); // Sky.wdMaterialAsset

    m_hCubeMapMaterial = wdResourceManager::CreateResource<wdMaterialResource>(cubeMapMaterialName, std::move(desc), cubeMapMaterialName);
  }

  UpdateMaterials();
}

wdResult wdSkyBoxComponent::GetLocalBounds(wdBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, wdMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return WD_SUCCESS;
}

void wdSkyBoxComponent::OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const
{
  // Don't extract sky render data for selection or in orthographic views.
  if (msg.m_OverrideCategory != wdInvalidRenderDataCategory || msg.m_pView->GetCamera()->IsOrthographic())
    return;

  wdMeshRenderData* pRenderData = wdCreateRenderDataForThisFrame<wdMeshRenderData>(GetOwner());
  {
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalTransform.m_vPosition.SetZero(); // skybox should always be at the origin
    pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
    pRenderData->m_hMesh = m_hMesh;
    pRenderData->m_hMaterial = m_hCubeMapMaterial;
    pRenderData->m_uiSubMeshIndex = 0;
    pRenderData->m_uiUniqueID = GetUniqueIdForRendering();

    pRenderData->FillBatchIdAndSortingKey();
  }

  msg.AddRenderData(pRenderData, wdDefaultRenderDataCategories::Sky, wdRenderData::Caching::Never);
}

void wdSkyBoxComponent::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  wdStreamWriter& s = inout_stream.GetStream();

  s << m_fExposureBias;
  s << m_bInverseTonemap;
  s << m_bUseFog;
  s << m_fVirtualDistance;
  s << m_hCubeMap;
}

void wdSkyBoxComponent::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const wdUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  wdStreamReader& s = inout_stream.GetStream();

  s >> m_fExposureBias;
  s >> m_bInverseTonemap;

  if (uiVersion >= 4)
  {
    s >> m_bUseFog;
    s >> m_fVirtualDistance;
  }

  if (uiVersion >= 3)
  {
    s >> m_hCubeMap;
  }
  else
  {
    wdTexture2DResourceHandle dummyHandle;
    for (int i = 0; i < 6; i++)
    {
      s >> dummyHandle;
    }
  }
}

void wdSkyBoxComponent::SetExposureBias(float fExposureBias)
{
  m_fExposureBias = fExposureBias;

  UpdateMaterials();
}

void wdSkyBoxComponent::SetInverseTonemap(bool bInverseTonemap)
{
  m_bInverseTonemap = bInverseTonemap;

  UpdateMaterials();
}

void wdSkyBoxComponent::SetUseFog(bool bUseFog)
{
  m_bUseFog = bUseFog;

  UpdateMaterials();
}

void wdSkyBoxComponent::SetVirtualDistance(float fVirtualDistance)
{
  m_fVirtualDistance = fVirtualDistance;

  UpdateMaterials();
}

void wdSkyBoxComponent::SetCubeMapFile(const char* szFile)
{
  wdTextureCubeResourceHandle hCubeMap;
  if (!wdStringUtils::IsNullOrEmpty(szFile))
  {
    hCubeMap = wdResourceManager::LoadResource<wdTextureCubeResource>(szFile);
  }

  SetCubeMap(hCubeMap);
}

const char* wdSkyBoxComponent::GetCubeMapFile() const
{
  return m_hCubeMap.IsValid() ? m_hCubeMap.GetResourceID().GetData() : "";
}

void wdSkyBoxComponent::SetCubeMap(const wdTextureCubeResourceHandle& hCubeMap)
{
  m_hCubeMap = hCubeMap;
  UpdateMaterials();
}

const wdTextureCubeResourceHandle& wdSkyBoxComponent::GetCubeMap() const
{
  return m_hCubeMap;
}

void wdSkyBoxComponent::OnActivated()
{
  SUPER::OnActivated();

  UpdateMaterials();
}

void wdSkyBoxComponent::UpdateMaterials()
{
  if (m_hCubeMapMaterial.IsValid())
  {
    wdResourceLock<wdMaterialResource> pMaterial(m_hCubeMapMaterial, wdResourceAcquireMode::AllowLoadingFallback);

    pMaterial->SetParameter("ExposureBias", m_fExposureBias);
    pMaterial->SetParameter("InverseTonemap", m_bInverseTonemap);
    pMaterial->SetParameter("UseFog", m_bUseFog);
    pMaterial->SetParameter("VirtualDistance", m_fVirtualDistance);
    pMaterial->SetTextureCubeBinding("CubeMap", m_hCubeMap);

    pMaterial->PreserveCurrentDesc();
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class wdSkyBoxComponentPatch_1_2 : public wdGraphPatch
{
public:
  wdSkyBoxComponentPatch_1_2()
    : wdGraphPatch("wdSkyBoxComponent", 2)
  {
  }

  virtual void Patch(wdGraphPatchContext& ref_context, wdAbstractObjectGraph* pGraph, wdAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Exposure Bias", "ExposureBias");
    pNode->RenameProperty("Inverse Tonemap", "InverseTonemap");
    pNode->RenameProperty("Left Texture", "LeftTexture");
    pNode->RenameProperty("Front Texture", "FrontTexture");
    pNode->RenameProperty("Right Texture", "RightTexture");
    pNode->RenameProperty("Back Texture", "BackTexture");
    pNode->RenameProperty("Up Texture", "UpTexture");
    pNode->RenameProperty("Down Texture", "DownTexture");
  }
};

wdSkyBoxComponentPatch_1_2 g_wdSkyBoxComponentPatch_1_2;



WD_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_SkyBoxComponent);
