#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Reflection/PropertyPath.h>

using wdPrefabResourceHandle = wdTypedResourceHandle<class wdPrefabResource>;

struct WD_CORE_DLL wdPrefabResourceDescriptor
{
};

struct WD_CORE_DLL wdExposedPrefabParameterDesc
{
  wdHashedString m_sExposeName;
  wdUInt32 m_uiWorldReaderChildObject : 1; // 0 -> use root object array, 1 -> use child object array
  wdUInt32 m_uiWorldReaderObjectIndex : 31;
  wdHashedString m_sComponentType;     // wdRTTI type name to identify which component is meant, empty string -> affects game object
  wdHashedString m_sProperty;          // which property to override
  wdPropertyPath m_CachedPropertyPath; // cached wdPropertyPath to apply a value to the specified property

  void Save(wdStreamWriter& inout_stream) const;
  void Load(wdStreamReader& inout_stream);
  void LoadOld(wdStreamReader& inout_stream);
};

class WD_CORE_DLL wdPrefabResource : public wdResource
{
  WD_ADD_DYNAMIC_REFLECTION(wdPrefabResource, wdResource);
  WD_RESOURCE_DECLARE_COMMON_CODE(wdPrefabResource);
  WD_RESOURCE_DECLARE_CREATEABLE(wdPrefabResource, wdPrefabResourceDescriptor);

public:
  wdPrefabResource();

  enum class InstantiateResult : wdUInt8
  {
    Success,
    NotYetLoaded,
    Error,
  };

  /// \brief Helper function to instantiate a prefab without having to deal with resource acquisition.
  static wdPrefabResource::InstantiateResult InstantiatePrefab(const wdPrefabResourceHandle& hPrefab, bool bBlockTillLoaded, wdWorld& ref_world, const wdTransform& rootTransform, wdPrefabInstantiationOptions options = {}, const wdArrayMap<wdHashedString, wdVariant>* pExposedParamValues = nullptr);

  /// \brief Creates an instance of this prefab in the given world.
  void InstantiatePrefab(wdWorld& ref_world, const wdTransform& rootTransform, wdPrefabInstantiationOptions options, const wdArrayMap<wdHashedString, wdVariant>* pExposedParamValues = nullptr);

  void ApplyExposedParameterValues(const wdArrayMap<wdHashedString, wdVariant>* pExposedParamValues, const wdDynamicArray<wdGameObject*>& createdChildObjects, const wdDynamicArray<wdGameObject*>& createdRootObjects) const;

private:
  virtual wdResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual wdResourceLoadDesc UpdateContent(wdStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  wdUInt32 FindFirstParamWithName(wdUInt64 uiNameHash) const;

  wdWorldReader m_WorldReader;
  wdDynamicArray<wdExposedPrefabParameterDesc> m_PrefabParamDescs;
};
