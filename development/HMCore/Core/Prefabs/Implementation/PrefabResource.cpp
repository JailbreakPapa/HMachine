#include <Core/CorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Foundation/Reflection/PropertyPath.h>
#include <Foundation/Reflection/ReflectionUtils.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdPrefabResource, 1, wdRTTIDefaultAllocator<wdPrefabResource>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_RESOURCE_IMPLEMENT_COMMON_CODE(wdPrefabResource);
// clang-format on

wdPrefabResource::wdPrefabResource()
  : wdResource(DoUpdate::OnAnyThread, 1)
{
}

void wdPrefabResource::InstantiatePrefab(wdWorld& ref_world, const wdTransform& rootTransform, wdPrefabInstantiationOptions options, const wdArrayMap<wdHashedString, wdVariant>* pExposedParamValues)
{
  if (GetLoadingState() != wdResourceState::Loaded)
    return;

  if (pExposedParamValues != nullptr && !pExposedParamValues->IsEmpty())
  {
    wdHybridArray<wdGameObject*, 8> createdRootObjects;
    wdHybridArray<wdGameObject*, 8> createdChildObjects;

    if (options.m_pCreatedRootObjectsOut == nullptr)
    {
      options.m_pCreatedRootObjectsOut = &createdRootObjects;
    }

    if (options.m_pCreatedChildObjectsOut == nullptr)
    {
      options.m_pCreatedChildObjectsOut = &createdChildObjects;
    }

    m_WorldReader.InstantiatePrefab(ref_world, rootTransform, options);

    WD_ASSERT_DEBUG(options.m_pCreatedRootObjectsOut != options.m_pCreatedChildObjectsOut, "These pointers must point to different arrays, otherwise applying exposed properties doesn't work correctly.");
    ApplyExposedParameterValues(pExposedParamValues, *options.m_pCreatedChildObjectsOut, *options.m_pCreatedRootObjectsOut);
  }
  else
  {
    m_WorldReader.InstantiatePrefab(ref_world, rootTransform, options);
  }
}

wdPrefabResource::InstantiateResult wdPrefabResource::InstantiatePrefab(const wdPrefabResourceHandle& hPrefab, bool bBlockTillLoaded, wdWorld& ref_world, const wdTransform& rootTransform, wdPrefabInstantiationOptions options, const wdArrayMap<wdHashedString, wdVariant>* pExposedParamValues /*= nullptr*/)
{
  wdResourceLock<wdPrefabResource> pPrefab(hPrefab, bBlockTillLoaded ? wdResourceAcquireMode::BlockTillLoaded_NeverFail : wdResourceAcquireMode::AllowLoadingFallback_NeverFail);

  switch (pPrefab.GetAcquireResult())
  {
    case wdResourceAcquireResult::Final:
      pPrefab->InstantiatePrefab(ref_world, rootTransform, options, pExposedParamValues);
      return InstantiateResult::Success;

    case wdResourceAcquireResult::LoadingFallback:
      return InstantiateResult::NotYetLoaded;

    default:
      return InstantiateResult::Error;
  }
}

void wdPrefabResource::ApplyExposedParameterValues(const wdArrayMap<wdHashedString, wdVariant>* pExposedParamValues, const wdDynamicArray<wdGameObject*>& createdChildObjects, const wdDynamicArray<wdGameObject*>& createdRootObjects) const
{
  const wdUInt32 uiNumParamDescs = m_PrefabParamDescs.GetCount();

  for (wdUInt32 i = 0; i < pExposedParamValues->GetCount(); ++i)
  {
    const wdHashedString& name = pExposedParamValues->GetKey(i);
    const wdUInt64 uiNameHash = name.GetHash();

    for (wdUInt32 uiCurParam = FindFirstParamWithName(uiNameHash); uiCurParam < uiNumParamDescs; ++uiCurParam)
    {
      const auto& ppd = m_PrefabParamDescs[uiCurParam];

      if (ppd.m_sExposeName.GetHash() != uiNameHash)
        break;

      wdGameObject* pTarget = ppd.m_uiWorldReaderChildObject ? createdChildObjects[ppd.m_uiWorldReaderObjectIndex] : createdRootObjects[ppd.m_uiWorldReaderObjectIndex];

      if (ppd.m_CachedPropertyPath.IsValid())
      {
        if (ppd.m_sComponentType.IsEmpty())
        {
          ppd.m_CachedPropertyPath.SetValue(pTarget, pExposedParamValues->GetValue(i));
        }
        else
        {
          for (wdComponent* pComp : pTarget->GetComponents())
          {
            const wdRTTI* pRtti = pComp->GetDynamicRTTI();

            // TODO: use component index instead
            // atm if the same component type is attached multiple times, they will all get the value applied
            if (pRtti->GetTypeNameHash() == ppd.m_sComponentType.GetHash())
            {
              ppd.m_CachedPropertyPath.SetValue(pComp, pExposedParamValues->GetValue(i));
            }
          }
        }
      }

      // Allow to bind multiple properties to the same exposed parameter name
      // Therefore, do not break here, but continue iterating
    }
  }
}

wdResourceLoadDesc wdPrefabResource::UnloadData(Unload WhatToUnload)
{
  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::Unloaded;

  if (WhatToUnload == wdResource::Unload::AllQualityLevels)
  {
    m_WorldReader.ClearAndCompact();
  }

  return res;
}

wdResourceLoadDesc wdPrefabResource::UpdateContent(wdStreamReader* Stream)
{
  WD_LOG_BLOCK("wdPrefabResource::UpdateContent", GetResourceDescription().GetData());

  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = wdResourceState::LoadedResourceMissing;
    return res;
  }

  wdStreamReader& s = *Stream;

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    wdString sAbsFilePath;
    s >> sAbsFilePath;
  }

  wdAssetFileHeader assetHeader;
  assetHeader.Read(s).IgnoreResult();

  char szSceneTag[16];
  s.ReadBytes(szSceneTag, sizeof(char) * 16);
  WD_ASSERT_DEV(wdStringUtils::IsEqualN(szSceneTag, "[wdBinaryScene]", 16), "The given file is not a valid prefab file");

  if (!wdStringUtils::IsEqualN(szSceneTag, "[wdBinaryScene]", 16))
  {
    res.m_State = wdResourceState::LoadedResourceMissing;
    return res;
  }

  m_WorldReader.ReadWorldDescription(s).IgnoreResult();

  if (assetHeader.GetFileVersion() >= 4)
  {
    wdUInt32 uiExposedParams = 0;

    s >> uiExposedParams;

    m_PrefabParamDescs.SetCount(uiExposedParams);

    for (wdUInt32 i = 0; i < uiExposedParams; ++i)
    {
      auto& ppd = m_PrefabParamDescs[i];

      if (assetHeader.GetFileVersion() < 6)
        ppd.LoadOld(s);
      else
        ppd.Load(s);

      // initialize the cached property path here once
      // so we can only apply it later as often as needed
      {
        if (ppd.m_sComponentType.IsEmpty())
        {
          ppd.m_CachedPropertyPath.InitializeFromPath(*wdGetStaticRTTI<wdGameObject>(), ppd.m_sProperty).IgnoreResult();
        }
        else
        {
          for (const wdRTTI* pRtti = wdRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
          {
            if (pRtti->GetTypeNameHash() == ppd.m_sComponentType.GetHash())
            {
              ppd.m_CachedPropertyPath.InitializeFromPath(*pRtti, ppd.m_sProperty).IgnoreResult();
              break;
            }
          }
        }
      }
    }

    // sort exposed parameter descriptions by name hash for quicker access
    m_PrefabParamDescs.Sort([](const wdExposedPrefabParameterDesc& lhs, const wdExposedPrefabParameterDesc& rhs) -> bool { return lhs.m_sExposeName.GetHash() < rhs.m_sExposeName.GetHash(); });
  }

  res.m_State = wdResourceState::Loaded;
  return res;
}

void wdPrefabResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = m_WorldReader.GetHeapMemoryUsage() + sizeof(this);
}

WD_RESOURCE_IMPLEMENT_CREATEABLE(wdPrefabResource, wdPrefabResourceDescriptor)
{
  wdResourceLoadDesc desc;
  desc.m_State = wdResourceState::Loaded;
  desc.m_uiQualityLevelsDiscardable = 0;
  desc.m_uiQualityLevelsLoadable = 0;
  return desc;
}

wdUInt32 wdPrefabResource::FindFirstParamWithName(wdUInt64 uiNameHash) const
{
  wdUInt32 lb = 0;
  wdUInt32 ub = m_PrefabParamDescs.GetCount();

  while (lb < ub)
  {
    const wdUInt32 middle = lb + ((ub - lb) >> 1);

    if (m_PrefabParamDescs[middle].m_sExposeName.GetHash() < uiNameHash)
    {
      lb = middle + 1;
    }
    else
    {
      ub = middle;
    }
  }

  return lb;
}

void wdExposedPrefabParameterDesc::Save(wdStreamWriter& inout_stream) const
{
  wdUInt32 comb = m_uiWorldReaderObjectIndex | (m_uiWorldReaderChildObject << 31);

  inout_stream << m_sExposeName;
  inout_stream << comb;
  inout_stream << m_sComponentType;
  inout_stream << m_sProperty;
}

void wdExposedPrefabParameterDesc::Load(wdStreamReader& inout_stream)
{
  wdUInt32 comb = 0;

  inout_stream >> m_sExposeName;
  inout_stream >> comb;
  inout_stream >> m_sComponentType;
  inout_stream >> m_sProperty;

  m_uiWorldReaderObjectIndex = comb & 0x7FFFFFFF;
  m_uiWorldReaderChildObject = (comb >> 31);
}

void wdExposedPrefabParameterDesc::LoadOld(wdStreamReader& inout_stream)
{
  wdUInt32 comb = 0;

  wdUInt32 uiComponentTypeMurmurHash;

  inout_stream >> m_sExposeName;
  inout_stream >> comb;
  inout_stream >> uiComponentTypeMurmurHash;
  inout_stream >> m_sProperty;

  m_sComponentType.Clear();
  if (uiComponentTypeMurmurHash != 0)
  {
    for (const wdRTTI* pRtti = wdRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
    {
      if (wdHashingUtils::MurmurHash32String(pRtti->GetTypeName()) == uiComponentTypeMurmurHash)
      {
        m_sComponentType.Assign(pRtti->GetTypeName());
        break;
      }
    }
  }

  m_uiWorldReaderObjectIndex = comb & 0x7FFFFFFF;
  m_uiWorldReaderChildObject = (comb >> 31);
}

WD_STATICLINK_FILE(Core, Core_Prefabs_Implementation_PrefabResource);
