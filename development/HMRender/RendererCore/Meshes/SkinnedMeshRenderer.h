#pragma once

#include <RendererCore/Meshes/MeshRenderer.h>

/// \brief Implements rendering of skinned meshes
class WD_RENDERERCORE_DLL wdSkinnedMeshRenderer : public wdMeshRenderer
{
  WD_ADD_DYNAMIC_REFLECTION(wdSkinnedMeshRenderer, wdMeshRenderer);
  WD_DISALLOW_COPY_AND_ASSIGN(wdSkinnedMeshRenderer);

public:
  wdSkinnedMeshRenderer();
  ~wdSkinnedMeshRenderer();

  // wdRenderer implementation
  virtual void GetSupportedRenderDataTypes(wdHybridArray<const wdRTTI*, 8>& ref_types) const override;

protected:
  virtual void SetAdditionalData(const wdRenderViewContext& renderViewContext, const wdMeshRenderData* pRenderData) const override;

  static wdUInt32 s_uiSkinningBufferUpdates;
};
