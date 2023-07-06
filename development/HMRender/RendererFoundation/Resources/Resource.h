
#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class WD_RENDERERFOUNDATION_DLL wdGALResourceBase : public wdRefCounted
{
public:
  void SetDebugName(const char* szName) const
  {
#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
    m_sDebugName.Assign(szName);
#endif

    SetDebugNamePlatform(szName);
  }

  virtual const wdGALResourceBase* GetParentResource() const { return this; }

protected:
  friend class wdGALDevice;

  inline ~wdGALResourceBase()
  {
    WD_ASSERT_DEV(m_hDefaultResourceView.IsInvalidated(), "");
    WD_ASSERT_DEV(m_hDefaultRenderTargetView.IsInvalidated(), "");

    WD_ASSERT_DEV(m_ResourceViews.IsEmpty(), "Dangling resource views");
    WD_ASSERT_DEV(m_RenderTargetViews.IsEmpty(), "Dangling render target views");
    WD_ASSERT_DEV(m_UnorderedAccessViews.IsEmpty(), "Dangling unordered access views");
  }

  virtual void SetDebugNamePlatform(const char* szName) const = 0;

#if WD_ENABLED(WD_COMPILE_FOR_DEVELOPMENT)
  mutable wdHashedString m_sDebugName;
#endif

  wdGALResourceViewHandle m_hDefaultResourceView;
  wdGALRenderTargetViewHandle m_hDefaultRenderTargetView;

  wdHashTable<wdUInt32, wdGALResourceViewHandle> m_ResourceViews;
  wdHashTable<wdUInt32, wdGALRenderTargetViewHandle> m_RenderTargetViews;
  wdHashTable<wdUInt32, wdGALUnorderedAccessViewHandle> m_UnorderedAccessViews;
};

/// \brief Base class for GAL resources, stores a creation description of the object and also allows for reference counting.
template <typename CreationDescription>
class wdGALResource : public wdGALResourceBase
{
public:
  WD_ALWAYS_INLINE wdGALResource(const CreationDescription& description)
    : m_Description(description)
  {
  }

  WD_ALWAYS_INLINE const CreationDescription& GetDescription() const { return m_Description; }

protected:
  const CreationDescription m_Description;
};
