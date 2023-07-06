#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/UnorderedAccesView.h>

wdGALUnorderedAccessView::wdGALUnorderedAccessView(wdGALResourceBase* pResource, const wdGALUnorderedAccessViewCreationDescription& description)
  : wdGALObject(description)
  , m_pResource(pResource)
{
  WD_ASSERT_DEV(m_pResource != nullptr, "Resource must not be null");
}

wdGALUnorderedAccessView::~wdGALUnorderedAccessView() {}

WD_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_UnorderedAccessView);
