#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Types/Enum.h>
#include <Texture/Image/ImageEnums.h>
#include <Texture/TexConv/TexConvEnums.h>

class wdStreamWriter;
class wdStreamReader;

struct WD_TEXTURE_DLL wdTexFormat
{
  bool m_bSRGB = false;
  wdEnum<wdImageAddressMode> m_AddressModeU;
  wdEnum<wdImageAddressMode> m_AddressModeV;
  wdEnum<wdImageAddressMode> m_AddressModeW;

  // version 2
  wdEnum<wdTextureFilterSetting> m_TextureFilter;

  // version 3
  wdInt16 m_iRenderTargetResolutionX = 0;
  wdInt16 m_iRenderTargetResolutionY = 0;

  // version 4
  float m_fResolutionScale = 1.0f;

  // version 5
  int m_GalRenderTargetFormat = 0;

  void WriteTextureHeader(wdStreamWriter& inout_stream) const;
  void WriteRenderTargetHeader(wdStreamWriter& inout_stream) const;
  void ReadHeader(wdStreamReader& inout_stream);
};
