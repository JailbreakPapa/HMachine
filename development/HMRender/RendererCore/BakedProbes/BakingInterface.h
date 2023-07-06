#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Utilities/Progress.h>
#include <RendererCore/RendererCoreDLL.h>

struct WD_RENDERERCORE_DLL wdBakingSettings
{
  wdVec3 m_vProbeSpacing = wdVec3(4);
  wdUInt32 m_uiNumSamplesPerProbe = 128;
  float m_fMaxRayDistance = 1000.0f;

  wdResult Serialize(wdStreamWriter& inout_stream) const;
  wdResult Deserialize(wdStreamReader& inout_stream);
};

WD_DECLARE_REFLECTABLE_TYPE(WD_RENDERERCORE_DLL, wdBakingSettings);

class wdWorld;

class wdBakingInterface
{
public:
  /// \brief Renders a debug view of the baking scene
  virtual wdResult RenderDebugView(const wdWorld& world, const wdMat4& mInverseViewProjection, wdUInt32 uiWidth, wdUInt32 uiHeight, wdDynamicArray<wdColorGammaUB>& out_pixels, wdProgress& ref_progress) const = 0;
};
