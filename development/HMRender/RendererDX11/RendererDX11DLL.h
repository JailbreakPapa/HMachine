#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/RendererFoundationDLL.h>

// Configure the DLL Import/Export Define
#if WD_ENABLED(WD_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERDX11_LIB
#    define WD_RENDERERDX11_DLL WD_DECL_EXPORT
#  else
#    define WD_RENDERERDX11_DLL WD_DECL_IMPORT
#  endif
#else
#  define WD_RENDERERDX11_DLL
#endif


#define WD_GAL_DX11_RELEASE(d3dobj)                                                                                                                  \
  do                                                                                                                                                 \
  {                                                                                                                                                  \
    if ((d3dobj) != nullptr)                                                                                                                         \
    {                                                                                                                                                \
      (d3dobj)->Release();                                                                                                                           \
      (d3dobj) = nullptr;                                                                                                                            \
    }                                                                                                                                                \
  } while (0)
