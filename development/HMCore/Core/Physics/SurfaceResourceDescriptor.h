#pragma once

#include <Core/CoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/RangeView.h>
#include <Foundation/Types/Variant.h>

using wdSurfaceResourceHandle = wdTypedResourceHandle<class wdSurfaceResource>;
using wdPrefabResourceHandle = wdTypedResourceHandle<class wdPrefabResource>;


struct wdSurfaceInteractionAlignment
{
  using StorageType = wdUInt8;

  enum Enum
  {
    SurfaceNormal,
    IncidentDirection,
    ReflectedDirection,
    ReverseSurfaceNormal,
    ReverseIncidentDirection,
    ReverseReflectedDirection,

    Default = SurfaceNormal
  };
};

WD_DECLARE_REFLECTABLE_TYPE(WD_CORE_DLL, wdSurfaceInteractionAlignment);


struct WD_CORE_DLL wdSurfaceInteraction
{
  void SetPrefab(const char* szPrefab);
  const char* GetPrefab() const;

  wdString m_sInteractionType;

  wdPrefabResourceHandle m_hPrefab;
  wdEnum<wdSurfaceInteractionAlignment> m_Alignment;
  wdAngle m_Deviation;
  float m_fImpulseThreshold = 0.0f;
  float m_fImpulseScale = 1.0f;

  const wdRangeView<const char*, wdUInt32> GetParameters() const;   // [ property ] (exposed parameter)
  void SetParameter(const char* szKey, const wdVariant& value);     // [ property ] (exposed parameter)
  void RemoveParameter(const char* szKey);                          // [ property ] (exposed parameter)
  bool GetParameter(const char* szKey, wdVariant& out_value) const; // [ property ] (exposed parameter)

  wdArrayMap<wdHashedString, wdVariant> m_Parameters;
};

WD_DECLARE_REFLECTABLE_TYPE(WD_CORE_DLL, wdSurfaceInteraction);

struct WD_CORE_DLL wdSurfaceResourceDescriptor : public wdReflectedClass
{
  WD_ADD_DYNAMIC_REFLECTION(wdSurfaceResourceDescriptor, wdReflectedClass);

public:
  void Load(wdStreamReader& inout_stream);
  void Save(wdStreamWriter& inout_stream) const;

  void SetBaseSurfaceFile(const char* szFile);
  const char* GetBaseSurfaceFile() const;

  void SetCollisionInteraction(const char* szName);
  const char* GetCollisionInteraction() const;

  void SetSlideReactionPrefabFile(const char* szFile);
  const char* GetSlideReactionPrefabFile() const;

  void SetRollReactionPrefabFile(const char* szFile);
  const char* GetRollReactionPrefabFile() const;


  wdSurfaceResourceHandle m_hBaseSurface;
  float m_fPhysicsRestitution;
  float m_fPhysicsFrictionStatic;
  float m_fPhysicsFrictionDynamic;
  wdHashedString m_sOnCollideInteraction;
  wdHashedString m_sSlideInteractionPrefab;
  wdHashedString m_sRollInteractionPrefab;

  wdHybridArray<wdSurfaceInteraction, 16> m_Interactions;
};
