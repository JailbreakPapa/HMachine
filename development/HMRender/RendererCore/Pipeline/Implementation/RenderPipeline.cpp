#include <RendererCore/RendererCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/ColorScheme.h>
#include <Foundation/Math/Frustum.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Utilities/DGMLWriter.h>
#include <RendererCore/Components/AlwaysVisibleComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/FrameDataProvider.h>
#include <RendererCore/Pipeline/Passes/TargetPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Rasterizer/RasterizerView.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
wdCVarBool wdRenderPipeline::cvar_SpatialCullingVis("Spatial.Culling.Vis", false, wdCVarFlags::Default, "Enables debug visualization of visibility culling");
wdCVarBool cvar_SpatialCullingShowStats("Spatial.Culling.ShowStats", false, wdCVarFlags::Default, "Display some stats of the visibility culling");
#endif

wdCVarBool cvar_SpatialCullingOcclusionEnable("Spatial.Occlusion.Enable", true, wdCVarFlags::Default, "Use software rasterization for occlusion culling.");
wdCVarBool cvar_SpatialCullingOcclusionVisView("Spatial.Occlusion.VisView", false, wdCVarFlags::Default, "Render the occlusion framebuffer as an overlay.");
wdCVarFloat cvar_SpatialCullingOcclusionBoundsInlation("Spatial.Occlusion.BoundsInflation", 0.5f, wdCVarFlags::Default, "How much to inflate bounds during occlusion check.");
wdCVarFloat cvar_SpatialCullingOcclusionFarPlane("Spatial.Occlusion.FarPlane", 50.0f, wdCVarFlags::Default, "Far plane distance for finding occluders.");

wdRenderPipeline::wdRenderPipeline()

{
  m_CurrentExtractThread = (wdThreadID)0;
  m_CurrentRenderThread = (wdThreadID)0;
  m_uiLastExtractionFrame = -1;
  m_uiLastRenderFrame = -1;

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  m_AverageCullingTime = wdTime::Seconds(0.1f);
#endif
}

wdRenderPipeline::~wdRenderPipeline()
{
  if (!m_hOcclusionDebugViewTexture.IsInvalidated())
  {
    wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();
    const wdGALTexture* pTexture = pDevice->GetTexture(m_hOcclusionDebugViewTexture);

    pDevice->DestroyTexture(m_hOcclusionDebugViewTexture);
    m_hOcclusionDebugViewTexture.Invalidate();
  }

  m_Data[0].Clear();
  m_Data[1].Clear();

  ClearRenderPassGraphTextures();
  while (!m_Passes.IsEmpty())
  {
    RemovePass(m_Passes.PeekBack().Borrow());
  }
}

void wdRenderPipeline::AddPass(wdUniquePtr<wdRenderPipelinePass>&& pPass)
{
  m_PipelineState = PipelineState::Uninitialized;
  pPass->m_pPipeline = this;
  pPass->InitializePins();

  auto it = m_Connections.Insert(pPass.Borrow(), ConnectionData());
  it.Value().m_Inputs.SetCount(pPass->GetInputPins().GetCount());
  it.Value().m_Outputs.SetCount(pPass->GetOutputPins().GetCount());
  m_Passes.PushBack(std::move(pPass));
}

void wdRenderPipeline::RemovePass(wdRenderPipelinePass* pPass)
{
  for (wdUInt32 i = 0; i < m_Passes.GetCount(); ++i)
  {
    if (m_Passes[i].Borrow() == pPass)
    {
      m_PipelineState = PipelineState::Uninitialized;
      RemoveConnections(pPass);
      m_Connections.Remove(pPass);
      pPass->m_pPipeline = nullptr;
      m_Passes.RemoveAtAndCopy(i);
      break;
    }
  }
}

void wdRenderPipeline::GetPasses(wdHybridArray<const wdRenderPipelinePass*, 16>& ref_passes) const
{
  ref_passes.Reserve(m_Passes.GetCount());

  for (auto& pPass : m_Passes)
  {
    ref_passes.PushBack(pPass.Borrow());
  }
}

void wdRenderPipeline::GetPasses(wdHybridArray<wdRenderPipelinePass*, 16>& ref_passes)
{
  ref_passes.Reserve(m_Passes.GetCount());

  for (auto& pPass : m_Passes)
  {
    ref_passes.PushBack(pPass.Borrow());
  }
}

wdRenderPipelinePass* wdRenderPipeline::GetPassByName(const wdStringView& sPassName)
{
  for (auto& pPass : m_Passes)
  {
    if (sPassName.IsEqual(pPass->GetName()))
    {
      return pPass.Borrow();
    }
  }

  return nullptr;
}

wdHashedString wdRenderPipeline::GetViewName() const
{
  return m_sName;
}

bool wdRenderPipeline::Connect(wdRenderPipelinePass* pOutputNode, const char* szOutputPinName, wdRenderPipelinePass* pInputNode, const char* szInputPinName)
{
  wdHashedString sOutputPinName;
  sOutputPinName.Assign(szOutputPinName);
  wdHashedString sInputPinName;
  sInputPinName.Assign(szInputPinName);
  return Connect(pOutputNode, sOutputPinName, pInputNode, sInputPinName);
}

bool wdRenderPipeline::Connect(wdRenderPipelinePass* pOutputNode, wdHashedString sOutputPinName, wdRenderPipelinePass* pInputNode, wdHashedString sInputPinName)
{
  wdLogBlock b("wdRenderPipeline::Connect");

  auto itOut = m_Connections.Find(pOutputNode);
  if (!itOut.IsValid())
  {
    wdLog::Error("Output node '{0}' not added to pipeline!", pOutputNode->GetName());
    return false;
  }
  auto itIn = m_Connections.Find(pInputNode);
  if (!itIn.IsValid())
  {
    wdLog::Error("Input node '{0}' not added to pipeline!", pInputNode->GetName());
    return false;
  }
  const wdRenderPipelineNodePin* pPinSource = pOutputNode->GetPinByName(sOutputPinName);
  if (!pPinSource)
  {
    wdLog::Error("Source pin '{0}::{1}' does not exist!", pOutputNode->GetName(), sOutputPinName);
    return false;
  }
  const wdRenderPipelineNodePin* pPinTarget = pInputNode->GetPinByName(sInputPinName);
  if (!pPinTarget)
  {
    wdLog::Error("Target pin '{0}::{1}' does not exist!", pInputNode->GetName(), sInputPinName);
    return false;
  }
  if (itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] != nullptr)
  {
    wdLog::Error("Pins already connected: '{0}::{1}' -> '{2}::{3}'!", pOutputNode->GetName(), sOutputPinName, pInputNode->GetName(), sInputPinName);
    return false;
  }

  // Add at output
  wdRenderPipelinePassConnection* pConnection = itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex];
  if (pConnection == nullptr)
  {
    pConnection = WD_DEFAULT_NEW(wdRenderPipelinePassConnection);
    pConnection->m_pOutput = pPinSource;
    itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex] = pConnection;
  }
  else
  {
    // Check that only one passthrough is connected
    if (pPinTarget->m_Type == wdRenderPipelineNodePin::Type::PassThrough)
    {
      for (const wdRenderPipelineNodePin* pPin : pConnection->m_Inputs)
      {
        if (pPin->m_Type == wdRenderPipelineNodePin::Type::PassThrough)
        {
          wdLog::Error("A pass through pin is already connected to the '{0}' pin!", sOutputPinName);
          return false;
        }
      }
    }
  }

  // Add at input
  pConnection->m_Inputs.PushBack(pPinTarget);
  itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] = pConnection;
  m_PipelineState = PipelineState::Uninitialized;
  return true;
}

bool wdRenderPipeline::Disconnect(wdRenderPipelinePass* pOutputNode, wdHashedString sOutputPinName, wdRenderPipelinePass* pInputNode, wdHashedString sInputPinName)
{
  wdLogBlock b("wdRenderPipeline::Connect");

  auto itOut = m_Connections.Find(pOutputNode);
  if (!itOut.IsValid())
  {
    wdLog::Error("Output node '{0}' not added to pipeline!", pOutputNode->GetName());
    return false;
  }
  auto itIn = m_Connections.Find(pInputNode);
  if (!itIn.IsValid())
  {
    wdLog::Error("Input node '{0}' not added to pipeline!", pInputNode->GetName());
    return false;
  }
  const wdRenderPipelineNodePin* pPinSource = pOutputNode->GetPinByName(sOutputPinName);
  if (!pPinSource)
  {
    wdLog::Error("Source pin '{0}::{1}' does not exist!", pOutputNode->GetName(), sOutputPinName);
    return false;
  }
  const wdRenderPipelineNodePin* pPinTarget = pInputNode->GetPinByName(sInputPinName);
  if (!pPinTarget)
  {
    wdLog::Error("Target pin '{0}::{1}' does not exist!", pInputNode->GetName(), sInputPinName);
    return false;
  }
  if (itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] == nullptr || itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] != itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex])
  {
    wdLog::Error("Pins not connected: '{0}::{1}' -> '{2}::{3}'!", pOutputNode->GetName(), sOutputPinName, pInputNode->GetName(), sInputPinName);
    return false;
  }

  // Remove at input
  wdRenderPipelinePassConnection* pConnection = itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex];
  pConnection->m_Inputs.RemoveAndCopy(pPinTarget);
  itIn.Value().m_Inputs[pPinTarget->m_uiInputIndex] = nullptr;

  if (pConnection->m_Inputs.IsEmpty())
  {
    // Remove at output
    itOut.Value().m_Outputs[pPinSource->m_uiOutputIndex] = nullptr;
    WD_DEFAULT_DELETE(pConnection);
  }

  m_PipelineState = PipelineState::Uninitialized;
  return true;
}

const wdRenderPipelinePassConnection* wdRenderPipeline::GetInputConnection(wdRenderPipelinePass* pPass, wdHashedString sInputPinName) const
{
  auto it = m_Connections.Find(pPass);
  if (!it.IsValid())
    return nullptr;

  auto& data = it.Value();
  const wdRenderPipelineNodePin* pPin = pPass->GetPinByName(sInputPinName);
  if (!pPin || pPin->m_uiInputIndex == 0xFF)
    return nullptr;

  return data.m_Inputs[pPin->m_uiInputIndex];
}

const wdRenderPipelinePassConnection* wdRenderPipeline::GetOutputConnection(wdRenderPipelinePass* pPass, wdHashedString sOutputPinName) const
{
  auto it = m_Connections.Find(pPass);
  if (!it.IsValid())
    return nullptr;

  auto& data = it.Value();
  const wdRenderPipelineNodePin* pPin = pPass->GetPinByName(sOutputPinName);
  if (!pPin)
    return nullptr;

  return data.m_Outputs[pPin->m_uiOutputIndex];
}

wdRenderPipeline::PipelineState wdRenderPipeline::Rebuild(const wdView& view)
{
  wdLogBlock b("wdRenderPipeline::Rebuild");

  ClearRenderPassGraphTextures();

  bool bRes = RebuildInternal(view);
  if (!bRes)
  {
    ClearRenderPassGraphTextures();
  }
  else
  {
    // make sure the renderdata stores the updated view data
    UpdateViewData(view, wdRenderWorld::GetDataIndexForRendering());
  }

  m_PipelineState = bRes ? PipelineState::Initialized : PipelineState::RebuildError;
  return m_PipelineState;
}

bool wdRenderPipeline::RebuildInternal(const wdView& view)
{
  if (!SortPasses())
    return false;
  if (!InitRenderTargetDescriptions(view))
    return false;
  if (!CreateRenderTargetUsage(view))
    return false;
  if (!InitRenderPipelinePasses())
    return false;

  SortExtractors();

  return true;
}

bool wdRenderPipeline::SortPasses()
{
  wdLogBlock b("Sort Passes");
  wdHybridArray<wdRenderPipelinePass*, 32> done;
  done.Reserve(m_Passes.GetCount());

  wdHybridArray<wdRenderPipelinePass*, 8> usable;     // Stack of passes with all connections setup, they can be asked for descriptions.
  wdHybridArray<wdRenderPipelinePass*, 8> candidates; // Not usable yet, but all input connections are available

  // Find all source passes from which we can start the output description propagation.
  for (auto& pPass : m_Passes)
  {
    // if (std::all_of(cbegin(it.Value().m_Inputs), cend(it.Value().m_Inputs), [](wdRenderPipelinePassConnection* pConn){return pConn ==
    // nullptr; }))
    if (AreInputDescriptionsAvailable(pPass.Borrow(), done))
    {
      usable.PushBack(pPass.Borrow());
    }
  }

  // Via a depth first traversal, order the passes
  while (!usable.IsEmpty())
  {
    wdRenderPipelinePass* pPass = usable.PeekBack();
    wdLogBlock b2("Traverse", pPass->GetName());

    usable.PopBack();
    ConnectionData& data = m_Connections[pPass];

    WD_ASSERT_DEBUG(data.m_Inputs.GetCount() == pPass->GetInputPins().GetCount(), "Input pin count missmatch!");
    WD_ASSERT_DEBUG(data.m_Outputs.GetCount() == pPass->GetOutputPins().GetCount(), "Output pin count missmatch!");

    // Check for new candidate passes. Can't be done in the previous loop as multiple connections may be required by a node.
    for (wdUInt32 i = 0; i < data.m_Outputs.GetCount(); i++)
    {
      if (data.m_Outputs[i] != nullptr)
      {
        // Go through all inputs this connection is connected to and test the corresponding node for availability
        for (const wdRenderPipelineNodePin* pPin : data.m_Outputs[i]->m_Inputs)
        {
          WD_ASSERT_DEBUG(pPin->m_pParent != nullptr, "Pass was not initialized!");
          wdRenderPipelinePass* pTargetPass = static_cast<wdRenderPipelinePass*>(pPin->m_pParent);
          if (done.Contains(pTargetPass))
          {
            wdLog::Error("Loop detected, graph not supported!");
            return false;
          }

          if (!usable.Contains(pTargetPass) && !candidates.Contains(pTargetPass))
          {
            candidates.PushBack(pTargetPass);
          }
        }
      }
    }

    done.PushBack(pPass);

    // Check for usable candidates. Reverse order for depth first traversal.
    for (wdInt32 i = (wdInt32)candidates.GetCount() - 1; i >= 0; i--)
    {
      wdRenderPipelinePass* pCandidatePass = candidates[i];
      if (AreInputDescriptionsAvailable(pCandidatePass, done) && ArePassThroughInputsDone(pCandidatePass, done))
      {
        usable.PushBack(pCandidatePass);
        candidates.RemoveAtAndCopy(i);
      }
    }
  }

  if (done.GetCount() < m_Passes.GetCount())
  {
    wdLog::Error("Pipeline: Not all nodes could be initialized");
    return false;
  }

  struct wdPipelineSorter
  {
    /// \brief Returns true if a is less than b
    WD_FORCE_INLINE bool Less(const wdUniquePtr<wdRenderPipelinePass>& a, const wdUniquePtr<wdRenderPipelinePass>& b) const { return m_pDone->IndexOf(a.Borrow()) < m_pDone->IndexOf(b.Borrow()); }

    /// \brief Returns true if a is equal to b
    WD_ALWAYS_INLINE bool Equal(const wdUniquePtr<wdRenderPipelinePass>& a, const wdUniquePtr<wdRenderPipelinePass>& b) const { return a.Borrow() == b.Borrow(); }

    wdHybridArray<wdRenderPipelinePass*, 32>* m_pDone;
  };

  wdPipelineSorter sorter;
  sorter.m_pDone = &done;
  m_Passes.Sort(sorter);
  return true;
}

bool wdRenderPipeline::InitRenderTargetDescriptions(const wdView& view)
{
  wdLogBlock b("Init Render Target Descriptions");
  wdHybridArray<wdGALTextureCreationDescription*, 10> inputs;
  wdHybridArray<wdGALTextureCreationDescription, 10> outputs;

  for (auto& pPass : m_Passes)
  {
    wdLogBlock b2("InitPass", pPass->GetName());

    if (view.GetCamera()->IsStereoscopic() && !pPass->IsStereoAware())
    {
      wdLog::Error("View '{0}' uses a stereoscopic camera, but the render pass '{1}' does not support stereo rendering!", view.GetName(), pPass->GetName());
    }

    ConnectionData& data = m_Connections[pPass.Borrow()];

    WD_ASSERT_DEBUG(data.m_Inputs.GetCount() == pPass->GetInputPins().GetCount(), "Input pin count missmatch!");
    WD_ASSERT_DEBUG(data.m_Outputs.GetCount() == pPass->GetOutputPins().GetCount(), "Output pin count missmatch!");

    inputs.SetCount(data.m_Inputs.GetCount());
    outputs.Clear();
    outputs.SetCount(data.m_Outputs.GetCount());
    // Fill inputs array
    for (wdUInt32 i = 0; i < data.m_Inputs.GetCount(); i++)
    {
      if (data.m_Inputs[i] != nullptr)
      {
        inputs[i] = &data.m_Inputs[i]->m_Desc;
      }
      else
      {
        inputs[i] = nullptr;
      }
    }

    bool bRes = pPass->GetRenderTargetDescriptions(view, inputs, outputs);
    if (!bRes)
    {
      wdLog::Error("The pass could not be successfully queried for render target descriptions.");
      return false;
    }

    // Copy queried outputs into the output connections.
    for (wdUInt32 i = 0; i < data.m_Outputs.GetCount(); i++)
    {
      if (data.m_Outputs[i] != nullptr)
      {
        data.m_Outputs[i]->m_Desc = outputs[i];
      }
    }

    // Check pass-through consistency of input / output target desc.
    auto inputPins = pPass->GetInputPins();
    for (const wdRenderPipelineNodePin* pPin : inputPins)
    {
      if (pPin->m_Type == wdRenderPipelineNodePin::Type::PassThrough)
      {
        if (data.m_Outputs[pPin->m_uiOutputIndex] != nullptr)
        {
          if (data.m_Inputs[pPin->m_uiInputIndex] == nullptr)
          {
            // wdLog::Error("The pass of type '{0}' has a pass through pin '{1}' that has an output but no input!",
            // pPass->GetDynamicRTTI()->GetTypeName(), pPass->GetPinName(pPin));  return false;
          }
          else if (data.m_Outputs[pPin->m_uiOutputIndex]->m_Desc.CalculateHash() != data.m_Inputs[pPin->m_uiInputIndex]->m_Desc.CalculateHash())
          {
            wdLog::Error("The pass has a pass through pin '{0}' that has different descriptors for input and output!", pPass->GetPinName(pPin));
            return false;
          }
        }
      }
    }
  }
  return true;
}

bool wdRenderPipeline::CreateRenderTargetUsage(const wdView& view)
{
  wdLogBlock b("Create Render Target Usage Data");
  WD_ASSERT_DEBUG(m_TextureUsage.IsEmpty(), "Need to call ClearRenderPassGraphTextures before re-creating the pipeline.");

  m_ConnectionToTextureIndex.Clear();

  // Gather all connections that share the same path-through texture and their first and last usage pass index.
  for (wdUInt16 i = 0; i < static_cast<wdUInt16>(m_Passes.GetCount()); i++)
  {
    const auto& pPass = m_Passes[i].Borrow();
    ConnectionData& data = m_Connections[pPass];
    for (wdRenderPipelinePassConnection* pConn : data.m_Inputs)
    {
      if (pConn != nullptr)
      {
        wdUInt32 uiDataIdx = m_ConnectionToTextureIndex[pConn];
        m_TextureUsage[uiDataIdx].m_uiLastUsageIdx = i;
      }
    }

    for (wdRenderPipelinePassConnection* pConn : data.m_Outputs)
    {
      if (pConn != nullptr)
      {
        if (pConn->m_pOutput->m_Type == wdRenderPipelineNodePin::Type::PassThrough && data.m_Inputs[pConn->m_pOutput->m_uiInputIndex] != nullptr)
        {
          wdRenderPipelinePassConnection* pCorrespondingInputConn = data.m_Inputs[pConn->m_pOutput->m_uiInputIndex];
          WD_ASSERT_DEV(m_ConnectionToTextureIndex.Contains(pCorrespondingInputConn), "");
          wdUInt32 uiDataIdx = m_ConnectionToTextureIndex[pCorrespondingInputConn];
          m_TextureUsage[uiDataIdx].m_UsedBy.PushBack(pConn);
          m_TextureUsage[uiDataIdx].m_uiLastUsageIdx = i;

          WD_ASSERT_DEV(!m_ConnectionToTextureIndex.Contains(pConn), "");
          m_ConnectionToTextureIndex[pConn] = uiDataIdx;
        }
        else
        {
          m_ConnectionToTextureIndex[pConn] = m_TextureUsage.GetCount();
          TextureUsageData& texData = m_TextureUsage.ExpandAndGetRef();

          texData.m_iTargetTextureIndex = -1;
          texData.m_uiFirstUsageIdx = i;
          texData.m_uiLastUsageIdx = i;
          texData.m_UsedBy.PushBack(pConn);
        }
      }
    }
  }

  static wdUInt32 defaultTextureDescHash = wdGALTextureCreationDescription().CalculateHash();
  // Set view's render target textures to target pass connections.
  for (wdUInt32 i = 0; i < m_Passes.GetCount(); i++)
  {
    const auto& pPass = m_Passes[i].Borrow();
    if (pPass->IsInstanceOf<wdTargetPass>())
    {
      const wdGALRenderTargets& renderTargets = view.GetActiveRenderTargets();

      wdTargetPass* pTargetPass = static_cast<wdTargetPass*>(pPass);
      ConnectionData& data = m_Connections[pPass];
      for (wdUInt32 j = 0; j < data.m_Inputs.GetCount(); j++)
      {
        wdRenderPipelinePassConnection* pConn = data.m_Inputs[j];
        if (pConn != nullptr)
        {
          const wdGALTextureHandle* hTexture = pTargetPass->GetTextureHandle(renderTargets, pPass->GetInputPins()[j]);
          WD_ASSERT_DEV(m_ConnectionToTextureIndex.Contains(pConn), "");

          if (!hTexture || !hTexture->IsInvalidated() || pConn->m_Desc.CalculateHash() == defaultTextureDescHash)
          {
            wdUInt32 uiDataIdx = m_ConnectionToTextureIndex[pConn];
            m_TextureUsage[uiDataIdx].m_iTargetTextureIndex = static_cast<wdInt32>(hTexture - reinterpret_cast<const wdGALTextureHandle*>(&renderTargets));
            WD_ASSERT_DEV(reinterpret_cast<const wdGALTextureHandle*>(&renderTargets)[m_TextureUsage[uiDataIdx].m_iTargetTextureIndex] == *hTexture, "Offset computation broken.");

            for (auto pUsedByConn : m_TextureUsage[uiDataIdx].m_UsedBy)
            {
              pUsedByConn->m_TextureHandle = *hTexture;
            }
          }
          else
          {
            // In this case, the wdTargetPass does not provide a render target for the connection but the descriptor is set so we can instead use the pool to supplement the missing texture.
          }
        }
      }
    }
  }

  // Stupid loop to gather all TextureUsageData indices that are not view render target textures.
  for (wdUInt32 i = 0; i < m_TextureUsage.GetCount(); i++)
  {
    TextureUsageData& data = m_TextureUsage[i];
    if (data.m_iTargetTextureIndex != -1)
      continue;

    m_TextureUsageIdxSortedByFirstUsage.PushBack((wdUInt16)i);
    m_TextureUsageIdxSortedByLastUsage.PushBack((wdUInt16)i);
  }

  // Sort first and last usage arrays, these will determine the lifetime of the pool textures.
  struct FirstUsageComparer
  {
    FirstUsageComparer(wdDynamicArray<TextureUsageData>& ref_textureUsage)
      : m_TextureUsage(ref_textureUsage)
    {
    }

    WD_ALWAYS_INLINE bool Less(wdUInt16 a, wdUInt16 b) const { return m_TextureUsage[a].m_uiFirstUsageIdx < m_TextureUsage[b].m_uiFirstUsageIdx; }

    wdDynamicArray<TextureUsageData>& m_TextureUsage;
  };

  struct LastUsageComparer
  {
    LastUsageComparer(wdDynamicArray<TextureUsageData>& ref_textureUsage)
      : m_TextureUsage(ref_textureUsage)
    {
    }

    WD_ALWAYS_INLINE bool Less(wdUInt16 a, wdUInt16 b) const { return m_TextureUsage[a].m_uiLastUsageIdx < m_TextureUsage[b].m_uiLastUsageIdx; }

    wdDynamicArray<TextureUsageData>& m_TextureUsage;
  };

  m_TextureUsageIdxSortedByFirstUsage.Sort(FirstUsageComparer(m_TextureUsage));
  m_TextureUsageIdxSortedByLastUsage.Sort(LastUsageComparer(m_TextureUsage));

  return true;
}

bool wdRenderPipeline::InitRenderPipelinePasses()
{
  wdLogBlock b("Init Render Pipeline Passes");
  // Init every pass now.
  for (auto& pPass : m_Passes)
  {
    ConnectionData& data = m_Connections[pPass.Borrow()];
    pPass->InitRenderPipelinePass(data.m_Inputs, data.m_Outputs);
  }

  return true;
}

void wdRenderPipeline::SortExtractors()
{
  struct Helper
  {
    static bool FindDependency(const wdHashedString& sDependency, wdArrayPtr<wdUniquePtr<wdExtractor>> container)
    {
      for (auto& extractor : container)
      {
        if (sDependency == wdTempHashedString(extractor->GetDynamicRTTI()->GetTypeNameHash()))
        {
          return true;
        }
      }

      return false;
    }
  };

  m_SortedExtractors.Clear();
  m_SortedExtractors.Reserve(m_Extractors.GetCount());

  wdUInt32 uiIndex = 0;
  while (!m_Extractors.IsEmpty())
  {
    wdUniquePtr<wdExtractor>& extractor = m_Extractors[uiIndex];

    bool allDependenciesFound = true;
    for (auto& sDependency : extractor->m_DependsOn)
    {
      if (!Helper::FindDependency(sDependency, m_SortedExtractors))
      {
        allDependenciesFound = false;
        break;
      }
    }

    if (allDependenciesFound)
    {
      m_SortedExtractors.PushBack(std::move(extractor));
      m_Extractors.RemoveAtAndCopy(uiIndex);
    }
    else
    {
      ++uiIndex;
    }

    if (uiIndex >= m_Extractors.GetCount())
    {
      uiIndex = 0;
    }
  }

  m_Extractors.Swap(m_SortedExtractors);
}

void wdRenderPipeline::UpdateViewData(const wdView& view, wdUInt32 uiDataIndex)
{
  if (!view.IsValid())
    return;

  if (uiDataIndex == wdRenderWorld::GetDataIndexForExtraction() && m_CurrentExtractThread != (wdThreadID)0)
    return;

  WD_ASSERT_DEV(uiDataIndex <= 1, "Data index must be 0 or 1");
  auto& data = m_Data[uiDataIndex];

  data.SetCamera(*view.GetCamera());
  data.SetViewData(view.GetData());
}

void wdRenderPipeline::AddExtractor(wdUniquePtr<wdExtractor>&& pExtractor)
{
  m_Extractors.PushBack(std::move(pExtractor));
}

void wdRenderPipeline::RemoveExtractor(wdExtractor* pExtractor)
{
  for (wdUInt32 i = 0; i < m_Extractors.GetCount(); ++i)
  {
    if (m_Extractors[i].Borrow() == pExtractor)
    {
      m_Extractors.RemoveAtAndCopy(i);
      break;
    }
  }
}

void wdRenderPipeline::GetExtractors(wdHybridArray<const wdExtractor*, 16>& ref_extractors) const
{
  ref_extractors.Reserve(m_Extractors.GetCount());

  for (auto& pExtractor : m_Extractors)
  {
    ref_extractors.PushBack(pExtractor.Borrow());
  }
}

void wdRenderPipeline::GetExtractors(wdHybridArray<wdExtractor*, 16>& ref_extractors)
{
  ref_extractors.Reserve(m_Extractors.GetCount());

  for (auto& pExtractor : m_Extractors)
  {
    ref_extractors.PushBack(pExtractor.Borrow());
  }
}


wdExtractor* wdRenderPipeline::GetExtractorByName(const wdStringView& sExtractorName)
{
  for (auto& pExtractor : m_Extractors)
  {
    if (sExtractorName.IsEqual(pExtractor->GetName()))
    {
      return pExtractor.Borrow();
    }
  }

  return nullptr;
}

void wdRenderPipeline::RemoveConnections(wdRenderPipelinePass* pPass)
{
  auto it = m_Connections.Find(pPass);
  if (!it.IsValid())
    return;

  ConnectionData& data = it.Value();
  for (wdUInt32 i = 0; i < data.m_Inputs.GetCount(); i++)
  {
    wdRenderPipelinePassConnection* pConn = data.m_Inputs[i];
    if (pConn != nullptr)
    {
      wdRenderPipelinePass* pSource = static_cast<wdRenderPipelinePass*>(pConn->m_pOutput->m_pParent);
      bool bRes = Disconnect(pSource, pSource->GetPinName(pConn->m_pOutput), pPass, pPass->GetPinName(pPass->GetInputPins()[i]));
      WD_IGNORE_UNUSED(bRes);
      WD_ASSERT_DEBUG(bRes, "wdRenderPipeline::RemoveConnections should not fail to disconnect pins!");
    }
  }
  for (wdUInt32 i = 0; i < data.m_Outputs.GetCount(); i++)
  {
    wdRenderPipelinePassConnection* pConn = data.m_Outputs[i];
    while (pConn != nullptr)
    {
      wdRenderPipelinePass* pTarget = static_cast<wdRenderPipelinePass*>(pConn->m_Inputs[0]->m_pParent);
      bool bRes = Disconnect(pPass, pPass->GetPinName(pConn->m_pOutput), pTarget, pTarget->GetPinName(pConn->m_Inputs[0]));
      WD_IGNORE_UNUSED(bRes);
      WD_ASSERT_DEBUG(bRes, "wdRenderPipeline::RemoveConnections should not fail to disconnect pins!");

      pConn = data.m_Outputs[i];
    }
  }
}

void wdRenderPipeline::ClearRenderPassGraphTextures()
{
  m_TextureUsage.Clear();
  m_TextureUsageIdxSortedByFirstUsage.Clear();
  m_TextureUsageIdxSortedByLastUsage.Clear();

  // wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  for (auto it = m_Connections.GetIterator(); it.IsValid(); ++it)
  {
    auto& conn = it.Value();
    for (auto pConn : conn.m_Outputs)
    {
      if (pConn)
      {
        pConn->m_Desc = wdGALTextureCreationDescription();
        if (!pConn->m_TextureHandle.IsInvalidated())
        {
          pConn->m_TextureHandle.Invalidate();
        }
      }
    }
  }
}

bool wdRenderPipeline::AreInputDescriptionsAvailable(const wdRenderPipelinePass* pPass, const wdHybridArray<wdRenderPipelinePass*, 32>& done) const
{
  auto it = m_Connections.Find(pPass);
  const ConnectionData& data = it.Value();
  for (wdUInt32 i = 0; i < data.m_Inputs.GetCount(); i++)
  {
    const wdRenderPipelinePassConnection* pConn = data.m_Inputs[i];
    if (pConn != nullptr)
    {
      // If the connections source is not done yet, the connections output is undefined yet and the inputs can't be processed yet.
      if (!done.Contains(static_cast<wdRenderPipelinePass*>(pConn->m_pOutput->m_pParent)))
      {
        return false;
      }
    }
  }

  return true;
}

bool wdRenderPipeline::ArePassThroughInputsDone(const wdRenderPipelinePass* pPass, const wdHybridArray<wdRenderPipelinePass*, 32>& done) const
{
  auto it = m_Connections.Find(pPass);
  const ConnectionData& data = it.Value();
  auto inputs = pPass->GetInputPins();
  for (wdUInt32 i = 0; i < inputs.GetCount(); i++)
  {
    const wdRenderPipelineNodePin* pPin = inputs[i];
    if (pPin->m_Type == wdRenderPipelineNodePin::Type::PassThrough)
    {
      const wdRenderPipelinePassConnection* pConn = data.m_Inputs[pPin->m_uiInputIndex];
      if (pConn != nullptr)
      {
        for (const wdRenderPipelineNodePin* pInputPin : pConn->m_Inputs)
        {
          // Any input that is also connected to the source of pPin must be done before we can use the pass through input
          if (pInputPin != pPin && !done.Contains(static_cast<wdRenderPipelinePass*>(pInputPin->m_pParent)))
          {
            return false;
          }
        }
      }
    }
  }
  return true;
}

wdFrameDataProviderBase* wdRenderPipeline::GetFrameDataProvider(const wdRTTI* pRtti) const
{
  wdUInt32 uiIndex = 0;
  if (m_TypeToDataProviderIndex.TryGetValue(pRtti, uiIndex))
  {
    return m_DataProviders[uiIndex].Borrow();
  }

  wdUniquePtr<wdFrameDataProviderBase> pNewDataProvider = pRtti->GetAllocator()->Allocate<wdFrameDataProviderBase>();
  wdFrameDataProviderBase* pResult = pNewDataProvider.Borrow();
  pResult->m_pOwnerPipeline = this;

  m_TypeToDataProviderIndex.Insert(pRtti, m_DataProviders.GetCount());
  m_DataProviders.PushBack(std::move(pNewDataProvider));

  return pResult;
}

void wdRenderPipeline::ExtractData(const wdView& view)
{
  WD_ASSERT_DEV(m_CurrentExtractThread == (wdThreadID)0, "Extract must not be called from multiple threads.");
  m_CurrentExtractThread = wdThreadUtils::GetCurrentThreadID();

  // Is this view already extracted?
  if (m_uiLastExtractionFrame == wdRenderWorld::GetFrameCounter())
  {
    WD_REPORT_FAILURE("View '{0}' is extracted multiple times", view.GetName());
    return;
  }

  m_uiLastExtractionFrame = wdRenderWorld::GetFrameCounter();

  // Determine visible objects
  FindVisibleObjects(view);

  // Extract and sort data
  auto& data = m_Data[wdRenderWorld::GetDataIndexForExtraction()];

  // Usually clear is not needed, only if the multithreading flag is switched during runtime.
  data.Clear();

  // Store camera and viewdata
  data.SetCamera(*view.GetCamera());
  data.SetLodCamera(*view.GetLodCamera());
  data.SetViewData(view.GetData());
  data.SetWorldTime(view.GetWorld()->GetClock().GetAccumulatedTime());
  data.SetWorldDebugContext(view.GetWorld());
  data.SetViewDebugContext(view.GetHandle());

  // Extract object render data
  for (auto& pExtractor : m_Extractors)
  {
    if (pExtractor->m_bActive)
    {
      WD_PROFILE_SCOPE(pExtractor->m_sName.GetData());

      pExtractor->Extract(view, m_VisibleObjects, data);
    }
  }

  data.SortAndBatch();

  for (auto& pExtractor : m_Extractors)
  {
    if (pExtractor->m_bActive)
    {
      WD_PROFILE_SCOPE(pExtractor->m_sName.GetData());

      pExtractor->PostSortAndBatch(view, m_VisibleObjects, data);
    }
  }

  m_CurrentExtractThread = (wdThreadID)0;
}

wdUniquePtr<wdRasterizerViewPool> g_pRasterizerViewPool;

// clang-format off
WD_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, SwRasterizer)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    g_pRasterizerViewPool = WD_DEFAULT_NEW(wdRasterizerViewPool);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    g_pRasterizerViewPool.Clear();
  }

WD_END_SUBSYSTEM_DECLARATION;
// clang-format on

void wdRenderPipeline::FindVisibleObjects(const wdView& view)
{
  WD_PROFILE_SCOPE("Visibility Culling");

  wdFrustum frustum;
  view.ComputeCullingFrustum(frustum);

  WD_LOCK(view.GetWorld()->GetReadMarker());

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  const bool bIsMainView = (view.GetCameraUsageHint() == wdCameraUsageHint::MainView || view.GetCameraUsageHint() == wdCameraUsageHint::EditorView);
  const bool bRecordStats = cvar_SpatialCullingShowStats && bIsMainView;
  wdSpatialSystem::QueryStats stats;
#endif

  wdSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = wdDefaultSpatialDataCategories::RenderStatic.GetBitmask() | wdDefaultSpatialDataCategories::RenderDynamic.GetBitmask();
  queryParams.m_IncludeTags = view.m_IncludeTags;
  queryParams.m_ExcludeTags = view.m_ExcludeTags;
#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  queryParams.m_pStats = bRecordStats ? &stats : nullptr;
#endif

  wdFrustum limitedFrustum = frustum;
  const wdPlane farPlane = limitedFrustum.GetPlane(wdFrustum::PlaneType::FarPlane);
  limitedFrustum.AccessPlane(wdFrustum::PlaneType::FarPlane).SetFromNormalAndPoint(farPlane.m_vNormal, view.GetCullingCamera()->GetCenterPosition() + farPlane.m_vNormal * cvar_SpatialCullingOcclusionFarPlane.GetValue()); // only use occluders closer than this

  wdRasterizerView* pRasterizer = PrepareOcclusionCulling(limitedFrustum, view);
  WD_SCOPE_EXIT(g_pRasterizerViewPool->ReturnRasterizerView(pRasterizer));

  const wdVisibilityState visType = bIsMainView ? wdVisibilityState::Direct : wdVisibilityState::Indirect;

  if (pRasterizer != nullptr && pRasterizer->HasRasterizedAnyOccluders())
  {
    WD_PROFILE_SCOPE("Occlusion::FindVisibleObjects");

    auto IsOccluded = [=](const wdSimdBBox& aabb) {
      // grow the bbox by some percent to counter the lower precision of the occlusion buffer

      wdSimdBBox aabb2;
      const wdSimdVec4f c = aabb.GetCenter();
      const wdSimdVec4f e = aabb.GetHalfExtents();
      aabb2.SetCenterAndHalfExtents(c, e.CompMul(wdSimdVec4f(1.0f + cvar_SpatialCullingOcclusionBoundsInlation)));

      return !pRasterizer->IsVisible(aabb2);
    };

    m_VisibleObjects.Clear();
    view.GetWorld()->GetSpatialSystem()->FindVisibleObjects(frustum, queryParams, m_VisibleObjects, IsOccluded, visType);
  }
  else
  {
    m_VisibleObjects.Clear();
    view.GetWorld()->GetSpatialSystem()->FindVisibleObjects(frustum, queryParams, m_VisibleObjects, {}, visType);
  }

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  if (pRasterizer)
  {
    if (view.GetCameraUsageHint() == wdCameraUsageHint::EditorView || view.GetCameraUsageHint() == wdCameraUsageHint::MainView)
    {
      PreviewOcclusionBuffer(*pRasterizer, view);
    }
  }
#endif

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  wdViewHandle hView = view.GetHandle();

  if (cvar_SpatialCullingVis && bIsMainView)
  {
    wdDebugRenderer::DrawLineFrustum(view.GetWorld(), frustum, wdColor::LimeGreen, false);
  }

  if (bRecordStats)
  {
    wdStringBuilder sb;

    wdDebugRenderer::DrawInfoText(hView, wdDebugRenderer::ScreenPlacement::TopLeft, "VisCulling", "Visibility Culling Stats", wdColor::LimeGreen);

    sb.Format("Total Num Objects: {0}", stats.m_uiTotalNumObjects);
    wdDebugRenderer::DrawInfoText(hView, wdDebugRenderer::ScreenPlacement::TopLeft, "VisCulling", sb, wdColor::LimeGreen);

    sb.Format("Num Objects Tested: {0}", stats.m_uiNumObjectsTested);
    wdDebugRenderer::DrawInfoText(hView, wdDebugRenderer::ScreenPlacement::TopLeft, "VisCulling", sb, wdColor::LimeGreen);

    sb.Format("Num Objects Passed: {0}", stats.m_uiNumObjectsPassed);
    wdDebugRenderer::DrawInfoText(hView, wdDebugRenderer::ScreenPlacement::TopLeft, "VisCulling", sb, wdColor::LimeGreen);

    // Exponential moving average for better readability.
    m_AverageCullingTime = wdMath::Lerp(m_AverageCullingTime, stats.m_TimeTaken, 0.05f);

    sb.Format("Time Taken: {0}ms", m_AverageCullingTime.GetMilliseconds());
    wdDebugRenderer::DrawInfoText(hView, wdDebugRenderer::ScreenPlacement::TopLeft, "VisCulling", sb, wdColor::LimeGreen);

    view.GetWorld()->GetSpatialSystem()->GetInternalStats(sb);
    wdDebugRenderer::DrawInfoText(hView, wdDebugRenderer::ScreenPlacement::TopLeft, "VisCulling", sb, wdColor::AntiqueWhite);
  }
#endif
}

void wdRenderPipeline::Render(wdRenderContext* pRenderContext)
{
  // WD_PROFILE_AND_MARKER(pRenderContext->GetGALContext(), m_sName.GetData());
  WD_PROFILE_SCOPE(m_sName.GetData());

  WD_ASSERT_DEV(m_PipelineState != PipelineState::Uninitialized, "Pipeline must be rebuild before rendering.");
  if (m_PipelineState == PipelineState::RebuildError)
  {
    return;
  }

  WD_ASSERT_DEV(m_CurrentRenderThread == (wdThreadID)0, "Render must not be called from multiple threads.");
  m_CurrentRenderThread = wdThreadUtils::GetCurrentThreadID();

  WD_ASSERT_DEV(m_uiLastRenderFrame != wdRenderWorld::GetFrameCounter(), "Render must not be called multiple times per frame.");
  m_uiLastRenderFrame = wdRenderWorld::GetFrameCounter();


  auto& data = m_Data[wdRenderWorld::GetDataIndexForRendering()];
  const wdCamera* pCamera = &data.GetCamera();
  const wdCamera* pLodCamera = &data.GetLodCamera();
  const wdViewData* pViewData = &data.GetViewData();

  auto& gc = pRenderContext->WriteGlobalConstants();
  for (int i = 0; i < 2; ++i)
  {
    gc.CameraToScreenMatrix[i] = pViewData->m_ProjectionMatrix[i];
    gc.ScreenToCameraMatrix[i] = pViewData->m_InverseProjectionMatrix[i];
    gc.WorldToCameraMatrix[i] = pViewData->m_ViewMatrix[i];
    gc.CameraToWorldMatrix[i] = pViewData->m_InverseViewMatrix[i];
    gc.WorldToScreenMatrix[i] = pViewData->m_ViewProjectionMatrix[i];
    gc.ScreenToWorldMatrix[i] = pViewData->m_InverseViewProjectionMatrix[i];
  }

  const wdRectFloat& viewport = pViewData->m_ViewPortRect;
  gc.ViewportSize = wdVec4(viewport.width, viewport.height, 1.0f / viewport.width, 1.0f / viewport.height);

  float fNear = pCamera->GetNearPlane();
  float fFar = pCamera->GetFarPlane();
  gc.ClipPlanes = wdVec4(fNear, fFar, 1.0f / fFar, 0.0f);

  const bool bIsDirectionalLightShadow = pViewData->m_CameraUsageHint == wdCameraUsageHint::Shadow && pCamera->IsOrthographic();
  gc.MaxZValue = bIsDirectionalLightShadow ? 0.0f : wdMath::MinValue<float>();

  // Wrap around to prevent floating point issues. Wrap around is dividable by all whole numbers up to 11.
  gc.DeltaTime = (float)wdClock::GetGlobalClock()->GetTimeDiff().GetSeconds();
  gc.GlobalTime = (float)wdMath::Mod(wdClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds(), 20790.0);
  gc.WorldTime = (float)wdMath::Mod(data.GetWorldTime().GetSeconds(), 20790.0);

  gc.Exposure = pCamera->GetExposure();
  gc.RenderPass = wdViewRenderMode::GetRenderPassForShader(pViewData->m_ViewRenderMode);

  wdRenderViewContext renderViewContext;
  renderViewContext.m_pCamera = pCamera;
  renderViewContext.m_pLodCamera = pLodCamera;
  renderViewContext.m_pViewData = pViewData;
  renderViewContext.m_pRenderContext = pRenderContext;
  renderViewContext.m_pWorldDebugContext = &data.GetWorldDebugContext();
  renderViewContext.m_pViewDebugContext = &data.GetViewDebugContext();

  // Set camera mode permutation variable here since it doesn't change throughout the frame
  static wdHashedString sCameraMode = wdMakeHashedString("CAMERA_MODE");
  static wdHashedString sOrtho = wdMakeHashedString("CAMERA_MODE_ORTHO");
  static wdHashedString sPerspective = wdMakeHashedString("CAMERA_MODE_PERSPECTIVE");
  static wdHashedString sStereo = wdMakeHashedString("CAMERA_MODE_STEREO");

  static wdHashedString sVSRTAI = wdMakeHashedString("VERTEX_SHADER_RENDER_TARGET_ARRAY_INDEX");
  static wdHashedString sClipSpaceFlipped = wdMakeHashedString("CLIP_SPACE_FLIPPED");
  static wdHashedString sTrue = wdMakeHashedString("TRUE");
  static wdHashedString sFalse = wdMakeHashedString("FALSE");

  if (pCamera->IsOrthographic())
    pRenderContext->SetShaderPermutationVariable(sCameraMode, sOrtho);
  else if (pCamera->IsStereoscopic())
    pRenderContext->SetShaderPermutationVariable(sCameraMode, sStereo);
  else
    pRenderContext->SetShaderPermutationVariable(sCameraMode, sPerspective);

  if (wdGALDevice::GetDefaultDevice()->GetCapabilities().m_bVertexShaderRenderTargetArrayIndex)
    pRenderContext->SetShaderPermutationVariable(sVSRTAI, sTrue);
  else
    pRenderContext->SetShaderPermutationVariable(sVSRTAI, sFalse);

  pRenderContext->SetShaderPermutationVariable(sClipSpaceFlipped, wdClipSpaceYMode::RenderToTextureDefault == wdClipSpaceYMode::Flipped ? sTrue : sFalse);

  // Also set pipeline specific permutation vars
  for (auto& var : m_PermutationVars)
  {
    pRenderContext->SetShaderPermutationVariable(var.m_sName, var.m_sValue);
  }

  wdRenderWorldRenderEvent renderEvent;
  renderEvent.m_Type = wdRenderWorldRenderEvent::Type::BeforePipelineExecution;
  renderEvent.m_pPipeline = this;
  renderEvent.m_pRenderViewContext = &renderViewContext;
  renderEvent.m_uiFrameCounter = wdRenderWorld::GetFrameCounter();
  {
    WD_PROFILE_SCOPE("BeforePipelineExecution");
    wdRenderWorld::s_RenderEvent.Broadcast(renderEvent);
  }

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  pDevice->BeginPipeline(m_sName, renderViewContext.m_pViewData->m_hSwapChain);

  if (const wdGALSwapChain* pSwapChain = pDevice->GetSwapChain(renderViewContext.m_pViewData->m_hSwapChain))
  {
    const wdGALRenderTargets& renderTargets = pSwapChain->GetRenderTargets();
    // Update target textures after the swap chain acquired new textures.
    for (wdUInt32 i = 0; i < m_TextureUsage.GetCount(); i++)
    {
      TextureUsageData& textureUsageData = m_TextureUsage[i];
      if (textureUsageData.m_iTargetTextureIndex != -1)
      {
        wdGALTextureHandle hTexture = reinterpret_cast<const wdGALTextureHandle*>(&renderTargets)[textureUsageData.m_iTargetTextureIndex];
        for (auto pUsedByConn : textureUsageData.m_UsedBy)
        {
          pUsedByConn->m_TextureHandle = hTexture;
        }
      }
    }
  }

  wdUInt32 uiCurrentFirstUsageIdx = 0;
  wdUInt32 uiCurrentLastUsageIdx = 0;
  for (wdUInt32 i = 0; i < m_Passes.GetCount(); ++i)
  {
    auto& pPass = m_Passes[i];
    WD_PROFILE_SCOPE(pPass->GetName());
    wdLogBlock passBlock("Render Pass", pPass->GetName());

    // Create pool textures
    for (; uiCurrentFirstUsageIdx < m_TextureUsageIdxSortedByFirstUsage.GetCount();)
    {
      wdUInt16 uiCurrentUsageData = m_TextureUsageIdxSortedByFirstUsage[uiCurrentFirstUsageIdx];
      TextureUsageData& usageData = m_TextureUsage[uiCurrentUsageData];
      if (usageData.m_uiFirstUsageIdx == i)
      {
        wdGALTextureHandle hTexture = wdGPUResourcePool::GetDefaultInstance()->GetRenderTarget(usageData.m_UsedBy[0]->m_Desc);
        WD_ASSERT_DEV(!hTexture.IsInvalidated(), "GPU pool returned an invalidated texture!");
        for (wdRenderPipelinePassConnection* pConn : usageData.m_UsedBy)
        {
          pConn->m_TextureHandle = hTexture;
        }
        ++uiCurrentFirstUsageIdx;
      }
      else
      {
        // The current usage data blocks m_uiFirstUsageIdx isn't reached yet so wait.
        break;
      }
    }

    // Execute pass block
    {
      ConnectionData& connectionData = m_Connections[pPass.Borrow()];
      if (pPass->m_bActive)
      {
        pPass->Execute(renderViewContext, connectionData.m_Inputs, connectionData.m_Outputs);
      }
      else
      {
        pPass->ExecuteInactive(renderViewContext, connectionData.m_Inputs, connectionData.m_Outputs);
      }
    }

    // Release pool textures
    for (; uiCurrentLastUsageIdx < m_TextureUsageIdxSortedByLastUsage.GetCount();)
    {
      wdUInt16 uiCurrentUsageData = m_TextureUsageIdxSortedByLastUsage[uiCurrentLastUsageIdx];
      TextureUsageData& usageData = m_TextureUsage[uiCurrentUsageData];
      if (usageData.m_uiLastUsageIdx == i)
      {
        wdGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(usageData.m_UsedBy[0]->m_TextureHandle);
        for (wdRenderPipelinePassConnection* pConn : usageData.m_UsedBy)
        {
          pConn->m_TextureHandle.Invalidate();
        }
        ++uiCurrentLastUsageIdx;
      }
      else
      {
        // The current usage data blocks m_uiLastUsageIdx isn't reached yet so wait.
        break;
      }
    }
  }
  WD_ASSERT_DEV(uiCurrentFirstUsageIdx == m_TextureUsageIdxSortedByFirstUsage.GetCount(), "Rendering all passes should have moved us through all texture usage blocks!");
  WD_ASSERT_DEV(uiCurrentLastUsageIdx == m_TextureUsageIdxSortedByLastUsage.GetCount(), "Rendering all passes should have moved us through all texture usage blocks!");

  pDevice->EndPipeline(renderViewContext.m_pViewData->m_hSwapChain);

  renderEvent.m_Type = wdRenderWorldRenderEvent::Type::AfterPipelineExecution;
  {
    WD_PROFILE_SCOPE("AfterPipelineExecution");
    wdRenderWorld::s_RenderEvent.Broadcast(renderEvent);
  }

  pRenderContext->ResetContextState();

  data.Clear();

  m_CurrentRenderThread = (wdThreadID)0;
}

const wdExtractedRenderData& wdRenderPipeline::GetRenderData() const
{
  return m_Data[wdRenderWorld::GetDataIndexForRendering()];
}

wdRenderDataBatchList wdRenderPipeline::GetRenderDataBatchesWithCategory(wdRenderData::Category category, wdRenderDataBatch::Filter filter) const
{
  auto& data = m_Data[wdRenderWorld::GetDataIndexForRendering()];
  return data.GetRenderDataBatchesWithCategory(category, filter);
}

void wdRenderPipeline::CreateDgmlGraph(wdDGMLGraph& ref_graph)
{
  wdStringBuilder sTmp;
  wdHashTable<const wdRenderPipelineNode*, wdUInt32> nodeMap;
  nodeMap.Reserve(m_Passes.GetCount() + m_TextureUsage.GetCount() * 3);
  for (wdUInt32 p = 0; p < m_Passes.GetCount(); ++p)
  {
    const auto& pPass = m_Passes[p];
    sTmp.Format("#{}: {}", p, wdStringUtils::IsNullOrEmpty(pPass->GetName()) ? pPass->GetDynamicRTTI()->GetTypeName() : pPass->GetName());

    wdDGMLGraph::NodeDesc nd;
    nd.m_Color = wdColor::Gray;
    nd.m_Shape = wdDGMLGraph::NodeShape::Rectangle;
    wdUInt32 uiGraphNode = ref_graph.AddNode(sTmp, &nd);
    nodeMap.Insert(pPass.Borrow(), uiGraphNode);
  }

  for (wdUInt32 i = 0; i < m_TextureUsage.GetCount(); ++i)
  {
    const TextureUsageData& data = m_TextureUsage[i];

    for (const wdRenderPipelinePassConnection* pCon : data.m_UsedBy)
    {
      wdDGMLGraph::NodeDesc nd;
      nd.m_Color = data.m_iTargetTextureIndex != -1 ? wdColor::Black : wdColorScheme::GetColor(static_cast<wdColorScheme::Enum>(i % wdColorScheme::Count), 4);
      nd.m_Shape = wdDGMLGraph::NodeShape::RoundedRectangle;

      wdStringBuilder sFormat;
      if (!wdReflectionUtils::EnumerationToString(wdGetStaticRTTI<wdGALResourceFormat>(), pCon->m_Desc.m_Format, sFormat, wdReflectionUtils::EnumConversionMode::ValueNameOnly))
      {
        sFormat.Format("Unknown Format {}", (int)pCon->m_Desc.m_Format);
      }
      sTmp.Format("{} #{}: {}x{}:{}, MSAA:{}, {}Format: {}", data.m_iTargetTextureIndex != -1 ? "RenderTarget" : "PoolTexture", i, pCon->m_Desc.m_uiWidth, pCon->m_Desc.m_uiHeight, pCon->m_Desc.m_uiArraySize, (int)pCon->m_Desc.m_SampleCount, wdGALResourceFormat::IsDepthFormat(pCon->m_Desc.m_Format) ? "Depth" : "Color", sFormat);
      wdUInt32 uiTextureNode = ref_graph.AddNode(sTmp, &nd);

      wdUInt32 uiOutputNode = *nodeMap.GetValue(pCon->m_pOutput->m_pParent);
      ref_graph.AddConnection(uiOutputNode, uiTextureNode, pCon->m_pOutput->m_pParent->GetPinName(pCon->m_pOutput));
      for (const wdRenderPipelineNodePin* pInput : pCon->m_Inputs)
      {
        wdUInt32 uiInputNode = *nodeMap.GetValue(pInput->m_pParent);
        ref_graph.AddConnection(uiTextureNode, uiInputNode, pInput->m_pParent->GetPinName(pInput));
      }
    }
  }
}

wdRasterizerView* wdRenderPipeline::PrepareOcclusionCulling(const wdFrustum& frustum, const wdView& view)
{
#if WD_ENABLED(WD_PLATFORM_ARCH_X86)
  if (!cvar_SpatialCullingOcclusionEnable)
    return nullptr;

  if (!wdSystemInformation::Get().GetCpuFeatures().IsAvx1Available())
    return nullptr;

  wdRasterizerView* pRasterizer = nullptr;

  // extract all occlusion geometry from the scene
  WD_PROFILE_SCOPE("Occlusion::RasterizeView");

  pRasterizer = g_pRasterizerViewPool->GetRasterizerView(static_cast<wdUInt32>(view.GetViewport().width / 2), static_cast<wdUInt32>(view.GetViewport().height / 2), (float)view.GetViewport().width / (float)view.GetViewport().height);
  pRasterizer->SetCamera(view.GetCullingCamera());

  {
    WD_PROFILE_SCOPE("Occlusion::FindOccluders");

    wdSpatialSystem::QueryParams queryParams;
    queryParams.m_uiCategoryBitmask = wdDefaultSpatialDataCategories::OcclusionStatic.GetBitmask() | wdDefaultSpatialDataCategories::OcclusionDynamic.GetBitmask();
    queryParams.m_IncludeTags = view.m_IncludeTags;
    queryParams.m_ExcludeTags = view.m_ExcludeTags;

    m_VisibleObjects.Clear();
    view.GetWorld()->GetSpatialSystem()->FindVisibleObjects(frustum, queryParams, m_VisibleObjects, {}, wdVisibilityState::Indirect);
  }

  pRasterizer->BeginScene();

  for (const wdGameObject* pObj : m_VisibleObjects)
  {
    wdMsgExtractOccluderData msg;
    pObj->SendMessage(msg);

    for (const auto& ed : msg.m_ExtractedOccluderData)
    {
      pRasterizer->AddObject(ed.m_pObject, ed.m_Transform);
    }
  }

  pRasterizer->EndScene();

  return pRasterizer;
#else
  return nullptr;
#endif
}

void wdRenderPipeline::PreviewOcclusionBuffer(const wdRasterizerView& rasterizer, const wdView& view)
{
  if (!cvar_SpatialCullingOcclusionVisView || !rasterizer.HasRasterizedAnyOccluders())
    return;

  WD_PROFILE_SCOPE("Occlusion::DebugPreview");

  const wdUInt32 uiImgWidth = rasterizer.GetResolutionX();
  const wdUInt32 uiImgHeight = rasterizer.GetResolutionY();

  // get the debug image from the rasterizer
  wdDynamicArray<wdColorLinearUB> fb;
  fb.SetCountUninitialized(uiImgWidth * uiImgHeight);
  rasterizer.ReadBackFrame(fb);

  const float w = (float)uiImgWidth;
  const float h = (float)uiImgHeight;
  wdRectFloat rectInPixel1 = wdRectFloat(5.0f, 5.0f, w + 10, h + 10);
  wdRectFloat rectInPixel2 = wdRectFloat(10.0f, 10.0f, w, h);

  wdDebugRenderer::Draw2DRectangle(view.GetHandle(), rectInPixel1, 0.0f, wdColor::MediumPurple);

  // TODO: it would be better to update a single texture every frame, however since this is a render pass,
  // we currently can't create nested passes
  // so either this has to be done elsewhere, or nested passes have to be allowed
  if (false)
  {
    wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

    // check whether we need to re-create the texture
    if (!m_hOcclusionDebugViewTexture.IsInvalidated())
    {
      const wdGALTexture* pTexture = pDevice->GetTexture(m_hOcclusionDebugViewTexture);

      if (pTexture->GetDescription().m_uiWidth != uiImgWidth ||
          pTexture->GetDescription().m_uiHeight != uiImgHeight)
      {
        pDevice->DestroyTexture(m_hOcclusionDebugViewTexture);
        m_hOcclusionDebugViewTexture.Invalidate();
      }
    }

    // create the texture
    if (m_hOcclusionDebugViewTexture.IsInvalidated())
    {
      wdGALTextureCreationDescription desc;
      desc.m_uiWidth = uiImgWidth;
      desc.m_uiHeight = uiImgHeight;
      desc.m_Format = wdGALResourceFormat::RGBAUByteNormalized;
      desc.m_ResourceAccess.m_bImmutable = false;

      m_hOcclusionDebugViewTexture = pDevice->CreateTexture(desc);
    }

    // upload the image to the texture
    {
      wdGALPass* pGALPass = pDevice->BeginPass("RasterizerDebugViewUpdate");
      auto pCommandEncoder = pGALPass->BeginCompute();

      wdBoundingBoxu32 destBox;
      destBox.m_vMin.SetZero();
      destBox.m_vMax = wdVec3U32(uiImgWidth, uiImgHeight, 1);

      wdGALSystemMemoryDescription sourceData;
      sourceData.m_pData = fb.GetData();
      sourceData.m_uiRowPitch = uiImgWidth * sizeof(wdColorLinearUB);

      pCommandEncoder->UpdateTexture(m_hOcclusionDebugViewTexture, wdGALTextureSubresource(), destBox, sourceData);

      pGALPass->EndCompute(pCommandEncoder);
      pDevice->EndPass(pGALPass);
    }

    wdDebugRenderer::Draw2DRectangle(view.GetHandle(), rectInPixel2, 0.0f, wdColor::White, pDevice->GetDefaultResourceView(m_hOcclusionDebugViewTexture), wdVec2(1, -1));
  }
  else
  {
    wdTexture2DResourceDescriptor d;
    d.m_DescGAL.m_uiWidth = rasterizer.GetResolutionX();
    d.m_DescGAL.m_uiHeight = rasterizer.GetResolutionY();
    d.m_DescGAL.m_Format = wdGALResourceFormat::RGBAByteNormalized;

    wdGALSystemMemoryDescription content[1];
    content[0].m_pData = fb.GetData();
    content[0].m_uiRowPitch = sizeof(wdColorLinearUB) * d.m_DescGAL.m_uiWidth;
    content[0].m_uiSlicePitch = content[0].m_uiRowPitch * d.m_DescGAL.m_uiHeight;
    d.m_InitialContent = content;

    static wdAtomicInteger32 name = 0;
    name.Increment();

    wdStringBuilder sName;
    sName.Format("RasterizerPreview-{}", name);

    wdTexture2DResourceHandle hDebug = wdResourceManager::CreateResource<wdTexture2DResource>(sName, std::move(d));

    wdDebugRenderer::Draw2DRectangle(view.GetHandle(), rectInPixel2, 0.0f, wdColor::White, hDebug, wdVec2(1, -1));
  }
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipeline);
