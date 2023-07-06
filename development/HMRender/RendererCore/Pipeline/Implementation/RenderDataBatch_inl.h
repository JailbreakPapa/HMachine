
template <typename T>
WD_ALWAYS_INLINE const T& wdRenderDataBatch::Iterator<T>::operator*() const
{
  return *wdStaticCast<const T*>(m_pCurrent->m_pRenderData);
}

template <typename T>
WD_ALWAYS_INLINE const T* wdRenderDataBatch::Iterator<T>::operator->() const
{
  return wdStaticCast<const T*>(m_pCurrent->m_pRenderData);
}

template <typename T>
WD_ALWAYS_INLINE wdRenderDataBatch::Iterator<T>::operator const T*() const
{
  return wdStaticCast<const T*>(m_pCurrent->m_pRenderData);
}

template <typename T>
WD_FORCE_INLINE void wdRenderDataBatch::Iterator<T>::Next()
{
  ++m_pCurrent;

  if (m_Filter.IsValid())
  {
    while (m_pCurrent < m_pEnd && m_Filter(m_pCurrent->m_pRenderData))
    {
      ++m_pCurrent;
    }
  }
}

template <typename T>
WD_ALWAYS_INLINE bool wdRenderDataBatch::Iterator<T>::IsValid() const
{
  return m_pCurrent < m_pEnd;
}

template <typename T>
WD_ALWAYS_INLINE void wdRenderDataBatch::Iterator<T>::operator++()
{
  Next();
}

template <typename T>
WD_FORCE_INLINE wdRenderDataBatch::Iterator<T>::Iterator(const SortableRenderData* pStart, const SortableRenderData* pEnd, Filter filter)
  : m_Filter(filter)
{
  const SortableRenderData* pCurrent = pStart;
  if (m_Filter.IsValid())
  {
    while (pCurrent < pEnd && m_Filter(pCurrent->m_pRenderData))
    {
      ++pCurrent;
    }
  }

  m_pCurrent = pCurrent;
  m_pEnd = pEnd;
}


WD_ALWAYS_INLINE wdUInt32 wdRenderDataBatch::GetCount() const
{
  return m_Data.GetCount();
}

template <typename T>
WD_FORCE_INLINE const T* wdRenderDataBatch::GetFirstData() const
{
  auto it = Iterator<T>(m_Data.GetPtr(), m_Data.GetPtr() + m_Data.GetCount(), m_Filter);
  return it.IsValid() ? (const T*)it : nullptr;
}

template <typename T>
WD_FORCE_INLINE wdRenderDataBatch::Iterator<T> wdRenderDataBatch::GetIterator(wdUInt32 uiStartIndex, wdUInt32 uiCount) const
{
  wdUInt32 uiEndIndex = wdMath::Min(uiStartIndex + uiCount, m_Data.GetCount());
  return Iterator<T>(m_Data.GetPtr() + uiStartIndex, m_Data.GetPtr() + uiEndIndex, m_Filter);
}

//////////////////////////////////////////////////////////////////////////

WD_ALWAYS_INLINE wdUInt32 wdRenderDataBatchList::GetBatchCount() const
{
  return m_Batches.GetCount();
}

WD_FORCE_INLINE wdRenderDataBatch wdRenderDataBatchList::GetBatch(wdUInt32 uiIndex) const
{
  wdRenderDataBatch batch = m_Batches[uiIndex];
  batch.m_Filter = m_Filter;

  return batch;
}
