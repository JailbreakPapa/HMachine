#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/SpriteComponent.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Textures/Texture2DResource.h>

// clang-format off
WD_BEGIN_STATIC_REFLECTED_ENUM(wdSpriteBlendMode, 1)
  WD_ENUM_CONSTANTS(wdSpriteBlendMode::Masked, wdSpriteBlendMode::Transparent, wdSpriteBlendMode::Additive)
WD_END_STATIC_REFLECTED_ENUM;
// clang-format on

// static
wdTempHashedString wdSpriteBlendMode::GetPermutationValue(Enum blendMode)
{
  switch (blendMode)
  {
    case wdSpriteBlendMode::Masked:
      return "BLEND_MODE_MASKED";
    case wdSpriteBlendMode::Transparent:
      return "BLEND_MODE_TRANSPARENT";
    case wdSpriteBlendMode::Additive:
      return "BLEND_MODE_ADDITIVE";
  }

  return "";
}

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdSpriteRenderData, 1, wdRTTIDefaultAllocator<wdSpriteRenderData>)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void wdSpriteRenderData::FillBatchIdAndSortingKey()
{
  // ignore upper 32 bit of the resource ID hash
  const wdUInt32 uiTextureIDHash = static_cast<wdUInt32>(m_hTexture.GetResourceIDHash());

  // Generate batch id from mode and texture
  wdUInt32 data[] = {(wdUInt32)m_BlendMode, uiTextureIDHash};
  m_uiBatchId = wdHashingUtils::xxHash32(data, sizeof(data));

  // Sort by mode and then by texture
  m_uiSortingKey = (m_BlendMode << 30) | (uiTextureIDHash & 0x3FFFFFFF);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_COMPONENT_TYPE(wdSpriteComponent, 3, wdComponentMode::Static)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("Texture", GetTextureFile, SetTextureFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    WD_ENUM_MEMBER_PROPERTY("BlendMode", wdSpriteBlendMode, m_BlendMode),
    WD_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new wdExposeColorAlphaAttribute()),
    WD_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new wdClampValueAttribute(0.0f, wdVariant()), new wdDefaultValueAttribute(1.0f), new wdSuffixAttribute(" m")),
    WD_ACCESSOR_PROPERTY("MaxScreenSize", GetMaxScreenSize, SetMaxScreenSize)->AddAttributes(new wdClampValueAttribute(0.0f, wdVariant()), new wdDefaultValueAttribute(64.0f), new wdSuffixAttribute(" px")),
    WD_MEMBER_PROPERTY("AspectRatio", m_fAspectRatio)->AddAttributes(new wdClampValueAttribute(0.0f, wdVariant()), new wdDefaultValueAttribute(1.0f)),
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
    WD_MESSAGE_HANDLER(wdMsgSetColor, OnMsgSetColor),
  }
  WD_END_MESSAGEHANDLERS;
}
WD_END_COMPONENT_TYPE;
// clang-format on

wdSpriteComponent::wdSpriteComponent() = default;
wdSpriteComponent::~wdSpriteComponent() = default;

wdResult wdSpriteComponent::GetLocalBounds(wdBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, wdMsgUpdateLocalBounds& ref_msg)
{
  ref_bounds = wdBoundingSphere(wdVec3::ZeroVector(), m_fSize * 0.5f);
  return WD_SUCCESS;
}

void wdSpriteComponent::OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const
{
  // Don't render in shadow views
  if (msg.m_pView->GetCameraUsageHint() == wdCameraUsageHint::Shadow)
    return;

  if (!m_hTexture.IsValid())
    return;

  wdSpriteRenderData* pRenderData = wdCreateRenderDataForThisFrame<wdSpriteRenderData>(GetOwner());
  {
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
    pRenderData->m_hTexture = m_hTexture;
    pRenderData->m_fSize = m_fSize;
    pRenderData->m_fMaxScreenSize = m_fMaxScreenSize;
    pRenderData->m_fAspectRatio = m_fAspectRatio;
    pRenderData->m_BlendMode = m_BlendMode;
    pRenderData->m_color = m_Color;
    pRenderData->m_texCoordScale = wdVec2(1.0f);
    pRenderData->m_texCoordOffset = wdVec2(0.0f);
    pRenderData->m_uiUniqueID = GetUniqueIdForRendering();

    pRenderData->FillBatchIdAndSortingKey();
  }

  // Determine render data category.
  wdRenderData::Category category = wdDefaultRenderDataCategories::LitTransparent;
  if (m_BlendMode == wdSpriteBlendMode::Masked)
  {
    category = wdDefaultRenderDataCategories::LitMasked;
  }

  msg.AddRenderData(pRenderData, category, wdRenderData::Caching::IfStatic);
}

void wdSpriteComponent::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  wdStreamWriter& s = inout_stream.GetStream();

  s << m_hTexture;
  s << m_fSize;
  s << m_fMaxScreenSize;

  // Version 3
  s << m_Color; // HDR now
  s << m_fAspectRatio;
  s << m_BlendMode;
}

void wdSpriteComponent::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const wdUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  wdStreamReader& s = inout_stream.GetStream();

  s >> m_hTexture;

  if (uiVersion < 3)
  {
    wdColorGammaUB color;
    s >> color;
    m_Color = color;
  }

  s >> m_fSize;
  s >> m_fMaxScreenSize;

  if (uiVersion >= 3)
  {
    s >> m_Color;
    s >> m_fAspectRatio;
    s >> m_BlendMode;
  }
}

void wdSpriteComponent::SetTexture(const wdTexture2DResourceHandle& hTexture)
{
  m_hTexture = hTexture;
}

const wdTexture2DResourceHandle& wdSpriteComponent::GetTexture() const
{
  return m_hTexture;
}

void wdSpriteComponent::SetTextureFile(const char* szFile)
{
  wdTexture2DResourceHandle hTexture;

  if (!wdStringUtils::IsNullOrEmpty(szFile))
  {
    hTexture = wdResourceManager::LoadResource<wdTexture2DResource>(szFile);
  }

  SetTexture(hTexture);
}

const char* wdSpriteComponent::GetTextureFile() const
{
  if (!m_hTexture.IsValid())
    return "";

  return m_hTexture.GetResourceID();
}

void wdSpriteComponent::SetColor(wdColor color)
{
  m_Color = color;
}

wdColor wdSpriteComponent::GetColor() const
{
  return m_Color;
}

void wdSpriteComponent::SetSize(float fSize)
{
  m_fSize = fSize;

  TriggerLocalBoundsUpdate();
}

float wdSpriteComponent::GetSize() const
{
  return m_fSize;
}

void wdSpriteComponent::SetMaxScreenSize(float fSize)
{
  m_fMaxScreenSize = fSize;
}

float wdSpriteComponent::GetMaxScreenSize() const
{
  return m_fMaxScreenSize;
}

void wdSpriteComponent::OnMsgSetColor(wdMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class wdSpriteComponentPatch_1_2 : public wdGraphPatch
{
public:
  wdSpriteComponentPatch_1_2()
    : wdGraphPatch("wdSpriteComponent", 2)
  {
  }

  virtual void Patch(wdGraphPatchContext& ref_context, wdAbstractObjectGraph* pGraph, wdAbstractObjectNode* pNode) const override { pNode->RenameProperty("Max Screen Size", "MaxScreenSize"); }
};

wdSpriteComponentPatch_1_2 g_wdSpriteComponentPatch_1_2;



WD_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_SpriteComponent);
