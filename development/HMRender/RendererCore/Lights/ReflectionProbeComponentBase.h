#pragma once

#include <Core/World/Component.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>

struct wdMsgUpdateLocalBounds;
struct wdMsgExtractRenderData;
struct wdMsgTransformChanged;
class wdAbstractObjectNode;

/// \brief Base class for all reflection probes.
class WD_RENDERERCORE_DLL wdReflectionProbeComponentBase : public wdComponent
{
  WD_ADD_DYNAMIC_REFLECTION(wdReflectionProbeComponentBase, wdComponent);

public:
  wdReflectionProbeComponentBase();
  ~wdReflectionProbeComponentBase();

  void SetReflectionProbeMode(wdEnum<wdReflectionProbeMode> mode); // [ property ]
  wdEnum<wdReflectionProbeMode> GetReflectionProbeMode() const;    // [ property ]

  const wdTagSet& GetIncludeTags() const;   // [ property ]
  void InsertIncludeTag(const char* szTag); // [ property ]
  void RemoveIncludeTag(const char* szTag); // [ property ]

  const wdTagSet& GetExcludeTags() const;   // [ property ]
  void InsertExcludeTag(const char* szTag); // [ property ]
  void RemoveExcludeTag(const char* szTag); // [ property ]

  float GetNearPlane() const { return m_Desc.m_fNearPlane; } // [ property ]
  void SetNearPlane(float fNearPlane);                       // [ property ]

  float GetFarPlane() const { return m_Desc.m_fFarPlane; } // [ property ]
  void SetFarPlane(float fFarPlane);                       // [ property ]

  const wdVec3& GetCaptureOffset() const { return m_Desc.m_vCaptureOffset; } // [ property ]
  void SetCaptureOffset(const wdVec3& vOffset);                              // [ property ]

  void SetShowDebugInfo(bool bShowDebugInfo); // [ property ]
  bool GetShowDebugInfo() const;              // [ property ]

  void SetShowMipMaps(bool bShowMipMaps); // [ property ]
  bool GetShowMipMaps() const;            // [ property ]

  //////////////////////////////////////////////////////////////////////////
  // wdComponent

public:
  virtual void SerializeComponent(wdWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(wdWorldReader& inout_stream) override;

protected:
  float ComputePriority(wdMsgExtractRenderData& msg, wdReflectionProbeRenderData* pRenderData, float fVolume, const wdVec3& vScale) const;

protected:
  wdReflectionProbeDesc m_Desc;

  wdReflectionProbeId m_Id;
  // Set to true if a change was made that requires recomputing the cube map.
  mutable bool m_bStatesDirty = true;
};
