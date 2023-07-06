#pragma once

#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <ShaderCompilerHLSL/ShaderCompilerHLSLDLL.h>

struct ID3D11ShaderReflectionConstantBuffer;

class WD_SHADERCOMPILERHLSL_DLL wdShaderCompilerHLSL : public wdShaderProgramCompiler
{
  WD_ADD_DYNAMIC_REFLECTION(wdShaderCompilerHLSL, wdShaderProgramCompiler);

public:
  virtual void GetSupportedPlatforms(wdHybridArray<wdString, 4>& ref_platforms) override
  {
    ref_platforms.PushBack("DX11_SM40_93");
    ref_platforms.PushBack("DX11_SM40");
    ref_platforms.PushBack("DX11_SM41");
    ref_platforms.PushBack("DX11_SM50");
  }

  virtual wdResult Compile(wdShaderProgramData& inout_data, wdLogInterface* pLog) override;

private:
  void ReflectShaderStage(wdShaderProgramData& inout_Data, wdGALShaderStage::Enum Stage);
  wdShaderConstantBufferLayout* ReflectConstantBufferLayout(wdShaderStageBinary& pStageBinary, ID3D11ShaderReflectionConstantBuffer* pConstantBufferReflection);
};
