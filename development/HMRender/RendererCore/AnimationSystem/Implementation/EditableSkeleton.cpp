#include <RendererCore/RendererCorePCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <RendererCore/AnimationSystem/Implementation/OzzUtils.h>
#include <RendererCore/AnimationSystem/SkeletonBuilder.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/runtime/skeleton.h>

// clang-format off
WD_BEGIN_STATIC_REFLECTED_ENUM(wdSkeletonJointGeometryType, 1)
WD_ENUM_CONSTANTS(wdSkeletonJointGeometryType::None, wdSkeletonJointGeometryType::Capsule, wdSkeletonJointGeometryType::Sphere, wdSkeletonJointGeometryType::Box)
WD_END_STATIC_REFLECTED_ENUM;

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdEditableSkeletonBoneShape, 1, wdRTTIDefaultAllocator<wdEditableSkeletonBoneShape>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ENUM_MEMBER_PROPERTY("Geometry", wdSkeletonJointGeometryType, m_Geometry),
    WD_MEMBER_PROPERTY("Offset", m_vOffset),
    WD_MEMBER_PROPERTY("Rotation", m_qRotation),
    WD_MEMBER_PROPERTY("Length", m_fLength)->AddAttributes(new wdDefaultValueAttribute(0.1f), new wdClampValueAttribute(0.01f, 10.0f)),
    WD_MEMBER_PROPERTY("Width", m_fWidth)->AddAttributes(new wdDefaultValueAttribute(0.05f), new wdClampValueAttribute(0.01f, 10.0f)),
    WD_MEMBER_PROPERTY("Thickness", m_fThickness)->AddAttributes(new wdDefaultValueAttribute(0.05f), new wdClampValueAttribute(0.01f, 10.0f)),

    WD_MEMBER_PROPERTY("OverrideName", m_bOverrideName),
    WD_MEMBER_PROPERTY("Name", m_sNameOverride),
    WD_MEMBER_PROPERTY("OverrideSurface", m_bOverrideSurface),
    WD_MEMBER_PROPERTY("Surface", m_sSurfaceOverride)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Surface", wdDependencyFlags::Package)),
    WD_MEMBER_PROPERTY("OverrideCollisionLayer", m_bOverrideCollisionLayer),
    WD_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayerOverride)->AddAttributes(new wdDynamicEnumAttribute("PhysicsCollisionLayer")),

  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdEditableSkeletonJoint, 2, wdRTTIDefaultAllocator<wdEditableSkeletonJoint>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("Name", GetName, SetName),
    WD_MEMBER_PROPERTY("Transform", m_LocalTransform)->AddFlags(wdPropertyFlags::Hidden)->AddAttributes(new wdDefaultValueAttribute(wdTransform::IdentityTransform())),
    WD_MEMBER_PROPERTY_READ_ONLY("GizmoOffsetTranslationRO", m_vGizmoOffsetPositionRO)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY_READ_ONLY("GizmoOffsetRotationRO", m_qGizmoOffsetRotationRO)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("LocalRotation", m_qLocalJointRotation),
    WD_MEMBER_PROPERTY("LimitSwing", m_bLimitSwing),
    WD_MEMBER_PROPERTY("SwingLimitY", m_SwingLimitY)->AddAttributes(new wdClampValueAttribute(wdAngle(), wdAngle::Degree(170)), new wdDefaultValueAttribute(wdAngle::Degree(30))),
    WD_MEMBER_PROPERTY("SwingLimitZ", m_SwingLimitZ)->AddAttributes(new wdClampValueAttribute(wdAngle(), wdAngle::Degree(170)), new wdDefaultValueAttribute(wdAngle::Degree(30))),
    WD_MEMBER_PROPERTY("LimitTwist", m_bLimitTwist),
    WD_MEMBER_PROPERTY("TwistLimitHalfAngle", m_TwistLimitHalfAngle)->AddAttributes(new wdClampValueAttribute(wdAngle::Degree(10), wdAngle::Degree(170)), new wdDefaultValueAttribute(wdAngle::Degree(30))),
    WD_MEMBER_PROPERTY("TwistLimitCenterAngle", m_TwistLimitCenterAngle)->AddAttributes(new wdClampValueAttribute(-wdAngle::Degree(170), wdAngle::Degree(170))),
    WD_ARRAY_MEMBER_PROPERTY("Children", m_Children)->AddFlags(wdPropertyFlags::PointerOwner | wdPropertyFlags::Hidden),
    WD_ARRAY_MEMBER_PROPERTY("BoneShapes", m_BoneShapes),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdTransformManipulatorAttribute(nullptr, "LocalRotation", nullptr, "GizmoOffsetTranslationRO", "GizmoOffsetRotationRO"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdEditableSkeleton, 1, wdRTTIDefaultAllocator<wdEditableSkeleton>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("File", m_sSourceFile)->AddAttributes(new wdFileBrowserAttribute("Select Mesh", wdFileBrowserAttribute::MeshesWithAnimations)),
    WD_ENUM_MEMBER_PROPERTY("RightDir", wdBasisAxis, m_RightDir)->AddAttributes(new wdDefaultValueAttribute((int)wdBasisAxis::PositiveX)),
    WD_ENUM_MEMBER_PROPERTY("UpDir", wdBasisAxis, m_UpDir)->AddAttributes(new wdDefaultValueAttribute((int)wdBasisAxis::PositiveY)),
    WD_MEMBER_PROPERTY("FlipForwardDir", m_bFlipForwardDir),
    WD_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new wdDefaultValueAttribute(1.0f), new wdClampValueAttribute(0.0001f, 10000.0f)),
    WD_ENUM_MEMBER_PROPERTY("BoneDirection", wdBasisAxis, m_BoneDirection)->AddAttributes(new wdDefaultValueAttribute((int)wdBasisAxis::PositiveY)),
    WD_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new wdDynamicEnumAttribute("PhysicsCollisionLayer")),
    WD_MEMBER_PROPERTY("Surface", m_sSurfaceFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Surface", wdDependencyFlags::Package)),

    WD_ARRAY_MEMBER_PROPERTY("Children", m_Children)->AddFlags(wdPropertyFlags::PointerOwner | wdPropertyFlags::Hidden),
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;

WD_BEGIN_STATIC_REFLECTED_TYPE(wdExposedBone, wdNoBase, 1, wdRTTIDefaultAllocator<wdExposedBone>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("Name", m_sName),
    WD_MEMBER_PROPERTY("Parent", m_sParent),
    WD_MEMBER_PROPERTY("Transform", m_Transform),
  }
  WD_END_PROPERTIES;
}
WD_END_STATIC_REFLECTED_TYPE;

WD_DEFINE_CUSTOM_VARIANT_TYPE(wdExposedBone);
// clang-format on


void operator<<(wdStreamWriter& inout_stream, const wdExposedBone& bone)
{
  inout_stream << bone.m_sName;
  inout_stream << bone.m_sParent;
  inout_stream << bone.m_Transform;
}

void operator>>(wdStreamReader& inout_stream, wdExposedBone& ref_bone)
{
  inout_stream >> ref_bone.m_sName;
  inout_stream >> ref_bone.m_sParent;
  inout_stream >> ref_bone.m_Transform;
}

bool operator==(const wdExposedBone& lhs, const wdExposedBone& rhs)
{
  if (lhs.m_sName != rhs.m_sName)
    return false;
  if (lhs.m_sParent != rhs.m_sParent)
    return false;
  if (lhs.m_Transform != rhs.m_Transform)
    return false;
  return true;
}

wdEditableSkeleton::wdEditableSkeleton() = default;
wdEditableSkeleton::~wdEditableSkeleton()
{
  ClearJoints();
}

void wdEditableSkeleton::ClearJoints()
{
  for (wdEditableSkeletonJoint* pChild : m_Children)
  {
    WD_DEFAULT_DELETE(pChild);
  }

  m_Children.Clear();
}

void wdEditableSkeleton::CreateJointsRecursive(wdSkeletonBuilder& ref_sb, wdSkeletonResourceDescriptor& ref_desc, const wdEditableSkeletonJoint* pParentJoint, const wdEditableSkeletonJoint* pThisJoint, wdUInt16 uiThisJointIdx, const wdQuat& qParentAccuRot, const wdMat4& mRootTransform) const
{
  for (auto& shape : pThisJoint->m_BoneShapes)
  {
    auto& geo = ref_desc.m_Geometry.ExpandAndGetRef();

    geo.m_Type = shape.m_Geometry;
    geo.m_uiAttachedToJoint = static_cast<wdUInt16>(uiThisJointIdx);
    geo.m_sName = pThisJoint->m_sName;
    geo.m_Transform.SetIdentity();
    geo.m_Transform.m_vScale.Set(shape.m_fLength, shape.m_fWidth, shape.m_fThickness);
    geo.m_Transform.m_vPosition = shape.m_vOffset;
    geo.m_Transform.m_qRotation = shape.m_qRotation;
    geo.m_hSurface = wdResourceManager::LoadResource<wdSurfaceResource>(m_sSurfaceFile);
    geo.m_uiCollisionLayer = m_uiCollisionLayer;

    if (shape.m_bOverrideName)
      geo.m_sName.Assign(shape.m_sNameOverride);
    if (shape.m_bOverrideCollisionLayer)
      geo.m_uiCollisionLayer = shape.m_uiCollisionLayerOverride;
    if (shape.m_bOverrideSurface)
      geo.m_hSurface = wdResourceManager::LoadResource<wdSurfaceResource>(shape.m_sSurfaceOverride);
  }

  WD_ASSERT_DEBUG(pThisJoint->m_LocalTransform.m_vScale.IsEqual(wdVec3(1), 0.1f), "fuck");

  const wdQuat qThisAccuRot = qParentAccuRot * pThisJoint->m_LocalTransform.m_qRotation;
  wdQuat qParentGlobalRot;

  {
    // as always, the root transform is the bane of my existence
    // since it can contain mirroring, the final global rotation of a joint will be incorrect if we don't incorporate the root scale
    // unfortunately this can't be done once for the first node, but has to be done on the result instead

    wdMat4 full;
    wdMsgAnimationPoseUpdated::ComputeFullBoneTransform(mRootTransform, qParentAccuRot.GetAsMat4(), full, qParentGlobalRot);
  }

  ref_sb.SetJointLimit(uiThisJointIdx, pThisJoint->m_qLocalJointRotation, pThisJoint->m_bLimitSwing, pThisJoint->m_SwingLimitY, pThisJoint->m_SwingLimitZ, pThisJoint->m_bLimitTwist, pThisJoint->m_TwistLimitHalfAngle, pThisJoint->m_TwistLimitCenterAngle);

  for (const auto* pChildJoint : pThisJoint->m_Children)
  {
    const wdUInt16 uiChildJointIdx = ref_sb.AddJoint(pChildJoint->GetName(), pChildJoint->m_LocalTransform, uiThisJointIdx);

    CreateJointsRecursive(ref_sb, ref_desc, pThisJoint, pChildJoint, uiChildJointIdx, qThisAccuRot, mRootTransform);
  }
}

void wdEditableSkeleton::FillResourceDescriptor(wdSkeletonResourceDescriptor& ref_desc) const
{
  ref_desc.m_Geometry.Clear();

  wdSkeletonBuilder sb;
  for (const auto* pJoint : m_Children)
  {
    const wdUInt16 idx = sb.AddJoint(pJoint->GetName(), pJoint->m_LocalTransform);

    CreateJointsRecursive(sb, ref_desc, nullptr, pJoint, idx, wdQuat::IdentityQuaternion(), ref_desc.m_RootTransform.GetAsMat4());
  }

  sb.BuildSkeleton(ref_desc.m_Skeleton);
  ref_desc.m_Skeleton.m_BoneDirection = m_BoneDirection;
}

static void BuildOzzRawSkeleton(const wdEditableSkeletonJoint& srcJoint, ozz::animation::offline::RawSkeleton::Joint& ref_dstJoint)
{
  ref_dstJoint.name = srcJoint.m_sName.GetString();
  ref_dstJoint.transform.translation.x = srcJoint.m_LocalTransform.m_vPosition.x;
  ref_dstJoint.transform.translation.y = srcJoint.m_LocalTransform.m_vPosition.y;
  ref_dstJoint.transform.translation.z = srcJoint.m_LocalTransform.m_vPosition.z;
  ref_dstJoint.transform.rotation.x = srcJoint.m_LocalTransform.m_qRotation.v.x;
  ref_dstJoint.transform.rotation.y = srcJoint.m_LocalTransform.m_qRotation.v.y;
  ref_dstJoint.transform.rotation.z = srcJoint.m_LocalTransform.m_qRotation.v.z;
  ref_dstJoint.transform.rotation.w = srcJoint.m_LocalTransform.m_qRotation.w;
  ref_dstJoint.transform.scale.x = srcJoint.m_LocalTransform.m_vScale.x;
  ref_dstJoint.transform.scale.y = srcJoint.m_LocalTransform.m_vScale.y;
  ref_dstJoint.transform.scale.z = srcJoint.m_LocalTransform.m_vScale.z;

  ref_dstJoint.children.resize((size_t)srcJoint.m_Children.GetCount());

  for (wdUInt32 b = 0; b < srcJoint.m_Children.GetCount(); ++b)
  {
    BuildOzzRawSkeleton(*srcJoint.m_Children[b], ref_dstJoint.children[b]);
  }
}

void wdEditableSkeleton::GenerateRawOzzSkeleton(ozz::animation::offline::RawSkeleton& out_skeleton) const
{
  out_skeleton.roots.resize((size_t)m_Children.GetCount());

  for (wdUInt32 b = 0; b < m_Children.GetCount(); ++b)
  {
    BuildOzzRawSkeleton(*m_Children[b], out_skeleton.roots[b]);
  }
}

void wdEditableSkeleton::GenerateOzzSkeleton(ozz::animation::Skeleton& out_skeleton) const
{
  ozz::animation::offline::RawSkeleton rawSkeleton;
  GenerateRawOzzSkeleton(rawSkeleton);

  ozz::animation::offline::SkeletonBuilder skeletonBuilder;
  auto pNewOzzSkeleton = skeletonBuilder(rawSkeleton);

  wdOzzUtils::CopySkeleton(&out_skeleton, pNewOzzSkeleton.get());
}

wdEditableSkeletonJoint::wdEditableSkeletonJoint() = default;

wdEditableSkeletonJoint::~wdEditableSkeletonJoint()
{
  ClearJoints();
}

const char* wdEditableSkeletonJoint::GetName() const
{
  return m_sName.GetData();
}

void wdEditableSkeletonJoint::SetName(const char* szSz)
{
  m_sName.Assign(szSz);
}

void wdEditableSkeletonJoint::ClearJoints()
{
  for (wdEditableSkeletonJoint* pChild : m_Children)
  {
    WD_DEFAULT_DELETE(pChild);
  }
  m_Children.Clear();
}

void wdEditableSkeletonJoint::CopyPropertiesFrom(const wdEditableSkeletonJoint* pJoint)
{
  // do not copy:
  //  name
  //  transform
  //  children

  m_BoneShapes = pJoint->m_BoneShapes;
  m_qLocalJointRotation = pJoint->m_qLocalJointRotation;
  m_SwingLimitY = pJoint->m_SwingLimitY;
  m_SwingLimitZ = pJoint->m_SwingLimitZ;
  m_TwistLimitHalfAngle = pJoint->m_TwistLimitHalfAngle;
  m_TwistLimitCenterAngle = pJoint->m_TwistLimitCenterAngle;
  m_bLimitSwing = pJoint->m_bLimitSwing;
  m_bLimitTwist = pJoint->m_bLimitTwist;
}

WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_EditableSkeleton);
