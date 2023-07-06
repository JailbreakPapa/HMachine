#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>

// clang-format off
WD_BEGIN_STATIC_REFLECTED_TYPE(wdGALSwapChain, wdNoBase, 1, wdRTTINoAllocator)
{
}
WD_END_STATIC_REFLECTED_TYPE;

WD_BEGIN_STATIC_REFLECTED_TYPE(wdGALWindowSwapChain, wdGALSwapChain, 1, wdRTTINoAllocator)
{
}
WD_END_STATIC_REFLECTED_TYPE;
// clang-format on

wdGALSwapChainCreationDescription CreateSwapChainCreationDescription(const wdRTTI* pType)
{
  wdGALSwapChainCreationDescription desc;
  desc.m_pSwapChainType = pType;
  return desc;
}

wdGALSwapChain::wdGALSwapChain(const wdRTTI* pSwapChainType)
  : wdGALObject(CreateSwapChainCreationDescription(pSwapChainType))
{
}

wdGALSwapChain::~wdGALSwapChain() {}

//////////////////////////////////////////////////////////////////////////

wdGALWindowSwapChain::Functor wdGALWindowSwapChain::s_Factory;


wdGALWindowSwapChain::wdGALWindowSwapChain(const wdGALWindowSwapChainCreationDescription& Description)
  : wdGALSwapChain(wdGetStaticRTTI<wdGALWindowSwapChain>())
  , m_WindowDesc(Description)
{
}

void wdGALWindowSwapChain::SetFactoryMethod(Functor factory)
{
  s_Factory = factory;
}

wdGALSwapChainHandle wdGALWindowSwapChain::Create(const wdGALWindowSwapChainCreationDescription& desc)
{
  WD_ASSERT_DEV(s_Factory.IsValid(), "No factory method assigned for wdGALWindowSwapChain.");
  return s_Factory(desc);
}

WD_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_SwapChain);
