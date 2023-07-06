#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/SkeletonPoseComponent.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>

// clang-format off
WD_BEGIN_STATIC_REFLECTED_ENUM(wdSkeletonPoseMode, 1)
  WD_ENUM_CONSTANTS(wdSkeletonPoseMode::CustomPose, wdSkeletonPoseMode::RestPose, wdSkeletonPoseMode::Disabled)
WD_END_STATIC_REFLECTED_ENUM;

WD_BEGIN_COMPONENT_TYPE(wdSkeletonPoseComponent, 4, wdComponentMode::Static)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("Skeleton", GetSkeletonFile, SetSkeletonFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Mesh_Skeleton")),
    WD_ENUM_ACCESSOR_PROPERTY("Mode", wdSkeletonPoseMode, GetPoseMode, SetPoseMode),
    WD_MEMBER_PROPERTY("EditBones", m_fDummy),
    WD_MAP_ACCESSOR_PROPERTY("Bones", GetBones, GetBone, SetBone, RemoveBone)->AddAttributes(new wdExposedParametersAttribute("Skeleton"), new wdContainerAttribute(false, true, false)),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Animation"),
    new wdBoneManipulatorAttribute("Bones", "EditBones"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdSkeletonPoseComponent::wdSkeletonPoseComponent() = default;
wdSkeletonPoseComponent::~wdSkeletonPoseComponent() = default;

void wdSkeletonPoseComponent::Update()
{
  if (m_uiResendPose == 0)
    return;

  if (--m_uiResendPose > 0)
  {
    static_cast<wdSkeletonPoseComponentManager*>(GetOwningManager())->EnqueueUpdate(GetHandle());
  }

  if (m_PoseMode == wdSkeletonPoseMode::RestPose)
  {
    SendRestPose();
    return;
  }

  if (m_PoseMode == wdSkeletonPoseMode::CustomPose)
  {
    SendCustomPose();
    return;
  }
}

void wdSkeletonPoseComponent::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_hSkeleton;
  s << m_PoseMode;

  m_Bones.Sort();
  wdUInt16 numBones = static_cast<wdUInt16>(m_Bones.GetCount());
  s << numBones;

  for (wdUInt16 i = 0; i < numBones; ++i)
  {
    s << m_Bones.GetKey(i);
    s << m_Bones.GetValue(i).m_sName;
    s << m_Bones.GetValue(i).m_sParent;
    s << m_Bones.GetValue(i).m_Transform;
  }
}

void wdSkeletonPoseComponent::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const wdUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_hSkeleton;
  s >> m_PoseMode;

  wdHashedString sKey;
  wdExposedBone bone;

  wdUInt16 numBones = 0;
  s >> numBones;
  m_Bones.Reserve(numBones);

  for (wdUInt16 i = 0; i < numBones; ++i)
  {
    s >> sKey;
    s >> bone.m_sName;
    s >> bone.m_sParent;
    s >> bone.m_Transform;

    m_Bones[sKey] = bone;
  }
  ResendPose();
}

void wdSkeletonPoseComponent::OnActivated()
{
  SUPER::OnActivated();

  ResendPose();
}

void wdSkeletonPoseComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  ResendPose();
}

void wdSkeletonPoseComponent::SetSkeletonFile(const char* szFile)
{
  wdSkeletonResourceHandle hResource;

  if (!wdStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = wdResourceManager::LoadResource<wdSkeletonResource>(szFile);
  }

  SetSkeleton(hResource);
}

const char* wdSkeletonPoseComponent::GetSkeletonFile() const
{
  if (!m_hSkeleton.IsValid())
    return "";

  return m_hSkeleton.GetResourceID();
}

void wdSkeletonPoseComponent::SetSkeleton(const wdSkeletonResourceHandle& hResource)
{
  if (m_hSkeleton != hResource)
  {
    m_hSkeleton = hResource;
    ResendPose();
  }
}

void wdSkeletonPoseComponent::SetPoseMode(wdEnum<wdSkeletonPoseMode> mode)
{
  m_PoseMode = mode;
  ResendPose();
}

void wdSkeletonPoseComponent::ResendPose()
{
  if (m_uiResendPose == 2)
    return;

  m_uiResendPose = 2;
  static_cast<wdSkeletonPoseComponentManager*>(GetOwningManager())->EnqueueUpdate(GetHandle());
}

const wdRangeView<const char*, wdUInt32> wdSkeletonPoseComponent::GetBones() const
{
  return wdRangeView<const char*, wdUInt32>([]() -> wdUInt32 { return 0; },
    [this]() -> wdUInt32 { return m_Bones.GetCount(); },
    [](wdUInt32& ref_uiIt) { ++ref_uiIt; },
    [this](const wdUInt32& uiIt) -> const char* { return m_Bones.GetKey(uiIt).GetString().GetData(); });
}

void wdSkeletonPoseComponent::SetBone(const char* szKey, const wdVariant& value)
{
  wdHashedString hs;
  hs.Assign(szKey);

  if (value.GetReflectedType() == wdGetStaticRTTI<wdExposedBone>())
  {
    m_Bones[hs] = *reinterpret_cast<const wdExposedBone*>(value.GetData());
  }

  // TODO
  // if (IsActiveAndInitialized())
  //{
  //  // only add to update list, if not yet activated,
  //  // since OnActivate will do the instantiation anyway
  //  GetWorld()->GetComponentManager<wdPrefabReferenceComponentManager>()->AddToUpdateList(this);
  //}
  ResendPose();
}

void wdSkeletonPoseComponent::RemoveBone(const char* szKey)
{
  if (m_Bones.RemoveAndCopy(wdTempHashedString(szKey)))
  {
    // TODO
    // if (IsActiveAndInitialized())
    //{
    //  // only add to update list, if not yet activated,
    //  // since OnActivate will do the instantiation anyway
    //  GetWorld()->GetComponentManager<wdPrefabReferenceComponentManager>()->AddToUpdateList(this);
    //}

    ResendPose();
  }
}

bool wdSkeletonPoseComponent::GetBone(const char* szKey, wdVariant& out_value) const
{
  wdUInt32 it = m_Bones.Find(szKey);

  if (it == wdInvalidIndex)
    return false;

  out_value.CopyTypedObject(&m_Bones.GetValue(it), wdGetStaticRTTI<wdExposedBone>());
  return true;
}

void wdSkeletonPoseComponent::SendRestPose()
{
  if (!m_hSkeleton.IsValid())
    return;

  wdResourceLock<wdSkeletonResource> pSkeleton(m_hSkeleton, wdResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != wdResourceAcquireResult::Final)
    return;

  const auto& desc = pSkeleton->GetDescriptor();
  const auto& skel = desc.m_Skeleton;

  if (skel.GetJointCount() == 0)
    return;

  wdHybridArray<wdMat4, 32> finalTransforms(wdFrameAllocator::GetCurrentAllocator());
  finalTransforms.SetCountUninitialized(skel.GetJointCount());

  {
    ozz::animation::LocalToModelJob job;
    job.input = skel.GetOzzSkeleton().joint_rest_poses();
    job.output = ozz::span<ozz::math::Float4x4>(reinterpret_cast<ozz::math::Float4x4*>(finalTransforms.GetData()), finalTransforms.GetCount());
    job.skeleton = &skel.GetOzzSkeleton();
    job.Run();
  }

  wdMsgAnimationPoseUpdated msg;
  msg.m_pRootTransform = &desc.m_RootTransform;
  msg.m_pSkeleton = &skel;
  msg.m_ModelTransforms = finalTransforms;

  GetOwner()->SendMessage(msg);

  if (msg.m_bContinueAnimating == false)
    m_PoseMode = wdSkeletonPoseMode::Disabled;
}

void wdSkeletonPoseComponent::SendCustomPose()
{
  if (!m_hSkeleton.IsValid())
    return;

  wdResourceLock<wdSkeletonResource> pSkeleton(m_hSkeleton, wdResourceAcquireMode::BlockTillLoaded);
  const auto& desc = pSkeleton->GetDescriptor();
  const auto& skel = desc.m_Skeleton;

  wdHybridArray<wdMat4, 32> finalTransforms(wdFrameAllocator::GetCurrentAllocator());
  finalTransforms.SetCountUninitialized(skel.GetJointCount());

  for (wdUInt32 i = 0; i < finalTransforms.GetCount(); ++i)
  {
    finalTransforms[i].SetIdentity();
  }

  ozz::vector<ozz::math::SoaTransform> ozzLocalTransforms;
  ozzLocalTransforms.resize((skel.GetJointCount() + 3) / 4);

  auto restPoses = skel.GetOzzSkeleton().joint_rest_poses();

  // initialize the skeleton with the rest pose
  for (wdUInt32 i = 0; i < ozzLocalTransforms.size(); ++i)
  {
    ozzLocalTransforms[i] = restPoses[i];
  }

  for (const auto& boneIt : m_Bones)
  {
    const wdUInt16 uiBone = skel.FindJointByName(boneIt.key);
    if (uiBone == wdInvalidJointIndex)
      continue;

    const wdExposedBone& thisBone = boneIt.value;

    // this can happen when the property was reverted
    if (thisBone.m_sName.IsEmpty() || thisBone.m_sParent.IsEmpty())
      continue;

    WD_ASSERT_DEBUG(!thisBone.m_Transform.m_qRotation.IsNaN(), "Invalid bone transform in pose component");

    const wdQuat& boneRot = thisBone.m_Transform.m_qRotation;

    const wdUInt32 idx0 = uiBone / 4;
    const wdUInt32 idx1 = uiBone % 4;

    ozz::math::SoaQuaternion& q = ozzLocalTransforms[idx0].rotation;
    reinterpret_cast<float*>(&q.x)[idx1] = boneRot.v.x;
    reinterpret_cast<float*>(&q.y)[idx1] = boneRot.v.y;
    reinterpret_cast<float*>(&q.z)[idx1] = boneRot.v.z;
    reinterpret_cast<float*>(&q.w)[idx1] = boneRot.w;
  }

  ozz::animation::LocalToModelJob job;
  job.input = ozz::span<const ozz::math::SoaTransform>(ozzLocalTransforms.data(), ozzLocalTransforms.size());
  job.output = ozz::span<ozz::math::Float4x4>(reinterpret_cast<ozz::math::Float4x4*>(finalTransforms.GetData()), finalTransforms.GetCount());
  job.skeleton = &skel.GetOzzSkeleton();
  WD_ASSERT_DEBUG(job.Validate(), "");
  job.Run();


  wdMsgAnimationPoseUpdated msg;
  msg.m_pRootTransform = &desc.m_RootTransform;
  msg.m_pSkeleton = &skel;
  msg.m_ModelTransforms = finalTransforms;

  GetOwner()->SendMessage(msg);

  if (msg.m_bContinueAnimating == false)
    m_PoseMode = wdSkeletonPoseMode::Disabled;
}

//////////////////////////////////////////////////////////////////////////

void wdSkeletonPoseComponentManager::Update(const wdWorldModule::UpdateContext& context)
{
  wdDeque<wdComponentHandle> requireUpdate;

  {
    WD_LOCK(m_Mutex);
    requireUpdate.Swap(m_RequireUpdate);
  }

  for (const auto& hComp : requireUpdate)
  {
    wdSkeletonPoseComponent* pComp = nullptr;
    if (!TryGetComponent(hComp, pComp) || !pComp->IsActiveAndInitialized())
      continue;

    pComp->Update();
  }
}

void wdSkeletonPoseComponentManager::EnqueueUpdate(wdComponentHandle hComponent)
{
  WD_LOCK(m_Mutex);

  if (m_RequireUpdate.IndexOf(hComponent) != wdInvalidIndex)
    return;

  m_RequireUpdate.PushBack(hComponent);
}

void wdSkeletonPoseComponentManager::Initialize()
{
  SUPER::Initialize();

  wdWorldModule::UpdateFunctionDesc desc = WD_CREATE_MODULE_UPDATE_FUNCTION_DESC(wdSkeletonPoseComponentManager::Update, this);
  desc.m_Phase = UpdateFunctionDesc::Phase::PreAsync;

  RegisterUpdateFunction(desc);
}


WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_SkeletonPoseComponent);
