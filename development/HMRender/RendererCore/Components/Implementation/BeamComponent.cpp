#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Messages/CollisionMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <RendererCore/Components/BeamComponent.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererFoundation/Device/Device.h>


// clang-format off
WD_BEGIN_COMPONENT_TYPE(wdBeamComponent, 1, wdComponentMode::Static)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("TargetObject", DummyGetter, SetTargetObject)->AddAttributes(new wdGameObjectReferenceAttribute()),
    WD_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Material")),
    WD_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new wdDefaultValueAttribute(wdColor::White)),
    WD_ACCESSOR_PROPERTY("Width", GetWidth, SetWidth)->AddAttributes(new wdDefaultValueAttribute(0.1f), new wdClampValueAttribute(0.001f, wdVariant()), new wdSuffixAttribute(" m")),
    WD_ACCESSOR_PROPERTY("UVUnitsPerWorldUnit", GetUVUnitsPerWorldUnit, SetUVUnitsPerWorldUnit)->AddAttributes(new wdDefaultValueAttribute(1.0f), new wdClampValueAttribute(0.01f, wdVariant())),
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
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdBeamComponent::wdBeamComponent() = default;

wdBeamComponent::~wdBeamComponent() = default;

void wdBeamComponent::Update()
{
  wdGameObject* pTargetObject = nullptr;
  if (GetWorld()->TryGetObject(m_hTargetObject, pTargetObject))
  {
    wdVec3 currentOwnerPosition = GetOwner()->GetGlobalPosition();
    wdVec3 currentTargetPosition = pTargetObject->GetGlobalPosition();

    if (!pTargetObject->IsActive())
    {
      currentTargetPosition = currentOwnerPosition;
    }

    bool updateMesh = false;

    if ((currentOwnerPosition - m_vLastOwnerPosition).GetLengthSquared() > m_fDistanceUpdateEpsilon)
    {
      updateMesh = true;
      m_vLastOwnerPosition = currentOwnerPosition;
    }

    if ((currentTargetPosition - m_vLastTargetPosition).GetLengthSquared() > m_fDistanceUpdateEpsilon)
    {
      updateMesh = true;
      m_vLastTargetPosition = currentTargetPosition;
    }

    if (updateMesh)
    {
      ReinitMeshes();
    }
  }
  else
  {
    m_hMesh.Invalidate();
  }
}

void wdBeamComponent::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  inout_stream.WriteGameObjectHandle(m_hTargetObject);

  s << m_fWidth;
  s << m_fUVUnitsPerWorldUnit;
  s << m_hMaterial;
  s << m_Color;
}

void wdBeamComponent::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  m_hTargetObject = inout_stream.ReadGameObjectHandle();

  s >> m_fWidth;
  s >> m_fUVUnitsPerWorldUnit;
  s >> m_hMaterial;
  s >> m_Color;
}

wdResult wdBeamComponent::GetLocalBounds(wdBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, wdMsgUpdateLocalBounds& ref_msg)
{
  wdGameObject* pTargetObject = nullptr;
  if (GetWorld()->TryGetObject(m_hTargetObject, pTargetObject))
  {
    const wdVec3 currentTargetPosition = pTargetObject->GetGlobalPosition();
    const wdVec3 targetPositionInOwnerSpace = GetOwner()->GetGlobalTransform().GetInverse().TransformPosition(currentTargetPosition);

    wdVec3 pts[] = {wdVec3::ZeroVector(), targetPositionInOwnerSpace};

    wdBoundingBox box;
    box.SetFromPoints(pts, 2);
    const float fHalfWidth = m_fWidth * 0.5f;
    box.m_vMin -= wdVec3(0, fHalfWidth, fHalfWidth);
    box.m_vMax += wdVec3(0, fHalfWidth, fHalfWidth);
    ref_bounds = box;

    return WD_SUCCESS;
  }

  return WD_FAILURE;
}


void wdBeamComponent::OnActivated()
{
  SUPER::OnActivated();

  ReinitMeshes();
}

void wdBeamComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  Cleanup();
}

void wdBeamComponent::OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid() || !m_hMaterial.IsValid())
    return;

  const wdUInt32 uiFlipWinding = GetOwner()->GetGlobalTransformSimd().ContainsNegativeScale() ? 1 : 0;
  const wdUInt32 uiUniformScale = GetOwner()->GetGlobalTransformSimd().ContainsUniformScale() ? 1 : 0;

  wdMeshRenderData* pRenderData = wdCreateRenderDataForThisFrame<wdMeshRenderData>(GetOwner());
  {
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
    pRenderData->m_hMesh = m_hMesh;
    pRenderData->m_hMaterial = m_hMaterial;
    pRenderData->m_Color = m_Color;
    pRenderData->m_uiSubMeshIndex = 0;
    pRenderData->m_uiUniqueID = GetUniqueIdForRendering();

    pRenderData->FillBatchIdAndSortingKey();
  }

  // Determine render data category.
  wdRenderData::Category category = wdDefaultRenderDataCategories::LitTransparent;
  wdResourceLock<wdMaterialResource> pMaterial(m_hMaterial, wdResourceAcquireMode::AllowLoadingFallback);
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

  msg.AddRenderData(pRenderData, category, wdRenderData::Caching::Never);
}

void wdBeamComponent::SetTargetObject(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hTargetObject = resolver(szReference, GetHandle(), "TargetObject");

  ReinitMeshes();
}

void wdBeamComponent::SetWidth(float fWidth)
{
  if (fWidth <= 0.0f)
    return;

  m_fWidth = fWidth;

  ReinitMeshes();
}

float wdBeamComponent::GetWidth() const
{
  return m_fWidth;
}

void wdBeamComponent::SetUVUnitsPerWorldUnit(float fUVUnitsPerWorldUnit)
{
  if (fUVUnitsPerWorldUnit <= 0.0f)
    return;

  m_fUVUnitsPerWorldUnit = fUVUnitsPerWorldUnit;

  ReinitMeshes();
}

float wdBeamComponent::GetUVUnitsPerWorldUnit() const
{
  return m_fUVUnitsPerWorldUnit;
}

void wdBeamComponent::SetMaterialFile(const char* szFile)
{
  if (!wdStringUtils::IsNullOrEmpty(szFile))
  {
    m_hMaterial = wdResourceManager::LoadResource<wdMaterialResource>(szFile);
  }
  else
  {
    m_hMaterial.Invalidate();
  }
}

const char* wdBeamComponent::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}

wdMaterialResourceHandle wdBeamComponent::GetMaterial() const
{
  return m_hMaterial;
}

void wdBeamComponent::CreateMeshes()
{
  wdVec3 targetPositionInOwnerSpace = GetOwner()->GetGlobalTransform().GetInverse().TransformPosition(m_vLastTargetPosition);

  if (targetPositionInOwnerSpace.IsZero(0.01f))
    return;

  // Create the beam mesh name, it expresses the beam in local space with it's width
  // this way multiple beams in a corridor can share the same mesh for example.
  wdStringBuilder meshName;
  meshName.Format("wdBeamComponent_{0}_{1}_{2}_{3}.createdAtRuntime.wdMesh", m_fWidth, wdArgF(targetPositionInOwnerSpace.x, 2), wdArgF(targetPositionInOwnerSpace.y, 2), wdArgF(targetPositionInOwnerSpace.z, 2));

  m_hMesh = wdResourceManager::GetExistingResource<wdMeshResource>(meshName);

  // We build a cross mesh, thus we need the following vectors, x is the origin and we need to construct
  // the star points.
  //
  //  3        1
  //
  //      x
  //
  //  4        2
  wdVec3 crossVector1 = (0.5f * wdVec3::UnitYAxis() + 0.5f * wdVec3::UnitZAxis());
  crossVector1.SetLength(m_fWidth * 0.5f).IgnoreResult();

  wdVec3 crossVector2 = (0.5f * wdVec3::UnitYAxis() - 0.5f * wdVec3::UnitZAxis());
  crossVector2.SetLength(m_fWidth * 0.5f).IgnoreResult();

  wdVec3 crossVector3 = (-0.5f * wdVec3::UnitYAxis() + 0.5f * wdVec3::UnitZAxis());
  crossVector3.SetLength(m_fWidth * 0.5f).IgnoreResult();

  wdVec3 crossVector4 = (-0.5f * wdVec3::UnitYAxis() - 0.5f * wdVec3::UnitZAxis());
  crossVector4.SetLength(m_fWidth * 0.5f).IgnoreResult();

  const float fDistance = (m_vLastOwnerPosition - m_vLastTargetPosition).GetLength();



  // Build mesh if no existing one is found
  if (!m_hMesh.IsValid())
  {
    wdGeometry g;

    // Quad 1
    {
      wdUInt32 index0 = g.AddVertex(wdVec3::ZeroVector() + crossVector1, wdVec3::UnitXAxis(), wdVec2(0, 0), wdColor::White);
      wdUInt32 index1 = g.AddVertex(wdVec3::ZeroVector() + crossVector4, wdVec3::UnitXAxis(), wdVec2(0, 1), wdColor::White);
      wdUInt32 index2 = g.AddVertex(targetPositionInOwnerSpace + crossVector1, wdVec3::UnitXAxis(), wdVec2(fDistance * m_fUVUnitsPerWorldUnit, 0), wdColor::White);
      wdUInt32 index3 = g.AddVertex(targetPositionInOwnerSpace + crossVector4, wdVec3::UnitXAxis(), wdVec2(fDistance * m_fUVUnitsPerWorldUnit, 1), wdColor::White);

      wdUInt32 indices[] = {index0, index2, index3, index1};
      g.AddPolygon(wdArrayPtr(indices), false);
      g.AddPolygon(wdArrayPtr(indices), true);
    }

    // Quad 2
    {
      wdUInt32 index0 = g.AddVertex(wdVec3::ZeroVector() + crossVector2, wdVec3::UnitXAxis(), wdVec2(0, 0), wdColor::White);
      wdUInt32 index1 = g.AddVertex(wdVec3::ZeroVector() + crossVector3, wdVec3::UnitXAxis(), wdVec2(0, 1), wdColor::White);
      wdUInt32 index2 = g.AddVertex(targetPositionInOwnerSpace + crossVector2, wdVec3::UnitXAxis(), wdVec2(fDistance * m_fUVUnitsPerWorldUnit, 0), wdColor::White);
      wdUInt32 index3 = g.AddVertex(targetPositionInOwnerSpace + crossVector3, wdVec3::UnitXAxis(), wdVec2(fDistance * m_fUVUnitsPerWorldUnit, 1), wdColor::White);

      wdUInt32 indices[] = {index0, index2, index3, index1};
      g.AddPolygon(wdArrayPtr(indices), false);
      g.AddPolygon(wdArrayPtr(indices), true);
    }

    g.ComputeTangents();

    wdMeshResourceDescriptor desc;
    BuildMeshResourceFromGeometry(g, desc);

    m_hMesh = wdResourceManager::CreateResource<wdMeshResource>(meshName, std::move(desc));
  }
}

void wdBeamComponent::BuildMeshResourceFromGeometry(wdGeometry& Geometry, wdMeshResourceDescriptor& MeshDesc) const
{
  auto& MeshBufferDesc = MeshDesc.MeshBufferDesc();

  MeshBufferDesc.AddCommonStreams();
  MeshBufferDesc.AllocateStreamsFromGeometry(Geometry, wdGALPrimitiveTopology::Triangles);

  MeshDesc.AddSubMesh(MeshBufferDesc.GetPrimitiveCount(), 0, 0);

  MeshDesc.ComputeBounds();
}

void wdBeamComponent::ReinitMeshes()
{
  Cleanup();

  if (IsActiveAndInitialized())
  {
    CreateMeshes();
    GetOwner()->UpdateLocalBounds();
  }
}

void wdBeamComponent::Cleanup()
{
  m_hMesh.Invalidate();
}


WD_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_BeamComponent);
