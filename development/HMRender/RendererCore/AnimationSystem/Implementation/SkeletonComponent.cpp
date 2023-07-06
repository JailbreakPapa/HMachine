#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/SkeletonComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/skeleton_utils.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>

// clang-format off
WD_BEGIN_COMPONENT_TYPE(wdSkeletonComponent, 5, wdComponentMode::Static)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("Skeleton", GetSkeletonFile, SetSkeletonFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Mesh_Skeleton")),
    WD_MEMBER_PROPERTY("VisualizeSkeleton", m_bVisualizeBones)->AddAttributes(new wdDefaultValueAttribute(true)),
    WD_MEMBER_PROPERTY("VisualizeColliders", m_bVisualizeColliders),
    WD_MEMBER_PROPERTY("VisualizeJoints", m_bVisualizeJoints),
    WD_MEMBER_PROPERTY("VisualizeSwingLimits", m_bVisualizeSwingLimits),
    WD_MEMBER_PROPERTY("VisualizeTwistLimits", m_bVisualizeTwistLimits),
    WD_ACCESSOR_PROPERTY("BonesToHighlight", GetBonesToHighlight, SetBonesToHighlight),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_MESSAGEHANDLERS
  {
    WD_MESSAGE_HANDLER(wdMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
    WD_MESSAGE_HANDLER(wdMsgQueryAnimationSkeleton, OnQueryAnimationSkeleton)
  }
  WD_END_MESSAGEHANDLERS;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Animation"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdSkeletonComponent::wdSkeletonComponent() = default;
wdSkeletonComponent::~wdSkeletonComponent() = default;

wdResult wdSkeletonComponent::GetLocalBounds(wdBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, wdMsgUpdateLocalBounds& ref_msg)
{
  if (m_MaxBounds.IsValid())
  {
    wdBoundingBox bbox = m_MaxBounds;
    ref_bounds = bbox;
    ref_bounds.Transform(m_RootTransform.GetAsMat4());
    return WD_SUCCESS;
  }

  return WD_FAILURE;
}

void wdSkeletonComponent::Update()
{
  if (m_hSkeleton.IsValid() && (m_bVisualizeBones || m_bVisualizeColliders || m_bVisualizeJoints || m_bVisualizeSwingLimits || m_bVisualizeTwistLimits))
  {
    wdResourceLock<wdSkeletonResource> pSkeleton(m_hSkeleton, wdResourceAcquireMode::AllowLoadingFallback_NeverFail);

    if (pSkeleton.GetAcquireResult() != wdResourceAcquireResult::Final)
      return;

    if (m_uiSkeletonChangeCounter != pSkeleton->GetCurrentResourceChangeCounter())
    {
      VisualizeSkeletonDefaultState();
    }

    const wdQuat qBoneDir = wdBasisAxis::GetBasisRotation_PosX(pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);
    const wdVec3 vBoneDir = qBoneDir * wdVec3(1, 0, 0);
    const wdVec3 vBoneTangent = qBoneDir * wdVec3(0, 1, 0);

    wdDebugRenderer::DrawLines(GetWorld(), m_LinesSkeleton, wdColor::White, GetOwner()->GetGlobalTransform());

    for (const auto& shape : m_SpheresShapes)
    {
      wdDebugRenderer::DrawLineSphere(GetWorld(), shape.m_Shape, shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform);
    }

    for (const auto& shape : m_BoxShapes)
    {
      wdDebugRenderer::DrawLineBox(GetWorld(), shape.m_Shape, shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform);
    }

    for (const auto& shape : m_CapsuleShapes)
    {
      wdDebugRenderer::DrawLineCapsuleZ(GetWorld(), shape.m_fLength, shape.m_fRadius, shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform);
    }

    for (const auto& shape : m_AngleShapes)
    {
      wdDebugRenderer::DrawAngle(GetWorld(), shape.m_StartAngle, shape.m_EndAngle, wdColor::ZeroColor(), shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform, vBoneTangent, vBoneDir);
    }

    for (const auto& shape : m_ConeLimitShapes)
    {
      wdDebugRenderer::DrawLimitCone(GetWorld(), shape.m_Angle1, shape.m_Angle2, wdColor::ZeroColor(), shape.m_Color, GetOwner()->GetGlobalTransform() * shape.m_Transform);
    }

    for (const auto& shape : m_CylinderShapes)
    {
      wdDebugRenderer::DrawCylinder(GetWorld(), shape.m_fRadius1, shape.m_fRadius2, shape.m_fLength, shape.m_Color, wdColor::ZeroColor(), GetOwner()->GetGlobalTransform() * shape.m_Transform, false, false);
    }
  }
}

void wdSkeletonComponent::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_hSkeleton;
  s << m_bVisualizeBones;
  s << m_sBonesToHighlight;
  s << m_bVisualizeColliders;
  s << m_bVisualizeJoints;
  s << m_bVisualizeSwingLimits;
  s << m_bVisualizeTwistLimits;
}

void wdSkeletonComponent::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const wdUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  if (uiVersion <= 4)
    return;

  auto& s = inout_stream.GetStream();

  s >> m_hSkeleton;
  s >> m_bVisualizeBones;
  s >> m_sBonesToHighlight;
  s >> m_bVisualizeColliders;
  s >> m_bVisualizeJoints;
  s >> m_bVisualizeSwingLimits;
  s >> m_bVisualizeTwistLimits;
}

void wdSkeletonComponent::OnActivated()
{
  SUPER::OnActivated();

  m_MaxBounds.SetInvalid();
  VisualizeSkeletonDefaultState();
}

void wdSkeletonComponent::SetSkeletonFile(const char* szFile)
{
  wdSkeletonResourceHandle hResource;

  if (!wdStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = wdResourceManager::LoadResource<wdSkeletonResource>(szFile);
  }

  SetSkeleton(hResource);
}

const char* wdSkeletonComponent::GetSkeletonFile() const
{
  if (!m_hSkeleton.IsValid())
    return "";

  return m_hSkeleton.GetResourceID();
}


void wdSkeletonComponent::SetSkeleton(const wdSkeletonResourceHandle& hResource)
{
  if (m_hSkeleton != hResource)
  {
    m_hSkeleton = hResource;

    m_MaxBounds.SetInvalid();
    VisualizeSkeletonDefaultState();
  }
}

void wdSkeletonComponent::SetBonesToHighlight(const char* szFilter)
{
  if (m_sBonesToHighlight != szFilter)
  {
    m_sBonesToHighlight = szFilter;

    m_uiSkeletonChangeCounter = 0xFFFFFFFF;

    VisualizeSkeletonDefaultState();
  }
}

const char* wdSkeletonComponent::GetBonesToHighlight() const
{
  return m_sBonesToHighlight;
}

void wdSkeletonComponent::OnAnimationPoseUpdated(wdMsgAnimationPoseUpdated& msg)
{
  m_LinesSkeleton.Clear();
  m_SpheresShapes.Clear();
  m_BoxShapes.Clear();
  m_CapsuleShapes.Clear();
  m_AngleShapes.Clear();
  m_ConeLimitShapes.Clear();
  m_CylinderShapes.Clear();

  m_RootTransform = *msg.m_pRootTransform;

  BuildSkeletonVisualization(msg);
  BuildColliderVisualization(msg);
  BuildJointVisualization(msg);

  wdBoundingBox poseBounds;
  poseBounds.SetInvalid();

  for (const auto& bone : msg.m_ModelTransforms)
  {
    poseBounds.ExpandToInclude(bone.GetTranslationVector());
  }

  if (poseBounds.IsValid() && (!m_MaxBounds.IsValid() || !m_MaxBounds.Contains(poseBounds)))
  {
    m_MaxBounds.ExpandToInclude(poseBounds);
    TriggerLocalBoundsUpdate();
  }
  else if (((wdRenderWorld::GetFrameCounter() + GetUniqueIdForRendering()) & (WD_BIT(10) - 1)) == 0) // reset the bbox every once in a while
  {
    m_MaxBounds = poseBounds;
    TriggerLocalBoundsUpdate();
  }
}

void wdSkeletonComponent::BuildSkeletonVisualization(wdMsgAnimationPoseUpdated& msg)
{
  if (!m_bVisualizeBones || !msg.m_pSkeleton)
    return;

  wdStringBuilder tmp;

  struct Bone
  {
    wdVec3 pos = wdVec3::ZeroVector();
    wdVec3 dir = wdVec3::ZeroVector();
    float distToParent = 0.0f;
    float minDistToChild = 10.0f;
    bool highlight = false;
  };

  wdHybridArray<Bone, 128> bones;

  bones.SetCount(msg.m_pSkeleton->GetJointCount());
  m_LinesSkeleton.Reserve(m_LinesSkeleton.GetCount() + msg.m_pSkeleton->GetJointCount());

  const wdVec3 vBoneDir = wdBasisAxis::GetBasisVector(msg.m_pSkeleton->m_BoneDirection);

  auto renderBone = [&](int iCurrentBone, int iParentBone) {
    if (iParentBone == ozz::animation::Skeleton::kNoParent)
      return;

    const wdVec3 v0 = *msg.m_pRootTransform * msg.m_ModelTransforms[iParentBone].GetTranslationVector();
    const wdVec3 v1 = *msg.m_pRootTransform * msg.m_ModelTransforms[iCurrentBone].GetTranslationVector();

    wdVec3 dirToBone = (v1 - v0);

    auto& bone = bones[iCurrentBone];
    bone.pos = v1;
    bone.distToParent = dirToBone.GetLength();
    bone.dir = *msg.m_pRootTransform * msg.m_ModelTransforms[iCurrentBone].TransformDirection(vBoneDir);
    bone.dir.NormalizeIfNotZero(wdVec3::ZeroVector()).IgnoreResult();

    auto& pb = bones[iParentBone];

    if (!pb.dir.IsZero() && dirToBone.NormalizeIfNotZero(wdVec3::ZeroVector()).Succeeded())
    {
      if (pb.dir.GetAngleBetween(dirToBone) < wdAngle::Degree(45))
      {
        wdPlane plane;
        plane.SetFromNormalAndPoint(pb.dir, pb.pos);
        pb.minDistToChild = wdMath::Min(pb.minDistToChild, plane.GetDistanceTo(v1));
      }
    }
  };

  ozz::animation::IterateJointsDF(msg.m_pSkeleton->GetOzzSkeleton(), renderBone);

  if (m_sBonesToHighlight == "*")
  {
    for (wdUInt32 b = 0; b < bones.GetCount(); ++b)
    {
      bones[b].highlight = true;
    }
  }
  else if (!m_sBonesToHighlight.IsEmpty())
  {
    const wdStringBuilder mask(";", m_sBonesToHighlight, ";");

    for (wdUInt16 b = 0; b < static_cast<wdUInt16>(bones.GetCount()); ++b)
    {
      const wdString currentName = msg.m_pSkeleton->GetJointByIndex(b).GetName().GetString();

      tmp.Set(";", currentName, ";");

      if (mask.FindSubString(tmp))
      {
        bones[b].highlight = true;
      }
    }
  }

  for (wdUInt32 b = 0; b < bones.GetCount(); ++b)
  {
    const auto& bone = bones[b];

    if (!bone.highlight)
    {
      float len = 0.3f;

      if (bone.minDistToChild < 10.0f)
      {
        len = bone.minDistToChild;
      }
      else if (bone.distToParent > 0)
      {
        len = wdMath::Max(bone.distToParent * 0.5f, 0.1f);
      }
      else
      {
        len = 0.1f;
      }

      wdVec3 v0 = bone.pos;
      wdVec3 v1 = bone.pos + bone.dir * len;

      m_LinesSkeleton.PushBack(wdDebugRenderer::Line(v0, v1));
      m_LinesSkeleton.PeekBack().m_startColor = wdColor::DarkCyan;
      m_LinesSkeleton.PeekBack().m_endColor = wdColor::DarkCyan;
    }
  }

  for (wdUInt32 b = 0; b < bones.GetCount(); ++b)
  {
    const auto& bone = bones[b];

    if (bone.highlight && !bone.dir.IsZero(0.0001f))
    {
      float len = 0.3f;

      if (bone.minDistToChild < 10.0f)
      {
        len = bone.minDistToChild;
      }
      else if (bone.distToParent > 0)
      {
        len = wdMath::Max(bone.distToParent * 0.5f, 0.1f);
      }
      else
      {
        len = 0.1f;
      }

      wdVec3 v0 = bone.pos;
      wdVec3 v1 = bone.pos + bone.dir * len;

      const wdVec3 vO1 = bone.dir.GetOrthogonalVector().GetNormalized();
      const wdVec3 vO2 = bone.dir.CrossRH(vO1).GetNormalized();

      wdVec3 s[4];
      s[0] = v0 + vO1 * len * 0.1f + bone.dir * len * 0.1f;
      s[1] = v0 + vO2 * len * 0.1f + bone.dir * len * 0.1f;
      s[2] = v0 - vO1 * len * 0.1f + bone.dir * len * 0.1f;
      s[3] = v0 - vO2 * len * 0.1f + bone.dir * len * 0.1f;

      m_LinesSkeleton.PushBack(wdDebugRenderer::Line(v0, v1));
      m_LinesSkeleton.PeekBack().m_startColor = wdColor::DarkCyan;
      m_LinesSkeleton.PeekBack().m_endColor = wdColor::DarkCyan;

      for (wdUInt32 si = 0; si < 4; ++si)
      {
        m_LinesSkeleton.PushBack(wdDebugRenderer::Line(v0, s[si]));
        m_LinesSkeleton.PeekBack().m_startColor = wdColor::Chartreuse;
        m_LinesSkeleton.PeekBack().m_endColor = wdColor::Chartreuse;

        m_LinesSkeleton.PushBack(wdDebugRenderer::Line(s[si], v1));
        m_LinesSkeleton.PeekBack().m_startColor = wdColor::Chartreuse;
        m_LinesSkeleton.PeekBack().m_endColor = wdColor::Chartreuse;
      }
    }
  }
}

void wdSkeletonComponent::BuildColliderVisualization(wdMsgAnimationPoseUpdated& msg)
{
  if (!m_bVisualizeColliders || !msg.m_pSkeleton || !m_hSkeleton.IsValid())
    return;

  wdResourceLock<wdSkeletonResource> pSkeleton(m_hSkeleton, wdResourceAcquireMode::BlockTillLoaded);

  const auto srcBoneDir = pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection;
  const wdQuat qBoneDirAdjustment = wdBasisAxis::GetBasisRotation(wdBasisAxis::PositiveX, srcBoneDir);

  wdStringBuilder bonesToHighlight(";", m_sBonesToHighlight, ";");
  wdStringBuilder boneName;
  if (m_sBonesToHighlight == "*")
    bonesToHighlight.Clear();

  wdQuat qRotZtoX; // the capsule should extend along X, but the debug renderer draws them along Z
  qRotZtoX.SetFromAxisAndAngle(wdVec3(0, 1, 0), wdAngle::Degree(-90));

  for (const auto& geo : pSkeleton->GetDescriptor().m_Geometry)
  {
    if (geo.m_Type == wdSkeletonJointGeometryType::None)
      continue;

    wdMat4 boneTrans;
    wdQuat boneRot;
    msg.ComputeFullBoneTransform(geo.m_uiAttachedToJoint, boneTrans, boneRot);

    boneName.Set(";", msg.m_pSkeleton->GetJointByIndex(geo.m_uiAttachedToJoint).GetName().GetString(), ";");
    const bool bHighlight = bonesToHighlight.IsEmpty() || bonesToHighlight.FindLastSubString(boneName) != nullptr;
    const wdColor hlS = wdMath::Lerp(wdColor::DimGrey, wdColor::Yellow, bHighlight ? 1.0f : 0.2f);

    const wdQuat qFinalBoneRot = boneRot * qBoneDirAdjustment;

    wdTransform st;
    st.SetIdentity();
    st.m_vPosition = boneTrans.GetTranslationVector() + qFinalBoneRot * geo.m_Transform.m_vPosition;
    st.m_qRotation = qFinalBoneRot * geo.m_Transform.m_qRotation;

    if (geo.m_Type == wdSkeletonJointGeometryType::Sphere)
    {
      auto& shape = m_SpheresShapes.ExpandAndGetRef();
      shape.m_Transform = st;
      shape.m_Shape.SetElements(wdVec3::ZeroVector(), geo.m_Transform.m_vScale.z);
      shape.m_Color = hlS;
    }

    if (geo.m_Type == wdSkeletonJointGeometryType::Box)
    {
      auto& shape = m_BoxShapes.ExpandAndGetRef();

      wdVec3 ext;
      ext.x = geo.m_Transform.m_vScale.x * 0.5f;
      ext.y = geo.m_Transform.m_vScale.y * 0.5f;
      ext.z = geo.m_Transform.m_vScale.z * 0.5f;

      // TODO: if offset desired
      st.m_vPosition += qFinalBoneRot * wdVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

      shape.m_Transform = st;
      shape.m_Shape.SetCenterAndHalfExtents(wdVec3::ZeroVector(), ext);
      shape.m_Color = hlS;
    }

    if (geo.m_Type == wdSkeletonJointGeometryType::Capsule)
    {
      st.m_qRotation = st.m_qRotation * qRotZtoX;

      // TODO: if offset desired
      st.m_vPosition += qFinalBoneRot * wdVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

      auto& shape = m_CapsuleShapes.ExpandAndGetRef();
      shape.m_Transform = st;
      shape.m_fLength = geo.m_Transform.m_vScale.x;
      shape.m_fRadius = geo.m_Transform.m_vScale.z;
      shape.m_Color = hlS;
    }
  }
}

void wdSkeletonComponent::BuildJointVisualization(wdMsgAnimationPoseUpdated& msg)
{
  if (!m_hSkeleton.IsValid() || (!m_bVisualizeJoints && !m_bVisualizeSwingLimits && !m_bVisualizeTwistLimits))
    return;

  wdResourceLock<wdSkeletonResource> pSkeleton(m_hSkeleton, wdResourceAcquireMode::BlockTillLoaded);
  const auto& skel = pSkeleton->GetDescriptor().m_Skeleton;

  wdStringBuilder bonesToHighlight(";", m_sBonesToHighlight, ";");
  wdStringBuilder boneName;
  if (m_sBonesToHighlight == "*")
    bonesToHighlight.Clear();

  const wdQuat qBoneDir = wdBasisAxis::GetBasisRotation_PosX(pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);
  const wdQuat qBoneDirT = wdBasisAxis::GetBasisRotation(wdBasisAxis::PositiveY, pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);
  const wdQuat qBoneDirBT = wdBasisAxis::GetBasisRotation(wdBasisAxis::PositiveZ, pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);
  const wdQuat qBoneDirT2 = wdBasisAxis::GetBasisRotation(wdBasisAxis::NegativeY, pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection);

  for (wdUInt16 uiJointIdx = 0; uiJointIdx < skel.GetJointCount(); ++uiJointIdx)
  {
    const auto& thisJoint = skel.GetJointByIndex(uiJointIdx);
    const wdUInt16 uiParentIdx = thisJoint.GetParentIndex();

    if (thisJoint.IsRootJoint())
      continue;

    boneName.Set(";", thisJoint.GetName().GetString(), ";");

    const bool bHighlight = bonesToHighlight.IsEmpty() || bonesToHighlight.FindSubString(boneName) != nullptr;

    wdMat4 parentTrans;
    wdQuat parentRot; // contains root transform
    msg.ComputeFullBoneTransform(uiParentIdx, parentTrans, parentRot);

    wdMat4 thisTrans; // contains root transform
    wdQuat thisRot;   // contains root transform
    msg.ComputeFullBoneTransform(uiJointIdx, thisTrans, thisRot);

    const wdVec3 vJointPos = thisTrans.GetTranslationVector();
    const wdQuat qLimitRot = parentRot * thisJoint.GetLocalOrientation();

    // main directions
    if (m_bVisualizeJoints)
    {
      const wdColor hlM = wdMath::Lerp(wdColor::OrangeRed, wdColor::DimGrey, bHighlight ? 0 : 0.8f);
      const wdColor hlT = wdMath::Lerp(wdColor::LawnGreen, wdColor::DimGrey, bHighlight ? 0 : 0.8f);
      const wdColor hlBT = wdMath::Lerp(wdColor::BlueViolet, wdColor::DimGrey, bHighlight ? 0 : 0.8f);

      {
        auto& cyl = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color = hlM;
        cyl.m_fLength = 0.07f;
        cyl.m_fRadius1 = 0.002f;
        cyl.m_fRadius2 = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDir;
        cyl.m_Transform.m_vScale.Set(1);
      }

      {
        auto& cyl = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color = hlT;
        cyl.m_fLength = 0.07f;
        cyl.m_fRadius1 = 0.002f;
        cyl.m_fRadius2 = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDirT;
        cyl.m_Transform.m_vScale.Set(1);
      }

      {
        auto& cyl = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color = hlBT;
        cyl.m_fLength = 0.07f;
        cyl.m_fRadius1 = 0.002f;
        cyl.m_fRadius2 = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDirBT;
        cyl.m_Transform.m_vScale.Set(1);
      }
    }

    // swing limit
    if (m_bVisualizeSwingLimits && (thisJoint.GetHalfSwingLimitY() > wdAngle() || thisJoint.GetHalfSwingLimitZ() > wdAngle()))
    {
      auto& shape = m_ConeLimitShapes.ExpandAndGetRef();
      shape.m_Angle1 = thisJoint.GetHalfSwingLimitY();
      shape.m_Angle2 = thisJoint.GetHalfSwingLimitZ();
      shape.m_Color = wdMath::Lerp(wdColor::DimGrey, wdColor::DeepPink, bHighlight ? 1.0f : 0.2f);
      shape.m_Transform.m_vScale.Set(0.05f);
      shape.m_Transform.m_vPosition = vJointPos;
      shape.m_Transform.m_qRotation = qLimitRot * qBoneDir;

      const wdColor hlM = wdMath::Lerp(wdColor::OrangeRed, wdColor::DimGrey, bHighlight ? 0 : 0.8f);

      {
        auto& cyl = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color = hlM;
        cyl.m_fLength = 0.07f;
        cyl.m_fRadius1 = 0.002f;
        cyl.m_fRadius2 = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDir;
        cyl.m_Transform.m_vScale.Set(1);
      }
    }

    // twist limit
    if (m_bVisualizeTwistLimits && thisJoint.GetTwistLimitHalfAngle() > wdAngle::Degree(0))
    {
      auto& shape = m_AngleShapes.ExpandAndGetRef();
      shape.m_StartAngle = thisJoint.GetTwistLimitLow();
      shape.m_EndAngle = thisJoint.GetTwistLimitHigh();
      shape.m_Color = wdMath::Lerp(wdColor::DimGrey, wdColor::LightPink, bHighlight ? 0.8f : 0.2f);
      shape.m_Transform.m_vScale.Set(0.04f);
      shape.m_Transform.m_vPosition = vJointPos;
      shape.m_Transform.m_qRotation = qLimitRot;

      const wdColor hlT = wdMath::Lerp(wdColor::DimGrey, wdColor::LightPink, bHighlight ? 1.0f : 0.4f);

      {
        auto& cyl = m_CylinderShapes.ExpandAndGetRef();
        cyl.m_Color = hlT;
        cyl.m_fLength = 0.07f;
        cyl.m_fRadius1 = 0.002f;
        cyl.m_fRadius2 = 0.0f;
        cyl.m_Transform.m_vPosition = vJointPos;
        cyl.m_Transform.m_qRotation = thisRot * qBoneDirT2;
        cyl.m_Transform.m_vScale.Set(1);

        wdVec3 vDir = cyl.m_Transform.m_qRotation * wdVec3(1, 0, 0);
        vDir.Normalize();

        wdVec3 vDirRef = shape.m_Transform.m_qRotation * qBoneDir * wdVec3(0, 1, 0);
        vDirRef.Normalize();

        const wdVec3 vRotDir = shape.m_Transform.m_qRotation * qBoneDir * wdVec3(1, 0, 0);
        wdQuat qRotRef;
        qRotRef.SetFromAxisAndAngle(vRotDir, thisJoint.GetTwistLimitCenterAngle());
        vDirRef = qRotRef * vDirRef;

        // if the current twist is outside the twist limit range, highlight the bone
        if (vDir.GetAngleBetween(vDirRef) > thisJoint.GetTwistLimitHalfAngle())
        {
          cyl.m_Color = wdColor::Orange;
        }
      }
    }
  }
}

void wdSkeletonComponent::VisualizeSkeletonDefaultState()
{
  if (!IsActiveAndInitialized())
    return;

  m_uiSkeletonChangeCounter = 0;

  if (m_hSkeleton.IsValid())
  {
    wdResourceLock<wdSkeletonResource> pSkeleton(m_hSkeleton, wdResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pSkeleton.GetAcquireResult() == wdResourceAcquireResult::Final)
    {
      m_uiSkeletonChangeCounter = pSkeleton->GetCurrentResourceChangeCounter();

      if (pSkeleton->GetDescriptor().m_Skeleton.GetJointCount() > 0)
      {
        ozz::vector<ozz::math::Float4x4> modelTransforms;
        modelTransforms.resize(pSkeleton->GetDescriptor().m_Skeleton.GetJointCount());

        {
          ozz::animation::LocalToModelJob job;
          job.input = pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().joint_rest_poses();
          job.output = make_span(modelTransforms);
          job.skeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();
          job.Run();
        }

        wdMsgAnimationPoseUpdated msg;
        msg.m_pRootTransform = &pSkeleton->GetDescriptor().m_RootTransform;
        msg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
        msg.m_ModelTransforms = wdArrayPtr<const wdMat4>(reinterpret_cast<const wdMat4*>(&modelTransforms[0]), (wdUInt32)modelTransforms.size());

        OnAnimationPoseUpdated(msg);
      }
    }
  }

  TriggerLocalBoundsUpdate();
}

wdDebugRenderer::Line& wdSkeletonComponent::AddLine(const wdVec3& vStart, const wdVec3& vEnd, const wdColor& color)
{
  auto& line = m_LinesSkeleton.ExpandAndGetRef();
  line.m_start = vStart;
  line.m_end = vEnd;
  line.m_startColor = color;
  line.m_endColor = color;
  return line;
}

void wdSkeletonComponent::OnQueryAnimationSkeleton(wdMsgQueryAnimationSkeleton& msg)
{
  // if we have a skeleton, always overwrite it any incoming message with that
  if (m_hSkeleton.IsValid())
  {
    msg.m_hSkeleton = m_hSkeleton;
  }
}

WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_SkeletonComponent);
