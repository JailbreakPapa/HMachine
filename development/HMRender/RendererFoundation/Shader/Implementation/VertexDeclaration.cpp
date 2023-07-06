#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Shader/VertexDeclaration.h>

wdGALVertexDeclaration::wdGALVertexDeclaration(const wdGALVertexDeclarationCreationDescription& Description)
  : wdGALObject(Description)
{
}

wdGALVertexDeclaration::~wdGALVertexDeclaration() {}



WD_STATICLINK_FILE(RendererFoundation, RendererFoundation_Shader_Implementation_VertexDeclaration);
