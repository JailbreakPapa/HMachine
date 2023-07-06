
#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <Foundation/Types/Bitflags.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>

#include <vulkan/vulkan.hpp>

class wdGALBlendStateVulkan;
class wdGALBufferVulkan;
class wdGALDepthStencilStateVulkan;
class wdGALRasterizerStateVulkan;
class wdGALResourceViewVulkan;
class wdGALSamplerStateVulkan;
class wdGALShaderVulkan;
class wdGALUnorderedAccessViewVulkan;
class wdGALDeviceVulkan;

class WD_RENDERERVULKAN_DLL wdGALCommandEncoderImplVulkan : public wdGALCommandEncoderCommonPlatformInterface, public wdGALCommandEncoderRenderPlatformInterface, public wdGALCommandEncoderComputePlatformInterface
{
public:
  wdGALCommandEncoderImplVulkan(wdGALDeviceVulkan& device);
  ~wdGALCommandEncoderImplVulkan();

  void Reset();
  void MarkDirty();
  void SetCurrentCommandBuffer(vk::CommandBuffer* commandBuffer, wdPipelineBarrierVulkan* pipelineBarrier);

  // wdGALCommandEncoderCommonPlatformInterface
  // State setting functions

  virtual void SetShaderPlatform(const wdGALShader* pShader) override;

  virtual void SetConstantBufferPlatform(wdUInt32 uiSlot, const wdGALBuffer* pBuffer) override;
  virtual void SetSamplerStatePlatform(wdGALShaderStage::Enum Stage, wdUInt32 uiSlot, const wdGALSamplerState* pSamplerState) override;
  virtual void SetResourceViewPlatform(wdGALShaderStage::Enum Stage, wdUInt32 uiSlot, const wdGALResourceView* pResourceView) override;
  virtual void SetUnorderedAccessViewPlatform(wdUInt32 uiSlot, const wdGALUnorderedAccessView* pUnorderedAccessView) override;

  // Query functions

  virtual void BeginQueryPlatform(const wdGALQuery* pQuery) override;
  virtual void EndQueryPlatform(const wdGALQuery* pQuery) override;
  virtual wdResult GetQueryResultPlatform(const wdGALQuery* pQuery, wdUInt64& uiQueryResult) override;

  // Timestamp functions

  virtual void InsertTimestampPlatform(wdGALTimestampHandle hTimestamp) override;

  // Resource update functions

  virtual void ClearUnorderedAccessViewPlatform(const wdGALUnorderedAccessView* pUnorderedAccessView, wdVec4 clearValues) override;
  virtual void ClearUnorderedAccessViewPlatform(const wdGALUnorderedAccessView* pUnorderedAccessView, wdVec4U32 clearValues) override;

  virtual void CopyBufferPlatform(const wdGALBuffer* pDestination, const wdGALBuffer* pSource) override;
  virtual void CopyBufferRegionPlatform(const wdGALBuffer* pDestination, wdUInt32 uiDestOffset, const wdGALBuffer* pSource, wdUInt32 uiSourceOffset, wdUInt32 uiByteCount) override;

  virtual void UpdateBufferPlatform(const wdGALBuffer* pDestination, wdUInt32 uiDestOffset, wdArrayPtr<const wdUInt8> pSourceData, wdGALUpdateMode::Enum updateMode) override;

  virtual void CopyTexturePlatform(const wdGALTexture* pDestination, const wdGALTexture* pSource) override;
  virtual void CopyTextureRegionPlatform(const wdGALTexture* pDestination, const wdGALTextureSubresource& DestinationSubResource, const wdVec3U32& DestinationPoint, const wdGALTexture* pSource, const wdGALTextureSubresource& SourceSubResource, const wdBoundingBoxu32& Box) override;

  virtual void UpdateTexturePlatform(const wdGALTexture* pDestination, const wdGALTextureSubresource& DestinationSubResource, const wdBoundingBoxu32& DestinationBox, const wdGALSystemMemoryDescription& pSourceData) override;

  virtual void ResolveTexturePlatform(const wdGALTexture* pDestination, const wdGALTextureSubresource& DestinationSubResource, const wdGALTexture* pSource, const wdGALTextureSubresource& SourceSubResource) override;

  virtual void ReadbackTexturePlatform(const wdGALTexture* pTexture) override;

  virtual void CopyTextureReadbackResultPlatform(const wdGALTexture* pTexture, wdArrayPtr<wdGALTextureSubresource> SourceSubResource, wdArrayPtr<wdGALSystemMemoryDescription> TargetData) override;

  virtual void GenerateMipMapsPlatform(const wdGALResourceView* pResourceView) override;

  void CopyImageToBuffer(const wdGALTextureVulkan* pSource, const wdGALBufferVulkan* pDestination);

  // Misc

  virtual void FlushPlatform() override;

  // Debug helper functions

  virtual void PushMarkerPlatform(const char* szMarker) override;
  virtual void PopMarkerPlatform() override;
  virtual void InsertEventMarkerPlatform(const char* szMarker) override;

  // wdGALCommandEncoderRenderPlatformInterface
  void BeginRendering(const wdGALRenderingSetup& renderingSetup);
  void EndRendering();

  // Draw functions

  virtual void ClearPlatform(const wdColor& ClearColor, wdUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, wdUInt8 uiStencilClear) override;

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
  virtual void SetPrimitiveTopologyPlatform(wdGALPrimitiveTopology::Enum Topology) override;

  virtual void SetBlendStatePlatform(const wdGALBlendState* pBlendState, const wdColor& BlendFactor, wdUInt32 uiSampleMask) override;
  virtual void SetDepthStencilStatePlatform(const wdGALDepthStencilState* pDepthStencilState, wdUInt8 uiStencilRefValue) override;
  virtual void SetRasterizerStatePlatform(const wdGALRasterizerState* pRasterizerState) override;

  virtual void SetViewportPlatform(const wdRectFloat& rect, float fMinDepth, float fMaxDepth) override;
  virtual void SetScissorRectPlatform(const wdRectU32& rect) override;

  virtual void SetStreamOutBufferPlatform(wdUInt32 uiSlot, const wdGALBuffer* pBuffer, wdUInt32 uiOffset) override;


  // wdGALCommandEncoderComputePlatformInterface
  // Dispatch
  void BeginCompute();
  void EndCompute();

  virtual void DispatchPlatform(wdUInt32 uiThreadGroupCountX, wdUInt32 uiThreadGroupCountY, wdUInt32 uiThreadGroupCountZ) override;
  virtual void DispatchIndirectPlatform(const wdGALBuffer* pIndirectArgumentBuffer, wdUInt32 uiArgumentOffsetInBytes) override;

private:
  void FlushDeferredStateChanges();

  wdGALDeviceVulkan& m_GALDeviceVulkan;
  vk::Device m_vkDevice;

  vk::CommandBuffer* m_pCommandBuffer = nullptr;
  wdPipelineBarrierVulkan* m_pPipelineBarrier = nullptr;


  // Cache flags.
  bool m_bPipelineStateDirty = true;
  bool m_bViewportDirty = true;
  bool m_bIndexBufferDirty = false;
  bool m_bDescriptorsDirty = false;
  wdGAL::ModifiedRange m_BoundVertexBuffersRange;
  bool m_bRenderPassActive = false; ///< #TODO_VULKAN Disabling and re-enabling the render pass is buggy as we might execute a clear twice.
  bool m_bClearSubmitted = false; ///< Start render pass is lazy so if no draw call is executed we need to make sure the clear is executed anyways.
  bool m_bInsideCompute = false;  ///< Within BeginCompute / EndCompute block.


  // Bound objects for deferred state flushes
  wdResourceCacheVulkan::PipelineLayoutDesc m_LayoutDesc;
  wdResourceCacheVulkan::GraphicsPipelineDesc m_PipelineDesc;
  wdResourceCacheVulkan::ComputePipelineDesc m_ComputeDesc;
  vk::Framebuffer m_frameBuffer;
  vk::RenderPassBeginInfo m_renderPass;
  wdHybridArray<vk::ClearValue, WD_GAL_MAX_RENDERTARGET_COUNT + 1> m_clearValues;
  vk::ImageAspectFlags m_depthMask = {};
  wdUInt32 m_uiLayers = 0;

  vk::Viewport m_viewport;
  vk::Rect2D m_scissor;
  bool m_bScissorEnabled = false;

  const wdGALRenderTargetView* m_pBoundRenderTargets[WD_GAL_MAX_RENDERTARGET_COUNT] = {};
  const wdGALRenderTargetView* m_pBoundDepthStencilTarget = nullptr;
  wdUInt32 m_uiBoundRenderTargetCount;

  const wdGALBufferVulkan* m_pIndexBuffer = nullptr;
  vk::Buffer m_pBoundVertexBuffers[WD_GAL_MAX_VERTEX_BUFFER_COUNT];
  vk::DeviceSize m_VertexBufferOffsets[WD_GAL_MAX_VERTEX_BUFFER_COUNT] = {};

  const wdGALBufferVulkan* m_pBoundConstantBuffers[WD_GAL_MAX_CONSTANT_BUFFER_COUNT] = {};
  wdHybridArray<const wdGALResourceViewVulkan*, 16> m_pBoundShaderResourceViews[wdGALShaderStage::ENUM_COUNT] = {};
  wdHybridArray<const wdGALUnorderedAccessViewVulkan*, 16> m_pBoundUnoderedAccessViews;
  const wdGALSamplerStateVulkan* m_pBoundSamplerStates[wdGALShaderStage::ENUM_COUNT][WD_GAL_MAX_SAMPLER_COUNT] = {};

  wdHybridArray<vk::WriteDescriptorSet, 16> m_DescriptorWrites;
};
