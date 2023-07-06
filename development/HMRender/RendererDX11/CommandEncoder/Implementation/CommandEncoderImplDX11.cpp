#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/CommandEncoder/CommandEncoderImplDX11.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/QueryDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Resources/ResourceViewDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererDX11/Resources/UnorderedAccessViewDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>
#include <RendererDX11/Shader/VertexDeclarationDX11.h>
#include <RendererDX11/State/StateDX11.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>

#include <d3d11_1.h>

wdGALCommandEncoderImplDX11::wdGALCommandEncoderImplDX11(wdGALDeviceDX11& ref_deviceDX11)
  : m_GALDeviceDX11(ref_deviceDX11)
{
  m_pDXContext = m_GALDeviceDX11.GetDXImmediateContext();

  if (FAILED(m_pDXContext->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)&m_pDXAnnotation)))
  {
    wdLog::Warning("Failed to get annotation interface. GALContext marker will not work");
  }
}

wdGALCommandEncoderImplDX11::~wdGALCommandEncoderImplDX11()
{
  WD_GAL_DX11_RELEASE(m_pDXAnnotation);
}

// State setting functions

void wdGALCommandEncoderImplDX11::SetShaderPlatform(const wdGALShader* pShader)
{
  ID3D11VertexShader* pVS = nullptr;
  ID3D11HullShader* pHS = nullptr;
  ID3D11DomainShader* pDS = nullptr;
  ID3D11GeometryShader* pGS = nullptr;
  ID3D11PixelShader* pPS = nullptr;
  ID3D11ComputeShader* pCS = nullptr;

  if (pShader != nullptr)
  {
    const wdGALShaderDX11* pDXShader = static_cast<const wdGALShaderDX11*>(pShader);

    pVS = pDXShader->GetDXVertexShader();
    pHS = pDXShader->GetDXHullShader();
    pDS = pDXShader->GetDXDomainShader();
    pGS = pDXShader->GetDXGeometryShader();
    pPS = pDXShader->GetDXPixelShader();
    pCS = pDXShader->GetDXComputeShader();
  }

  if (pVS != m_pBoundShaders[wdGALShaderStage::VertexShader])
  {
    m_pDXContext->VSSetShader(pVS, nullptr, 0);
    m_pBoundShaders[wdGALShaderStage::VertexShader] = pVS;
  }

  if (pHS != m_pBoundShaders[wdGALShaderStage::HullShader])
  {
    m_pDXContext->HSSetShader(pHS, nullptr, 0);
    m_pBoundShaders[wdGALShaderStage::HullShader] = pHS;
  }

  if (pDS != m_pBoundShaders[wdGALShaderStage::DomainShader])
  {
    m_pDXContext->DSSetShader(pDS, nullptr, 0);
    m_pBoundShaders[wdGALShaderStage::DomainShader] = pDS;
  }

  if (pGS != m_pBoundShaders[wdGALShaderStage::GeometryShader])
  {
    m_pDXContext->GSSetShader(pGS, nullptr, 0);
    m_pBoundShaders[wdGALShaderStage::GeometryShader] = pGS;
  }

  if (pPS != m_pBoundShaders[wdGALShaderStage::PixelShader])
  {
    m_pDXContext->PSSetShader(pPS, nullptr, 0);
    m_pBoundShaders[wdGALShaderStage::PixelShader] = pPS;
  }

  if (pCS != m_pBoundShaders[wdGALShaderStage::ComputeShader])
  {
    m_pDXContext->CSSetShader(pCS, nullptr, 0);
    m_pBoundShaders[wdGALShaderStage::ComputeShader] = pCS;
  }
}

void wdGALCommandEncoderImplDX11::SetConstantBufferPlatform(wdUInt32 uiSlot, const wdGALBuffer* pBuffer)
{
  /// \todo Check if the device supports the slot index?
  m_pBoundConstantBuffers[uiSlot] = pBuffer != nullptr ? static_cast<const wdGALBufferDX11*>(pBuffer)->GetDXBuffer() : nullptr;

  // The GAL doesn't care about stages for constant buffer, but we need to handle this internaly.
  for (wdUInt32 stage = 0; stage < wdGALShaderStage::ENUM_COUNT; ++stage)
    m_BoundConstantBuffersRange[stage].SetToIncludeValue(uiSlot);
}

void wdGALCommandEncoderImplDX11::SetSamplerStatePlatform(wdGALShaderStage::Enum stage, wdUInt32 uiSlot, const wdGALSamplerState* pSamplerState)
{
  /// \todo Check if the device supports the stage / the slot index
  m_pBoundSamplerStates[stage][uiSlot] =
    pSamplerState != nullptr ? static_cast<const wdGALSamplerStateDX11*>(pSamplerState)->GetDXSamplerState() : nullptr;
  m_BoundSamplerStatesRange[stage].SetToIncludeValue(uiSlot);
}

void wdGALCommandEncoderImplDX11::SetResourceViewPlatform(wdGALShaderStage::Enum stage, wdUInt32 uiSlot, const wdGALResourceView* pResourceView)
{
  auto& boundShaderResourceViews = m_pBoundShaderResourceViews[stage];
  boundShaderResourceViews.EnsureCount(uiSlot + 1);
  boundShaderResourceViews[uiSlot] =
    pResourceView != nullptr ? static_cast<const wdGALResourceViewDX11*>(pResourceView)->GetDXResourceView() : nullptr;
  m_BoundShaderResourceViewsRange[stage].SetToIncludeValue(uiSlot);
}

void wdGALCommandEncoderImplDX11::SetUnorderedAccessViewPlatform(wdUInt32 uiSlot, const wdGALUnorderedAccessView* pUnorderedAccessView)
{
  m_BoundUnoderedAccessViews.EnsureCount(uiSlot + 1);
  m_BoundUnoderedAccessViews[uiSlot] =
    pUnorderedAccessView != nullptr ? static_cast<const wdGALUnorderedAccessViewDX11*>(pUnorderedAccessView)->GetDXResourceView() : nullptr;
  m_BoundUnoderedAccessViewsRange.SetToIncludeValue(uiSlot);
}

// Query functions

void wdGALCommandEncoderImplDX11::BeginQueryPlatform(const wdGALQuery* pQuery)
{
  m_pDXContext->Begin(static_cast<const wdGALQueryDX11*>(pQuery)->GetDXQuery());
}

void wdGALCommandEncoderImplDX11::EndQueryPlatform(const wdGALQuery* pQuery)
{
  m_pDXContext->End(static_cast<const wdGALQueryDX11*>(pQuery)->GetDXQuery());
}

wdResult wdGALCommandEncoderImplDX11::GetQueryResultPlatform(const wdGALQuery* pQuery, wdUInt64& ref_uiQueryResult)
{
  return m_pDXContext->GetData(
           static_cast<const wdGALQueryDX11*>(pQuery)->GetDXQuery(), &ref_uiQueryResult, sizeof(wdUInt64), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_FALSE
           ? WD_FAILURE
           : WD_SUCCESS;
}

// Timestamp functions

void wdGALCommandEncoderImplDX11::InsertTimestampPlatform(wdGALTimestampHandle hTimestamp)
{
  ID3D11Query* pDXQuery = m_GALDeviceDX11.GetTimestamp(hTimestamp);

  m_pDXContext->End(pDXQuery);
}

// Resource update functions

void wdGALCommandEncoderImplDX11::ClearUnorderedAccessViewPlatform(const wdGALUnorderedAccessView* pUnorderedAccessView, wdVec4 vClearValues)
{
  const wdGALUnorderedAccessViewDX11* pUnorderedAccessViewDX11 = static_cast<const wdGALUnorderedAccessViewDX11*>(pUnorderedAccessView);
  m_pDXContext->ClearUnorderedAccessViewFloat(pUnorderedAccessViewDX11->GetDXResourceView(), &vClearValues.x);
}

void wdGALCommandEncoderImplDX11::ClearUnorderedAccessViewPlatform(const wdGALUnorderedAccessView* pUnorderedAccessView, wdVec4U32 vClearValues)
{
  const wdGALUnorderedAccessViewDX11* pUnorderedAccessViewDX11 = static_cast<const wdGALUnorderedAccessViewDX11*>(pUnorderedAccessView);
  m_pDXContext->ClearUnorderedAccessViewUint(pUnorderedAccessViewDX11->GetDXResourceView(), &vClearValues.x);
}

void wdGALCommandEncoderImplDX11::CopyBufferPlatform(const wdGALBuffer* pDestination, const wdGALBuffer* pSource)
{
  ID3D11Buffer* pDXDestination = static_cast<const wdGALBufferDX11*>(pDestination)->GetDXBuffer();
  ID3D11Buffer* pDXSource = static_cast<const wdGALBufferDX11*>(pSource)->GetDXBuffer();

  m_pDXContext->CopyResource(pDXDestination, pDXSource);
}

void wdGALCommandEncoderImplDX11::CopyBufferRegionPlatform(const wdGALBuffer* pDestination, wdUInt32 uiDestOffset, const wdGALBuffer* pSource, wdUInt32 uiSourceOffset, wdUInt32 uiByteCount)
{
  ID3D11Buffer* pDXDestination = static_cast<const wdGALBufferDX11*>(pDestination)->GetDXBuffer();
  ID3D11Buffer* pDXSource = static_cast<const wdGALBufferDX11*>(pSource)->GetDXBuffer();

  D3D11_BOX srcBox = {uiSourceOffset, 0, 0, uiSourceOffset + uiByteCount, 1, 1};
  m_pDXContext->CopySubresourceRegion(pDXDestination, 0, uiDestOffset, 0, 0, pDXSource, 0, &srcBox);
}

void wdGALCommandEncoderImplDX11::UpdateBufferPlatform(const wdGALBuffer* pDestination, wdUInt32 uiDestOffset, wdArrayPtr<const wdUInt8> sourceData, wdGALUpdateMode::Enum updateMode)
{
  WD_CHECK_ALIGNMENT_16(sourceData.GetPtr());

  ID3D11Buffer* pDXDestination = static_cast<const wdGALBufferDX11*>(pDestination)->GetDXBuffer();

  if (pDestination->GetDescription().m_BufferType == wdGALBufferType::ConstantBuffer)
  {
    WD_ASSERT_DEV(uiDestOffset == 0 && sourceData.GetCount() == pDestination->GetSize(),
      "Constant buffers can't be updated partially (and we don't check for DX11.1)!");

    D3D11_MAPPED_SUBRESOURCE MapResult;
    if (SUCCEEDED(m_pDXContext->Map(pDXDestination, 0, D3D11_MAP_WRITE_DISCARD, 0, &MapResult)))
    {
      memcpy(MapResult.pData, sourceData.GetPtr(), sourceData.GetCount());

      m_pDXContext->Unmap(pDXDestination, 0);
    }
  }
  else
  {
    if (updateMode == wdGALUpdateMode::CopyToTempStorage)
    {
      if (ID3D11Resource* pDXTempBuffer = m_GALDeviceDX11.FindTempBuffer(sourceData.GetCount()))
      {
        D3D11_MAPPED_SUBRESOURCE MapResult;
        HRESULT hRes = m_pDXContext->Map(pDXTempBuffer, 0, D3D11_MAP_WRITE, 0, &MapResult);
        WD_ASSERT_DEV(SUCCEEDED(hRes), "Implementation error");

        memcpy(MapResult.pData, sourceData.GetPtr(), sourceData.GetCount());

        m_pDXContext->Unmap(pDXTempBuffer, 0);

        D3D11_BOX srcBox = {0, 0, 0, sourceData.GetCount(), 1, 1};
        m_pDXContext->CopySubresourceRegion(pDXDestination, 0, uiDestOffset, 0, 0, pDXTempBuffer, 0, &srcBox);
      }
      else
      {
        WD_REPORT_FAILURE("Could not find a temp buffer for update.");
      }
    }
    else
    {
      D3D11_MAP mapType = (updateMode == wdGALUpdateMode::Discard) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;

      D3D11_MAPPED_SUBRESOURCE MapResult;
      if (SUCCEEDED(m_pDXContext->Map(pDXDestination, 0, mapType, 0, &MapResult)))
      {
        memcpy(wdMemoryUtils::AddByteOffset(MapResult.pData, uiDestOffset), sourceData.GetPtr(), sourceData.GetCount());

        m_pDXContext->Unmap(pDXDestination, 0);
      }
      else
      {
        wdLog::Error("Could not map buffer to update content.");
      }
    }
  }
}

void wdGALCommandEncoderImplDX11::CopyTexturePlatform(const wdGALTexture* pDestination, const wdGALTexture* pSource)
{
  ID3D11Resource* pDXDestination = static_cast<const wdGALTextureDX11*>(pDestination)->GetDXTexture();
  ID3D11Resource* pDXSource = static_cast<const wdGALTextureDX11*>(pSource)->GetDXTexture();

  m_pDXContext->CopyResource(pDXDestination, pDXSource);
}

void wdGALCommandEncoderImplDX11::CopyTextureRegionPlatform(const wdGALTexture* pDestination, const wdGALTextureSubresource& destinationSubResource,
  const wdVec3U32& vDestinationPoint, const wdGALTexture* pSource, const wdGALTextureSubresource& sourceSubResource, const wdBoundingBoxu32& box)
{
  ID3D11Resource* pDXDestination = static_cast<const wdGALTextureDX11*>(pDestination)->GetDXTexture();
  ID3D11Resource* pDXSource = static_cast<const wdGALTextureDX11*>(pSource)->GetDXTexture();

  wdUInt32 dstSubResource = D3D11CalcSubresource(
    destinationSubResource.m_uiMipLevel, destinationSubResource.m_uiArraySlice, pDestination->GetDescription().m_uiMipLevelCount);
  wdUInt32 srcSubResource =
    D3D11CalcSubresource(sourceSubResource.m_uiMipLevel, sourceSubResource.m_uiArraySlice, pSource->GetDescription().m_uiMipLevelCount);

  D3D11_BOX srcBox = {box.m_vMin.x, box.m_vMin.y, box.m_vMin.z, box.m_vMax.x, box.m_vMax.y, box.m_vMax.z};
  m_pDXContext->CopySubresourceRegion(
    pDXDestination, dstSubResource, vDestinationPoint.x, vDestinationPoint.y, vDestinationPoint.z, pDXSource, srcSubResource, &srcBox);
}

void wdGALCommandEncoderImplDX11::UpdateTexturePlatform(const wdGALTexture* pDestination, const wdGALTextureSubresource& destinationSubResource,
  const wdBoundingBoxu32& destinationBox, const wdGALSystemMemoryDescription& sourceData)
{
  ID3D11Resource* pDXDestination = static_cast<const wdGALTextureDX11*>(pDestination)->GetDXTexture();

  wdUInt32 uiWidth = wdMath::Max(destinationBox.m_vMax.x - destinationBox.m_vMin.x, 1u);
  wdUInt32 uiHeight = wdMath::Max(destinationBox.m_vMax.y - destinationBox.m_vMin.y, 1u);
  wdUInt32 uiDepth = wdMath::Max(destinationBox.m_vMax.z - destinationBox.m_vMin.z, 1u);
  wdGALResourceFormat::Enum format = pDestination->GetDescription().m_Format;

  if (ID3D11Resource* pDXTempTexture = m_GALDeviceDX11.FindTempTexture(uiWidth, uiHeight, uiDepth, format))
  {
    D3D11_MAPPED_SUBRESOURCE MapResult;
    HRESULT hRes = m_pDXContext->Map(pDXTempTexture, 0, D3D11_MAP_WRITE, 0, &MapResult);
    WD_ASSERT_DEV(SUCCEEDED(hRes), "Implementation error");

    wdUInt32 uiRowPitch = uiWidth * wdGALResourceFormat::GetBitsPerElement(format) / 8;
    wdUInt32 uiSlicePitch = uiRowPitch * uiHeight;
    WD_ASSERT_DEV(sourceData.m_uiRowPitch == uiRowPitch, "Invalid row pitch. Expected {0} got {1}", uiRowPitch, sourceData.m_uiRowPitch);
    WD_ASSERT_DEV(sourceData.m_uiSlicePitch == 0 || sourceData.m_uiSlicePitch == uiSlicePitch, "Invalid slice pitch. Expected {0} got {1}",
      uiSlicePitch, sourceData.m_uiSlicePitch);

    if (MapResult.RowPitch == uiRowPitch && MapResult.DepthPitch == uiSlicePitch)
    {
      memcpy(MapResult.pData, sourceData.m_pData, uiSlicePitch * uiDepth);
    }
    else
    {
      // Copy row by row
      for (wdUInt32 z = 0; z < uiDepth; ++z)
      {
        const void* pSource = wdMemoryUtils::AddByteOffset(sourceData.m_pData, z * uiSlicePitch);
        void* pDest = wdMemoryUtils::AddByteOffset(MapResult.pData, z * MapResult.DepthPitch);

        for (wdUInt32 y = 0; y < uiHeight; ++y)
        {
          memcpy(pDest, pSource, uiRowPitch);

          pSource = wdMemoryUtils::AddByteOffset(pSource, uiRowPitch);
          pDest = wdMemoryUtils::AddByteOffset(pDest, MapResult.RowPitch);
        }
      }
    }

    m_pDXContext->Unmap(pDXTempTexture, 0);

    wdUInt32 dstSubResource = D3D11CalcSubresource(destinationSubResource.m_uiMipLevel, destinationSubResource.m_uiArraySlice, pDestination->GetDescription().m_uiMipLevelCount);

    D3D11_BOX srcBox = {0, 0, 0, uiWidth, uiHeight, uiDepth};
    m_pDXContext->CopySubresourceRegion(pDXDestination, dstSubResource, destinationBox.m_vMin.x, destinationBox.m_vMin.y, destinationBox.m_vMin.z, pDXTempTexture, 0, &srcBox);
  }
  else
  {
    WD_REPORT_FAILURE("Could not find a temp texture for update.");
  }
}

void wdGALCommandEncoderImplDX11::ResolveTexturePlatform(const wdGALTexture* pDestination, const wdGALTextureSubresource& destinationSubResource,
  const wdGALTexture* pSource, const wdGALTextureSubresource& sourceSubResource)
{
  ID3D11Resource* pDXDestination = static_cast<const wdGALTextureDX11*>(pDestination)->GetDXTexture();
  ID3D11Resource* pDXSource = static_cast<const wdGALTextureDX11*>(pSource)->GetDXTexture();

  wdUInt32 dstSubResource = D3D11CalcSubresource(destinationSubResource.m_uiMipLevel, destinationSubResource.m_uiArraySlice, pDestination->GetDescription().m_uiMipLevelCount);
  wdUInt32 srcSubResource = D3D11CalcSubresource(sourceSubResource.m_uiMipLevel, sourceSubResource.m_uiArraySlice, pSource->GetDescription().m_uiMipLevelCount);

  DXGI_FORMAT DXFormat = m_GALDeviceDX11.GetFormatLookupTable().GetFormatInfo(pDestination->GetDescription().m_Format).m_eResourceViewType;

  m_pDXContext->ResolveSubresource(pDXDestination, dstSubResource, pDXSource, srcSubResource, DXFormat);
}

void wdGALCommandEncoderImplDX11::ReadbackTexturePlatform(const wdGALTexture* pTexture)
{
  const wdGALTextureDX11* pDXTexture = static_cast<const wdGALTextureDX11*>(pTexture);

  // MSAA textures (e.g. backbuffers) need to be converted to non MSAA versions
  const bool bMSAASourceTexture = pDXTexture->GetDescription().m_SampleCount != wdGALMSAASampleCount::None;

  WD_ASSERT_DEV(pDXTexture->GetDXStagingTexture() != nullptr, "No staging resource available for read-back");
  WD_ASSERT_DEV(pDXTexture->GetDXTexture() != nullptr, "Texture object is invalid");

  if (bMSAASourceTexture)
  {
    /// \todo Other mip levels etc?
    m_pDXContext->ResolveSubresource(pDXTexture->GetDXStagingTexture(), 0, pDXTexture->GetDXTexture(), 0, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
  }
  else
  {
    m_pDXContext->CopyResource(pDXTexture->GetDXStagingTexture(), pDXTexture->GetDXTexture());
  }
}

wdUInt32 GetMipSize(wdUInt32 uiSize, wdUInt32 uiMipLevel)
{
  for (wdUInt32 i = 0; i < uiMipLevel; i++)
  {
    uiSize = uiSize / 2;
  }
  return wdMath::Max(1u, uiSize);
}

void wdGALCommandEncoderImplDX11::CopyTextureReadbackResultPlatform(const wdGALTexture* pTexture, wdArrayPtr<wdGALTextureSubresource> sourceSubResource, wdArrayPtr<wdGALSystemMemoryDescription> targetData)
{
  const wdGALTextureDX11* pDXTexture = static_cast<const wdGALTextureDX11*>(pTexture);

  WD_ASSERT_DEV(pDXTexture->GetDXStagingTexture() != nullptr, "No staging resource available for read-back");
  WD_ASSERT_DEV(sourceSubResource.GetCount() == targetData.GetCount(), "Source and target arrays must be of the same size.");

  const wdUInt32 uiSubResources = sourceSubResource.GetCount();
  for (wdUInt32 i = 0; i < uiSubResources; i++)
  {
    const wdGALTextureSubresource& subRes = sourceSubResource[i];
    const wdGALSystemMemoryDescription& memDesc = targetData[i];
    const wdUInt32 uiSubResourceIndex = D3D11CalcSubresource(subRes.m_uiMipLevel, subRes.m_uiArraySlice, pTexture->GetDescription().m_uiMipLevelCount);

    D3D11_MAPPED_SUBRESOURCE Mapped;
    if (SUCCEEDED(m_pDXContext->Map(pDXTexture->GetDXStagingTexture(), uiSubResourceIndex, D3D11_MAP_READ, 0, &Mapped)))
    {
      // TODO: Depth pitch
      if (Mapped.RowPitch == memDesc.m_uiRowPitch)
      {
        const wdUInt32 uiMemorySize = wdGALResourceFormat::GetBitsPerElement(pDXTexture->GetDescription().m_Format) *
                                      GetMipSize(pDXTexture->GetDescription().m_uiWidth, subRes.m_uiMipLevel) *
                                      GetMipSize(pDXTexture->GetDescription().m_uiHeight, subRes.m_uiMipLevel) / 8;
        memcpy(memDesc.m_pData, Mapped.pData, uiMemorySize);
      }
      else
      {
        // Copy row by row
        const wdUInt32 uiHeight = GetMipSize(pDXTexture->GetDescription().m_uiHeight, subRes.m_uiMipLevel);
        for (wdUInt32 y = 0; y < uiHeight; ++y)
        {
          const void* pSource = wdMemoryUtils::AddByteOffset(Mapped.pData, y * Mapped.RowPitch);
          void* pDest = wdMemoryUtils::AddByteOffset(memDesc.m_pData, y * memDesc.m_uiRowPitch);

          memcpy(
            pDest, pSource, wdGALResourceFormat::GetBitsPerElement(pDXTexture->GetDescription().m_Format) * GetMipSize(pDXTexture->GetDescription().m_uiWidth, subRes.m_uiMipLevel) / 8);
        }
      }

      m_pDXContext->Unmap(pDXTexture->GetDXStagingTexture(), uiSubResourceIndex);
    }
  }
}

void wdGALCommandEncoderImplDX11::GenerateMipMapsPlatform(const wdGALResourceView* pResourceView)
{
  const wdGALResourceViewDX11* pDXResourceView = static_cast<const wdGALResourceViewDX11*>(pResourceView);

  m_pDXContext->GenerateMips(pDXResourceView->GetDXResourceView());
}

void wdGALCommandEncoderImplDX11::FlushPlatform()
{
  FlushDeferredStateChanges();
}

// Debug helper functions

void wdGALCommandEncoderImplDX11::PushMarkerPlatform(const char* szMarker)
{
  if (m_pDXAnnotation != nullptr)
  {
    wdStringWChar wsMarker(szMarker);
    m_pDXAnnotation->BeginEvent(wsMarker.GetData());
  }
}

void wdGALCommandEncoderImplDX11::PopMarkerPlatform()
{
  if (m_pDXAnnotation != nullptr)
  {
    m_pDXAnnotation->EndEvent();
  }
}

void wdGALCommandEncoderImplDX11::InsertEventMarkerPlatform(const char* szMarker)
{
  if (m_pDXAnnotation != nullptr)
  {
    wdStringWChar wsMarker(szMarker);
    m_pDXAnnotation->SetMarker(wsMarker.GetData());
  }
}

//////////////////////////////////////////////////////////////////////////

void wdGALCommandEncoderImplDX11::BeginRendering(const wdGALRenderingSetup& renderingSetup)
{
  if (m_RenderTargetSetup != renderingSetup.m_RenderTargetSetup)
  {
    m_RenderTargetSetup = renderingSetup.m_RenderTargetSetup;

    const wdGALRenderTargetView* pRenderTargetViews[WD_GAL_MAX_RENDERTARGET_COUNT] = {nullptr};
    const wdGALRenderTargetView* pDepthStencilView = nullptr;

    const wdUInt32 uiRenderTargetCount = m_RenderTargetSetup.GetRenderTargetCount();

    bool bFlushNeeded = false;

    for (wdUInt8 uiIndex = 0; uiIndex < uiRenderTargetCount; ++uiIndex)
    {
      const wdGALRenderTargetView* pRenderTargetView = m_GALDeviceDX11.GetRenderTargetView(m_RenderTargetSetup.GetRenderTarget(uiIndex));
      if (pRenderTargetView != nullptr)
      {
        const wdGALResourceBase* pTexture = pRenderTargetView->GetTexture()->GetParentResource();

        bFlushNeeded |= m_pOwner->UnsetResourceViews(pTexture);
        bFlushNeeded |= m_pOwner->UnsetUnorderedAccessViews(pTexture);
      }

      pRenderTargetViews[uiIndex] = pRenderTargetView;
    }

    pDepthStencilView = m_GALDeviceDX11.GetRenderTargetView(m_RenderTargetSetup.GetDepthStencilTarget());
    if (pDepthStencilView != nullptr)
    {
      const wdGALResourceBase* pTexture = pDepthStencilView->GetTexture()->GetParentResource();

      bFlushNeeded |= m_pOwner->UnsetResourceViews(pTexture);
      bFlushNeeded |= m_pOwner->UnsetUnorderedAccessViews(pTexture);
    }

    if (bFlushNeeded)
    {
      FlushPlatform();
    }

    for (wdUInt32 i = 0; i < WD_GAL_MAX_RENDERTARGET_COUNT; i++)
    {
      m_pBoundRenderTargets[i] = nullptr;
    }
    m_pBoundDepthStencilTarget = nullptr;

    if (uiRenderTargetCount != 0 || pDepthStencilView != nullptr)
    {
      for (wdUInt32 i = 0; i < uiRenderTargetCount; i++)
      {
        if (pRenderTargetViews[i] != nullptr)
        {
          m_pBoundRenderTargets[i] = static_cast<const wdGALRenderTargetViewDX11*>(pRenderTargetViews[i])->GetRenderTargetView();
        }
      }

      if (pDepthStencilView != nullptr)
      {
        m_pBoundDepthStencilTarget = static_cast<const wdGALRenderTargetViewDX11*>(pDepthStencilView)->GetDepthStencilView();
      }

      // Bind rendertargets, bind max(new rt count, old rt count) to overwrite bound rts if new count < old count
      m_pDXContext->OMSetRenderTargets(wdMath::Max(uiRenderTargetCount, m_uiBoundRenderTargetCount), m_pBoundRenderTargets, m_pBoundDepthStencilTarget);

      m_uiBoundRenderTargetCount = uiRenderTargetCount;
    }
    else
    {
      m_pBoundDepthStencilTarget = nullptr;
      m_pDXContext->OMSetRenderTargets(0, nullptr, nullptr);
      m_uiBoundRenderTargetCount = 0;
    }
  }

  ClearPlatform(renderingSetup.m_ClearColor, renderingSetup.m_uiRenderTargetClearMask, renderingSetup.m_bClearDepth, renderingSetup.m_bClearStencil, renderingSetup.m_fDepthClear, renderingSetup.m_uiStencilClear);
}

void wdGALCommandEncoderImplDX11::BeginCompute()
{
  // We need to unbind all render targets as otherwise using them in a compute shader as input will fail:
  // DEVICE_CSSETSHADERRESOURCES_HAZARD: Resource being set to CS shader resource slot 0 is still bound on output!
  m_RenderTargetSetup = wdGALRenderTargetSetup();
  m_pDXContext->OMSetRenderTargets(0, nullptr, nullptr);
}

// Draw functions

void wdGALCommandEncoderImplDX11::ClearPlatform(const wdColor& clearColor, wdUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, wdUInt8 uiStencilClear)
{
  for (wdUInt32 i = 0; i < m_uiBoundRenderTargetCount; i++)
  {
    if (uiRenderTargetClearMask & (1u << i) && m_pBoundRenderTargets[i])
    {
      m_pDXContext->ClearRenderTargetView(m_pBoundRenderTargets[i], clearColor.GetData());
    }
  }

  if ((bClearDepth || bClearStencil) && m_pBoundDepthStencilTarget)
  {
    wdUInt32 uiClearFlags = bClearDepth ? D3D11_CLEAR_DEPTH : 0;
    uiClearFlags |= bClearStencil ? D3D11_CLEAR_STENCIL : 0;

    m_pDXContext->ClearDepthStencilView(m_pBoundDepthStencilTarget, uiClearFlags, fDepthClear, uiStencilClear);
  }
}

void wdGALCommandEncoderImplDX11::DrawPlatform(wdUInt32 uiVertexCount, wdUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

  m_pDXContext->Draw(uiVertexCount, uiStartVertex);
}

void wdGALCommandEncoderImplDX11::DrawIndexedPlatform(wdUInt32 uiIndexCount, wdUInt32 uiStartIndex)
{
  FlushDeferredStateChanges();

#if WD_ENABLED(WD_COMPILE_FOR_DEBUG)
  m_pDXContext->DrawIndexed(uiIndexCount, uiStartIndex, 0);

  // In debug builds, with a debugger attached, the engine will break on D3D errors
  // this can be very annoying when an error happens repeatedly
  // you can disable it at runtime, by using the debugger to set bChangeBreakPolicy to 'true', or dragging the
  // the instruction pointer into the if
  volatile bool bChangeBreakPolicy = false;
  if (bChangeBreakPolicy)
  {
    if (m_GALDeviceDX11.m_pDebug != nullptr)
    {
      ID3D11InfoQueue* pInfoQueue = nullptr;
      if (SUCCEEDED(m_GALDeviceDX11.m_pDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&pInfoQueue)))
      {
        // modify these, if you want to keep certain things enabled
        static BOOL bBreakOnCorruption = FALSE;
        static BOOL bBreakOnError = FALSE;
        static BOOL bBreakOnWarning = FALSE;

        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, bBreakOnCorruption);
        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, bBreakOnError);
        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, bBreakOnWarning);
      }
    }
  }
#else
  m_pDXContext->DrawIndexed(uiIndexCount, uiStartIndex, 0);
#endif
}

void wdGALCommandEncoderImplDX11::DrawIndexedInstancedPlatform(wdUInt32 uiIndexCountPerInstance, wdUInt32 uiInstanceCount, wdUInt32 uiStartIndex)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawIndexedInstanced(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex, 0, 0);
}

void wdGALCommandEncoderImplDX11::DrawIndexedInstancedIndirectPlatform(const wdGALBuffer* pIndirectArgumentBuffer, wdUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawIndexedInstancedIndirect(static_cast<const wdGALBufferDX11*>(pIndirectArgumentBuffer)->GetDXBuffer(), uiArgumentOffsetInBytes);
}

void wdGALCommandEncoderImplDX11::DrawInstancedPlatform(wdUInt32 uiVertexCountPerInstance, wdUInt32 uiInstanceCount, wdUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawInstanced(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex, 0);
}

void wdGALCommandEncoderImplDX11::DrawInstancedIndirectPlatform(const wdGALBuffer* pIndirectArgumentBuffer, wdUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawInstancedIndirect(static_cast<const wdGALBufferDX11*>(pIndirectArgumentBuffer)->GetDXBuffer(), uiArgumentOffsetInBytes);
}

void wdGALCommandEncoderImplDX11::DrawAutoPlatform()
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawAuto();
}

void wdGALCommandEncoderImplDX11::BeginStreamOutPlatform()
{
  FlushDeferredStateChanges();

  WD_ASSERT_NOT_IMPLEMENTED;
}

void wdGALCommandEncoderImplDX11::EndStreamOutPlatform()
{
  WD_ASSERT_NOT_IMPLEMENTED;
}

void wdGALCommandEncoderImplDX11::SetIndexBufferPlatform(const wdGALBuffer* pIndexBuffer)
{
  if (pIndexBuffer != nullptr)
  {
    const wdGALBufferDX11* pDX11Buffer = static_cast<const wdGALBufferDX11*>(pIndexBuffer);
    m_pDXContext->IASetIndexBuffer(pDX11Buffer->GetDXBuffer(), pDX11Buffer->GetIndexFormat(), 0 /* \todo: Expose */);
  }
  else
  {
    m_pDXContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
  }
}

void wdGALCommandEncoderImplDX11::SetVertexBufferPlatform(wdUInt32 uiSlot, const wdGALBuffer* pVertexBuffer)
{
  WD_ASSERT_DEV(uiSlot < WD_GAL_MAX_VERTEX_BUFFER_COUNT, "Invalid slot index");

  m_pBoundVertexBuffers[uiSlot] = pVertexBuffer != nullptr ? static_cast<const wdGALBufferDX11*>(pVertexBuffer)->GetDXBuffer() : nullptr;
  m_VertexBufferStrides[uiSlot] = pVertexBuffer != nullptr ? pVertexBuffer->GetDescription().m_uiStructSize : 0;
  m_BoundVertexBuffersRange.SetToIncludeValue(uiSlot);
}

void wdGALCommandEncoderImplDX11::SetVertexDeclarationPlatform(const wdGALVertexDeclaration* pVertexDeclaration)
{
  m_pDXContext->IASetInputLayout(
    pVertexDeclaration != nullptr ? static_cast<const wdGALVertexDeclarationDX11*>(pVertexDeclaration)->GetDXInputLayout() : nullptr);
}

static const D3D11_PRIMITIVE_TOPOLOGY GALTopologyToDX11[wdGALPrimitiveTopology::ENUM_COUNT] = {
  D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
  D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
  D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
};

void wdGALCommandEncoderImplDX11::SetPrimitiveTopologyPlatform(wdGALPrimitiveTopology::Enum topology)
{
  m_pDXContext->IASetPrimitiveTopology(GALTopologyToDX11[topology]);
}

void wdGALCommandEncoderImplDX11::SetBlendStatePlatform(const wdGALBlendState* pBlendState, const wdColor& blendFactor, wdUInt32 uiSampleMask)
{
  FLOAT BlendFactors[4] = {blendFactor.r, blendFactor.g, blendFactor.b, blendFactor.a};

  m_pDXContext->OMSetBlendState(
    pBlendState != nullptr ? static_cast<const wdGALBlendStateDX11*>(pBlendState)->GetDXBlendState() : nullptr, BlendFactors, uiSampleMask);
}

void wdGALCommandEncoderImplDX11::SetDepthStencilStatePlatform(const wdGALDepthStencilState* pDepthStencilState, wdUInt8 uiStencilRefValue)
{
  m_pDXContext->OMSetDepthStencilState(
    pDepthStencilState != nullptr ? static_cast<const wdGALDepthStencilStateDX11*>(pDepthStencilState)->GetDXDepthStencilState() : nullptr,
    uiStencilRefValue);
}

void wdGALCommandEncoderImplDX11::SetRasterizerStatePlatform(const wdGALRasterizerState* pRasterizerState)
{
  m_pDXContext->RSSetState(pRasterizerState != nullptr ? static_cast<const wdGALRasterizerStateDX11*>(pRasterizerState)->GetDXRasterizerState() : nullptr);
}

void wdGALCommandEncoderImplDX11::SetViewportPlatform(const wdRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  D3D11_VIEWPORT Viewport;
  Viewport.TopLeftX = rect.x;
  Viewport.TopLeftY = rect.y;
  Viewport.Width = rect.width;
  Viewport.Height = rect.height;
  Viewport.MinDepth = fMinDepth;
  Viewport.MaxDepth = fMaxDepth;

  m_pDXContext->RSSetViewports(1, &Viewport);
}

void wdGALCommandEncoderImplDX11::SetScissorRectPlatform(const wdRectU32& rect)
{
  D3D11_RECT ScissorRect;
  ScissorRect.left = rect.x;
  ScissorRect.top = rect.y;
  ScissorRect.right = rect.x + rect.width;
  ScissorRect.bottom = rect.y + rect.height;

  m_pDXContext->RSSetScissorRects(1, &ScissorRect);
}

void wdGALCommandEncoderImplDX11::SetStreamOutBufferPlatform(wdUInt32 uiSlot, const wdGALBuffer* pBuffer, wdUInt32 uiOffset)
{
  WD_ASSERT_NOT_IMPLEMENTED;
}

//////////////////////////////////////////////////////////////////////////

void wdGALCommandEncoderImplDX11::DispatchPlatform(wdUInt32 uiThreadGroupCountX, wdUInt32 uiThreadGroupCountY, wdUInt32 uiThreadGroupCountZ)
{
  FlushDeferredStateChanges();

  m_pDXContext->Dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

void wdGALCommandEncoderImplDX11::DispatchIndirectPlatform(const wdGALBuffer* pIndirectArgumentBuffer, wdUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  m_pDXContext->DispatchIndirect(static_cast<const wdGALBufferDX11*>(pIndirectArgumentBuffer)->GetDXBuffer(), uiArgumentOffsetInBytes);
}

//////////////////////////////////////////////////////////////////////////

static void SetShaderResources(wdGALShaderStage::Enum stage, ID3D11DeviceContext* pContext, wdUInt32 uiStartSlot, wdUInt32 uiNumSlots,
  ID3D11ShaderResourceView** pShaderResourceViews)
{
  switch (stage)
  {
    case wdGALShaderStage::VertexShader:
      pContext->VSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case wdGALShaderStage::HullShader:
      pContext->HSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case wdGALShaderStage::DomainShader:
      pContext->DSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case wdGALShaderStage::GeometryShader:
      pContext->GSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case wdGALShaderStage::PixelShader:
      pContext->PSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case wdGALShaderStage::ComputeShader:
      pContext->CSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    default:
      WD_ASSERT_NOT_IMPLEMENTED;
  }
}

static void SetConstantBuffers(
  wdGALShaderStage::Enum stage, ID3D11DeviceContext* pContext, wdUInt32 uiStartSlot, wdUInt32 uiNumSlots, ID3D11Buffer** pConstantBuffers)
{
  switch (stage)
  {
    case wdGALShaderStage::VertexShader:
      pContext->VSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case wdGALShaderStage::HullShader:
      pContext->HSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case wdGALShaderStage::DomainShader:
      pContext->DSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case wdGALShaderStage::GeometryShader:
      pContext->GSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case wdGALShaderStage::PixelShader:
      pContext->PSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case wdGALShaderStage::ComputeShader:
      pContext->CSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    default:
      WD_ASSERT_NOT_IMPLEMENTED;
  }
}

static void SetSamplers(
  wdGALShaderStage::Enum stage, ID3D11DeviceContext* pContext, wdUInt32 uiStartSlot, wdUInt32 uiNumSlots, ID3D11SamplerState** pSamplerStates)
{
  switch (stage)
  {
    case wdGALShaderStage::VertexShader:
      pContext->VSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case wdGALShaderStage::HullShader:
      pContext->HSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case wdGALShaderStage::DomainShader:
      pContext->DSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case wdGALShaderStage::GeometryShader:
      pContext->GSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case wdGALShaderStage::PixelShader:
      pContext->PSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case wdGALShaderStage::ComputeShader:
      pContext->CSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    default:
      WD_ASSERT_NOT_IMPLEMENTED;
  }
}

// Some state changes are deferred so they can be updated faster
void wdGALCommandEncoderImplDX11::FlushDeferredStateChanges()
{
  if (m_BoundVertexBuffersRange.IsValid())
  {
    const wdUInt32 uiStartSlot = m_BoundVertexBuffersRange.m_uiMin;
    const wdUInt32 uiNumSlots = m_BoundVertexBuffersRange.GetCount();

    m_pDXContext->IASetVertexBuffers(uiStartSlot, uiNumSlots, m_pBoundVertexBuffers + uiStartSlot, m_VertexBufferStrides + uiStartSlot, m_VertexBufferOffsets + uiStartSlot);

    m_BoundVertexBuffersRange.Reset();
  }

  for (wdUInt32 stage = 0; stage < wdGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (m_pBoundShaders[stage] != nullptr && m_BoundConstantBuffersRange[stage].IsValid())
    {
      const wdUInt32 uiStartSlot = m_BoundConstantBuffersRange[stage].m_uiMin;
      const wdUInt32 uiNumSlots = m_BoundConstantBuffersRange[stage].GetCount();

      SetConstantBuffers((wdGALShaderStage::Enum)stage, m_pDXContext, uiStartSlot, uiNumSlots, m_pBoundConstantBuffers + uiStartSlot);

      m_BoundConstantBuffersRange[stage].Reset();
    }
  }

  // Do UAV bindings before SRV since UAV are outputs which need to be unbound before they are potentially rebound as SRV again.
  if (m_BoundUnoderedAccessViewsRange.IsValid())
  {
    const wdUInt32 uiStartSlot = m_BoundUnoderedAccessViewsRange.m_uiMin;
    const wdUInt32 uiNumSlots = m_BoundUnoderedAccessViewsRange.GetCount();
    m_pDXContext->CSSetUnorderedAccessViews(uiStartSlot, uiNumSlots, m_BoundUnoderedAccessViews.GetData() + uiStartSlot, nullptr); // Todo: Count reset.

    m_BoundUnoderedAccessViewsRange.Reset();
  }

  for (wdUInt32 stage = 0; stage < wdGALShaderStage::ENUM_COUNT; ++stage)
  {
    // Need to do bindings even on inactive shader stages since we might miss unbindings otherwise!
    if (m_BoundShaderResourceViewsRange[stage].IsValid())
    {
      const wdUInt32 uiStartSlot = m_BoundShaderResourceViewsRange[stage].m_uiMin;
      const wdUInt32 uiNumSlots = m_BoundShaderResourceViewsRange[stage].GetCount();

      SetShaderResources((wdGALShaderStage::Enum)stage, m_pDXContext, uiStartSlot, uiNumSlots, m_pBoundShaderResourceViews[stage].GetData() + uiStartSlot);

      m_BoundShaderResourceViewsRange[stage].Reset();
    }

    // Don't need to unset sampler stages for unbound shader stages.
    if (m_pBoundShaders[stage] == nullptr)
      continue;

    if (m_BoundSamplerStatesRange[stage].IsValid())
    {
      const wdUInt32 uiStartSlot = m_BoundSamplerStatesRange[stage].m_uiMin;
      const wdUInt32 uiNumSlots = m_BoundSamplerStatesRange[stage].GetCount();

      SetSamplers((wdGALShaderStage::Enum)stage, m_pDXContext, uiStartSlot, uiNumSlots, m_pBoundSamplerStates[stage] + uiStartSlot);

      m_BoundSamplerStatesRange[stage].Reset();
    }
  }
}
