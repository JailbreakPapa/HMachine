#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/TagSet.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>

struct wdReflectionProbeMode
{
  using StorageType = wdUInt8;

  enum Enum
  {
    Static,
    Dynamic,

    Default = Static
  };
};
WD_DECLARE_REFLECTABLE_TYPE(WD_RENDERERCORE_DLL, wdReflectionProbeMode);

/// \brief Describes how a cube map should be generated.
struct WD_RENDERERCORE_DLL wdReflectionProbeDesc
{
  wdUuid m_uniqueID;

  wdTagSet m_IncludeTags;
  wdTagSet m_ExcludeTags;

  wdEnum<wdReflectionProbeMode> m_Mode;

  bool m_bShowDebugInfo = false;
  bool m_bShowMipMaps = false;

  float m_fIntensity = 1.0f;
  float m_fSaturation = 1.0f;
  float m_fNearPlane = 0.0f;
  float m_fFarPlane = 100.0f;
  wdVec3 m_vCaptureOffset = wdVec3::ZeroVector();
};

using wdReflectionProbeId = wdGenericId<24, 8>;

template <>
struct wdHashHelper<wdReflectionProbeId>
{
  WD_ALWAYS_INLINE static wdUInt32 Hash(wdReflectionProbeId value) { return wdHashHelper<wdUInt32>::Hash(value.m_Data); }

  WD_ALWAYS_INLINE static bool Equal(wdReflectionProbeId a, wdReflectionProbeId b) { return a == b; }
};

/// \brief Render data for a reflection probe.
class WD_RENDERERCORE_DLL wdReflectionProbeRenderData : public wdRenderData
{
  WD_ADD_DYNAMIC_REFLECTION(wdReflectionProbeRenderData, wdRenderData);

public:
  wdReflectionProbeRenderData()
  {
    m_Id.Invalidate();
    m_vHalfExtents.SetZero();
  }

  wdReflectionProbeId m_Id;
  wdUInt32 m_uiIndex = 0;
  wdVec3 m_vProbePosition; ///< Probe position in world space.
  wdVec3 m_vHalfExtents;
  wdVec3 m_vPositiveFalloff;
  wdVec3 m_vNegativeFalloff;
  wdVec3 m_vInfluenceScale;
  wdVec3 m_vInfluenceShift;
};

/// \brief A unique reference to a reflection probe.
struct wdReflectionProbeRef
{
  bool operator==(const wdReflectionProbeRef& b) const
  {
    return m_Id == b.m_Id && m_uiWorldIndex == b.m_uiWorldIndex;
  }

  wdUInt32 m_uiWorldIndex = 0;
  wdReflectionProbeId m_Id;
};
WD_CHECK_AT_COMPILETIME(sizeof(wdReflectionProbeRef) == 8);

template <>
struct wdHashHelper<wdReflectionProbeRef>
{
  WD_ALWAYS_INLINE static wdUInt32 Hash(wdReflectionProbeRef value) { return wdHashHelper<wdUInt64>::Hash(reinterpret_cast<wdUInt64&>(value)); }

  WD_ALWAYS_INLINE static bool Equal(wdReflectionProbeRef a, wdReflectionProbeRef b) { return a.m_Id == b.m_Id && a.m_uiWorldIndex == b.m_uiWorldIndex; }
};

/// \brief Flags that describe a reflection probe.
struct wdProbeFlags
{
  using StorageType = wdUInt8;

  enum Enum
  {
    SkyLight = WD_BIT(0),
    HasCustomCubeMap = WD_BIT(1),
    Sphere = WD_BIT(2),
    Box = WD_BIT(3),
    Dynamic = WD_BIT(4),
    Default = 0
  };

  struct Bits
  {
    StorageType SkyLight : 1;
    StorageType HasCustomCubeMap : 1;
    StorageType Sphere : 1;
    StorageType Box : 1;
    StorageType Dynamic : 1;
  };
};

WD_DECLARE_FLAGS_OPERATORS(wdProbeFlags);

WD_DECLARE_REFLECTABLE_TYPE(WD_RENDERERCORE_DLL, wdProbeFlags);
