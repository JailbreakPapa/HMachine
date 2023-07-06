#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

struct wdMsgSetColor;
using wdTexture2DResourceHandle = wdTypedResourceHandle<class wdTexture2DResource>;

struct wdSpriteBlendMode
{
  using StorageType = wdUInt8;

  enum Enum
  {
    Masked,
    Transparent,
    Additive,

    Default = Masked
  };

  static wdTempHashedString GetPermutationValue(Enum blendMode);
};

WD_DECLARE_REFLECTABLE_TYPE(WD_RENDERERCORE_DLL, wdSpriteBlendMode);

class WD_RENDERERCORE_DLL wdSpriteRenderData : public wdRenderData
{
  WD_ADD_DYNAMIC_REFLECTION(wdSpriteRenderData, wdRenderData);

public:
  void FillBatchIdAndSortingKey();

  wdTexture2DResourceHandle m_hTexture;

  float m_fSize;
  float m_fMaxScreenSize;
  float m_fAspectRatio;
  wdEnum<wdSpriteBlendMode> m_BlendMode;

  wdColor m_color;

  wdVec2 m_texCoordScale;
  wdVec2 m_texCoordOffset;

  wdUInt32 m_uiUniqueID;
};

using wdSpriteComponentManager = wdComponentManager<class wdSpriteComponent, wdBlockStorageType::Compact>;

class WD_RENDERERCORE_DLL wdSpriteComponent : public wdRenderComponent
{
  WD_DECLARE_COMPONENT_TYPE(wdSpriteComponent, wdRenderComponent, wdSpriteComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // wdComponent

public:
  virtual void SerializeComponent(wdWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(wdWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // wdRenderComponent

public:
  virtual wdResult GetLocalBounds(wdBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, wdMsgUpdateLocalBounds& ref_msg) override;


  //////////////////////////////////////////////////////////////////////////
  // wdSpriteComponent

public:
  wdSpriteComponent();
  ~wdSpriteComponent();

  void SetTexture(const wdTexture2DResourceHandle& hTexture);
  const wdTexture2DResourceHandle& GetTexture() const;

  void SetTextureFile(const char* szFile); // [ property ]
  const char* GetTextureFile() const;      // [ property ]

  void SetColor(wdColor color); // [ property ]
  wdColor GetColor() const;     // [ property ]

  void SetSize(float fSize); // [ property ]
  float GetSize() const;     // [ property ]

  void SetMaxScreenSize(float fSize); // [ property ]
  float GetMaxScreenSize() const;     // [ property ]

  void OnMsgSetColor(wdMsgSetColor& ref_msg); // [ property ]

private:
  void OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const;

  wdTexture2DResourceHandle m_hTexture;
  wdEnum<wdSpriteBlendMode> m_BlendMode;
  wdColor m_Color = wdColor::White;

  float m_fSize = 1.0f;
  float m_fMaxScreenSize = 64.0f;
  float m_fAspectRatio = 1.0f;
};
