#pragma once

#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Types/RangeView.h>

class wdPrefabReferenceComponent;

class WD_CORE_DLL wdPrefabReferenceComponentManager : public wdComponentManager<wdPrefabReferenceComponent, wdBlockStorageType::Compact>
{
public:
  wdPrefabReferenceComponentManager(wdWorld* pWorld);
  ~wdPrefabReferenceComponentManager();

  virtual void Initialize() override;

  void Update(const wdWorldModule::UpdateContext& context);
  void AddToUpdateList(wdPrefabReferenceComponent* pComponent);

private:
  void ResourceEventHandler(const wdResourceEvent& e);

  wdDeque<wdComponentHandle> m_ComponentsToUpdate;
};

class WD_CORE_DLL wdPrefabReferenceComponent : public wdComponent
{
  WD_DECLARE_COMPONENT_TYPE(wdPrefabReferenceComponent, wdComponent, wdPrefabReferenceComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // wdComponent

public:
  virtual void SerializeComponent(wdWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(wdWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // wdPrefabReferenceComponent

public:
  wdPrefabReferenceComponent();
  ~wdPrefabReferenceComponent();

  void SetPrefabFile(const char* szFile); // [ property ]
  const char* GetPrefabFile() const;      // [ property ]

  void SetPrefab(const wdPrefabResourceHandle& hPrefab);                                 // [ property ]
  WD_ALWAYS_INLINE const wdPrefabResourceHandle& GetPrefab() const { return m_hPrefab; } // [ property ]

  const wdRangeView<const char*, wdUInt32> GetParameters() const;   // [ property ] (exposed parameter)
  void SetParameter(const char* szKey, const wdVariant& value);     // [ property ] (exposed parameter)
  void RemoveParameter(const char* szKey);                          // [ property ] (exposed parameter)
  bool GetParameter(const char* szKey, wdVariant& out_value) const; // [ property ] (exposed parameter)

  static void SerializePrefabParameters(const wdWorld& world, wdWorldWriter& inout_stream, wdArrayMap<wdHashedString, wdVariant> parameters);
  static void DeserializePrefabParameters(wdArrayMap<wdHashedString, wdVariant>& out_parameters, wdWorldReader& inout_stream);

private:
  void InstantiatePrefab();
  void ClearPreviousInstances();

  wdPrefabResourceHandle m_hPrefab;
  wdArrayMap<wdHashedString, wdVariant> m_Parameters;
  bool m_bInUpdateList = false;
};
