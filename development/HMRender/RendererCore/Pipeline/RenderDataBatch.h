#pragma once

#include <RendererCore/Pipeline/Declarations.h>

class wdRenderDataBatch
{
private:
  struct SortableRenderData
  {
    WD_DECLARE_POD_TYPE();

    const wdRenderData* m_pRenderData;
    wdUInt64 m_uiSortingKey;
  };

public:
  WD_DECLARE_POD_TYPE();

  /// \brief This function should return true if the given render data should be filtered and not rendered.
  using Filter = wdDelegate<bool(const wdRenderData*)>;

  template <typename T>
  class Iterator
  {
  public:
    const T& operator*() const;
    const T* operator->() const;

    operator const T*() const;

    void Next();

    bool IsValid() const;

    void operator++();

  private:
    friend class wdRenderDataBatch;

    Iterator(const SortableRenderData* pStart, const SortableRenderData* pEnd, Filter filter);

    Filter m_Filter;
    const SortableRenderData* m_pCurrent;
    const SortableRenderData* m_pEnd;
  };

  wdUInt32 GetCount() const;

  template <typename T>
  const T* GetFirstData() const;

  template <typename T>
  Iterator<T> GetIterator(wdUInt32 uiStartIndex = 0, wdUInt32 uiCount = wdInvalidIndex) const;

private:
  friend class wdExtractedRenderData;
  friend class wdRenderDataBatchList;

  Filter m_Filter;
  wdArrayPtr<SortableRenderData> m_Data;
};

class wdRenderDataBatchList
{
public:
  wdUInt32 GetBatchCount() const;

  wdRenderDataBatch GetBatch(wdUInt32 uiIndex) const;

private:
  friend class wdExtractedRenderData;

  wdRenderDataBatch::Filter m_Filter;
  wdArrayPtr<const wdRenderDataBatch> m_Batches;
};

#include <RendererCore/Pipeline/Implementation/RenderDataBatch_inl.h>
