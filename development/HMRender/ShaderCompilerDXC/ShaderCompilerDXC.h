#pragma once

#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <ShaderCompilerDXC/ShaderCompilerDXCDLL.h>

struct SpvReflectDescriptorBinding;

class WD_SHADERCOMPILERDXC_DLL wdShaderCompilerDXC : public wdShaderProgramCompiler
{
  WD_ADD_DYNAMIC_REFLECTION(wdShaderCompilerDXC, wdShaderProgramCompiler);

public:
  virtual void GetSupportedPlatforms(wdHybridArray<wdString, 4>& Platforms) override { Platforms.PushBack("VULKAN"); }

  virtual wdResult Compile(wdShaderProgramData& inout_Data, wdLogInterface* pLog) override;

private:
  wdResult ReflectShaderStage(wdShaderProgramData& inout_Data, wdGALShaderStage::Enum Stage);
  wdShaderConstantBufferLayout* ReflectConstantBufferLayout(wdShaderStageBinary& pStageBinary, const SpvReflectDescriptorBinding& pConstantBufferReflection);
  wdResult FillResourceBinding(wdShaderStageBinary& shaderBinary, wdShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  wdResult FillSRVResourceBinding(wdShaderStageBinary& shaderBinary, wdShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);
  wdResult FillUAVResourceBinding(wdShaderStageBinary& shaderBinary, wdShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info);

  wdResult Initialize();

private:
  wdMap<const char*, wdGALVertexAttributeSemantic::Enum, CompareConstChar> m_VertexInputMapping;
};
