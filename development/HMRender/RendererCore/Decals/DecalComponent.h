#pragma once

#include <Foundation/Math/Color16f.h>
#include <Foundation/Types/VarianceTypes.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>

class wdAbstractObjectNode;
struct wdMsgComponentInternalTrigger;
struct wdMsgOnlyApplyToObject;
struct wdMsgSetColor;

class WD_RENDERERCORE_DLL wdDecalComponentManager final : public wdComponentManager<class wdDecalComponent, wdBlockStorageType::Compact>
{
public:
  wdDecalComponentManager(wdWorld* pWorld);

  virtual void Initialize() override;

private:
  friend class wdDecalComponent;
  wdDecalAtlasResourceHandle m_hDecalAtlas;
};

class WD_RENDERERCORE_DLL wdDecalRenderData : public wdRenderData
{
  WD_ADD_DYNAMIC_REFLECTION(wdDecalRenderData, wdRenderData);

public:
  wdUInt32 m_uiApplyOnlyToId;
  wdUInt32 m_uiFlags;
  wdUInt32 m_uiAngleFadeParams;

  wdColorLinearUB m_BaseColor;
  wdColorLinear16f m_EmissiveColor;

  wdUInt32 m_uiBaseColorAtlasScale;
  wdUInt32 m_uiBaseColorAtlasOffset;

  wdUInt32 m_uiNormalAtlasScale;
  wdUInt32 m_uiNormalAtlasOffset;

  wdUInt32 m_uiORMAtlasScale;
  wdUInt32 m_uiORMAtlasOffset;
};

class WD_RENDERERCORE_DLL wdDecalComponent final : public wdRenderComponent
{
  WD_DECLARE_COMPONENT_TYPE(wdDecalComponent, wdRenderComponent, wdDecalComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // wdComponent

public:
  virtual void SerializeComponent(wdWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(wdWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnActivated() override;

  //////////////////////////////////////////////////////////////////////////
  // wdRenderComponent

protected:
  virtual wdResult GetLocalBounds(wdBoundingBoxSphere& bounds, bool& bAlwaysVisible, wdMsgUpdateLocalBounds& msg) override;
  void OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const;


  //////////////////////////////////////////////////////////////////////////
  // wdDecalComponent

public:
  wdDecalComponent();
  ~wdDecalComponent();

  void SetExtents(const wdVec3& value); // [ property ]
  const wdVec3& GetExtents() const;     // [ property ]

  void SetSizeVariance(float fVariance); // [ property ]
  float GetSizeVariance() const;         // [ property ]

  void SetColor(wdColorGammaUB color); // [ property ]
  wdColorGammaUB GetColor() const;     // [ property ]

  void SetEmissiveColor(wdColor color); // [ property ]
  wdColor GetEmissiveColor() const;     // [ property ]

  void SetInnerFadeAngle(wdAngle fadeAngle);  // [ property ]
  wdAngle GetInnerFadeAngle() const;          // [ property ]

  void SetOuterFadeAngle(wdAngle fadeAngle);  // [ property ]
  wdAngle GetOuterFadeAngle() const;          // [ property ]

  void SetSortOrder(float fOrder); // [ property ]
  float GetSortOrder() const;      // [ property ]

  void SetWrapAround(bool bWrapAround); // [ property ]
  bool GetWrapAround() const;           // [ property ]

  void SetMapNormalToGeometry(bool bMapNormal); // [ property ]
  bool GetMapNormalToGeometry() const;          // [ property ]

  void SetDecal(wdUInt32 uiIndex, const wdDecalResourceHandle& hResource); // [ property ]
  const wdDecalResourceHandle& GetDecal(wdUInt32 uiIndex) const;           // [ property ]

  wdVarianceTypeTime m_FadeOutDelay;                      // [ property ]
  wdTime m_FadeOutDuration;                               // [ property ]
  wdEnum<wdOnComponentFinishedAction> m_OnFinishedAction; // [ property ]

  void SetProjectionAxis(wdEnum<wdBasisAxis> projectionAxis); // [ property ]
  wdEnum<wdBasisAxis> GetProjectionAxis() const;              // [ property ]

  void SetApplyOnlyTo(wdGameObjectHandle hObject);
  wdGameObjectHandle GetApplyOnlyTo() const;

  wdUInt32 DecalFile_GetCount() const;                         // [ property ]
  const char* DecalFile_Get(wdUInt32 uiIndex) const;           // [ property ]
  void DecalFile_Set(wdUInt32 uiIndex, const char* szFile);    // [ property ]
  void DecalFile_Insert(wdUInt32 uiIndex, const char* szFile); // [ property ]
  void DecalFile_Remove(wdUInt32 uiIndex);                     // [ property ]


protected:
  void SetApplyToRef(const char* szReference); // [ property ]
  void UpdateApplyTo();

  void OnTriggered(wdMsgComponentInternalTrigger& msg);
  void OnMsgDeleteGameObject(wdMsgDeleteGameObject& msg);
  void OnMsgOnlyApplyToObject(wdMsgOnlyApplyToObject& msg);
  void OnMsgSetColor(wdMsgSetColor& msg);

  wdVec3 m_vExtents = wdVec3(1.0f);
  float m_fSizeVariance = 0;
  wdColorGammaUB m_Color = wdColor::White;
  wdColor m_EmissiveColor = wdColor::Black;
  wdAngle m_InnerFadeAngle = wdAngle::Degree(50.0f);
  wdAngle m_OuterFadeAngle = wdAngle::Degree(80.0f);
  float m_fSortOrder = 0;
  bool m_bWrapAround = false;
  bool m_bMapNormalToGeometry = false;
  wdUInt8 m_uiRandomDecalIdx = 0xFF;
  wdEnum<wdBasisAxis> m_ProjectionAxis;
  wdHybridArray<wdDecalResourceHandle, 1> m_Decals;

  wdGameObjectHandle m_hApplyOnlyToObject;
  wdUInt32 m_uiApplyOnlyToId = 0;

  wdTime m_StartFadeOutTime;
  wdUInt32 m_uiInternalSortKey = 0;

private:
  const char* DummyGetter() const { return nullptr; }
};
