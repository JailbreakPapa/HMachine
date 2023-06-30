#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Implementation/TelemetryMessage.h>

wdTelemetryMessage::wdTelemetryMessage()
  : m_Reader(&m_Storage)
  , m_Writer(&m_Storage)
{
  m_uiSystemID = 0;
  m_uiMsgID = 0;
}

wdTelemetryMessage::wdTelemetryMessage(const wdTelemetryMessage& rhs)
  : m_Storage(rhs.m_Storage)
  , m_Reader(&m_Storage)
  , m_Writer(&m_Storage)
{
  m_uiSystemID = rhs.m_uiSystemID;
  m_uiMsgID = rhs.m_uiMsgID;
}

void wdTelemetryMessage::operator=(const wdTelemetryMessage& rhs)
{
  m_Storage = rhs.m_Storage;
  m_uiSystemID = rhs.m_uiSystemID;
  m_uiMsgID = rhs.m_uiMsgID;
  m_Reader.SetStorage(&m_Storage);
  m_Writer.SetStorage(&m_Storage);
}

wdTelemetryMessage::~wdTelemetryMessage()
{
  m_Reader.SetStorage(nullptr);
  m_Writer.SetStorage(nullptr);
}



WD_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_TelemetryMessage);
