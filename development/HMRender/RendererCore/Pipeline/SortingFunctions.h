#pragma once

#include <RendererCore/Pipeline/Declarations.h>

class WD_RENDERERCORE_DLL wdRenderSortingFunctions
{
public:
  static wdUInt64 ByRenderDataThenFrontToBack(const wdRenderData* pRenderData, wdUInt32 uiRenderDataSortingKey, const wdCamera& camera);
  static wdUInt64 BackToFrontThenByRenderData(const wdRenderData* pRenderData, wdUInt32 uiRenderDataSortingKey, const wdCamera& camera);
};
