
#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Rect.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class WD_RENDERERFOUNDATION_DLL wdGALCommandEncoderCommonPlatformInterface
{
public:
  // State setting functions

  virtual void SetShaderPlatform(const wdGALShader* pShader) = 0;

  virtual void SetConstantBufferPlatform(wdUInt32 uiSlot, const wdGALBuffer* pBuffer) = 0;
  virtual void SetSamplerStatePlatform(wdGALShaderStage::Enum stage, wdUInt32 uiSlot, const wdGALSamplerState* pSamplerState) = 0;
  virtual void SetResourceViewPlatform(wdGALShaderStage::Enum stage, wdUInt32 uiSlot, const wdGALResourceView* pResourceView) = 0;
  virtual void SetUnorderedAccessViewPlatform(wdUInt32 uiSlot, const wdGALUnorderedAccessView* pUnorderedAccessView) = 0;

  // Query functions

  virtual void BeginQueryPlatform(const wdGALQuery* pQuery) = 0;
  virtual void EndQueryPlatform(const wdGALQuery* pQuery) = 0;
  virtual wdResult GetQueryResultPlatform(const wdGALQuery* pQuery, wdUInt64& ref_uiQueryResult) = 0;

  // Timestamp functions

  virtual void InsertTimestampPlatform(wdGALTimestampHandle hTimestamp) = 0;

  // Resource update functions

  virtual void ClearUnorderedAccessViewPlatform(const wdGALUnorderedAccessView* pUnorderedAccessView, wdVec4 vClearValues) = 0;
  virtual void ClearUnorderedAccessViewPlatform(const wdGALUnorderedAccessView* pUnorderedAccessView, wdVec4U32 vClearValues) = 0;

  virtual void CopyBufferPlatform(const wdGALBuffer* pDestination, const wdGALBuffer* pSource) = 0;
  virtual void CopyBufferRegionPlatform(const wdGALBuffer* pDestination, wdUInt32 uiDestOffset, const wdGALBuffer* pSource, wdUInt32 uiSourceOffset, wdUInt32 uiByteCount) = 0;

  virtual void UpdateBufferPlatform(const wdGALBuffer* pDestination, wdUInt32 uiDestOffset, wdArrayPtr<const wdUInt8> sourceData, wdGALUpdateMode::Enum updateMode) = 0;

  virtual void CopyTexturePlatform(const wdGALTexture* pDestination, const wdGALTexture* pSource) = 0;
  virtual void CopyTextureRegionPlatform(const wdGALTexture* pDestination, const wdGALTextureSubresource& destinationSubResource, const wdVec3U32& vDestinationPoint, const wdGALTexture* pSource, const wdGALTextureSubresource& sourceSubResource, const wdBoundingBoxu32& box) = 0;

  virtual void UpdateTexturePlatform(const wdGALTexture* pDestination, const wdGALTextureSubresource& destinationSubResource, const wdBoundingBoxu32& destinationBox, const wdGALSystemMemoryDescription& sourceData) = 0;

  virtual void ResolveTexturePlatform(const wdGALTexture* pDestination, const wdGALTextureSubresource& destinationSubResource, const wdGALTexture* pSource, const wdGALTextureSubresource& sourceSubResource) = 0;

  virtual void ReadbackTexturePlatform(const wdGALTexture* pTexture) = 0;

  virtual void CopyTextureReadbackResultPlatform(const wdGALTexture* pTexture, wdArrayPtr<wdGALTextureSubresource> sourceSubResource, wdArrayPtr<wdGALSystemMemoryDescription> targetData) = 0;

  virtual void GenerateMipMapsPlatform(const wdGALResourceView* pResourceView) = 0;

  // Misc

  virtual void FlushPlatform() = 0;

  // Debug helper functions

  virtual void PushMarkerPlatform(const char* szMarker) = 0;
  virtual void PopMarkerPlatform() = 0;
  virtual void InsertEventMarkerPlatform(const char* szMarker) = 0;
};

class WD_RENDERERFOUNDATION_DLL wdGALCommandEncoderRenderPlatformInterface
{
public:
  // Draw functions

  virtual void ClearPlatform(const wdColor& clearColor, wdUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, wdUInt8 uiStencilClear) = 0;

  virtual void DrawPlatform(wdUInt32 uiVertexCount, wdUInt32 uiStartVertex) = 0;
  virtual void DrawIndexedPlatform(wdUInt32 uiIndexCount, wdUInt32 uiStartIndex) = 0;
  virtual void DrawIndexedInstancedPlatform(wdUInt32 uiIndexCountPerInstance, wdUInt32 uiInstanceCount, wdUInt32 uiStartIndex) = 0;
  virtual void DrawIndexedInstancedIndirectPlatform(const wdGALBuffer* pIndirectArgumentBuffer, wdUInt32 uiArgumentOffsetInBytes) = 0;
  virtual void DrawInstancedPlatform(wdUInt32 uiVertexCountPerInstance, wdUInt32 uiInstanceCount, wdUInt32 uiStartVertex) = 0;
  virtual void DrawInstancedIndirectPlatform(const wdGALBuffer* pIndirectArgumentBuffer, wdUInt32 uiArgumentOffsetInBytes) = 0;
  virtual void DrawAutoPlatform() = 0;

  virtual void BeginStreamOutPlatform() = 0;
  virtual void EndStreamOutPlatform() = 0;

  // State functions

  virtual void SetIndexBufferPlatform(const wdGALBuffer* pIndexBuffer) = 0;
  virtual void SetVertexBufferPlatform(wdUInt32 uiSlot, const wdGALBuffer* pVertexBuffer) = 0;
  virtual void SetVertexDeclarationPlatform(const wdGALVertexDeclaration* pVertexDeclaration) = 0;
  virtual void SetPrimitiveTopologyPlatform(wdGALPrimitiveTopology::Enum topology) = 0;

  virtual void SetBlendStatePlatform(const wdGALBlendState* pBlendState, const wdColor& blendFactor, wdUInt32 uiSampleMask) = 0;
  virtual void SetDepthStencilStatePlatform(const wdGALDepthStencilState* pDepthStencilState, wdUInt8 uiStencilRefValue) = 0;
  virtual void SetRasterizerStatePlatform(const wdGALRasterizerState* pRasterizerState) = 0;

  virtual void SetViewportPlatform(const wdRectFloat& rect, float fMinDepth, float fMaxDepth) = 0;
  virtual void SetScissorRectPlatform(const wdRectU32& rect) = 0;

  virtual void SetStreamOutBufferPlatform(wdUInt32 uiSlot, const wdGALBuffer* pBuffer, wdUInt32 uiOffset) = 0;
};

class WD_RENDERERFOUNDATION_DLL wdGALCommandEncoderComputePlatformInterface
{
public:
  // Dispatch

  virtual void DispatchPlatform(wdUInt32 uiThreadGroupCountX, wdUInt32 uiThreadGroupCountY, wdUInt32 uiThreadGroupCountZ) = 0;
  virtual void DispatchIndirectPlatform(const wdGALBuffer* pIndirectArgumentBuffer, wdUInt32 uiArgumentOffsetInBytes) = 0;
};
