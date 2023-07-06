#include <Texture/TexturePCH.h>

#include <Texture/Utils/TexturePacker.h>

wdTexturePacker::wdTexturePacker() {}

wdTexturePacker::~wdTexturePacker() {}

void wdTexturePacker::SetTextureSize(wdUInt32 uiWidth, wdUInt32 uiHeight, wdUInt32 uiReserveTextures /*= 0*/)
{
  m_uiWidth = uiWidth;
  m_uiHeight = uiHeight;

  m_Textures.Clear();
  m_Textures.Reserve(uiReserveTextures);

  // initializes all values to false
  m_Grid.Clear();
  m_Grid.SetCount(m_uiWidth * m_uiHeight);
}

void wdTexturePacker::AddTexture(wdUInt32 uiWidth, wdUInt32 uiHeight)
{
  Texture& tex = m_Textures.ExpandAndGetRef();
  tex.m_Size.x = uiWidth;
  tex.m_Size.y = uiHeight;
  tex.m_Priority = 2 * uiWidth + 2 * uiHeight;
}

struct sortdata
{
  WD_DECLARE_POD_TYPE();

  wdInt32 m_Priority;
  wdInt32 m_Index;
};

wdResult wdTexturePacker::PackTextures()
{
  wdDynamicArray<sortdata> sorted;
  sorted.SetCountUninitialized(m_Textures.GetCount());

  for (wdUInt32 i = 0; i < m_Textures.GetCount(); ++i)
  {
    sorted[i].m_Index = i;
    sorted[i].m_Priority = m_Textures[i].m_Priority;
  }

  sorted.Sort([](const sortdata& lhs, const sortdata& rhs) -> bool { return lhs.m_Priority > rhs.m_Priority; });

  for (wdUInt32 idx = 0; idx < sorted.GetCount(); ++idx)
  {
    if (!TryPlaceTexture(sorted[idx].m_Index))
      return WD_FAILURE;
  }

  return WD_SUCCESS;
}

wdUInt32 wdTexturePacker::PosToIndex(wdUInt32 x, wdUInt32 y) const
{
  return (y * m_uiWidth + x);
}

bool wdTexturePacker::TryPlaceTexture(wdUInt32 idx)
{
  Texture& tex = m_Textures[idx];

  for (wdUInt32 y = 0; y < m_uiHeight; ++y)
  {
    for (wdUInt32 x = 0; x < m_uiWidth; ++x)
    {
      if (!TryPlaceAt(wdVec2U32(x, y), tex.m_Size))
        continue;

      tex.m_Position.Set(x, y);
      return true;
    }
  }

  return false;
}

bool wdTexturePacker::CanPlaceAt(wdVec2U32 pos, wdVec2U32 size)
{
  if (pos.x + size.x > m_uiWidth)
    return false;
  if (pos.y + size.y > m_uiHeight)
    return false;

  for (wdUInt32 y = 0; y < size.y; ++y)
  {
    for (wdUInt32 x = 0; x < size.x; ++x)
    {
      if (m_Grid[PosToIndex(pos.x + x, pos.y + y)])
        return false;
    }
  }

  return true;
}

bool wdTexturePacker::TryPlaceAt(wdVec2U32 pos, wdVec2U32 size)
{
  if (!CanPlaceAt(pos, size))
    return false;

  for (wdUInt32 y = 0; y < size.y; ++y)
  {
    for (wdUInt32 x = 0; x < size.x; ++x)
    {
      m_Grid[PosToIndex(pos.x + x, pos.y + y)] = true;
    }
  }

  return true;
}



WD_STATICLINK_FILE(Texture, Texture_Utils_Implementation_TexturePacker);
