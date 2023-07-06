#pragma once

#include <Foundation/Profiling/Profiling.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct GPUTimingScope;

/// Sets profiling marker and GPU timings for the current scope.
class WD_RENDERERFOUNDATION_DLL wdProfilingScopeAndMarker : public wdProfilingScope
{
public:
  static GPUTimingScope* Start(wdGALCommandEncoder* pCommandEncoder, const char* szName);
  static void Stop(wdGALCommandEncoder* pCommandEncoder, GPUTimingScope*& ref_pTimingScope);

  wdProfilingScopeAndMarker(wdGALCommandEncoder* pCommandEncoder, const char* szName);

  ~wdProfilingScopeAndMarker();

protected:
  wdGALCommandEncoder* m_pCommandEncoder;
  GPUTimingScope* m_pTimingScope;
};

#if WD_ENABLED(WD_USE_PROFILING) || defined(WD_DOCS)

/// \brief Profiles the current scope using the given name and also inserts a marker with the given GALContext.
#  define WD_PROFILE_AND_MARKER(GALContext, szName) wdProfilingScopeAndMarker WD_CONCAT(_wdProfilingScope, WD_SOURCE_LINE)(GALContext, szName)

#else

#  define WD_PROFILE_AND_MARKER(GALContext, szName) /*empty*/

#endif
