#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class WD_RENDERERCORE_DLL wdConstantBufferStorageBase
{
protected:
  friend class wdRenderContext;
  friend class wdMemoryUtils;

  wdConstantBufferStorageBase(wdUInt32 uiSizeInBytes);
  ~wdConstantBufferStorageBase();

public:
  wdArrayPtr<wdUInt8> GetRawDataForWriting();
  wdArrayPtr<const wdUInt8> GetRawDataForReading() const;

  void UploadData(wdGALCommandEncoder* pCommandEncoder);

  WD_ALWAYS_INLINE wdGALBufferHandle GetGALBufferHandle() const { return m_hGALConstantBuffer; }

protected:
  bool m_bHasBeenModified = false;
  wdUInt32 m_uiLastHash = 0;
  wdGALBufferHandle m_hGALConstantBuffer;

  wdArrayPtr<wdUInt8> m_Data;
};

template <typename T>
class wdConstantBufferStorage : public wdConstantBufferStorageBase
{
public:
  WD_FORCE_INLINE T& GetDataForWriting()
  {
    wdArrayPtr<wdUInt8> rawData = GetRawDataForWriting();
    WD_ASSERT_DEV(rawData.GetCount() == sizeof(T), "Invalid data size");
    return *reinterpret_cast<T*>(rawData.GetPtr());
  }

  WD_FORCE_INLINE const T& GetDataForReading() const
  {
    wdArrayPtr<const wdUInt8> rawData = GetRawDataForReading();
    WD_ASSERT_DEV(rawData.GetCount() == sizeof(T), "Invalid data size");
    return *reinterpret_cast<const T*>(rawData.GetPtr());
  }
};

using wdConstantBufferStorageId = wdGenericId<24, 8>;

class wdConstantBufferStorageHandle
{
  WD_DECLARE_HANDLE_TYPE(wdConstantBufferStorageHandle, wdConstantBufferStorageId);

  friend class wdRenderContext;
};
