#pragma once

#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Bitflags.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Shader/Implementation/Helper.h>
#include <RendererCore/Shader/ShaderPermutationBinary.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

// \brief Flags that affect the compilation process of a shader
struct wdShaderCompilerFlags
{
  using StorageType = wdUInt8;
  enum Enum
  {
    Debug = WD_BIT(0),
    Default = 0,
  };

  struct Bits
  {
    StorageType Debug : 1;
  };
};
WD_DECLARE_FLAGS_OPERATORS(wdShaderCompilerFlags);

class WD_RENDERERCORE_DLL wdShaderProgramCompiler : public wdReflectedClass
{
  WD_ADD_DYNAMIC_REFLECTION(wdShaderProgramCompiler, wdReflectedClass);

public:
  struct wdShaderProgramData
  {
    wdShaderProgramData()
    {
      m_szPlatform = nullptr;
      m_szSourceFile = nullptr;

      for (wdUInt32 stage = 0; stage < wdGALShaderStage::ENUM_COUNT; ++stage)
      {
        m_bWriteToDisk[stage] = true;
        m_szShaderSource[stage] = nullptr;
      }
    }

    wdBitflags<wdShaderCompilerFlags> m_Flags;
    const char* m_szPlatform;
    const char* m_szSourceFile;
    const char* m_szShaderSource[wdGALShaderStage::ENUM_COUNT];
    wdShaderStageBinary m_StageBinary[wdGALShaderStage::ENUM_COUNT];
    bool m_bWriteToDisk[wdGALShaderStage::ENUM_COUNT];
  };

  virtual void GetSupportedPlatforms(wdHybridArray<wdString, 4>& ref_platforms) = 0;

  virtual wdResult Compile(wdShaderProgramData& inout_data, wdLogInterface* pLog) = 0;
};

class WD_RENDERERCORE_DLL wdShaderCompiler
{
public:
  wdResult CompileShaderPermutationForPlatforms(
    const char* szFile, const wdArrayPtr<const wdPermutationVar>& permutationVars, wdLogInterface* pLog, const char* szPlatform = "ALL");

private:
  wdResult RunShaderCompiler(const char* szFile, const char* szPlatform, wdShaderProgramCompiler* pCompiler, wdLogInterface* pLog);

  void WriteFailedShaderSource(wdShaderProgramCompiler::wdShaderProgramData& spd, wdLogInterface* pLog);

  bool PassThroughUnknownCommandCB(const char* szCmd) { return wdStringUtils::IsEqual(szCmd, "version"); }

  struct wdShaderData
  {
    wdString m_Platforms;
    wdHybridArray<wdPermutationVar, 16> m_Permutations;
    wdHybridArray<wdPermutationVar, 16> m_FixedPermVars;
    wdString m_StateSource;
    wdString m_ShaderStageSource[wdGALShaderStage::ENUM_COUNT];
  };

  wdResult FileOpen(const char* szAbsoluteFile, wdDynamicArray<wdUInt8>& FileContent, wdTimestamp& out_FileModification);

  wdStringBuilder m_StageSourceFile[wdGALShaderStage::ENUM_COUNT];

  wdTokenizedFileCache m_FileCache;
  wdShaderData m_ShaderData;

  wdSet<wdString> m_IncludeFiles;
};
