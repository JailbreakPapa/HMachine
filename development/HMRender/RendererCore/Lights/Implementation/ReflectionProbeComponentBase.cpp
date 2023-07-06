#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/ReflectionProbeComponentBase.h>

#include <Core/Graphics/Camera.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

namespace
{
  static wdVariantArray GetDefaultExcludeTags()
  {
    wdVariantArray value(wdStaticAllocatorWrapper::GetAllocator());
    value.PushBack(wdStringView("SkyLight"));
    return value;
  }
} // namespace


// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdReflectionProbeComponentBase, 2, wdRTTINoAllocator)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ENUM_ACCESSOR_PROPERTY("ReflectionProbeMode", wdReflectionProbeMode, GetReflectionProbeMode, SetReflectionProbeMode)->AddAttributes(new wdDefaultValueAttribute(wdReflectionProbeMode::Static), new wdGroupAttribute("Capture Description")),
    WD_SET_ACCESSOR_PROPERTY("IncludeTags", GetIncludeTags, InsertIncludeTag, RemoveIncludeTag)->AddAttributes(new wdTagSetWidgetAttribute("Default")),
    WD_SET_ACCESSOR_PROPERTY("ExcludeTags", GetExcludeTags, InsertExcludeTag, RemoveExcludeTag)->AddAttributes(new wdTagSetWidgetAttribute("Default"), new wdDefaultValueAttribute(GetDefaultExcludeTags())),
    WD_ACCESSOR_PROPERTY("NearPlane", GetNearPlane, SetNearPlane)->AddAttributes(new wdDefaultValueAttribute(0.0f), new wdClampValueAttribute(0.0f, {}), new wdMinValueTextAttribute("Auto")),
    WD_ACCESSOR_PROPERTY("FarPlane", GetFarPlane, SetFarPlane)->AddAttributes(new wdDefaultValueAttribute(100.0f), new wdClampValueAttribute(0.01f, 10000.0f)),
    WD_ACCESSOR_PROPERTY("CaptureOffset", GetCaptureOffset, SetCaptureOffset),
    WD_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    WD_ACCESSOR_PROPERTY("ShowMipMaps", GetShowMipMaps, SetShowMipMaps),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdTransformManipulatorAttribute("CaptureOffset"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdReflectionProbeComponentBase::wdReflectionProbeComponentBase()
{
  m_Desc.m_uniqueID.CreateNewUuid();
}

wdReflectionProbeComponentBase::~wdReflectionProbeComponentBase() = default;

void wdReflectionProbeComponentBase::SetReflectionProbeMode(wdEnum<wdReflectionProbeMode> mode)
{
  m_Desc.m_Mode = mode;
  m_bStatesDirty = true;
}

wdEnum<wdReflectionProbeMode> wdReflectionProbeComponentBase::GetReflectionProbeMode() const
{
  return m_Desc.m_Mode;
}

const wdTagSet& wdReflectionProbeComponentBase::GetIncludeTags() const
{
  return m_Desc.m_IncludeTags;
}

void wdReflectionProbeComponentBase::InsertIncludeTag(const char* szTag)
{
  m_Desc.m_IncludeTags.SetByName(szTag);
  m_bStatesDirty = true;
}

void wdReflectionProbeComponentBase::RemoveIncludeTag(const char* szTag)
{
  m_Desc.m_IncludeTags.RemoveByName(szTag);
  m_bStatesDirty = true;
}


const wdTagSet& wdReflectionProbeComponentBase::GetExcludeTags() const
{
  return m_Desc.m_ExcludeTags;
}

void wdReflectionProbeComponentBase::InsertExcludeTag(const char* szTag)
{
  m_Desc.m_ExcludeTags.SetByName(szTag);
  m_bStatesDirty = true;
}

void wdReflectionProbeComponentBase::RemoveExcludeTag(const char* szTag)
{
  m_Desc.m_ExcludeTags.RemoveByName(szTag);
  m_bStatesDirty = true;
}

void wdReflectionProbeComponentBase::SetNearPlane(float fNearPlane)
{
  m_Desc.m_fNearPlane = fNearPlane;
  m_bStatesDirty = true;
}

void wdReflectionProbeComponentBase::SetFarPlane(float fFarPlane)
{
  m_Desc.m_fFarPlane = fFarPlane;
  m_bStatesDirty = true;
}

void wdReflectionProbeComponentBase::SetCaptureOffset(const wdVec3& vOffset)
{
  m_Desc.m_vCaptureOffset = vOffset;
  m_bStatesDirty = true;
}

void wdReflectionProbeComponentBase::SetShowDebugInfo(bool bShowDebugInfo)
{
  m_Desc.m_bShowDebugInfo = bShowDebugInfo;
  m_bStatesDirty = true;
}

bool wdReflectionProbeComponentBase::GetShowDebugInfo() const
{
  return m_Desc.m_bShowDebugInfo;
}

void wdReflectionProbeComponentBase::SetShowMipMaps(bool bShowMipMaps)
{
  m_Desc.m_bShowMipMaps = bShowMipMaps;
  m_bStatesDirty = true;
}

bool wdReflectionProbeComponentBase::GetShowMipMaps() const
{
  return m_Desc.m_bShowMipMaps;
}

void wdReflectionProbeComponentBase::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  wdStreamWriter& s = inout_stream.GetStream();

  m_Desc.m_IncludeTags.Save(s);
  m_Desc.m_ExcludeTags.Save(s);
  s << m_Desc.m_Mode;
  s << m_Desc.m_bShowDebugInfo;
  s << m_Desc.m_uniqueID;
  s << m_Desc.m_fNearPlane;
  s << m_Desc.m_fFarPlane;
  s << m_Desc.m_vCaptureOffset;
}

void wdReflectionProbeComponentBase::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  //const wdUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  wdStreamReader& s = inout_stream.GetStream();

  m_Desc.m_IncludeTags.Load(s, wdTagRegistry::GetGlobalRegistry());
  m_Desc.m_ExcludeTags.Load(s, wdTagRegistry::GetGlobalRegistry());
  s >> m_Desc.m_Mode;
  s >> m_Desc.m_bShowDebugInfo;
  s >> m_Desc.m_uniqueID;
  s >> m_Desc.m_fNearPlane;
  s >> m_Desc.m_fFarPlane;
  s >> m_Desc.m_vCaptureOffset;
}

float wdReflectionProbeComponentBase::ComputePriority(wdMsgExtractRenderData& msg, wdReflectionProbeRenderData* pRenderData, float fVolume, const wdVec3& vScale) const
{
  float fPriority = 0.0f;
  const float fLogVolume = wdMath::Log2(1.0f + fVolume); // +1 to make sure it never goes negative.
  // This sorting is only by size to make sure the probes in a cluster are iterating from smallest to largest on the GPU. Which probes are actually used is determined below by the returned priority.
  pRenderData->m_uiSortingKey = wdMath::FloatToInt(static_cast<float>(wdMath::MaxValue<wdUInt32>()) * fLogVolume / 40.0f);

  //#TODO This is a pretty poor distance / size based score.
  if (msg.m_pView)
  {
    if (auto pCamera = msg.m_pView->GetLodCamera())
    {
      float fDistance = (pCamera->GetPosition() - pRenderData->m_GlobalTransform.m_vPosition).GetLength();
      float fRadius = (wdMath::Abs(vScale.x) + wdMath::Abs(vScale.y) + wdMath::Abs(vScale.z)) / 3.0f;
      fPriority = fRadius / fDistance;
    }
  }

#ifdef WD_SHOW_REFLECTION_PROBE_PRIORITIES
  wdStringBuilder s;
  s.Format("{}, {}", pRenderData->m_uiSortingKey, fPriority);
  wdDebugRenderer::Draw3DText(GetWorld(), s, pRenderData->m_GlobalTransform.m_vPosition, wdColor::Wheat);
#endif
  return fPriority;
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ReflectionProbeComponentBase);
