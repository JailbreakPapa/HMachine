#include <Foundation/FoundationPCH.h>

#if WD_ENABLED(WD_PLATFORM_WINDOWS)
#  include <Foundation/System/Implementation/Win/MiniDumpUtils_win.h>
#elif WD_ENABLED(WD_PLATFORM_OSX)
#  include <Foundation/System/Implementation/OSX/MiniDumpUtils_OSX.h>
#elif WD_ENABLED(WD_PLATFORM_LINUX) || WD_ENABLED(WD_PLATFORM_ANDROID)
#  include <Foundation/System/Implementation/Posix/MiniDumpUtils_posix.h>
#else
#  error "Mini-dump functions are not implemented on current platform"
#endif



WD_STATICLINK_FILE(Foundation, Foundation_System_Implementation_MiniDumpUtils);
