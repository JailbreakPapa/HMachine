#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/Components/RopeRenderComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/SkinnedMeshComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/Types.h>
#include <RendererFoundation/Device/Device.h>

wdCVarBool cvar_FeatureRopesVisBones("Feature.Ropes.VisBones", false, wdCVarFlags::Default, "Enables debug visualization of rope bones");

// clang-format off
WD_BEGIN_COMPONENT_TYPE(wdRopeRenderComponent, 2, wdComponentMode::Static)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Material")),
    WD_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new wdDefaultValueAttribute(wdColor::White), new wdExposeColorAlphaAttribute()),
    WD_ACCESSOR_PROPERTY("Thickness", GetThickness, SetThickness)->AddAttributes(new wdDefaultValueAttribute(0.05f), new wdClampValueAttribute(0.0f, wdVariant())),
    WD_ACCESSOR_PROPERTY("Detail", GetDetail, SetDetail)->AddAttributes(new wdDefaultValueAttribute(6), new wdClampValueAttribute(3, 16)),
    WD_ACCESSOR_PROPERTY("Subdivide", GetSubdivide, SetSubdivide),
    WD_ACCESSOR_PROPERTY("UScale", GetUScale, SetUScale)->AddAttributes(new wdDefaultValueAttribute(1.0f)),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_MESSAGEHANDLERS
  {
    WD_MESSAGE_HANDLER(wdMsgExtractRenderData, OnMsgExtractRenderData),
    WD_MESSAGE_HANDLER(wdMsgRopePoseUpdated, OnRopePoseUpdated),
    WD_MESSAGE_HANDLER(wdMsgSetColor, OnMsgSetColor),
    WD_MESSAGE_HANDLER(wdMsgSetMeshMaterial, OnMsgSetMeshMaterial),      
  }
  WD_END_MESSAGEHANDLERS;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Effects/Ropes"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdRopeRenderComponent::wdRopeRenderComponent() = default;
wdRopeRenderComponent::~wdRopeRenderComponent() = default;

void wdRopeRenderComponent::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_Color;
  s << m_hMaterial;
  s << m_fThickness;
  s << m_uiDetail;
  s << m_bSubdivide;
  s << m_fUScale;
}

void wdRopeRenderComponent::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const wdUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_Color;
  s >> m_hMaterial;
  s >> m_fThickness;
  s >> m_uiDetail;
  s >> m_bSubdivide;
  s >> m_fUScale;
}

void wdRopeRenderComponent::OnActivated()
{
  SUPER::OnActivated();

  m_LocalBounds.SetInvalid();
}

void wdRopeRenderComponent::OnDeactivated()
{
  m_SkinningState.Clear();

  SUPER::OnDeactivated();
}

wdResult wdRopeRenderComponent::GetLocalBounds(wdBoundingBoxSphere& bounds, bool& bAlwaysVisible, wdMsgUpdateLocalBounds& msg)
{
  bounds = m_LocalBounds;
  return WD_SUCCESS;
}

void wdRopeRenderComponent::OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  const wdUInt32 uiFlipWinding = GetOwner()->GetGlobalTransformSimd().ContainsNegativeScale() ? 1 : 0;
  const wdUInt32 uiUniformScale = GetOwner()->GetGlobalTransformSimd().ContainsUniformScale() ? 1 : 0;

  wdResourceLock<wdMeshResource> pMesh(m_hMesh, wdResourceAcquireMode::AllowLoadingFallback);
  wdMaterialResourceHandle hMaterial = m_hMaterial.IsValid() ? m_hMaterial : pMesh->GetMaterials()[0];

  wdSkinnedMeshRenderData* pRenderData = wdCreateRenderDataForThisFrame<wdSkinnedMeshRenderData>(GetOwner());
  {
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
    pRenderData->m_hMesh = m_hMesh;
    pRenderData->m_hMaterial = hMaterial;
    pRenderData->m_Color = m_Color;

    pRenderData->m_uiSubMeshIndex = 0;
    pRenderData->m_uiFlipWinding = uiFlipWinding;
    pRenderData->m_uiUniformScale = uiUniformScale;

    pRenderData->m_uiUniqueID = GetUniqueIdForRendering();

    m_SkinningState.FillSkinnedMeshRenderData(*pRenderData);

    pRenderData->FillBatchIdAndSortingKey();
  }

  // Determine render data category.
  wdRenderData::Category category = wdDefaultRenderDataCategories::LitOpaque;

  if (hMaterial.IsValid())
  {
    wdResourceLock<wdMaterialResource> pMaterial(hMaterial, wdResourceAcquireMode::AllowLoadingFallback);

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

  msg.AddRenderData(pRenderData, category, wdRenderData::Caching::Never);

  if (cvar_FeatureRopesVisBones)
  {
    wdHybridArray<wdDebugRenderer::Line, 128> lines(wdFrameAllocator::GetCurrentAllocator());
    lines.Reserve(m_SkinningState.m_Transforms.GetCount() * 3);

    wdMat4 offsetMat;
    offsetMat.SetIdentity();

    for (wdUInt32 i = 0; i < m_SkinningState.m_Transforms.GetCount(); ++i)
    {
      offsetMat.SetTranslationVector(wdVec3(static_cast<float>(i), 0, 0));
      wdMat4 skinningMat = m_SkinningState.m_Transforms[i].GetAsMat4() * offsetMat;

      wdVec3 pos = skinningMat.GetTranslationVector();

      auto& x = lines.ExpandAndGetRef();
      x.m_start = pos;
      x.m_end = x.m_start + skinningMat.TransformDirection(wdVec3::UnitXAxis());
      x.m_startColor = wdColor::Red;
      x.m_endColor = wdColor::Red;

      auto& y = lines.ExpandAndGetRef();
      y.m_start = pos;
      y.m_end = y.m_start + skinningMat.TransformDirection(wdVec3::UnitYAxis() * 2.0f);
      y.m_startColor = wdColor::Green;
      y.m_endColor = wdColor::Green;

      auto& z = lines.ExpandAndGetRef();
      z.m_start = pos;
      z.m_end = z.m_start + skinningMat.TransformDirection(wdVec3::UnitZAxis() * 2.0f);
      z.m_startColor = wdColor::Blue;
      z.m_endColor = wdColor::Blue;
    }

    wdDebugRenderer::DrawLines(msg.m_pView->GetHandle(), lines, wdColor::White, GetOwner()->GetGlobalTransform());
  }
}

void wdRopeRenderComponent::SetMaterialFile(const char* szFile)
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

const char* wdRopeRenderComponent::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}

void wdRopeRenderComponent::SetThickness(float fThickness)
{
  if (m_fThickness != fThickness)
  {
    m_fThickness = fThickness;

    if (IsActiveAndInitialized() && !m_SkinningState.m_Transforms.IsEmpty())
    {
      wdHybridArray<wdTransform, 128> transforms;
      transforms.SetCountUninitialized(m_SkinningState.m_Transforms.GetCount());

      wdMat4 offsetMat;
      offsetMat.SetIdentity();

      for (wdUInt32 i = 0; i < m_SkinningState.m_Transforms.GetCount(); ++i)
      {
        offsetMat.SetTranslationVector(wdVec3(static_cast<float>(i), 0, 0));
        wdMat4 skinningMat = m_SkinningState.m_Transforms[i].GetAsMat4() * offsetMat;

        transforms[i].SetFromMat4(skinningMat);
      }

      UpdateSkinningTransformBuffer(transforms);
    }
  }
}

void wdRopeRenderComponent::SetDetail(wdUInt32 uiDetail)
{
  if (m_uiDetail != uiDetail)
  {
    m_uiDetail = uiDetail;

    if (IsActiveAndInitialized() && !m_SkinningState.m_Transforms.IsEmpty())
    {
      GenerateRenderMesh(m_SkinningState.m_Transforms.GetCount());
    }
  }
}

void wdRopeRenderComponent::SetSubdivide(bool bSubdivide)
{
  if (m_bSubdivide != bSubdivide)
  {
    m_bSubdivide = bSubdivide;

    if (IsActiveAndInitialized() && !m_SkinningState.m_Transforms.IsEmpty())
    {
      GenerateRenderMesh(m_SkinningState.m_Transforms.GetCount());
    }
  }
}

void wdRopeRenderComponent::SetUScale(float fUScale)
{
  if (m_fUScale != fUScale)
  {
    m_fUScale = fUScale;

    if (IsActiveAndInitialized() && !m_SkinningState.m_Transforms.IsEmpty())
    {
      GenerateRenderMesh(m_SkinningState.m_Transforms.GetCount());
    }
  }
}

void wdRopeRenderComponent::OnMsgSetColor(wdMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);
}

void wdRopeRenderComponent::OnMsgSetMeshMaterial(wdMsgSetMeshMaterial& ref_msg)
{
  SetMaterial(ref_msg.m_hMaterial);
}

void wdRopeRenderComponent::OnRopePoseUpdated(wdMsgRopePoseUpdated& msg)
{
  if (msg.m_LinkTransforms.IsEmpty())
    return;

  if (m_SkinningState.m_Transforms.GetCount() != msg.m_LinkTransforms.GetCount())
  {
    m_SkinningState.Clear();

    GenerateRenderMesh(msg.m_LinkTransforms.GetCount());
  }

  UpdateSkinningTransformBuffer(msg.m_LinkTransforms);

  wdBoundingBox newBounds;
  newBounds.SetFromPoints(&msg.m_LinkTransforms[0].m_vPosition, msg.m_LinkTransforms.GetCount(), sizeof(wdTransform));

  // if the existing bounds are big enough, don't update them
  if (!m_LocalBounds.IsValid() || !m_LocalBounds.GetBox().Contains(newBounds))
  {
    m_LocalBounds.ExpandToInclude(newBounds);

    TriggerLocalBoundsUpdate();
  }
}

void wdRopeRenderComponent::GenerateRenderMesh(wdUInt32 uiNumRopePieces)
{
  wdStringBuilder sResourceName;
  sResourceName.Format("Rope-Mesh:{}{}-d{}-u{}", uiNumRopePieces, m_bSubdivide ? "Sub" : "", m_uiDetail, m_fUScale);

  m_hMesh = wdResourceManager::GetExistingResource<wdMeshResource>(sResourceName);
  if (m_hMesh.IsValid())
    return;

  wdGeometry geom;

  const wdAngle fDegStep = wdAngle::Degree(360.0f / m_uiDetail);
  const float fVStep = 1.0f / m_uiDetail;

  auto addCap = [&](float x, const wdVec3& vNormal, wdUInt16 uiBoneIndex, bool bFlipWinding) {
    wdVec4U16 boneIndices(uiBoneIndex, 0, 0, 0);

    wdUInt32 centerIndex = geom.AddVertex(wdVec3(x, 0, 0), vNormal, wdVec2(0.5f, 0.5f), wdColor::White, boneIndices);

    wdAngle deg = wdAngle::Radian(0);
    for (wdUInt32 s = 0; s < m_uiDetail; ++s)
    {
      const float fY = wdMath::Cos(deg);
      const float fZ = wdMath::Sin(deg);

      geom.AddVertex(wdVec3(x, fY, fZ), vNormal, wdVec2(fY, fZ), wdColor::White, boneIndices);

      deg += fDegStep;
    }

    wdUInt32 triangle[3];
    triangle[0] = centerIndex;
    for (wdUInt32 s = 0; s < m_uiDetail; ++s)
    {
      triangle[1] = s + triangle[0] + 1;
      triangle[2] = ((s + 1) % m_uiDetail) + triangle[0] + 1;

      geom.AddPolygon(triangle, bFlipWinding);
    }
  };

  auto addPiece = [&](float x, const wdVec4U16& vBoneIndices, const wdColorLinearUB& boneWeights, bool bCreatePolygons) {
    wdAngle deg = wdAngle::Radian(0);
    float fU = x * m_fUScale;
    float fV = 0;

    for (wdUInt32 s = 0; s <= m_uiDetail; ++s)
    {
      const float fY = wdMath::Cos(deg);
      const float fZ = wdMath::Sin(deg);

      const wdVec3 pos(x, fY, fZ);
      const wdVec3 normal(0, fY, fZ);

      geom.AddVertex(pos, normal, wdVec2(fU, fV), wdColor::White, vBoneIndices, boneWeights);

      deg += fDegStep;
      fV += fVStep;
    }

    if (bCreatePolygons)
    {
      wdUInt32 endIndex = geom.GetVertices().GetCount() - (m_uiDetail + 1);
      wdUInt32 startIndex = endIndex - (m_uiDetail + 1);

      wdUInt32 triangle[3];
      for (wdUInt32 s = 0; s < m_uiDetail; ++s)
      {
        triangle[0] = startIndex + s;
        triangle[1] = startIndex + s + 1;
        triangle[2] = endIndex + s + 1;
        geom.AddPolygon(triangle, false);

        triangle[0] = startIndex + s;
        triangle[1] = endIndex + s + 1;
        triangle[2] = endIndex + s;
        geom.AddPolygon(triangle, false);
      }
    }
  };

  // cap
  {
    const wdVec3 normal = wdVec3(-1, 0, 0);
    addCap(0.0f, normal, 0, true);
  }

  // pieces
  {
    // first ring full weight to first bone
    addPiece(0.0f, wdVec4U16(0, 0, 0, 0), wdColorLinearUB(255, 0, 0, 0), false);

    wdUInt16 p = 1;

    if (m_bSubdivide)
    {
      addPiece(0.75f, wdVec4U16(0, 0, 0, 0), wdColorLinearUB(255, 0, 0, 0), true);

      for (; p < uiNumRopePieces - 2; ++p)
      {
        addPiece(static_cast<float>(p) + 0.25f, wdVec4U16(p, 0, 0, 0), wdColorLinearUB(255, 0, 0, 0), true);
        addPiece(static_cast<float>(p) + 0.75f, wdVec4U16(p, 0, 0, 0), wdColorLinearUB(255, 0, 0, 0), true);
      }

      addPiece(static_cast<float>(p) + 0.25f, wdVec4U16(p, 0, 0, 0), wdColorLinearUB(255, 0, 0, 0), true);
      ++p;
    }
    else
    {
      for (; p < uiNumRopePieces - 1; ++p)
      {
        // Middle rings half weight between bones. To ensure that weights sum up to 1 we weight one bone with 128 and the other with 127,
        // since "ubyte normalized" can't represent 0.5 perfectly.
        addPiece(static_cast<float>(p), wdVec4U16(p - 1, p, 0, 0), wdColorLinearUB(128, 127, 0, 0), true);
      }
    }

    // last ring full weight to last bone
    addPiece(static_cast<float>(p), wdVec4U16(p, 0, 0, 0), wdColorLinearUB(255, 0, 0, 0), true);
  }

  // cap
  {
    const wdVec3 normal = wdVec3(1, 0, 0);
    addCap(static_cast<float>(uiNumRopePieces - 1), normal, static_cast<wdUInt16>(uiNumRopePieces - 1), false);
  }

  geom.ComputeTangents();

  wdMeshResourceDescriptor desc;

  // Data/Base/Materials/Prototyping/PrototypeBlack.wdMaterialAsset
  desc.SetMaterial(0, "{ d615cd66-0904-00ca-81f9-768ff4fc24ee }");

  auto& meshBufferDesc = desc.MeshBufferDesc();
  meshBufferDesc.AddCommonStreams();
  meshBufferDesc.AddStream(wdGALVertexAttributeSemantic::BoneIndices0, wdGALResourceFormat::RGBAUByte);
  meshBufferDesc.AddStream(wdGALVertexAttributeSemantic::BoneWeights0, wdGALResourceFormat::RGBAUByteNormalized);
  meshBufferDesc.AllocateStreamsFromGeometry(geom, wdGALPrimitiveTopology::Triangles);

  desc.AddSubMesh(meshBufferDesc.GetPrimitiveCount(), 0, 0);

  desc.ComputeBounds();

  m_hMesh = wdResourceManager::CreateResource<wdMeshResource>(sResourceName, std::move(desc), sResourceName);
}

void wdRopeRenderComponent::UpdateSkinningTransformBuffer(wdArrayPtr<const wdTransform> skinningTransforms)
{
  wdMat4 bindPoseMat;
  bindPoseMat.SetIdentity();
  m_SkinningState.m_Transforms.SetCountUninitialized(skinningTransforms.GetCount());

  const wdVec3 newScale = wdVec3(1.0f, m_fThickness * 0.5f, m_fThickness * 0.5f);
  for (wdUInt32 i = 0; i < skinningTransforms.GetCount(); ++i)
  {
    wdTransform t = skinningTransforms[i];
    t.m_vScale = newScale;

    // scale x axis to match the distance between this bone and the next bone
    if (i < skinningTransforms.GetCount() - 1)
    {
      t.m_vScale.x = (skinningTransforms[i + 1].m_vPosition - skinningTransforms[i].m_vPosition).GetLength();
    }

    bindPoseMat.SetTranslationVector(wdVec3(-static_cast<float>(i), 0, 0));

    m_SkinningState.m_Transforms[i] = t.GetAsMat4() * bindPoseMat;
  }

  m_SkinningState.TransformsChanged();
}


WD_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_RopeRenderComponent);
