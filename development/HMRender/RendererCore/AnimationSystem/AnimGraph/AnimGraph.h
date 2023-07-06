#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Core/Utils/Blackboard.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Types/SharedPtr.h>

class wdGameObject;

using wdSkeletonResourceHandle = wdTypedResourceHandle<class wdSkeletonResource>;

WD_DEFINE_AS_POD_TYPE(ozz::math::SimdFloat4);

struct wdAnimGraphPinDataBoneWeights
{
  wdUInt16 m_uiOwnIndex = 0xFFFF;
  float m_fOverallWeight = 1.0f;
  const wdAnimGraphSharedBoneWeights* m_pSharedBoneWeights = nullptr;
};

struct wdAnimGraphPinDataLocalTransforms
{
  wdUInt16 m_uiOwnIndex = 0xFFFF;
  wdAnimPoseGeneratorCommandID m_CommandID;
  const wdAnimGraphPinDataBoneWeights* m_pWeights = nullptr;
  float m_fOverallWeight = 1.0f;
  wdVec3 m_vRootMotion = wdVec3::ZeroVector();
  bool m_bUseRootMotion = false;
};

struct wdAnimGraphPinDataModelTransforms
{
  wdUInt16 m_uiOwnIndex = 0xFFFF;
  wdAnimPoseGeneratorCommandID m_CommandID;
  wdVec3 m_vRootMotion = wdVec3::ZeroVector();
  wdAngle m_RootRotationX;
  wdAngle m_RootRotationY;
  wdAngle m_RootRotationZ;
  bool m_bUseRootMotion = false;
};

class WD_RENDERERCORE_DLL wdAnimGraph
{
  WD_DISALLOW_COPY_AND_ASSIGN(wdAnimGraph);

public:
  wdAnimGraph();
  ~wdAnimGraph();

  void Configure(const wdSkeletonResourceHandle& hSkeleton, wdAnimPoseGenerator& ref_poseGenerator, const wdSharedPtr<wdBlackboard>& pBlackboard = nullptr);

  void Update(wdTime diff, wdGameObject* pTarget);
  void GetRootMotion(wdVec3& ref_vTranslation, wdAngle& ref_rotationX, wdAngle& ref_rotationY, wdAngle& ref_rotationZ) const;

  const wdSharedPtr<wdBlackboard>& GetBlackboard() { return m_pBlackboard; }

  wdResult Serialize(wdStreamWriter& inout_stream) const;
  wdResult Deserialize(wdStreamReader& inout_stream);

  wdAnimPoseGenerator& GetPoseGenerator() { return *m_pPoseGenerator; }

  static wdSharedPtr<wdAnimGraphSharedBoneWeights> CreateBoneWeights(const char* szUniqueName, const wdSkeletonResource& skeleton, wdDelegate<void(wdAnimGraphSharedBoneWeights&)> fill);

  wdAnimGraphPinDataBoneWeights* AddPinDataBoneWeights();
  wdAnimGraphPinDataLocalTransforms* AddPinDataLocalTransforms();
  wdAnimGraphPinDataModelTransforms* AddPinDataModelTransforms();

  void SetOutputModelTransform(wdAnimGraphPinDataModelTransforms* pModelTransform);
  void SetRootMotion(const wdVec3& vTranslation, wdAngle rotationX, wdAngle rotationY, wdAngle rotationZ);

private:
  wdDynamicArray<wdUniquePtr<wdAnimGraphNode>> m_Nodes;
  wdSkeletonResourceHandle m_hSkeleton;

  wdDynamicArray<wdDynamicArray<wdUInt16>> m_OutputPinToInputPinMapping[wdAnimGraphPin::ENUM_COUNT];

  // EXTEND THIS if a new type is introduced
  wdDynamicArray<wdInt8> m_TriggerInputPinStates;
  wdDynamicArray<double> m_NumberInputPinStates;
  wdDynamicArray<wdUInt16> m_BoneWeightInputPinStates;
  wdDynamicArray<wdHybridArray<wdUInt16, 1>> m_LocalPoseInputPinStates;
  wdDynamicArray<wdUInt16> m_ModelPoseInputPinStates;

  wdAnimGraphPinDataModelTransforms* m_pCurrentModelTransforms = nullptr;

  wdVec3 m_vRootMotion = wdVec3::ZeroVector();
  wdAngle m_RootRotationX;
  wdAngle m_RootRotationY;
  wdAngle m_RootRotationZ;

private:
  friend class wdAnimationControllerAssetDocument;
  friend class wdAnimGraphTriggerOutputPin;
  friend class wdAnimGraphTriggerInputPin;
  friend class wdAnimGraphBoneWeightsInputPin;
  friend class wdAnimGraphBoneWeightsOutputPin;
  friend class wdAnimGraphLocalPoseInputPin;
  friend class wdAnimGraphLocalPoseOutputPin;
  friend class wdAnimGraphModelPoseInputPin;
  friend class wdAnimGraphModelPoseOutputPin;
  friend class wdAnimGraphLocalPoseMultiInputPin;
  friend class wdAnimGraphNumberInputPin;
  friend class wdAnimGraphNumberOutputPin;

  bool m_bInitialized = false;

  wdAnimPoseGenerator* m_pPoseGenerator = nullptr;
  wdSharedPtr<wdBlackboard> m_pBlackboard = nullptr;

  wdHybridArray<wdAnimGraphPinDataBoneWeights, 4> m_PinDataBoneWeights;
  wdHybridArray<wdAnimGraphPinDataLocalTransforms, 4> m_PinDataLocalTransforms;
  wdHybridArray<wdAnimGraphPinDataModelTransforms, 2> m_PinDataModelTransforms;

  static wdMutex s_SharedDataMutex;
  static wdHashTable<wdString, wdSharedPtr<wdAnimGraphSharedBoneWeights>> s_SharedBoneWeights;
};
