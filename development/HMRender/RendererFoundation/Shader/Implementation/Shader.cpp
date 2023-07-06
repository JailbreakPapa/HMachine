#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

wdGALShader::wdGALShader(const wdGALShaderCreationDescription& Description)
  : wdGALObject(Description)
{
}

wdGALShader::~wdGALShader() {}

wdDelegate<void(wdShaderUtils::wdBuiltinShaderType type, wdShaderUtils::wdBuiltinShader& out_shader)> wdShaderUtils::g_RequestBuiltinShaderCallback;

WD_STATICLINK_FILE(RendererFoundation, RendererFoundation_Shader_Implementation_Shader);
