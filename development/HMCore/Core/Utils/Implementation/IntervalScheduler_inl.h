
#include <Foundation/SimdMath/SimdRandom.h>

WD_ALWAYS_INLINE wdUInt32 wdIntervalSchedulerBase::GetHistogramIndex(wdTime value)
{
  constexpr wdUInt32 maxSlotIndex = HistogramSize - 1;
  const double x = wdMath::Max((value - m_MinInterval).GetSeconds() * m_fInvIntervalRange, 0.0);
  const double i = wdMath::Sqrt(x) * maxSlotIndex;
  return wdMath::Min(static_cast<wdUInt32>(i), maxSlotIndex);
}

WD_ALWAYS_INLINE wdTime wdIntervalSchedulerBase::GetHistogramSlotValue(wdUInt32 uiIndex)
{
  constexpr double norm = 1.0 / (HistogramSize - 1.0);
  const double x = uiIndex * norm;
  return (x * x) * (m_MaxInterval - m_MinInterval) + m_MinInterval;
}

// static
WD_ALWAYS_INLINE float wdIntervalSchedulerBase::GetRandomZeroToOne(int pos, wdUInt32& seed)
{
  return wdSimdRandom::FloatZeroToOne(wdSimdVec4i(pos), wdSimdVec4u(seed++)).x();
}

constexpr wdTime s_JitterRange = wdTime::Microseconds(10);

// static
WD_ALWAYS_INLINE wdTime wdIntervalSchedulerBase::GetRandomTimeJitter(int pos, wdUInt32& seed)
{
  const float x = wdSimdRandom::FloatZeroToOne(wdSimdVec4i(pos), wdSimdVec4u(seed++)).x();
  return s_JitterRange * (x * 2.0f - 1.0f);
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
void wdIntervalScheduler<T>::AddOrUpdateWork(const T& work, wdTime interval)
{
  typename DataMap::Iterator it;
  if (m_WorkIdToData.TryGetValue(work, it))
  {
    wdTime oldInterval = it.Value().m_Interval;
    if (interval == oldInterval)
      return;

    m_Data.Remove(it);

    const wdUInt32 uiHistogramIndex = GetHistogramIndex(oldInterval);
    m_Histogram[uiHistogramIndex]--;
  }

  Data data;
  data.m_Work = work;
  data.m_Interval = wdMath::Max(interval, wdTime::Zero());
  data.m_DueTime = m_CurrentTime + GetRandomZeroToOne(m_Data.GetCount(), m_uiSeed) * data.m_Interval;
  data.m_LastScheduledTime = m_CurrentTime;

  m_WorkIdToData[work] = InsertData(data);

  const wdUInt32 uiHistogramIndex = GetHistogramIndex(data.m_Interval);
  m_Histogram[uiHistogramIndex]++;
}

template <typename T>
void wdIntervalScheduler<T>::RemoveWork(const T& work)
{
  typename DataMap::Iterator it;
  WD_VERIFY(m_WorkIdToData.Remove(work, &it), "Entry not found");

  wdTime oldInterval = it.Value().m_Interval;
  m_Data.Remove(it);

  const wdUInt32 uiHistogramIndex = GetHistogramIndex(oldInterval);
  m_Histogram[uiHistogramIndex]--;
}

template <typename T>
wdTime wdIntervalScheduler<T>::GetInterval(const T& work) const
{
  typename DataMap::Iterator it;
  WD_VERIFY(m_WorkIdToData.TryGetValue(work, it), "Entry not found");
  return it.Value().m_Interval;
}

template <typename T>
void wdIntervalScheduler<T>::Update(wdTime deltaTime, RunWorkCallback runWorkCallback)
{
  if (deltaTime <= wdTime::Zero())
    return;

  if (m_Data.IsEmpty())
  {
    m_fNumWorkToSchedule = 0.0;
  }
  else
  {
    double fNumWork = 0;
    for (wdUInt32 i = 0; i < HistogramSize; ++i)
    {
      fNumWork += (1.0 / wdMath::Max(m_HistogramSlotValues[i], deltaTime).GetSeconds()) * m_Histogram[i];
    }
    fNumWork *= deltaTime.GetSeconds();

    if (m_fNumWorkToSchedule == 0.0)
    {
      m_fNumWorkToSchedule = fNumWork;
    }
    else
    {
      // running average of num work per update to prevent huge spikes
      m_fNumWorkToSchedule = wdMath::Lerp<double>(m_fNumWorkToSchedule, fNumWork, 0.05);
    }

    const float fRemainder = static_cast<float>(wdMath::Fraction(m_fNumWorkToSchedule));
    const int pos = static_cast<int>(m_CurrentTime.GetNanoseconds());
    const wdUInt32 extra = GetRandomZeroToOne(pos, m_uiSeed) < fRemainder ? 1 : 0;
    const wdUInt32 uiScheduleCount = wdMath::Min(static_cast<wdUInt32>(m_fNumWorkToSchedule) + extra, m_Data.GetCount());

    // schedule work
    {
      auto it = m_Data.GetIterator();
      for (wdUInt32 i = 0; i < uiScheduleCount; ++i, ++it)
      {
        auto& data = it.Value();
        if (runWorkCallback.IsValid())
        {
          runWorkCallback(data.m_Work, m_CurrentTime - data.m_LastScheduledTime);
        }

        // add a little bit of random jitter so we don't end up with perfect timings that might collide with other work
        data.m_DueTime = m_CurrentTime + wdMath::Max(data.m_Interval, deltaTime) + GetRandomTimeJitter(i, m_uiSeed);
        data.m_LastScheduledTime = m_CurrentTime;

        m_ScheduledWork.PushBack(it);
      }
    }

    // re-sort
    for (auto& it : m_ScheduledWork)
    {
      Data data = it.Value();
      m_WorkIdToData[data.m_Work] = InsertData(data);
      m_Data.Remove(it);
    }
    m_ScheduledWork.Clear();
  }

  m_CurrentTime += deltaTime;
}

template <typename T>
WD_FORCE_INLINE typename wdIntervalScheduler<T>::DataMap::Iterator wdIntervalScheduler<T>::InsertData(Data& data)
{
  // make sure that we have a unique due time since the map can't store multiple keys with the same value
  int pos = 0;
  while (m_Data.Contains(data.m_DueTime))
  {
    data.m_DueTime += GetRandomTimeJitter(pos++, m_uiSeed);
  }

  return m_Data.Insert(data.m_DueTime, data);
}
