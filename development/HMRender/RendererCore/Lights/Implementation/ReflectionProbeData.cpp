#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>

// clang-format off
WD_BEGIN_STATIC_REFLECTED_ENUM(wdReflectionProbeMode, 1)
  WD_BITFLAGS_CONSTANTS(wdReflectionProbeMode::Static, wdReflectionProbeMode::Dynamic)
WD_END_STATIC_REFLECTED_ENUM;

WD_BEGIN_STATIC_REFLECTED_BITFLAGS(wdProbeFlags, 1)
  WD_BITFLAGS_CONSTANTS(wdProbeFlags::SkyLight, wdProbeFlags::HasCustomCubeMap, wdProbeFlags::Sphere, wdProbeFlags::Box, wdProbeFlags::Dynamic)
WD_END_STATIC_REFLECTED_BITFLAGS;

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdReflectionProbeRenderData, 1, wdRTTIDefaultAllocator<wdReflectionProbeRenderData>)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WD_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ReflectionProbeData);
