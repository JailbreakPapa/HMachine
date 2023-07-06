#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec2.h>
#include <Texture/TextureDLL.h>

class WD_TEXTURE_DLL wdTexturePacker
{
public:
  struct Texture
  {
    WD_DECLARE_POD_TYPE();

    wdVec2U32 m_Size;
    wdVec2U32 m_Position;
    wdInt32 m_Priority = 0;
  };

  wdTexturePacker();
  ~wdTexturePacker();

  void SetTextureSize(wdUInt32 uiWidth, wdUInt32 uiHeight, wdUInt32 uiReserveTextures = 0);

  void AddTexture(wdUInt32 uiWidth, wdUInt32 uiHeight);

  const wdDynamicArray<Texture>& GetTextures() const { return m_Textures; }

  wdResult PackTextures();

private:
  bool CanPlaceAt(wdVec2U32 pos, wdVec2U32 size);
  bool TryPlaceAt(wdVec2U32 pos, wdVec2U32 size);
  wdUInt32 PosToIndex(wdUInt32 x, wdUInt32 y) const;
  bool TryPlaceTexture(wdUInt32 idx);

  wdUInt32 m_uiWidth = 0;
  wdUInt32 m_uiHeight = 0;

  wdDynamicArray<Texture> m_Textures;
  wdDynamicArray<bool> m_Grid;
};
