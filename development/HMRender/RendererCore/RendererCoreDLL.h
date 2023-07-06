#pragma once

#include <Foundation/Basics.h>

// Configure the DLL Import/Export Define
#if WD_ENABLED(WD_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERCORE_LIB
#    define WD_RENDERERCORE_DLL WD_DECL_EXPORT
#  else
#    define WD_RENDERERCORE_DLL WD_DECL_IMPORT
#  endif
#else
#  define WD_RENDERERCORE_DLL
#endif

#define WD_EMBED_FONT_FILE WD_ON
