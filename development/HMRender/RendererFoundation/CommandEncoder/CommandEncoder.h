
#pragma once

#include <Foundation/Threading/ThreadUtils.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>

class WD_RENDERERFOUNDATION_DLL wdGALCommandEncoder
{
  WD_DISALLOW_COPY_AND_ASSIGN(wdGALCommandEncoder);

public:
  // State setting functions

  void SetShader(wdGALShaderHandle hShader);

  void SetConstantBuffer(wdUInt32 uiSlot, wdGALBufferHandle hBuffer);
  void SetSamplerState(wdGALShaderStage::Enum stage, wdUInt32 uiSlot, wdGALSamplerStateHandle hSamplerState);
  void SetResourceView(wdGALShaderStage::Enum stage, wdUInt32 uiSlot, wdGALResourceViewHandle hResourceView);
  void SetUnorderedAccessView(wdUInt32 uiSlot, wdGALUnorderedAccessViewHandle hUnorderedAccessView);

  // Returns whether a resource view has been unset for the given resource
  bool UnsetResourceViews(const wdGALResourceBase* pResource);
  // Returns whether a unordered access view has been unset for the given resource
  bool UnsetUnorderedAccessViews(const wdGALResourceBase* pResource);

  // Query functions

  void BeginQuery(wdGALQueryHandle hQuery);
  void EndQuery(wdGALQueryHandle hQuery);

  /// \return Success if retrieving the query succeeded.
  wdResult GetQueryResult(wdGALQueryHandle hQuery, wdUInt64& ref_uiQueryResult);

  // Timestamp functions

  wdGALTimestampHandle InsertTimestamp();

  // Resource functions

  /// Clears an unordered access view with a float value.
  void ClearUnorderedAccessView(wdGALUnorderedAccessViewHandle hUnorderedAccessView, wdVec4 vClearValues);

  /// Clears an unordered access view with an int value.
  void ClearUnorderedAccessView(wdGALUnorderedAccessViewHandle hUnorderedAccessView, wdVec4U32 vClearValues);

  void CopyBuffer(wdGALBufferHandle hDest, wdGALBufferHandle hSource);
  void CopyBufferRegion(wdGALBufferHandle hDest, wdUInt32 uiDestOffset, wdGALBufferHandle hSource, wdUInt32 uiSourceOffset, wdUInt32 uiByteCount);
  void UpdateBuffer(wdGALBufferHandle hDest, wdUInt32 uiDestOffset, wdArrayPtr<const wdUInt8> sourceData, wdGALUpdateMode::Enum updateMode = wdGALUpdateMode::Discard);

  void CopyTexture(wdGALTextureHandle hDest, wdGALTextureHandle hSource);
  void CopyTextureRegion(wdGALTextureHandle hDest, const wdGALTextureSubresource& destinationSubResource, const wdVec3U32& vDestinationPoint, wdGALTextureHandle hSource, const wdGALTextureSubresource& sourceSubResource, const wdBoundingBoxu32& box);

  void UpdateTexture(wdGALTextureHandle hDest, const wdGALTextureSubresource& destinationSubResource, const wdBoundingBoxu32& destinationBox, const wdGALSystemMemoryDescription& sourceData);

  void ResolveTexture(wdGALTextureHandle hDest, const wdGALTextureSubresource& destinationSubResource, wdGALTextureHandle hSource, const wdGALTextureSubresource& sourceSubResource);

  void ReadbackTexture(wdGALTextureHandle hTexture);
  void CopyTextureReadbackResult(wdGALTextureHandle hTexture, wdArrayPtr<wdGALTextureSubresource> sourceSubResource, wdArrayPtr<wdGALSystemMemoryDescription> targetData);

  void GenerateMipMaps(wdGALResourceViewHandle hResourceView);

  // Misc

  void Flush();

  // Debug helper functions

  void PushMarker(const char* szMarker);
  void PopMarker();
  void InsertEventMarker(const char* szMarker);

  virtual void ClearStatisticsCounters();

  WD_ALWAYS_INLINE wdGALDevice& GetDevice() { return m_Device; }
  // Don't use light hearted ;)
  void InvalidateState();

protected:
  friend class wdGALDevice;

  wdGALCommandEncoder(wdGALDevice& device, wdGALCommandEncoderState& state, wdGALCommandEncoderCommonPlatformInterface& commonImpl);
  virtual ~wdGALCommandEncoder();


  void AssertRenderingThread()
  {
    WD_ASSERT_DEV(wdThreadUtils::IsMainThread(), "This function can only be executed on the main thread.");
  }

  void CountStateChange() { m_uiStateChanges++; }
  void CountRedundantStateChange() { m_uiRedundantStateChanges++; }

private:
  friend class wdMemoryUtils;

  // Parent Device
  wdGALDevice& m_Device;

  // Statistic variables
  wdUInt32 m_uiStateChanges = 0;
  wdUInt32 m_uiRedundantStateChanges = 0;

  wdGALCommandEncoderState& m_State;

  wdGALCommandEncoderCommonPlatformInterface& m_CommonImpl;
};
