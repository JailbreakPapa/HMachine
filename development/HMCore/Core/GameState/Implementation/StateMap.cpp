#include <Core/CorePCH.h>

#include <Core/GameState/StateMap.h>

wdStateMap::wdStateMap() = default;
wdStateMap::~wdStateMap() = default;


void wdStateMap::Clear()
{
  m_Bools.Clear();
  m_Integers.Clear();
  m_Doubles.Clear();
  m_Vec3s.Clear();
  m_Colors.Clear();
  m_Strings.Clear();
}

void wdStateMap::StoreBool(const wdTempHashedString& sName, bool value)
{
  m_Bools[sName] = value;
}

void wdStateMap::StoreInteger(const wdTempHashedString& sName, wdInt64 value)
{
  m_Integers[sName] = value;
}

void wdStateMap::StoreDouble(const wdTempHashedString& sName, double value)
{
  m_Doubles[sName] = value;
}

void wdStateMap::StoreVec3(const wdTempHashedString& sName, const wdVec3& value)
{
  m_Vec3s[sName] = value;
}

void wdStateMap::StoreColor(const wdTempHashedString& sName, const wdColor& value)
{
  m_Colors[sName] = value;
}

void wdStateMap::StoreString(const wdTempHashedString& sName, const wdString& value)
{
  m_Strings[sName] = value;
}

void wdStateMap::RetrieveBool(const wdTempHashedString& sName, bool& out_bValue, bool bDefaultValue /*= false*/)
{
  if (!m_Bools.TryGetValue(sName, out_bValue))
  {
    out_bValue = bDefaultValue;
  }
}

void wdStateMap::RetrieveInteger(const wdTempHashedString& sName, wdInt64& out_iValue, wdInt64 iDefaultValue /*= 0*/)
{
  if (!m_Integers.TryGetValue(sName, out_iValue))
  {
    out_iValue = iDefaultValue;
  }
}

void wdStateMap::RetrieveDouble(const wdTempHashedString& sName, double& out_fValue, double fDefaultValue /*= 0*/)
{
  if (!m_Doubles.TryGetValue(sName, out_fValue))
  {
    out_fValue = fDefaultValue;
  }
}

void wdStateMap::RetrieveVec3(const wdTempHashedString& sName, wdVec3& out_vValue, wdVec3 vDefaultValue /*= wdVec3(0)*/)
{
  if (!m_Vec3s.TryGetValue(sName, out_vValue))
  {
    out_vValue = vDefaultValue;
  }
}

void wdStateMap::RetrieveColor(const wdTempHashedString& sName, wdColor& out_value, wdColor defaultValue /*= wdColor::White*/)
{
  if (!m_Colors.TryGetValue(sName, out_value))
  {
    out_value = defaultValue;
  }
}

void wdStateMap::RetrieveString(const wdTempHashedString& sName, wdString& out_sValue, const char* szDefaultValue /*= nullptr*/)
{
  if (!m_Strings.TryGetValue(sName, out_sValue))
  {
    out_sValue = szDefaultValue;
  }
}



WD_STATICLINK_FILE(Core, Core_GameState_Implementation_StateMap);
