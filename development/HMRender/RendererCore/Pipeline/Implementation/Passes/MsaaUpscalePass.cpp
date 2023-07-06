#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/MsaaUpscalePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdMsaaUpscalePass, 2, wdRTTIDefaultAllocator<wdMsaaUpscalePass>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("Input", m_PinInput),
    WD_MEMBER_PROPERTY("Output", m_PinOutput),
    WD_ENUM_MEMBER_PROPERTY("MSAA_Mode", wdGALMSAASampleCount, m_MsaaMode)
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdMsaaUpscalePass::wdMsaaUpscalePass()
  : wdRenderPipelinePass("MsaaUpscalePass")

{
  {
    // Load shader.
    m_hShader = wdResourceManager::LoadResource<wdShaderResource>("Shaders/Pipeline/MsaaUpscale.wdShader");
    WD_ASSERT_DEV(m_hShader.IsValid(), "Could not load msaa upscale shader!");
  }
}

wdMsaaUpscalePass::~wdMsaaUpscalePass() = default;

bool wdMsaaUpscalePass::GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs)
{
  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  if (pInput != nullptr)
  {
    if (pInput->m_SampleCount != wdGALMSAASampleCount::None)
    {
      wdLog::Error("Input must not be a msaa target");
      return false;
    }

    wdGALTextureCreationDescription desc = *pInput;
    desc.m_SampleCount = m_MsaaMode;

    outputs[m_PinOutput.m_uiOutputIndex] = desc;
  }
  else
  {
    wdLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void wdMsaaUpscalePass::Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs)
{
  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  // Setup render target
  wdGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));

  // Bind render target and viewport
  auto pCommandEncoder = wdRenderContext::BeginPassAndRenderingScope(renderViewContext, std::move(renderingSetup), GetName(), renderViewContext.m_pCamera->IsStereoscopic());

  renderViewContext.m_pRenderContext->BindShader(m_hShader);
  renderViewContext.m_pRenderContext->BindMeshBuffer(wdGALBufferHandle(), wdGALBufferHandle(), nullptr, wdGALPrimitiveTopology::Triangles, 1);
  renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", pDevice->GetDefaultResourceView(pInput->m_TextureHandle));

  renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
}



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class wdMsaaUpscalePassPatch_1_2 : public wdGraphPatch
{
public:
  wdMsaaUpscalePassPatch_1_2()
    : wdGraphPatch("wdMsaaUpscalePass", 2)
  {
  }

  virtual void Patch(wdGraphPatchContext& ref_context, wdAbstractObjectGraph* pGraph, wdAbstractObjectNode* pNode) const override { pNode->RenameProperty("MSAA Mode", "MSAA_Mode"); }
};

wdMsaaUpscalePassPatch_1_2 g_wdMsaaUpscalePassPatch_1_2;



WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_MsaaUpscalePass);
