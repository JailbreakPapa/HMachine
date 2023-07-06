#pragma once

#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Strings/String.h>
#include <Texture/TexConv/TexConvEnums.h>

struct WD_TEXTURE_DLL wdTextureAtlasCreationDesc
{
  struct Layer
  {
    wdEnum<wdTexConvUsage> m_Usage;
    wdUInt8 m_uiNumChannels = 4;
  };

  struct Item
  {
    wdUInt32 m_uiUniqueID;
    wdUInt32 m_uiFlags;
    wdString m_sAlphaInput;
    wdString m_sLayerInput[4];
  };

  wdHybridArray<Layer, 4> m_Layers;
  wdDynamicArray<Item> m_Items;

  wdResult Serialize(wdStreamWriter& inout_stream) const;
  wdResult Deserialize(wdStreamReader& inout_stream);

  wdResult Save(const char* szFile) const;
  wdResult Load(const char* szFile);
};

struct WD_TEXTURE_DLL wdTextureAtlasRuntimeDesc
{
  struct Item
  {
    wdUInt32 m_uiFlags;
    wdRectU32 m_LayerRects[4];
  };

  wdUInt32 m_uiNumLayers = 0;
  wdArrayMap<wdUInt32, Item> m_Items;

  void Clear();

  wdResult Serialize(wdStreamWriter& inout_stream) const;
  wdResult Deserialize(wdStreamReader& inout_stream);
};
