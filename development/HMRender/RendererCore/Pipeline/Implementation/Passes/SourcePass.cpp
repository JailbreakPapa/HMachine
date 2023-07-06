#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <RendererCore/Pipeline/Passes/SourcePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdSourcePass, 2, wdRTTIDefaultAllocator<wdSourcePass>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("Output", m_PinOutput),
    WD_ENUM_MEMBER_PROPERTY("Format", wdGALResourceFormat, m_Format),
    WD_ENUM_MEMBER_PROPERTY("MSAA_Mode", wdGALMSAASampleCount, m_MsaaMode),
    WD_MEMBER_PROPERTY("ClearColor", m_ClearColor)->AddAttributes(new wdExposeColorAlphaAttribute()),
    WD_MEMBER_PROPERTY("Clear", m_bClear),
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdSourcePass::wdSourcePass(const char* szName)
  : wdRenderPipelinePass(szName, true)
{
  m_Format = wdGALResourceFormat::RGBAUByteNormalizedsRGB;
  m_MsaaMode = wdGALMSAASampleCount::None;
  m_bClear = true;
  m_ClearColor = wdColor::Black;
}

wdSourcePass::~wdSourcePass() = default;

bool wdSourcePass::GetRenderTargetDescriptions(
  const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs)
{
  wdUInt32 uiWidth = static_cast<wdUInt32>(view.GetViewport().width);
  wdUInt32 uiHeight = static_cast<wdUInt32>(view.GetViewport().height);

  wdGALTextureCreationDescription desc;
  desc.m_uiWidth = uiWidth;
  desc.m_uiHeight = uiHeight;
  desc.m_SampleCount = m_MsaaMode;
  desc.m_Format = m_Format;
  desc.m_bCreateRenderTarget = true;
  desc.m_uiArraySize = view.GetCamera()->IsStereoscopic() ? 2 : 1;

  outputs[m_PinOutput.m_uiOutputIndex] = desc;

  return true;
}

void wdSourcePass::Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs,
  const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs)
{
  if (!m_bClear)
    return;

  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr)
  {
    return;
  }

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  // Setup render target
  wdGALRenderingSetup renderingSetup;
  renderingSetup.m_ClearColor = m_ClearColor;
  renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
  renderingSetup.m_bClearDepth = true;
  renderingSetup.m_bClearStencil = true;

  if (wdGALResourceFormat::IsDepthFormat(m_Format))
  {
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
  }
  else
  {
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
  }

  auto pCommandEncoder = wdRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, GetName());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class wdSourcePassPatch_1_2 : public wdGraphPatch
{
public:
  wdSourcePassPatch_1_2()
    : wdGraphPatch("wdSourcePass", 2)
  {
  }

  virtual void Patch(wdGraphPatchContext& ref_context, wdAbstractObjectGraph* pGraph, wdAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("MSAA Mode", "MSAA_Mode");
    pNode->RenameProperty("Clear Color", "ClearColor");
  }
};

wdSourcePassPatch_1_2 g_wdSourcePassPatch_1_2;



WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SourcePass);
