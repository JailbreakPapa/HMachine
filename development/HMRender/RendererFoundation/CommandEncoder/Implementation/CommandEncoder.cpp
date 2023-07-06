#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Query.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/ResourceView.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Resources/UnorderedAccesView.h>

void wdGALCommandEncoder::SetShader(wdGALShaderHandle hShader)
{
  AssertRenderingThread();
  /// \todo Assert for shader capabilities (supported shader stages etc.)

  if (m_State.m_hShader == hShader)
  {
    CountRedundantStateChange();
    return;
  }

  const wdGALShader* pShader = m_Device.GetShader(hShader);
  WD_ASSERT_DEV(pShader != nullptr, "The given shader handle isn't valid, this may be a use after destroy!");

  m_CommonImpl.SetShaderPlatform(pShader);

  m_State.m_hShader = hShader;
  CountStateChange();
}

void wdGALCommandEncoder::SetConstantBuffer(wdUInt32 uiSlot, wdGALBufferHandle hBuffer)
{
  AssertRenderingThread();
  WD_ASSERT_RELEASE(uiSlot < WD_GAL_MAX_CONSTANT_BUFFER_COUNT, "Constant buffer slot index too big!");

  if (m_State.m_hConstantBuffers[uiSlot] == hBuffer)
  {
    CountRedundantStateChange();
    return;
  }

  const wdGALBuffer* pBuffer = m_Device.GetBuffer(hBuffer);
  WD_ASSERT_DEV(pBuffer == nullptr || pBuffer->GetDescription().m_BufferType == wdGALBufferType::ConstantBuffer, "Wrong buffer type");

  m_CommonImpl.SetConstantBufferPlatform(uiSlot, pBuffer);

  m_State.m_hConstantBuffers[uiSlot] = hBuffer;

  CountStateChange();
}

void wdGALCommandEncoder::SetSamplerState(wdGALShaderStage::Enum stage, wdUInt32 uiSlot, wdGALSamplerStateHandle hSamplerState)
{
  AssertRenderingThread();
  WD_ASSERT_RELEASE(uiSlot < WD_GAL_MAX_SAMPLER_COUNT, "Sampler state slot index too big!");

  if (m_State.m_hSamplerStates[stage][uiSlot] == hSamplerState)
  {
    CountRedundantStateChange();
    return;
  }

  const wdGALSamplerState* pSamplerState = m_Device.GetSamplerState(hSamplerState);

  m_CommonImpl.SetSamplerStatePlatform(stage, uiSlot, pSamplerState);

  m_State.m_hSamplerStates[stage][uiSlot] = hSamplerState;

  CountStateChange();
}

void wdGALCommandEncoder::SetResourceView(wdGALShaderStage::Enum stage, wdUInt32 uiSlot, wdGALResourceViewHandle hResourceView)
{
  AssertRenderingThread();

  /// \todo Check if the device supports the stage / the slot index

  auto& boundResourceViews = m_State.m_hResourceViews[stage];
  if (uiSlot < boundResourceViews.GetCount() && boundResourceViews[uiSlot] == hResourceView)
  {
    CountRedundantStateChange();
    return;
  }

  const wdGALResourceView* pResourceView = m_Device.GetResourceView(hResourceView);
  if (pResourceView != nullptr)
  {
    if (UnsetUnorderedAccessViews(pResourceView->GetResource()))
    {
      m_CommonImpl.FlushPlatform();
    }
  }

  m_CommonImpl.SetResourceViewPlatform(stage, uiSlot, pResourceView);

  boundResourceViews.EnsureCount(uiSlot + 1);
  boundResourceViews[uiSlot] = hResourceView;

  auto& boundResources = m_State.m_pResourcesForResourceViews[stage];
  boundResources.EnsureCount(uiSlot + 1);
  boundResources[uiSlot] = pResourceView != nullptr ? pResourceView->GetResource()->GetParentResource() : nullptr;

  CountStateChange();
}

void wdGALCommandEncoder::SetUnorderedAccessView(wdUInt32 uiSlot, wdGALUnorderedAccessViewHandle hUnorderedAccessView)
{
  AssertRenderingThread();

  /// \todo Check if the device supports the stage / the slot index

  if (uiSlot < m_State.m_hUnorderedAccessViews.GetCount() && m_State.m_hUnorderedAccessViews[uiSlot] == hUnorderedAccessView)
  {
    CountRedundantStateChange();
    return;
  }

  const wdGALUnorderedAccessView* pUnorderedAccessView = m_Device.GetUnorderedAccessView(hUnorderedAccessView);
  if (pUnorderedAccessView != nullptr)
  {
    if (UnsetResourceViews(pUnorderedAccessView->GetResource()))
    {
      m_CommonImpl.FlushPlatform();
    }
  }

  m_CommonImpl.SetUnorderedAccessViewPlatform(uiSlot, pUnorderedAccessView);

  m_State.m_hUnorderedAccessViews.EnsureCount(uiSlot + 1);
  m_State.m_hUnorderedAccessViews[uiSlot] = hUnorderedAccessView;

  m_State.m_pResourcesForUnorderedAccessViews.EnsureCount(uiSlot + 1);
  m_State.m_pResourcesForUnorderedAccessViews[uiSlot] = pUnorderedAccessView != nullptr ? pUnorderedAccessView->GetResource()->GetParentResource() : nullptr;

  CountStateChange();
}

bool wdGALCommandEncoder::UnsetResourceViews(const wdGALResourceBase* pResource)
{
  WD_ASSERT_DEV(pResource->GetParentResource() == pResource, "No proxies allowed");

  bool bResult = false;

  for (wdUInt32 stage = 0; stage < wdGALShaderStage::ENUM_COUNT; ++stage)
  {
    for (wdUInt32 uiSlot = 0; uiSlot < m_State.m_pResourcesForResourceViews[stage].GetCount(); ++uiSlot)
    {
      if (m_State.m_pResourcesForResourceViews[stage][uiSlot] == pResource)
      {
        m_CommonImpl.SetResourceViewPlatform((wdGALShaderStage::Enum)stage, uiSlot, nullptr);

        m_State.m_hResourceViews[stage][uiSlot].Invalidate();
        m_State.m_pResourcesForResourceViews[stage][uiSlot] = nullptr;

        bResult = true;
      }
    }
  }

  return bResult;
}

bool wdGALCommandEncoder::UnsetUnorderedAccessViews(const wdGALResourceBase* pResource)
{
  WD_ASSERT_DEV(pResource->GetParentResource() == pResource, "No proxies allowed");

  bool bResult = false;

  for (wdUInt32 uiSlot = 0; uiSlot < m_State.m_pResourcesForUnorderedAccessViews.GetCount(); ++uiSlot)
  {
    if (m_State.m_pResourcesForUnorderedAccessViews[uiSlot] == pResource)
    {
      m_CommonImpl.SetUnorderedAccessViewPlatform(uiSlot, nullptr);

      m_State.m_hUnorderedAccessViews[uiSlot].Invalidate();
      m_State.m_pResourcesForUnorderedAccessViews[uiSlot] = nullptr;

      bResult = true;
    }
  }

  return bResult;
}

void wdGALCommandEncoder::BeginQuery(wdGALQueryHandle hQuery)
{
  AssertRenderingThread();

  auto query = m_Device.GetQuery(hQuery);
  WD_ASSERT_DEV(!query->m_bStarted, "Can't stat wdGALQuery because it is already running.");

  m_CommonImpl.BeginQueryPlatform(query);
}

void wdGALCommandEncoder::EndQuery(wdGALQueryHandle hQuery)
{
  AssertRenderingThread();

  auto query = m_Device.GetQuery(hQuery);
  WD_ASSERT_DEV(query->m_bStarted, "Can't end wdGALQuery, query hasn't started yet.");

  m_CommonImpl.EndQueryPlatform(query);
}

wdResult wdGALCommandEncoder::GetQueryResult(wdGALQueryHandle hQuery, wdUInt64& ref_uiQueryResult)
{
  AssertRenderingThread();

  auto query = m_Device.GetQuery(hQuery);
  WD_ASSERT_DEV(!query->m_bStarted, "Can't retrieve data from wdGALQuery while query is still running.");

  return m_CommonImpl.GetQueryResultPlatform(query, ref_uiQueryResult);
}

wdGALTimestampHandle wdGALCommandEncoder::InsertTimestamp()
{
  wdGALTimestampHandle hTimestamp = m_Device.GetTimestamp();

  m_CommonImpl.InsertTimestampPlatform(hTimestamp);

  return hTimestamp;
}

void wdGALCommandEncoder::ClearUnorderedAccessView(wdGALUnorderedAccessViewHandle hUnorderedAccessView, wdVec4 vClearValues)
{
  AssertRenderingThread();

  const wdGALUnorderedAccessView* pUnorderedAccessView = m_Device.GetUnorderedAccessView(hUnorderedAccessView);
  if (pUnorderedAccessView == nullptr)
  {
    WD_REPORT_FAILURE("ClearUnorderedAccessView failed, unordered access view handle invalid.");
    return;
  }

  m_CommonImpl.ClearUnorderedAccessViewPlatform(pUnorderedAccessView, vClearValues);
}

void wdGALCommandEncoder::ClearUnorderedAccessView(wdGALUnorderedAccessViewHandle hUnorderedAccessView, wdVec4U32 vClearValues)
{
  AssertRenderingThread();

  const wdGALUnorderedAccessView* pUnorderedAccessView = m_Device.GetUnorderedAccessView(hUnorderedAccessView);
  if (pUnorderedAccessView == nullptr)
  {
    WD_REPORT_FAILURE("ClearUnorderedAccessView failed, unordered access view handle invalid.");
    return;
  }

  m_CommonImpl.ClearUnorderedAccessViewPlatform(pUnorderedAccessView, vClearValues);
}

void wdGALCommandEncoder::CopyBuffer(wdGALBufferHandle hDest, wdGALBufferHandle hSource)
{
  AssertRenderingThread();

  const wdGALBuffer* pDest = m_Device.GetBuffer(hDest);
  const wdGALBuffer* pSource = m_Device.GetBuffer(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    m_CommonImpl.CopyBufferPlatform(pDest, pSource);
  }
  else
  {
    WD_REPORT_FAILURE("CopyBuffer failed, buffer handle invalid - destination = {0}, source = {1}", wdArgP(pDest), wdArgP(pSource));
  }
}

void wdGALCommandEncoder::CopyBufferRegion(
  wdGALBufferHandle hDest, wdUInt32 uiDestOffset, wdGALBufferHandle hSource, wdUInt32 uiSourceOffset, wdUInt32 uiByteCount)
{
  AssertRenderingThread();

  const wdGALBuffer* pDest = m_Device.GetBuffer(hDest);
  const wdGALBuffer* pSource = m_Device.GetBuffer(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    const wdUInt32 uiDestSize = pDest->GetSize();
    const wdUInt32 uiSourceSize = pSource->GetSize();

    WD_ASSERT_DEV(uiDestSize >= uiDestOffset + uiByteCount, "Destination buffer too small (or offset too big)");
    WD_ASSERT_DEV(uiSourceSize >= uiSourceOffset + uiByteCount, "Source buffer too small (or offset too big)");

    m_CommonImpl.CopyBufferRegionPlatform(pDest, uiDestOffset, pSource, uiSourceOffset, uiByteCount);
  }
  else
  {
    WD_REPORT_FAILURE("CopyBuffer failed, buffer handle invalid - destination = {0}, source = {1}", wdArgP(pDest), wdArgP(pSource));
  }
}

void wdGALCommandEncoder::UpdateBuffer(wdGALBufferHandle hDest, wdUInt32 uiDestOffset, wdArrayPtr<const wdUInt8> sourceData, wdGALUpdateMode::Enum updateMode)
{
  AssertRenderingThread();

  WD_ASSERT_DEV(!sourceData.IsEmpty(), "Source data for buffer update is invalid!");

  const wdGALBuffer* pDest = m_Device.GetBuffer(hDest);

  if (pDest != nullptr)
  {
    if (updateMode == wdGALUpdateMode::NoOverwrite && !(GetDevice().GetCapabilities().m_bNoOverwriteBufferUpdate))
    {
      updateMode = wdGALUpdateMode::CopyToTempStorage;
    }

    WD_ASSERT_DEV(pDest->GetSize() >= (uiDestOffset + sourceData.GetCount()), "Buffer {} is too small (or offset {} too big) for {} bytes", pDest->GetSize(), uiDestOffset, sourceData.GetCount());
    m_CommonImpl.UpdateBufferPlatform(pDest, uiDestOffset, sourceData, updateMode);
  }
  else
  {
    WD_REPORT_FAILURE("UpdateBuffer failed, buffer handle invalid");
  }
}

void wdGALCommandEncoder::CopyTexture(wdGALTextureHandle hDest, wdGALTextureHandle hSource)
{
  AssertRenderingThread();

  const wdGALTexture* pDest = m_Device.GetTexture(hDest);
  const wdGALTexture* pSource = m_Device.GetTexture(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    m_CommonImpl.CopyTexturePlatform(pDest, pSource);
  }
  else
  {
    WD_REPORT_FAILURE("CopyTexture failed, texture handle invalid - destination = {0}, source = {1}", wdArgP(pDest), wdArgP(pSource));
  }
}

void wdGALCommandEncoder::CopyTextureRegion(wdGALTextureHandle hDest, const wdGALTextureSubresource& destinationSubResource,
  const wdVec3U32& vDestinationPoint, wdGALTextureHandle hSource, const wdGALTextureSubresource& sourceSubResource, const wdBoundingBoxu32& box)
{
  AssertRenderingThread();

  const wdGALTexture* pDest = m_Device.GetTexture(hDest);
  const wdGALTexture* pSource = m_Device.GetTexture(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    m_CommonImpl.CopyTextureRegionPlatform(pDest, destinationSubResource, vDestinationPoint, pSource, sourceSubResource, box);
  }
  else
  {
    WD_REPORT_FAILURE("CopyTextureRegion failed, texture handle invalid - destination = {0}, source = {1}", wdArgP(pDest), wdArgP(pSource));
  }
}

void wdGALCommandEncoder::UpdateTexture(wdGALTextureHandle hDest, const wdGALTextureSubresource& destinationSubResource,
  const wdBoundingBoxu32& destinationBox, const wdGALSystemMemoryDescription& sourceData)
{
  AssertRenderingThread();

  const wdGALTexture* pDest = m_Device.GetTexture(hDest);

  if (pDest != nullptr)
  {
    m_CommonImpl.UpdateTexturePlatform(pDest, destinationSubResource, destinationBox, sourceData);
  }
  else
  {
    WD_REPORT_FAILURE("UpdateTexture failed, texture handle invalid - destination = {0}", wdArgP(pDest));
  }
}

void wdGALCommandEncoder::ResolveTexture(wdGALTextureHandle hDest, const wdGALTextureSubresource& destinationSubResource, wdGALTextureHandle hSource,
  const wdGALTextureSubresource& sourceSubResource)
{
  AssertRenderingThread();

  const wdGALTexture* pDest = m_Device.GetTexture(hDest);
  const wdGALTexture* pSource = m_Device.GetTexture(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    m_CommonImpl.ResolveTexturePlatform(pDest, destinationSubResource, pSource, sourceSubResource);
  }
  else
  {
    WD_REPORT_FAILURE("ResolveTexture failed, texture handle invalid - destination = {0}, source = {1}", wdArgP(pDest), wdArgP(pSource));
  }
}

void wdGALCommandEncoder::ReadbackTexture(wdGALTextureHandle hTexture)
{
  AssertRenderingThread();

  const wdGALTexture* pTexture = m_Device.GetTexture(hTexture);

  if (pTexture != nullptr)
  {
    WD_ASSERT_RELEASE(pTexture->GetDescription().m_ResourceAccess.m_bReadBack,
      "A texture supplied to read-back needs to be created with the correct resource usage (m_bReadBack = true)!");

    m_CommonImpl.ReadbackTexturePlatform(pTexture);
  }
}

void wdGALCommandEncoder::CopyTextureReadbackResult(wdGALTextureHandle hTexture, wdArrayPtr<wdGALTextureSubresource> sourceSubResource, wdArrayPtr<wdGALSystemMemoryDescription> targetData)
{
  AssertRenderingThread();

  const wdGALTexture* pTexture = m_Device.GetTexture(hTexture);

  if (pTexture != nullptr)
  {
    WD_ASSERT_RELEASE(pTexture->GetDescription().m_ResourceAccess.m_bReadBack,
      "A texture supplied to read-back needs to be created with the correct resource usage (m_bReadBack = true)!");

    m_CommonImpl.CopyTextureReadbackResultPlatform(pTexture, sourceSubResource, targetData);
  }
}

void wdGALCommandEncoder::GenerateMipMaps(wdGALResourceViewHandle hResourceView)
{
  AssertRenderingThread();

  const wdGALResourceView* pResourceView = m_Device.GetResourceView(hResourceView);
  if (pResourceView != nullptr)
  {
    WD_ASSERT_DEV(!pResourceView->GetDescription().m_hTexture.IsInvalidated(), "Resource view needs a valid texture to generate mip maps.");
    const wdGALTexture* pTexture = m_Device.GetTexture(pResourceView->GetDescription().m_hTexture);
    WD_ASSERT_DEV(pTexture->GetDescription().m_bAllowDynamicMipGeneration,
      "Dynamic mip map generation needs to be enabled (m_bAllowDynamicMipGeneration = true)!");

    m_CommonImpl.GenerateMipMapsPlatform(pResourceView);
  }
}

void wdGALCommandEncoder::Flush()
{
  AssertRenderingThread();

  m_CommonImpl.FlushPlatform();
}

// Debug helper functions

void wdGALCommandEncoder::PushMarker(const char* szMarker)
{
  AssertRenderingThread();

  WD_ASSERT_DEV(szMarker != nullptr, "Invalid marker!");

  m_CommonImpl.PushMarkerPlatform(szMarker);
}

void wdGALCommandEncoder::PopMarker()
{
  AssertRenderingThread();

  m_CommonImpl.PopMarkerPlatform();
}

void wdGALCommandEncoder::InsertEventMarker(const char* szMarker)
{
  AssertRenderingThread();

  WD_ASSERT_DEV(szMarker != nullptr, "Invalid marker!");

  m_CommonImpl.InsertEventMarkerPlatform(szMarker);
}

void wdGALCommandEncoder::ClearStatisticsCounters()
{
  // Reset counters for various statistics
  m_uiStateChanges = 0;
  m_uiRedundantStateChanges = 0;
}

wdGALCommandEncoder::wdGALCommandEncoder(wdGALDevice& device, wdGALCommandEncoderState& state, wdGALCommandEncoderCommonPlatformInterface& commonImpl)
  : m_Device(device)
  , m_State(state)
  , m_CommonImpl(commonImpl)
{
}

wdGALCommandEncoder::~wdGALCommandEncoder() = default;

void wdGALCommandEncoder::InvalidateState()
{
  m_State.InvalidateState();
}


WD_STATICLINK_FILE(RendererFoundation, RendererFoundation_CommandEncoder_Implementation_CommandEncoder);
