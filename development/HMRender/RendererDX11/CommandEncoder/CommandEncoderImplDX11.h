
#pragma once

#include <Foundation/Types/Bitflags.h>
#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

struct ID3D11DeviceChild;
struct ID3D11DeviceContext;
struct ID3DUserDefinedAnnotation;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11Buffer;
struct ID3D11ShaderResourceView;
struct ID3D11UnorderedAccessView;
struct ID3D11SamplerState;
struct ID3D11Query;

class wdGALDeviceDX11;

class WD_RENDERERDX11_DLL wdGALCommandEncoderImplDX11 : public wdGALCommandEncoderCommonPlatformInterface, public wdGALCommandEncoderRenderPlatformInterface, public wdGALCommandEncoderComputePlatformInterface
{
public:
  wdGALCommandEncoderImplDX11(wdGALDeviceDX11& ref_deviceDX11);
  ~wdGALCommandEncoderImplDX11();

  // wdGALCommandEncoderCommonPlatformInterface
  // State setting functions

  virtual void SetShaderPlatform(const wdGALShader* pShader) override;

  virtual void SetConstantBufferPlatform(wdUInt32 uiSlot, const wdGALBuffer* pBuffer) override;
  virtual void SetSamplerStatePlatform(wdGALShaderStage::Enum stage, wdUInt32 uiSlot, const wdGALSamplerState* pSamplerState) override;
  virtual void SetResourceViewPlatform(wdGALShaderStage::Enum stage, wdUInt32 uiSlot, const wdGALResourceView* pResourceView) override;
  virtual void SetUnorderedAccessViewPlatform(wdUInt32 uiSlot, const wdGALUnorderedAccessView* pUnorderedAccessView) override;

  // Query functions

  virtual void BeginQueryPlatform(const wdGALQuery* pQuery) override;
  virtual void EndQueryPlatform(const wdGALQuery* pQuery) override;
  virtual wdResult GetQueryResultPlatform(const wdGALQuery* pQuery, wdUInt64& ref_uiQueryResult) override;

  // Timestamp functions

  virtual void InsertTimestampPlatform(wdGALTimestampHandle hTimestamp) override;

  // Resource update functions

  virtual void ClearUnorderedAccessViewPlatform(const wdGALUnorderedAccessView* pUnorderedAccessView, wdVec4 vClearValues) override;
  virtual void ClearUnorderedAccessViewPlatform(const wdGALUnorderedAccessView* pUnorderedAccessView, wdVec4U32 vClearValues) override;

  virtual void CopyBufferPlatform(const wdGALBuffer* pDestination, const wdGALBuffer* pSource) override;
  virtual void CopyBufferRegionPlatform(const wdGALBuffer* pDestination, wdUInt32 uiDestOffset, const wdGALBuffer* pSource, wdUInt32 uiSourceOffset, wdUInt32 uiByteCount) override;

  virtual void UpdateBufferPlatform(const wdGALBuffer* pDestination, wdUInt32 uiDestOffset, wdArrayPtr<const wdUInt8> sourceData, wdGALUpdateMode::Enum updateMode) override;

  virtual void CopyTexturePlatform(const wdGALTexture* pDestination, const wdGALTexture* pSource) override;
  virtual void CopyTextureRegionPlatform(const wdGALTexture* pDestination, const wdGALTextureSubresource& destinationSubResource, const wdVec3U32& vDestinationPoint, const wdGALTexture* pSource, const wdGALTextureSubresource& sourceSubResource, const wdBoundingBoxu32& box) override;

  virtual void UpdateTexturePlatform(const wdGALTexture* pDestination, const wdGALTextureSubresource& destinationSubResource,
    const wdBoundingBoxu32& destinationBox, const wdGALSystemMemoryDescription& sourceData) override;

  virtual void ResolveTexturePlatform(const wdGALTexture* pDestination, const wdGALTextureSubresource& destinationSubResource,
    const wdGALTexture* pSource, const wdGALTextureSubresource& sourceSubResource) override;

  virtual void ReadbackTexturePlatform(const wdGALTexture* pTexture) override;

  virtual void CopyTextureReadbackResultPlatform(const wdGALTexture* pTexture, wdArrayPtr<wdGALTextureSubresource> sourceSubResource, wdArrayPtr<wdGALSystemMemoryDescription> targetData) override;

  virtual void GenerateMipMapsPlatform(const wdGALResourceView* pResourceView) override;

  // Misc

  virtual void FlushPlatform() override;

  // Debug helper functions

  virtual void PushMarkerPlatform(const char* szMarker) override;
  virtual void PopMarkerPlatform() override;
  virtual void InsertEventMarkerPlatform(const char* szMarker) override;


  // wdGALCommandEncoderRenderPlatformInterface
  void BeginRendering(const wdGALRenderingSetup& renderingSetup);
  void BeginCompute();

  // Draw functions

  virtual void ClearPlatform(const wdColor& clearColor, wdUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, wdUInt8 uiStencilClear) override;

  virtual void DrawPlatform(wdUInt32 uiVertexCount, wdUInt32 uiStartVertex) override;
  virtual void DrawIndexedPlatform(wdUInt32 uiIndexCount, wdUInt32 uiStartIndex) override;
  virtual void DrawIndexedInstancedPlatform(wdUInt32 uiIndexCountPerInstance, wdUInt32 uiInstanceCount, wdUInt32 uiStartIndex) override;
  virtual void DrawIndexedInstancedIndirectPlatform(const wdGALBuffer* pIndirectArgumentBuffer, wdUInt32 uiArgumentOffsetInBytes) override;
  virtual void DrawInstancedPlatform(wdUInt32 uiVertexCountPerInstance, wdUInt32 uiInstanceCount, wdUInt32 uiStartVertex) override;
  virtual void DrawInstancedIndirectPlatform(const wdGALBuffer* pIndirectArgumentBuffer, wdUInt32 uiArgumentOffsetInBytes) override;
  virtual void DrawAutoPlatform() override;

  virtual void BeginStreamOutPlatform() override;
  virtual void EndStreamOutPlatform() override;

  // State functions

  virtual void SetIndexBufferPlatform(const wdGALBuffer* pIndexBuffer) override;
  virtual void SetVertexBufferPlatform(wdUInt32 uiSlot, const wdGALBuffer* pVertexBuffer) override;
  virtual void SetVertexDeclarationPlatform(const wdGALVertexDeclaration* pVertexDeclaration) override;
  virtual void SetPrimitiveTopologyPlatform(wdGALPrimitiveTopology::Enum topology) override;

  virtual void SetBlendStatePlatform(const wdGALBlendState* pBlendState, const wdColor& blendFactor, wdUInt32 uiSampleMask) override;
  virtual void SetDepthStencilStatePlatform(const wdGALDepthStencilState* pDepthStencilState, wdUInt8 uiStencilRefValue) override;
  virtual void SetRasterizerStatePlatform(const wdGALRasterizerState* pRasterizerState) override;

  virtual void SetViewportPlatform(const wdRectFloat& rect, float fMinDepth, float fMaxDepth) override;
  virtual void SetScissorRectPlatform(const wdRectU32& rect) override;

  virtual void SetStreamOutBufferPlatform(wdUInt32 uiSlot, const wdGALBuffer* pBuffer, wdUInt32 uiOffset) override;


  // wdGALCommandEncoderComputePlatformInterface
  // Dispatch

  virtual void DispatchPlatform(wdUInt32 uiThreadGroupCountX, wdUInt32 uiThreadGroupCountY, wdUInt32 uiThreadGroupCountZ) override;
  virtual void DispatchIndirectPlatform(const wdGALBuffer* pIndirectArgumentBuffer, wdUInt32 uiArgumentOffsetInBytes) override;

private:
  friend class wdGALPassDX11;

  void FlushDeferredStateChanges();

  wdGALDeviceDX11& m_GALDeviceDX11;
  wdGALCommandEncoder* m_pOwner = nullptr;

  ID3D11DeviceContext* m_pDXContext = nullptr;
  ID3DUserDefinedAnnotation* m_pDXAnnotation = nullptr;

  // Bound objects for deferred state flushes
  ID3D11Buffer* m_pBoundConstantBuffers[WD_GAL_MAX_CONSTANT_BUFFER_COUNT] = {};
  wdGAL::ModifiedRange m_BoundConstantBuffersRange[wdGALShaderStage::ENUM_COUNT];

  wdHybridArray<ID3D11ShaderResourceView*, 16> m_pBoundShaderResourceViews[wdGALShaderStage::ENUM_COUNT] = {};
  wdGAL::ModifiedRange m_BoundShaderResourceViewsRange[wdGALShaderStage::ENUM_COUNT];

  wdHybridArray<ID3D11UnorderedAccessView*, 16> m_BoundUnoderedAccessViews;
  wdGAL::ModifiedRange m_BoundUnoderedAccessViewsRange;

  ID3D11SamplerState* m_pBoundSamplerStates[wdGALShaderStage::ENUM_COUNT][WD_GAL_MAX_SAMPLER_COUNT] = {};
  wdGAL::ModifiedRange m_BoundSamplerStatesRange[wdGALShaderStage::ENUM_COUNT];

  ID3D11DeviceChild* m_pBoundShaders[wdGALShaderStage::ENUM_COUNT] = {};

  wdGALRenderTargetSetup m_RenderTargetSetup;
  ID3D11RenderTargetView* m_pBoundRenderTargets[WD_GAL_MAX_RENDERTARGET_COUNT] = {};
  wdUInt32 m_uiBoundRenderTargetCount = 0;
  ID3D11DepthStencilView* m_pBoundDepthStencilTarget = nullptr;

  ID3D11Buffer* m_pBoundVertexBuffers[WD_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  wdGAL::ModifiedRange m_BoundVertexBuffersRange;

  wdUInt32 m_VertexBufferStrides[WD_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  wdUInt32 m_VertexBufferOffsets[WD_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
};
