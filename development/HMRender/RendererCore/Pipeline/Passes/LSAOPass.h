#pragma once

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/LSAOConstants.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// \brief Defines the depth compare function to be used to decide sample weights.
struct WD_RENDERERCORE_DLL wdLSAODepthCompareFunction
{
  using StorageType = wdUInt8;

  enum Enum
  {
    Depth,                   ///< A hard cutoff function between the linear depth values. Samples with an absolute distance greater than
                             ///< wdLSAOPass::SetDepthCutoffDistance are ignored.
    Normal,                  ///< Samples that are on the same plane as constructed by the center position and normal will be weighted higher than those samples that
                             ///< are above or below the plane.
    NormalAndSampleDistance, ///< Same as Normal, but if two samples are tested, their distance to the center position is is inversely multiplied as
                             ///< well, giving closer matches a higher weight.
    Default = NormalAndSampleDistance
  };
};

WD_DECLARE_REFLECTABLE_TYPE(WD_RENDERERCORE_DLL, wdLSAODepthCompareFunction);

/// Screen space ambient occlusion using "line sweep ambient occlusion" by Ville Timonen
///
/// Resources:
/// Use in Quantum Break: http://wili.cc/research/quantum_break/SIGGRAPH_2015_Remedy_Notes.pdf
/// Presentation slides EGSR: http://wili.cc/research/lsao/EGSR13_LSAO.pdf
/// Paper: http://wili.cc/research/lsao/lsao.pdf
///
/// There are a few adjustments and own ideas worked into this implementation.
/// The biggest change probably is that pixels in the gather pass compute their target linesample arithmetically instead of relying on lookups.
class WD_RENDERERCORE_DLL wdLSAOPass : public wdRenderPipelinePass
{
  WD_ADD_DYNAMIC_REFLECTION(wdLSAOPass, wdRenderPipelinePass);

public:
  wdLSAOPass();
  ~wdLSAOPass();

  virtual bool GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs) override;
  virtual void InitRenderPipelinePass(const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) override;

  virtual void Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) override;
  virtual void ExecuteInactive(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) override;

  wdUInt32 GetLineToLinePixelOffset() const { return m_iLineToLinePixelOffset; }
  void SetLineToLinePixelOffset(wdUInt32 uiPixelOffset);
  wdUInt32 GetLineSamplePixelOffset() const { return m_iLineSamplePixelOffsetFactor; }
  void SetLineSamplePixelOffset(wdUInt32 uiPixelOffset);

  // Factor used for depth cutoffs (determines when a depth difference is too large to be considered)
  float GetDepthCutoffDistance() const;
  void SetDepthCutoffDistance(float fDepthCutoffDistance);

  // Determines how quickly the occlusion falls of.
  float GetOcclusionFalloff() const;
  void SetOcclusionFalloff(float fFalloff);


protected:
  /// Destroys all GPU data that might have been created in in SetupLineSweepData
  void DestroyLineSweepData();
  void SetupLineSweepData(const wdVec3I32& imageResolution);


  void AddLinesForDirection(const wdVec3I32& imageResolution, const wdVec2I32& sampleDir, wdUInt32 lineIndex, wdDynamicArray<LineInstruction>& outinLineInstructions, wdUInt32& outinTotalNumberOfSamples);

  wdRenderPipelineNodeInputPin m_PinDepthInput;
  wdRenderPipelineNodeOutputPin m_PinOutput;

  wdConstantBufferStorageHandle m_hLineSweepCB;

  bool m_bSweepDataDirty = true;

  /// Output of the line sweep pass.
  wdGALBufferHandle m_hLineSweepOutputBuffer;
  wdGALUnorderedAccessViewHandle m_hLineSweepOutputUAV;
  wdGALResourceViewHandle m_hLineSweepOutputSRV;

  /// Structured buffer containing instructions for every single line to trace.
  wdGALBufferHandle m_hLineInfoBuffer;
  wdGALResourceViewHandle m_hLineSweepInfoSRV;

  /// Total number of lines to be traced.
  wdUInt32 m_uiNumSweepLines = 0;

  wdInt32 m_iLineToLinePixelOffset = 2;
  wdInt32 m_iLineSamplePixelOffsetFactor = 1;
  wdEnum<wdLSAODepthCompareFunction> m_DepthCompareFunction;
  bool m_bDistributedGathering = true;

  wdShaderResourceHandle m_hShaderLineSweep;
  wdShaderResourceHandle m_hShaderGather;
  wdShaderResourceHandle m_hShaderAverage;
};
