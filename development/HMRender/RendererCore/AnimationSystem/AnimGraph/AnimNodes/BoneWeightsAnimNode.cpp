#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/BoneWeightsAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

#include <ozz/animation/runtime/skeleton_utils.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdBoneWeightsAnimNode, 1, wdRTTIDefaultAllocator<wdBoneWeightsAnimNode>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("Weight", m_fWeight)->AddAttributes(new wdDefaultValueAttribute(1.0), new wdClampValueAttribute(0.0f, 1.0f)),
    WD_ARRAY_ACCESSOR_PROPERTY("RootBones", RootBones_GetCount, RootBones_GetValue, RootBones_SetValue, RootBones_Insert, RootBones_Remove),

    WD_MEMBER_PROPERTY("Weights", m_WeightsPin)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("InverseWeights", m_InverseWeightsPin)->AddAttributes(new wdHiddenAttribute()),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Weights"),
    new wdColorAttribute(wdColorScheme::DarkUI(wdColorScheme::Cyan)),
    new wdTitleAttribute("Bone Weights '{RootBones[0]}' '{RootBones[1]}' '{RootBones[2]}'"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdBoneWeightsAnimNode::wdBoneWeightsAnimNode() = default;
wdBoneWeightsAnimNode::~wdBoneWeightsAnimNode() = default;

wdUInt32 wdBoneWeightsAnimNode::RootBones_GetCount() const
{
  return m_RootBones.GetCount();
}

const char* wdBoneWeightsAnimNode::RootBones_GetValue(wdUInt32 uiIndex) const
{
  return m_RootBones[uiIndex].GetString();
}

void wdBoneWeightsAnimNode::RootBones_SetValue(wdUInt32 uiIndex, const char* value)
{
  m_RootBones[uiIndex].Assign(value);
}

void wdBoneWeightsAnimNode::RootBones_Insert(wdUInt32 uiIndex, const char* value)
{
  wdHashedString tmp;
  tmp.Assign(value);
  m_RootBones.Insert(tmp, uiIndex);
}

void wdBoneWeightsAnimNode::RootBones_Remove(wdUInt32 uiIndex)
{
  m_RootBones.RemoveAtAndCopy(uiIndex);
}

wdResult wdBoneWeightsAnimNode::SerializeNode(wdStreamWriter& stream) const
{
  stream.WriteVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  WD_SUCCEED_OR_RETURN(stream.WriteArray(m_RootBones));

  stream << m_fWeight;

  WD_SUCCEED_OR_RETURN(m_WeightsPin.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_InverseWeightsPin.Serialize(stream));

  return WD_SUCCESS;
}

wdResult wdBoneWeightsAnimNode::DeserializeNode(wdStreamReader& stream)
{
  stream.ReadVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  WD_SUCCEED_OR_RETURN(stream.ReadArray(m_RootBones));

  stream >> m_fWeight;

  WD_SUCCEED_OR_RETURN(m_WeightsPin.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_InverseWeightsPin.Deserialize(stream));

  return WD_SUCCESS;
}

void wdBoneWeightsAnimNode::Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget)
{
  if (!m_WeightsPin.IsConnected() && !m_InverseWeightsPin.IsConnected())
    return;

  if (m_RootBones.IsEmpty())
  {
    wdLog::Warning("No root-bones added to bone weight node in animation controller.");
    return;
  }

  if (m_pSharedBoneWeights == nullptr && m_pSharedInverseBoneWeights == nullptr)
  {
    const auto pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();

    wdStringBuilder name;
    name.Format("{}", pSkeleton->GetResourceIDHash());

    for (const auto& rootBone : m_RootBones)
    {
      name.AppendFormat("-{}", rootBone);
    }

    m_pSharedBoneWeights = graph.CreateBoneWeights(name, *pSkeleton, [this, pOzzSkeleton](wdAnimGraphSharedBoneWeights& ref_bw) {
      for (const auto& rootBone : m_RootBones)
      {
        int iRootBone = -1;
        for (int iBone = 0; iBone < pOzzSkeleton->num_joints(); ++iBone)
        {
          if (wdStringUtils::IsEqual(pOzzSkeleton->joint_names()[iBone], rootBone.GetData()))
          {
            iRootBone = iBone;
            break;
          }
        }

        const float fBoneWeight = 1.0f;

        auto setBoneWeight = [&](int iCurrentBone, int) {
          const int iJointIdx0 = iCurrentBone / 4;
          const int iJointIdx1 = iCurrentBone % 4;

          ozz::math::SimdFloat4& soa_weight = ref_bw.m_Weights[iJointIdx0];
          soa_weight = ozz::math::SetI(soa_weight, ozz::math::simd_float4::Load1(fBoneWeight), iJointIdx1);
        };

        ozz::animation::IterateJointsDF(*pOzzSkeleton, setBoneWeight, iRootBone);
      }
    });

    if (m_InverseWeightsPin.IsConnected())
    {
      name.Append("-inv");

      m_pSharedInverseBoneWeights = graph.CreateBoneWeights(name, *pSkeleton, [this](wdAnimGraphSharedBoneWeights& ref_bw) {
        const ozz::math::SimdFloat4 oneBone = ozz::math::simd_float4::one();

        for (wdUInt32 b = 0; b < ref_bw.m_Weights.GetCount(); ++b)
        {
          ref_bw.m_Weights[b] = ozz::math::MSub(oneBone, oneBone, m_pSharedBoneWeights->m_Weights[b]);
        }
      });
    }

    if (!m_WeightsPin.IsConnected())
    {
      m_pSharedBoneWeights.Clear();
    }
  }

  if (m_WeightsPin.IsConnected())
  {
    wdAnimGraphPinDataBoneWeights* pPinData = graph.AddPinDataBoneWeights();
    pPinData->m_fOverallWeight = m_fWeight;
    pPinData->m_pSharedBoneWeights = m_pSharedBoneWeights.Borrow();

    m_WeightsPin.SetWeights(graph, pPinData);
  }

  if (m_InverseWeightsPin.IsConnected())
  {
    wdAnimGraphPinDataBoneWeights* pPinData = graph.AddPinDataBoneWeights();
    pPinData->m_fOverallWeight = m_fWeight;
    pPinData->m_pSharedBoneWeights = m_pSharedInverseBoneWeights.Borrow();

    m_InverseWeightsPin.SetWeights(graph, pPinData);
  }
}


WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_BoneWeightsAnimNode);
