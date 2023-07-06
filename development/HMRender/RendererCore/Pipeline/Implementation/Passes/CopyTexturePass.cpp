#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/CopyTexturePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdCopyTexturePass, 1, wdRTTIDefaultAllocator<wdCopyTexturePass>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("Input", m_PinInput),
    WD_MEMBER_PROPERTY("Output", m_PinOutput)
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdCopyTexturePass::wdCopyTexturePass()
  : wdRenderPipelinePass("CopyTexturePass")
{
}

wdCopyTexturePass::~wdCopyTexturePass() = default;

bool wdCopyTexturePass::GetRenderTargetDescriptions(
  const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs)
{
  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  const wdGALTextureCreationDescription* pInput = inputs[m_PinInput.m_uiInputIndex];

  if (pInput != nullptr)
  {
    wdGALTextureCreationDescription desc = *pInput;

    outputs[m_PinOutput.m_uiOutputIndex] = desc;
  }
  else
  {
    wdLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void wdCopyTexturePass::Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs)
{
  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];

  if (pInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  const wdGALTexture* pDest = pDevice->GetTexture(pOutput->m_TextureHandle);
  const wdGALTexture* pSource = pDevice->GetTexture(pInput->m_TextureHandle);

  if (pDest->GetDescription().m_Format != pSource->GetDescription().m_Format)
  {
    // TODO: use a shader when the format doesn't match exactly

    wdLog::Error("Copying textures of different formats is not implemented");
  }
  else
  {
    auto pCommandEncoder = wdRenderContext::BeginPassAndComputeScope(renderViewContext, GetName());

    pCommandEncoder->CopyTexture(pOutput->m_TextureHandle, pInput->m_TextureHandle);
  }
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_CopyTexturePass);
