#include <Core/CorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/Messages/ApplyOnlyToMessage.h>
#include <Core/Messages/CommonMessages.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/Prefabs/PrefabResource.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdSurfaceResource, 1, wdRTTIDefaultAllocator<wdSurfaceResource>)
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_RESOURCE_IMPLEMENT_COMMON_CODE(wdSurfaceResource);
// clang-format on

wdEvent<const wdSurfaceResourceEvent&, wdMutex> wdSurfaceResource::s_Events;

wdSurfaceResource::wdSurfaceResource()
  : wdResource(DoUpdate::OnAnyThread, 1)
{
}

wdSurfaceResource::~wdSurfaceResource()
{
  WD_ASSERT_DEV(m_pPhysicsMaterialPhysX == nullptr, "Physics material has not been cleaned up properly");
  WD_ASSERT_DEV(m_pPhysicsMaterialJolt == nullptr, "Physics material has not been cleaned up properly");
}

wdResourceLoadDesc wdSurfaceResource::UnloadData(Unload WhatToUnload)
{
  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::Unloaded;

  wdSurfaceResourceEvent e;
  e.m_pSurface = this;
  e.m_Type = wdSurfaceResourceEvent::Type::Destroyed;
  s_Events.Broadcast(e);

  return res;
}

wdResourceLoadDesc wdSurfaceResource::UpdateContent(wdStreamReader* Stream)
{
  WD_LOG_BLOCK("wdSurfaceResource::UpdateContent", GetResourceDescription().GetData());

  m_Interactions.Clear();

  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = wdResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    wdStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }


  wdAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  {
    wdSurfaceResourceDescriptor dummy;
    dummy.Load(*Stream);

    CreateResource(std::move(dummy));
  }

  // configure the lookup table
  {
    m_Interactions.Reserve(m_Descriptor.m_Interactions.GetCount());
    for (const auto& i : m_Descriptor.m_Interactions)
    {
      wdTempHashedString s(i.m_sInteractionType.GetData());
      auto& item = m_Interactions.ExpandAndGetRef();
      item.m_uiInteractionTypeHash = s.GetHash();
      item.m_pInteraction = &i;
    }

    m_Interactions.Sort([](const SurfInt& lhs, const SurfInt& rhs) -> bool {
      if (lhs.m_uiInteractionTypeHash != rhs.m_uiInteractionTypeHash)
        return lhs.m_uiInteractionTypeHash < rhs.m_uiInteractionTypeHash;

      return lhs.m_pInteraction->m_fImpulseThreshold > rhs.m_pInteraction->m_fImpulseThreshold;
    });
  }

  res.m_State = wdResourceState::Loaded;
  return res;
}

void wdSurfaceResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(wdSurfaceResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

WD_RESOURCE_IMPLEMENT_CREATEABLE(wdSurfaceResource, wdSurfaceResourceDescriptor)
{
  m_Descriptor = descriptor;

  wdResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = wdResourceState::Loaded;

  wdSurfaceResourceEvent e;
  e.m_pSurface = this;
  e.m_Type = wdSurfaceResourceEvent::Type::Created;
  s_Events.Broadcast(e);

  return res;
}

const wdSurfaceInteraction* wdSurfaceResource::FindInteraction(const wdSurfaceResource* pCurSurf, wdUInt64 uiHash, float fImpulseSqr, float& out_fImpulseParamValue)
{
  while (true)
  {
    bool bFoundAny = false;

    // try to find a matching interaction
    for (const auto& interaction : pCurSurf->m_Interactions)
    {
      if (interaction.m_uiInteractionTypeHash > uiHash)
        break;

      if (interaction.m_uiInteractionTypeHash == uiHash)
      {
        bFoundAny = true;

        // only use it if the threshold is large enough
        if (fImpulseSqr >= wdMath::Square(interaction.m_pInteraction->m_fImpulseThreshold))
        {
          const float fImpulse = wdMath::Sqrt(fImpulseSqr);
          out_fImpulseParamValue = (fImpulse - interaction.m_pInteraction->m_fImpulseThreshold) * interaction.m_pInteraction->m_fImpulseScale;

          return interaction.m_pInteraction;
        }
      }
    }

    // if we did find something, we just never exceeded the threshold, then do not search in the base surface
    if (bFoundAny)
      break;

    if (pCurSurf->m_Descriptor.m_hBaseSurface.IsValid())
    {
      wdResourceLock<wdSurfaceResource> pBase(pCurSurf->m_Descriptor.m_hBaseSurface, wdResourceAcquireMode::BlockTillLoaded);
      pCurSurf = pBase.GetPointer();
    }
    else
    {
      break;
    }
  }

  return nullptr;
}

bool wdSurfaceResource::InteractWithSurface(wdWorld* pWorld, wdGameObjectHandle hObject, const wdVec3& vPosition, const wdVec3& vSurfaceNormal, const wdVec3& vIncomingDirection, const wdTempHashedString& sInteraction, const wdUInt16* pOverrideTeamID, float fImpulseSqr /*= 0.0f*/) const
{
  float fImpulseParam = 0;
  const wdSurfaceInteraction* pIA = FindInteraction(this, sInteraction.GetHash(), fImpulseSqr, fImpulseParam);

  if (pIA == nullptr)
    return false;

  // defined, but set to be empty
  if (!pIA->m_hPrefab.IsValid())
    return false;

  wdResourceLock<wdPrefabResource> pPrefab(pIA->m_hPrefab, wdResourceAcquireMode::BlockTillLoaded);

  wdVec3 vDir;

  switch (pIA->m_Alignment)
  {
    case wdSurfaceInteractionAlignment::SurfaceNormal:
      vDir = vSurfaceNormal;
      break;

    case wdSurfaceInteractionAlignment::IncidentDirection:
      vDir = -vIncomingDirection;
      ;
      break;

    case wdSurfaceInteractionAlignment::ReflectedDirection:
      vDir = vIncomingDirection.GetReflectedVector(vSurfaceNormal);
      break;

    case wdSurfaceInteractionAlignment::ReverseSurfaceNormal:
      vDir = -vSurfaceNormal;
      break;

    case wdSurfaceInteractionAlignment::ReverseIncidentDirection:
      vDir = vIncomingDirection;
      ;
      break;

    case wdSurfaceInteractionAlignment::ReverseReflectedDirection:
      vDir = -vIncomingDirection.GetReflectedVector(vSurfaceNormal);
      break;
  }

  vDir.Normalize();
  wdVec3 vTangent = vDir.GetOrthogonalVector().GetNormalized();

  // random rotation around the spawn direction
  {
    double randomAngle = pWorld->GetRandomNumberGenerator().DoubleInRange(0.0, wdMath::Pi<double>() * 2.0);

    wdMat3 rotMat;
    rotMat.SetRotationMatrix(vDir, wdAngle::Radian((float)randomAngle));

    vTangent = rotMat * vTangent;
  }

  if (pIA->m_Deviation > wdAngle::Radian(0.0f))
  {
    wdAngle maxDeviation;

    /// \todo do random deviation, make sure to clamp max deviation angle
    switch (pIA->m_Alignment)
    {
      case wdSurfaceInteractionAlignment::IncidentDirection:
      case wdSurfaceInteractionAlignment::ReverseReflectedDirection:
      {
        const float fCosAngle = vDir.Dot(-vSurfaceNormal);
        const float fMaxDeviation = wdMath::Pi<float>() - wdMath::ACos(fCosAngle).GetRadian();

        maxDeviation = wdMath::Min(pIA->m_Deviation, wdAngle::Radian(fMaxDeviation));
      }
      break;

      case wdSurfaceInteractionAlignment::ReflectedDirection:
      case wdSurfaceInteractionAlignment::ReverseIncidentDirection:
      {
        const float fCosAngle = vDir.Dot(vSurfaceNormal);
        const float fMaxDeviation = wdMath::Pi<float>() - wdMath::ACos(fCosAngle).GetRadian();

        maxDeviation = wdMath::Min(pIA->m_Deviation, wdAngle::Radian(fMaxDeviation));
      }
      break;

      default:
        maxDeviation = pIA->m_Deviation;
        break;
    }

    const wdAngle deviation = wdAngle::Radian((float)pWorld->GetRandomNumberGenerator().DoubleMinMax(-maxDeviation.GetRadian(), maxDeviation.GetRadian()));

    // tilt around the tangent (we don't want to compute another random rotation here)
    wdMat3 matTilt;
    matTilt.SetRotationMatrix(vTangent, deviation);

    vDir = matTilt * vDir;
  }


  // finally compute the bi-tangent
  const wdVec3 vBiTangent = vDir.CrossRH(vTangent);

  wdMat3 mRot;
  mRot.SetColumn(0, vDir); // we always use X as the main axis, so align X with the direction
  mRot.SetColumn(1, vTangent);
  mRot.SetColumn(2, vBiTangent);

  wdTransform t;
  t.m_vPosition = vPosition;
  t.m_qRotation.SetFromMat3(mRot);
  t.m_vScale.Set(1.0f);

  // attach to dynamic objects
  wdGameObjectHandle hParent;

  wdGameObject* pObject = nullptr;
  if (pWorld->TryGetObject(hObject, pObject) && pObject->IsDynamic())
  {
    hParent = hObject;
    t.SetLocalTransform(pObject->GetGlobalTransform(), t);
  }

  wdHybridArray<wdGameObject*, 8> rootObjects;

  wdPrefabInstantiationOptions options;
  options.m_hParent = hParent;
  options.m_pCreatedRootObjectsOut = &rootObjects;
  options.m_pOverrideTeamID = pOverrideTeamID;

  pPrefab->InstantiatePrefab(*pWorld, t, options, &pIA->m_Parameters);

  {
    wdMsgSetFloatParameter msgSetFloat;
    msgSetFloat.m_sParameterName = "Impulse";
    msgSetFloat.m_fValue = fImpulseParam;

    for (auto pRootObject : rootObjects)
    {
      pRootObject->PostMessageRecursive(msgSetFloat, wdTime::Zero(), wdObjectMsgQueueType::AfterInitialized);
    }
  }

  if (pObject != nullptr && pObject->IsDynamic())
  {
    wdMsgOnlyApplyToObject msg;
    msg.m_hObject = hParent;

    for (auto pRootObject : rootObjects)
    {
      pRootObject->PostMessageRecursive(msg, wdTime::Zero(), wdObjectMsgQueueType::AfterInitialized);
    }
  }

  return true;
}

bool wdSurfaceResource::IsBasedOn(const wdSurfaceResource* pThisOrBaseSurface) const
{
  if (pThisOrBaseSurface == this)
    return true;

  if (m_Descriptor.m_hBaseSurface.IsValid())
  {
    wdResourceLock<wdSurfaceResource> pBase(m_Descriptor.m_hBaseSurface, wdResourceAcquireMode::BlockTillLoaded);

    return pBase->IsBasedOn(pThisOrBaseSurface);
  }

  return false;
}

bool wdSurfaceResource::IsBasedOn(const wdSurfaceResourceHandle hThisOrBaseSurface) const
{
  wdResourceLock<wdSurfaceResource> pThisOrBaseSurface(hThisOrBaseSurface, wdResourceAcquireMode::BlockTillLoaded);

  return IsBasedOn(pThisOrBaseSurface.GetPointer());
}


WD_STATICLINK_FILE(Core, Core_Physics_Implementation_SurfaceResource);
