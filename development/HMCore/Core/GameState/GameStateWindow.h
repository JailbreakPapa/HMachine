#pragma once

#include <Core/System/Window.h>

/// \brief A window class that expands a little on wdWindow. Default type used by wdGameState to create a window.
class WD_CORE_DLL wdGameStateWindow : public wdWindow
{
public:
  wdGameStateWindow(const wdWindowCreationDesc& windowdesc, wdDelegate<void()> onClickClose = {});
  ~wdGameStateWindow();

  void ResetOnClickClose(wdDelegate<void()> onClickClose);

private:
  virtual void OnResize(const wdSizeU32& newWindowSize) override;
  virtual void OnClickClose() override;

  wdDelegate<void()> m_OnClickClose;
};
