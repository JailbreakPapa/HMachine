#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <RendererCore/AnimationSystem/Declarations.h>

class wdSkeletonBuilder;
class wdSkeleton;

namespace ozz::animation
{
  class Skeleton;

  namespace offline
  {
    struct RawSkeleton;
  }
} // namespace ozz::animation

WD_DECLARE_REFLECTABLE_TYPE(WD_RENDERERCORE_DLL, wdSkeletonJointGeometryType);

struct WD_RENDERERCORE_DLL wdEditableSkeletonBoneShape : public wdReflectedClass
{
  WD_ADD_DYNAMIC_REFLECTION(wdEditableSkeletonBoneShape, wdReflectedClass);

  wdEnum<wdSkeletonJointGeometryType> m_Geometry;

  wdVec3 m_vOffset = wdVec3::ZeroVector();
  wdQuat m_qRotation = wdQuat::IdentityQuaternion();

  float m_fLength = 0;    // Box, Capsule; 0 means parent joint to this joint (auto mode)
  float m_fWidth = 0;     // Box
  float m_fThickness = 0; // Sphere radius, Capsule radius

  bool m_bOverrideName = false;
  bool m_bOverrideSurface = false;
  bool m_bOverrideCollisionLayer = false;

  wdString m_sNameOverride;
  wdString m_sSurfaceOverride;
  wdUInt8 m_uiCollisionLayerOverride;
};

class WD_RENDERERCORE_DLL wdEditableSkeletonJoint : public wdReflectedClass
{
  WD_ADD_DYNAMIC_REFLECTION(wdEditableSkeletonJoint, wdReflectedClass);

public:
  wdEditableSkeletonJoint();
  ~wdEditableSkeletonJoint();

  const char* GetName() const;
  void SetName(const char* szSz);

  void ClearJoints();

  // copies the properties for geometry etc. from another joint
  // does NOT copy the name, the transform or the children
  void CopyPropertiesFrom(const wdEditableSkeletonJoint* pJoint);

  wdHashedString m_sName;
  wdTransform m_LocalTransform = wdTransform::IdentityTransform();

  bool m_bLimitTwist = false;
  bool m_bLimitSwing = false;

  wdAngle m_TwistLimitHalfAngle;
  wdAngle m_TwistLimitCenterAngle;
  wdAngle m_SwingLimitY;
  wdAngle m_SwingLimitZ;

  wdVec3 m_vGizmoOffsetPositionRO = wdVec3::ZeroVector();
  wdQuat m_qGizmoOffsetRotationRO = wdQuat::IdentityQuaternion();

  wdQuat m_qLocalJointRotation = wdQuat::IdentityQuaternion();

  wdHybridArray<wdEditableSkeletonJoint*, 4> m_Children;
  wdHybridArray<wdEditableSkeletonBoneShape, 1> m_BoneShapes;
};

class WD_RENDERERCORE_DLL wdEditableSkeleton : public wdReflectedClass
{
  WD_ADD_DYNAMIC_REFLECTION(wdEditableSkeleton, wdReflectedClass);

public:
  wdEditableSkeleton();
  ~wdEditableSkeleton();

  void ClearJoints();
  void FillResourceDescriptor(wdSkeletonResourceDescriptor& ref_desc) const;
  void GenerateRawOzzSkeleton(ozz::animation::offline::RawSkeleton& out_skeleton) const;
  void GenerateOzzSkeleton(ozz::animation::Skeleton& out_skeleton) const;
  void CreateJointsRecursive(wdSkeletonBuilder& ref_sb, wdSkeletonResourceDescriptor& ref_desc, const wdEditableSkeletonJoint* pParentJoint, const wdEditableSkeletonJoint* pThisJoint, wdUInt16 uiThisJointIdx, const wdQuat& qParentAccuRot, const wdMat4& mRootTransform) const;

  wdString m_sSourceFile;
  wdString m_sSurfaceFile;
  wdUInt8 m_uiCollisionLayer = 0;

  float m_fUniformScaling = 1.0f;

  wdEnum<wdBasisAxis> m_RightDir;
  wdEnum<wdBasisAxis> m_UpDir;
  bool m_bFlipForwardDir = false;
  wdEnum<wdBasisAxis> m_BoneDirection;

  wdHybridArray<wdEditableSkeletonJoint*, 4> m_Children;
};

struct WD_RENDERERCORE_DLL wdExposedBone
{
  wdString m_sName;
  wdString m_sParent;
  wdTransform m_Transform;
  // when adding new values, the hash function below has to be adjusted
};

WD_DECLARE_CUSTOM_VARIANT_TYPE(wdExposedBone);

WD_RENDERERCORE_DLL void operator<<(wdStreamWriter& inout_stream, const wdExposedBone& bone);
WD_RENDERERCORE_DLL void operator>>(wdStreamReader& inout_stream, wdExposedBone& ref_bone);
WD_RENDERERCORE_DLL bool operator==(const wdExposedBone& lhs, const wdExposedBone& rhs);

template <>
struct wdHashHelper<wdExposedBone>
{
  WD_ALWAYS_INLINE static wdUInt32 Hash(const wdExposedBone& value)
  {
    return wdHashingUtils::xxHash32String(value.m_sName) + wdHashingUtils::xxHash32String(value.m_sParent) + wdHashingUtils::xxHash32(&value, sizeof(wdTransform));
  }

  WD_ALWAYS_INLINE static bool Equal(const wdExposedBone& a, const wdExposedBone& b) { return a == b; }
};

WD_DECLARE_REFLECTABLE_TYPE(WD_RENDERERCORE_DLL, wdExposedBone);
