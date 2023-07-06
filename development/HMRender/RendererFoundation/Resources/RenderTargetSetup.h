
#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/RendererFoundationDLL.h>

// \brief This class can be used to define the render targets to be used by an wdView.
struct WD_RENDERERFOUNDATION_DLL wdGALRenderTargets
{
  bool operator==(const wdGALRenderTargets& other) const;
  bool operator!=(const wdGALRenderTargets& other) const;

  wdGALTextureHandle m_hRTs[WD_GAL_MAX_RENDERTARGET_COUNT];
  wdGALTextureHandle m_hDSTarget;
};

// \brief This class can be used to construct render target setups on the stack.
class WD_RENDERERFOUNDATION_DLL wdGALRenderTargetSetup
{
public:
  wdGALRenderTargetSetup();

  wdGALRenderTargetSetup& SetRenderTarget(wdUInt8 uiIndex, wdGALRenderTargetViewHandle hRenderTarget);
  wdGALRenderTargetSetup& SetDepthStencilTarget(wdGALRenderTargetViewHandle hDSTarget);

  bool operator==(const wdGALRenderTargetSetup& other) const;
  bool operator!=(const wdGALRenderTargetSetup& other) const;

  inline wdUInt8 GetRenderTargetCount() const;

  inline wdGALRenderTargetViewHandle GetRenderTarget(wdUInt8 uiIndex) const;
  inline wdGALRenderTargetViewHandle GetDepthStencilTarget() const;

  void DestroyAllAttachedViews();

protected:
  wdGALRenderTargetViewHandle m_hRTs[WD_GAL_MAX_RENDERTARGET_COUNT];
  wdGALRenderTargetViewHandle m_hDSTarget;

  wdUInt8 m_uiRTCount = 0;
};

struct WD_RENDERERFOUNDATION_DLL wdGALRenderingSetup
{
  bool operator==(const wdGALRenderingSetup& other) const;
  bool operator!=(const wdGALRenderingSetup& other) const;

  wdGALRenderTargetSetup m_RenderTargetSetup;
  wdColor m_ClearColor = wdColor(0, 0, 0, 0);
  wdUInt32 m_uiRenderTargetClearMask = 0x0;
  float m_fDepthClear = 1.0f;
  wdUInt8 m_uiStencilClear = 0;
  bool m_bClearDepth = false;
  bool m_bClearStencil = false;
  bool m_bDiscardColor = false;
  bool m_bDiscardDepth = false;
};

#include <RendererFoundation/Resources/Implementation/RenderTargetSetup_inl.h>
