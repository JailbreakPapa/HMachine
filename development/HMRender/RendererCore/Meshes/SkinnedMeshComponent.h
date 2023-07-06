#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Shader/Types.h>
#include <memory>

class wdShaderTransform;

class WD_RENDERERCORE_DLL wdSkinnedMeshRenderData : public wdMeshRenderData
{
  WD_ADD_DYNAMIC_REFLECTION(wdSkinnedMeshRenderData, wdMeshRenderData);

public:
  virtual void FillBatchIdAndSortingKey() override;
  wdGALBufferHandle m_hSkinningTransforms;
  wdArrayPtr<const wdUInt8> m_pNewSkinningTransformData;
  std::shared_ptr<bool> m_bTransformsUpdated;
};

struct WD_RENDERERCORE_DLL wdSkinningState
{
  wdSkinningState();
  ~wdSkinningState();

  void Clear();

  /// \brief Holds the current CPU-side copy of the skinning matrices. Modify these and call TransformsChanged() to send them to the GPU.
  wdDynamicArray<wdShaderTransform, wdAlignedAllocatorWrapper> m_Transforms;

  /// \brief Call this, after modifying m_Transforms, to make the renderer apply the update.
  void TransformsChanged();

  void FillSkinnedMeshRenderData(wdSkinnedMeshRenderData& ref_renderData) const;

private:
  wdGALBufferHandle m_hGpuBuffer;
  std::shared_ptr<bool> m_bTransformsUpdated[2];
};
