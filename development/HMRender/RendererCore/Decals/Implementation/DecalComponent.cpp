#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Messages/ApplyOnlyToMessage.h>
#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <RendererCore/Decals/DecalAtlasResource.h>
#include <RendererCore/Decals/DecalComponent.h>
#include <RendererCore/Decals/DecalResource.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightData.h>

wdDecalComponentManager::wdDecalComponentManager(wdWorld* pWorld)
  : wdComponentManager<wdDecalComponent, wdBlockStorageType::Compact>(pWorld)
{
}

void wdDecalComponentManager::Initialize()
{
  m_hDecalAtlas = wdDecalAtlasResource::GetDecalAtlasResource();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdDecalRenderData, 1, wdRTTIDefaultAllocator<wdDecalRenderData>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_BEGIN_COMPONENT_TYPE(wdDecalComponent, 8, wdComponentMode::Static)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ARRAY_ACCESSOR_PROPERTY("Decals", DecalFile_GetCount, DecalFile_Get, DecalFile_Set, DecalFile_Insert, DecalFile_Remove)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Decal")),
    WD_ENUM_ACCESSOR_PROPERTY("ProjectionAxis", wdBasisAxis, GetProjectionAxis, SetProjectionAxis),
    WD_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new wdDefaultValueAttribute(wdVec3(1.0f)), new wdClampValueAttribute(wdVec3(0.01f), wdVariant(25.0f))),
    WD_ACCESSOR_PROPERTY("SizeVariance", GetSizeVariance, SetSizeVariance)->AddAttributes(new wdClampValueAttribute(0.0f, 1.0f)),
    WD_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new wdExposeColorAlphaAttribute()),
    WD_ACCESSOR_PROPERTY("EmissiveColor", GetEmissiveColor, SetEmissiveColor)->AddAttributes(new wdDefaultValueAttribute(wdColor::Black)),
    WD_ACCESSOR_PROPERTY("SortOrder", GetSortOrder, SetSortOrder)->AddAttributes(new wdClampValueAttribute(-64.0f, 64.0f)),
    WD_ACCESSOR_PROPERTY("WrapAround", GetWrapAround, SetWrapAround),
    WD_ACCESSOR_PROPERTY("MapNormalToGeometry", GetMapNormalToGeometry, SetMapNormalToGeometry)->AddAttributes(new wdDefaultValueAttribute(true)),
    WD_ACCESSOR_PROPERTY("InnerFadeAngle", GetInnerFadeAngle, SetInnerFadeAngle)->AddAttributes(new wdClampValueAttribute(wdAngle::Degree(0.0f), wdAngle::Degree(89.0f)), new wdDefaultValueAttribute(wdAngle::Degree(50.0f))),
    WD_ACCESSOR_PROPERTY("OuterFadeAngle", GetOuterFadeAngle, SetOuterFadeAngle)->AddAttributes(new wdClampValueAttribute(wdAngle::Degree(0.0f), wdAngle::Degree(89.0f)), new wdDefaultValueAttribute(wdAngle::Degree(80.0f))),
    WD_MEMBER_PROPERTY("FadeOutDelay", m_FadeOutDelay),
    WD_MEMBER_PROPERTY("FadeOutDuration", m_FadeOutDuration),
    WD_ENUM_MEMBER_PROPERTY("OnFinishedAction", wdOnComponentFinishedAction, m_OnFinishedAction),
    WD_ACCESSOR_PROPERTY("ApplyToDynamic", DummyGetter, SetApplyToRef)->AddAttributes(new wdGameObjectReferenceAttribute()),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Effects"),
    new wdDirectionVisualizerAttribute("ProjectionAxis", 0.5f, wdColorScheme::LightUI(wdColorScheme::Blue)),
    new wdBoxManipulatorAttribute("Extents", 1.0f, true),
    new wdBoxVisualizerAttribute("Extents"),
  }
  WD_END_ATTRIBUTES;
  WD_BEGIN_MESSAGEHANDLERS
  {
    WD_MESSAGE_HANDLER(wdMsgExtractRenderData, OnMsgExtractRenderData),
    WD_MESSAGE_HANDLER(wdMsgComponentInternalTrigger, OnTriggered),
    WD_MESSAGE_HANDLER(wdMsgDeleteGameObject, OnMsgDeleteGameObject),
    WD_MESSAGE_HANDLER(wdMsgOnlyApplyToObject, OnMsgOnlyApplyToObject),
    WD_MESSAGE_HANDLER(wdMsgSetColor, OnMsgSetColor),
  }
  WD_END_MESSAGEHANDLERS;
}
WD_END_COMPONENT_TYPE
// clang-format on

wdDecalComponent::wdDecalComponent() = default;

wdDecalComponent::~wdDecalComponent() = default;

void wdDecalComponent::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  wdStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
  s << m_Color;
  s << m_EmissiveColor;
  s << m_InnerFadeAngle;
  s << m_OuterFadeAngle;
  s << m_fSortOrder;
  s << m_FadeOutDelay.m_Value;
  s << m_FadeOutDelay.m_fVariance;
  s << m_FadeOutDuration;
  s << m_StartFadeOutTime;
  s << m_fSizeVariance;
  s << m_OnFinishedAction;
  s << m_bWrapAround;
  s << m_bMapNormalToGeometry;

  // version 5
  s << m_ProjectionAxis;

  // version 6
  inout_stream.WriteGameObjectHandle(m_hApplyOnlyToObject);

  // version 7
  s << m_uiRandomDecalIdx;
  s.WriteArray(m_Decals).IgnoreResult();
}

void wdDecalComponent::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const wdUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  wdStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;

  if (uiVersion >= 4)
  {
    s >> m_Color;
    s >> m_EmissiveColor;
  }
  else
  {
    wdColor tmp;
    s >> tmp;
    m_Color = tmp;
  }

  s >> m_InnerFadeAngle;
  s >> m_OuterFadeAngle;
  s >> m_fSortOrder;

  if (uiVersion <= 7)
  {
    wdUInt32 dummy;
    s >> dummy;
  }

  m_uiInternalSortKey = GetOwner()->GetStableRandomSeed();
  m_uiInternalSortKey = (m_uiInternalSortKey >> 16) ^ (m_uiInternalSortKey & 0xFFFF);

  if (uiVersion < 7)
  {
    m_Decals.SetCount(1);
    s >> m_Decals[0];
  }

  s >> m_FadeOutDelay.m_Value;
  s >> m_FadeOutDelay.m_fVariance;
  s >> m_FadeOutDuration;
  s >> m_StartFadeOutTime;
  s >> m_fSizeVariance;
  s >> m_OnFinishedAction;

  if (uiVersion >= 3)
  {
    s >> m_bWrapAround;
  }

  if (uiVersion >= 4)
  {
    s >> m_bMapNormalToGeometry;
  }

  if (uiVersion >= 5)
  {
    s >> m_ProjectionAxis;
  }

  if (uiVersion >= 6)
  {
    SetApplyOnlyTo(inout_stream.ReadGameObjectHandle());
  }

  if (uiVersion >= 7)
  {
    s >> m_uiRandomDecalIdx;
    s.ReadArray(m_Decals).IgnoreResult();
  }
}

wdResult wdDecalComponent::GetLocalBounds(wdBoundingBoxSphere& bounds, bool& bAlwaysVisible, wdMsgUpdateLocalBounds& msg)
{
  if (m_Decals.IsEmpty())
    return WD_FAILURE;

  m_uiRandomDecalIdx = (GetOwner()->GetStableRandomSeed() % m_Decals.GetCount()) & 0xFF;

  const wdUInt32 uiDecalIndex = wdMath::Min<wdUInt32>(m_uiRandomDecalIdx, m_Decals.GetCount() - 1);

  if (!m_Decals[uiDecalIndex].IsValid() || m_vExtents.IsZero())
    return WD_FAILURE;

  float fAspectRatio = 1.0f;

  {
    auto hDecalAtlas = GetWorld()->GetComponentManager<wdDecalComponentManager>()->m_hDecalAtlas;
    wdResourceLock<wdDecalAtlasResource> pDecalAtlas(hDecalAtlas, wdResourceAcquireMode::BlockTillLoaded);

    const auto& atlas = pDecalAtlas->GetAtlas();
    const wdUInt32 decalIdx = atlas.m_Items.Find(wdHashingUtils::StringHashTo32(m_Decals[uiDecalIndex].GetResourceIDHash()));

    if (decalIdx != wdInvalidIndex)
    {
      const auto& item = atlas.m_Items.GetValue(decalIdx);
      fAspectRatio = (float)item.m_LayerRects[0].width / item.m_LayerRects[0].height;
    }
  }

  wdVec3 vAspectCorrection = wdVec3(1.0f);
  if (!wdMath::IsEqual(fAspectRatio, 1.0f, 0.001f))
  {
    if (fAspectRatio > 1.0f)
    {
      vAspectCorrection.z /= fAspectRatio;
    }
    else
    {
      vAspectCorrection.y *= fAspectRatio;
    }
  }

  const wdQuat axisRotation = wdBasisAxis::GetBasisRotation_PosX(m_ProjectionAxis);
  wdVec3 vHalfExtents = (axisRotation * vAspectCorrection).Abs().CompMul(m_vExtents * 0.5f);

  bounds = wdBoundingBox(-vHalfExtents, vHalfExtents);
  return WD_SUCCESS;
}

void wdDecalComponent::SetExtents(const wdVec3& value)
{
  m_vExtents = value.CompMax(wdVec3::ZeroVector());

  TriggerLocalBoundsUpdate();
}

const wdVec3& wdDecalComponent::GetExtents() const
{
  return m_vExtents;
}

void wdDecalComponent::SetSizeVariance(float fVariance)
{
  m_fSizeVariance = wdMath::Clamp(fVariance, 0.0f, 1.0f);
}

float wdDecalComponent::GetSizeVariance() const
{
  return m_fSizeVariance;
}

void wdDecalComponent::SetColor(wdColorGammaUB color)
{
  m_Color = color;
}

wdColorGammaUB wdDecalComponent::GetColor() const
{
  return m_Color;
}

void wdDecalComponent::SetEmissiveColor(wdColor color)
{
  m_EmissiveColor = color;
}

wdColor wdDecalComponent::GetEmissiveColor() const
{
  return m_EmissiveColor;
}

void wdDecalComponent::SetInnerFadeAngle(wdAngle spotAngle)
{
  m_InnerFadeAngle = wdMath::Clamp(spotAngle, wdAngle::Degree(0.0f), m_OuterFadeAngle);
}

wdAngle wdDecalComponent::GetInnerFadeAngle() const
{
  return m_InnerFadeAngle;
}

void wdDecalComponent::SetOuterFadeAngle(wdAngle spotAngle)
{
  m_OuterFadeAngle = wdMath::Clamp(spotAngle, m_InnerFadeAngle, wdAngle::Degree(90.0f));
}

wdAngle wdDecalComponent::GetOuterFadeAngle() const
{
  return m_OuterFadeAngle;
}

void wdDecalComponent::SetSortOrder(float fOrder)
{
  m_fSortOrder = fOrder;
}

float wdDecalComponent::GetSortOrder() const
{
  return m_fSortOrder;
}

void wdDecalComponent::SetWrapAround(bool bWrapAround)
{
  m_bWrapAround = bWrapAround;
}

bool wdDecalComponent::GetWrapAround() const
{
  return m_bWrapAround;
}

void wdDecalComponent::SetMapNormalToGeometry(bool bMapNormal)
{
  m_bMapNormalToGeometry = bMapNormal;
}

bool wdDecalComponent::GetMapNormalToGeometry() const
{
  return m_bMapNormalToGeometry;
}

void wdDecalComponent::SetDecal(wdUInt32 uiIndex, const wdDecalResourceHandle& hDecal)
{
  m_Decals[uiIndex] = hDecal;

  TriggerLocalBoundsUpdate();
}

const wdDecalResourceHandle& wdDecalComponent::GetDecal(wdUInt32 uiIndex) const
{
  return m_Decals[uiIndex];
}

void wdDecalComponent::SetProjectionAxis(wdEnum<wdBasisAxis> projectionAxis)
{
  m_ProjectionAxis = projectionAxis;

  TriggerLocalBoundsUpdate();
}

wdEnum<wdBasisAxis> wdDecalComponent::GetProjectionAxis() const
{
  return m_ProjectionAxis;
}

void wdDecalComponent::SetApplyOnlyTo(wdGameObjectHandle hObject)
{
  if (m_hApplyOnlyToObject != hObject)
  {
    m_hApplyOnlyToObject = hObject;
    UpdateApplyTo();
  }
}

wdGameObjectHandle wdDecalComponent::GetApplyOnlyTo() const
{
  return m_hApplyOnlyToObject;
}

void wdDecalComponent::OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const
{
  // Don't extract decal render data for selection.
  if (msg.m_OverrideCategory != wdInvalidRenderDataCategory)
    return;

  if (m_Decals.IsEmpty())
    return;

  const wdUInt32 uiDecalIndex = wdMath::Min<wdUInt32>(m_uiRandomDecalIdx, m_Decals.GetCount() - 1);

  if (!m_Decals[uiDecalIndex].IsValid() || m_vExtents.IsZero() || GetOwner()->GetLocalScaling().IsZero())
    return;

  float fFade = 1.0f;

  const wdTime tNow = GetWorld()->GetClock().GetAccumulatedTime();
  if (tNow > m_StartFadeOutTime)
  {
    fFade -= wdMath::Min<float>(1.0f, (float)((tNow - m_StartFadeOutTime).GetSeconds() / m_FadeOutDuration.GetSeconds()));
  }

  wdColor finalColor = m_Color;
  finalColor.a *= fFade;

  if (finalColor.a <= 0.0f)
    return;

  const bool bNoFade = m_InnerFadeAngle == wdAngle::Radian(0.0f) && m_OuterFadeAngle == wdAngle::Radian(0.0f);
  const float fCosInner = wdMath::Cos(m_InnerFadeAngle);
  const float fCosOuter = wdMath::Cos(m_OuterFadeAngle);
  const float fFadeParamScale = bNoFade ? 0.0f : (1.0f / wdMath::Max(0.001f, (fCosInner - fCosOuter)));
  const float fFadeParamOffset = bNoFade ? 1.0f : (-fCosOuter * fFadeParamScale);

  auto hDecalAtlas = GetWorld()->GetComponentManager<wdDecalComponentManager>()->m_hDecalAtlas;
  wdVec4 baseAtlasScaleOffset = wdVec4(0.5f);
  wdVec4 normalAtlasScaleOffset = wdVec4(0.5f);
  wdVec4 ormAtlasScaleOffset = wdVec4(0.5f);
  wdUInt32 uiDecalFlags = 0;

  float fAspectRatio = 1.0f;

  {
    wdResourceLock<wdDecalAtlasResource> pDecalAtlas(hDecalAtlas, wdResourceAcquireMode::BlockTillLoaded);

    const auto& atlas = pDecalAtlas->GetAtlas();
    const wdUInt32 decalIdx = atlas.m_Items.Find(wdHashingUtils::StringHashTo32(m_Decals[uiDecalIndex].GetResourceIDHash()));

    if (decalIdx != wdInvalidIndex)
    {
      const auto& item = atlas.m_Items.GetValue(decalIdx);
      uiDecalFlags = item.m_uiFlags;

      auto layerRectToScaleOffset = [](wdRectU32 layerRect, wdVec2U32 vTextureSize) {
        wdVec4 result;
        result.x = (float)layerRect.width / vTextureSize.x * 0.5f;
        result.y = (float)layerRect.height / vTextureSize.y * 0.5f;
        result.z = (float)layerRect.x / vTextureSize.x + result.x;
        result.w = (float)layerRect.y / vTextureSize.y + result.y;
        return result;
      };

      baseAtlasScaleOffset = layerRectToScaleOffset(item.m_LayerRects[0], pDecalAtlas->GetBaseColorTextureSize());
      normalAtlasScaleOffset = layerRectToScaleOffset(item.m_LayerRects[1], pDecalAtlas->GetNormalTextureSize());
      ormAtlasScaleOffset = layerRectToScaleOffset(item.m_LayerRects[2], pDecalAtlas->GetORMTextureSize());

      fAspectRatio = (float)item.m_LayerRects[0].width / item.m_LayerRects[0].height;
    }
  }

  auto pRenderData = wdCreateRenderDataForThisFrame<wdDecalRenderData>(GetOwner());

  wdUInt32 uiSortingId = (wdUInt32)(wdMath::Min(m_fSortOrder * 512.0f, 32767.0f) + 32768.0f);
  pRenderData->m_uiSortingKey = (uiSortingId << 16) | (m_uiInternalSortKey & 0xFFFF);

  const wdQuat axisRotation = wdBasisAxis::GetBasisRotation_PosX(m_ProjectionAxis);

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalTransform.m_vScale = (axisRotation * (pRenderData->m_GlobalTransform.m_vScale.CompMul(m_vExtents * 0.5f))).Abs();
  pRenderData->m_GlobalTransform.m_qRotation = pRenderData->m_GlobalTransform.m_qRotation * axisRotation;

  if (!wdMath::IsEqual(fAspectRatio, 1.0f, 0.001f))
  {
    if (fAspectRatio > 1.0f)
    {
      pRenderData->m_GlobalTransform.m_vScale.z /= fAspectRatio;
    }
    else
    {
      pRenderData->m_GlobalTransform.m_vScale.y *= fAspectRatio;
    }
  }

  pRenderData->m_uiApplyOnlyToId = m_uiApplyOnlyToId;
  pRenderData->m_uiFlags = uiDecalFlags;
  pRenderData->m_uiFlags |= (m_bWrapAround ? DECAL_WRAP_AROUND : 0);
  pRenderData->m_uiFlags |= (m_bMapNormalToGeometry ? DECAL_MAP_NORMAL_TO_GEOMETRY : 0);
  pRenderData->m_uiAngleFadeParams = wdShaderUtils::Float2ToRG16F(wdVec2(fFadeParamScale, fFadeParamOffset));
  pRenderData->m_BaseColor = finalColor;
  pRenderData->m_EmissiveColor = m_EmissiveColor;
  wdShaderUtils::Float4ToRGBA16F(baseAtlasScaleOffset, pRenderData->m_uiBaseColorAtlasScale, pRenderData->m_uiBaseColorAtlasOffset);
  wdShaderUtils::Float4ToRGBA16F(normalAtlasScaleOffset, pRenderData->m_uiNormalAtlasScale, pRenderData->m_uiNormalAtlasOffset);
  wdShaderUtils::Float4ToRGBA16F(ormAtlasScaleOffset, pRenderData->m_uiORMAtlasScale, pRenderData->m_uiORMAtlasOffset);

  wdRenderData::Caching::Enum caching = (m_FadeOutDelay.m_Value.GetSeconds() > 0.0 || m_FadeOutDuration.GetSeconds() > 0.0) ? wdRenderData::Caching::Never : wdRenderData::Caching::IfStatic;
  msg.AddRenderData(pRenderData, wdDefaultRenderDataCategories::Decal, caching);
}

void wdDecalComponent::SetApplyToRef(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  wdGameObjectHandle hTarget = resolver(szReference, GetHandle(), "ApplyTo");

  if (m_hApplyOnlyToObject == hTarget)
    return;

  m_hApplyOnlyToObject = hTarget;

  if (IsActiveAndInitialized())
  {
    UpdateApplyTo();
  }
}

void wdDecalComponent::UpdateApplyTo()
{
  wdUInt32 uiPrevId = m_uiApplyOnlyToId;

  m_uiApplyOnlyToId = 0;

  if (!m_hApplyOnlyToObject.IsInvalidated())
  {
    m_uiApplyOnlyToId = wdInvalidIndex;

    wdGameObject* pObject = nullptr;
    if (GetWorld()->TryGetObject(m_hApplyOnlyToObject, pObject))
    {
      wdRenderComponent* pRenderComponent = nullptr;
      if (pObject->TryGetComponentOfBaseType(pRenderComponent))
      {
        // this only works for dynamic objects, for static ones we must use ID 0
        if (pRenderComponent->GetOwner()->IsDynamic())
        {
          m_uiApplyOnlyToId = pRenderComponent->GetUniqueIdForRendering();
        }
      }
    }
  }

  if (uiPrevId != m_uiApplyOnlyToId && GetOwner()->IsStatic())
  {
    InvalidateCachedRenderData();
  }
}

static wdHashedString s_sSuicide = wdMakeHashedString("Suicide");

void wdDecalComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  wdWorld* pWorld = GetWorld();

  // no fade out -> fade out pretty late
  m_StartFadeOutTime = wdTime::Hours(24.0 * 365.0 * 100.0); // 100 years should be enough for everybody (ignoring leap years)

  if (m_FadeOutDelay.m_Value.GetSeconds() > 0.0 || m_FadeOutDuration.GetSeconds() > 0.0)
  {
    const wdTime tFadeOutDelay = wdTime::Seconds(pWorld->GetRandomNumberGenerator().DoubleVariance(m_FadeOutDelay.m_Value.GetSeconds(), m_FadeOutDelay.m_fVariance));
    m_StartFadeOutTime = pWorld->GetClock().GetAccumulatedTime() + tFadeOutDelay;

    if (m_OnFinishedAction != wdOnComponentFinishedAction::None)
    {
      wdMsgComponentInternalTrigger msg;
      msg.m_sMessage = s_sSuicide;

      const wdTime tKill = tFadeOutDelay + m_FadeOutDuration;

      PostMessage(msg, tKill);
    }
  }

  if (m_fSizeVariance > 0)
  {
    const float scale = (float)pWorld->GetRandomNumberGenerator().DoubleVariance(1.0, m_fSizeVariance);
    m_vExtents *= scale;

    TriggerLocalBoundsUpdate();

    InvalidateCachedRenderData();
  }
}

void wdDecalComponent::OnActivated()
{
  SUPER::OnActivated();

  m_uiInternalSortKey = GetOwner()->GetStableRandomSeed();
  m_uiInternalSortKey = (m_uiInternalSortKey >> 16) ^ (m_uiInternalSortKey & 0xFFFF);

  UpdateApplyTo();
}

void wdDecalComponent::OnTriggered(wdMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage != s_sSuicide)
    return;

  wdOnComponentFinishedAction::HandleFinishedAction(this, m_OnFinishedAction);
}

void wdDecalComponent::OnMsgDeleteGameObject(wdMsgDeleteGameObject& msg)
{
  wdOnComponentFinishedAction::HandleDeleteObjectMsg(msg, m_OnFinishedAction);
}

void wdDecalComponent::OnMsgOnlyApplyToObject(wdMsgOnlyApplyToObject& msg)
{
  SetApplyOnlyTo(msg.m_hObject);
}

void wdDecalComponent::OnMsgSetColor(wdMsgSetColor& msg)
{
  msg.ModifyColor(m_Color);
}

wdUInt32 wdDecalComponent::DecalFile_GetCount() const
{
  return m_Decals.GetCount();
}

const char* wdDecalComponent::DecalFile_Get(wdUInt32 uiIndex) const
{
  if (!m_Decals[uiIndex].IsValid())
    return "";

  return m_Decals[uiIndex].GetResourceID();
}

void wdDecalComponent::DecalFile_Set(wdUInt32 uiIndex, const char* szFile)
{
  wdDecalResourceHandle hResource;

  if (!wdStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = wdResourceManager::LoadResource<wdDecalResource>(szFile);
  }

  SetDecal(uiIndex, hResource);
}

void wdDecalComponent::DecalFile_Insert(wdUInt32 uiIndex, const char* szFile)
{
  m_Decals.Insert(wdDecalResourceHandle(), uiIndex);
  DecalFile_Set(uiIndex, szFile);
}

void wdDecalComponent::DecalFile_Remove(wdUInt32 uiIndex)
{
  m_Decals.RemoveAtAndCopy(uiIndex);
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class wdDecalComponent_6_7 : public wdGraphPatch
{
public:
  wdDecalComponent_6_7()
    : wdGraphPatch("wdDecalComponent", 7)
  {
  }

  virtual void Patch(wdGraphPatchContext& ref_context, wdAbstractObjectGraph* pGraph, wdAbstractObjectNode* pNode) const override
  {
    auto* pDecal = pNode->FindProperty("Decal");
    if (pDecal && pDecal->m_Value.IsA<wdString>())
    {
      wdVariantArray ar;
      ar.PushBack(pDecal->m_Value.Get<wdString>());
      pNode->AddProperty("Decals", ar);
    }
  }
};

wdDecalComponent_6_7 g_wdDecalComponent_6_7;

WD_STATICLINK_FILE(RendererCore, RendererCore_Decals_Implementation_DecalComponent);
