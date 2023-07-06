#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/ImageUtils.h>
#include <Texture/TexConv/TexConvProcessor.h>
#include <Texture/Utils/TextureAtlasDesc.h>
#include <Texture/Utils/TexturePacker.h>

wdResult wdTexConvProcessor::GenerateTextureAtlas(wdMemoryStreamWriter& stream)
{
  if (m_Descriptor.m_OutputType != wdTexConvOutputType::Atlas)
    return WD_SUCCESS;


  if (m_Descriptor.m_sTextureAtlasDescFile.IsEmpty())
  {
    wdLog::Error("Texture atlas description file is not specified.");
    return WD_FAILURE;
  }

  wdTextureAtlasCreationDesc atlasDesc;
  wdDynamicArray<TextureAtlasItem> atlasItems;

  if (atlasDesc.Load(m_Descriptor.m_sTextureAtlasDescFile).Failed())
  {
    wdLog::Error("Failed to load texture atlas description '{0}'", wdArgSensitive(m_Descriptor.m_sTextureAtlasDescFile, "File"));
    return WD_FAILURE;
  }

  m_Descriptor.m_uiMinResolution = wdMath::Max(32u, m_Descriptor.m_uiMinResolution);

  WD_SUCCEED_OR_RETURN(LoadAtlasInputs(atlasDesc, atlasItems));

  const wdUInt8 uiVersion = 3;
  stream << uiVersion;

  wdDdsFileFormat ddsWriter;
  wdImage atlasImg;

  for (wdUInt32 layerIdx = 0; layerIdx < atlasDesc.m_Layers.GetCount(); ++layerIdx)
  {
    WD_SUCCEED_OR_RETURN(CreateAtlasLayerTexture(atlasDesc, atlasItems, layerIdx, atlasImg));

    if (ddsWriter.WriteImage(stream, atlasImg, "dds").Failed())
    {
      wdLog::Error("Failed to write DDS image to texture atlas file.");
      return WD_FAILURE;
    }

    // debug: write out atlas slices as pure DDS
    if (false)
    {
      wdStringBuilder sOut;
      sOut.Format("D:/atlas_{}.dds", layerIdx);

      wdFileWriter fOut;
      if (fOut.Open(sOut).Succeeded())
      {
        WD_SUCCEED_OR_RETURN(ddsWriter.WriteImage(fOut, atlasImg, "dds"));
      }
    }
  }

  WD_SUCCEED_OR_RETURN(WriteTextureAtlasInfo(atlasItems, atlasDesc.m_Layers.GetCount(), stream));

  return WD_SUCCESS;
}

wdResult wdTexConvProcessor::LoadAtlasInputs(const wdTextureAtlasCreationDesc& atlasDesc, wdDynamicArray<TextureAtlasItem>& items) const
{
  items.Clear();

  for (const auto& srcItem : atlasDesc.m_Items)
  {
    auto& item = items.ExpandAndGetRef();
    item.m_uiUniqueID = srcItem.m_uiUniqueID;
    item.m_uiFlags = srcItem.m_uiFlags;

    for (wdUInt32 layer = 0; layer < atlasDesc.m_Layers.GetCount(); ++layer)
    {
      if (!srcItem.m_sLayerInput[layer].IsEmpty())
      {
        if (item.m_InputImage[layer].LoadFrom(srcItem.m_sLayerInput[layer]).Failed())
        {
          wdLog::Error("Failed to load texture atlas texture '{0}'", wdArgSensitive(srcItem.m_sLayerInput[layer], "File"));
          return WD_FAILURE;
        }

        if (atlasDesc.m_Layers[layer].m_Usage == wdTexConvUsage::Color)
        {
          // enforce sRGB format for all color textures
          item.m_InputImage[layer].ReinterpretAs(wdImageFormat::AsSrgb(item.m_InputImage[layer].GetImageFormat()));
        }

        wdUInt32 uiResX = 0, uiResY = 0;
        WD_SUCCEED_OR_RETURN(DetermineTargetResolution(item.m_InputImage[layer], wdImageFormat::UNKNOWN, uiResX, uiResY));

        WD_SUCCEED_OR_RETURN(ConvertAndScaleImage(srcItem.m_sLayerInput[layer], item.m_InputImage[layer], uiResX, uiResY, atlasDesc.m_Layers[layer].m_Usage));
      }
    }


    if (!srcItem.m_sAlphaInput.IsEmpty())
    {
      wdImage alphaImg;

      if (alphaImg.LoadFrom(srcItem.m_sAlphaInput).Failed())
      {
        wdLog::Error("Failed to load texture atlas alpha mask '{0}'", srcItem.m_sAlphaInput);
        return WD_FAILURE;
      }

      wdUInt32 uiResX = 0, uiResY = 0;
      WD_SUCCEED_OR_RETURN(DetermineTargetResolution(alphaImg, wdImageFormat::UNKNOWN, uiResX, uiResY));

      WD_SUCCEED_OR_RETURN(ConvertAndScaleImage(srcItem.m_sAlphaInput, alphaImg, uiResX, uiResY, wdTexConvUsage::Linear));


      // layer 0 must have the exact same size as the alpha texture
      WD_SUCCEED_OR_RETURN(ConvertAndScaleImage(srcItem.m_sLayerInput[0], item.m_InputImage[0], uiResX, uiResY, wdTexConvUsage::Linear));

      // copy alpha channel into layer 0
      WD_SUCCEED_OR_RETURN(wdImageUtils::CopyChannel(item.m_InputImage[0], 3, alphaImg, 0));

      // rescale all layers to be no larger than the alpha mask texture
      for (wdUInt32 layer = 1; layer < atlasDesc.m_Layers.GetCount(); ++layer)
      {
        if (item.m_InputImage[layer].GetWidth() <= uiResX && item.m_InputImage[layer].GetHeight() <= uiResY)
          continue;

        WD_SUCCEED_OR_RETURN(ConvertAndScaleImage(srcItem.m_sLayerInput[layer], item.m_InputImage[layer], uiResX, uiResY, wdTexConvUsage::Linear));
      }
    }
  }

  return WD_SUCCESS;
}

wdResult wdTexConvProcessor::WriteTextureAtlasInfo(const wdDynamicArray<TextureAtlasItem>& atlasItems, wdUInt32 uiNumLayers, wdStreamWriter& stream)
{
  wdTextureAtlasRuntimeDesc runtimeAtlas;
  runtimeAtlas.m_uiNumLayers = uiNumLayers;

  runtimeAtlas.m_Items.Reserve(atlasItems.GetCount());

  for (const auto& item : atlasItems)
  {
    auto& e = runtimeAtlas.m_Items[item.m_uiUniqueID];
    e.m_uiFlags = item.m_uiFlags;

    for (wdUInt32 l = 0; l < uiNumLayers; ++l)
    {
      e.m_LayerRects[l] = item.m_AtlasRect[l];
    }
  }

  return runtimeAtlas.Serialize(stream);
}

constexpr wdUInt32 uiAtlasCellSize = 32;

wdResult wdTexConvProcessor::TrySortItemsIntoAtlas(wdDynamicArray<TextureAtlasItem>& items, wdUInt32 uiWidth, wdUInt32 uiHeight, wdInt32 layer)
{
  wdTexturePacker packer;

  // TODO: review, currently the texture packer only works on 32 sized cells
  wdUInt32 uiPixelAlign = uiAtlasCellSize;

  packer.SetTextureSize(uiWidth, uiHeight, items.GetCount() * 2);

  for (const auto& item : items)
  {
    if (item.m_InputImage[layer].IsValid())
    {
      packer.AddTexture((item.m_InputImage[layer].GetWidth() + (uiPixelAlign - 1)) / uiPixelAlign, (item.m_InputImage[layer].GetHeight() + (uiPixelAlign - 1)) / uiPixelAlign);
    }
  }

  WD_SUCCEED_OR_RETURN(packer.PackTextures());

  wdUInt32 uiTexIdx = 0;
  for (auto& item : items)
  {
    if (item.m_InputImage[layer].IsValid())
    {
      const auto& tex = packer.GetTextures()[uiTexIdx++];

      item.m_AtlasRect[layer].x = tex.m_Position.x * uiAtlasCellSize;
      item.m_AtlasRect[layer].y = tex.m_Position.y * uiAtlasCellSize;
      item.m_AtlasRect[layer].width = tex.m_Size.x * uiAtlasCellSize;
      item.m_AtlasRect[layer].height = tex.m_Size.y * uiAtlasCellSize;
    }
  }

  return WD_SUCCESS;
}

wdResult wdTexConvProcessor::SortItemsIntoAtlas(wdDynamicArray<TextureAtlasItem>& items, wdUInt32& out_ResX, wdUInt32& out_ResY, wdInt32 layer)
{
  for (wdUInt32 power = 8; power < 14; ++power)
  {
    const wdUInt32 halfRes = 1 << (power - 1);
    const wdUInt32 resolution = 1 << power;
    const wdUInt32 resDivCellSize = resolution / uiAtlasCellSize;
    const wdUInt32 halfResDivCellSize = halfRes / uiAtlasCellSize;

    if (TrySortItemsIntoAtlas(items, resDivCellSize, halfResDivCellSize, layer).Succeeded())
    {
      out_ResX = resolution;
      out_ResY = halfRes;
      return WD_SUCCESS;
    }

    if (TrySortItemsIntoAtlas(items, halfResDivCellSize, resDivCellSize, layer).Succeeded())
    {
      out_ResX = halfRes;
      out_ResY = resolution;
      return WD_SUCCESS;
    }

    if (TrySortItemsIntoAtlas(items, resDivCellSize, resDivCellSize, layer).Succeeded())
    {
      out_ResX = resolution;
      out_ResY = resolution;
      return WD_SUCCESS;
    }
  }

  wdLog::Error("Could not sort items into texture atlas. Too many too large textures.");
  return WD_FAILURE;
}

wdResult wdTexConvProcessor::CreateAtlasTexture(wdDynamicArray<TextureAtlasItem>& items, wdUInt32 uiResX, wdUInt32 uiResY, wdImage& atlas, wdInt32 layer)
{
  wdImageHeader imgHeader;
  imgHeader.SetWidth(uiResX);
  imgHeader.SetHeight(uiResY);
  imgHeader.SetImageFormat(wdImageFormat::R32G32B32A32_FLOAT);
  atlas.ResetAndAlloc(imgHeader);

  // make sure the target texture is filled with all black
  {
    auto pixelData = atlas.GetBlobPtr<wdUInt8>();
    wdMemoryUtils::ZeroFill(pixelData.GetPtr(), static_cast<size_t>(pixelData.GetCount()));
  }

  for (auto& item : items)
  {
    if (item.m_InputImage[layer].IsValid())
    {
      wdImage& itemImage = item.m_InputImage[layer];

      wdRectU32 r;
      r.x = 0;
      r.y = 0;
      r.width = itemImage.GetWidth();
      r.height = itemImage.GetHeight();

      WD_SUCCEED_OR_RETURN(wdImageUtils::Copy(itemImage, r, atlas, wdVec3U32(item.m_AtlasRect[layer].x, item.m_AtlasRect[layer].y, 0)));
    }
  }

  return WD_SUCCESS;
}

wdResult wdTexConvProcessor::FillAtlasBorders(wdDynamicArray<TextureAtlasItem>& items, wdImage& atlas, wdInt32 layer)
{
  const wdUInt32 uiBorderPixels = 2;

  const wdUInt32 uiNumMipmaps = atlas.GetHeader().GetNumMipLevels();
  for (wdUInt32 uiMipLevel = 0; uiMipLevel < uiNumMipmaps; ++uiMipLevel)
  {
    for (auto& item : items)
    {
      if (!item.m_InputImage[layer].IsValid())
        continue;

      wdRectU32& itemRect = item.m_AtlasRect[layer];
      const wdUInt32 uiRectX = itemRect.x >> uiMipLevel;
      const wdUInt32 uiRectY = itemRect.y >> uiMipLevel;
      const wdUInt32 uiWidth = wdMath::Max(1u, itemRect.width >> uiMipLevel);
      const wdUInt32 uiHeight = wdMath::Max(1u, itemRect.height >> uiMipLevel);

      // fill the border of the item rect with alpha 0 to prevent bleeding into other decals in the atlas
      if (uiWidth <= 2 * uiBorderPixels || uiHeight <= 2 * uiBorderPixels)
      {
        for (wdUInt32 y = 0; y < uiHeight; ++y)
        {
          for (wdUInt32 x = 0; x < uiWidth; ++x)
          {
            const wdUInt32 xClamped = wdMath::Min(uiRectX + x, atlas.GetWidth(uiMipLevel));
            const wdUInt32 yClamped = wdMath::Min(uiRectY + y, atlas.GetHeight(uiMipLevel));
            atlas.GetPixelPointer<wdColor>(uiMipLevel, 0, 0, xClamped, yClamped)->a = 0.0f;
          }
        }
      }
      else
      {
        for (wdUInt32 i = 0; i < uiBorderPixels; ++i)
        {
          for (wdUInt32 y = 0; y < uiHeight; ++y)
          {
            atlas.GetPixelPointer<wdColor>(uiMipLevel, 0, 0, uiRectX + i, uiRectY + y)->a = 0.0f;
            atlas.GetPixelPointer<wdColor>(uiMipLevel, 0, 0, uiRectX + uiWidth - 1 - i, uiRectY + y)->a = 0.0f;
          }

          for (wdUInt32 x = 0; x < uiWidth; ++x)
          {
            atlas.GetPixelPointer<wdColor>(uiMipLevel, 0, 0, uiRectX + x, uiRectY + i)->a = 0.0f;
            atlas.GetPixelPointer<wdColor>(uiMipLevel, 0, 0, uiRectX + x, uiRectY + uiHeight - 1 - i)->a = 0.0f;
          }
        }
      }
    }
  }

  return WD_SUCCESS;
}

wdResult wdTexConvProcessor::CreateAtlasLayerTexture(const wdTextureAtlasCreationDesc& atlasDesc, wdDynamicArray<TextureAtlasItem>& atlasItems, wdInt32 layer, wdImage& dstImg)
{
  wdUInt32 uiTexWidth, uiTexHeight;
  WD_SUCCEED_OR_RETURN(SortItemsIntoAtlas(atlasItems, uiTexWidth, uiTexHeight, layer));

  wdLog::Success("Required Resolution for Texture Atlas: {0} x {1}", uiTexWidth, uiTexHeight);

  wdImage atlasImg;
  WD_SUCCEED_OR_RETURN(CreateAtlasTexture(atlasItems, uiTexWidth, uiTexHeight, atlasImg, layer));

  wdUInt32 uiNumMipmaps = atlasImg.GetHeader().ComputeNumberOfMipMaps();
  WD_SUCCEED_OR_RETURN(GenerateMipmaps(atlasImg, uiNumMipmaps));

  if (atlasDesc.m_Layers[layer].m_uiNumChannels == 4)
  {
    WD_SUCCEED_OR_RETURN(FillAtlasBorders(atlasItems, atlasImg, layer));
  }

  wdEnum<wdImageFormat> OutputImageFormat;

  WD_SUCCEED_OR_RETURN(ChooseOutputFormat(OutputImageFormat, atlasDesc.m_Layers[layer].m_Usage, atlasDesc.m_Layers[layer].m_uiNumChannels));

  WD_SUCCEED_OR_RETURN(GenerateOutput(std::move(atlasImg), dstImg, OutputImageFormat));

  return WD_SUCCESS;
}


WD_STATICLINK_FILE(Texture, Texture_TexConv_Implementation_TextureAtlas);
