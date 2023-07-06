#pragma once

#include <Core/Scripting/DuktapeFunction.h>
#include <Foundation/Memory/CommonAllocators.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

struct duk_hthread;
using duk_context = duk_hthread;
using duk_c_function = int (*)(duk_context*);


class WD_CORE_DLL wdDuktapeContext : public wdDuktapeHelper
{
  WD_DISALLOW_COPY_AND_ASSIGN(wdDuktapeContext);

public:
  wdDuktapeContext(const char* szWrapperName);
  ~wdDuktapeContext();

  /// \name Basics
  ///@{

  /// \brief Enables support for loading modules via the 'require' function
  void EnableModuleSupport(duk_c_function moduleSearchFunction);

  ///@}

private:
  void InitializeContext();
  void DestroyContext();

  static void FatalErrorHandler(void* pUserData, const char* szMsg);
  static void* DukAlloc(void* pUserData, size_t size);
  static void* DukRealloc(void* pUserData, void* pPointer, size_t size);
  static void DukFree(void* pUserData, void* pPointer);

protected:
  bool m_bInitializedModuleSupport = false;

private:
#  if WD_ENABLED(WD_COMPILE_FOR_DEBUG)
  wdAllocator<wdMemoryPolicies::wdHeapAllocation, wdMemoryTrackingFlags::RegisterAllocator | wdMemoryTrackingFlags::EnableAllocationTracking>
    m_Allocator;
#  else
  wdAllocator<wdMemoryPolicies::wdHeapAllocation, wdMemoryTrackingFlags::None> m_Allocator;
#  endif
};

#endif // BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT
