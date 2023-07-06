#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Shader/ShaderByteCode.h>

wdGALShaderByteCode::wdGALShaderByteCode() {}

wdGALShaderByteCode::wdGALShaderByteCode(const wdArrayPtr<const wdUInt8>& byteCode)
{
  CopyFrom(byteCode);
}

void wdGALShaderByteCode::CopyFrom(const wdArrayPtr<const wdUInt8>& pByteCode)
{
  WD_ASSERT_DEV(pByteCode.GetPtr() != nullptr && pByteCode.GetCount() != 0, "Byte code is invalid!");

  m_Source = pByteCode;
}



WD_STATICLINK_FILE(RendererFoundation, RendererFoundation_Shader_Implementation_ShaderByteCode);
