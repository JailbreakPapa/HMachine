#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

struct WD_CORE_DLL wdUpdateRate
{
  using StorageType = wdUInt8;

  enum Enum
  {
    EveryFrame,
    Max30fps,
    Max20fps,
    Max10fps,
    Max5fps,
    Max2fps,
    Max1fps,

    Default = Max30fps
  };

  static wdTime GetInterval(Enum updateRate);
};

WD_DECLARE_REFLECTABLE_TYPE(WD_CORE_DLL, wdUpdateRate);

//////////////////////////////////////////////////////////////////////////

/// \brief Helper class to schedule work in intervals typically larger than the duration of one frame
///
/// Tries to maintain an even workload per frame and also keep the given interval for a work as best as possible.
/// A typical use case would be e.g. component update functions that don't need to be called every frame.
class WD_CORE_DLL wdIntervalSchedulerBase
{
protected:
  wdIntervalSchedulerBase(wdTime minInterval, wdTime maxInterval);
  ~wdIntervalSchedulerBase();

  wdUInt32 GetHistogramIndex(wdTime value);
  wdTime GetHistogramSlotValue(wdUInt32 uiIndex);

  static float GetRandomZeroToOne(int pos, wdUInt32& seed);
  static wdTime GetRandomTimeJitter(int pos, wdUInt32& seed);

  wdTime m_MinInterval;
  wdTime m_MaxInterval;
  double m_fInvIntervalRange;

  wdTime m_CurrentTime;
  double m_fNumWorkToSchedule = 0.0;

  wdUInt32 m_uiSeed = 0;

  static constexpr wdUInt32 HistogramSize = 32;
  wdUInt32 m_Histogram[HistogramSize] = {};
  wdTime m_HistogramSlotValues[HistogramSize] = {};
};

//////////////////////////////////////////////////////////////////////////

/// \brief \see wdIntervalSchedulerBase
template <typename T>
class wdIntervalScheduler : public wdIntervalSchedulerBase
{
  using SUPER = wdIntervalSchedulerBase;

public:
  WD_ALWAYS_INLINE wdIntervalScheduler(wdTime minInterval = wdTime::Milliseconds(1), wdTime maxInterval = wdTime::Seconds(1))
    : SUPER(minInterval, maxInterval)
  {
  }

  void AddOrUpdateWork(const T& work, wdTime interval);
  void RemoveWork(const T& work);

  wdTime GetInterval(const T& work) const;

  // reference to the work that should be run and time passed since this work has been last run.
  using RunWorkCallback = wdDelegate<void(const T&, wdTime)>;

  /// \brief Advances the scheduler by deltaTime and triggers runWorkCallback for each work that should be run during this update step.
  /// Since it is not possible to maintain the exact interval all the time the actual delta time for the work is also passed to runWorkCallback.
  void Update(wdTime deltaTime, RunWorkCallback runWorkCallback);

private:
  struct Data
  {
    T m_Work;
    wdTime m_Interval;
    wdTime m_DueTime;
    wdTime m_LastScheduledTime;
  };

  using DataMap = wdMap<wdTime, Data>;
  DataMap m_Data;
  wdHashTable<T, typename DataMap::Iterator> m_WorkIdToData;

  typename DataMap::Iterator InsertData(Data& data);
  wdDynamicArray<typename DataMap::Iterator> m_ScheduledWork;
};

#include <Core/Utils/Implementation/IntervalScheduler_inl.h>
