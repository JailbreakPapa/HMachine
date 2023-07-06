#pragma once

#include <Core/CoreDLL.h>
#include <Core/ResourceManager/Resource.h>

/// \brief Represents one resource to load / preload through an wdCollectionResource
struct WD_CORE_DLL wdCollectionEntry
{
  wdString m_sOptionalNiceLookupName; ///< Optional, can be used to lookup the resource at runtime with a nice name. E.g. "SkyTexture" instead of some GUID.
  wdString m_sResourceID;             ///< The ID / path to the resource to load.
  wdHashedString m_sAssetTypeName;
  wdUInt64 m_uiFileSize = 0;
};

/// \brief Describes a full wdCollectionResource, ie. lists all the resources that the collection contains
struct WD_CORE_DLL wdCollectionResourceDescriptor
{
  wdDynamicArray<wdCollectionEntry> m_Resources;

  void Save(wdStreamWriter& inout_stream) const;
  void Load(wdStreamReader& inout_stream);
};

using wdCollectionResourceHandle = wdTypedResourceHandle<class wdCollectionResource>;

/// \brief An wdCollectionResource is used to tell the engine about resources that it should preload in the background
///
/// Collection resources can be used to improve the user experience by ensuring data is already (more likely) available when it is needed.
/// For instance when a player walks into a longer corridor, a collection resource can be triggered to preload the data that will be needed
/// when he reaches the door at the end of the corridor.
/// In scenes this can be achieved using an wdCollectionComponent.
///
/// Collection resources can also be used to query how much percent of the references resources are already loaded, which enables building
/// progress bars or allowing functionality only after all necessary data is available.
///
/// Finally, collections can also be used to register resources with a nice name, such that it can be referenced in code by that name.
/// This is useful if you want to reference some asset in code with a readable resource ID, and may want to change which resource is referenced
/// at a later time. For example the nice name "Avatar" could point to some dummy mesh at the beginning of the project, and be updated to point
/// to something else later, without the need to change the code.
///
/// \note Once wdCollectionResource::PreloadResources() has been triggered, the resource will hold a handle to all referenced resources,
///       meaning those resources cannot get unloaded anymore. Make sure to discard the collection resource if the data it references
///       should get freed up.
class WD_CORE_DLL wdCollectionResource : public wdResource
{
  WD_ADD_DYNAMIC_REFLECTION(wdCollectionResource, wdResource);
  WD_RESOURCE_DECLARE_COMMON_CODE(wdCollectionResource);
  WD_RESOURCE_DECLARE_CREATEABLE(wdCollectionResource, wdCollectionResourceDescriptor);

public:
  wdCollectionResource();

  /// \brief Registers the named resources in the collection with the wdResourceManager, such that they can be loaded by those names.
  ///
  /// \note This has to be called MANUALLY on collection resources, they do NOT do this automatically when loaded.
  /// Since resources are streamed, there is no guaranteed point in time when those names would be registered, which would introduce timing issues,
  /// where sometimes the name is registered when it is used and sometimes not.
  /// By calling this manually, the point in time where this is done is fixed and guaranteed.
  ///
  /// Calling this twice has no effect.
  void RegisterNames();

  /// \brief Removes the registered names from the wdResourceManager.
  ///
  /// Calling this twice has no effect.
  void UnregisterNames();

  /// \brief Puts up to the given number of resources for which a resource type could be found into the preload queue of the wdResourceManager.
  ///
  /// This has to be called manually. It will return false if no more resources can be queued for preloading. This can be used
  /// as a workflow where PreloadResources and IsLoadingFinished are called repeadedly in tandem, so only a smaller fraction
  /// of resources gets queued and waited for, to allow simple resource load-balancing.
  bool PreloadResources(wdUInt32 uiNumResourcesToPreload = wdMath::MaxValue<wdUInt32>());

  /// \brief Returns true if all resources added for preloading via PreloadResources have finished loading.
  /// if `out_progress` is defined:
  ///     * Assigns a value between 0.0 and 1.0 representing how many of the collection's resources are in a loaded state at the moment.
  ///     * Always assigns 1.0 if all resources are in a loaded state.
  ///     * Always assigns 1.0 if the collection contains no resources, or PreloadResources() was not triggered previously.
  /// Note: the progress can reach at maximum the fraction of resources that have been queued for preloading via PreloadResources.
  /// The progress will only reach 1.0 if all resources of this collection have been queued via PreloadResources and finished loading.
  bool IsLoadingFinished(float* out_pProgress = nullptr) const;

  /// \brief Returns the resource descriptor for this resource.
  const wdCollectionResourceDescriptor& GetDescriptor() const;

  /// \brief Returns the current list of resources that have already been added to the preload list. See PreloadResources().
  wdArrayPtr<const wdTypelessResourceHandle> GetPreloadedResources() const { return m_PreloadedResources; }

private:
  virtual wdResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual wdResourceLoadDesc UpdateContent(wdStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  mutable wdMutex m_PreloadMutex;
  bool m_bRegistered = false;
  wdCollectionResourceDescriptor m_Collection;
  wdDynamicArray<wdTypelessResourceHandle> m_PreloadedResources;
};
