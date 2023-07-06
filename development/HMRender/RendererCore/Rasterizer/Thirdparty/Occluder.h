#pragma once

#include <memory>
#include <vector>

#include <Foundation/Basics.h>

#define WD_RASTERIZER_SUPPORTED WD_COMPILER_MSVC_PURE

#if WD_ENABLED(WD_RASTERIZER_SUPPORTED)
#  include <intrin.h>
#endif

#if WD_ENABLED(WD_RASTERIZER_SUPPORTED)

struct Occluder
{
  ~Occluder();

  void bake(const __m128* vertices, wdUInt32 numVertices, __m128 refMin, __m128 refMax);

  __m128 m_center;

  __m128 m_refMin;
  __m128 m_refMax;

  __m128 m_boundsMin;
  __m128 m_boundsMax;

  __m256i* m_vertexData = nullptr;
  uint32_t m_packetCount;
};
#else

struct Occluder
{
};

#endif
