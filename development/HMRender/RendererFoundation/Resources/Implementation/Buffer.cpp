#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/Buffer.h>

wdGALBuffer::wdGALBuffer(const wdGALBufferCreationDescription& Description)
  : wdGALResource(Description)
{
}

wdGALBuffer::~wdGALBuffer() {}



WD_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_Buffer);
