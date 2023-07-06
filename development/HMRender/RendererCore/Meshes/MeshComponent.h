#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>

struct wdMsgExtractGeometry;
using wdMeshComponentManager = wdComponentManager<class wdMeshComponent, wdBlockStorageType::Compact>;

class WD_RENDERERCORE_DLL wdMeshComponent : public wdMeshComponentBase
{
  WD_DECLARE_COMPONENT_TYPE(wdMeshComponent, wdMeshComponentBase, wdMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // wdMeshComponent

public:
  wdMeshComponent();
  ~wdMeshComponent();

  /// \brief Extracts the render geometry for export etc.
  void OnMsgExtractGeometry(wdMsgExtractGeometry& ref_msg) const; // [ msg handler ]
};
