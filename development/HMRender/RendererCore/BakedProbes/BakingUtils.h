#pragma once

#include <Core/Graphics/AmbientCubeBasis.h>
#include <RendererCore/Declarations.h>

using wdCompressedSkyVisibility = wdUInt32;

namespace wdBakingUtils
{
  WD_RENDERERCORE_DLL wdVec3 FibonacciSphere(wdUInt32 uiSampleIndex, wdUInt32 uiNumSamples);

  WD_RENDERERCORE_DLL wdCompressedSkyVisibility CompressSkyVisibility(const wdAmbientCube<float>& skyVisibility);
  WD_RENDERERCORE_DLL void DecompressSkyVisibility(wdCompressedSkyVisibility compressedSkyVisibility, wdAmbientCube<float>& out_skyVisibility);
} // namespace wdBakingUtils
