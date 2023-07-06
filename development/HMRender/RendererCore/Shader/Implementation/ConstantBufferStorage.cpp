#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>

wdConstantBufferStorageBase::wdConstantBufferStorageBase(wdUInt32 uiSizeInBytes)

{
  m_Data = wdMakeArrayPtr(static_cast<wdUInt8*>(wdFoundation::GetAlignedAllocator()->Allocate(uiSizeInBytes, 16)), uiSizeInBytes);
  wdMemoryUtils::ZeroFill(m_Data.GetPtr(), m_Data.GetCount());

  m_hGALConstantBuffer = wdGALDevice::GetDefaultDevice()->CreateConstantBuffer(uiSizeInBytes);
}

wdConstantBufferStorageBase::~wdConstantBufferStorageBase()
{
  wdGALDevice::GetDefaultDevice()->DestroyBuffer(m_hGALConstantBuffer);

  wdFoundation::GetAlignedAllocator()->Deallocate(m_Data.GetPtr());
  m_Data.Clear();
}

wdArrayPtr<wdUInt8> wdConstantBufferStorageBase::GetRawDataForWriting()
{
  m_bHasBeenModified = true;
  return m_Data;
}

wdArrayPtr<const wdUInt8> wdConstantBufferStorageBase::GetRawDataForReading() const
{
  return m_Data;
}

void wdConstantBufferStorageBase::UploadData(wdGALCommandEncoder* pCommandEncoder)
{
  if (!m_bHasBeenModified)
    return;

  m_bHasBeenModified = false;

  wdUInt32 uiNewHash = wdHashingUtils::xxHash32(m_Data.GetPtr(), m_Data.GetCount());
  if (m_uiLastHash != uiNewHash)
  {
    pCommandEncoder->UpdateBuffer(m_hGALConstantBuffer, 0, m_Data);
    m_uiLastHash = uiNewHash;
  }
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Shader_Implementation_ConstantBufferStorage);
