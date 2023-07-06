#include <Core/CorePCH.h>

#include <Core/GameState/GameStateWindow.h>

wdGameStateWindow::wdGameStateWindow(const wdWindowCreationDesc& windowdesc, wdDelegate<void()> onClickClose)
  : m_OnClickClose(onClickClose)
{
  m_CreationDescription = windowdesc;
  m_CreationDescription.AdjustWindowSizeAndPosition().IgnoreResult();

  Initialize().IgnoreResult();
}

wdGameStateWindow::~wdGameStateWindow()
{
  Destroy().IgnoreResult();
}


void wdGameStateWindow::ResetOnClickClose(wdDelegate<void()> onClickClose)
{
  m_OnClickClose = onClickClose;
}

void wdGameStateWindow::OnClickClose()
{
  if (m_OnClickClose.IsValid())
  {
    m_OnClickClose();
  }
}

void wdGameStateWindow::OnResize(const wdSizeU32& newWindowSize)
{
  wdLog::Info("Resolution changed to {0} * {1}", newWindowSize.width, newWindowSize.height);

  m_CreationDescription.m_Resolution = newWindowSize;
}



WD_STATICLINK_FILE(Core, Core_GameState_Implementation_GameStateWindow);
