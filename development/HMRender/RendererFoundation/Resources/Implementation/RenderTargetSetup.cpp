#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

bool wdGALRenderTargets::operator==(const wdGALRenderTargets& other) const
{
  if (m_hDSTarget != other.m_hDSTarget)
    return false;

  for (wdUInt8 uiRTIndex = 0; uiRTIndex < WD_GAL_MAX_RENDERTARGET_COUNT; ++uiRTIndex)
  {
    if (m_hRTs[uiRTIndex] != other.m_hRTs[uiRTIndex])
      return false;
  }
  return true;
}

bool wdGALRenderTargets::operator!=(const wdGALRenderTargets& other) const
{
  return !(*this == other);
}

wdGALRenderTargetSetup::wdGALRenderTargetSetup()
{
}

wdGALRenderTargetSetup& wdGALRenderTargetSetup::SetRenderTarget(wdUInt8 uiIndex, wdGALRenderTargetViewHandle hRenderTarget)
{
  WD_ASSERT_DEV(uiIndex < WD_GAL_MAX_RENDERTARGET_COUNT, "Render target index out of bounds - should be less than WD_GAL_MAX_RENDERTARGET_COUNT");

  m_hRTs[uiIndex] = hRenderTarget;

  m_uiRTCount = wdMath::Max(m_uiRTCount, static_cast<wdUInt8>(uiIndex + 1u));

  return *this;
}

wdGALRenderTargetSetup& wdGALRenderTargetSetup::SetDepthStencilTarget(wdGALRenderTargetViewHandle hDSTarget)
{
  m_hDSTarget = hDSTarget;

  return *this;
}

bool wdGALRenderTargetSetup::operator==(const wdGALRenderTargetSetup& other) const
{
  if (m_hDSTarget != other.m_hDSTarget)
    return false;

  if (m_uiRTCount != other.m_uiRTCount)
    return false;

  for (wdUInt8 uiRTIndex = 0; uiRTIndex < m_uiRTCount; ++uiRTIndex)
  {
    if (m_hRTs[uiRTIndex] != other.m_hRTs[uiRTIndex])
      return false;
  }

  return true;
}

bool wdGALRenderTargetSetup::operator!=(const wdGALRenderTargetSetup& other) const
{
  return !(*this == other);
}

void wdGALRenderTargetSetup::DestroyAllAttachedViews()
{
  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  wdArrayPtr<wdGALRenderTargetViewHandle> colorViews(m_hRTs);
  for (wdGALRenderTargetViewHandle& hView : colorViews)
  {
    if (!hView.IsInvalidated())
    {
      pDevice->DestroyRenderTargetView(hView);
      hView.Invalidate();
    }
  }

  if (!m_hDSTarget.IsInvalidated())
  {
    pDevice->DestroyRenderTargetView(m_hDSTarget);
    m_hDSTarget.Invalidate();
  }
  m_uiRTCount = 0;
}

bool wdGALRenderingSetup::operator==(const wdGALRenderingSetup& other) const
{
  return m_RenderTargetSetup == other.m_RenderTargetSetup && m_ClearColor == other.m_ClearColor && m_uiRenderTargetClearMask == other.m_uiRenderTargetClearMask && m_fDepthClear == other.m_fDepthClear && m_uiStencilClear == other.m_uiStencilClear && m_bClearDepth == other.m_bClearDepth && m_bClearStencil == other.m_bClearStencil && m_bDiscardColor == other.m_bDiscardColor && m_bDiscardDepth == other.m_bDiscardDepth;
}

bool wdGALRenderingSetup::operator!=(const wdGALRenderingSetup& other) const
{
  return !(*this == other);
}

WD_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_RenderTargetSetup);
