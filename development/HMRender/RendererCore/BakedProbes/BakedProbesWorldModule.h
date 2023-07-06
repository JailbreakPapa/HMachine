#pragma once

#include <Core/Graphics/AmbientCubeBasis.h>
#include <Core/World/WorldModule.h>
#include <RendererCore/Declarations.h>

using wdProbeTreeSectorResourceHandle = wdTypedResourceHandle<class wdProbeTreeSectorResource>;

class WD_RENDERERCORE_DLL wdBakedProbesWorldModule : public wdWorldModule
{
  WD_DECLARE_WORLD_MODULE();
  WD_ADD_DYNAMIC_REFLECTION(wdBakedProbesWorldModule, wdWorldModule);

public:
  wdBakedProbesWorldModule(wdWorld* pWorld);
  ~wdBakedProbesWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  bool HasProbeData() const;

  struct ProbeIndexData
  {
    static constexpr wdUInt32 NumProbes = 8;
    wdUInt32 m_probeIndices[NumProbes];
    float m_probeWeights[NumProbes];
  };

  wdResult GetProbeIndexData(const wdVec3& vGlobalPosition, const wdVec3& vNormal, ProbeIndexData& out_probeIndexData) const;

  wdAmbientCube<float> GetSkyVisibility(const ProbeIndexData& indexData) const;

private:
  friend class wdBakedProbesComponent;

  void SetProbeTreeResourcePrefix(const wdHashedString& prefix);

  wdProbeTreeSectorResourceHandle m_hProbeTree;
};
