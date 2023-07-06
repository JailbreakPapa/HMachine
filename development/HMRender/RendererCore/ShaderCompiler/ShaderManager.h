#pragma once

#include <Foundation/Containers/HashTable.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>

class WD_RENDERERCORE_DLL wdShaderManager
{
public:
  static void Configure(const char* szActivePlatform, bool bEnableRuntimeCompilation, const char* szShaderCacheDirectory = ":shadercache/ShaderCache",
    const char* szPermVarSubDirectory = "Shaders/PermutationVars");
  static const wdString& GetPermutationVarSubDirectory() { return s_sPermVarSubDir; }
  static const wdString& GetActivePlatform() { return s_sPlatform; }
  static const wdString& GetCacheDirectory() { return s_sShaderCacheDirectory; }
  static bool IsRuntimeCompilationEnabled() { return s_bEnableRuntimeCompilation; }

  static void ReloadPermutationVarConfig(const char* szName, const wdTempHashedString& sHashedName);
  static bool IsPermutationValueAllowed(const char* szName, const wdTempHashedString& sHashedName, const wdTempHashedString& sValue,
    wdHashedString& out_sName, wdHashedString& out_sValue);
  static bool IsPermutationValueAllowed(const wdHashedString& sName, const wdHashedString& sValue);

  /// \brief If the given permutation variable is an enum variable, this returns the possible values.
  /// Returns an empty array for other types of permutation variables.
  static wdArrayPtr<const wdShaderParser::EnumValue> GetPermutationEnumValues(const wdHashedString& sName);

  /// \brief Same as GetPermutationEnumValues() but also returns values for other types of variables.
  /// E.g. returns TRUE and FALSE for boolean variables.
  static void GetPermutationValues(const wdHashedString& sName, wdDynamicArray<wdHashedString>& out_values);

  static void PreloadPermutations(
    wdShaderResourceHandle hShader, const wdHashTable<wdHashedString, wdHashedString>& permVars, wdTime shouldBeAvailableIn);
  static wdShaderPermutationResourceHandle PreloadSinglePermutation(
    wdShaderResourceHandle hShader, const wdHashTable<wdHashedString, wdHashedString>& permVars, bool bAllowFallback);

private:
  static wdUInt32 FilterPermutationVars(wdArrayPtr<const wdHashedString> usedVars, const wdHashTable<wdHashedString, wdHashedString>& permVars,
    wdDynamicArray<wdPermutationVar>& out_FilteredPermutationVariables);
  static wdShaderPermutationResourceHandle PreloadSinglePermutationInternal(
    const char* szResourceId, wdUInt64 uiResourceIdHash, wdUInt32 uiPermutationHash, wdArrayPtr<wdPermutationVar> filteredPermutationVariables);

  static bool s_bEnableRuntimeCompilation;
  static wdString s_sPlatform;
  static wdString s_sPermVarSubDir;
  static wdString s_sShaderCacheDirectory;
};
