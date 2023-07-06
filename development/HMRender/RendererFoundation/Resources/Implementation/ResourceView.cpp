#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/ResourceView.h>


wdGALResourceView::wdGALResourceView(wdGALResourceBase* pResource, const wdGALResourceViewCreationDescription& description)
  : wdGALObject(description)
  , m_pResource(pResource)
{
  WD_ASSERT_DEV(m_pResource != nullptr, "Resource must not be null");
}

wdGALResourceView::~wdGALResourceView() {}



WD_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_ResourceView);
