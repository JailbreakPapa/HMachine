#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_IMPLEMENT_MESSAGE_TYPE(wdMsgSetMeshMaterial);
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdMsgSetMeshMaterial, 1, wdRTTIDefaultAllocator<wdMsgSetMeshMaterial>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("Material", GetMaterialFile, SetMaterialFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Material")),
    WD_MEMBER_PROPERTY("MaterialSlot", m_uiMaterialSlot),
  }
  WD_END_PROPERTIES;

  WD_BEGIN_ATTRIBUTES
  {
    new wdAutoGenVisScriptMsgSender,
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void wdMsgSetMeshMaterial::SetMaterialFile(const char* szFile)
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

const char* wdMsgSetMeshMaterial::GetMaterialFile() const
{
  if (!m_hMaterial.IsValid())
    return "";

  return m_hMaterial.GetResourceID();
}

void wdMsgSetMeshMaterial::Serialize(wdStreamWriter& inout_stream) const
{
  // has to be stringyfied for transfer
  inout_stream << GetMaterialFile();
  inout_stream << m_uiMaterialSlot;
}

void wdMsgSetMeshMaterial::Deserialize(wdStreamReader& inout_stream, wdUInt8 uiTypeVersion)
{
  wdStringBuilder file;
  inout_stream >> file;
  SetMaterialFile(file);

  inout_stream >> m_uiMaterialSlot;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdMeshRenderData, 1, wdRTTIDefaultAllocator<wdMeshRenderData>)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void wdMeshRenderData::FillBatchIdAndSortingKey()
{
  FillBatchIdAndSortingKeyInternal(0);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_ABSTRACT_COMPONENT_TYPE(wdMeshComponentBase, 1)
{
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Rendering"),
  }
  WD_END_ATTRIBUTES;
  WD_BEGIN_MESSAGEHANDLERS
  {
    WD_MESSAGE_HANDLER(wdMsgExtractRenderData, OnMsgExtractRenderData),
    WD_MESSAGE_HANDLER(wdMsgSetMeshMaterial, OnMsgSetMeshMaterial),
    WD_MESSAGE_HANDLER(wdMsgSetColor, OnMsgSetColor),
  } WD_END_MESSAGEHANDLERS;
}
WD_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

wdMeshComponentBase::wdMeshComponentBase() = default;
wdMeshComponentBase::~wdMeshComponentBase() = default;

void wdMeshComponentBase::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  wdStreamWriter& s = inout_stream.GetStream();

  // ignore components that have created meshes (?)

  s << m_hMesh;

  wdUInt32 uiCategory = m_RenderDataCategory.m_uiValue;
  s << uiCategory;

  s << m_Materials.GetCount();

  for (const auto& mat : m_Materials)
  {
    s << mat;
  }

  s << m_Color;
}

void wdMeshComponentBase::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const wdUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  wdStreamReader& s = inout_stream.GetStream();

  s >> m_hMesh;

  wdUInt32 uiCategory = 0;
  s >> uiCategory;
  m_RenderDataCategory.m_uiValue = static_cast<wdUInt16>(uiCategory);

  wdUInt32 uiMaterials = 0;
  s >> uiMaterials;

  m_Materials.SetCount(uiMaterials);

  for (auto& mat : m_Materials)
  {
    s >> mat;
  }

  s >> m_Color;
}

wdResult wdMeshComponentBase::GetLocalBounds(wdBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, wdMsgUpdateLocalBounds& ref_msg)
{
  if (m_hMesh.IsValid())
  {
    wdResourceLock<wdMeshResource> pMesh(m_hMesh, wdResourceAcquireMode::AllowLoadingFallback);
    ref_bounds = pMesh->GetBounds();
    return WD_SUCCESS;
  }

  return WD_FAILURE;
}

void wdMeshComponentBase::OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  wdResourceLock<wdMeshResource> pMesh(m_hMesh, wdResourceAcquireMode::AllowLoadingFallback);
  wdArrayPtr<const wdMeshResourceDescriptor::SubMesh> parts = pMesh->GetSubMeshes();

  for (wdUInt32 uiPartIndex = 0; uiPartIndex < parts.GetCount(); ++uiPartIndex)
  {
    const wdUInt32 uiMaterialIndex = parts[uiPartIndex].m_uiMaterialIndex;
    wdMaterialResourceHandle hMaterial;

    // If we have a material override, use that otherwise use the default mesh material.
    if (GetMaterial(uiMaterialIndex).IsValid())
      hMaterial = m_Materials[uiMaterialIndex];
    else
      hMaterial = pMesh->GetMaterials()[uiMaterialIndex];

    wdMeshRenderData* pRenderData = CreateRenderData();
    {
      pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform() * pRenderData->m_GlobalTransform;
      pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
      pRenderData->m_hMesh = m_hMesh;
      pRenderData->m_hMaterial = hMaterial;
      pRenderData->m_Color = m_Color;
      pRenderData->m_uiSubMeshIndex = uiPartIndex;
      pRenderData->m_uiUniqueID = GetUniqueIdForRendering(uiMaterialIndex);

      pRenderData->FillBatchIdAndSortingKey();
    }

    bool bDontCacheYet = false;

    // Determine render data category.
    wdRenderData::Category category = m_RenderDataCategory;
    if (category == wdInvalidRenderDataCategory)
    {
      if (hMaterial.IsValid())
      {
        wdResourceLock<wdMaterialResource> pMaterial(hMaterial, wdResourceAcquireMode::AllowLoadingFallback);

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
      else
      {
        category = wdDefaultRenderDataCategories::LitOpaque;
      }
    }

    msg.AddRenderData(pRenderData, category, bDontCacheYet ? wdRenderData::Caching::Never : wdRenderData::Caching::IfStatic);
  }
}

void wdMeshComponentBase::SetMesh(const wdMeshResourceHandle& hMesh)
{
  if (m_hMesh != hMesh)
  {
    m_hMesh = hMesh;

    TriggerLocalBoundsUpdate();
    InvalidateCachedRenderData();
  }
}

void wdMeshComponentBase::SetMaterial(wdUInt32 uiIndex, const wdMaterialResourceHandle& hMaterial)
{
  m_Materials.EnsureCount(uiIndex + 1);

  if (m_Materials[uiIndex] != hMaterial)
  {
    m_Materials[uiIndex] = hMaterial;

    InvalidateCachedRenderData();
  }
}

wdMaterialResourceHandle wdMeshComponentBase::GetMaterial(wdUInt32 uiIndex) const
{
  if (uiIndex >= m_Materials.GetCount())
    return wdMaterialResourceHandle();

  return m_Materials[uiIndex];
}

void wdMeshComponentBase::SetMeshFile(const char* szFile)
{
  wdMeshResourceHandle hMesh;

  if (!wdStringUtils::IsNullOrEmpty(szFile))
  {
    hMesh = wdResourceManager::LoadResource<wdMeshResource>(szFile);
  }

  SetMesh(hMesh);
}

const char* wdMeshComponentBase::GetMeshFile() const
{
  if (!m_hMesh.IsValid())
    return "";

  return m_hMesh.GetResourceID();
}

void wdMeshComponentBase::SetColor(const wdColor& color)
{
  m_Color = color;

  InvalidateCachedRenderData();
}

const wdColor& wdMeshComponentBase::GetColor() const
{
  return m_Color;
}

void wdMeshComponentBase::OnMsgSetMeshMaterial(wdMsgSetMeshMaterial& ref_msg)
{
  SetMaterial(ref_msg.m_uiMaterialSlot, ref_msg.m_hMaterial);
}

void wdMeshComponentBase::OnMsgSetColor(wdMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);

  InvalidateCachedRenderData();
}

wdMeshRenderData* wdMeshComponentBase::CreateRenderData() const
{
  return wdCreateRenderDataForThisFrame<wdMeshRenderData>(GetOwner());
}

wdUInt32 wdMeshComponentBase::Materials_GetCount() const
{
  return m_Materials.GetCount();
}

const char* wdMeshComponentBase::Materials_GetValue(wdUInt32 uiIndex) const
{
  auto hMat = GetMaterial(uiIndex);

  if (!hMat.IsValid())
    return "";

  return hMat.GetResourceID();
}


void wdMeshComponentBase::Materials_SetValue(wdUInt32 uiIndex, const char* value)
{
  if (wdStringUtils::IsNullOrEmpty(value))
    SetMaterial(uiIndex, wdMaterialResourceHandle());
  else
  {
    auto hMat = wdResourceManager::LoadResource<wdMaterialResource>(value);
    SetMaterial(uiIndex, hMat);
  }
}


void wdMeshComponentBase::Materials_Insert(wdUInt32 uiIndex, const char* value)
{
  wdMaterialResourceHandle hMat;

  if (!wdStringUtils::IsNullOrEmpty(value))
    hMat = wdResourceManager::LoadResource<wdMaterialResource>(value);

  m_Materials.Insert(hMat, uiIndex);

  InvalidateCachedRenderData();
}


void wdMeshComponentBase::Materials_Remove(wdUInt32 uiIndex)
{
  m_Materials.RemoveAtAndCopy(uiIndex);

  InvalidateCachedRenderData();
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshComponentBase);
