#include <Core/CorePCH.h>

#include <Core/Graphics/AmbientCubeBasis.h>

wdVec3 wdAmbientCubeBasis::s_Dirs[NumDirs] = {wdVec3(1.0f, 0.0f, 0.0f), wdVec3(-1.0f, 0.0f, 0.0f), wdVec3(0.0f, 1.0f, 0.0f),
  wdVec3(0.0f, -1.0f, 0.0f), wdVec3(0.0f, 0.0f, 1.0f), wdVec3(0.0f, 0.0f, -1.0f)};


WD_STATICLINK_FILE(Core, Core_Graphics_Implementation_AmbientCubeBasis);
