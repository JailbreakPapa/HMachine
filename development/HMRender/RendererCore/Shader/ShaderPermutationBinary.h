#pragma once

#include <Foundation/IO/DependencyFile.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Shader/ShaderStageBinary.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

struct WD_RENDERERCORE_DLL wdShaderStateResourceDescriptor
{
  wdGALBlendStateCreationDescription m_BlendDesc;
  wdGALDepthStencilStateCreationDescription m_DepthStencilDesc;
  wdGALRasterizerStateCreationDescription m_RasterizerDesc;

  wdResult Load(const char* szSource);
  void Load(wdStreamReader& inout_stream);
  void Save(wdStreamWriter& inout_stream) const;

  wdUInt32 CalculateHash() const;
};

class WD_RENDERERCORE_DLL wdShaderPermutationBinary
{
public:
  wdShaderPermutationBinary();

  wdResult Write(wdStreamWriter& inout_stream);
  wdResult Read(wdStreamReader& inout_stream, bool& out_bOldVersion);

  wdUInt32 m_uiShaderStageHashes[wdGALShaderStage::ENUM_COUNT];

  wdDependencyFile m_DependencyFile;

  wdShaderStateResourceDescriptor m_StateDescriptor;

  wdHybridArray<wdPermutationVar, 16> m_PermutationVars;
};
