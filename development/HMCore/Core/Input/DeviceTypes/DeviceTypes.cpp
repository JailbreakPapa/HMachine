#include <Core/CorePCH.h>

#include <Core/Input/DeviceTypes/Controller.h>
#include <Core/Input/DeviceTypes/MouseKeyboard.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdInputDeviceMouseKeyboard, 1, wdRTTINoAllocator)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdInputDeviceController, 1, wdRTTINoAllocator)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdInt32 wdInputDeviceMouseKeyboard::s_iMouseIsOverWindowNumber = -1;

wdInputDeviceController::wdInputDeviceController()
{
  m_uiVibrationTrackPos = 0;

  for (wdInt8 c = 0; c < MaxControllers; ++c)
  {
    m_bVibrationEnabled[c] = false;
    m_iControllerMapping[c] = c;

    for (wdInt8 m = 0; m < Motor::ENUM_COUNT; ++m)
    {
      m_fVibrationStrength[c][m] = 0.0f;

      for (wdUInt8 t = 0; t < MaxVibrationSamples; ++t)
        m_fVibrationTracks[c][m][t] = 0.0f;
    }
  }
}

void wdInputDeviceController::EnableVibration(wdUInt8 uiVirtual, bool bEnable)
{
  WD_ASSERT_DEV(uiVirtual < MaxControllers, "Controller Index {0} is larger than allowed ({1}).", uiVirtual, MaxControllers);

  m_bVibrationEnabled[uiVirtual] = bEnable;
}

bool wdInputDeviceController::IsVibrationEnabled(wdUInt8 uiVirtual) const
{
  WD_ASSERT_DEV(uiVirtual < MaxControllers, "Controller Index {0} is larger than allowed ({1}).", uiVirtual, MaxControllers);

  return m_bVibrationEnabled[uiVirtual];
}

void wdInputDeviceController::SetVibrationStrength(wdUInt8 uiVirtual, Motor::Enum motor, float fValue)
{
  WD_ASSERT_DEV(uiVirtual < MaxControllers, "Controller Index {0} is larger than allowed ({1}).", uiVirtual, MaxControllers);
  WD_ASSERT_DEV(motor < Motor::ENUM_COUNT, "Invalid Vibration Motor Index.");

  m_fVibrationStrength[uiVirtual][motor] = wdMath::Clamp(fValue, 0.0f, 1.0f);
}

float wdInputDeviceController::GetVibrationStrength(wdUInt8 uiVirtual, Motor::Enum motor)
{
  WD_ASSERT_DEV(uiVirtual < MaxControllers, "Controller Index {0} is larger than allowed ({1}).", uiVirtual, MaxControllers);
  WD_ASSERT_DEV(motor < Motor::ENUM_COUNT, "Invalid Vibration Motor Index.");

  return m_fVibrationStrength[uiVirtual][motor];
}

void wdInputDeviceController::SetControllerMapping(wdUInt8 uiVirtualController, wdInt8 iTakeInputFromPhysical)
{
  WD_ASSERT_DEV(
    uiVirtualController < MaxControllers, "Virtual Controller Index {0} is larger than allowed ({1}).", uiVirtualController, MaxControllers);
  WD_ASSERT_DEV(
    iTakeInputFromPhysical < MaxControllers, "Physical Controller Index {0} is larger than allowed ({1}).", iTakeInputFromPhysical, MaxControllers);

  if (iTakeInputFromPhysical < 0)
  {
    // deactivates this virtual controller
    m_iControllerMapping[uiVirtualController] = -1;
  }
  else
  {
    // if any virtual controller already maps to the given physical controller, let it use the physical controller that
    // uiVirtualController is currently mapped to
    for (wdInt32 c = 0; c < MaxControllers; ++c)
    {
      if (m_iControllerMapping[c] == iTakeInputFromPhysical)
      {
        m_iControllerMapping[c] = m_iControllerMapping[uiVirtualController];
        break;
      }
    }

    m_iControllerMapping[uiVirtualController] = iTakeInputFromPhysical;
  }
}

wdInt8 wdInputDeviceController::GetControllerMapping(wdUInt8 uiVirtual) const
{
  WD_ASSERT_DEV(uiVirtual < MaxControllers, "Virtual Controller Index {0} is larger than allowed ({1}).", uiVirtual, MaxControllers);

  return m_iControllerMapping[uiVirtual];
}

void wdInputDeviceController::AddVibrationTrack(
  wdUInt8 uiVirtual, Motor::Enum motor, float* pVibrationTrackValue, wdUInt32 uiSamples, float fScalingFactor)
{
  uiSamples = wdMath::Min<wdUInt32>(uiSamples, MaxVibrationSamples);

  for (wdUInt32 s = 0; s < uiSamples; ++s)
  {
    float& fVal = m_fVibrationTracks[uiVirtual][motor][(m_uiVibrationTrackPos + 1 + s) % MaxVibrationSamples];

    fVal = wdMath::Max(fVal, pVibrationTrackValue[s] * fScalingFactor);
    fVal = wdMath::Clamp(fVal, 0.0f, 1.0f);
  }
}

void wdInputDeviceController::UpdateVibration(wdTime tTimeDifference)
{
  static wdTime tElapsedTime;
  tElapsedTime += tTimeDifference;

  const wdTime tTimePerSample = wdTime::Seconds(1.0 / VibrationSamplesPerSecond);

  // advance the vibration track sampling
  while (tElapsedTime >= tTimePerSample)
  {
    tElapsedTime -= tTimePerSample;

    for (wdUInt32 c = 0; c < MaxControllers; ++c)
    {
      for (wdUInt32 m = 0; m < Motor::ENUM_COUNT; ++m)
        m_fVibrationTracks[c][m][m_uiVibrationTrackPos] = 0.0f;
    }

    m_uiVibrationTrackPos = (m_uiVibrationTrackPos + 1) % MaxVibrationSamples;
  }

  // we will temporarily store how much vibration is to be applied on each physical controller
  float fVibrationToApply[MaxControllers][Motor::ENUM_COUNT];

  // Initialize with zero (we might not set all values later)
  for (wdUInt32 c = 0; c < MaxControllers; ++c)
  {
    for (wdUInt32 m = 0; m < Motor::ENUM_COUNT; ++m)
      fVibrationToApply[c][m] = 0.0f;
  }

  // go through all controllers and motors
  for (wdUInt8 c = 0; c < MaxControllers; ++c)
  {
    // ignore if vibration is disabled on this controller
    if (!m_bVibrationEnabled[c])
      continue;

    // check which physical controller this virtual controller is attached to
    const wdInt8 iPhysical = GetControllerMapping(c);

    // if it is attached to any physical controller, store the vibration value
    if (iPhysical >= 0)
    {
      for (wdUInt32 m = 0; m < Motor::ENUM_COUNT; ++m)
        fVibrationToApply[(wdUInt8)iPhysical][m] = wdMath::Max(m_fVibrationStrength[c][m], m_fVibrationTracks[c][m][m_uiVibrationTrackPos]);
    }
  }

  // now send the back-end all the information about how to vibrate which physical controller
  // this also always resets vibration to zero for controllers that might have been changed to another virtual controller etc.
  for (wdUInt8 c = 0; c < MaxControllers; ++c)
  {
    for (wdUInt32 m = 0; m < Motor::ENUM_COUNT; ++m)
    {
      ApplyVibration(c, (Motor::Enum)m, fVibrationToApply[c][m]);
    }
  }
}

void wdInputDeviceMouseKeyboard::UpdateInputSlotValues()
{
  const char* slots[3] = {wdInputSlot_MouseButton0, wdInputSlot_MouseButton1, wdInputSlot_MouseButton2};
  const char* dlbSlots[3] = {wdInputSlot_MouseDblClick0, wdInputSlot_MouseDblClick1, wdInputSlot_MouseDblClick2};

  const wdTime tNow = wdTime::Now();

  for (int i = 0; i < 3; ++i)
  {
    m_InputSlotValues[dlbSlots[i]] = 0.0f;

    const bool bDown = m_InputSlotValues[slots[i]] > 0;
    if (bDown)
    {
      if (!m_bMouseDown[i])
      {
        if (tNow - m_LastMouseClick[i] <= m_DoubleClickTime)
        {
          m_InputSlotValues[dlbSlots[i]] = 1.0f;
          m_LastMouseClick[i].SetZero(); // this prevents triple-clicks from appearing as two double clicks
        }
        else
        {
          m_LastMouseClick[i] = tNow;
        }
      }
    }

    m_bMouseDown[i] = bDown;
  }
}

WD_STATICLINK_FILE(Core, Core_Input_DeviceTypes_DeviceTypes);
