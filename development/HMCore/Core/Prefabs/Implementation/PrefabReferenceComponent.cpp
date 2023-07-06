#include <Core/CorePCH.h>

#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
WD_BEGIN_COMPONENT_TYPE(wdPrefabReferenceComponent, 4, wdComponentMode::Static)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("Prefab", GetPrefabFile, SetPrefabFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Prefab")),
    WD_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new wdExposedParametersAttribute("Prefab")),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("General"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

enum PrefabComponentFlags
{
  SelfDeletion = 1
};

wdPrefabReferenceComponent::wdPrefabReferenceComponent() = default;
wdPrefabReferenceComponent::~wdPrefabReferenceComponent() = default;

void wdPrefabReferenceComponent::SerializePrefabParameters(const wdWorld& world, wdWorldWriter& inout_stream, wdArrayMap<wdHashedString, wdVariant> parameters)
{
  // we need a copy of the parameters here, therefore we don't take it by reference

  auto& s = inout_stream.GetStream();
  const wdUInt32 numParams = parameters.GetCount();

  wdHybridArray<wdGameObjectHandle, 8> GoReferences;

  // Version 4
  {
    // to support game object references as exposed parameters (which are currently exposed as strings)
    // we need to remap the string from an 'editor uuid' to something that can be interpreted as a proper wdGameObjectHandle at runtime

    // so first we get the resolver and try to map any string parameter to a valid wdGameObjectHandle
    auto resolver = world.GetGameObjectReferenceResolver();

    if (resolver.IsValid())
    {
      wdStringBuilder tmp;

      for (wdUInt32 i = 0; i < numParams; ++i)
      {
        // if this is a string parameter
        wdVariant& var = parameters.GetValue(i);
        if (var.IsA<wdString>())
        {
          // and the resolver CAN map this string to a game object handle
          wdGameObjectHandle hObject = resolver(var.Get<wdString>().GetData(), wdComponentHandle(), nullptr);
          if (!hObject.IsInvalidated())
          {
            // write the handle properly to file (this enables correct remapping during deserialization)
            // and discard the string's value, and instead write a string that specifies the index of the serialized handle to use

            // local game object reference - index into GoReferences
            tmp.Format("#!LGOR-{}", GoReferences.GetCount());
            var = tmp.GetData();

            GoReferences.PushBack(hObject);
          }
        }
      }
    }

    // now write all the wdGameObjectHandle's such that during deserialization the wdWorldReader will remap it as needed
    const wdUInt8 numRefs = static_cast<wdUInt8>(GoReferences.GetCount());
    s << numRefs;

    for (wdUInt8 i = 0; i < numRefs; ++i)
    {
      inout_stream.WriteGameObjectHandle(GoReferences[i]);
    }
  }

  // Version 2
  s << numParams;
  for (wdUInt32 i = 0; i < numParams; ++i)
  {
    s << parameters.GetKey(i);
    s << parameters.GetValue(i); // this may contain modified strings now, to map the game object handle references
  }
}

void wdPrefabReferenceComponent::DeserializePrefabParameters(wdArrayMap<wdHashedString, wdVariant>& out_parameters, wdWorldReader& inout_stream)
{
  out_parameters.Clear();

  // versioning of this stuff is tied to the version number of wdPrefabReferenceComponent
  const wdUInt32 uiVersion = inout_stream.GetComponentTypeVersion(wdGetStaticRTTI<wdPrefabReferenceComponent>());
  auto& s = inout_stream.GetStream();

  // temp array to hold (and remap) the serialized game object handles
  wdHybridArray<wdGameObjectHandle, 8> GoReferences;

  if (uiVersion >= 4)
  {
    wdUInt8 numRefs = 0;
    s >> numRefs;
    GoReferences.SetCountUninitialized(numRefs);

    // just read them all, this will remap as necessary to the wdWorldReader
    for (wdUInt8 i = 0; i < numRefs; ++i)
    {
      GoReferences[i] = inout_stream.ReadGameObjectHandle();
    }
  }

  if (uiVersion >= 2)
  {
    wdUInt32 numParams = 0;
    s >> numParams;

    out_parameters.Reserve(numParams);

    wdHashedString key;
    wdVariant value;
    wdStringBuilder tmp;

    for (wdUInt32 i = 0; i < numParams; ++i)
    {
      s >> key;
      s >> value;

      if (value.IsA<wdString>())
      {
        // if we find a string parameter, check if it is a 'local game object reference'
        const wdString& str = value.Get<wdString>();
        if (str.StartsWith("#!LGOR-"))
        {
          // if so, extract the index into the GoReferences array
          wdInt32 idx;
          if (wdConversionUtils::StringToInt(str.GetData() + 7, idx).Succeeded())
          {
            // now we can lookup the remapped wdGameObjectHandle from our array
            const wdGameObjectHandle hObject = GoReferences[idx];

            // and stringify the handle into a 'global game object reference', ie. one that contains the internal integer data of the handle
            // a regular runtime world has a reference resolver that is capable to reverse this stringified format to a handle again
            // which will happen once 'InstantiatePrefab' passes the m_Parameters list to the newly created objects
            tmp.Format("#!GGOR-{}", hObject.GetInternalID().m_Data);

            // map local game object reference to global game object reference
            value = tmp.GetData();
          }
        }
      }

      out_parameters.Insert(key, value);
    }
  }
}

void wdPrefabReferenceComponent::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hPrefab;

  wdPrefabReferenceComponent::SerializePrefabParameters(*GetWorld(), inout_stream, m_Parameters);
}

void wdPrefabReferenceComponent::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const wdUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_hPrefab;

  if (uiVersion < 3)
  {
    bool bDummy;
    s >> bDummy;
  }

  wdPrefabReferenceComponent::DeserializePrefabParameters(m_Parameters, inout_stream);
}

void wdPrefabReferenceComponent::SetPrefabFile(const char* szFile)
{
  wdPrefabResourceHandle hResource;

  if (!wdStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = wdResourceManager::LoadResource<wdPrefabResource>(szFile);
    wdResourceManager::PreloadResource(hResource);
  }

  SetPrefab(hResource);
}

const char* wdPrefabReferenceComponent::GetPrefabFile() const
{
  if (!m_hPrefab.IsValid())
    return "";

  return m_hPrefab.GetResourceID();
}

void wdPrefabReferenceComponent::SetPrefab(const wdPrefabResourceHandle& hPrefab)
{
  if (m_hPrefab == hPrefab)
    return;

  m_hPrefab = hPrefab;

  if (IsActiveAndInitialized())
  {
    // only add to update list, if not yet activated,
    // since OnActivate will do the instantiation anyway

    GetWorld()->GetComponentManager<wdPrefabReferenceComponentManager>()->AddToUpdateList(this);
  }
}

void wdPrefabReferenceComponent::InstantiatePrefab()
{
  // now instantiate the prefab
  if (m_hPrefab.IsValid())
  {
    wdResourceLock<wdPrefabResource> pResource(m_hPrefab, wdResourceAcquireMode::AllowLoadingFallback);

    wdTransform id;
    id.SetIdentity();

    wdPrefabInstantiationOptions options;
    options.m_hParent = GetOwner()->GetHandle();
    options.m_ReplaceNamedRootWithParent = "<Prefab-Root>";
    options.m_pOverrideTeamID = &GetOwner()->GetTeamID();

    // if this ID is valid, this prefab is instantiated at editor runtime
    // replicate the same ID across all instantiated sub components to get correct picking behavior
    if (GetUniqueID() != wdInvalidIndex)
    {
      wdHybridArray<wdGameObject*, 8> createdRootObjects;
      wdHybridArray<wdGameObject*, 16> createdChildObjects;

      options.m_pCreatedRootObjectsOut = &createdRootObjects;
      options.m_pCreatedChildObjectsOut = &createdChildObjects;

      wdUInt32 uiPrevCompCount = GetOwner()->GetComponents().GetCount();

      pResource->InstantiatePrefab(*GetWorld(), id, options, &m_Parameters);

      auto FixComponent = [](wdGameObject* pChild, wdUInt32 uiUniqueID) {
        // while exporting a scene all game objects with this flag are ignored and not exported
        // set this flag on all game objects that were created by instantiating this prefab
        // instead it should be instantiated at runtime again
        // only do this at editor time though, at regular runtime we do want to fully serialize the entire sub tree
        pChild->SetCreatedByPrefab();

        for (auto pComponent : pChild->GetComponents())
        {
          pComponent->SetUniqueID(uiUniqueID);
          pComponent->SetCreatedByPrefab();
        }
      };

      const wdUInt32 uiUniqueID = GetUniqueID();

      for (wdGameObject* pChild : createdRootObjects)
      {
        FixComponent(pChild, uiUniqueID);
      }

      for (wdGameObject* pChild : createdChildObjects)
      {
        FixComponent(pChild, uiUniqueID);
      }

      for (; uiPrevCompCount < GetOwner()->GetComponents().GetCount(); ++uiPrevCompCount)
      {
        GetOwner()->GetComponents()[uiPrevCompCount]->SetUniqueID(GetUniqueID());
        GetOwner()->GetComponents()[uiPrevCompCount]->SetCreatedByPrefab();
      }
    }
    else
    {
      pResource->InstantiatePrefab(*GetWorld(), id, options, &m_Parameters);
    }
  }
}

void wdPrefabReferenceComponent::OnActivated()
{
  SUPER::OnActivated();

  // instantiate the prefab right away, such that game play code can access it as soon as possible
  // additionally the manager may update the instance later on, to properly enable editor work flows
  InstantiatePrefab();
}

void wdPrefabReferenceComponent::OnDeactivated()
{
  // if this was created procedurally during editor runtime, we do not need to clear specific nodes
  // after simulation, the scene is deleted anyway

  ClearPreviousInstances();

  SUPER::OnDeactivated();
}

void wdPrefabReferenceComponent::ClearPreviousInstances()
{
  if (GetUniqueID() != wdInvalidIndex)
  {
    // if this is in the editor, and the 'activate' flag is toggled,
    // get rid of all our created child objects

    wdArrayPtr<wdComponent* const> comps = GetOwner()->GetComponents();

    for (wdUInt32 ip1 = comps.GetCount(); ip1 > 0; ip1--)
    {
      const wdUInt32 i = ip1 - 1;

      if (comps[i] != this && // don't try to delete yourself
          comps[i]->WasCreatedByPrefab())
      {
        comps[i]->GetOwningManager()->DeleteComponent(comps[i]);
      }
    }

    for (auto it = GetOwner()->GetChildren(); it.IsValid(); ++it)
    {
      if (it->WasCreatedByPrefab())
      {
        GetWorld()->DeleteObjectNow(it->GetHandle());
      }
    }
  }
}

void wdPrefabReferenceComponent::Deinitialize()
{
  if (GetUserFlag(PrefabComponentFlags::SelfDeletion))
  {
    // do nothing, ie do not call OnDeactivated()
    // we do want to keep the created child objects around when this component gets destroyed during simulation
    // that's because the component actually deletes itself when simulation starts
    return;
  }

  // remove the children (through Deactivate)
  OnDeactivated();
}

void wdPrefabReferenceComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (GetUniqueID() == wdInvalidIndex)
  {
    SetUserFlag(PrefabComponentFlags::SelfDeletion, true);

    // remove the prefab reference component, to prevent issues after another serialization/deserialization
    // and also to save some memory
    DeleteComponent();
  }
}

const wdRangeView<const char*, wdUInt32> wdPrefabReferenceComponent::GetParameters() const
{
  return wdRangeView<const char*, wdUInt32>([]() -> wdUInt32
    { return 0; },
    [this]() -> wdUInt32
    { return m_Parameters.GetCount(); },
    [](wdUInt32& ref_uiIt)
    { ++ref_uiIt; },
    [this](const wdUInt32& uiIt) -> const char*
    { return m_Parameters.GetKey(uiIt).GetString().GetData(); });
}

void wdPrefabReferenceComponent::SetParameter(const char* szKey, const wdVariant& value)
{
  wdHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != wdInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;

  if (IsActiveAndInitialized())
  {
    // only add to update list, if not yet activated,
    // since OnActivate will do the instantiation anyway
    GetWorld()->GetComponentManager<wdPrefabReferenceComponentManager>()->AddToUpdateList(this);
  }
}

void wdPrefabReferenceComponent::RemoveParameter(const char* szKey)
{
  if (m_Parameters.RemoveAndCopy(wdTempHashedString(szKey)))
  {
    if (IsActiveAndInitialized())
    {
      // only add to update list, if not yet activated,
      // since OnActivate will do the instantiation anyway
      GetWorld()->GetComponentManager<wdPrefabReferenceComponentManager>()->AddToUpdateList(this);
    }
  }
}

bool wdPrefabReferenceComponent::GetParameter(const char* szKey, wdVariant& out_value) const
{
  wdUInt32 it = m_Parameters.Find(szKey);

  if (it == wdInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}

//////////////////////////////////////////////////////////////////////////

wdPrefabReferenceComponentManager::wdPrefabReferenceComponentManager(wdWorld* pWorld)
  : wdComponentManager<ComponentType, wdBlockStorageType::Compact>(pWorld)
{
  wdResourceManager::GetResourceEvents().AddEventHandler(wdMakeDelegate(&wdPrefabReferenceComponentManager::ResourceEventHandler, this));
}


wdPrefabReferenceComponentManager::~wdPrefabReferenceComponentManager()
{
  wdResourceManager::GetResourceEvents().RemoveEventHandler(wdMakeDelegate(&wdPrefabReferenceComponentManager::ResourceEventHandler, this));
}

void wdPrefabReferenceComponentManager::Initialize()
{
  auto desc = WD_CREATE_MODULE_UPDATE_FUNCTION_DESC(wdPrefabReferenceComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void wdPrefabReferenceComponentManager::ResourceEventHandler(const wdResourceEvent& e)
{
  if (e.m_Type == wdResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<wdPrefabResource>())
  {
    wdPrefabResourceHandle hPrefab((wdPrefabResource*)(e.m_pResource));

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hPrefab == hPrefab)
      {
        AddToUpdateList(it);
      }
    }
  }
}

void wdPrefabReferenceComponentManager::Update(const wdWorldModule::UpdateContext& context)
{
  for (auto hComp : m_ComponentsToUpdate)
  {
    wdPrefabReferenceComponent* pComponent;
    if (!TryGetComponent(hComp, pComponent))
      continue;

    pComponent->m_bInUpdateList = false;
    if (!pComponent->IsActive())
      continue;

    pComponent->ClearPreviousInstances();
    pComponent->InstantiatePrefab();
  }

  m_ComponentsToUpdate.Clear();
}

void wdPrefabReferenceComponentManager::AddToUpdateList(wdPrefabReferenceComponent* pComponent)
{
  if (!pComponent->m_bInUpdateList)
  {
    m_ComponentsToUpdate.PushBack(pComponent->GetHandle());
    pComponent->m_bInUpdateList = true;
  }
}



WD_STATICLINK_FILE(Core, Core_Prefabs_Implementation_PrefabReferenceComponent);
