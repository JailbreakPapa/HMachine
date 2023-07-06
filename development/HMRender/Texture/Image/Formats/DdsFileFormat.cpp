#include <Texture/TexturePCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Formats/ImageFormatMappings.h>
#include <Texture/Image/Image.h>

wdDdsFileFormat g_ddsFormat;

struct wdDdsPixelFormat
{
  wdUInt32 m_uiSize;
  wdUInt32 m_uiFlags;
  wdUInt32 m_uiFourCC;
  wdUInt32 m_uiRGBBitCount;
  wdUInt32 m_uiRBitMask;
  wdUInt32 m_uiGBitMask;
  wdUInt32 m_uiBBitMask;
  wdUInt32 m_uiABitMask;
};

struct wdDdsHeader
{
  wdUInt32 m_uiMagic;
  wdUInt32 m_uiSize;
  wdUInt32 m_uiFlags;
  wdUInt32 m_uiHeight;
  wdUInt32 m_uiWidth;
  wdUInt32 m_uiPitchOrLinearSize;
  wdUInt32 m_uiDepth;
  wdUInt32 m_uiMipMapCount;
  wdUInt32 m_uiReserved1[11];
  wdDdsPixelFormat m_ddspf;
  wdUInt32 m_uiCaps;
  wdUInt32 m_uiCaps2;
  wdUInt32 m_uiCaps3;
  wdUInt32 m_uiCaps4;
  wdUInt32 m_uiReserved2;
};

struct wdDdsResourceDimension
{
  enum Enum
  {
    TEXTURE1D = 2,
    TEXTURE2D = 3,
    TEXTURE3D = 4,
  };
};

struct wdDdsResourceMiscFlags
{
  enum Enum
  {
    TEXTURECUBE = 0x4,
  };
};

struct wdDdsHeaderDxt10
{
  wdUInt32 m_uiDxgiFormat;
  wdUInt32 m_uiResourceDimension;
  wdUInt32 m_uiMiscFlag;
  wdUInt32 m_uiArraySize;
  wdUInt32 m_uiMiscFlags2;
};

struct wdDdsdFlags
{
  enum Enum
  {
    CAPS = 0x000001,
    HEIGHT = 0x000002,
    WIDTH = 0x000004,
    PITCH = 0x000008,
    PIXELFORMAT = 0x001000,
    MIPMAPCOUNT = 0x020000,
    LINEARSIZE = 0x080000,
    DEPTH = 0x800000,
  };
};

struct wdDdpfFlags
{
  enum Enum
  {
    ALPHAPIXELS = 0x00001,
    ALPHA = 0x00002,
    FOURCC = 0x00004,
    RGB = 0x00040,
    YUV = 0x00200,
    LUMINANCE = 0x20000,
  };
};

struct wdDdsCaps
{
  enum Enum
  {
    COMPLEX = 0x000008,
    MIPMAP = 0x400000,
    TEXTURE = 0x001000,
  };
};

struct wdDdsCaps2
{
  enum Enum
  {
    CUBEMAP = 0x000200,
    CUBEMAP_POSITIVEX = 0x000400,
    CUBEMAP_NEGATIVEX = 0x000800,
    CUBEMAP_POSITIVEY = 0x001000,
    CUBEMAP_NEGATIVEY = 0x002000,
    CUBEMAP_POSITIVEZ = 0x004000,
    CUBEMAP_NEGATIVEZ = 0x008000,
    VOLUME = 0x200000,
  };
};

static const wdUInt32 wdDdsMagic = 0x20534444;
static const wdUInt32 wdDdsDxt10FourCc = 0x30315844;

static wdResult ReadImageData(wdStreamReader& inout_stream, wdImageHeader& ref_imageHeader, wdDdsHeader& ref_ddsHeader)
{
  if (inout_stream.ReadBytes(&ref_ddsHeader, sizeof(wdDdsHeader)) != sizeof(wdDdsHeader))
  {
    wdLog::Error("Failed to read file header.");
    return WD_FAILURE;
  }

  if (ref_ddsHeader.m_uiMagic != wdDdsMagic)
  {
    wdLog::Error("The file is not a recognized DDS file.");
    return WD_FAILURE;
  }

  if (ref_ddsHeader.m_uiSize != 124)
  {
    wdLog::Error("The file header size {0} doesn't match the expected size of 124.", ref_ddsHeader.m_uiSize);
    return WD_FAILURE;
  }

  // Required in every .dds file. According to the spec, CAPS and PIXELFORMAT are also required, but D3DX outputs
  // files not conforming to this.
  if ((ref_ddsHeader.m_uiFlags & wdDdsdFlags::WIDTH) == 0 || (ref_ddsHeader.m_uiFlags & wdDdsdFlags::HEIGHT) == 0)
  {
    wdLog::Error("The file header doesn't specify the mandatory WIDTH or HEIGHT flag.");
    return WD_FAILURE;
  }

  if ((ref_ddsHeader.m_uiCaps & wdDdsCaps::TEXTURE) == 0)
  {
    wdLog::Error("The file header doesn't specify the mandatory TEXTURE flag.");
    return WD_FAILURE;
  }

  ref_imageHeader.SetWidth(ref_ddsHeader.m_uiWidth);
  ref_imageHeader.SetHeight(ref_ddsHeader.m_uiHeight);

  if (ref_ddsHeader.m_ddspf.m_uiSize != 32)
  {
    wdLog::Error("The pixel format size {0} doesn't match the expected value of 32.", ref_ddsHeader.m_ddspf.m_uiSize);
    return WD_FAILURE;
  }

  wdDdsHeaderDxt10 headerDxt10;

  wdImageFormat::Enum format = wdImageFormat::UNKNOWN;

  // Data format specified in RGBA masks
  if ((ref_ddsHeader.m_ddspf.m_uiFlags & wdDdpfFlags::ALPHAPIXELS) != 0 || (ref_ddsHeader.m_ddspf.m_uiFlags & wdDdpfFlags::RGB) != 0 ||
      (ref_ddsHeader.m_ddspf.m_uiFlags & wdDdpfFlags::ALPHA) != 0)
  {
    format = wdImageFormat::FromPixelMask(ref_ddsHeader.m_ddspf.m_uiRBitMask, ref_ddsHeader.m_ddspf.m_uiGBitMask, ref_ddsHeader.m_ddspf.m_uiBBitMask,
      ref_ddsHeader.m_ddspf.m_uiABitMask, ref_ddsHeader.m_ddspf.m_uiRGBBitCount);

    if (format == wdImageFormat::UNKNOWN)
    {
      wdLog::Error("The pixel mask specified was not recognized (R: {0}, G: {1}, B: {2}, A: {3}, Bpp: {4}).",
        wdArgU(ref_ddsHeader.m_ddspf.m_uiRBitMask, 1, false, 16), wdArgU(ref_ddsHeader.m_ddspf.m_uiGBitMask, 1, false, 16),
        wdArgU(ref_ddsHeader.m_ddspf.m_uiBBitMask, 1, false, 16), wdArgU(ref_ddsHeader.m_ddspf.m_uiABitMask, 1, false, 16),
        ref_ddsHeader.m_ddspf.m_uiRGBBitCount);
      return WD_FAILURE;
    }

    // Verify that the format we found is correct
    if (wdImageFormat::GetBitsPerPixel(format) != ref_ddsHeader.m_ddspf.m_uiRGBBitCount)
    {
      wdLog::Error("The number of bits per pixel specified in the file ({0}) does not match the expected value of {1} for the format '{2}'.",
        ref_ddsHeader.m_ddspf.m_uiRGBBitCount, wdImageFormat::GetBitsPerPixel(format), wdImageFormat::GetName(format));
      return WD_FAILURE;
    }
  }
  else if ((ref_ddsHeader.m_ddspf.m_uiFlags & wdDdpfFlags::FOURCC) != 0)
  {
    if (ref_ddsHeader.m_ddspf.m_uiFourCC == wdDdsDxt10FourCc)
    {
      if (inout_stream.ReadBytes(&headerDxt10, sizeof(wdDdsHeaderDxt10)) != sizeof(wdDdsHeaderDxt10))
      {
        wdLog::Error("Failed to read file header.");
        return WD_FAILURE;
      }

      format = wdImageFormatMappings::FromDxgiFormat(headerDxt10.m_uiDxgiFormat);

      if (format == wdImageFormat::UNKNOWN)
      {
        wdLog::Error("The DXGI format {0} has no equivalent image format.", headerDxt10.m_uiDxgiFormat);
        return WD_FAILURE;
      }
    }
    else
    {
      format = wdImageFormatMappings::FromFourCc(ref_ddsHeader.m_ddspf.m_uiFourCC);

      if (format == wdImageFormat::UNKNOWN)
      {
        wdLog::Error("The FourCC code '{0}{1}{2}{3}' was not recognized.", wdArgC((char)(ref_ddsHeader.m_ddspf.m_uiFourCC >> 0)),
          wdArgC((char)(ref_ddsHeader.m_ddspf.m_uiFourCC >> 8)), wdArgC((char)(ref_ddsHeader.m_ddspf.m_uiFourCC >> 16)),
          wdArgC((char)(ref_ddsHeader.m_ddspf.m_uiFourCC >> 24)));
        return WD_FAILURE;
      }
    }
  }
  else
  {
    wdLog::Error("The image format is neither specified as a pixel mask nor as a FourCC code.");
    return WD_FAILURE;
  }

  ref_imageHeader.SetImageFormat(format);

  const bool bHasMipMaps = (ref_ddsHeader.m_uiCaps & wdDdsCaps::MIPMAP) != 0;
  const bool bCubeMap = (ref_ddsHeader.m_uiCaps2 & wdDdsCaps2::CUBEMAP) != 0;
  const bool bVolume = (ref_ddsHeader.m_uiCaps2 & wdDdsCaps2::VOLUME) != 0;


  if (bHasMipMaps)
  {
    ref_imageHeader.SetNumMipLevels(ref_ddsHeader.m_uiMipMapCount);
  }

  // Cubemap and volume texture are mutually exclusive
  if (bVolume && bCubeMap)
  {
    wdLog::Error("The header specifies both the VOLUME and CUBEMAP flags.");
    return WD_FAILURE;
  }

  if (bCubeMap)
  {
    ref_imageHeader.SetNumFaces(6);
  }
  else if (bVolume)
  {
    ref_imageHeader.SetDepth(ref_ddsHeader.m_uiDepth);
  }

  return WD_SUCCESS;
}

wdResult wdDdsFileFormat::ReadImageHeader(wdStreamReader& inout_stream, wdImageHeader& ref_header, const char* szFileExtension) const
{
  WD_PROFILE_SCOPE("wdDdsFileFormat::ReadImageHeader");

  wdDdsHeader ddsHeader;
  return ReadImageData(inout_stream, ref_header, ddsHeader);
}

wdResult wdDdsFileFormat::ReadImage(wdStreamReader& inout_stream, wdImage& ref_image, const char* szFileExtension) const
{
  WD_PROFILE_SCOPE("wdDdsFileFormat::ReadImage");

  wdImageHeader imageHeader;
  wdDdsHeader ddsHeader;
  WD_SUCCEED_OR_RETURN(ReadImageData(inout_stream, imageHeader, ddsHeader));

  ref_image.ResetAndAlloc(imageHeader);

  const bool bPitch = (ddsHeader.m_uiFlags & wdDdsdFlags::PITCH) != 0;

  // If pitch is specified, it must match the computed value
  if (bPitch && ref_image.GetRowPitch(0) != ddsHeader.m_uiPitchOrLinearSize)
  {
    wdLog::Error("The row pitch specified in the header doesn't match the expected pitch.");
    return WD_FAILURE;
  }

  wdUInt64 uiDataSize = ref_image.GetByteBlobPtr().GetCount();

  if (inout_stream.ReadBytes(ref_image.GetByteBlobPtr().GetPtr(), uiDataSize) != uiDataSize)
  {
    wdLog::Error("Failed to read image data.");
    return WD_FAILURE;
  }

  return WD_SUCCESS;
}

wdResult wdDdsFileFormat::WriteImage(wdStreamWriter& inout_stream, const wdImageView& image, const char* szFileExtension) const
{
  const wdImageFormat::Enum format = image.GetImageFormat();
  const wdUInt32 uiBpp = wdImageFormat::GetBitsPerPixel(format);

  const wdUInt32 uiNumFaces = image.GetNumFaces();
  const wdUInt32 uiNumMipLevels = image.GetNumMipLevels();
  const wdUInt32 uiNumArrayIndices = image.GetNumArrayIndices();

  const wdUInt32 uiWidth = image.GetWidth(0);
  const wdUInt32 uiHeight = image.GetHeight(0);
  const wdUInt32 uiDepth = image.GetDepth(0);

  bool bHasMipMaps = uiNumMipLevels > 1;
  bool bVolume = uiDepth > 1;
  bool bCubeMap = uiNumFaces > 1;
  bool bArray = uiNumArrayIndices > 1;

  bool bDxt10 = false;

  wdDdsHeader fileHeader;
  wdDdsHeaderDxt10 headerDxt10;

  wdMemoryUtils::ZeroFill(&fileHeader, 1);
  wdMemoryUtils::ZeroFill(&headerDxt10, 1);

  fileHeader.m_uiMagic = wdDdsMagic;
  fileHeader.m_uiSize = 124;
  fileHeader.m_uiWidth = uiWidth;
  fileHeader.m_uiHeight = uiHeight;

  // Required in every .dds file.
  fileHeader.m_uiFlags = wdDdsdFlags::WIDTH | wdDdsdFlags::HEIGHT | wdDdsdFlags::CAPS | wdDdsdFlags::PIXELFORMAT;

  if (bHasMipMaps)
  {
    fileHeader.m_uiFlags |= wdDdsdFlags::MIPMAPCOUNT;
    fileHeader.m_uiMipMapCount = uiNumMipLevels;
  }

  if (bVolume)
  {
    // Volume and array are incompatible
    if (bArray)
    {
      wdLog::Error("The image is both an array and volume texture. This is not supported.");
      return WD_FAILURE;
    }

    fileHeader.m_uiFlags |= wdDdsdFlags::DEPTH;
    fileHeader.m_uiDepth = uiDepth;
  }

  switch (wdImageFormat::GetType(image.GetImageFormat()))
  {
    case wdImageFormatType::LINEAR:
      [[fallthrough]];
    case wdImageFormatType::PLANAR:
      fileHeader.m_uiFlags |= wdDdsdFlags::PITCH;
      fileHeader.m_uiPitchOrLinearSize = static_cast<wdUInt32>(image.GetRowPitch(0));
      break;

    case wdImageFormatType::BLOCK_COMPRESSED:
      fileHeader.m_uiFlags |= wdDdsdFlags::LINEARSIZE;
      fileHeader.m_uiPitchOrLinearSize = 0; /// \todo sub-image size
      break;

    default:
      wdLog::Error("Unknown image format type.");
      return WD_FAILURE;
  }

  fileHeader.m_uiCaps = wdDdsCaps::TEXTURE;

  if (bCubeMap)
  {
    if (uiNumFaces != 6)
    {
      wdLog::Error("The image is a cubemap, but has {0} faces instead of the expected 6.", uiNumFaces);
      return WD_FAILURE;
    }

    if (bVolume)
    {
      wdLog::Error("The image is both a cubemap and volume texture. This is not supported.");
      return WD_FAILURE;
    }

    fileHeader.m_uiCaps |= wdDdsCaps::COMPLEX;
    fileHeader.m_uiCaps2 |= wdDdsCaps2::CUBEMAP | wdDdsCaps2::CUBEMAP_POSITIVEX | wdDdsCaps2::CUBEMAP_NEGATIVEX | wdDdsCaps2::CUBEMAP_POSITIVEY |
                            wdDdsCaps2::CUBEMAP_NEGATIVEY | wdDdsCaps2::CUBEMAP_POSITIVEZ | wdDdsCaps2::CUBEMAP_NEGATIVEZ;
  }

  if (bArray)
  {
    fileHeader.m_uiCaps |= wdDdsCaps::COMPLEX;

    // Must be written as DXT10
    bDxt10 = true;
  }

  if (bVolume)
  {
    fileHeader.m_uiCaps |= wdDdsCaps::COMPLEX;
    fileHeader.m_uiCaps2 |= wdDdsCaps2::VOLUME;
  }

  if (bHasMipMaps)
  {
    fileHeader.m_uiCaps |= wdDdsCaps::MIPMAP | wdDdsCaps::COMPLEX;
  }

  fileHeader.m_ddspf.m_uiSize = 32;

  wdUInt32 uiRedMask = wdImageFormat::GetRedMask(format);
  wdUInt32 uiGreenMask = wdImageFormat::GetGreenMask(format);
  wdUInt32 uiBlueMask = wdImageFormat::GetBlueMask(format);
  wdUInt32 uiAlphaMask = wdImageFormat::GetAlphaMask(format);

  wdUInt32 uiFourCc = wdImageFormatMappings::ToFourCc(format);
  wdUInt32 uiDxgiFormat = wdImageFormatMappings::ToDxgiFormat(format);

  // When not required to use a DXT10 texture, try to write a legacy DDS by specifying FourCC or pixel masks
  if (!bDxt10)
  {
    // The format has a known mask and we would also recognize it as the same when reading back in, since multiple formats may have the same pixel
    // masks
    if ((uiRedMask | uiGreenMask | uiBlueMask | uiAlphaMask) &&
        format == wdImageFormat::FromPixelMask(uiRedMask, uiGreenMask, uiBlueMask, uiAlphaMask, uiBpp))
    {
      fileHeader.m_ddspf.m_uiFlags = wdDdpfFlags::ALPHAPIXELS | wdDdpfFlags::RGB;
      fileHeader.m_ddspf.m_uiRBitMask = uiRedMask;
      fileHeader.m_ddspf.m_uiGBitMask = uiGreenMask;
      fileHeader.m_ddspf.m_uiBBitMask = uiBlueMask;
      fileHeader.m_ddspf.m_uiABitMask = uiAlphaMask;
      fileHeader.m_ddspf.m_uiRGBBitCount = wdImageFormat::GetBitsPerPixel(format);
    }
    // The format has a known FourCC
    else if (uiFourCc != 0)
    {
      fileHeader.m_ddspf.m_uiFlags = wdDdpfFlags::FOURCC;
      fileHeader.m_ddspf.m_uiFourCC = uiFourCc;
    }
    else
    {
      // Fallback to DXT10 path
      bDxt10 = true;
    }
  }

  if (bDxt10)
  {
    // We must write a DXT10 file, but there is no matching DXGI_FORMAT - we could also try converting, but that is rarely intended when writing .dds
    if (uiDxgiFormat == 0)
    {
      wdLog::Error("The image needs to be written as a DXT10 file, but no matching DXGI format was found for '{0}'.", wdImageFormat::GetName(format));
      return WD_FAILURE;
    }

    fileHeader.m_ddspf.m_uiFlags = wdDdpfFlags::FOURCC;
    fileHeader.m_ddspf.m_uiFourCC = wdDdsDxt10FourCc;

    headerDxt10.m_uiDxgiFormat = uiDxgiFormat;

    if (bVolume)
    {
      headerDxt10.m_uiResourceDimension = wdDdsResourceDimension::TEXTURE3D;
    }
    else if (uiHeight > 1)
    {
      headerDxt10.m_uiResourceDimension = wdDdsResourceDimension::TEXTURE2D;
    }
    else
    {
      headerDxt10.m_uiResourceDimension = wdDdsResourceDimension::TEXTURE1D;
    }

    if (bCubeMap)
    {
      headerDxt10.m_uiMiscFlag = wdDdsResourceMiscFlags::TEXTURECUBE;
    }

    // NOT multiplied by number of cubemap faces
    headerDxt10.m_uiArraySize = uiNumArrayIndices;

    // Can be used to describe the alpha channel usage, but automatically makes it incompatible with the D3DX libraries if not 0.
    headerDxt10.m_uiMiscFlags2 = 0;
  }

  if (inout_stream.WriteBytes(&fileHeader, sizeof(fileHeader)) != WD_SUCCESS)
  {
    wdLog::Error("Failed to write image header.");
    return WD_FAILURE;
  }

  if (bDxt10)
  {
    if (inout_stream.WriteBytes(&headerDxt10, sizeof(headerDxt10)) != WD_SUCCESS)
    {
      wdLog::Error("Failed to write image DX10 header.");
      return WD_FAILURE;
    }
  }

  if (inout_stream.WriteBytes(image.GetByteBlobPtr().GetPtr(), image.GetByteBlobPtr().GetCount()) != WD_SUCCESS)
  {
    wdLog::Error("Failed to write image data.");
    return WD_FAILURE;
  }

  return WD_SUCCESS;
}

bool wdDdsFileFormat::CanReadFileType(const char* szExtension) const
{
  return wdStringUtils::IsEqual_NoCase(szExtension, "dds");
}

bool wdDdsFileFormat::CanWriteFileType(const char* szExtension) const
{
  return CanReadFileType(szExtension);
}



WD_STATICLINK_FILE(Texture, Texture_Image_Formats_DdsFileFormat);
