#pragma once

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>

class wdProfilingId;
class wdView;
class wdRenderPipelinePass;
class wdFrameDataProviderBase;
struct wdPermutationVar;
class wdDGMLGraph;
class wdFrustum;
class wdRasterizerView;

class WD_RENDERERCORE_DLL wdRenderPipeline : public wdRefCounted
{
public:
  enum class PipelineState
  {
    Uninitialized,
    RebuildError,
    Initialized
  };

  wdRenderPipeline();
  ~wdRenderPipeline();

  void AddPass(wdUniquePtr<wdRenderPipelinePass>&& pPass);
  void RemovePass(wdRenderPipelinePass* pPass);
  void GetPasses(wdHybridArray<const wdRenderPipelinePass*, 16>& ref_passes) const;
  void GetPasses(wdHybridArray<wdRenderPipelinePass*, 16>& ref_passes);
  wdRenderPipelinePass* GetPassByName(const wdStringView& sPassName);
  wdHashedString GetViewName() const;

  bool Connect(wdRenderPipelinePass* pOutputNode, const char* szOutputPinName, wdRenderPipelinePass* pInputNode, const char* szInputPinName);
  bool Connect(wdRenderPipelinePass* pOutputNode, wdHashedString sOutputPinName, wdRenderPipelinePass* pInputNode, wdHashedString sInputPinName);
  bool Disconnect(wdRenderPipelinePass* pOutputNode, wdHashedString sOutputPinName, wdRenderPipelinePass* pInputNode, wdHashedString sInputPinName);

  const wdRenderPipelinePassConnection* GetInputConnection(wdRenderPipelinePass* pPass, wdHashedString sInputPinName) const;
  const wdRenderPipelinePassConnection* GetOutputConnection(wdRenderPipelinePass* pPass, wdHashedString sOutputPinName) const;

  void AddExtractor(wdUniquePtr<wdExtractor>&& pExtractor);
  void RemoveExtractor(wdExtractor* pExtractor);
  void GetExtractors(wdHybridArray<const wdExtractor*, 16>& ref_extractors) const;
  void GetExtractors(wdHybridArray<wdExtractor*, 16>& ref_extractors);
  wdExtractor* GetExtractorByName(const wdStringView& sExtractorName);

  template <typename T>
  WD_ALWAYS_INLINE T* GetFrameDataProvider() const
  {
    return static_cast<T*>(GetFrameDataProvider(wdGetStaticRTTI<T>()));
  }

  const wdExtractedRenderData& GetRenderData() const;
  wdRenderDataBatchList GetRenderDataBatchesWithCategory(
    wdRenderData::Category category, wdRenderDataBatch::Filter filter = wdRenderDataBatch::Filter()) const;

  /// \brief Creates a DGML graph of all passes and textures. Can be used to verify that no accidental temp textures are created due to poorly constructed pipelines or errors in code.
  void CreateDgmlGraph(wdDGMLGraph& ref_graph);

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  static wdCVarBool cvar_SpatialCullingVis;
#endif

  WD_DISALLOW_COPY_AND_ASSIGN(wdRenderPipeline);

private:
  friend class wdRenderWorld;
  friend class wdView;

  // \brief Rebuilds the render pipeline, e.g. sorting passes via dependencies and creating render targets.
  PipelineState Rebuild(const wdView& view);
  bool RebuildInternal(const wdView& view);
  bool SortPasses();
  bool InitRenderTargetDescriptions(const wdView& view);
  bool CreateRenderTargetUsage(const wdView& view);
  bool InitRenderPipelinePasses();
  void SortExtractors();
  void UpdateViewData(const wdView& view, wdUInt32 uiDataIndex);

  void RemoveConnections(wdRenderPipelinePass* pPass);
  void ClearRenderPassGraphTextures();
  bool AreInputDescriptionsAvailable(const wdRenderPipelinePass* pPass, const wdHybridArray<wdRenderPipelinePass*, 32>& done) const;
  bool ArePassThroughInputsDone(const wdRenderPipelinePass* pPass, const wdHybridArray<wdRenderPipelinePass*, 32>& done) const;

  wdFrameDataProviderBase* GetFrameDataProvider(const wdRTTI* pRtti) const;

  void ExtractData(const wdView& view);
  void FindVisibleObjects(const wdView& view);

  void Render(wdRenderContext* pRenderer);

  wdRasterizerView* PrepareOcclusionCulling(const wdFrustum& frustum, const wdView& view);
  void PreviewOcclusionBuffer(const wdRasterizerView& rasterizer, const wdView& view);

private: // Member data
  // Thread data
  wdThreadID m_CurrentExtractThread;
  wdThreadID m_CurrentRenderThread;

  // Pipeline render data
  wdExtractedRenderData m_Data[2];
  wdDynamicArray<const wdGameObject*> m_VisibleObjects;

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  wdTime m_AverageCullingTime;
#endif

  wdHashedString m_sName;
  wdUInt64 m_uiLastExtractionFrame;
  wdUInt64 m_uiLastRenderFrame;

  // Render pass graph data
  PipelineState m_PipelineState = PipelineState::Uninitialized;

  struct ConnectionData
  {
    // Inputs / outputs match the node pin indices. Value at index is nullptr if not connected.
    wdDynamicArray<wdRenderPipelinePassConnection*> m_Inputs;
    wdDynamicArray<wdRenderPipelinePassConnection*> m_Outputs;
  };
  wdDynamicArray<wdUniquePtr<wdRenderPipelinePass>> m_Passes;
  wdMap<const wdRenderPipelinePass*, ConnectionData> m_Connections;

  /// \brief Contains all connections that share the same path-through texture and their first and last usage pass index.
  struct TextureUsageData
  {
    wdHybridArray<wdRenderPipelinePassConnection*, 4> m_UsedBy;
    wdUInt16 m_uiFirstUsageIdx;
    wdUInt16 m_uiLastUsageIdx;
    wdInt32 m_iTargetTextureIndex = -1;
  };
  wdDynamicArray<TextureUsageData> m_TextureUsage;
  wdDynamicArray<wdUInt16> m_TextureUsageIdxSortedByFirstUsage; ///< Indices map into m_TextureUsage
  wdDynamicArray<wdUInt16> m_TextureUsageIdxSortedByLastUsage;  ///< Indices map into m_TextureUsage

  wdHashTable<wdRenderPipelinePassConnection*, wdUInt32> m_ConnectionToTextureIndex;

  // Extractors
  wdDynamicArray<wdUniquePtr<wdExtractor>> m_Extractors;
  wdDynamicArray<wdUniquePtr<wdExtractor>> m_SortedExtractors;

  // Data Providers
  mutable wdDynamicArray<wdUniquePtr<wdFrameDataProviderBase>> m_DataProviders;
  mutable wdHashTable<const wdRTTI*, wdUInt32> m_TypeToDataProviderIndex;

  wdDynamicArray<wdPermutationVar> m_PermutationVars;

  // Occlusion Culling
  wdGALTextureHandle m_hOcclusionDebugViewTexture;
};
