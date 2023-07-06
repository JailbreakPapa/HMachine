
#pragma once

wdUInt8 wdGALRenderTargetSetup::GetRenderTargetCount() const
{
  return m_uiRTCount;
}

wdGALRenderTargetViewHandle wdGALRenderTargetSetup::GetRenderTarget(wdUInt8 uiIndex) const
{
  WD_ASSERT_DEBUG(uiIndex < m_uiRTCount, "Render target index out of range");

  return m_hRTs[uiIndex];
}

wdGALRenderTargetViewHandle wdGALRenderTargetSetup::GetDepthStencilTarget() const
{
  return m_hDSTarget;
}
