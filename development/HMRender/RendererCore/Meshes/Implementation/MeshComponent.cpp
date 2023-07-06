#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
WD_BEGIN_COMPONENT_TYPE(wdMeshComponent, 3, wdComponentMode::Static)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("Mesh", GetMeshFile, SetMeshFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    WD_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new wdExposeColorAlphaAttribute()),
    WD_ARRAY_ACCESSOR_PROPERTY("Materials", Materials_GetCount, Materials_GetValue, Materials_SetValue, Materials_Insert, Materials_Remove)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Material")),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_MESSAGEHANDLERS
  {
    WD_MESSAGE_HANDLER(wdMsgExtractGeometry, OnMsgExtractGeometry)
  }
  WD_END_MESSAGEHANDLERS;
}
WD_END_COMPONENT_TYPE
// clang-format on

wdMeshComponent::wdMeshComponent() = default;
wdMeshComponent::~wdMeshComponent() = default;

void wdMeshComponent::OnMsgExtractGeometry(wdMsgExtractGeometry& ref_msg) const
{
  if (ref_msg.m_Mode != wdWorldGeoExtractionUtil::ExtractionMode::RenderMesh)
    return;

  // ignore invalid and created resources
  {
    wdMeshResourceHandle hRenderMesh = GetMesh();
    if (!hRenderMesh.IsValid())
      return;

    wdResourceLock<wdMeshResource> pRenderMesh(hRenderMesh, wdResourceAcquireMode::PointerOnly);
    if (pRenderMesh->GetBaseResourceFlags().IsAnySet(wdResourceFlags::IsCreatedResource))
      return;
  }

  ref_msg.AddMeshObject(GetOwner()->GetGlobalTransform(), wdResourceManager::LoadResource<wdCpuMeshResource>(GetMeshFile()));
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshComponent);
