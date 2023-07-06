#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <RendererCore/AnimationSystem/Implementation/OzzUtils.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/io/archive.h>

wdOzzArchiveData::wdOzzArchiveData() = default;
wdOzzArchiveData::~wdOzzArchiveData() = default;

wdResult wdOzzArchiveData::FetchRegularFile(const char* szFile)
{
  wdFileReader file;
  WD_SUCCEED_OR_RETURN(file.Open(szFile));

  m_Storage.Clear();
  m_Storage.Reserve(file.GetFileSize());
  m_Storage.ReadAll(file);

  return WD_SUCCESS;
}

wdResult wdOzzArchiveData::FetchEmbeddedArchive(wdStreamReader& inout_stream)
{
  char szTag[8] = "";

  inout_stream.ReadBytes(szTag, 8);
  szTag[7] = '\0';

  if (!wdStringUtils::IsEqual(szTag, "wdOzzAr"))
    return WD_FAILURE;

  /*const wdTypeVersion version =*/inout_stream.ReadVersion(1);

  wdUInt64 uiArchiveSize = 0;
  inout_stream >> uiArchiveSize;

  m_Storage.Clear();
  m_Storage.Reserve(uiArchiveSize);
  m_Storage.ReadAll(inout_stream, uiArchiveSize);

  if (m_Storage.GetStorageSize64() != uiArchiveSize)
    return WD_FAILURE;

  return WD_SUCCESS;
}

wdResult wdOzzArchiveData::StoreEmbeddedArchive(wdStreamWriter& inout_stream) const
{
  const char szTag[8] = "wdOzzAr";

  WD_SUCCEED_OR_RETURN(inout_stream.WriteBytes(szTag, 8));

  inout_stream.WriteVersion(1);

  const wdUInt64 uiArchiveSize = m_Storage.GetStorageSize64();

  inout_stream << uiArchiveSize;

  return m_Storage.CopyToStream(inout_stream);
}

wdOzzStreamReader::wdOzzStreamReader(const wdOzzArchiveData& data)
  : m_Reader(&data.m_Storage)
{
}

bool wdOzzStreamReader::opened() const
{
  return true;
}

size_t wdOzzStreamReader::Read(void* pBuffer, size_t uiSize)
{
  return static_cast<size_t>(m_Reader.ReadBytes(pBuffer, uiSize));
}

size_t wdOzzStreamReader::Write(const void* pBuffer, size_t uiSize)
{
  WD_ASSERT_NOT_IMPLEMENTED;
  return 0;
}

int wdOzzStreamReader::Seek(int iOffset, Origin origin)
{
  switch (origin)
  {
    case ozz::io::Stream::kCurrent:
      m_Reader.SetReadPosition(m_Reader.GetReadPosition() + iOffset);
      break;
    case ozz::io::Stream::kEnd:
      m_Reader.SetReadPosition(m_Reader.GetByteCount64() - iOffset);
      break;
    case ozz::io::Stream::kSet:
      m_Reader.SetReadPosition(iOffset);
      break;

      WD_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return 0;
}

int wdOzzStreamReader::Tell() const
{
  return static_cast<int>(m_Reader.GetReadPosition());
}

size_t wdOzzStreamReader::Size() const
{
  return static_cast<size_t>(m_Reader.GetByteCount64());
}

wdOzzStreamWriter::wdOzzStreamWriter(wdOzzArchiveData& ref_data)
  : m_Writer(&ref_data.m_Storage)
{
}

bool wdOzzStreamWriter::opened() const
{
  return true;
}

size_t wdOzzStreamWriter::Read(void* pBuffer, size_t uiSize)
{
  WD_ASSERT_NOT_IMPLEMENTED;
  return 0;
}

size_t wdOzzStreamWriter::Write(const void* pBuffer, size_t uiSize)
{
  if (m_Writer.WriteBytes(pBuffer, uiSize).Failed())
    return 0;

  return uiSize;
}

int wdOzzStreamWriter::Seek(int iOffset, Origin origin)
{
  switch (origin)
  {
    case ozz::io::Stream::kCurrent:
      m_Writer.SetWritePosition(m_Writer.GetWritePosition() + iOffset);
      break;
    case ozz::io::Stream::kEnd:
      m_Writer.SetWritePosition(m_Writer.GetByteCount64() - iOffset);
      break;
    case ozz::io::Stream::kSet:
      m_Writer.SetWritePosition(iOffset);
      break;

      WD_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return 0;
}

int wdOzzStreamWriter::Tell() const
{
  return static_cast<int>(m_Writer.GetWritePosition());
}

size_t wdOzzStreamWriter::Size() const
{
  return static_cast<size_t>(m_Writer.GetByteCount64());
}

void wdOzzUtils::CopyAnimation(ozz::animation::Animation* pDst, const ozz::animation::Animation* pSrc)
{
  wdOzzArchiveData ozzArchiveData;

  // store in ozz archive
  {
    wdOzzStreamWriter ozzWriter(ozzArchiveData);
    ozz::io::OArchive ozzArchive(&ozzWriter);

    ozzArchive << *pSrc;
  }

  // read it from archive again
  {
    wdOzzStreamReader ozzReader(ozzArchiveData);
    ozz::io::IArchive ozzArchive(&ozzReader);

    ozzArchive >> *pDst;
  }
}

WD_RENDERERCORE_DLL void wdOzzUtils::CopySkeleton(ozz::animation::Skeleton* pDst, const ozz::animation::Skeleton* pSrc)
{
  wdOzzArchiveData ozzArchiveData;

  // store in ozz archive
  {
    wdOzzStreamWriter ozzWriter(ozzArchiveData);
    ozz::io::OArchive ozzArchive(&ozzWriter);

    ozzArchive << *pSrc;
  }

  // read it from archive again
  {
    wdOzzStreamReader ozzReader(ozzArchiveData);
    ozz::io::IArchive ozzArchive(&ozzReader);

    ozzArchive >> *pDst;
  }
}


WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_OzzUtils);
