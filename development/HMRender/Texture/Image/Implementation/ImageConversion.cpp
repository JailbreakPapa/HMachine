#include <Texture/TexturePCH.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Profiling/Profiling.h>
#include <Texture/Image/ImageConversion.h>

WD_ENUMERABLE_CLASS_IMPLEMENTATION(wdImageConversionStep);

namespace
{
  struct TableEntry
  {
    TableEntry() = default;

    TableEntry(const wdImageConversionStep* pStep, const wdImageConversionEntry& entry)
    {
      m_step = pStep;
      m_sourceFormat = entry.m_sourceFormat;
      m_targetFormat = entry.m_targetFormat;
      m_numChannels = wdMath::Min(wdImageFormat::GetNumChannels(entry.m_sourceFormat), wdImageFormat::GetNumChannels(entry.m_targetFormat));

      float sourceBpp = wdImageFormat::GetExactBitsPerPixel(m_sourceFormat);
      float targetBpp = wdImageFormat::GetExactBitsPerPixel(m_targetFormat);

      m_flags = entry.m_flags;

      // Base cost is amount of bits processed
      m_cost = sourceBpp + targetBpp;

      // Penalty for non-inplace conversion
      if ((m_flags & wdImageConversionFlags::InPlace) == 0)
      {
        m_cost *= 2;
      }

      // Penalize formats that aren't aligned to powers of two
      if (!wdImageFormat::IsCompressed(m_sourceFormat) && !wdImageFormat::IsCompressed(m_targetFormat))
      {
        auto sourceBppInt = static_cast<wdUInt32>(sourceBpp);
        auto targetBppInt = static_cast<wdUInt32>(targetBpp);
        if (!wdMath::IsPowerOf2(sourceBppInt) || !wdMath::IsPowerOf2(targetBppInt))
        {
          m_cost *= 2;
        }
      }

      m_cost += entry.m_additionalPenalty;
    }

    const wdImageConversionStep* m_step = nullptr;
    wdImageFormat::Enum m_sourceFormat = wdImageFormat::UNKNOWN;
    wdImageFormat::Enum m_targetFormat = wdImageFormat::UNKNOWN;
    wdBitflags<wdImageConversionFlags> m_flags;
    float m_cost = wdMath::MaxValue<float>();
    wdUInt32 m_numChannels = 0;

    static TableEntry chain(const TableEntry& a, const TableEntry& b)
    {
      if (wdImageFormat::GetExactBitsPerPixel(a.m_sourceFormat) > wdImageFormat::GetExactBitsPerPixel(a.m_targetFormat) &&
          wdImageFormat::GetExactBitsPerPixel(b.m_sourceFormat) < wdImageFormat::GetExactBitsPerPixel(b.m_targetFormat))
      {
        // Disallow chaining conversions which first reduce to a smaller intermediate and then go back to a larger one, since
        // we end up throwing away information.
        return {};
      }

      TableEntry entry;
      entry.m_step = a.m_step;
      entry.m_cost = a.m_cost + b.m_cost;
      entry.m_sourceFormat = a.m_sourceFormat;
      entry.m_targetFormat = a.m_targetFormat;
      entry.m_flags = a.m_flags;
      entry.m_numChannels = wdMath::Min(a.m_numChannels, b.m_numChannels);
      return entry;
    }

    bool operator<(const TableEntry& other) const
    {
      if (m_numChannels > other.m_numChannels)
        return true;

      if (m_numChannels < other.m_numChannels)
        return false;

      return m_cost < other.m_cost;
    }

    bool isAdmissible() const
    {
      if (m_numChannels == 0)
        return false;

      return m_cost < wdMath::MaxValue<float>();
    }
  };

  wdMutex s_conversionTableLock;
  wdHashTable<wdUInt32, TableEntry> s_conversionTable;
  bool s_conversionTableValid = false;

  constexpr wdUInt32 MakeKey(wdImageFormat::Enum a, wdImageFormat::Enum b) { return a * wdImageFormat::NUM_FORMATS + b; }
  constexpr wdUInt32 MakeTypeKey(wdImageFormatType::Enum a, wdImageFormatType::Enum b) { return (a << 16) + b; }

  struct IntermediateBuffer
  {
    IntermediateBuffer(wdUInt32 uiBitsPerBlock)
      : m_bitsPerBlock(uiBitsPerBlock)
    {
    }
    wdUInt32 m_bitsPerBlock;
  };

  wdUInt32 allocateScratchBufferIndex(wdHybridArray<IntermediateBuffer, 16>& ref_scratchBuffers, wdUInt32 uiBitsPerBlock, wdUInt32 uiExcludedIndex)
  {
    int foundIndex = -1;

    for (wdUInt32 bufferIndex = 0; bufferIndex < wdUInt32(ref_scratchBuffers.GetCount()); ++bufferIndex)
    {
      if (bufferIndex == uiExcludedIndex)
      {
        continue;
      }

      if (ref_scratchBuffers[bufferIndex].m_bitsPerBlock == uiBitsPerBlock)
      {
        foundIndex = bufferIndex;
        break;
      }
    }

    if (foundIndex >= 0)
    {
      // Reuse existing scratch buffer
      return foundIndex;
    }
    else
    {
      // Allocate new scratch buffer
      ref_scratchBuffers.PushBack(IntermediateBuffer(uiBitsPerBlock));
      return ref_scratchBuffers.GetCount() - 1;
    }
  }
} // namespace

wdImageConversionStep::wdImageConversionStep()
{
  s_conversionTableValid = false;
}

wdImageConversionStep::~wdImageConversionStep()
{
  s_conversionTableValid = false;
}

wdResult wdImageConversion::BuildPath(wdImageFormat::Enum sourceFormat, wdImageFormat::Enum targetFormat, bool bSourceEqualsTarget,
  wdHybridArray<wdImageConversion::ConversionPathNode, 16>& ref_path_out, wdUInt32& ref_uiNumScratchBuffers_out)
{
  WD_LOCK(s_conversionTableLock);

  ref_path_out.Clear();
  ref_uiNumScratchBuffers_out = 0;

  if (sourceFormat == targetFormat)
  {
    ConversionPathNode node;
    node.m_sourceFormat = sourceFormat;
    node.m_targetFormat = targetFormat;
    node.m_inPlace = bSourceEqualsTarget;
    node.m_sourceBufferIndex = 0;
    node.m_targetBufferIndex = 0;
    node.m_step = nullptr;
    ref_path_out.PushBack(node);
    return WD_SUCCESS;
  }

  if (!s_conversionTableValid)
  {
    RebuildConversionTable();
  }

  for (wdImageFormat::Enum current = sourceFormat; current != targetFormat;)
  {
    wdUInt32 currentTableIndex = MakeKey(current, targetFormat);

    TableEntry entry;

    if (!s_conversionTable.TryGetValue(currentTableIndex, entry))
    {
      return WD_FAILURE;
    }

    wdImageConversion::ConversionPathNode step;
    step.m_sourceFormat = entry.m_sourceFormat;
    step.m_targetFormat = entry.m_targetFormat;
    step.m_inPlace = entry.m_flags.IsAnySet(wdImageConversionFlags::InPlace);
    step.m_step = entry.m_step;

    current = entry.m_targetFormat;

    ref_path_out.PushBack(step);
  }

  wdHybridArray<IntermediateBuffer, 16> scratchBuffers;
  scratchBuffers.PushBack(IntermediateBuffer(wdImageFormat::GetBitsPerBlock(targetFormat)));

  for (int i = ref_path_out.GetCount() - 1; i >= 0; --i)
  {
    if (i == ref_path_out.GetCount() - 1)
      ref_path_out[i].m_targetBufferIndex = 0;
    else
      ref_path_out[i].m_targetBufferIndex = ref_path_out[i + 1].m_sourceBufferIndex;

    if (i > 0)
    {
      if (ref_path_out[i].m_inPlace)
      {
        ref_path_out[i].m_sourceBufferIndex = ref_path_out[i].m_targetBufferIndex;
      }
      else
      {
        wdUInt32 bitsPerBlock = wdImageFormat::GetBitsPerBlock(ref_path_out[i].m_sourceFormat);

        ref_path_out[i].m_sourceBufferIndex = allocateScratchBufferIndex(scratchBuffers, bitsPerBlock, ref_path_out[i].m_targetBufferIndex);
      }
    }
  }

  if (bSourceEqualsTarget)
  {
    // Enforce constraint that source == target
    ref_path_out[0].m_sourceBufferIndex = 0;

    // Did we accidentally break the in-place invariant?
    if (ref_path_out[0].m_sourceBufferIndex == ref_path_out[0].m_targetBufferIndex && !ref_path_out[0].m_inPlace)
    {
      if (ref_path_out.GetCount() == 1)
      {
        // Only a single step, so we need to add a copy step
        wdImageConversion::ConversionPathNode copy;
        copy.m_inPlace = false;
        copy.m_sourceFormat = sourceFormat;
        copy.m_targetFormat = sourceFormat;
        copy.m_sourceBufferIndex = ref_path_out[0].m_sourceBufferIndex;
        copy.m_targetBufferIndex =
          allocateScratchBufferIndex(scratchBuffers, wdImageFormat::GetBitsPerBlock(ref_path_out[0].m_sourceFormat), ref_path_out[0].m_sourceBufferIndex);
        ref_path_out[0].m_sourceBufferIndex = copy.m_targetBufferIndex;
        copy.m_step = nullptr;
        ref_path_out.Insert(copy, 0);
      }
      else
      {
        // Turn second step to non-inplace
        ref_path_out[1].m_inPlace = false;
        ref_path_out[1].m_sourceBufferIndex =
          allocateScratchBufferIndex(scratchBuffers, wdImageFormat::GetBitsPerBlock(ref_path_out[1].m_sourceFormat), ref_path_out[0].m_sourceBufferIndex);
        ref_path_out[0].m_targetBufferIndex = ref_path_out[1].m_sourceBufferIndex;
      }
    }
  }
  else
  {
    ref_path_out[0].m_sourceBufferIndex = scratchBuffers.GetCount();
  }

  ref_uiNumScratchBuffers_out = scratchBuffers.GetCount() - 1;

  return WD_SUCCESS;
}

void wdImageConversion::RebuildConversionTable()
{
  WD_LOCK(s_conversionTableLock);

  s_conversionTable.Clear();

  // Prime conversion table with known conversions
  for (wdImageConversionStep* conversion = wdImageConversionStep::GetFirstInstance(); conversion; conversion = conversion->GetNextInstance())
  {
    wdArrayPtr<const wdImageConversionEntry> entries = conversion->GetSupportedConversions();

    for (wdUInt32 subIndex = 0; subIndex < (wdUInt32)entries.GetCount(); subIndex++)
    {
      const wdImageConversionEntry& subConversion = entries[subIndex];

      if (subConversion.m_flags.IsAnySet(wdImageConversionFlags::InPlace))
      {
        WD_ASSERT_DEV(wdImageFormat::IsCompressed(subConversion.m_sourceFormat) == wdImageFormat::IsCompressed(subConversion.m_targetFormat) &&
                        wdImageFormat::GetBitsPerBlock(subConversion.m_sourceFormat) == wdImageFormat::GetBitsPerBlock(subConversion.m_targetFormat),
          "In-place conversions are only allowed between formats of the same number of bits per pixel and compressedness");
      }

      if (wdImageFormat::GetType(subConversion.m_sourceFormat) == wdImageFormatType::PLANAR)
      {
        WD_ASSERT_DEV(wdImageFormat::GetType(subConversion.m_targetFormat) == wdImageFormatType::LINEAR, "Conversions from planar formats must target linear formats");
      }
      else if (wdImageFormat::GetType(subConversion.m_targetFormat) == wdImageFormatType::PLANAR)
      {
        WD_ASSERT_DEV(wdImageFormat::GetType(subConversion.m_sourceFormat) == wdImageFormatType::LINEAR, "Conversions to planar formats must sourced from linear formats");
      }

      wdUInt32 tableIndex = MakeKey(subConversion.m_sourceFormat, subConversion.m_targetFormat);

      // Use the cheapest known conversion for each combination in case there are multiple ones
      TableEntry candidate(conversion, subConversion);

      TableEntry existing;

      if (!s_conversionTable.TryGetValue(tableIndex, existing) || candidate < existing)
      {
        s_conversionTable.Insert(tableIndex, candidate);
      }
    }
  }

  for (wdUInt32 i = 0; i < wdImageFormat::NUM_FORMATS; i++)
  {
    const wdImageFormat::Enum format = static_cast<wdImageFormat::Enum>(i);
    // Add copy-conversion (from and to same format)
    s_conversionTable.Insert(
      MakeKey(format, format), TableEntry(nullptr, wdImageConversionEntry(wdImageConversionEntry(format, format, wdImageConversionFlags::InPlace))));
  }

  // Straight from http://en.wikipedia.org/wiki/Floyd-Warshall_algorithm
  for (wdUInt32 k = 1; k < wdImageFormat::NUM_FORMATS; k++)
  {
    for (wdUInt32 i = 1; i < wdImageFormat::NUM_FORMATS; i++)
    {
      if (k == i)
      {
        continue;
      }

      wdUInt32 tableIndexIK = MakeKey(static_cast<wdImageFormat::Enum>(i), static_cast<wdImageFormat::Enum>(k));

      TableEntry entryIK;
      if (!s_conversionTable.TryGetValue(tableIndexIK, entryIK))
      {
        continue;
      }

      for (wdUInt32 j = 1; j < wdImageFormat::NUM_FORMATS; j++)
      {
        if (j == i || j == k)
        {
          continue;
        }

        wdUInt32 tableIndexIJ = MakeKey(static_cast<wdImageFormat::Enum>(i), static_cast<wdImageFormat::Enum>(j));
        wdUInt32 tableIndexKJ = MakeKey(static_cast<wdImageFormat::Enum>(k), static_cast<wdImageFormat::Enum>(j));

        TableEntry entryKJ;
        if (!s_conversionTable.TryGetValue(tableIndexKJ, entryKJ))
        {
          continue;
        }

        TableEntry candidate = TableEntry::chain(entryIK, entryKJ);

        TableEntry existing;
        if (candidate.isAdmissible() && candidate < s_conversionTable[tableIndexIJ])
        {
          // To Convert from format I to format J, first Convert from I to K
          s_conversionTable[tableIndexIJ] = candidate;
        }
      }
    }
  }

  s_conversionTableValid = true;
}

wdResult wdImageConversion::Convert(const wdImageView& source, wdImage& ref_target, wdImageFormat::Enum targetFormat)
{
  WD_PROFILE_SCOPE("wdImageConversion::Convert");

  wdImageFormat::Enum sourceFormat = source.GetImageFormat();

  // Trivial copy
  if (sourceFormat == targetFormat)
  {
    if (&source != &ref_target)
    {
      // copy if not already the same
      ref_target.ResetAndCopy(source);
    }
    return WD_SUCCESS;
  }

  wdHybridArray<ConversionPathNode, 16> path;
  wdUInt32 numScratchBuffers = 0;
  if (BuildPath(sourceFormat, targetFormat, &source == &ref_target, path, numScratchBuffers).Failed())
  {
    return WD_FAILURE;
  }

  return Convert(source, ref_target, path, numScratchBuffers);
}

wdResult wdImageConversion::Convert(const wdImageView& source, wdImage& ref_target, wdArrayPtr<ConversionPathNode> path, wdUInt32 uiNumScratchBuffers)
{
  WD_ASSERT_DEV(path.GetCount() > 0, "Invalid conversion path");
  WD_ASSERT_DEV(path[0].m_sourceFormat == source.GetImageFormat(), "Invalid conversion path");

  wdHybridArray<wdImage, 16> intermediates;
  intermediates.SetCount(uiNumScratchBuffers);

  const wdImageView* pSource = &source;

  for (wdUInt32 i = 0; i < path.GetCount(); ++i)
  {
    wdUInt32 targetIndex = path[i].m_targetBufferIndex;

    wdImage* pTarget = targetIndex == 0 ? &ref_target : &intermediates[targetIndex - 1];

    if (ConvertSingleStep(path[i].m_step, *pSource, *pTarget, path[i].m_targetFormat).Failed())
    {
      return WD_FAILURE;
    }

    pSource = pTarget;
  }

  return WD_SUCCESS;
}

wdResult wdImageConversion::ConvertRaw(
  wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt32 uiNumElements, wdImageFormat::Enum sourceFormat, wdImageFormat::Enum targetFormat)
{
  if (uiNumElements == 0)
  {
    return WD_SUCCESS;
  }

  // Trivial copy
  if (sourceFormat == targetFormat)
  {
    if (target.GetPtr() != source.GetPtr())
      memcpy(target.GetPtr(), source.GetPtr(), uiNumElements * wdUInt64(wdImageFormat::GetBitsPerPixel(sourceFormat)) / 8);
    return WD_SUCCESS;
  }

  if (wdImageFormat::IsCompressed(sourceFormat) || wdImageFormat::IsCompressed(targetFormat))
  {
    return WD_FAILURE;
  }

  wdHybridArray<ConversionPathNode, 16> path;
  wdUInt32 numScratchBuffers;
  if (BuildPath(sourceFormat, targetFormat, source.GetPtr() == target.GetPtr(), path, numScratchBuffers).Failed())
  {
    return WD_FAILURE;
  }

  return ConvertRaw(source, target, uiNumElements, path, numScratchBuffers);
}

wdResult wdImageConversion::ConvertRaw(
  wdConstByteBlobPtr source, wdByteBlobPtr target, wdUInt32 uiNumElements, wdArrayPtr<ConversionPathNode> path, wdUInt32 uiNumScratchBuffers)
{
  WD_ASSERT_DEV(path.GetCount() > 0, "Path of length 0 is invalid.");

  if (uiNumElements == 0)
  {
    return WD_SUCCESS;
  }

  if (wdImageFormat::IsCompressed(path.GetPtr()->m_sourceFormat) || wdImageFormat::IsCompressed((path.GetEndPtr() - 1)->m_targetFormat))
  {
    return WD_FAILURE;
  }

  wdHybridArray<wdBlob, 16> intermediates;
  intermediates.SetCount(uiNumScratchBuffers);

  for (wdUInt32 i = 0; i < path.GetCount(); ++i)
  {
    wdUInt32 targetIndex = path[i].m_targetBufferIndex;
    wdUInt32 targetBpp = wdImageFormat::GetBitsPerPixel(path[i].m_targetFormat);

    wdByteBlobPtr stepTarget;
    if (targetIndex == 0)
    {
      stepTarget = target;
    }
    else
    {
      wdUInt32 expectedSize = static_cast<wdUInt32>(targetBpp * uiNumElements / 8);
      intermediates[targetIndex - 1].SetCountUninitialized(expectedSize);
      stepTarget = intermediates[targetIndex - 1].GetByteBlobPtr();
    }

    if (path[i].m_step == nullptr)
    {
      memcpy(stepTarget.GetPtr(), source.GetPtr(), uiNumElements * targetBpp / 8);
    }
    else
    {
      if (static_cast<const wdImageConversionStepLinear*>(path[i].m_step)
            ->ConvertPixels(source, stepTarget, uiNumElements, path[i].m_sourceFormat, path[i].m_targetFormat)
            .Failed())
      {
        return WD_FAILURE;
      }
    }

    source = stepTarget;
  }

  return WD_SUCCESS;
}

wdResult wdImageConversion::ConvertSingleStep(
  const wdImageConversionStep* pStep, const wdImageView& source, wdImage& target, wdImageFormat::Enum targetFormat)
{
  if (!pStep)
  {
    target.ResetAndCopy(source);
    return WD_SUCCESS;
  }

  wdImageFormat::Enum sourceFormat = source.GetImageFormat();

  wdImageHeader header = source.GetHeader();
  header.SetImageFormat(targetFormat);
  target.ResetAndAlloc(header);

  switch (MakeTypeKey(wdImageFormat::GetType(sourceFormat), wdImageFormat::GetType(targetFormat)))
  {
    case MakeTypeKey(wdImageFormatType::LINEAR, wdImageFormatType::LINEAR):
    {
      // we have to do the computation in 64-bit otherwise it might overflow for very large textures (8k x 4k or bigger).
      wdUInt64 numElements = wdUInt64(8) * target.GetByteBlobPtr().GetCount() / (wdUInt64)wdImageFormat::GetBitsPerPixel(targetFormat);
      return static_cast<const wdImageConversionStepLinear*>(pStep)->ConvertPixels(
        source.GetByteBlobPtr(), target.GetByteBlobPtr(), (wdUInt32)numElements, sourceFormat, targetFormat);
    }

    case MakeTypeKey(wdImageFormatType::LINEAR, wdImageFormatType::BLOCK_COMPRESSED):
      return ConvertSingleStepCompress(source, target, sourceFormat, targetFormat, pStep);

    case MakeTypeKey(wdImageFormatType::LINEAR, wdImageFormatType::PLANAR):
      return ConvertSingleStepPlanarize(source, target, sourceFormat, targetFormat, pStep);

    case MakeTypeKey(wdImageFormatType::BLOCK_COMPRESSED, wdImageFormatType::LINEAR):
      return ConvertSingleStepDecompress(source, target, sourceFormat, targetFormat, pStep);

    case MakeTypeKey(wdImageFormatType::PLANAR, wdImageFormatType::LINEAR):
      return ConvertSingleStepDeplanarize(source, target, sourceFormat, targetFormat, pStep);

    default:
      WD_ASSERT_NOT_IMPLEMENTED;
      return WD_FAILURE;
  }
}

wdResult wdImageConversion::ConvertSingleStepDecompress(
  const wdImageView& source, wdImage& target, wdImageFormat::Enum sourceFormat, wdImageFormat::Enum targetFormat, const wdImageConversionStep* pStep)
{
  for (wdUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (wdUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      for (wdUInt32 mipLevel = 0; mipLevel < source.GetNumMipLevels(); mipLevel++)
      {
        const wdUInt32 width = target.GetWidth(mipLevel);
        const wdUInt32 height = target.GetHeight(mipLevel);

        const wdUInt32 blockSizeX = wdImageFormat::GetBlockWidth(sourceFormat);
        const wdUInt32 blockSizeY = wdImageFormat::GetBlockHeight(sourceFormat);

        const wdUInt32 numBlocksX = source.GetNumBlocksX(mipLevel);
        const wdUInt32 numBlocksY = source.GetNumBlocksY(mipLevel);

        const wdUInt64 targetRowPitch = target.GetRowPitch(mipLevel);
        const wdUInt32 targetBytesPerPixel = wdImageFormat::GetBitsPerPixel(targetFormat) / 8;

        // Decompress into a temp memory block so we don't have to explicitly handle the case where the image is not a multiple of the block
        // size
        wdHybridArray<wdUInt8, 256> tempBuffer;
        tempBuffer.SetCount(numBlocksX * blockSizeX * blockSizeY * targetBytesPerPixel);

        for (wdUInt32 slice = 0; slice < source.GetDepth(mipLevel); slice++)
        {
          for (wdUInt32 blockY = 0; blockY < numBlocksY; blockY++)
          {
            wdImageView sourceRowView = source.GetRowView(mipLevel, face, arrayIndex, blockY, slice);

            if (static_cast<const wdImageConversionStepDecompressBlocks*>(pStep)
                  ->DecompressBlocks(sourceRowView.GetByteBlobPtr(), wdByteBlobPtr(tempBuffer.GetData(), tempBuffer.GetCount()), numBlocksX,
                    sourceFormat, targetFormat)
                  .Failed())
            {
              return WD_FAILURE;
            }

            for (wdUInt32 blockX = 0; blockX < numBlocksX; blockX++)
            {
              wdUInt8* targetPointer = target.GetPixelPointer<wdUInt8>(mipLevel, face, arrayIndex, blockX * blockSizeX, blockY * blockSizeY, slice);

              // Copy into actual target, clamping to image dimensions
              wdUInt32 copyWidth = wdMath::Min(blockSizeX, width - blockX * blockSizeX);
              wdUInt32 copyHeight = wdMath::Min(blockSizeY, height - blockY * blockSizeY);
              for (wdUInt32 row = 0; row < copyHeight; row++)
              {
                memcpy(targetPointer, &tempBuffer[(blockX * blockSizeX + row) * blockSizeY * targetBytesPerPixel],
                  wdMath::SafeMultiply32(copyWidth, targetBytesPerPixel));
                targetPointer += targetRowPitch;
              }
            }
          }
        }
      }
    }
  }

  return WD_SUCCESS;
}

wdResult wdImageConversion::ConvertSingleStepCompress(
  const wdImageView& source, wdImage& target, wdImageFormat::Enum sourceFormat, wdImageFormat::Enum targetFormat, const wdImageConversionStep* pStep)
{
  for (wdUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (wdUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      for (wdUInt32 mipLevel = 0; mipLevel < source.GetNumMipLevels(); mipLevel++)
      {
        const wdUInt32 sourceWidth = source.GetWidth(mipLevel);
        const wdUInt32 sourceHeight = source.GetHeight(mipLevel);

        const wdUInt32 numBlocksX = target.GetNumBlocksX(mipLevel);
        const wdUInt32 numBlocksY = target.GetNumBlocksY(mipLevel);

        const wdUInt32 targetWidth = numBlocksX * wdImageFormat::GetBlockWidth(targetFormat);
        const wdUInt32 targetHeight = numBlocksY * wdImageFormat::GetBlockHeight(targetFormat);

        const wdUInt64 sourceRowPitch = source.GetRowPitch(mipLevel);
        const wdUInt32 sourceBytesPerPixel = wdImageFormat::GetBitsPerPixel(sourceFormat) / 8;

        // Pad image to multiple of block size for compression
        wdImageHeader paddedSliceHeader;
        paddedSliceHeader.SetWidth(targetWidth);
        paddedSliceHeader.SetHeight(targetHeight);
        paddedSliceHeader.SetImageFormat(sourceFormat);

        wdImage paddedSlice;
        paddedSlice.ResetAndAlloc(paddedSliceHeader);

        for (wdUInt32 slice = 0; slice < source.GetDepth(mipLevel); slice++)
        {
          for (wdUInt32 y = 0; y < targetHeight; ++y)
          {
            wdUInt32 sourceY = wdMath::Min(y, sourceHeight - 1);

            memcpy(paddedSlice.GetPixelPointer<void>(0, 0, 0, 0, y), source.GetPixelPointer<void>(mipLevel, face, arrayIndex, 0, sourceY, slice),
              static_cast<size_t>(sourceRowPitch));

            for (wdUInt32 x = sourceWidth; x < targetWidth; ++x)
            {
              memcpy(paddedSlice.GetPixelPointer<void>(0, 0, 0, x, y),
                source.GetPixelPointer<void>(mipLevel, face, arrayIndex, sourceWidth - 1, sourceY, slice), sourceBytesPerPixel);
            }
          }

          wdResult result = static_cast<const wdImageConversionStepCompressBlocks*>(pStep)->CompressBlocks(paddedSlice.GetByteBlobPtr(),
            target.GetSliceView(mipLevel, face, arrayIndex, slice).GetByteBlobPtr(), numBlocksX, numBlocksY, sourceFormat, targetFormat);

          if (result.Failed())
          {
            return WD_FAILURE;
          }
        }
      }
    }
  }

  return WD_SUCCESS;
}

wdResult wdImageConversion::ConvertSingleStepDeplanarize(
  const wdImageView& source, wdImage& target, wdImageFormat::Enum sourceFormat, wdImageFormat::Enum targetFormat, const wdImageConversionStep* pStep)
{
  for (wdUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (wdUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      for (wdUInt32 mipLevel = 0; mipLevel < source.GetNumMipLevels(); mipLevel++)
      {
        const wdUInt32 width = target.GetWidth(mipLevel);
        const wdUInt32 height = target.GetHeight(mipLevel);

        wdHybridArray<wdImageView, 2> sourcePlanes;
        for (wdUInt32 planeIndex = 0; planeIndex < source.GetPlaneCount(); ++planeIndex)
        {
          const wdUInt32 blockSizeX = wdImageFormat::GetBlockWidth(sourceFormat, planeIndex);
          const wdUInt32 blockSizeY = wdImageFormat::GetBlockHeight(sourceFormat, planeIndex);

          if (width % blockSizeX != 0 || height % blockSizeY != 0)
          {
            // Input image must be aligned to block dimensions already.
            return WD_FAILURE;
          }

          sourcePlanes.PushBack(source.GetPlaneView(mipLevel, face, arrayIndex, planeIndex));
        }

        if (static_cast<const wdImageConversionStepDeplanarize*>(pStep)
              ->ConvertPixels(sourcePlanes, target.GetSubImageView(mipLevel, face, arrayIndex), width, height, sourceFormat, targetFormat)
              .Failed())
        {
          return WD_FAILURE;
        }
      }
    }
  }

  return WD_SUCCESS;
}

wdResult wdImageConversion::ConvertSingleStepPlanarize(
  const wdImageView& source, wdImage& target, wdImageFormat::Enum sourceFormat, wdImageFormat::Enum targetFormat, const wdImageConversionStep* pStep)
{
  for (wdUInt32 arrayIndex = 0; arrayIndex < source.GetNumArrayIndices(); arrayIndex++)
  {
    for (wdUInt32 face = 0; face < source.GetNumFaces(); face++)
    {
      for (wdUInt32 mipLevel = 0; mipLevel < source.GetNumMipLevels(); mipLevel++)
      {
        const wdUInt32 width = target.GetWidth(mipLevel);
        const wdUInt32 height = target.GetHeight(mipLevel);

        wdHybridArray<wdImage, 2> targetPlanes;
        for (wdUInt32 planeIndex = 0; planeIndex < target.GetPlaneCount(); ++planeIndex)
        {
          const wdUInt32 blockSizeX = wdImageFormat::GetBlockWidth(targetFormat, planeIndex);
          const wdUInt32 blockSizeY = wdImageFormat::GetBlockHeight(targetFormat, planeIndex);

          if (width % blockSizeX != 0 || height % blockSizeY != 0)
          {
            // Input image must be aligned to block dimensions already.
            return WD_FAILURE;
          }

          targetPlanes.PushBack(target.GetPlaneView(mipLevel, face, arrayIndex, planeIndex));
        }

        if (static_cast<const wdImageConversionStepPlanarize*>(pStep)
              ->ConvertPixels(source.GetSubImageView(mipLevel, face, arrayIndex), targetPlanes, width, height, sourceFormat, targetFormat)
              .Failed())
        {
          return WD_FAILURE;
        }
      }
    }
  }

  return WD_SUCCESS;
}

bool wdImageConversion::IsConvertible(wdImageFormat::Enum sourceFormat, wdImageFormat::Enum targetFormat)
{
  WD_LOCK(s_conversionTableLock);

  if (!s_conversionTableValid)
  {
    RebuildConversionTable();
  }

  wdUInt32 tableIndex = MakeKey(sourceFormat, targetFormat);
  return s_conversionTable.Contains(tableIndex);
}

wdImageFormat::Enum wdImageConversion::FindClosestCompatibleFormat(
  wdImageFormat::Enum format, wdArrayPtr<const wdImageFormat::Enum> compatibleFormats)
{
  WD_LOCK(s_conversionTableLock);

  if (!s_conversionTableValid)
  {
    RebuildConversionTable();
  }

  TableEntry bestEntry;
  wdImageFormat::Enum bestFormat = wdImageFormat::UNKNOWN;

  for (wdUInt32 targetIndex = 0; targetIndex < wdUInt32(compatibleFormats.GetCount()); targetIndex++)
  {
    wdUInt32 tableIndex = MakeKey(format, compatibleFormats[targetIndex]);
    TableEntry candidate;
    if (s_conversionTable.TryGetValue(tableIndex, candidate) && candidate < bestEntry)
    {
      bestEntry = candidate;
      bestFormat = compatibleFormats[targetIndex];
    }
  }

  return bestFormat;
}

WD_STATICLINK_FILE(Texture, Texture_Image_Implementation_ImageConversion);
