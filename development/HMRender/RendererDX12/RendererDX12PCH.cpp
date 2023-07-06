/*
 *   Copyright (c) 2023 Watch Dogs LLC
 *   All rights reserved.
 *   You are only allowed access to this code, if given WRITTEN permission by Watch Dogs LLC.
 */

#include <RendererDX12/RendererDX12PCH.h>

WD_STATICLINK_LIBRARY(RendererDX12)
{
  if (bReturn)
    return;

  WD_STATICLINK_REFERENCE(RenderDX12_Context_Implementation_ContextDX12);
  WD_STATICLINK_REFERENCE(RendererDX12_Device_Implementation_DeviceDX12);
  WD_STATICLINK_REFERENCE(RendererDX12_Device_Implementation_SwapChainDX12);
  WD_STATICLINK_REFERENCE(RendererDX12_Resources_Implementation_ResourceDX12);
  WD_STATICLINK_REFERENCE(RendererDX12_Resources_Implementation_FenceDX12);
  WD_STATICLINK_REFERENCE(RendererDX12_Resources_Implementation_QueryDX12);
  WD_STATICLINK_REFERENCE(RendererDX12_Resources_Implementation_RenderTargetViewDX12);
  WD_STATICLINK_REFERENCE(RendererDX12_Resources_Implementation_ResourceViewDX12);
  WD_STATICLINK_REFERENCE(RendererDX12_Resources_Implementation_TextureDX12);
  WD_STATICLINK_REFERENCE(RendererDX12_Resources_Implementation_UnorderedAccessViewDX12);
  WD_STATICLINK_REFERENCE(RendererDX12_Shader_Implementation_ShaderDX12);
  WD_STATICLINK_REFERENCE(RendererDX12_Shader_Implementation_VertexDeclarationDX12);
  WD_STATICLINK_REFERENCE(RendererDX12_State_Implementation_StateDX12);
}
