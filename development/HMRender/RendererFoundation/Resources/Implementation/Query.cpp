#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/Query.h>

wdGALQuery::wdGALQuery(const wdGALQueryCreationDescription& Description)
  : wdGALResource<wdGALQueryCreationDescription>(Description)
  , m_bStarted(false)
{
}

wdGALQuery::~wdGALQuery() {}

WD_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_Query);
