#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Threading/Mutex.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>

struct wdGALDeviceEvent;

/// \brief This class serves as a pool for GPU related resources (e.g. buffers and textures required for rendering).
/// Note that the functions creating and returning render targets are thread safe (by using a mutex).
class WD_RENDERERCORE_DLL wdGPUResourcePool
{
public:
  wdGPUResourcePool();
  ~wdGPUResourcePool();

  /// \brief Returns a render target handle for the given texture description
  /// Note that you should return the handle to the pool and never destroy it directly with the device.
  wdGALTextureHandle GetRenderTarget(const wdGALTextureCreationDescription& textureDesc);

  /// \brief Convenience functions which creates a texture description fit for a 2d render target without a mip chains.
  wdGALTextureHandle GetRenderTarget(wdUInt32 uiWidth, wdUInt32 uiHeight, wdGALResourceFormat::Enum format,
    wdGALMSAASampleCount::Enum sampleCount = wdGALMSAASampleCount::None, wdUInt32 uiSliceColunt = 1);

  /// \brief Returns a render target to the pool so other consumers can use it.
  /// Note that targets which are returned to the pool are susceptible to destruction due to garbage collection.
  void ReturnRenderTarget(wdGALTextureHandle hRenderTarget);


  /// \brief Returns a buffer handle for the given buffer description
  wdGALBufferHandle GetBuffer(const wdGALBufferCreationDescription& bufferDesc);

  /// \brief Returns a buffer to the pool so other consumers can use it.
  void ReturnBuffer(wdGALBufferHandle hBuffer);


  /// \brief Tries to free resources which are currently in the pool.
  /// Triggered automatically due to allocation number / size thresholds but can be triggered manually (e.g. after editor window resize)
  ///
  /// \param uiMinimumAge How many frames at least the resource needs to have been unused before it will be GCed.
  void RunGC(wdUInt32 uiMinimumAge);


  static wdGPUResourcePool* GetDefaultInstance();
  static void SetDefaultInstance(wdGPUResourcePool* pDefaultInstance);

protected:
  void CheckAndPotentiallyRunGC();
  void UpdateMemoryStats() const;
  void GALDeviceEventHandler(const wdGALDeviceEvent& e);

  struct TextureHandleWithAge
  {
    wdGALTextureHandle m_hTexture;
    wdUInt64 m_uiLastUsed = 0;
  };

  struct BufferHandleWithAge
  {
    wdGALBufferHandle m_hBuffer;
    wdUInt64 m_uiLastUsed = 0;
  };

  wdEventSubscriptionID m_GALDeviceEventSubscriptionID = 0;
  wdUInt64 m_uiMemoryThresholdForGC = 256 * 1024 * 1024;
  wdUInt64 m_uiCurrentlyAllocatedMemory = 0;
  wdUInt16 m_uiNumAllocationsThresholdForGC = 128;
  wdUInt16 m_uiNumAllocationsSinceLastGC = 0;
  wdUInt16 m_uiFramesThresholdSinceLastGC = 60; ///< Every 60 frames resources unused for more than 10 frames in a row are GCed.
  wdUInt16 m_uiFramesSinceLastGC = 0;

  wdMap<wdUInt32, wdDynamicArray<TextureHandleWithAge>> m_AvailableTextures;
  wdSet<wdGALTextureHandle> m_TexturesInUse;

  wdMap<wdUInt32, wdDynamicArray<BufferHandleWithAge>> m_AvailableBuffers;
  wdSet<wdGALBufferHandle> m_BuffersInUse;

  wdMutex m_Lock;

  wdGALDevice* m_pDevice;

private:
  static wdGPUResourcePool* s_pDefaultInstance;
};
