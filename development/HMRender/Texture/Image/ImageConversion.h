#pragma once

#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Utilities/EnumerableClass.h>

#include <Texture/Image/Image.h>

WD_DECLARE_FLAGS(wdUInt8, wdImageConversionFlags, InPlace);

/// A structure describing the pairs of source/target format that may be converted using the conversion routine.
struct wdImageConversionEntry
{
  wdImageConversionEntry(wdImageFormat::Enum source, wdImageFormat::Enum target, wdImageConversionFlags::Enum flags)
    : m_sourceFormat(source)
    , m_targetFormat(target)
    , m_flags(flags)
  {
  }

  const wdImageFormat::Enum m_sourceFormat;
  const wdImageFormat::Enum m_targetFormat;
  const wdBitflags<wdImageConversionFlags> m_flags;

  /// This member adds an additional amount to the cost estimate for this conversion step.
  /// It can be used to bias the choice between steps when there are comparable conversion
  /// steps available.
  float m_additionalPenalty = 0.0f;
};

/// \brief Interface for a single image conversion step.
///
/// The actual functionality is implemented as either wdImageConversionStepLinear or wdImageConversionStepDecompressBlocks.
/// Depending on the types on conversion advertised by GetSupportedConversions(), users of this class need to cast it to a derived type
/// first to access the desired functionality.
class WD_TEXTURE_DLL wdImageConversionStep : public wdEnumerable<wdImageConversionStep>
{
  WD_DECLARE_ENUMERABLE_CLASS(wdImageConversionStep);

protected:
  wdImageConversionStep();
  virtual ~wdImageConversionStep();

public:
  /// \brief Returns an array pointer of supported conversions.
  ///
  /// \note The returned array must have the same entries each time this method is called.
  virtual wdArrayPtr<const wdImageConversionEntry> GetSupportedConversions() const = 0;
};

/// \brief Interface for a single image conversion step where both the source and target format are uncompressed.
class WD_TEXTURE_DLL wdImageConversionStepLinear : public wdImageConversionStep
{
public:
  /// \brief Converts a batch of pixels.
  virtual wdResult ConvertPixels(wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt64 uiNumElements, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const = 0;
};

/// \brief Interface for a single image conversion step where the source format is compressed and the target format is uncompressed.
class WD_TEXTURE_DLL wdImageConversionStepDecompressBlocks : public wdImageConversionStep
{
public:
  /// \brief Decompresses the given number of blocks.
  virtual wdResult DecompressBlocks(wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt32 uiNumBlocks, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const = 0;
};

/// \brief Interface for a single image conversion step where the source format is uncompressed and the target format is compressed.
class WD_TEXTURE_DLL wdImageConversionStepCompressBlocks : public wdImageConversionStep
{
public:
  /// \brief Compresses the given number of blocks.
  virtual wdResult CompressBlocks(wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt32 uiNumBlocksX, wdUInt32 uiNumBlocksY,
    wdImageFormat::Enum sourceFormat, wdImageFormat::Enum targetFormat) const = 0;
};

/// \brief Interface for a single image conversion step from a linear to a planar format.
class WD_TEXTURE_DLL wdImageConversionStepPlanarize : public wdImageConversionStep
{
public:
  /// \brief Converts a batch of pixels into the given target planes.
  virtual wdResult ConvertPixels(const wdImageView& source, wdArrayPtr<wdImage> target, wdUInt32 uiNumPixelsX, wdUInt32 uiNumPixelsY, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const = 0;
};

/// \brief Interface for a single image conversion step from a planar to a linear format.
class WD_TEXTURE_DLL wdImageConversionStepDeplanarize : public wdImageConversionStep
{
public:
  /// \brief Converts a batch of pixels from the given source planes.
  virtual wdResult ConvertPixels(wdArrayPtr<wdImageView> source, wdImage target, wdUInt32 uiNumPixelsX, wdUInt32 uiNumPixelsY, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat) const = 0;
};


/// \brief Helper class containing utilities to convert between different image formats and layouts.
class WD_TEXTURE_DLL wdImageConversion
{
public:
  /// \brief Checks if there is a known conversion path between the two formats
  static bool IsConvertible(wdImageFormat::Enum sourceFormat, wdImageFormat::Enum targetFormat);

  /// \brief Finds the image format from a given list of formats which is the cheapest to convert to.
  static wdImageFormat::Enum FindClosestCompatibleFormat(wdImageFormat::Enum format, wdArrayPtr<const wdImageFormat::Enum> compatibleFormats);

  /// \brief A single node along a computed conversion path.
  struct ConversionPathNode
  {
    WD_DECLARE_POD_TYPE();

    const wdImageConversionStep* m_step;
    wdImageFormat::Enum m_sourceFormat;
    wdImageFormat::Enum m_targetFormat;
    wdUInt32 m_sourceBufferIndex;
    wdUInt32 m_targetBufferIndex;
    bool m_inPlace;
  };

  /// \brief Precomputes an optimal conversion path between two formats and the minimal number of required scratch buffers.
  ///
  /// The generated path can be cached by the user if the same conversion is performed multiple times. The path must not be reused if the
  /// set of supported conversions changes, e.g. when plugins are loaded or unloaded.
  ///
  /// \param sourceFormat           The source format.
  /// \param targetFormat           The target format.
  /// \param sourceEqualsTarget     If true, the generated path is applicable if source and target memory regions are equal, and may contain
  /// additional copy-steps if the conversion can't be performed in-place.
  ///                               A path generated with sourceEqualsTarget == true will work correctly even if source and target are not
  ///                               the same, but may not be optimal. A path generated with sourceEqualsTarget == false will not work
  ///                               correctly when source and target are the same.
  /// \param path_out               The generated path.
  /// \param numScratchBuffers_out The number of scratch buffers required for the conversion path.
  /// \returns                      wd_SUCCESS if a path was found, wd_FAILURE otherwise.
  static wdResult BuildPath(wdImageFormat::Enum sourceFormat, wdImageFormat::Enum targetFormat, bool bSourceEqualsTarget,
    wdHybridArray<ConversionPathNode, 16>& ref_path_out, wdUInt32& ref_uiNumScratchBuffers_out);

  /// \brief  Converts the source image into a target image with the given format. Source and target may be the same.
  static wdResult Convert(const wdImageView& source, wdImage& ref_target, wdImageFormat::Enum targetFormat);

  /// \brief Converts the source image into a target image using a precomputed conversion path.
  static wdResult Convert(const wdImageView& source, wdImage& ref_target, wdArrayPtr<ConversionPathNode> path, wdUInt32 uiNumScratchBuffers);

  /// \brief Converts the raw source data into a target data buffer with the given format. Source and target may be the same.
  static wdResult ConvertRaw(
    wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt32 uiNumElements, wdImageFormat::Enum sourceFormat, wdImageFormat::Enum targetFormat);

  /// \brief Converts the raw source data into a target data buffer using a precomputed conversion path.
  static wdResult ConvertRaw(
    wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt32 uiNumElements, wdArrayPtr<ConversionPathNode> path, wdUInt32 uiNumScratchBuffers);

private:
  wdImageConversion();
  wdImageConversion(const wdImageConversion&);

  static wdResult ConvertSingleStep(const wdImageConversionStep* pStep, const wdImageView& source, wdImage& target, wdImageFormat::Enum targetFormat);

  static wdResult ConvertSingleStepDecompress(const wdImageView& source, wdImage& target, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat, const wdImageConversionStep* pStep);

  static wdResult ConvertSingleStepCompress(const wdImageView& source, wdImage& target, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat, const wdImageConversionStep* pStep);

    static wdResult ConvertSingleStepDeplanarize(const wdImageView& source, wdImage& target, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat, const wdImageConversionStep* pStep);

  static wdResult ConvertSingleStepPlanarize(const wdImageView& source, wdImage& target, wdImageFormat::Enum sourceFormat,
    wdImageFormat::Enum targetFormat, const wdImageConversionStep* pStep);

  static void RebuildConversionTable();
};
