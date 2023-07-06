#pragma once

#include <RendererCore/Pipeline/RenderData.h>

/// \brief This is the base class for types that handle rendering of different object types.
///
/// E.g. there are different renderers for meshes, particle effects, light sources, etc.
class WD_RENDERERCORE_DLL wdRenderer : public wdReflectedClass
{
  WD_ADD_DYNAMIC_REFLECTION(wdRenderer, wdReflectedClass);

public:
  virtual void GetSupportedRenderDataTypes(wdHybridArray<const wdRTTI*, 8>& ref_types) const = 0;
  virtual void GetSupportedRenderDataCategories(wdHybridArray<wdRenderData::Category, 8>& ref_categories) const = 0;

  virtual void RenderBatch(const wdRenderViewContext& renderViewContext, const wdRenderPipelinePass* pPass, const wdRenderDataBatch& batch) const = 0;
};
