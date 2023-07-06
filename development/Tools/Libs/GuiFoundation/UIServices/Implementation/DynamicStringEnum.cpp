#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

wdMap<wdString, wdDynamicStringEnum> wdDynamicStringEnum::s_DynamicEnums;
wdDelegate<void(wdStringView sEnumName, wdDynamicStringEnum& e)> wdDynamicStringEnum::s_RequestUnknownCallback;

// static
wdDynamicStringEnum& wdDynamicStringEnum::GetDynamicEnum(wdStringView sEnumName)
{
  bool bExisted = false;
  auto it = s_DynamicEnums.FindOrAdd(sEnumName, &bExisted);

  if (!bExisted && s_RequestUnknownCallback.IsValid())
  {
    s_RequestUnknownCallback(sEnumName, it.Value());
  }

  return it.Value();
}

// static
wdDynamicStringEnum& wdDynamicStringEnum::CreateDynamicEnum(wdStringView sEnumName)
{
  bool bExisted = false;
  auto it = s_DynamicEnums.FindOrAdd(sEnumName, &bExisted);

  wdDynamicStringEnum& e = it.Value();
  e.Clear();
  e.SetStorageFile(nullptr);

  return e;
}

// static
void wdDynamicStringEnum::RemoveEnum(wdStringView sEnumName)
{
  s_DynamicEnums.Remove(sEnumName);
}

void wdDynamicStringEnum::Clear()
{
  m_ValidValues.Clear();
}

void wdDynamicStringEnum::AddValidValue(wdStringView sValue, bool bSortValues /*= false*/)
{
  wdString sNewValue = sValue;

  if (!m_ValidValues.Contains(sNewValue))
    m_ValidValues.PushBack(sNewValue);

  if (bSortValues)
    SortValues();
}

void wdDynamicStringEnum::RemoveValue(wdStringView sValue)
{
  m_ValidValues.RemoveAndCopy(sValue);
}

bool wdDynamicStringEnum::IsValueValid(wdStringView sValue) const
{
  return m_ValidValues.Contains(sValue);
}

void wdDynamicStringEnum::SortValues()
{
  m_ValidValues.Sort();
}

void wdDynamicStringEnum::ReadFromStorage()
{
  Clear();

  wdStringBuilder sFile, tmp;

  wdFileReader file;
  if (file.Open(m_sStorageFile).Failed())
    return;

  sFile.ReadAll(file);

  wdHybridArray<wdStringView, 32> values;

  sFile.Split(false, values, "\n", "\r");

  for (auto val : values)
  {
    AddValidValue(val.GetData(tmp));
  }
}

void wdDynamicStringEnum::SaveToStorage()
{
  if (m_sStorageFile.IsEmpty())
    return;

  wdFileWriter file;
  if (file.Open(m_sStorageFile).Failed())
    return;

  wdStringBuilder tmp;

  for (const auto& val : m_ValidValues)
  {
    tmp.Set(val, "\n");
    file.WriteBytes(tmp.GetData(), tmp.GetElementCount()).IgnoreResult();
  }
}
