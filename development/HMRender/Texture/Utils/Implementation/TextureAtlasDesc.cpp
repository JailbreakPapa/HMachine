#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Texture/Utils/TextureAtlasDesc.h>

wdResult wdTextureAtlasCreationDesc::Serialize(wdStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(3);

  if (m_Layers.GetCount() > 255u)
    return WD_FAILURE;

  const wdUInt8 uiNumLayers = static_cast<wdUInt8>(m_Layers.GetCount());
  inout_stream << uiNumLayers;

  for (wdUInt32 l = 0; l < uiNumLayers; ++l)
  {
    inout_stream << m_Layers[l].m_Usage;
    inout_stream << m_Layers[l].m_uiNumChannels;
  }

  inout_stream << m_Items.GetCount();
  for (auto& item : m_Items)
  {
    inout_stream << item.m_uiUniqueID;
    inout_stream << item.m_uiFlags;

    for (wdUInt32 l = 0; l < uiNumLayers; ++l)
    {
      inout_stream << item.m_sLayerInput[l];
    }

    inout_stream << item.m_sAlphaInput;
  }

  return WD_SUCCESS;
}

wdResult wdTextureAtlasCreationDesc::Deserialize(wdStreamReader& inout_stream)
{
  const wdTypeVersion uiVersion = inout_stream.ReadVersion(3);

  wdUInt8 uiNumLayers = 0;
  inout_stream >> uiNumLayers;

  m_Layers.SetCount(uiNumLayers);

  for (wdUInt32 l = 0; l < uiNumLayers; ++l)
  {
    inout_stream >> m_Layers[l].m_Usage;
    inout_stream >> m_Layers[l].m_uiNumChannels;
  }

  wdUInt32 uiNumItems = 0;
  inout_stream >> uiNumItems;
  m_Items.SetCount(uiNumItems);

  for (auto& item : m_Items)
  {
    inout_stream >> item.m_uiUniqueID;
    inout_stream >> item.m_uiFlags;

    for (wdUInt32 l = 0; l < uiNumLayers; ++l)
    {
      inout_stream >> item.m_sLayerInput[l];
    }

    if (uiVersion >= 3)
    {
      inout_stream >> item.m_sAlphaInput;
    }
  }

  return WD_SUCCESS;
}

wdResult wdTextureAtlasCreationDesc::Save(const char* szFile) const
{
  wdFileWriter file;
  WD_SUCCEED_OR_RETURN(file.Open(szFile));

  return Serialize(file);
}

wdResult wdTextureAtlasCreationDesc::Load(const char* szFile)
{
  wdFileReader file;
  WD_SUCCEED_OR_RETURN(file.Open(szFile));

  return Deserialize(file);
}

void wdTextureAtlasRuntimeDesc::Clear()
{
  m_uiNumLayers = 0;
  m_Items.Clear();
}

wdResult wdTextureAtlasRuntimeDesc::Serialize(wdStreamWriter& inout_stream) const
{
  m_Items.Sort();

  inout_stream << m_uiNumLayers;
  inout_stream << m_Items.GetCount();

  for (wdUInt32 i = 0; i < m_Items.GetCount(); ++i)
  {
    inout_stream << m_Items.GetKey(i);
    inout_stream << m_Items.GetValue(i).m_uiFlags;

    for (wdUInt32 l = 0; l < m_uiNumLayers; ++l)
    {
      const auto& r = m_Items.GetValue(i).m_LayerRects[l];
      inout_stream << r.x;
      inout_stream << r.y;
      inout_stream << r.width;
      inout_stream << r.height;
    }
  }

  return WD_SUCCESS;
}

wdResult wdTextureAtlasRuntimeDesc::Deserialize(wdStreamReader& inout_stream)
{
  Clear();

  inout_stream >> m_uiNumLayers;

  wdUInt32 uiNumItems = 0;
  inout_stream >> uiNumItems;
  m_Items.Reserve(uiNumItems);

  for (wdUInt32 i = 0; i < uiNumItems; ++i)
  {
    wdUInt32 key = 0;
    inout_stream >> key;

    auto& item = m_Items[key];
    inout_stream >> item.m_uiFlags;

    for (wdUInt32 l = 0; l < m_uiNumLayers; ++l)
    {
      auto& r = item.m_LayerRects[l];
      inout_stream >> r.x;
      inout_stream >> r.y;
      inout_stream >> r.width;
      inout_stream >> r.height;
    }
  }

  m_Items.Sort();
  return WD_SUCCESS;
}

WD_STATICLINK_FILE(Texture, Texture_Utils_Implementation_TextureAtlasDesc);
