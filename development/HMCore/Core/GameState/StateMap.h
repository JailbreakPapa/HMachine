#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>

/// \brief A simple registry that stores name/value pairs of types that are common to store game state
///
class WD_CORE_DLL wdStateMap
{
public:
  wdStateMap();
  ~wdStateMap();

  /// void Load(wdStreamReader& stream);
  /// void Save(wdStreamWriter& stream) const;
  /// Lock / Unlock

  void Clear();

  void StoreBool(const wdTempHashedString& sName, bool value);
  void StoreInteger(const wdTempHashedString& sName, wdInt64 value);
  void StoreDouble(const wdTempHashedString& sName, double value);
  void StoreVec3(const wdTempHashedString& sName, const wdVec3& value);
  void StoreColor(const wdTempHashedString& sName, const wdColor& value);
  void StoreString(const wdTempHashedString& sName, const wdString& value);

  void RetrieveBool(const wdTempHashedString& sName, bool& out_bValue, bool bDefaultValue = false);
  void RetrieveInteger(const wdTempHashedString& sName, wdInt64& out_iValue, wdInt64 iDefaultValue = 0);
  void RetrieveDouble(const wdTempHashedString& sName, double& out_fValue, double fDefaultValue = 0);
  void RetrieveVec3(const wdTempHashedString& sName, wdVec3& out_vValue, wdVec3 vDefaultValue = wdVec3(0));
  void RetrieveColor(const wdTempHashedString& sName, wdColor& out_value, wdColor defaultValue = wdColor::White);
  void RetrieveString(const wdTempHashedString& sName, wdString& out_sValue, const char* szDefaultValue = nullptr);

private:
  wdHashTable<wdTempHashedString, bool> m_Bools;
  wdHashTable<wdTempHashedString, wdInt64> m_Integers;
  wdHashTable<wdTempHashedString, double> m_Doubles;
  wdHashTable<wdTempHashedString, wdVec3> m_Vec3s;
  wdHashTable<wdTempHashedString, wdColor> m_Colors;
  wdHashTable<wdTempHashedString, wdString> m_Strings;
};
