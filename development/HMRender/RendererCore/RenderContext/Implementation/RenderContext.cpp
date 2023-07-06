#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Shader/ShaderPermutationResource.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

wdRenderContext* wdRenderContext::s_pDefaultInstance = nullptr;
wdHybridArray<wdRenderContext*, 4> wdRenderContext::s_Instances;

wdMap<wdRenderContext::ShaderVertexDecl, wdGALVertexDeclarationHandle> wdRenderContext::s_GALVertexDeclarations;

wdMutex wdRenderContext::s_ConstantBufferStorageMutex;
wdIdTable<wdConstantBufferStorageId, wdConstantBufferStorageBase*> wdRenderContext::s_ConstantBufferStorageTable;
wdMap<wdUInt32, wdDynamicArray<wdConstantBufferStorageBase*>> wdRenderContext::s_FreeConstantBufferStorage;

wdGALSamplerStateHandle wdRenderContext::s_hDefaultSamplerStates[4];

// clang-format off
WD_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, RendererContext)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation",
  "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    wdShaderUtils::g_RequestBuiltinShaderCallback = wdMakeDelegate(wdRenderContext::LoadBuiltinShader);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    wdShaderUtils::g_RequestBuiltinShaderCallback = {};
    wdRenderContext::OnEngineShutdown();
  }

WD_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////

wdRenderContext::Statistics::Statistics()
{
  Reset();
}

void wdRenderContext::Statistics::Reset()
{
  m_uiFailedDrawcalls = 0;
}

//////////////////////////////////////////////////////////////////////////

wdRenderContext* wdRenderContext::GetDefaultInstance()
{
  if (s_pDefaultInstance == nullptr)
    s_pDefaultInstance = CreateInstance();

  return s_pDefaultInstance;
}

wdRenderContext* wdRenderContext::CreateInstance()
{
  return WD_DEFAULT_NEW(wdRenderContext);
}

void wdRenderContext::DestroyInstance(wdRenderContext* pRenderer)
{
  WD_DEFAULT_DELETE(pRenderer);
}

wdRenderContext::wdRenderContext()
{
  if (s_pDefaultInstance == nullptr)
  {
    s_pDefaultInstance = this;
  }

  s_Instances.PushBack(this);

  m_StateFlags = wdRenderContextFlags::AllStatesInvalid;
  m_Topology = wdGALPrimitiveTopology::ENUM_COUNT; // Set to something invalid
  m_uiMeshBufferPrimitiveCount = 0;
  m_DefaultTextureFilter = wdTextureFilterSetting::FixedAnisotropic4x;
  m_bAllowAsyncShaderLoading = false;

  m_hGlobalConstantBufferStorage = CreateConstantBufferStorage<wdGlobalConstants>();

  ResetContextState();
}

wdRenderContext::~wdRenderContext()
{
  DeleteConstantBufferStorage(m_hGlobalConstantBufferStorage);

  if (s_pDefaultInstance == this)
    s_pDefaultInstance = nullptr;

  s_Instances.RemoveAndSwap(this);
}

wdRenderContext::Statistics wdRenderContext::GetAndResetStatistics()
{
  wdRenderContext::Statistics ret = m_Statistics;
  ret.Reset();

  return ret;
}

wdGALRenderCommandEncoder* wdRenderContext::BeginRendering(wdGALPass* pGALPass, const wdGALRenderingSetup& renderingSetup, const wdRectFloat& viewport, const char* szName, bool bStereoSupport)
{
  wdGALMSAASampleCount::Enum msaaSampleCount = wdGALMSAASampleCount::None;

  wdGALRenderTargetViewHandle hRTV;
  if (renderingSetup.m_RenderTargetSetup.GetRenderTargetCount() > 0)
  {
    hRTV = renderingSetup.m_RenderTargetSetup.GetRenderTarget(0);
  }
  if (hRTV.IsInvalidated())
  {
    hRTV = renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget();
  }

  if (const wdGALRenderTargetView* pRTV = wdGALDevice::GetDefaultDevice()->GetRenderTargetView(hRTV))
  {
    msaaSampleCount = pRTV->GetTexture()->GetDescription().m_SampleCount;
  }

  if (msaaSampleCount != wdGALMSAASampleCount::None)
  {
    SetShaderPermutationVariable("MSAA", "TRUE");
  }
  else
  {
    SetShaderPermutationVariable("MSAA", "FALSE");
  }

  auto& gc = WriteGlobalConstants();
  gc.ViewportSize = wdVec4(viewport.width, viewport.height, 1.0f / viewport.width, 1.0f / viewport.height);
  gc.NumMsaaSamples = msaaSampleCount;

  auto pGALCommandEncoder = pGALPass->BeginRendering(renderingSetup, szName);

  pGALCommandEncoder->SetViewport(viewport);

  m_pGALPass = pGALPass;
  m_pGALCommandEncoder = pGALCommandEncoder;
  m_bCompute = false;
  m_bStereoRendering = bStereoSupport;

  return pGALCommandEncoder;
}

void wdRenderContext::EndRendering()
{
  m_pGALPass->EndRendering(GetRenderCommandEncoder());

  m_pGALPass = nullptr;
  m_pGALCommandEncoder = nullptr;
  m_bStereoRendering = false;

  // TODO: The render context needs to reset its state after every encoding block if we want to record to separate command buffers.
  // Although this is currently not possible since a lot of high level code binds stuff only once per frame on the render context.
  // Resetting the state after every encoding block breaks those assumptions.
  //ResetContextState();
}

wdGALComputeCommandEncoder* wdRenderContext::BeginCompute(wdGALPass* pGALPass, const char* szName /*= ""*/)
{
  auto pGALCommandEncoder = pGALPass->BeginCompute(szName);

  m_pGALPass = pGALPass;
  m_pGALCommandEncoder = pGALCommandEncoder;
  m_bCompute = true;

  return pGALCommandEncoder;
}

void wdRenderContext::EndCompute()
{
  m_pGALPass->EndCompute(GetComputeCommandEncoder());

  m_pGALPass = nullptr;
  m_pGALCommandEncoder = nullptr;

  // TODO: See EndRendering
  //ResetContextState();
}

void wdRenderContext::SetShaderPermutationVariable(const char* szName, const wdTempHashedString& sTempValue)
{
  wdTempHashedString sHashedName(szName);

  wdHashedString sName;
  wdHashedString sValue;
  if (wdShaderManager::IsPermutationValueAllowed(szName, sHashedName, sTempValue, sName, sValue))
  {
    SetShaderPermutationVariableInternal(sName, sValue);
  }
}

void wdRenderContext::SetShaderPermutationVariable(const wdHashedString& sName, const wdHashedString& sValue)
{
  if (wdShaderManager::IsPermutationValueAllowed(sName, sValue))
  {
    SetShaderPermutationVariableInternal(sName, sValue);
  }
}


void wdRenderContext::BindMaterial(const wdMaterialResourceHandle& hMaterial)
{
  // Don't set m_hMaterial directly since we first need to check whether the material has been modified in the mean time.
  m_hNewMaterial = hMaterial;
  m_StateFlags.Add(wdRenderContextFlags::MaterialBindingChanged);
}

void wdRenderContext::BindTexture2D(const wdTempHashedString& sSlotName, const wdTexture2DResourceHandle& hTexture,
  wdResourceAcquireMode acquireMode /*= wdResourceAcquireMode::AllowLoadingFallback*/)
{
  if (hTexture.IsValid())
  {
    wdResourceLock<wdTexture2DResource> pTexture(hTexture, acquireMode);
    BindTexture2D(sSlotName, wdGALDevice::GetDefaultDevice()->GetDefaultResourceView(pTexture->GetGALTexture()));
    BindSamplerState(sSlotName, pTexture->GetGALSamplerState());
  }
  else
  {
    BindTexture2D(sSlotName, wdGALResourceViewHandle());
  }
}

void wdRenderContext::BindTexture3D(const wdTempHashedString& sSlotName, const wdTexture3DResourceHandle& hTexture,
  wdResourceAcquireMode acquireMode /*= wdResourceAcquireMode::AllowLoadingFallback*/)
{
  if (hTexture.IsValid())
  {
    wdResourceLock<wdTexture3DResource> pTexture(hTexture, acquireMode);
    BindTexture3D(sSlotName, wdGALDevice::GetDefaultDevice()->GetDefaultResourceView(pTexture->GetGALTexture()));
    BindSamplerState(sSlotName, pTexture->GetGALSamplerState());
  }
  else
  {
    BindTexture3D(sSlotName, wdGALResourceViewHandle());
  }
}

void wdRenderContext::BindTextureCube(const wdTempHashedString& sSlotName, const wdTextureCubeResourceHandle& hTexture,
  wdResourceAcquireMode acquireMode /*= wdResourceAcquireMode::AllowLoadingFallback*/)
{
  if (hTexture.IsValid())
  {
    wdResourceLock<wdTextureCubeResource> pTexture(hTexture, acquireMode);
    BindTextureCube(sSlotName, wdGALDevice::GetDefaultDevice()->GetDefaultResourceView(pTexture->GetGALTexture()));
    BindSamplerState(sSlotName, pTexture->GetGALSamplerState());
  }
  else
  {
    BindTextureCube(sSlotName, wdGALResourceViewHandle());
  }
}

void wdRenderContext::BindTexture2D(const wdTempHashedString& sSlotName, wdGALResourceViewHandle hResourceView)
{
  wdGALResourceViewHandle* pOldResourceView = nullptr;
  if (m_BoundTextures2D.TryGetValue(sSlotName.GetHash(), pOldResourceView))
  {
    if (*pOldResourceView == hResourceView)
      return;

    *pOldResourceView = hResourceView;
  }
  else
  {
    m_BoundTextures2D.Insert(sSlotName.GetHash(), hResourceView);
  }

  m_StateFlags.Add(wdRenderContextFlags::TextureBindingChanged);
}

void wdRenderContext::BindTexture3D(const wdTempHashedString& sSlotName, wdGALResourceViewHandle hResourceView)
{
  wdGALResourceViewHandle* pOldResourceView = nullptr;
  if (m_BoundTextures3D.TryGetValue(sSlotName.GetHash(), pOldResourceView))
  {
    if (*pOldResourceView == hResourceView)
      return;

    *pOldResourceView = hResourceView;
  }
  else
  {
    m_BoundTextures3D.Insert(sSlotName.GetHash(), hResourceView);
  }

  m_StateFlags.Add(wdRenderContextFlags::TextureBindingChanged);
}

void wdRenderContext::BindTextureCube(const wdTempHashedString& sSlotName, wdGALResourceViewHandle hResourceView)
{
  wdGALResourceViewHandle* pOldResourceView = nullptr;
  if (m_BoundTexturesCube.TryGetValue(sSlotName.GetHash(), pOldResourceView))
  {
    if (*pOldResourceView == hResourceView)
      return;

    *pOldResourceView = hResourceView;
  }
  else
  {
    m_BoundTexturesCube.Insert(sSlotName.GetHash(), hResourceView);
  }

  m_StateFlags.Add(wdRenderContextFlags::TextureBindingChanged);
}

void wdRenderContext::BindUAV(const wdTempHashedString& sSlotName, wdGALUnorderedAccessViewHandle hUnorderedAccessView)
{
  wdGALUnorderedAccessViewHandle* pOldResourceView = nullptr;
  if (m_BoundUAVs.TryGetValue(sSlotName.GetHash(), pOldResourceView))
  {
    if (*pOldResourceView == hUnorderedAccessView)
      return;

    *pOldResourceView = hUnorderedAccessView;
  }
  else
  {
    m_BoundUAVs.Insert(sSlotName.GetHash(), hUnorderedAccessView);
  }

  m_StateFlags.Add(wdRenderContextFlags::UAVBindingChanged);
}


void wdRenderContext::BindSamplerState(const wdTempHashedString& sSlotName, wdGALSamplerStateHandle hSamplerSate)
{
  WD_ASSERT_DEBUG(sSlotName != "LinearSampler", "'LinearSampler' is a resevered sampler name and must not be set manually.");
  WD_ASSERT_DEBUG(sSlotName != "LinearClampSampler", "'LinearClampSampler' is a resevered sampler name and must not be set manually.");
  WD_ASSERT_DEBUG(sSlotName != "PointSampler", "'PointSampler' is a resevered sampler name and must not be set manually.");
  WD_ASSERT_DEBUG(sSlotName != "PointClampSampler", "'PointClampSampler' is a resevered sampler name and must not be set manually.");

  wdGALSamplerStateHandle* pOldSamplerState = nullptr;
  if (m_BoundSamplers.TryGetValue(sSlotName.GetHash(), pOldSamplerState))
  {
    if (*pOldSamplerState == hSamplerSate)
      return;

    *pOldSamplerState = hSamplerSate;
  }
  else
  {
    m_BoundSamplers.Insert(sSlotName.GetHash(), hSamplerSate);
  }

  m_StateFlags.Add(wdRenderContextFlags::SamplerBindingChanged);
}

void wdRenderContext::BindBuffer(const wdTempHashedString& sSlotName, wdGALResourceViewHandle hResourceView)
{
  wdGALResourceViewHandle* pOldResourceView = nullptr;
  if (m_BoundBuffer.TryGetValue(sSlotName.GetHash(), pOldResourceView))
  {
    if (*pOldResourceView == hResourceView)
      return;

    *pOldResourceView = hResourceView;
  }
  else
  {
    m_BoundBuffer.Insert(sSlotName.GetHash(), hResourceView);
  }

  m_StateFlags.Add(wdRenderContextFlags::BufferBindingChanged);
}

void wdRenderContext::BindConstantBuffer(const wdTempHashedString& sSlotName, wdGALBufferHandle hConstantBuffer)
{
  BoundConstantBuffer* pBoundConstantBuffer = nullptr;
  if (m_BoundConstantBuffers.TryGetValue(sSlotName.GetHash(), pBoundConstantBuffer))
  {
    if (pBoundConstantBuffer->m_hConstantBuffer == hConstantBuffer)
      return;

    pBoundConstantBuffer->m_hConstantBuffer = hConstantBuffer;
    pBoundConstantBuffer->m_hConstantBufferStorage.Invalidate();
  }
  else
  {
    m_BoundConstantBuffers.Insert(sSlotName.GetHash(), BoundConstantBuffer(hConstantBuffer));
  }

  m_StateFlags.Add(wdRenderContextFlags::ConstantBufferBindingChanged);
}

void wdRenderContext::BindConstantBuffer(const wdTempHashedString& sSlotName, wdConstantBufferStorageHandle hConstantBufferStorage)
{
  BoundConstantBuffer* pBoundConstantBuffer = nullptr;
  if (m_BoundConstantBuffers.TryGetValue(sSlotName.GetHash(), pBoundConstantBuffer))
  {
    if (pBoundConstantBuffer->m_hConstantBufferStorage == hConstantBufferStorage)
      return;

    pBoundConstantBuffer->m_hConstantBuffer.Invalidate();
    pBoundConstantBuffer->m_hConstantBufferStorage = hConstantBufferStorage;
  }
  else
  {
    m_BoundConstantBuffers.Insert(sSlotName.GetHash(), BoundConstantBuffer(hConstantBufferStorage));
  }

  m_StateFlags.Add(wdRenderContextFlags::ConstantBufferBindingChanged);
}

void wdRenderContext::BindShader(const wdShaderResourceHandle& hShader, wdBitflags<wdShaderBindFlags> flags)
{
  m_hMaterial.Invalidate();
  m_StateFlags.Remove(wdRenderContextFlags::MaterialBindingChanged);

  BindShaderInternal(hShader, flags);
}

void wdRenderContext::BindMeshBuffer(const wdMeshBufferResourceHandle& hMeshBuffer)
{
  wdResourceLock<wdMeshBufferResource> pMeshBuffer(hMeshBuffer, wdResourceAcquireMode::AllowLoadingFallback);
  BindMeshBuffer(pMeshBuffer->GetVertexBuffer(), pMeshBuffer->GetIndexBuffer(), &(pMeshBuffer->GetVertexDeclaration()), pMeshBuffer->GetTopology(),
    pMeshBuffer->GetPrimitiveCount());
}

void wdRenderContext::BindMeshBuffer(wdGALBufferHandle hVertexBuffer, wdGALBufferHandle hIndexBuffer,
  const wdVertexDeclarationInfo* pVertexDeclarationInfo, wdGALPrimitiveTopology::Enum topology, wdUInt32 uiPrimitiveCount, wdGALBufferHandle hVertexBuffer2, wdGALBufferHandle hVertexBuffer3, wdGALBufferHandle hVertexBuffer4)
{
  if (m_hVertexBuffers[0] == hVertexBuffer && m_hVertexBuffers[1] == hVertexBuffer2 && m_hVertexBuffers[2] == hVertexBuffer3 && m_hVertexBuffers[3] == hVertexBuffer4 && m_hIndexBuffer == hIndexBuffer && m_pVertexDeclarationInfo == pVertexDeclarationInfo && m_Topology == topology && m_uiMeshBufferPrimitiveCount == uiPrimitiveCount)
  {
    return;
  }

#if WD_ENABLED(WD_COMPILE_FOR_DEBUG)
  if (pVertexDeclarationInfo)
  {
    for (wdUInt32 i1 = 0; i1 < pVertexDeclarationInfo->m_VertexStreams.GetCount(); ++i1)
    {
      for (wdUInt32 i2 = 0; i2 < pVertexDeclarationInfo->m_VertexStreams.GetCount(); ++i2)
      {
        if (i1 != i2)
        {
          WD_ASSERT_DEBUG(pVertexDeclarationInfo->m_VertexStreams[i1].m_Semantic != pVertexDeclarationInfo->m_VertexStreams[i2].m_Semantic,
            "Same semantic cannot be used twice in the same vertex declaration");
        }
      }
    }
  }
#endif

  if (m_Topology != topology)
  {
    m_Topology = topology;

    wdTempHashedString sTopologies[wdGALPrimitiveTopology::ENUM_COUNT] = {
      wdTempHashedString("TOPOLOGY_POINTS"), wdTempHashedString("TOPOLOGY_LINES"), wdTempHashedString("TOPOLOGY_TRIANGLES")};

    SetShaderPermutationVariable("TOPOLOGY", sTopologies[m_Topology]);
  }

  m_hVertexBuffers[0] = hVertexBuffer;
  m_hVertexBuffers[1] = hVertexBuffer2;
  m_hVertexBuffers[2] = hVertexBuffer3;
  m_hVertexBuffers[3] = hVertexBuffer4;
  m_hIndexBuffer = hIndexBuffer;
  m_pVertexDeclarationInfo = pVertexDeclarationInfo;
  m_uiMeshBufferPrimitiveCount = uiPrimitiveCount;

  m_StateFlags.Add(wdRenderContextFlags::MeshBufferBindingChanged);
}

void wdRenderContext::BindMeshBuffer(const wdDynamicMeshBufferResourceHandle& hDynamicMeshBuffer)
{
  wdResourceLock<wdDynamicMeshBufferResource> pMeshBuffer(hDynamicMeshBuffer, wdResourceAcquireMode::AllowLoadingFallback);
  BindMeshBuffer(pMeshBuffer->GetVertexBuffer(), pMeshBuffer->GetIndexBuffer(), &(pMeshBuffer->GetVertexDeclaration()), pMeshBuffer->GetDescriptor().m_Topology, pMeshBuffer->GetDescriptor().m_uiMaxPrimitives, pMeshBuffer->GetColorBuffer());
}

wdResult wdRenderContext::DrawMeshBuffer(wdUInt32 uiPrimitiveCount, wdUInt32 uiFirstPrimitive, wdUInt32 uiInstanceCount)
{
  if (ApplyContextStates().Failed() || uiPrimitiveCount == 0 || uiInstanceCount == 0)
  {
    m_Statistics.m_uiFailedDrawcalls++;
    return WD_FAILURE;
  }

  WD_ASSERT_DEV(uiFirstPrimitive < m_uiMeshBufferPrimitiveCount, "Invalid primitive range: first primitive ({0}) can't be larger than number of primitives ({1})", uiFirstPrimitive, uiPrimitiveCount);

  uiPrimitiveCount = wdMath::Min(uiPrimitiveCount, m_uiMeshBufferPrimitiveCount - uiFirstPrimitive);
  WD_ASSERT_DEV(uiPrimitiveCount > 0, "Invalid primitive range: number of primitives can't be zero.");

  auto pCommandEncoder = GetRenderCommandEncoder();

  const wdUInt32 uiVertsPerPrimitive = wdGALPrimitiveTopology::VerticesPerPrimitive(pCommandEncoder->GetPrimitiveTopology());

  uiPrimitiveCount *= uiVertsPerPrimitive;
  uiFirstPrimitive *= uiVertsPerPrimitive;
  if (m_bStereoRendering)
  {
    uiInstanceCount *= 2;
  }

  if (uiInstanceCount > 1)
  {
    if (!m_hIndexBuffer.IsInvalidated())
    {
      pCommandEncoder->DrawIndexedInstanced(uiPrimitiveCount, uiInstanceCount, uiFirstPrimitive);
    }
    else
    {
      pCommandEncoder->DrawInstanced(uiPrimitiveCount, uiInstanceCount, uiFirstPrimitive);
    }
  }
  else
  {
    if (!m_hIndexBuffer.IsInvalidated())
    {
      pCommandEncoder->DrawIndexed(uiPrimitiveCount, uiFirstPrimitive);
    }
    else
    {
      pCommandEncoder->Draw(uiPrimitiveCount, uiFirstPrimitive);
    }
  }

  return WD_SUCCESS;
}

wdResult wdRenderContext::Dispatch(wdUInt32 uiThreadGroupCountX, wdUInt32 uiThreadGroupCountY, wdUInt32 uiThreadGroupCountZ)
{
  if (ApplyContextStates().Failed())
  {
    m_Statistics.m_uiFailedDrawcalls++;
    return WD_FAILURE;
  }

  GetComputeCommandEncoder()->Dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);

  return WD_SUCCESS;
}

wdResult wdRenderContext::ApplyContextStates(bool bForce)
{
  // First apply material state since this can modify all other states.
  // Note ApplyMaterialState only returns a valid material pointer if the constant buffer of this material needs to be updated.
  // This needs to be done once we have determined the correct shader permutation.
  wdMaterialResource* pMaterial = nullptr;
  WD_SCOPE_EXIT(if (pMaterial != nullptr) { wdResourceManager::EndAcquireResource(pMaterial); });

  if (bForce || m_StateFlags.IsSet(wdRenderContextFlags::MaterialBindingChanged))
  {
    pMaterial = ApplyMaterialState();

    m_StateFlags.Remove(wdRenderContextFlags::MaterialBindingChanged);
  }

  wdShaderPermutationResource* pShaderPermutation = nullptr;
  WD_SCOPE_EXIT(if (pShaderPermutation != nullptr) { wdResourceManager::EndAcquireResource(pShaderPermutation); });

  bool bRebuildVertexDeclaration = m_StateFlags.IsAnySet(wdRenderContextFlags::ShaderStateChanged | wdRenderContextFlags::MeshBufferBindingChanged);

  if (bForce || m_StateFlags.IsSet(wdRenderContextFlags::ShaderStateChanged))
  {
    pShaderPermutation = ApplyShaderState();

    if (pShaderPermutation == nullptr)
    {
      return WD_FAILURE;
    }

    m_StateFlags.Remove(wdRenderContextFlags::ShaderStateChanged);
  }

  if (m_hActiveShaderPermutation.IsValid())
  {
    if ((bForce || m_StateFlags.IsAnySet(wdRenderContextFlags::TextureBindingChanged | wdRenderContextFlags::UAVBindingChanged |
                                         wdRenderContextFlags::SamplerBindingChanged | wdRenderContextFlags::BufferBindingChanged |
                                         wdRenderContextFlags::ConstantBufferBindingChanged)))
    {
      if (pShaderPermutation == nullptr)
        pShaderPermutation = wdResourceManager::BeginAcquireResource(m_hActiveShaderPermutation, wdResourceAcquireMode::BlockTillLoaded);
    }

    wdLogBlock applyBindingsBlock("Applying Shader Bindings", pShaderPermutation != nullptr ? pShaderPermutation->GetResourceDescription().GetData() : "");

    if (bForce || m_StateFlags.IsSet(wdRenderContextFlags::UAVBindingChanged))
    {
      // RWTextures/UAV are usually only supported in compute and pixel shader.
      if (auto pBin = pShaderPermutation->GetShaderStageBinary(wdGALShaderStage::ComputeShader))
      {
        ApplyUAVBindings(pBin);
      }
      if (auto pBin = pShaderPermutation->GetShaderStageBinary(wdGALShaderStage::PixelShader))
      {
        ApplyUAVBindings(pBin);
      }

      m_StateFlags.Remove(wdRenderContextFlags::UAVBindingChanged);
    }

    if (bForce || m_StateFlags.IsSet(wdRenderContextFlags::TextureBindingChanged))
    {
      for (wdUInt32 stage = 0; stage < wdGALShaderStage::ENUM_COUNT; ++stage)
      {
        if (auto pBin = pShaderPermutation->GetShaderStageBinary((wdGALShaderStage::Enum)stage))
        {
          ApplyTextureBindings((wdGALShaderStage::Enum)stage, pBin);
        }
      }

      m_StateFlags.Remove(wdRenderContextFlags::TextureBindingChanged);
    }

    if (bForce || m_StateFlags.IsSet(wdRenderContextFlags::SamplerBindingChanged))
    {
      for (wdUInt32 stage = 0; stage < wdGALShaderStage::ENUM_COUNT; ++stage)
      {
        if (auto pBin = pShaderPermutation->GetShaderStageBinary((wdGALShaderStage::Enum)stage))
        {
          ApplySamplerBindings((wdGALShaderStage::Enum)stage, pBin);
        }
      }

      m_StateFlags.Remove(wdRenderContextFlags::SamplerBindingChanged);
    }

    if (bForce || m_StateFlags.IsSet(wdRenderContextFlags::BufferBindingChanged))
    {
      for (wdUInt32 stage = 0; stage < wdGALShaderStage::ENUM_COUNT; ++stage)
      {
        if (auto pBin = pShaderPermutation->GetShaderStageBinary((wdGALShaderStage::Enum)stage))
        {
          ApplyBufferBindings((wdGALShaderStage::Enum)stage, pBin);
        }
      }

      m_StateFlags.Remove(wdRenderContextFlags::BufferBindingChanged);
    }

    if (pMaterial != nullptr)
    {
      pMaterial->UpdateConstantBuffer(pShaderPermutation);
      BindConstantBuffer("wdMaterialConstants", pMaterial->m_hConstantBufferStorage);
    }

    UploadConstants();

    if (bForce || m_StateFlags.IsSet(wdRenderContextFlags::ConstantBufferBindingChanged))
    {
      for (wdUInt32 stage = 0; stage < wdGALShaderStage::ENUM_COUNT; ++stage)
      {
        if (auto pBin = pShaderPermutation->GetShaderStageBinary((wdGALShaderStage::Enum)stage))
        {
          ApplyConstantBufferBindings(pBin);
        }
      }

      m_StateFlags.Remove(wdRenderContextFlags::ConstantBufferBindingChanged);
    }
  }

  if ((bForce || bRebuildVertexDeclaration) && !m_bCompute)
  {
    if (m_hActiveGALShader.IsInvalidated())
      return WD_FAILURE;

    auto pCommandEncoder = GetRenderCommandEncoder();

    if (bForce || m_StateFlags.IsSet(wdRenderContextFlags::MeshBufferBindingChanged))
    {
      pCommandEncoder->SetPrimitiveTopology(m_Topology);

      for (wdUInt32 i = 0; i < WD_ARRAY_SIZE(m_hVertexBuffers); ++i)
      {
        pCommandEncoder->SetVertexBuffer(i, m_hVertexBuffers[i]);
      }

      if (!m_hIndexBuffer.IsInvalidated())
        pCommandEncoder->SetIndexBuffer(m_hIndexBuffer);
    }

    wdGALVertexDeclarationHandle hVertexDeclaration;
    if (m_pVertexDeclarationInfo != nullptr && BuildVertexDeclaration(m_hActiveGALShader, *m_pVertexDeclarationInfo, hVertexDeclaration).Failed())
      return WD_FAILURE;

    // If there is a vertex buffer we need a valid vertex declaration as well.
    if ((!m_hVertexBuffers[0].IsInvalidated() || !m_hVertexBuffers[1].IsInvalidated() || !m_hVertexBuffers[2].IsInvalidated() || !m_hVertexBuffers[3].IsInvalidated()) && hVertexDeclaration.IsInvalidated())
      return WD_FAILURE;

    pCommandEncoder->SetVertexDeclaration(hVertexDeclaration);

    m_StateFlags.Remove(wdRenderContextFlags::MeshBufferBindingChanged);
  }

  return WD_SUCCESS;
}

void wdRenderContext::ResetContextState()
{
  m_StateFlags = wdRenderContextFlags::AllStatesInvalid;

  m_hActiveShader.Invalidate();
  m_hActiveGALShader.Invalidate();

  m_PermutationVariables.Clear();
  m_hNewMaterial.Invalidate();
  m_hMaterial.Invalidate();

  m_hActiveShaderPermutation.Invalidate();

  for (wdUInt32 i = 0; i < WD_ARRAY_SIZE(m_hVertexBuffers); ++i)
  {
    m_hVertexBuffers[i].Invalidate();
  }

  m_hIndexBuffer.Invalidate();
  m_pVertexDeclarationInfo = nullptr;
  m_Topology = wdGALPrimitiveTopology::ENUM_COUNT; // Set to something invalid
  m_uiMeshBufferPrimitiveCount = 0;

  m_BoundTextures2D.Clear();
  m_BoundTextures3D.Clear();
  m_BoundTexturesCube.Clear();
  m_BoundBuffer.Clear();

  m_BoundSamplers.Clear();
  m_BoundSamplers.Insert(wdHashingUtils::StringHash("LinearSampler"), GetDefaultSamplerState(wdDefaultSamplerFlags::LinearFiltering));
  m_BoundSamplers.Insert(wdHashingUtils::StringHash("LinearClampSampler"), GetDefaultSamplerState(wdDefaultSamplerFlags::LinearFiltering | wdDefaultSamplerFlags::Clamp));
  m_BoundSamplers.Insert(wdHashingUtils::StringHash("PointSampler"), GetDefaultSamplerState(wdDefaultSamplerFlags::PointFiltering));
  m_BoundSamplers.Insert(wdHashingUtils::StringHash("PointClampSampler"), GetDefaultSamplerState(wdDefaultSamplerFlags::PointFiltering | wdDefaultSamplerFlags::Clamp));

  m_BoundUAVs.Clear();
  m_BoundConstantBuffers.Clear();
}

wdGlobalConstants& wdRenderContext::WriteGlobalConstants()
{
  wdConstantBufferStorage<wdGlobalConstants>* pStorage = nullptr;
  WD_VERIFY(TryGetConstantBufferStorage(m_hGlobalConstantBufferStorage, pStorage), "Invalid Global Constant Storage");
  return pStorage->GetDataForWriting();
}

const wdGlobalConstants& wdRenderContext::ReadGlobalConstants() const
{
  wdConstantBufferStorage<wdGlobalConstants>* pStorage = nullptr;
  WD_VERIFY(TryGetConstantBufferStorage(m_hGlobalConstantBufferStorage, pStorage), "Invalid Global Constant Storage");
  return pStorage->GetDataForReading();
}

// static
wdConstantBufferStorageHandle wdRenderContext::CreateConstantBufferStorage(wdUInt32 uiSizeInBytes, wdConstantBufferStorageBase*& out_pStorage)
{
  WD_ASSERT_DEV(wdMemoryUtils::IsSizeAligned(uiSizeInBytes, 16u), "Storage struct for constant buffer is not aligned to 16 bytes");

  WD_LOCK(s_ConstantBufferStorageMutex);

  wdConstantBufferStorageBase* pStorage = nullptr;

  auto it = s_FreeConstantBufferStorage.Find(uiSizeInBytes);
  if (it.IsValid())
  {
    wdDynamicArray<wdConstantBufferStorageBase*>& storageForSize = it.Value();
    if (!storageForSize.IsEmpty())
    {
      pStorage = storageForSize[0];
      storageForSize.RemoveAtAndSwap(0);
    }
  }

  if (pStorage == nullptr)
  {
    pStorage = WD_DEFAULT_NEW(wdConstantBufferStorageBase, uiSizeInBytes);
  }

  out_pStorage = pStorage;
  return wdConstantBufferStorageHandle(s_ConstantBufferStorageTable.Insert(pStorage));
}

// static
void wdRenderContext::DeleteConstantBufferStorage(wdConstantBufferStorageHandle hStorage)
{
  WD_LOCK(s_ConstantBufferStorageMutex);

  wdConstantBufferStorageBase* pStorage = nullptr;
  if (!s_ConstantBufferStorageTable.Remove(hStorage.m_InternalId, &pStorage))
  {
    // already deleted
    return;
  }

  wdUInt32 uiSizeInBytes = pStorage->m_Data.GetCount();

  auto it = s_FreeConstantBufferStorage.Find(uiSizeInBytes);
  if (!it.IsValid())
  {
    it = s_FreeConstantBufferStorage.Insert(uiSizeInBytes, wdDynamicArray<wdConstantBufferStorageBase*>());
  }

  it.Value().PushBack(pStorage);
}

// static
bool wdRenderContext::TryGetConstantBufferStorage(wdConstantBufferStorageHandle hStorage, wdConstantBufferStorageBase*& out_pStorage)
{
  WD_LOCK(s_ConstantBufferStorageMutex);
  return s_ConstantBufferStorageTable.TryGetValue(hStorage.m_InternalId, out_pStorage);
}

// static
wdGALSamplerStateHandle wdRenderContext::GetDefaultSamplerState(wdBitflags<wdDefaultSamplerFlags> flags)
{
  wdUInt32 uiSamplerStateIndex = flags.GetValue();
  WD_ASSERT_DEV(uiSamplerStateIndex < WD_ARRAY_SIZE(s_hDefaultSamplerStates), "");

  if (s_hDefaultSamplerStates[uiSamplerStateIndex].IsInvalidated())
  {
    wdGALSamplerStateCreationDescription desc;
    desc.m_MinFilter = flags.IsSet(wdDefaultSamplerFlags::LinearFiltering) ? wdGALTextureFilterMode::Linear : wdGALTextureFilterMode::Point;
    desc.m_MagFilter = flags.IsSet(wdDefaultSamplerFlags::LinearFiltering) ? wdGALTextureFilterMode::Linear : wdGALTextureFilterMode::Point;
    desc.m_MipFilter = flags.IsSet(wdDefaultSamplerFlags::LinearFiltering) ? wdGALTextureFilterMode::Linear : wdGALTextureFilterMode::Point;

    desc.m_AddressU = flags.IsSet(wdDefaultSamplerFlags::Clamp) ? wdImageAddressMode::Clamp : wdImageAddressMode::Repeat;
    desc.m_AddressV = flags.IsSet(wdDefaultSamplerFlags::Clamp) ? wdImageAddressMode::Clamp : wdImageAddressMode::Repeat;
    desc.m_AddressW = flags.IsSet(wdDefaultSamplerFlags::Clamp) ? wdImageAddressMode::Clamp : wdImageAddressMode::Repeat;

    s_hDefaultSamplerStates[uiSamplerStateIndex] = wdGALDevice::GetDefaultDevice()->CreateSamplerState(desc);
  }

  return s_hDefaultSamplerStates[uiSamplerStateIndex];
}

// private functions
//////////////////////////////////////////////////////////////////////////

// static
void wdRenderContext::LoadBuiltinShader(wdShaderUtils::wdBuiltinShaderType type, wdShaderUtils::wdBuiltinShader& out_shader)
{
  wdShaderResourceHandle hActiveShader;
  bool bStereo = false;
  switch (type)
  {
    case wdShaderUtils::wdBuiltinShaderType::CopyImageArray:
      bStereo = true;
      [[fallthrough]];
    case wdShaderUtils::wdBuiltinShaderType::CopyImage:
      hActiveShader = wdResourceManager::LoadResource<wdShaderResource>("Shaders/Pipeline/Copy.wdShader");
      break;
    case wdShaderUtils::wdBuiltinShaderType::DownscaleImageArray:
      bStereo = true;
      [[fallthrough]];
    case wdShaderUtils::wdBuiltinShaderType::DownscaleImage:
      hActiveShader = wdResourceManager::LoadResource<wdShaderResource>("Shaders/Pipeline/Downscale.wdShader");
      break;
  }

  WD_ASSERT_DEV(hActiveShader.IsValid(), "Could not load builtin shader!");

  wdHashTable<wdHashedString, wdHashedString> permutationVariables;
  static wdHashedString sVSRTAI = wdMakeHashedString("VERTEX_SHADER_RENDER_TARGET_ARRAY_INDEX");
  static wdHashedString sTrue = wdMakeHashedString("TRUE");
  static wdHashedString sFalse = wdMakeHashedString("FALSE");
  static wdHashedString sCameraMode = wdMakeHashedString("CAMERA_MODE");
  static wdHashedString sPerspective = wdMakeHashedString("CAMERA_MODE_PERSPECTIVE");
  static wdHashedString sStereo = wdMakeHashedString("CAMERA_MODE_STEREO");

  permutationVariables.Insert(sCameraMode, bStereo ? sStereo : sPerspective);
  if (wdGALDevice::GetDefaultDevice()->GetCapabilities().m_bVertexShaderRenderTargetArrayIndex)
    permutationVariables.Insert(sVSRTAI, sTrue);
  else
    permutationVariables.Insert(sVSRTAI, sFalse);


  wdShaderPermutationResourceHandle hActiveShaderPermutation = wdShaderManager::PreloadSinglePermutation(hActiveShader, permutationVariables, false);

  WD_ASSERT_DEV(hActiveShaderPermutation.IsValid(), "Could not load builtin shader permutation!");

  wdResourceLock<wdShaderPermutationResource> pShaderPermutation(hActiveShaderPermutation, wdResourceAcquireMode::BlockTillLoaded);

  WD_ASSERT_DEV(pShaderPermutation->IsShaderValid(), "Builtin shader permutation shader is invalid!");

  out_shader.m_hActiveGALShader = pShaderPermutation->GetGALShader();
  WD_ASSERT_DEV(!out_shader.m_hActiveGALShader.IsInvalidated(), "Invalid GAL Shader handle.");

  out_shader.m_hBlendState = pShaderPermutation->GetBlendState();
  out_shader.m_hDepthStencilState = pShaderPermutation->GetDepthStencilState();
  out_shader.m_hRasterizerState = pShaderPermutation->GetRasterizerState();
}

// static
void wdRenderContext::OnEngineShutdown()
{
  wdShaderStageBinary::OnEngineShutdown();

  for (auto rc : s_Instances)
    WD_DEFAULT_DELETE(rc);

  s_Instances.Clear();

  // Cleanup sampler states
  for (wdUInt32 i = 0; i < WD_ARRAY_SIZE(s_hDefaultSamplerStates); ++i)
  {
    if (!s_hDefaultSamplerStates[i].IsInvalidated())
    {
      wdGALDevice::GetDefaultDevice()->DestroySamplerState(s_hDefaultSamplerStates[i]);
      s_hDefaultSamplerStates[i].Invalidate();
    }
  }

  // Cleanup vertex declarations
  {
    for (auto it = s_GALVertexDeclarations.GetIterator(); it.IsValid(); ++it)
    {
      wdGALDevice::GetDefaultDevice()->DestroyVertexDeclaration(it.Value());
    }

    s_GALVertexDeclarations.Clear();
  }

  // Cleanup constant buffer storage
  {
    for (auto it = s_ConstantBufferStorageTable.GetIterator(); it.IsValid(); ++it)
    {
      wdConstantBufferStorageBase* pStorage = it.Value();
      WD_DEFAULT_DELETE(pStorage);
    }

    s_ConstantBufferStorageTable.Clear();

    for (auto it = s_FreeConstantBufferStorage.GetIterator(); it.IsValid(); ++it)
    {
      wdDynamicArray<wdConstantBufferStorageBase*>& storageForSize = it.Value();
      for (auto& pStorage : storageForSize)
      {
        WD_DEFAULT_DELETE(pStorage);
      }
    }

    s_FreeConstantBufferStorage.Clear();
  }
}

// static
wdResult wdRenderContext::BuildVertexDeclaration(wdGALShaderHandle hShader, const wdVertexDeclarationInfo& decl, wdGALVertexDeclarationHandle& out_Declaration)
{
  ShaderVertexDecl svd;
  svd.m_hShader = hShader;
  svd.m_uiVertexDeclarationHash = decl.m_uiHash;

  bool bExisted = false;
  auto it = s_GALVertexDeclarations.FindOrAdd(svd, &bExisted);

  if (!bExisted)
  {
    const wdGALShader* pShader = wdGALDevice::GetDefaultDevice()->GetShader(hShader);

    auto pBytecode = pShader->GetDescription().m_ByteCodes[wdGALShaderStage::VertexShader];

    wdGALVertexDeclarationCreationDescription vd;
    vd.m_hShader = hShader;

    for (wdUInt32 slot = 0; slot < decl.m_VertexStreams.GetCount(); ++slot)
    {
      auto& stream = decl.m_VertexStreams[slot];

      // stream.m_Format
      wdGALVertexAttribute gal;
      gal.m_bInstanceData = false;
      gal.m_eFormat = stream.m_Format;
      gal.m_eSemantic = stream.m_Semantic;
      gal.m_uiOffset = stream.m_uiOffset;
      gal.m_uiVertexBufferSlot = stream.m_uiVertexBufferSlot;
      vd.m_VertexAttributes.PushBack(gal);
    }

    out_Declaration = wdGALDevice::GetDefaultDevice()->CreateVertexDeclaration(vd);

    if (out_Declaration.IsInvalidated())
    {
      /* This can happen when the resource system gives you a fallback resource, which then selects a shader that
      does not fit the mesh layout.
      E.g. when a material is not yet loaded and the fallback material is used, that fallback material may
      use another shader, that requires more data streams, than what the mesh provides.
      This problem will go away, once the proper material is loaded.

      This can be fixed by ensuring that the fallback material uses a shader that only requires data that is
      always there, e.g. only position and maybe a texcoord, and of course all meshes must provide at least those
      data streams.

      Otherwise, this is harmless, the renderer will ignore invalid drawcalls and once all the correct stuff is
      available, it will work.
      */

      wdLog::Warning("Failed to create vertex declaration");
      return WD_FAILURE;
    }

    it.Value() = out_Declaration;
  }

  out_Declaration = it.Value();
  return WD_SUCCESS;
}

void wdRenderContext::UploadConstants()
{
  BindConstantBuffer("wdGlobalConstants", m_hGlobalConstantBufferStorage);

  for (auto it = m_BoundConstantBuffers.GetIterator(); it.IsValid(); ++it)
  {
    wdConstantBufferStorageHandle hConstantBufferStorage = it.Value().m_hConstantBufferStorage;
    wdConstantBufferStorageBase* pConstantBufferStorage = nullptr;
    if (TryGetConstantBufferStorage(hConstantBufferStorage, pConstantBufferStorage))
    {
      pConstantBufferStorage->UploadData(m_pGALCommandEncoder);
    }
  }
}

void wdRenderContext::SetShaderPermutationVariableInternal(const wdHashedString& sName, const wdHashedString& sValue)
{
  wdHashedString* pOldValue = nullptr;
  m_PermutationVariables.TryGetValue(sName, pOldValue);

  if (pOldValue == nullptr || *pOldValue != sValue)
  {
    m_PermutationVariables.Insert(sName, sValue);
    m_StateFlags.Add(wdRenderContextFlags::ShaderStateChanged);
  }
}

void wdRenderContext::BindShaderInternal(const wdShaderResourceHandle& hShader, wdBitflags<wdShaderBindFlags> flags)
{
  if (flags.IsAnySet(wdShaderBindFlags::ForceRebind) || m_hActiveShader != hShader)
  {
    m_ShaderBindFlags = flags;
    m_hActiveShader = hShader;

    m_StateFlags.Add(wdRenderContextFlags::ShaderStateChanged);
  }
}

wdShaderPermutationResource* wdRenderContext::ApplyShaderState()
{
  m_hActiveGALShader.Invalidate();

  m_StateFlags.Add(wdRenderContextFlags::TextureBindingChanged | wdRenderContextFlags::SamplerBindingChanged |
                   wdRenderContextFlags::BufferBindingChanged | wdRenderContextFlags::ConstantBufferBindingChanged);

  if (!m_hActiveShader.IsValid())
    return nullptr;

  m_hActiveShaderPermutation = wdShaderManager::PreloadSinglePermutation(m_hActiveShader, m_PermutationVariables, m_bAllowAsyncShaderLoading);

  if (!m_hActiveShaderPermutation.IsValid())
    return nullptr;

  wdShaderPermutationResource* pShaderPermutation = wdResourceManager::BeginAcquireResource(
    m_hActiveShaderPermutation, m_bAllowAsyncShaderLoading ? wdResourceAcquireMode::AllowLoadingFallback : wdResourceAcquireMode::BlockTillLoaded);

  if (!pShaderPermutation->IsShaderValid())
  {
    wdResourceManager::EndAcquireResource(pShaderPermutation);
    return nullptr;
  }

  m_hActiveGALShader = pShaderPermutation->GetGALShader();
  WD_ASSERT_DEV(!m_hActiveGALShader.IsInvalidated(), "Invalid GAL Shader handle.");

  m_pGALCommandEncoder->SetShader(m_hActiveGALShader);

  // Set render state from shader
  if (!m_bCompute)
  {
    auto pCommandEncoder = GetRenderCommandEncoder();

    if (!m_ShaderBindFlags.IsSet(wdShaderBindFlags::NoBlendState))
      pCommandEncoder->SetBlendState(pShaderPermutation->GetBlendState());

    if (!m_ShaderBindFlags.IsSet(wdShaderBindFlags::NoRasterizerState))
      pCommandEncoder->SetRasterizerState(pShaderPermutation->GetRasterizerState());

    if (!m_ShaderBindFlags.IsSet(wdShaderBindFlags::NoDepthStencilState))
      pCommandEncoder->SetDepthStencilState(pShaderPermutation->GetDepthStencilState());
  }

  return pShaderPermutation;
}

wdMaterialResource* wdRenderContext::ApplyMaterialState()
{
  if (!m_hNewMaterial.IsValid())
  {
    BindShaderInternal(wdShaderResourceHandle(), wdShaderBindFlags::Default);
    return nullptr;
  }

  // check whether material has been modified
  wdMaterialResource* pMaterial = wdResourceManager::BeginAcquireResource(m_hNewMaterial, wdResourceAcquireMode::AllowLoadingFallback);

  if (m_hNewMaterial != m_hMaterial || pMaterial->IsModified())
  {
    auto pCachedValues = pMaterial->GetOrUpdateCachedValues();

    BindShaderInternal(pCachedValues->m_hShader, wdShaderBindFlags::Default);

    if (!pMaterial->m_hConstantBufferStorage.IsInvalidated())
    {
      BindConstantBuffer("wdMaterialConstants", pMaterial->m_hConstantBufferStorage);
    }

    for (auto it = pCachedValues->m_PermutationVars.GetIterator(); it.IsValid(); ++it)
    {
      SetShaderPermutationVariableInternal(it.Key(), it.Value());
    }

    for (auto it = pCachedValues->m_Texture2DBindings.GetIterator(); it.IsValid(); ++it)
    {
      BindTexture2D(it.Key(), it.Value());
    }

    for (auto it = pCachedValues->m_TextureCubeBindings.GetIterator(); it.IsValid(); ++it)
    {
      BindTextureCube(it.Key(), it.Value());
    }

    m_hMaterial = m_hNewMaterial;
  }

  // The material needs its constant buffer updated.
  // Thus we keep it acquired until we have the correct shader permutation for the constant buffer layout.
  if (pMaterial->AreConstantsModified())
  {
    m_StateFlags.Add(wdRenderContextFlags::ConstantBufferBindingChanged);

    return pMaterial;
  }

  wdResourceManager::EndAcquireResource(pMaterial);
  return nullptr;
}

void wdRenderContext::ApplyConstantBufferBindings(const wdShaderStageBinary* pBinary)
{
  for (const auto& binding : pBinary->m_ShaderResourceBindings)
  {
    if (binding.m_Type != wdShaderResourceType::ConstantBuffer)
      continue;

    const wdUInt64 uiResourceHash = binding.m_sName.GetHash();

    BoundConstantBuffer boundConstantBuffer;
    if (!m_BoundConstantBuffers.TryGetValue(uiResourceHash, boundConstantBuffer))
    {
      // If the shader was compiled with debug info the shader compiler will not strip unused resources and
      // thus this error would trigger although the shader doesn't actually uses the resource.
      if (!pBinary->m_bWasCompiledWithDebug)
      {
        wdLog::Error("No resource is bound for constant buffer slot '{0}'", binding.m_sName);
      }
      m_pGALCommandEncoder->SetConstantBuffer(binding.m_iSlot, wdGALBufferHandle());
      continue;
    }

    if (!boundConstantBuffer.m_hConstantBuffer.IsInvalidated())
    {
      m_pGALCommandEncoder->SetConstantBuffer(binding.m_iSlot, boundConstantBuffer.m_hConstantBuffer);
    }
    else
    {
      wdConstantBufferStorageBase* pConstantBufferStorage = nullptr;
      if (TryGetConstantBufferStorage(boundConstantBuffer.m_hConstantBufferStorage, pConstantBufferStorage))
      {
        m_pGALCommandEncoder->SetConstantBuffer(binding.m_iSlot, pConstantBufferStorage->GetGALBufferHandle());
      }
      else
      {
        wdLog::Error("Invalid constant buffer storage is bound for slot '{0}'", binding.m_sName);
        m_pGALCommandEncoder->SetConstantBuffer(binding.m_iSlot, wdGALBufferHandle());
      }
    }
  }
}

void wdRenderContext::ApplyTextureBindings(wdGALShaderStage::Enum stage, const wdShaderStageBinary* pBinary)
{
  for (const auto& binding : pBinary->m_ShaderResourceBindings)
  {
    // we currently only support 2D and cube textures

    const wdUInt64 uiResourceHash = binding.m_sName.GetHash();
    wdGALResourceViewHandle hResourceView;

    if (binding.m_Type >= wdShaderResourceType::Texture2D && binding.m_Type <= wdShaderResourceType::Texture2DMSArray)
    {
      m_BoundTextures2D.TryGetValue(uiResourceHash, hResourceView);
      m_pGALCommandEncoder->SetResourceView(stage, binding.m_iSlot, hResourceView);
    }

    if (binding.m_Type == wdShaderResourceType::Texture3D)
    {
      m_BoundTextures3D.TryGetValue(uiResourceHash, hResourceView);
      m_pGALCommandEncoder->SetResourceView(stage, binding.m_iSlot, hResourceView);
    }

    if (binding.m_Type >= wdShaderResourceType::TextureCube && binding.m_Type <= wdShaderResourceType::TextureCubeArray)
    {
      m_BoundTexturesCube.TryGetValue(uiResourceHash, hResourceView);
      m_pGALCommandEncoder->SetResourceView(stage, binding.m_iSlot, hResourceView);
    }
  }
}

void wdRenderContext::ApplyUAVBindings(const wdShaderStageBinary* pBinary)
{
  for (const auto& binding : pBinary->m_ShaderResourceBindings)
  {
    if (binding.m_Type != wdShaderResourceType::UAV)
      continue;

    const wdUInt64 uiResourceHash = binding.m_sName.GetHash();

    wdGALUnorderedAccessViewHandle hResourceView;
    m_BoundUAVs.TryGetValue(uiResourceHash, hResourceView);

    m_pGALCommandEncoder->SetUnorderedAccessView(binding.m_iSlot, hResourceView);
  }
}

void wdRenderContext::ApplySamplerBindings(wdGALShaderStage::Enum stage, const wdShaderStageBinary* pBinary)
{
  for (const auto& binding : pBinary->m_ShaderResourceBindings)
  {
    if (binding.m_Type != wdShaderResourceType::Sampler)
      continue;

    const wdUInt64 uiResourceHash = binding.m_sName.GetHash();

    wdGALSamplerStateHandle hSamplerState;
    if (!m_BoundSamplers.TryGetValue(uiResourceHash, hSamplerState))
    {
      hSamplerState = GetDefaultSamplerState(wdDefaultSamplerFlags::LinearFiltering); // Bind a default state to avoid DX11 errors.
    }

    m_pGALCommandEncoder->SetSamplerState(stage, binding.m_iSlot, hSamplerState);
  }
}

void wdRenderContext::ApplyBufferBindings(wdGALShaderStage::Enum stage, const wdShaderStageBinary* pBinary)
{
  for (const auto& binding : pBinary->m_ShaderResourceBindings)
  {
    if (binding.m_Type != wdShaderResourceType::GenericBuffer)
      continue;

    const wdUInt64 uiResourceHash = binding.m_sName.GetHash();

    wdGALResourceViewHandle hResourceView;
    m_BoundBuffer.TryGetValue(uiResourceHash, hResourceView);

    m_pGALCommandEncoder->SetResourceView(stage, binding.m_iSlot, hResourceView);
  }
}

void wdRenderContext::SetDefaultTextureFilter(wdTextureFilterSetting::Enum filter)
{
  WD_ASSERT_DEBUG(
    filter >= wdTextureFilterSetting::FixedBilinear && filter <= wdTextureFilterSetting::FixedAnisotropic16x, "Invalid default texture filter");
  filter = wdMath::Clamp(filter, wdTextureFilterSetting::FixedBilinear, wdTextureFilterSetting::FixedAnisotropic16x);

  if (m_DefaultTextureFilter == filter)
    return;

  m_DefaultTextureFilter = filter;
}

wdTextureFilterSetting::Enum wdRenderContext::GetSpecificTextureFilter(wdTextureFilterSetting::Enum configuration) const
{
  if (configuration >= wdTextureFilterSetting::FixedNearest && configuration <= wdTextureFilterSetting::FixedAnisotropic16x)
    return configuration;

  int iFilter = m_DefaultTextureFilter;

  switch (configuration)
  {
    case wdTextureFilterSetting::LowestQuality:
      iFilter -= 2;
      break;
    case wdTextureFilterSetting::LowQuality:
      iFilter -= 1;
      break;
    case wdTextureFilterSetting::HighQuality:
      iFilter += 1;
      break;
    case wdTextureFilterSetting::HighestQuality:
      iFilter += 2;
      break;
    default:
      break;
  }

  iFilter = wdMath::Clamp<int>(iFilter, wdTextureFilterSetting::FixedBilinear, wdTextureFilterSetting::FixedAnisotropic16x);

  return (wdTextureFilterSetting::Enum)iFilter;
}

void wdRenderContext::SetAllowAsyncShaderLoading(bool bAllow)
{
  m_bAllowAsyncShaderLoading = bAllow;
}

bool wdRenderContext::GetAllowAsyncShaderLoading()
{
  return m_bAllowAsyncShaderLoading;
}

WD_STATICLINK_FILE(RendererCore, RendererCore_RenderContext_Implementation_RenderContext);
