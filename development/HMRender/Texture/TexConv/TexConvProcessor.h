#pragma once

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Math/Rect.h>
#include <Texture/TexConv/TexConvDesc.h>

struct wdTextureAtlasCreationDesc;

class WD_TEXTURE_DLL wdTexConvProcessor
{
  WD_DISALLOW_COPY_AND_ASSIGN(wdTexConvProcessor);

public:
  wdTexConvProcessor();

  wdTexConvDesc m_Descriptor;

  wdResult Process();

  wdImage m_OutputImage;
  wdImage m_LowResOutputImage;
  wdImage m_ThumbnailOutputImage;
  wdDefaultMemoryStreamStorage m_TextureAtlas;

private:
  //////////////////////////////////////////////////////////////////////////
  // Modifying the Descriptor

  wdResult LoadInputImages();
  wdResult ForceSRGBFormats();
  wdResult ConvertAndScaleInputImages(wdUInt32 uiResolutionX, wdUInt32 uiResolutionY, wdEnum<wdTexConvUsage> usage);
  wdResult ConvertToNormalMap(wdImage& bumpMap) const;
  wdResult ConvertToNormalMap(wdArrayPtr<wdImage> bumpMap) const;
  wdResult ClampInputValues(wdArrayPtr<wdImage> images, float maxValue) const;
  wdResult ClampInputValues(wdImage& image, float maxValue) const;
  wdResult DetectNumChannels(wdArrayPtr<const wdTexConvSliceChannelMapping> channelMapping, wdUInt32& uiNumChannels);

  //////////////////////////////////////////////////////////////////////////
  // Reading from the descriptor

  enum class MipmapChannelMode
  {
    AllChannels,
    SingleChannel
  };

  wdResult ChooseOutputFormat(wdEnum<wdImageFormat>& out_Format, wdEnum<wdTexConvUsage> usage, wdUInt32 uiNumChannels) const;
  wdResult DetermineTargetResolution(
    const wdImage& image, wdEnum<wdImageFormat> OutputImageFormat, wdUInt32& out_uiTargetResolutionX, wdUInt32& out_uiTargetResolutionY) const;
  wdResult Assemble2DTexture(const wdImageHeader& refImg, wdImage& dst) const;
  wdResult AssembleCubemap(wdImage& dst) const;
  wdResult Assemble3DTexture(wdImage& dst) const;
  wdResult AdjustHdrExposure(wdImage& img) const;
  wdResult PremultiplyAlpha(wdImage& image) const;
  wdResult DilateColor2D(wdImage& img) const;
  wdResult Assemble2DSlice(const wdTexConvSliceChannelMapping& mapping, wdUInt32 uiResolutionX, wdUInt32 uiResolutionY, wdColor* pPixelOut) const;
  wdResult GenerateMipmaps(wdImage& img, wdUInt32 uiNumMips /* =0 */, MipmapChannelMode channelMode = MipmapChannelMode::AllChannels) const;

  //////////////////////////////////////////////////////////////////////////
  // Purely functional
  static wdResult AdjustUsage(const char* szFilename, const wdImage& srcImg, wdEnum<wdTexConvUsage>& inout_Usage);
  static wdResult ConvertAndScaleImage(const char* szImageName, wdImage& inout_Image, wdUInt32 uiResolutionX, wdUInt32 uiResolutionY, wdEnum<wdTexConvUsage> usage);

  //////////////////////////////////////////////////////////////////////////
  // Output Generation

  static wdResult GenerateOutput(wdImage&& src, wdImage& dst, wdEnum<wdImageFormat> format);
  static wdResult GenerateThumbnailOutput(const wdImage& srcImg, wdImage& dstImg, wdUInt32 uiTargetRes);
  static wdResult GenerateLowResOutput(const wdImage& srcImg, wdImage& dstImg, wdUInt32 uiLowResMip);

  //////////////////////////////////////////////////////////////////////////
  // Texture Atlas

  struct TextureAtlasItem
  {
    wdUInt32 m_uiUniqueID = 0;
    wdUInt32 m_uiFlags = 0;
    wdImage m_InputImage[4];
    wdRectU32 m_AtlasRect[4];
  };

  wdResult LoadAtlasInputs(const wdTextureAtlasCreationDesc& atlasDesc, wdDynamicArray<TextureAtlasItem>& items) const;
  wdResult CreateAtlasLayerTexture(
    const wdTextureAtlasCreationDesc& atlasDesc, wdDynamicArray<TextureAtlasItem>& atlasItems, wdInt32 layer, wdImage& dstImg);

  static wdResult WriteTextureAtlasInfo(const wdDynamicArray<TextureAtlasItem>& atlasItems, wdUInt32 uiNumLayers, wdStreamWriter& stream);
  static wdResult TrySortItemsIntoAtlas(wdDynamicArray<TextureAtlasItem>& items, wdUInt32 uiWidth, wdUInt32 uiHeight, wdInt32 layer);
  static wdResult SortItemsIntoAtlas(wdDynamicArray<TextureAtlasItem>& items, wdUInt32& out_ResX, wdUInt32& out_ResY, wdInt32 layer);
  static wdResult CreateAtlasTexture(wdDynamicArray<TextureAtlasItem>& items, wdUInt32 uiResX, wdUInt32 uiResY, wdImage& atlas, wdInt32 layer);
  static wdResult FillAtlasBorders(wdDynamicArray<TextureAtlasItem>& items, wdImage& atlas, wdInt32 layer);

  //////////////////////////////////////////////////////////////////////////
  // Texture Atlas

  wdResult GenerateTextureAtlas(wdMemoryStreamWriter& stream);
};
