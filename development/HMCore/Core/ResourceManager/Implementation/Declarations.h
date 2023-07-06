#pragma once

/// \file

#include <Core/CoreDLL.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Bitflags.h>

class wdResource;
class wdResourceManager;
class wdResourceTypeLoader;
class wdStreamReader;

template <typename ResourceType>
class wdTypedResourceHandle;

// clang-format off

/// \brief These events may be sent by a specific wdResource or by the wdResourceManager
struct wdResourceEvent
{
  enum class Type
  {
    ResourceExists,           ///< Used to broadcast that a resource exists. Used to inform inspection tools which resources are currently existing.
                              ///< Triggered by wdResourceManager::BroadcastExistsEvent().
    ResourceCreated,          ///< Sent whenever a new resource is added to the system.
    ResourceDeleted,          ///< Sent right before a resource gets deallocated.
    ResourceContentUpdated,   ///< Sent shortly after wdResource::UpdateContent() has been called on a resource.
    ResourceContentUnloading, ///< Resource is about to be unloaded, but still valid at this point. \note When a resource is 'reloaded' this
                              ///< is the important event to track. Every reload starts with an unload. The actual 'load' (UpdateContant)
                              ///< only happens on demand.
    ResourcePriorityChanged,  ///< Sent when the priority of a resource is modified.
  };

  Type m_Type;
  const wdResource* m_pResource = nullptr;
};

/// \brief Events sent by the wdResourceManager
struct wdResourceManagerEvent
{
  enum class Type
  {
    ManagerShuttingDown,      ///< Sent first thing by wdResourceManager::OnEngineShutdown().
    ReloadAllResources,       ///< Sent by wdResourceManager::ReloadAllResources() if any resource got unloaded (not yet reloaded)
  };

  Type m_Type;
};

/// \brief The flags of an wdResource instance.
struct wdResourceFlags
{
  using StorageType = wdUInt16;

  /// \brief The flags of an wdResource instance.
  enum Enum
  {
    UpdateOnMainThread      = WD_BIT(0),  ///< After loading the resource data on a thread, it must be uploaded on the main thread. Use this for resources which require a context that is only available on the main thread.
    NoFileAccessRequired    = WD_BIT(1),  ///< The resource 'loading' does not require file accesses and can therefore be done on one or several non-file-loading threads. Use this for procedurally generated data.
    /// \todo implement NoFileAccessRequired
    ResourceHasFallback     = WD_BIT(2),  ///< Specifies whether this resource has a valid fallback resource that could be used. Automatically updated in wdResource::SetFallbackResource.
    ResourceHasTypeFallback = WD_BIT(3),  ///< Specifies whether this resource has a valid type fallback that could be used.
    IsReloadable            = WD_BIT(4),  ///< The resource was created, not loaded from file
    IsQueuedForLoading      = WD_BIT(5),
    HasCustomDataLoader     = WD_BIT(6),  ///< True if someone wants to update a resource with custom data and has created a resource loader to update this specific resource
    PreventFileReload       = WD_BIT(7),  ///< Once this flag is set, no reloading from file is done, until the flag is manually removed. Automatically set when a custom loader is used. To restore a file to the disk state, this flag must be removed and then the resource can be reloaded.
    HasLowResData           = WD_BIT(8),  ///< Whether low resolution data was set on a resource once before
    IsCreatedResource       = WD_BIT(9),  ///< When this is set, the resource was created and not loaded from file
    Default                 = 0,
  };

  struct Bits
  {
    StorageType UpdateOnMainThread      : 1;
    StorageType NoFileAccessRequired    : 1;
    StorageType ResourceHasFallback     : 1;
    StorageType ResourceHasTypeFallback : 1;
    StorageType IsReloadable            : 1;
    StorageType IsQueuedForLoading      : 1;
    StorageType HasCustomDataLoader     : 1;
    StorageType PreventFileReload       : 1;
    StorageType HasLowResData           : 1;
    StorageType IsCreatedResource       : 1;
  };
};

WD_DECLARE_FLAGS_OPERATORS(wdResourceFlags);

/// \brief Describes the state in which a resource can be in.
enum class wdResourceState
{
  Invalid,                    ///< Initial state
  Unloaded,                   ///< The resource instance has been created, but no meta info about the resource is available and no data is loaded.
  LoadedResourceMissing,      ///< The resource could not be loaded, uses a 'Missing Resource' fallback if available
  Loaded,                     ///< The resource is fully loaded.
};

/// \brief Describes in which loading state a resource currently is, and how many different quality levels there are
struct wdResourceLoadDesc
{
  wdResourceState m_State = wdResourceState::Invalid;

  /// How often the resource could discard a 'quality level' of its data and still be usable. Zero for the vast majority of resource types.
  wdUInt8 m_uiQualityLevelsDiscardable = 0xFF;  // invalid
  /// How often the resource could load another 'quality level' of its data until everything is loaded. Zero for the vast majority of resource types.
  wdUInt8 m_uiQualityLevelsLoadable = 0xFF;     // invalid
};

/// \brief Describes what data of a resource needs to be accessed and thus how much of the resource needs to be loaded.
///
/// \note Inspect the wdResourceAcquireResult to know whether acquisition failed or whether a fallback was returned.
enum class wdResourceAcquireMode
{
  PointerOnly,                ///< We really only want the pointer (maybe to update some state), no data needs to be loaded.
                              ///< This will never block and can never fail.
  AllowLoadingFallback,       ///< We want to use the resource, but if it has a loading fallback, using that is fine as well.
                              ///< This should be the default usage.
  AllowLoadingFallback_NeverFail,///< Same as 'AllowLoadingFallback' but in case the resource is fully missing (and no 'Missing Fallback' is available),
                              ///< no assertion triggers and no error is logged.
                              ///< The calling code signals with this that it can handle the situation.
  BlockTillLoaded,            ///< The full resource data is required. The loader will block until the resource is in the 'Loaded' state.
                              ///< This does NOT mean that all quality levels are loaded.
                              ///< If the resource is missing, the 'Missing Resource' fallback may be returned.
                              ///< If no 'Missing Fallback' exists, the engine will assert/crash.
  BlockTillLoaded_NeverFail,  ///< Same as 'BlockTillLoaded' but in case the resource is fully missing (and no 'Missing Fallback' is available),
                              ///< no assertion triggers and no error is logged.
                              ///< The calling code signals with this that it can handle the situation.
};

/// \brief Indicates whether acquiring a resource was successful.
enum class wdResourceAcquireResult
{
  None,             ///< No result available, ie the resource could not be loaded, not even a missing fallback was available, the resource pointer is
                    ///< nullptr. This result mode is only available when wdResourceAcquireMode::BlockTillLoaded_NeverFail was used.
  MissingFallback, ///< The resource could not be loaded, but a missing fallback was available and has been returned.
  LoadingFallback, ///< The resource has not yet been loaded, but a loading fallback was available and has been returned.
  Final,           ///< The resource is fully available.
};

enum class wdResourcePriority
{
  Critical = 0,
  VeryHigh = 1,
  High     = 2,
  Medium   = 3,
  Low      = 4,
  VeryLow  = 5,
};

// clang-format on
