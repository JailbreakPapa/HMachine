#pragma once

#include <Core/CoreDLL.h>

#include <Core/Physics/SurfaceResourceDescriptor.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/World/Declarations.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>

class wdWorld;
class wdUuid;

struct wdSurfaceResourceEvent
{
  enum class Type
  {
    Created,
    Destroyed
  };

  Type m_Type;
  wdSurfaceResource* m_pSurface;
};

class WD_CORE_DLL wdSurfaceResource : public wdResource
{
  WD_ADD_DYNAMIC_REFLECTION(wdSurfaceResource, wdResource);
  WD_RESOURCE_DECLARE_COMMON_CODE(wdSurfaceResource);
  WD_RESOURCE_DECLARE_CREATEABLE(wdSurfaceResource, wdSurfaceResourceDescriptor);

public:
  wdSurfaceResource();
  ~wdSurfaceResource();

  const wdSurfaceResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

  static wdEvent<const wdSurfaceResourceEvent&, wdMutex> s_Events;

  void* m_pPhysicsMaterialPhysX = nullptr;
  void* m_pPhysicsMaterialJolt = nullptr;

  /// \brief Spawns the prefab that was defined for the given interaction at the given position and using the configured orientation.
  /// Returns false, if the interaction type was not defined in this surface or any of its base surfaces
  bool InteractWithSurface(wdWorld* pWorld, wdGameObjectHandle hObject, const wdVec3& vPosition, const wdVec3& vSurfaceNormal, const wdVec3& vIncomingDirection, const wdTempHashedString& sInteraction, const wdUInt16* pOverrideTeamID, float fImpulseSqr = 0.0f) const;

  bool IsBasedOn(const wdSurfaceResource* pThisOrBaseSurface) const;

  bool IsBasedOn(const wdSurfaceResourceHandle hThisOrBaseSurface) const;

private:
  virtual wdResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual wdResourceLoadDesc UpdateContent(wdStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  static const wdSurfaceInteraction* FindInteraction(const wdSurfaceResource* pCurSurf, wdUInt64 uiHash, float fImpulseSqr, float& out_fImpulseParamValue);

  wdSurfaceResourceDescriptor m_Descriptor;

  struct SurfInt
  {
    wdUInt64 m_uiInteractionTypeHash = 0;
    const wdSurfaceInteraction* m_pInteraction;
  };

  wdDynamicArray<SurfInt> m_Interactions;
};
