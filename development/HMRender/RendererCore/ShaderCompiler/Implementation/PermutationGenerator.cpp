#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/ShaderCompiler/PermutationGenerator.h>

void wdPermutationGenerator::Clear()
{
  m_Permutations.Clear();
}


void wdPermutationGenerator::RemovePermutations(const wdHashedString& sPermVarName)
{
  m_Permutations.Remove(sPermVarName);
}

void wdPermutationGenerator::AddPermutation(const wdHashedString& sName, const wdHashedString& sValue)
{
  WD_ASSERT_DEV(!sName.IsEmpty(), "");
  WD_ASSERT_DEV(!sValue.IsEmpty(), "");

  m_Permutations[sName].Insert(sValue);
}

wdUInt32 wdPermutationGenerator::GetPermutationCount() const
{
  wdUInt32 uiPermutations = 1;

  for (auto it = m_Permutations.GetIterator(); it.IsValid(); ++it)
  {
    uiPermutations *= it.Value().GetCount();
  }

  return uiPermutations;
}

void wdPermutationGenerator::GetPermutation(wdUInt32 uiPerm, wdHybridArray<wdPermutationVar, 16>& out_permVars) const
{
  out_permVars.Clear();

  for (auto itVariable = m_Permutations.GetIterator(); itVariable.IsValid(); ++itVariable)
  {
    const wdUInt32 uiValues = itVariable.Value().GetCount();
    wdUInt32 uiUseValue = uiPerm % uiValues;

    uiPerm /= uiValues;

    auto itValue = itVariable.Value().GetIterator();

    for (; uiUseValue > 0; --uiUseValue)
    {
      ++itValue;
    }

    wdPermutationVar& pv = out_permVars.ExpandAndGetRef();
    pv.m_sName = itVariable.Key();
    pv.m_sValue = itValue.Key();
  }
}

WD_STATICLINK_FILE(RendererCore, RendererCore_ShaderCompiler_Implementation_PermutationGenerator);
