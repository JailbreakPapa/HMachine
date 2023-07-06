#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Texture.h>

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
#  include <Foundation/Utilities/Stats.h>
#endif

wdGPUResourcePool* wdGPUResourcePool::s_pDefaultInstance = nullptr;

wdGPUResourcePool::wdGPUResourcePool()
{
  m_pDevice = wdGALDevice::GetDefaultDevice();

  m_GALDeviceEventSubscriptionID = m_pDevice->m_Events.AddEventHandler(wdMakeDelegate(&wdGPUResourcePool::GALDeviceEventHandler, this));
}

wdGPUResourcePool::~wdGPUResourcePool()
{
  m_pDevice->m_Events.RemoveEventHandler(m_GALDeviceEventSubscriptionID);
  if (!m_TexturesInUse.IsEmpty())
  {
    wdLog::SeriousWarning("Destructing a GPU resource pool of which textures are still in use!");
  }

  // Free remaining resources
  RunGC(0);
}

wdGALTextureHandle wdGPUResourcePool::GetRenderTarget(const wdGALTextureCreationDescription& textureDesc)
{
  WD_LOCK(m_Lock);

  if (!textureDesc.m_bCreateRenderTarget)
  {
    wdLog::Error("Texture description for render target usage has not set bCreateRenderTarget!");
    return wdGALTextureHandle();
  }

  const wdUInt32 uiTextureDescHash = textureDesc.CalculateHash();

  // Check if there is a fitting texture available
  auto it = m_AvailableTextures.Find(uiTextureDescHash);
  if (it.IsValid())
  {
    wdDynamicArray<TextureHandleWithAge>& textures = it.Value();
    if (!textures.IsEmpty())
    {
      wdGALTextureHandle hTexture = textures.PeekBack().m_hTexture;
      textures.PopBack();

      WD_ASSERT_DEV(m_pDevice->GetTexture(hTexture) != nullptr, "Invalid texture in resource pool");

      m_TexturesInUse.Insert(hTexture);

      return hTexture;
    }
  }

  // Since we found no matching texture we need to create a new one, but we check if we should run a GC
  // first since we need to allocate memory now
  CheckAndPotentiallyRunGC();

  wdGALTextureHandle hNewTexture = m_pDevice->CreateTexture(textureDesc);

  if (hNewTexture.IsInvalidated())
  {
    wdLog::Error("GPU resource pool couldn't create new texture for given desc (size: {0} x {1}, format: {2})", textureDesc.m_uiWidth,
      textureDesc.m_uiHeight, textureDesc.m_Format);
    return wdGALTextureHandle();
  }

  // Also track the new created texture
  m_TexturesInUse.Insert(hNewTexture);

  m_uiNumAllocationsSinceLastGC++;
  m_uiCurrentlyAllocatedMemory += m_pDevice->GetMemoryConsumptionForTexture(textureDesc);

  UpdateMemoryStats();

  return hNewTexture;
}

wdGALTextureHandle wdGPUResourcePool::GetRenderTarget(
  wdUInt32 uiWidth, wdUInt32 uiHeight, wdGALResourceFormat::Enum format, wdGALMSAASampleCount::Enum sampleCount, wdUInt32 uiSliceColunt)
{
  wdGALTextureCreationDescription TextureDesc;
  TextureDesc.m_bCreateRenderTarget = true;
  TextureDesc.m_bAllowShaderResourceView = true;
  TextureDesc.m_Format = format;
  TextureDesc.m_Type = wdGALTextureType::Texture2D;
  TextureDesc.m_uiWidth = uiWidth;
  TextureDesc.m_uiHeight = uiHeight;
  TextureDesc.m_SampleCount = sampleCount;
  TextureDesc.m_uiArraySize = uiSliceColunt;

  return GetRenderTarget(TextureDesc);
}

void wdGPUResourcePool::ReturnRenderTarget(wdGALTextureHandle hRenderTarget)
{
  WD_LOCK(m_Lock);

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)

  // First check if this texture actually came from the pool
  if (!m_TexturesInUse.Contains(hRenderTarget))
  {
    wdLog::Error("Returning a texture to the GPU resource pool which wasn't created by the pool is not valid!");
    return;
  }

#endif

  m_TexturesInUse.Remove(hRenderTarget);

  if (const wdGALTexture* pTexture = m_pDevice->GetTexture(hRenderTarget))
  {
    const wdUInt32 uiTextureDescHash = pTexture->GetDescription().CalculateHash();

    auto it = m_AvailableTextures.Find(uiTextureDescHash);
    if (!it.IsValid())
    {
      it = m_AvailableTextures.Insert(uiTextureDescHash, wdDynamicArray<TextureHandleWithAge>());
    }

    it.Value().PushBack({hRenderTarget, wdRenderWorld::GetFrameCounter()});
  }
}

wdGALBufferHandle wdGPUResourcePool::GetBuffer(const wdGALBufferCreationDescription& bufferDesc)
{
  WD_LOCK(m_Lock);

  const wdUInt32 uiBufferDescHash = bufferDesc.CalculateHash();

  // Check if there is a fitting buffer available
  auto it = m_AvailableBuffers.Find(uiBufferDescHash);
  if (it.IsValid())
  {
    wdDynamicArray<BufferHandleWithAge>& buffers = it.Value();
    if (!buffers.IsEmpty())
    {
      wdGALBufferHandle hBuffer = buffers.PeekBack().m_hBuffer;
      buffers.PopBack();

      WD_ASSERT_DEV(m_pDevice->GetBuffer(hBuffer) != nullptr, "Invalid buffer in resource pool");

      m_BuffersInUse.Insert(hBuffer);

      return hBuffer;
    }
  }

  // Since we found no matching buffer we need to create a new one, but we check if we should run a GC
  // first since we need to allocate memory now
  CheckAndPotentiallyRunGC();

  wdGALBufferHandle hNewBuffer = m_pDevice->CreateBuffer(bufferDesc);

  if (hNewBuffer.IsInvalidated())
  {
    wdLog::Error("GPU resource pool couldn't create new buffer for given desc (size: {0})", bufferDesc.m_uiTotalSize);
    return wdGALBufferHandle();
  }

  // Also track the new created buffer
  m_BuffersInUse.Insert(hNewBuffer);

  m_uiNumAllocationsSinceLastGC++;
  m_uiCurrentlyAllocatedMemory += m_pDevice->GetMemoryConsumptionForBuffer(bufferDesc);

  UpdateMemoryStats();

  return hNewBuffer;
}

void wdGPUResourcePool::ReturnBuffer(wdGALBufferHandle hBuffer)
{
  WD_LOCK(m_Lock);

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)

  // First check if this texture actually came from the pool
  if (!m_BuffersInUse.Contains(hBuffer))
  {
    wdLog::Error("Returning a buffer to the GPU resource pool which wasn't created by the pool is not valid!");
    return;
  }

#endif

  m_BuffersInUse.Remove(hBuffer);

  if (const wdGALBuffer* pBuffer = m_pDevice->GetBuffer(hBuffer))
  {
    const wdUInt32 uiBufferDescHash = pBuffer->GetDescription().CalculateHash();

    auto it = m_AvailableBuffers.Find(uiBufferDescHash);
    if (!it.IsValid())
    {
      it = m_AvailableBuffers.Insert(uiBufferDescHash, wdDynamicArray<BufferHandleWithAge>());
    }

    it.Value().PushBack({hBuffer, wdRenderWorld::GetFrameCounter()});
  }
}

void wdGPUResourcePool::RunGC(wdUInt32 uiMinimumAge)
{
  WD_LOCK(m_Lock);

  WD_PROFILE_SCOPE("RunGC");
  wdUInt64 uiCurrentFrame = wdRenderWorld::GetFrameCounter();
  // Destroy all available textures older than uiMinimumAge frames
  {
    for (auto it = m_AvailableTextures.GetIterator(); it.IsValid();)
    {
      auto& textures = it.Value();
      for (wdInt32 i = (wdInt32)textures.GetCount() - 1; i >= 0; i--)
      {
        TextureHandleWithAge& texture = textures[i];
        if (texture.m_uiLastUsed + uiMinimumAge <= uiCurrentFrame)
        {
          if (const wdGALTexture* pTexture = m_pDevice->GetTexture(texture.m_hTexture))
          {
            m_uiCurrentlyAllocatedMemory -= m_pDevice->GetMemoryConsumptionForTexture(pTexture->GetDescription());
          }

          m_pDevice->DestroyTexture(texture.m_hTexture);
          textures.RemoveAtAndCopy(i);
        }
        else
        {
          // The available textures are used as a stack. Thus they are ordered by last used.
          break;
        }
      }
      if (textures.IsEmpty())
      {
        auto itCopy = it;
        ++it;
        m_AvailableTextures.Remove(itCopy);
      }
      else
      {
        ++it;
      }
    }
  }

  // Destroy all available buffers older than uiMinimumAge frames
  {
    for (auto it = m_AvailableBuffers.GetIterator(); it.IsValid();)
    {
      auto& buffers = it.Value();
      for (wdInt32 i = (wdInt32)buffers.GetCount() - 1; i >= 0; i--)
      {
        BufferHandleWithAge& buffer = buffers[i];
        if (buffer.m_uiLastUsed + uiMinimumAge <= uiCurrentFrame)
        {
          if (const wdGALBuffer* pBuffer = m_pDevice->GetBuffer(buffer.m_hBuffer))
          {
            m_uiCurrentlyAllocatedMemory -= m_pDevice->GetMemoryConsumptionForBuffer(pBuffer->GetDescription());
          }

          m_pDevice->DestroyBuffer(buffer.m_hBuffer);
          buffers.RemoveAtAndCopy(i);
        }
        else
        {
          // The available buffers are used as a stack. Thus they are ordered by last used.
          break;
        }
      }
      if (buffers.IsEmpty())
      {
        auto itCopy = it;
        ++it;
        m_AvailableBuffers.Remove(itCopy);
      }
      else
      {
        ++it;
      }
    }
  }

  m_uiNumAllocationsSinceLastGC = 0;

  UpdateMemoryStats();
}



wdGPUResourcePool* wdGPUResourcePool::GetDefaultInstance()
{
  return s_pDefaultInstance;
}

void wdGPUResourcePool::SetDefaultInstance(wdGPUResourcePool* pDefaultInstance)
{
  WD_DEFAULT_DELETE(s_pDefaultInstance);
  s_pDefaultInstance = pDefaultInstance;
}


void wdGPUResourcePool::CheckAndPotentiallyRunGC()
{
  if ((m_uiNumAllocationsSinceLastGC >= m_uiNumAllocationsThresholdForGC) || (m_uiCurrentlyAllocatedMemory >= m_uiMemoryThresholdForGC))
  {
    // Only try to collect resources unused for 3 or more frames. Using a smaller number will result in constant memory thrashing.
    RunGC(3);
  }
}

void wdGPUResourcePool::UpdateMemoryStats() const
{
#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)

  float fMegaBytes = float(m_uiCurrentlyAllocatedMemory) / (1024.0f * 1024.0f);

  wdStringBuilder sOut;
  sOut.Format("{0} (Mb)", wdArgF(fMegaBytes, 4));
  wdStats::SetStat("GPU Resource Pool/Memory Consumption", sOut.GetData());

#endif
}

void wdGPUResourcePool::GALDeviceEventHandler(const wdGALDeviceEvent& e)
{
  if (e.m_Type == wdGALDeviceEvent::AfterEndFrame)
  {
    ++m_uiFramesSinceLastGC;
    if (m_uiFramesSinceLastGC >= m_uiFramesThresholdSinceLastGC)
    {
      m_uiFramesSinceLastGC = 0;
      RunGC(10);
    }
  }
}

WD_STATICLINK_FILE(RendererCore, RendererCore_GPUResourcePool_Implementation_GPUResourcePool);
