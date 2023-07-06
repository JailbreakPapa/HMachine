#pragma once

#include <RendererCore/RenderContext/Implementation/RenderContextStructs.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>
#include <Texture/Image/Image.h>

struct WD_RENDERERCORE_DLL wdTextureUtils
{
  static wdGALResourceFormat::Enum ImageFormatToGalFormat(wdImageFormat::Enum format, bool bSRGB);
  static wdImageFormat::Enum GalFormatToImageFormat(wdGALResourceFormat::Enum format, bool bRemoveSRGB);
  static wdImageFormat::Enum GalFormatToImageFormat(wdGALResourceFormat::Enum format);


  static void ConfigureSampler(wdTextureFilterSetting::Enum filter, wdGALSamplerStateCreationDescription& out_sampler);

  /// \brief If enabled, textures are always loaded to full quality immediately. Mostly necessary for image comparison unit tests.
  static bool s_bForceFullQualityAlways;
};
