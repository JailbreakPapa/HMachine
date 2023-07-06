#pragma once

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>
#include <RendererCore/RendererCoreDLL.h>
#include <ozz/base/io/stream.h>

namespace ozz::animation
{
  class Skeleton;
  class Animation;
}; // namespace ozz::animation

/// \brief Stores or gather the data for an ozz file, for random access operations (seek / tell).
///
/// Since ozz::io::Stream requires seek/tell functionality, it cannot be implemented with basic wdStreamReader / wdStreamWriter.
/// Instead, we must have the entire ozz archive data in memory, to be able to jump around arbitrarily.
class WD_RENDERERCORE_DLL wdOzzArchiveData
{
  WD_DISALLOW_COPY_AND_ASSIGN(wdOzzArchiveData);

public:
  wdOzzArchiveData();
  ~wdOzzArchiveData();

  wdResult FetchRegularFile(const char* szFile);
  wdResult FetchEmbeddedArchive(wdStreamReader& inout_stream);
  wdResult StoreEmbeddedArchive(wdStreamWriter& inout_stream) const;

  wdDefaultMemoryStreamStorage m_Storage;
};

/// \brief Implements the ozz::io::Stream interface for reading. The data has to be present in an wdOzzArchiveData object.
///
/// The class is implemented inline and not DLL exported because ozz is only available as a static library.
class WD_RENDERERCORE_DLL wdOzzStreamReader : public ozz::io::Stream
{
public:
  wdOzzStreamReader(const wdOzzArchiveData& data);

  virtual bool opened() const override;

  virtual size_t Read(void* pBuffer, size_t uiSize) override;

  virtual size_t Write(const void* pBuffer, size_t uiSize) override;

  virtual int Seek(int iOffset, Origin origin) override;

  virtual int Tell() const override;

  virtual size_t Size() const override;

private:
  wdMemoryStreamReader m_Reader;
};

/// \brief Implements the ozz::io::Stream interface for writing. The data is gathered in an wdOzzArchiveData object.
///
/// The class is implemented inline and not DLL exported because ozz is only available as a static library.
class WD_RENDERERCORE_DLL wdOzzStreamWriter : public ozz::io::Stream
{
public:
  wdOzzStreamWriter(wdOzzArchiveData& ref_data);

  virtual bool opened() const override;

  virtual size_t Read(void* pBuffer, size_t uiSize) override;

  virtual size_t Write(const void* pBuffer, size_t uiSize) override;

  virtual int Seek(int iOffset, Origin origin) override;

  virtual int Tell() const override;

  virtual size_t Size() const override;

private:
  wdMemoryStreamWriter m_Writer;
};

namespace wdOzzUtils
{
  WD_RENDERERCORE_DLL void CopyAnimation(ozz::animation::Animation* pDst, const ozz::animation::Animation* pSrc);
  WD_RENDERERCORE_DLL void CopySkeleton(ozz::animation::Skeleton* pDst, const ozz::animation::Skeleton* pSrc);
} // namespace wdOzzUtils
