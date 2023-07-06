#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/LSAOPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>

// clang-format off
WD_BEGIN_STATIC_REFLECTED_ENUM(wdLSAODepthCompareFunction, 1)
  WD_ENUM_CONSTANT(wdLSAODepthCompareFunction::Depth),
  WD_ENUM_CONSTANT(wdLSAODepthCompareFunction::Normal),
  WD_ENUM_CONSTANT(wdLSAODepthCompareFunction::NormalAndSampleDistance),
WD_END_STATIC_REFLECTED_ENUM;

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdLSAOPass, 1, wdRTTIDefaultAllocator<wdLSAOPass>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("Depth", m_PinDepthInput),
    WD_MEMBER_PROPERTY("AmbientObscurance", m_PinOutput),
    WD_ACCESSOR_PROPERTY("LineToLineDistance", GetLineToLinePixelOffset, SetLineToLinePixelOffset)->AddAttributes(new wdDefaultValueAttribute(2), new wdClampValueAttribute(1, 20)),
    WD_ACCESSOR_PROPERTY("LineSampleDistanceFactor", GetLineSamplePixelOffset, SetLineSamplePixelOffset)->AddAttributes(new wdDefaultValueAttribute(1), new wdClampValueAttribute(1, 10)),
    WD_ACCESSOR_PROPERTY("OcclusionFalloff", GetOcclusionFalloff, SetOcclusionFalloff)->AddAttributes(new wdDefaultValueAttribute(0.2f), new wdClampValueAttribute(0.01f, 2.0f)),
    WD_ENUM_MEMBER_PROPERTY("DepthCompareFunction", wdLSAODepthCompareFunction, m_DepthCompareFunction),
    WD_ACCESSOR_PROPERTY("DepthCutoffDistance", GetDepthCutoffDistance, SetDepthCutoffDistance)->AddAttributes(new wdDefaultValueAttribute(4.0f), new wdClampValueAttribute(0.1f, 100.0f)),
    WD_MEMBER_PROPERTY("DistributedGathering", m_bDistributedGathering)->AddAttributes(new wdDefaultValueAttribute(true)),
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  float HaltonSequence(int iBase, int j)
  {
    static int primes[61] = {
      2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283};

    WD_ASSERT_DEV(iBase < 61, "Don't have prime number for this base.");

    // Halton sequence with reverse permutation
    const int p = primes[iBase];
    float h = 0.0f;
    float f = 1.0f / static_cast<float>(p);
    float fct = f;
    while (j > 0)
    {
      int i = j % p;
      h += (i == 0 ? i : p - i) * fct;
      j /= p;
      fct *= f;
    }
    return h;
  }
} // namespace

wdLSAOPass::wdLSAOPass()
  : wdRenderPipelinePass("LSAOPass", true)

{
  {
    // Load shader.
    m_hShaderLineSweep = wdResourceManager::LoadResource<wdShaderResource>("Shaders/Pipeline/LSAOSweep.wdShader");
    WD_ASSERT_DEV(m_hShaderLineSweep.IsValid(), "Could not lsao sweep shader!");
    m_hShaderGather = wdResourceManager::LoadResource<wdShaderResource>("Shaders/Pipeline/LSAOGather.wdShader");
    WD_ASSERT_DEV(m_hShaderGather.IsValid(), "Could not lsao gather shader!");
    m_hShaderAverage = wdResourceManager::LoadResource<wdShaderResource>("Shaders/Pipeline/LSAOAverage.wdShader");
    WD_ASSERT_DEV(m_hShaderGather.IsValid(), "Could not lsao average shader!");
  }

  {
    m_hLineSweepCB = wdRenderContext::CreateConstantBufferStorage<wdLSAOConstants>();
    wdLSAOConstants* cb = wdRenderContext::GetConstantBufferData<wdLSAOConstants>(m_hLineSweepCB);
    cb->DepthCutoffDistance = 8.0f;
    cb->OcclusionFalloff = 0.25f;
  }
}

wdLSAOPass::~wdLSAOPass()
{
  DestroyLineSweepData();

  wdRenderContext::DeleteConstantBufferStorage(m_hLineSweepCB);
  m_hLineSweepCB.Invalidate();
}

bool wdLSAOPass::GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs)
{
  WD_ASSERT_DEBUG(inputs.GetCount() == 1, "Unexpected number of inputs for wdScreenSpaceAmbientOcclusionPass.");

  // Depth
  if (!inputs[m_PinDepthInput.m_uiInputIndex])
  {
    wdLog::Error("No depth input connected to ssao pass!");
    return false;
  }
  if (!inputs[m_PinDepthInput.m_uiInputIndex]->m_bAllowShaderResourceView)
  {
    wdLog::Error("All ssao pass inputs must allow shader resource view.");
    return false;
  }
  if (inputs[m_PinDepthInput.m_uiInputIndex]->m_SampleCount != wdGALMSAASampleCount::None)
  {
    wdLog::Error("'{0}' input must be resolved", GetName());
    return false;
  }

  // Output format matches input format but is f16.
  outputs[m_PinOutput.m_uiOutputIndex] = *inputs[m_PinDepthInput.m_uiInputIndex];
  outputs[m_PinOutput.m_uiOutputIndex].m_Format = wdGALResourceFormat::RGHalf;

  return true;
}

void wdLSAOPass::InitRenderPipelinePass(const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs)
{
  // Todo: Support half resolution.
  const wdGALTextureCreationDescription& desc = inputs[m_PinDepthInput.m_uiInputIndex]->m_Desc;
  SetupLineSweepData(wdVec3I32(desc.m_uiWidth, desc.m_uiHeight, desc.m_uiArraySize));
}

void wdLSAOPass::Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs)
{
  if (m_bSweepDataDirty)
  {
    const wdGALTextureCreationDescription& desc = inputs[m_PinDepthInput.m_uiInputIndex]->m_Desc;
    SetupLineSweepData(wdVec3I32(desc.m_uiWidth, desc.m_uiHeight, desc.m_uiArraySize));
  }
  if (outputs[m_PinOutput.m_uiOutputIndex] == nullptr)
    return;

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();
  wdGALPass* pGALPass = pDevice->BeginPass(GetName());
  WD_SCOPE_EXIT(pDevice->EndPass(pGALPass));

  wdGALRenderingSetup renderingSetup;
  wdGALTextureHandle tempTexture;
  if (m_bDistributedGathering)
  {
    wdGALTextureCreationDescription tempTextureDesc = outputs[m_PinOutput.m_uiOutputIndex]->m_Desc;
    tempTextureDesc.m_bAllowShaderResourceView = true;
    tempTextureDesc.m_bCreateRenderTarget = true;
    tempTexture = wdGPUResourcePool::GetDefaultInstance()->GetRenderTarget(tempTextureDesc);
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(tempTexture));
  }
  else
  {
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(outputs[m_PinOutput.m_uiOutputIndex]->m_TextureHandle));
  }

  // Line Sweep part (compute)
  {
    WD_PROFILE_SCOPE("Line Sweep");
    auto pCommandEncoder = renderViewContext.m_pRenderContext->BeginComputeScope(pGALPass, renderViewContext, "Line Sweep");
    renderViewContext.m_pRenderContext->BindConstantBuffer("wdLSAOConstants", m_hLineSweepCB);
    renderViewContext.m_pRenderContext->BindTexture2D("DepthBuffer", pDevice->GetDefaultResourceView(inputs[m_PinDepthInput.m_uiInputIndex]->m_TextureHandle));
    renderViewContext.m_pRenderContext->BindShader(m_hShaderLineSweep);
    renderViewContext.m_pRenderContext->BindBuffer("LineInstructions", m_hLineSweepInfoSRV);
    renderViewContext.m_pRenderContext->BindUAV("LineSweepOutputBuffer", m_hLineSweepOutputUAV);

    const wdUInt32 dispatchSize = m_uiNumSweepLines / SSAO_LINESWEEP_THREAD_GROUP + (m_uiNumSweepLines % SSAO_LINESWEEP_THREAD_GROUP != 0 ? 1 : 0);
    const wdUInt32 uiRenderedInstances = renderViewContext.m_pCamera->IsStereoscopic() ? 2 : 1;
    renderViewContext.m_pRenderContext->Dispatch(dispatchSize, uiRenderedInstances).IgnoreResult();
  }

  // Gather samples.
  {
    WD_PROFILE_SCOPE("Gather");
    auto pCommandEncoder = renderViewContext.m_pRenderContext->BeginRenderingScope(pGALPass, renderViewContext, renderingSetup, "Gather Samples", renderViewContext.m_pCamera->IsStereoscopic());

    if (m_bDistributedGathering)
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("DISTRIBUTED_SSAO_GATHERING", "TRUE");
    else
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("DISTRIBUTED_SSAO_GATHERING", "FALSE");

    switch (m_DepthCompareFunction)
    {
      case wdLSAODepthCompareFunction::Depth:
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LSAO_DEPTH_COMPARE", "LSAO_DEPTH_COMPARE_DEPTH");
        break;
      case wdLSAODepthCompareFunction::Normal:
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LSAO_DEPTH_COMPARE", "LSAO_DEPTH_COMPARE_NORMAL");
        break;
      case wdLSAODepthCompareFunction::NormalAndSampleDistance:
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LSAO_DEPTH_COMPARE", "LSAO_DEPTH_COMPARE_NORMAL_AND_SAMPLE_DISTANCE");
        break;
    }

    renderViewContext.m_pRenderContext->BindConstantBuffer("wdLSAOConstants", m_hLineSweepCB);
    renderViewContext.m_pRenderContext->BindTexture2D("DepthBuffer", pDevice->GetDefaultResourceView(inputs[m_PinDepthInput.m_uiInputIndex]->m_TextureHandle));
    renderViewContext.m_pRenderContext->BindShader(m_hShaderGather);
    renderViewContext.m_pRenderContext->BindBuffer("LineInstructions", m_hLineSweepInfoSRV);
    renderViewContext.m_pRenderContext->BindBuffer("LineSweepOutputBuffer", m_hLineSweepOutputSRV);
    renderViewContext.m_pRenderContext->BindMeshBuffer(wdGALBufferHandle(), wdGALBufferHandle(), nullptr, wdGALPrimitiveTopology::Triangles, 1);
    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }

  // If enabled, average distributed gather samples and write to output.
  if (m_bDistributedGathering)
  {
    WD_PROFILE_SCOPE("Averaging");

    switch (m_DepthCompareFunction)
    {
      case wdLSAODepthCompareFunction::Depth:
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LSAO_DEPTH_COMPARE", "LSAO_DEPTH_COMPARE_DEPTH");
        break;
      case wdLSAODepthCompareFunction::Normal:
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LSAO_DEPTH_COMPARE", "LSAO_DEPTH_COMPARE_NORMAL");
        break;
      case wdLSAODepthCompareFunction::NormalAndSampleDistance:
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("LSAO_DEPTH_COMPARE", "LSAO_DEPTH_COMPARE_NORMAL_AND_SAMPLE_DISTANCE");
        break;
    }

    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(outputs[m_PinOutput.m_uiOutputIndex]->m_TextureHandle));

    auto pCommandEncoder = renderViewContext.m_pRenderContext->BeginRenderingScope(pGALPass, renderViewContext, renderingSetup, "Averaging", renderViewContext.m_pCamera->IsStereoscopic());

    renderViewContext.m_pRenderContext->BindConstantBuffer("wdLSAOConstants", m_hLineSweepCB);
    renderViewContext.m_pRenderContext->BindTexture2D("DepthBuffer", pDevice->GetDefaultResourceView(inputs[m_PinDepthInput.m_uiInputIndex]->m_TextureHandle));
    renderViewContext.m_pRenderContext->BindShader(m_hShaderAverage);
    renderViewContext.m_pRenderContext->BindTexture2D("SSAOGatherOutput", pDevice->GetDefaultResourceView(tempTexture));

    renderViewContext.m_pRenderContext->BindMeshBuffer(wdGALBufferHandle(), wdGALBufferHandle(), nullptr, wdGALPrimitiveTopology::Triangles, 1);
    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

    // Give back temp texture.
    wdGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(tempTexture);
  }
}

void wdLSAOPass::ExecuteInactive(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs)
{
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr)
  {
    return;
  }

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  wdGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
  renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
  renderingSetup.m_ClearColor = wdColor::White;

  auto pCommandEncoder = wdRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, "Clear");
}

void wdLSAOPass::SetLineToLinePixelOffset(wdUInt32 uiPixelOffset)
{
  m_iLineToLinePixelOffset = uiPixelOffset;
  m_bSweepDataDirty = true;
}

void wdLSAOPass::SetLineSamplePixelOffset(wdUInt32 uiPixelOffset)
{
  m_iLineSamplePixelOffsetFactor = uiPixelOffset;
  m_bSweepDataDirty = true;
}

float wdLSAOPass::GetDepthCutoffDistance() const
{
  wdLSAOConstants* cb = wdRenderContext::GetConstantBufferData<wdLSAOConstants>(m_hLineSweepCB);
  return cb->DepthCutoffDistance;
}

void wdLSAOPass::SetDepthCutoffDistance(float fDepthCutoffDistance)
{
  wdLSAOConstants* cb = wdRenderContext::GetConstantBufferData<wdLSAOConstants>(m_hLineSweepCB);
  cb->DepthCutoffDistance = fDepthCutoffDistance;
}

float wdLSAOPass::GetOcclusionFalloff() const
{
  wdLSAOConstants* cb = wdRenderContext::GetConstantBufferData<wdLSAOConstants>(m_hLineSweepCB);
  return cb->OcclusionFalloff;
}

void wdLSAOPass::SetOcclusionFalloff(float fFalloff)
{
  wdLSAOConstants* cb = wdRenderContext::GetConstantBufferData<wdLSAOConstants>(m_hLineSweepCB);
  cb->OcclusionFalloff = fFalloff;
}

void wdLSAOPass::DestroyLineSweepData()
{
  wdGALDevice* device = wdGALDevice::GetDefaultDevice();

  if (!m_hLineSweepOutputUAV.IsInvalidated())
    device->DestroyUnorderedAccessView(m_hLineSweepOutputUAV);
  m_hLineSweepOutputUAV.Invalidate();

  if (!m_hLineSweepOutputSRV.IsInvalidated())
    device->DestroyResourceView(m_hLineSweepOutputSRV);
  m_hLineSweepOutputSRV.Invalidate();

  if (!m_hLineSweepOutputBuffer.IsInvalidated())
    device->DestroyBuffer(m_hLineSweepOutputBuffer);
  m_hLineSweepOutputBuffer.Invalidate();

  if (!m_hLineInfoBuffer.IsInvalidated())
    device->DestroyBuffer(m_hLineInfoBuffer);
  m_hLineInfoBuffer.Invalidate();
}

void wdLSAOPass::SetupLineSweepData(const wdVec3I32& imageResolution)
{
  // imageResolution.z defines the number of render layers (1 for mono, 2 for stereo rendering).
  DestroyLineSweepData();

  wdDynamicArray<LineInstruction> lineInstructions;
  wdUInt32 totalNumberOfSamples = 0;
  wdLSAOConstants* cb = wdRenderContext::GetConstantBufferData<wdLSAOConstants>(m_hLineSweepCB);
  cb->LineToLinePixelOffset = m_iLineToLinePixelOffset;

  // Compute general information per direction and create line instructions.

  // As long as we don't span out different line samplings across multiple frames, the number of prepared directions here is always equal to
  // the number of directions per frame. Note that if we were to do temporal sampling with a different line set every frame, we would need
  // to precompute all *possible* sampling directions still as a whole here!
  wdVec2I32 samplingDir[NUM_SWEEP_DIRECTIONS_PER_FRAME];
  {
    constexpr int numSweepDirs = NUM_SWEEP_DIRECTIONS_PER_FRAME;

    // As described in the paper, all directions are aligned so that we always hit  pixels on a square.
    static_assert(numSweepDirs % 4 == 0, "Invalid number of sweep directions for LSAO!");
    // static_assert((numSweepDirs * NUM_SWEEP_DIRECTIONS_PER_PIXEL) % 9 == 0, "Invalid number of sweep directions for LSAO!");
    const int perSide = (numSweepDirs + 4) / 4 - 1; // side length of the square on which all directions lie -1
    const int halfPerSide = perSide / 2 + (perSide % 2);
    for (int i = 0; i < perSide; ++i)
    {
      // Put opposing directions next to each other, so that a gather pass that doesn't sample all directions, only needs to sample an even
      // number of directions to end up with non-negative occlusion.
      samplingDir[i * 4 + 0] = wdVec2I32(i - halfPerSide, halfPerSide) * m_iLineSamplePixelOffsetFactor;  // Top
      samplingDir[i * 4 + 1] = -samplingDir[i * 4 + 0];                                                   // Bottom
      samplingDir[i * 4 + 2] = wdVec2I32(halfPerSide, halfPerSide - i) * m_iLineSamplePixelOffsetFactor;  // Right
      samplingDir[i * 4 + 3] = -samplingDir[i * 4 + 2];                                                   // Left
    }

    // todo: Ddd debug test to check whether any direction is duplicated. Mistakes in the equations above can easily happen!
#if WD_ENABLED(WD_COMPILE_FOR_DEBUG)
    for (int i = 0; i < numSweepDirs - 1; ++i)
    {
      for (int j = i + 1; j < numSweepDirs; ++j)
        WD_ASSERT_DEBUG(samplingDir[i] != samplingDir[j], "Two SSAO sampling directions are equal. Implementation for direction determination is broken.");
    }
#endif
  }

  for (int dirIndex = 0; dirIndex < WD_ARRAY_SIZE(samplingDir); ++dirIndex)
  {
    wdUInt32 totalLineCountBefore = lineInstructions.GetCount();
    AddLinesForDirection(imageResolution, samplingDir[dirIndex], dirIndex, lineInstructions, totalNumberOfSamples);
    WD_ASSERT_DEBUG(totalNumberOfSamples % 2 == 0, "Only even number of line samples are allowed");

    cb->Directions[dirIndex].Direction = wdVec2(static_cast<float>(samplingDir[dirIndex].x), static_cast<float>(samplingDir[dirIndex].y));
    cb->Directions[dirIndex].NumLines = lineInstructions.GetCount() - totalLineCountBefore;
    cb->Directions[dirIndex].LineInstructionOffset = totalLineCountBefore;
  }
  m_uiNumSweepLines = lineInstructions.GetCount();
  cb->TotalLineNumber = m_uiNumSweepLines;
  cb->TotalNumberOfSamples = totalNumberOfSamples;
  // Allocate and upload data structures to GPU
  {
    wdGALDevice* device = wdGALDevice::GetDefaultDevice();
    DestroyLineSweepData();

    // Output UAV for line sweep pass.
    // DX11 allows only float and int for writing RWBuffer, so we need to do manual packing.
    {
      wdGALBufferCreationDescription bufferDesc;
      bufferDesc.m_uiStructSize = 4;
      bufferDesc.m_uiTotalSize = imageResolution.z * 2 * totalNumberOfSamples;
      bufferDesc.m_BufferType = wdGALBufferType::Generic;
      bufferDesc.m_bUseForIndirectArguments = false;
      bufferDesc.m_bUseAsStructuredBuffer = false;
      bufferDesc.m_bAllowRawViews = false;
      bufferDesc.m_bStreamOutputTarget = false;
      bufferDesc.m_bAllowShaderResourceView = true;
      bufferDesc.m_bAllowUAV = true;
      bufferDesc.m_ResourceAccess.m_bReadBack = false;
      bufferDesc.m_ResourceAccess.m_bImmutable = false;

      m_hLineSweepOutputBuffer = device->CreateBuffer(bufferDesc);

      wdGALUnorderedAccessViewCreationDescription uavDesc;
      uavDesc.m_hBuffer = m_hLineSweepOutputBuffer;
      uavDesc.m_OverrideViewFormat = wdGALResourceFormat::RUInt;
      uavDesc.m_uiFirstElement = 0;
      uavDesc.m_uiNumElements = imageResolution.z * totalNumberOfSamples / 2;
      uavDesc.m_bRawView = false;
      uavDesc.m_bAppend = false;
      m_hLineSweepOutputUAV = device->CreateUnorderedAccessView(uavDesc);

      wdGALResourceViewCreationDescription srvDesc;
      srvDesc.m_hBuffer = m_hLineSweepOutputBuffer;
      srvDesc.m_OverrideViewFormat = wdGALResourceFormat::RUInt;
      srvDesc.m_uiFirstElement = 0;
      srvDesc.m_uiNumElements = imageResolution.z * totalNumberOfSamples / 2;
      srvDesc.m_bRawView = false;
      m_hLineSweepOutputSRV = device->CreateResourceView(srvDesc);
    }

    // Structured buffer per line.
    {
      wdGALBufferCreationDescription bufferDesc;
      bufferDesc.m_uiStructSize = sizeof(LineInstruction);
      bufferDesc.m_uiTotalSize = sizeof(LineInstruction) * m_uiNumSweepLines;
      bufferDesc.m_BufferType = wdGALBufferType::Generic;
      bufferDesc.m_bUseForIndirectArguments = false;
      bufferDesc.m_bUseAsStructuredBuffer = true;
      bufferDesc.m_bAllowRawViews = false;
      bufferDesc.m_bStreamOutputTarget = false;
      bufferDesc.m_bAllowShaderResourceView = true;
      bufferDesc.m_bAllowUAV = false;
      bufferDesc.m_ResourceAccess.m_bReadBack = false;
      bufferDesc.m_ResourceAccess.m_bImmutable = true;

      m_hLineInfoBuffer = device->CreateBuffer(bufferDesc, wdArrayPtr<const wdUInt8>(reinterpret_cast<const wdUInt8*>(lineInstructions.GetData()), lineInstructions.GetCount() * sizeof(LineInstruction)));

      m_hLineSweepInfoSRV = device->GetDefaultResourceView(m_hLineInfoBuffer);
    }
  }

  m_bSweepDataDirty = false;
}

void wdLSAOPass::AddLinesForDirection(const wdVec3I32& imageResolution, const wdVec2I32& sampleDir, wdUInt32 lineIndex, wdDynamicArray<LineInstruction>& outinLineInstructions, wdUInt32& outinTotalNumberOfSamples)
{
  WD_ASSERT_DEBUG(sampleDir.x != 0 || sampleDir.y != 0, "Sample direction is null (not pointing anywhere)");

  wdUInt32 firstNewLineInstructionIndex = outinLineInstructions.GetCount();

  // Always walk positive and flip if necessary later.
  wdVec2I32 walkDir(wdMath::Abs(sampleDir.x), wdMath::Abs(sampleDir.y));
  wdVec2 walkDirF(static_cast<float>(walkDir.x), static_cast<float>(walkDir.y));

  // Line "creation" always starts from 0,0 and walks along EITHER x or y depending which one is the less dominant axis.

  // Helper to avoid duplication for dominant x/y
  int domDir = walkDir.x > walkDir.y ? 0 : 1;
  int secDir = 1 - domDir;
#define DOM GetData()[domDir]
#define SEC GetData()[secDir]

  // Walk along secondary axis backwards.
  for (wdInt32 sec = imageResolution.SEC - 1; true; sec -= m_iLineToLinePixelOffset)
  {
    LineInstruction& newLine = outinLineInstructions.ExpandAndGetRef();
    newLine.FirstSamplePos.DOM = 0.0f;
    newLine.FirstSamplePos.SEC = static_cast<float>(sec);

    // If we are already outside of the screen with sec, this is not a point inside the screen!
    if (sec < 0)
    {
      // If we don't walk in the secondary direction at all this means that we're done.
      if (walkDir.SEC == 0)
      {
        outinLineInstructions.PopBack();
        break;
      }
      // Otherwise we just need to walk long enough to hit the screen again.
      else
      {
        // Find new start on the sec axis. (dom axis is fine)
        wdVec2 minimalStepToBorder = walkDirF * wdMath::Ceil(static_cast<float>(-sec) / walkDirF.SEC); // Remember: Only walk discrete steps!
        newLine.FirstSamplePos.DOM += minimalStepToBorder.DOM;
        newLine.FirstSamplePos.SEC += minimalStepToBorder.SEC;

        // Outside, we're done.
        if (newLine.FirstSamplePos.DOM >= imageResolution.DOM - walkDir.DOM * 2)
        {
          outinLineInstructions.PopBack();
          break;
        }
      }
    }

    // Add a pseudo random offset to distributed the samples a bit.
    // We still want to go from discrete pixel to discrete pixel so we have to round which can mess up our line placement.
    // So this is introducing some error. Visual comparison clearly shows that it's worth it though.
    float offset = HaltonSequence(lineIndex, sec + lineIndex);
    newLine.FirstSamplePos.DOM += wdMath::Round(offset * walkDir.DOM);
    newLine.FirstSamplePos.SEC += wdMath::Round(offset * walkDir.SEC);

    // Clamp back to possible area.
    // Due to the way we jump from pixels to line in the gather shader, we can't just discard lines.
    newLine.FirstSamplePos.x = wdMath::Clamp<float>(newLine.FirstSamplePos.x, 0.0f, imageResolution.x - 1.0f);
    newLine.FirstSamplePos.y = wdMath::Clamp<float>(newLine.FirstSamplePos.y, 0.0f, imageResolution.y - 1.0f);

    // Compute how many samples this line will consume.
    unsigned int stepsToDOMBorder = static_cast<unsigned int>((imageResolution.DOM - newLine.FirstSamplePos.DOM) / walkDir.DOM + 1);
    unsigned int numSamples = 0;
    if (walkDir.SEC > 0)
    {
      unsigned int stepsToSECBorder = static_cast<unsigned int>((imageResolution.SEC - newLine.FirstSamplePos.SEC) / walkDir.SEC + 1);
      numSamples = wdMath::Min(stepsToSECBorder, stepsToDOMBorder);
    }
    else
      numSamples = stepsToDOMBorder;

    // Due to output packing restrictions only even number of samples are allowed. Remove one if necessary.
    if (numSamples % 2 != 0)
      --numSamples;

    newLine.LineSweepOutputBufferOffset = outinTotalNumberOfSamples;
    outinTotalNumberOfSamples += numSamples;
    newLine.LineDirIndex_NumSamples = lineIndex | (numSamples << 16);
  }

#undef SEC
#undef DOM

  // Now consider x/y being negative.
  for (int c = 0; c < 2; ++c)
  {
    if (sampleDir.GetData()[c] < 0)
    {
      for (wdUInt32 i = firstNewLineInstructionIndex; i < outinLineInstructions.GetCount(); ++i)
      {
        outinLineInstructions[i].FirstSamplePos.GetData()[c] = imageResolution.GetData()[c] - 1 - outinLineInstructions[i].FirstSamplePos.GetData()[c];
      }
    }
  }

  // Validation.
#if WD_ENABLED(WD_COMPILE_FOR_DEBUG)
  for (wdUInt32 i = firstNewLineInstructionIndex; i < outinLineInstructions.GetCount(); ++i)
  {
    auto p = outinLineInstructions[i].FirstSamplePos;
    WD_ASSERT_DEV(p.x >= 0 && p.y >= 0 && p.x < imageResolution.x && p.y < imageResolution.y, "First sweep line sample pos is invalid. Something is wrong with the sweep line generation algorithm.");
  }
#endif
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_LSAOPass);
