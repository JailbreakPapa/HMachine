#include <Core/CorePCH.h>

#include <Core/Utils/IntervalScheduler.h>

// clang-format off
WD_BEGIN_STATIC_REFLECTED_ENUM(wdUpdateRate, 1)
  WD_ENUM_CONSTANTS(wdUpdateRate::EveryFrame)
  WD_ENUM_CONSTANTS(wdUpdateRate::Max30fps, wdUpdateRate::Max20fps, wdUpdateRate::Max10fps)
  WD_ENUM_CONSTANTS(wdUpdateRate::Max5fps, wdUpdateRate::Max2fps, wdUpdateRate::Max1fps)
WD_END_STATIC_REFLECTED_ENUM;
// clang-format on

static wdTime s_Intervals[] = {
  wdTime::Zero(),              // EveryFrame
  wdTime::Seconds(1.0 / 30.0), // Max30fps
  wdTime::Seconds(1.0 / 20.0), // Max20fps
  wdTime::Seconds(1.0 / 10.0), // Max10fps
  wdTime::Seconds(1.0 / 5.0),  // Max5fps
  wdTime::Seconds(1.0 / 2.0),  // Max2fps
  wdTime::Seconds(1.0 / 1.0),  // Max1fps
};

static_assert(WD_ARRAY_SIZE(s_Intervals) == wdUpdateRate::Max1fps + 1);

wdTime wdUpdateRate::GetInterval(Enum updateRate)
{
  return s_Intervals[updateRate];
}

//////////////////////////////////////////////////////////////////////////

wdIntervalSchedulerBase::wdIntervalSchedulerBase(wdTime minInterval, wdTime maxInterval)
  : m_MinInterval(minInterval)
  , m_MaxInterval(maxInterval)
{
  m_fInvIntervalRange = 1.0 / (m_MaxInterval - m_MinInterval).GetSeconds();

  for (wdUInt32 i = 0; i < HistogramSize; ++i)
  {
    m_HistogramSlotValues[i] = GetHistogramSlotValue(i);
  }
}

wdIntervalSchedulerBase::~wdIntervalSchedulerBase() = default;


WD_STATICLINK_FILE(Core, Core_Utils_Implementation_IntervalScheduler);
