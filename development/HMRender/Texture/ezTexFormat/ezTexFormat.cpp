#include <Texture/TexturePCH.h>

#include <Foundation/IO/Stream.h>
#include <Texture/ezTexFormat/ezTexFormat.h>

void wdTexFormat::WriteTextureHeader(wdStreamWriter& inout_stream) const
{
  wdUInt8 uiFileFormatVersion = 2;
  inout_stream << uiFileFormatVersion;

  inout_stream << m_bSRGB;
  inout_stream << m_AddressModeU;
  inout_stream << m_AddressModeV;
  inout_stream << m_AddressModeW;
  inout_stream << m_TextureFilter;
}

void wdTexFormat::WriteRenderTargetHeader(wdStreamWriter& inout_stream) const
{
  wdUInt8 uiFileFormatVersion = 5;
  inout_stream << uiFileFormatVersion;

  // version 2
  inout_stream << m_bSRGB;
  inout_stream << m_AddressModeU;
  inout_stream << m_AddressModeV;
  inout_stream << m_AddressModeW;
  inout_stream << m_TextureFilter;

  // version 3
  inout_stream << m_iRenderTargetResolutionX;
  inout_stream << m_iRenderTargetResolutionY;

  // version 4
  inout_stream << m_fResolutionScale;

  // version 5
  inout_stream << m_GalRenderTargetFormat;
}

void wdTexFormat::ReadHeader(wdStreamReader& inout_stream)
{
  wdUInt8 uiFileFormatVersion = 0;
  inout_stream >> uiFileFormatVersion;

  // version 2
  if (uiFileFormatVersion >= 2)
  {
    inout_stream >> m_bSRGB;
    inout_stream >> m_AddressModeU;
    inout_stream >> m_AddressModeV;
    inout_stream >> m_AddressModeW;
    inout_stream >> m_TextureFilter;
  }

  // version 3
  if (uiFileFormatVersion >= 3)
  {
    inout_stream >> m_iRenderTargetResolutionX;
    inout_stream >> m_iRenderTargetResolutionY;
  }

  // version 4
  if (uiFileFormatVersion >= 4)
  {
    inout_stream >> m_fResolutionScale;
  }

  // version 5
  if (uiFileFormatVersion >= 5)
  {
    inout_stream >> m_GalRenderTargetFormat;
  }
}



WD_STATICLINK_FILE(Texture, Texture_wdTexFormat_wdTexFormat);
