#include <Core/CorePCH.h>

#include <Core/Input/VirtualThumbStick.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdVirtualThumbStick, 1, wdRTTINoAllocator)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdInt32 wdVirtualThumbStick::s_iThumbsticks = 0;

wdVirtualThumbStick::wdVirtualThumbStick()
{
  SetAreaFocusMode(wdInputActionConfig::RequireKeyUp, wdInputActionConfig::KeepFocus);
  SetTriggerInputSlot(wdVirtualThumbStick::Input::Touchpoint);
  SetThumbstickOutput(wdVirtualThumbStick::Output::Controller0_LeftStick);

  SetInputArea(wdVec2(0.0f), wdVec2(0.0f), 0.0f, 0.0f);

  wdStringBuilder s;
  s.Format("Thumbstick_{0}", s_iThumbsticks);
  m_sName = s;

  ++s_iThumbsticks;

  m_bEnabled = false;
  m_bConfigChanged = false;
  m_bIsActive = false;
}

wdVirtualThumbStick::~wdVirtualThumbStick()
{
  wdInputManager::RemoveInputAction(GetDynamicRTTI()->GetTypeName(), m_sName.GetData());
}

void wdVirtualThumbStick::SetTriggerInputSlot(wdVirtualThumbStick::Input::Enum input, const wdInputActionConfig* pCustomConfig)
{
  for (wdInt32 i = 0; i < wdInputActionConfig::MaxInputSlotAlternatives; ++i)
  {
    m_ActionConfig.m_sFilterByInputSlotX[i] = wdInputSlot_None;
    m_ActionConfig.m_sFilterByInputSlotY[i] = wdInputSlot_None;
    m_ActionConfig.m_sInputSlotTrigger[i] = wdInputSlot_None;
  }

  switch (input)
  {
    case wdVirtualThumbStick::Input::Touchpoint:
    {
      m_ActionConfig.m_sFilterByInputSlotX[0] = wdInputSlot_TouchPoint0_PositionX;
      m_ActionConfig.m_sFilterByInputSlotY[0] = wdInputSlot_TouchPoint0_PositionY;
      m_ActionConfig.m_sInputSlotTrigger[0] = wdInputSlot_TouchPoint0;

      m_ActionConfig.m_sFilterByInputSlotX[1] = wdInputSlot_TouchPoint1_PositionX;
      m_ActionConfig.m_sFilterByInputSlotY[1] = wdInputSlot_TouchPoint1_PositionY;
      m_ActionConfig.m_sInputSlotTrigger[1] = wdInputSlot_TouchPoint1;

      m_ActionConfig.m_sFilterByInputSlotX[2] = wdInputSlot_TouchPoint2_PositionX;
      m_ActionConfig.m_sFilterByInputSlotY[2] = wdInputSlot_TouchPoint2_PositionY;
      m_ActionConfig.m_sInputSlotTrigger[2] = wdInputSlot_TouchPoint2;
    }
    break;
    case wdVirtualThumbStick::Input::MousePosition:
    {
      m_ActionConfig.m_sFilterByInputSlotX[0] = wdInputSlot_MousePositionX;
      m_ActionConfig.m_sFilterByInputSlotY[0] = wdInputSlot_MousePositionY;
      m_ActionConfig.m_sInputSlotTrigger[0] = wdInputSlot_MouseButton0;
    }
    break;
    case wdVirtualThumbStick::Input::Custom:
    {
      WD_ASSERT_DEV(pCustomConfig != nullptr, "Must pass a custom config, if you want to have a custom config.");

      for (wdInt32 i = 0; i < wdInputActionConfig::MaxInputSlotAlternatives; ++i)
      {
        m_ActionConfig.m_sFilterByInputSlotX[i] = pCustomConfig->m_sFilterByInputSlotX[i];
        m_ActionConfig.m_sFilterByInputSlotY[i] = pCustomConfig->m_sFilterByInputSlotY[i];
        m_ActionConfig.m_sInputSlotTrigger[i] = pCustomConfig->m_sInputSlotTrigger[i];
      }
    }
    break;
  }

  m_bConfigChanged = true;
}

void wdVirtualThumbStick::SetThumbstickOutput(
  wdVirtualThumbStick::Output::Enum output, const char* szOutputLeft, const char* szOutputRight, const char* szOutputUp, const char* szOutputDown)
{
  switch (output)
  {
    case wdVirtualThumbStick::Output::Controller0_LeftStick:
    {
      m_szOutputLeft = wdInputSlot_Controller0_LeftStick_NegX;
      m_szOutputRight = wdInputSlot_Controller0_LeftStick_PosX;
      m_szOutputUp = wdInputSlot_Controller0_LeftStick_PosY;
      m_szOutputDown = wdInputSlot_Controller0_LeftStick_NegY;
    }
    break;
    case wdVirtualThumbStick::Output::Controller0_RightStick:
    {
      m_szOutputLeft = wdInputSlot_Controller0_RightStick_NegX;
      m_szOutputRight = wdInputSlot_Controller0_RightStick_PosX;
      m_szOutputUp = wdInputSlot_Controller0_RightStick_PosY;
      m_szOutputDown = wdInputSlot_Controller0_RightStick_NegY;
    }
    break;
    case wdVirtualThumbStick::Output::Controller1_LeftStick:
    {
      m_szOutputLeft = wdInputSlot_Controller1_LeftStick_NegX;
      m_szOutputRight = wdInputSlot_Controller1_LeftStick_PosX;
      m_szOutputUp = wdInputSlot_Controller1_LeftStick_PosY;
      m_szOutputDown = wdInputSlot_Controller1_LeftStick_NegY;
    }
    break;
    case wdVirtualThumbStick::Output::Controller1_RightStick:
    {
      m_szOutputLeft = wdInputSlot_Controller1_RightStick_NegX;
      m_szOutputRight = wdInputSlot_Controller1_RightStick_PosX;
      m_szOutputUp = wdInputSlot_Controller1_RightStick_PosY;
      m_szOutputDown = wdInputSlot_Controller1_RightStick_NegY;
    }
    break;
    case wdVirtualThumbStick::Output::Controller2_LeftStick:
    {
      m_szOutputLeft = wdInputSlot_Controller2_LeftStick_NegX;
      m_szOutputRight = wdInputSlot_Controller2_LeftStick_PosX;
      m_szOutputUp = wdInputSlot_Controller2_LeftStick_PosY;
      m_szOutputDown = wdInputSlot_Controller2_LeftStick_NegY;
    }
    break;
    case wdVirtualThumbStick::Output::Controller2_RightStick:
    {
      m_szOutputLeft = wdInputSlot_Controller2_RightStick_NegX;
      m_szOutputRight = wdInputSlot_Controller2_RightStick_PosX;
      m_szOutputUp = wdInputSlot_Controller2_RightStick_PosY;
      m_szOutputDown = wdInputSlot_Controller2_RightStick_NegY;
    }
    break;
    case wdVirtualThumbStick::Output::Controller3_LeftStick:
    {
      m_szOutputLeft = wdInputSlot_Controller3_LeftStick_NegX;
      m_szOutputRight = wdInputSlot_Controller3_LeftStick_PosX;
      m_szOutputUp = wdInputSlot_Controller3_LeftStick_PosY;
      m_szOutputDown = wdInputSlot_Controller3_LeftStick_NegY;
    }
    break;
    case wdVirtualThumbStick::Output::Controller3_RightStick:
    {
      m_szOutputLeft = wdInputSlot_Controller3_RightStick_NegX;
      m_szOutputRight = wdInputSlot_Controller3_RightStick_PosX;
      m_szOutputUp = wdInputSlot_Controller3_RightStick_PosY;
      m_szOutputDown = wdInputSlot_Controller3_RightStick_NegY;
    }
    break;
    case wdVirtualThumbStick::Output::Custom:
    {
      m_szOutputLeft = szOutputLeft;
      m_szOutputRight = szOutputRight;
      m_szOutputUp = szOutputUp;
      m_szOutputDown = szOutputDown;
    }
    break;
  }

  m_bConfigChanged = true;
}

void wdVirtualThumbStick::SetAreaFocusMode(wdInputActionConfig::OnEnterArea onEnter, wdInputActionConfig::OnLeaveArea onLeave)
{
  m_bConfigChanged = true;

  m_ActionConfig.m_OnEnterArea = onEnter;
  m_ActionConfig.m_OnLeaveArea = onLeave;
}

void wdVirtualThumbStick::SetInputArea(
  const wdVec2& vLowerLeft, const wdVec2& vUpperRight, float fThumbstickRadius, float fPriority, CenterMode::Enum center)
{
  m_bConfigChanged = true;

  m_vLowerLeft = vLowerLeft;
  m_vUpperRight = vUpperRight;
  m_fRadius = fThumbstickRadius;
  m_ActionConfig.m_fFilteredPriority = fPriority;
  m_CenterMode = center;
}

void wdVirtualThumbStick::GetInputArea(wdVec2& out_vLowerLeft, wdVec2& out_vUpperRight)
{
  out_vLowerLeft = m_vLowerLeft;
  out_vUpperRight = m_vUpperRight;
}

void wdVirtualThumbStick::UpdateActionMapping()
{
  if (!m_bConfigChanged)
    return;

  m_ActionConfig.m_fFilterXMinValue = m_vLowerLeft.x;
  m_ActionConfig.m_fFilterXMaxValue = m_vUpperRight.x;
  m_ActionConfig.m_fFilterYMinValue = m_vLowerLeft.y;
  m_ActionConfig.m_fFilterYMaxValue = m_vUpperRight.y;

  wdInputManager::SetInputActionConfig(GetDynamicRTTI()->GetTypeName(), m_sName.GetData(), m_ActionConfig, false);

  m_bConfigChanged = false;
}

void wdVirtualThumbStick::UpdateInputSlotValues()
{
  m_bIsActive = false;

  m_InputSlotValues[m_szOutputLeft] = 0.0f;
  m_InputSlotValues[m_szOutputRight] = 0.0f;
  m_InputSlotValues[m_szOutputUp] = 0.0f;
  m_InputSlotValues[m_szOutputDown] = 0.0f;

  if (!m_bEnabled)
  {
    wdInputManager::RemoveInputAction(GetDynamicRTTI()->GetTypeName(), m_sName.GetData());
    return;
  }

  UpdateActionMapping();

  float fValue;
  wdInt8 iTriggerAlt;

  const wdKeyState::Enum ks = wdInputManager::GetInputActionState(GetDynamicRTTI()->GetTypeName(), m_sName.GetData(), &fValue, &iTriggerAlt);

  if (ks != wdKeyState::Up)
  {
    m_bIsActive = true;

    wdVec2 vTouchPos(0.0f);

    wdInputManager::GetInputSlotState(m_ActionConfig.m_sFilterByInputSlotX[(wdUInt32)iTriggerAlt].GetData(), &vTouchPos.x);
    wdInputManager::GetInputSlotState(m_ActionConfig.m_sFilterByInputSlotY[(wdUInt32)iTriggerAlt].GetData(), &vTouchPos.y);

    if (ks == wdKeyState::Pressed)
    {
      switch (m_CenterMode)
      {
        case CenterMode::InputArea:
          m_vCenter = m_vLowerLeft + (m_vUpperRight - m_vLowerLeft) * 0.5f;
          break;
        case CenterMode::ActivationPoint:
          m_vCenter = vTouchPos;
          break;
      }
    }

    wdVec2 vDir = vTouchPos - m_vCenter;
    vDir.y *= -1;

    const float fLength = wdMath::Min(vDir.GetLength(), m_fRadius) / m_fRadius;
    vDir.Normalize();

    m_InputSlotValues[m_szOutputLeft] = wdMath::Max(0.0f, -vDir.x) * fLength;
    m_InputSlotValues[m_szOutputRight] = wdMath::Max(0.0f, vDir.x) * fLength;
    m_InputSlotValues[m_szOutputUp] = wdMath::Max(0.0f, vDir.y) * fLength;
    m_InputSlotValues[m_szOutputDown] = wdMath::Max(0.0f, -vDir.y) * fLength;
  }
}

void wdVirtualThumbStick::RegisterInputSlots()
{
  RegisterInputSlot(wdInputSlot_Controller0_LeftStick_NegX, "Left Stick Left", wdInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(wdInputSlot_Controller0_LeftStick_PosX, "Left Stick Right", wdInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(wdInputSlot_Controller0_LeftStick_NegY, "Left Stick Down", wdInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(wdInputSlot_Controller0_LeftStick_PosY, "Left Stick Up", wdInputSlotFlags::IsAnalogStick);

  RegisterInputSlot(wdInputSlot_Controller0_RightStick_NegX, "Right Stick Left", wdInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(wdInputSlot_Controller0_RightStick_PosX, "Right Stick Right", wdInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(wdInputSlot_Controller0_RightStick_NegY, "Right Stick Down", wdInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(wdInputSlot_Controller0_RightStick_PosY, "Right Stick Up", wdInputSlotFlags::IsAnalogStick);
}


WD_STATICLINK_FILE(Core, Core_Input_Implementation_VirtualThumbStick);
