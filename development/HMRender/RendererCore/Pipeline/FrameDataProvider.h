#pragma once

#include <RendererCore/Pipeline/Declarations.h>

class WD_RENDERERCORE_DLL wdFrameDataProviderBase : public wdReflectedClass
{
  WD_ADD_DYNAMIC_REFLECTION(wdFrameDataProviderBase, wdReflectedClass);

protected:
  wdFrameDataProviderBase();

  virtual void* UpdateData(const wdRenderViewContext& renderViewContext, const wdExtractedRenderData& extractedData) = 0;

  void* GetData(const wdRenderViewContext& renderViewContext);

private:
  friend class wdRenderPipeline;

  const wdRenderPipeline* m_pOwnerPipeline = nullptr;
  void* m_pData = nullptr;
  wdUInt64 m_uiLastUpdateFrame = 0;
};

template <typename T>
class wdFrameDataProvider : public wdFrameDataProviderBase
{
public:
  T* GetData(const wdRenderViewContext& renderViewContext) { return static_cast<T*>(wdFrameDataProviderBase::GetData(renderViewContext)); }
};
