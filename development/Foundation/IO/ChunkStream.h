#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/IO/Stream.h>

/// \brief A stream writer that separates data into 'chunks', which act like sub-streams.
///
/// This stream writer allows to subdivide a stream into chunks, where each chunk stores a chunk name,
/// version and size in bytes.
class WD_FOUNDATION_DLL wdChunkStreamWriter : public wdStreamWriter
{
public:
  /// \brief Pass the underlying stream writer to the constructor.
  wdChunkStreamWriter(wdStreamWriter& inout_stream); // [tested]

  /// \brief Writes bytes directly to the stream. Only allowed when a chunk is open (between BeginChunk / EndChunk).
  virtual wdResult WriteBytes(const void* pWriteBuffer, wdUInt64 uiBytesToWrite) override; // [tested]

  /// \brief Starts writing to the chunk file. Has to be the first thing that is called. The version number is written to the stream and is returned
  /// by wdChunkStreamReader::BeginStream()
  virtual void BeginStream(wdUInt16 uiVersion); // [tested]

  /// \brief Stops writing to the chunk file. Has to be the last thing that is called.
  virtual void EndStream(); // [tested]

  /// \brief Opens the next chunk for writing. Chunks cannot be nested (except by using multiple chunk format writers).
  virtual void BeginChunk(wdStringView sName, wdUInt32 uiVersion); // [tested]

  /// \brief Closes the current chunk.
  virtual void EndChunk(); // [tested]


private:
  bool m_bWritingFile;
  bool m_bWritingChunk;
  wdString m_sChunkName;
  wdDeque<wdUInt8> m_Storage;
  wdStreamWriter& m_Stream;
};


/// \brief Reader for the chunk format that wdChunkStreamWriter writes.
///
///
class WD_FOUNDATION_DLL wdChunkStreamReader : public wdStreamReader
{
public:
  /// \brief Pass the underlying stream writer to the constructor.
  wdChunkStreamReader(wdStreamReader& inout_stream); // [tested]

  /// \brief Reads bytes directly from the stream. Only allowed while a valid chunk is available.
  /// Returns 0 bytes when the end of a chunk is reached, even if there are more chunks to come.
  virtual wdUInt64 ReadBytes(void* pReadBuffer, wdUInt64 uiBytesToRead) override; // [tested]

  enum class EndChunkFileMode
  {
    SkipToEnd, ///< Makes sure all data is properly read, so that the stream read position is after the chunk file data. Useful if the chunk file is
               ///< embedded in another file stream.
    JustClose  ///< Just stops, leaving the stream at the last read position. This should be used if definitely nothing more needs to be read from all
               ///< underlying streams.
  };

  void SetEndChunkFileMode(EndChunkFileMode mode) { m_EndChunkFileMode = mode; } // [tested]

  /// \brief Starts reading from the chunk file. Returns the version number that was passed to wdChunkStreamWriter::BeginStream().
  virtual wdUInt16 BeginStream(); // [tested]

  /// \brief Stops reading from the chunk file. Optionally skips the remaining bytes, so that the underlying streams read position is after the chunk
  /// file content.
  virtual void EndStream(); // [tested]

  /// \brief Describes the state of the current chunk.
  struct ChunkInfo
  {
    ChunkInfo()
    {
      m_bValid = false;
      m_uiChunkVersion = 0;
      m_uiChunkBytes = 0;
      m_uiUnreadChunkBytes = 0;
    }

    bool m_bValid;                 ///< If this is false, the end of the chunk file has been reached and no further chunk is available.
    wdString m_sChunkName;         ///< The name of the chunk.
    wdUInt32 m_uiChunkVersion;     ///< The version number of the chunk.
    wdUInt32 m_uiChunkBytes;       ///< The total size of the chunk.
    wdUInt32 m_uiUnreadChunkBytes; ///< The number of bytes in the chunk that have not yet been read.
  };

  /// \brief Returns information about the current chunk.
  const ChunkInfo& GetCurrentChunk() const { return m_ChunkInfo; } // [tested]

  /// \brief Skips the rest of the current chunk and starts reading the next chunk.
  void NextChunk(); // [tested]

private:
  void TryReadChunkHeader();

  EndChunkFileMode m_EndChunkFileMode;
  ChunkInfo m_ChunkInfo;

  wdStreamReader& m_Stream;
};
