#pragma once

#include <Foundation/Containers/HashSet.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Declarations.h>

/// \brief A helper class to iterate over all possible permutations.
///
/// Just add all permutation variables and their possible values.
/// Then the number of possible permutations and each permutation
/// can be queried.
class WD_RENDERERCORE_DLL wdPermutationGenerator
{
public:
  /// \brief Resets everything.
  void Clear();

  /// \brief Removes all permutations for the given variable
  void RemovePermutations(const wdHashedString& sPermVarName);

  /// \brief Adds the name and one of the possible values of a permutation variable.
  void AddPermutation(const wdHashedString& sName, const wdHashedString& sValue);

  /// \brief Returns how many permutations are possible.
  wdUInt32 GetPermutationCount() const;

  /// \brief Returns the n-th permutation.
  void GetPermutation(wdUInt32 uiPerm, wdHybridArray<wdPermutationVar, 16>& out_permVars) const;

private:
  wdMap<wdHashedString, wdHashSet<wdHashedString>> m_Permutations;
};
