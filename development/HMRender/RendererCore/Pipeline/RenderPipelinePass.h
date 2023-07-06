#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/RenderPipelineNode.h>

struct wdGALTextureCreationDescription;

/// \brief Passed to wdRenderPipelinePass::InitRenderPipelinePass to inform about
/// existing connections on each input / output pin index.
struct wdRenderPipelinePassConnection
{
  wdRenderPipelinePassConnection() { m_pOutput = nullptr; }

  wdGALTextureCreationDescription m_Desc;
  wdGALTextureHandle m_TextureHandle;
  const wdRenderPipelineNodePin* m_pOutput;                  ///< The output pin that this connection spawns from.
  wdHybridArray<const wdRenderPipelineNodePin*, 4> m_Inputs; ///< The various input pins this connection is connected to.
};

class WD_RENDERERCORE_DLL wdRenderPipelinePass : public wdRenderPipelineNode
{
  WD_ADD_DYNAMIC_REFLECTION(wdRenderPipelinePass, wdRenderPipelineNode);
  WD_DISALLOW_COPY_AND_ASSIGN(wdRenderPipelinePass);

public:
  wdRenderPipelinePass(const char* szName, bool bIsStereoAware = false);
  ~wdRenderPipelinePass();

  /// \brief Sets the name of the pass.
  void SetName(const char* szName);

  /// \brief returns the name of the pass.
  const char* GetName() const;

  /// \brief True if the render pipeline pass can handle stereo cameras correctly.
  bool IsStereoAware() const { return m_bIsStereoAware; }

  /// \brief For a given input pin configuration, provide the output configuration of this node.
  /// Outputs is already resized to the number of output pins.
  virtual bool GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs) = 0;

  /// \brief After GetRenderTargetDescriptions was called successfully for each pass, this function is called
  /// with the inputs and outputs for review. Disconnected pins have a nullptr value in the passed in arrays.
  /// This is the time to create additional resources that are not covered by the pins automatically, e.g. a picking texture or eye
  /// adaptation buffer.
  virtual void InitRenderPipelinePass(const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs);

  /// \brief Render into outputs. Both inputs and outputs are passed in with actual texture handles.
  /// Disconnected pins have a nullptr value in the passed in arrays. You can now create views and render target setups on the fly and
  /// fill the output targets with data.
  virtual void Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) = 0;

  virtual void ExecuteInactive(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs);

  /// \brief Allows for the pass to write data back using wdView::SetRenderPassReadBackProperty. E.g. picking results etc.
  virtual void ReadBackProperties(wdView* pView);

  void RenderDataWithCategory(const wdRenderViewContext& renderViewContext, wdRenderData::Category category, wdRenderDataBatch::Filter filter = wdRenderDataBatch::Filter());

  WD_ALWAYS_INLINE wdRenderPipeline* GetPipeline() { return m_pPipeline; }
  WD_ALWAYS_INLINE const wdRenderPipeline* GetPipeline() const { return m_pPipeline; }

private:
  friend class wdRenderPipeline;

  bool m_bActive = true;

  const bool m_bIsStereoAware;
  wdHashedString m_sName;

  wdRenderPipeline* m_pPipeline = nullptr;
};
