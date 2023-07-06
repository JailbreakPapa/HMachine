#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/State/State.h>

wdGALBlendState::wdGALBlendState(const wdGALBlendStateCreationDescription& Description)
  : wdGALObject(Description)
{
}

wdGALBlendState::~wdGALBlendState() {}



wdGALDepthStencilState::wdGALDepthStencilState(const wdGALDepthStencilStateCreationDescription& Description)
  : wdGALObject(Description)
{
}

wdGALDepthStencilState::~wdGALDepthStencilState() {}



wdGALRasterizerState::wdGALRasterizerState(const wdGALRasterizerStateCreationDescription& Description)
  : wdGALObject(Description)
{
}

wdGALRasterizerState::~wdGALRasterizerState() {}


wdGALSamplerState::wdGALSamplerState(const wdGALSamplerStateCreationDescription& Description)
  : wdGALObject(Description)
{
}

wdGALSamplerState::~wdGALSamplerState() {}



WD_STATICLINK_FILE(RendererFoundation, RendererFoundation_State_Implementation_State);
