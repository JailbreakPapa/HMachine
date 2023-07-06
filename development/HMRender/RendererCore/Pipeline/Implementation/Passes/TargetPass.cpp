#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/TargetPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetView.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdTargetPass, 1, wdRTTIDefaultAllocator<wdTargetPass>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("Color0", m_PinColor0),
    WD_MEMBER_PROPERTY("Color1", m_PinColor1),
    WD_MEMBER_PROPERTY("Color2", m_PinColor2),
    WD_MEMBER_PROPERTY("Color3", m_PinColor3),
    WD_MEMBER_PROPERTY("Color4", m_PinColor4),
    WD_MEMBER_PROPERTY("Color5", m_PinColor5),
    WD_MEMBER_PROPERTY("Color6", m_PinColor6),
    WD_MEMBER_PROPERTY("Color7", m_PinColor7),
    WD_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdTargetPass::wdTargetPass(const char* szName)
  : wdRenderPipelinePass(szName, true)
{
}

wdTargetPass::~wdTargetPass() = default;

const wdGALTextureHandle* wdTargetPass::GetTextureHandle(const wdGALRenderTargets& renderTargets, const wdRenderPipelineNodePin* pPin)
{
  // auto inputs = GetInputPins();
  if (pPin->m_pParent != this)
  {
    wdLog::Error("wdTargetPass::GetTextureHandle: The given pin is not part of this pass!");
    return nullptr;
  }

  wdGALTextureHandle hTarget;
  if (pPin->m_uiInputIndex == 8)
  {
    return &renderTargets.m_hDSTarget;
  }
  else
  {
    return &renderTargets.m_hRTs[pPin->m_uiInputIndex];
  }

  return nullptr;
}

bool wdTargetPass::GetRenderTargetDescriptions(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, wdArrayPtr<wdGALTextureCreationDescription> outputs)
{
  const char* pinNames[] = {
    "Color0",
    "Color1",
    "Color2",
    "Color3",
    "Color4",
    "Color5",
    "Color6",
    "Color7",
    "DepthStencil",
  };

  for (wdUInt32 i = 0; i < WD_ARRAY_SIZE(pinNames); ++i)
  {
    if (!VerifyInput(view, inputs, pinNames[i]))
      return false;
  }

  return true;
}

void wdTargetPass::Execute(const wdRenderViewContext& renderViewContext, const wdArrayPtr<wdRenderPipelinePassConnection* const> inputs, const wdArrayPtr<wdRenderPipelinePassConnection* const> outputs) {}

bool wdTargetPass::VerifyInput(const wdView& view, const wdArrayPtr<wdGALTextureCreationDescription* const> inputs, const char* szPinName)
{
  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  const wdRenderPipelineNodePin* pPin = GetPinByName(szPinName);
  if (inputs[pPin->m_uiInputIndex])
  {
    const wdGALTextureHandle* pHandle = GetTextureHandle(view.GetActiveRenderTargets(), pPin);
    if (pHandle)
    {
      const wdGALTexture* pTexture = pDevice->GetTexture(*pHandle);
      if (pTexture)
      {
        // TODO: Need a more sophisticated check here what is considered 'matching'
        // if (inputs[pPin->m_uiInputIndex]->CalculateHash() != pTexture->GetDescription().CalculateHash())
        //  return false;
      }
    }
  }

  return true;
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_TargetPass);
