#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

#include <ozz/animation/runtime/skeleton.h>

wdMutex wdAnimGraph::s_SharedDataMutex;
wdHashTable<wdString, wdSharedPtr<wdAnimGraphSharedBoneWeights>> wdAnimGraph::s_SharedBoneWeights;

wdAnimGraph::wdAnimGraph() = default;
wdAnimGraph::~wdAnimGraph() = default;

void wdAnimGraph::Configure(const wdSkeletonResourceHandle& hSkeleton, wdAnimPoseGenerator& ref_poseGenerator, const wdSharedPtr<wdBlackboard>& pBlackboard /*= nullptr*/)
{
  m_hSkeleton = hSkeleton;
  m_pPoseGenerator = &ref_poseGenerator;
  m_pBlackboard = pBlackboard;
}

void wdAnimGraph::Update(wdTime diff, wdGameObject* pTarget)
{
  if (!m_hSkeleton.IsValid())
    return;

  wdResourceLock<wdSkeletonResource> pSkeleton(m_hSkeleton, wdResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != wdResourceAcquireResult::Final)
    return;

  if (!m_bInitialized)
  {
    m_bInitialized = true;

    WD_LOG_BLOCK("Initializing animation controller graph");

    for (const auto& pNode : m_Nodes)
    {
      pNode->Initialize(*this, pSkeleton.GetPointer());
    }
  }

  m_pCurrentModelTransforms = nullptr;

  m_pPoseGenerator->Reset(pSkeleton.GetPointer());

  // reset all pin states
  {
    m_PinDataBoneWeights.Clear();
    m_PinDataLocalTransforms.Clear();
    m_PinDataModelTransforms.Clear();

    for (auto& pin : m_TriggerInputPinStates)
    {
      pin = 0;
    }
    for (auto& pin : m_NumberInputPinStates)
    {
      pin = 0;
    }
    for (auto& pin : m_BoneWeightInputPinStates)
    {
      pin = 0xFFFF;
    }
    for (auto& pins : m_LocalPoseInputPinStates)
    {
      pins.Clear();
    }
    for (auto& pin : m_ModelPoseInputPinStates)
    {
      pin = 0xFFFF;
    }
  }

  for (const auto& pNode : m_Nodes)
  {
    pNode->Step(*this, diff, pSkeleton.GetPointer(), pTarget);
  }

  if (auto newPose = GetPoseGenerator().GeneratePose(pTarget); !newPose.IsEmpty())
  {
    wdMsgAnimationPoseUpdated msg;
    msg.m_pRootTransform = &pSkeleton->GetDescriptor().m_RootTransform;
    msg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
    msg.m_ModelTransforms = newPose;

    pTarget->SendMessageRecursive(msg);
  }
}

void wdAnimGraph::GetRootMotion(wdVec3& ref_vTranslation, wdAngle& ref_rotationX, wdAngle& ref_rotationY, wdAngle& ref_rotationZ) const
{
  ref_vTranslation = m_vRootMotion;
  ref_rotationX = m_RootRotationX;
  ref_rotationY = m_RootRotationY;
  ref_rotationZ = m_RootRotationZ;
}

wdResult wdAnimGraph::Serialize(wdStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(5);

  const wdUInt32 uiNumNodes = m_Nodes.GetCount();
  inout_stream << uiNumNodes;

  for (const auto& node : m_Nodes)
  {
    inout_stream << node->GetDynamicRTTI()->GetTypeName();

    WD_SUCCEED_OR_RETURN(node->SerializeNode(inout_stream));
  }

  inout_stream << m_hSkeleton;

  {
    WD_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_TriggerInputPinStates));

    inout_stream << m_OutputPinToInputPinMapping[wdAnimGraphPin::Trigger].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[wdAnimGraphPin::Trigger])
    {
      WD_SUCCEED_OR_RETURN(inout_stream.WriteArray(ar));
    }
  }
  {
    WD_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_NumberInputPinStates));

    inout_stream << m_OutputPinToInputPinMapping[wdAnimGraphPin::Number].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[wdAnimGraphPin::Number])
    {
      WD_SUCCEED_OR_RETURN(inout_stream.WriteArray(ar));
    }
  }
  {
    inout_stream << m_BoneWeightInputPinStates.GetCount();

    inout_stream << m_OutputPinToInputPinMapping[wdAnimGraphPin::BoneWeights].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[wdAnimGraphPin::BoneWeights])
    {
      WD_SUCCEED_OR_RETURN(inout_stream.WriteArray(ar));
    }
  }
  {
    inout_stream << m_LocalPoseInputPinStates.GetCount();

    inout_stream << m_OutputPinToInputPinMapping[wdAnimGraphPin::LocalPose].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[wdAnimGraphPin::LocalPose])
    {
      WD_SUCCEED_OR_RETURN(inout_stream.WriteArray(ar));
    }
  }
  {
    inout_stream << m_ModelPoseInputPinStates.GetCount();

    inout_stream << m_OutputPinToInputPinMapping[wdAnimGraphPin::ModelPose].GetCount();
    for (const auto& ar : m_OutputPinToInputPinMapping[wdAnimGraphPin::ModelPose])
    {
      WD_SUCCEED_OR_RETURN(inout_stream.WriteArray(ar));
    }
  }
  // EXTEND THIS if a new type is introduced

  return WD_SUCCESS;
}

wdResult wdAnimGraph::Deserialize(wdStreamReader& inout_stream)
{
  const auto uiVersion = inout_stream.ReadVersion(5);

  wdUInt32 uiNumNodes = 0;
  inout_stream >> uiNumNodes;
  m_Nodes.SetCount(uiNumNodes);

  wdStringBuilder sTypeName;

  for (auto& node : m_Nodes)
  {
    inout_stream >> sTypeName;
    node = std::move(wdRTTI::FindTypeByName(sTypeName)->GetAllocator()->Allocate<wdAnimGraphNode>());

    WD_SUCCEED_OR_RETURN(node->DeserializeNode(inout_stream));
  }

  inout_stream >> m_hSkeleton;

  if (uiVersion >= 2)
  {
    WD_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_TriggerInputPinStates));

    wdUInt32 sar = 0;
    inout_stream >> sar;
    m_OutputPinToInputPinMapping[wdAnimGraphPin::Trigger].SetCount(sar);
    for (auto& ar : m_OutputPinToInputPinMapping[wdAnimGraphPin::Trigger])
    {
      WD_SUCCEED_OR_RETURN(inout_stream.ReadArray(ar));
    }
  }
  if (uiVersion >= 3)
  {
    WD_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_NumberInputPinStates));

    wdUInt32 sar = 0;
    inout_stream >> sar;
    m_OutputPinToInputPinMapping[wdAnimGraphPin::Number].SetCount(sar);
    for (auto& ar : m_OutputPinToInputPinMapping[wdAnimGraphPin::Number])
    {
      WD_SUCCEED_OR_RETURN(inout_stream.ReadArray(ar));
    }
  }
  if (uiVersion >= 4)
  {
    wdUInt32 sar = 0;

    inout_stream >> sar;
    m_BoneWeightInputPinStates.SetCount(sar);

    inout_stream >> sar;
    m_OutputPinToInputPinMapping[wdAnimGraphPin::BoneWeights].SetCount(sar);
    for (auto& ar : m_OutputPinToInputPinMapping[wdAnimGraphPin::BoneWeights])
    {
      WD_SUCCEED_OR_RETURN(inout_stream.ReadArray(ar));
    }
  }
  if (uiVersion >= 5)
  {
    wdUInt32 sar = 0;

    inout_stream >> sar;
    m_LocalPoseInputPinStates.SetCount(sar);

    inout_stream >> sar;
    m_OutputPinToInputPinMapping[wdAnimGraphPin::LocalPose].SetCount(sar);
    for (auto& ar : m_OutputPinToInputPinMapping[wdAnimGraphPin::LocalPose])
    {
      WD_SUCCEED_OR_RETURN(inout_stream.ReadArray(ar));
    }
  }
  if (uiVersion >= 5)
  {
    wdUInt32 sar = 0;

    inout_stream >> sar;
    m_ModelPoseInputPinStates.SetCount(sar);

    inout_stream >> sar;
    m_OutputPinToInputPinMapping[wdAnimGraphPin::ModelPose].SetCount(sar);
    for (auto& ar : m_OutputPinToInputPinMapping[wdAnimGraphPin::ModelPose])
    {
      WD_SUCCEED_OR_RETURN(inout_stream.ReadArray(ar));
    }
  }
  // EXTEND THIS if a new type is introduced

  return WD_SUCCESS;
}

wdAnimGraphPinDataBoneWeights* wdAnimGraph::AddPinDataBoneWeights()
{
  wdAnimGraphPinDataBoneWeights* pData = &m_PinDataBoneWeights.ExpandAndGetRef();
  pData->m_uiOwnIndex = static_cast<wdUInt16>(m_PinDataBoneWeights.GetCount()) - 1;
  return pData;
}

wdAnimGraphPinDataLocalTransforms* wdAnimGraph::AddPinDataLocalTransforms()
{
  wdAnimGraphPinDataLocalTransforms* pData = &m_PinDataLocalTransforms.ExpandAndGetRef();
  pData->m_uiOwnIndex = static_cast<wdUInt16>(m_PinDataLocalTransforms.GetCount()) - 1;
  return pData;
}

wdAnimGraphPinDataModelTransforms* wdAnimGraph::AddPinDataModelTransforms()
{
  wdAnimGraphPinDataModelTransforms* pData = &m_PinDataModelTransforms.ExpandAndGetRef();
  pData->m_uiOwnIndex = static_cast<wdUInt16>(m_PinDataModelTransforms.GetCount()) - 1;
  return pData;
}

void wdAnimGraph::SetOutputModelTransform(wdAnimGraphPinDataModelTransforms* pModelTransform)
{
  m_pCurrentModelTransforms = pModelTransform;
}

void wdAnimGraph::SetRootMotion(const wdVec3& vTranslation, wdAngle rotationX, wdAngle rotationY, wdAngle rotationZ)
{
  m_vRootMotion = vTranslation;
  m_RootRotationX = rotationX;
  m_RootRotationY = rotationY;
  m_RootRotationZ = rotationZ;
}

wdSharedPtr<wdAnimGraphSharedBoneWeights> wdAnimGraph::CreateBoneWeights(const char* szUniqueName, const wdSkeletonResource& skeleton, wdDelegate<void(wdAnimGraphSharedBoneWeights&)> fill)
{
  WD_LOCK(s_SharedDataMutex);

  wdSharedPtr<wdAnimGraphSharedBoneWeights>& bw = s_SharedBoneWeights[szUniqueName];

  if (bw == nullptr)
  {
    bw = WD_DEFAULT_NEW(wdAnimGraphSharedBoneWeights);
    bw->m_Weights.SetCountUninitialized(skeleton.GetDescriptor().m_Skeleton.GetOzzSkeleton().num_soa_joints());
    wdMemoryUtils::ZeroFill<ozz::math::SimdFloat4>(bw->m_Weights.GetData(), bw->m_Weights.GetCount());
  }

  fill(*bw);

  return bw;
}


WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Implementation_AnimGraph);
