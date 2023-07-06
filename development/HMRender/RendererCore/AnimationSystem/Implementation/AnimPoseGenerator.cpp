#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/World/GameObject.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/span.h>

void wdAnimPoseGenerator::Reset(const wdSkeletonResource* pSkeleton)
{
  m_pSkeleton = pSkeleton;
  m_LocalPoseCounter = 0;
  m_ModelPoseCounter = 0;

  m_CommandsSampleTrack.Clear();
  m_CommandsCombinePoses.Clear();
  m_CommandsLocalToModelPose.Clear();
  m_CommandsModelPoseToOutput.Clear();

  m_UsedLocalTransforms.Clear();

  m_OutputPose.Clear();

  // don't clear these arrays, they are reused
  //m_UsedModelTransforms.Clear();
  //m_SamplingCaches.Clear();
}

static WD_ALWAYS_INLINE wdAnimPoseGeneratorCommandID CreateCommandID(wdAnimPoseGeneratorCommandType type, wdUInt32 uiIndex)
{
  return (static_cast<wdUInt32>(type) << 24u) | uiIndex;
}

static WD_ALWAYS_INLINE wdUInt32 GetCommandIndex(wdAnimPoseGeneratorCommandID id)
{
  return static_cast<wdUInt32>(id) & 0x00FFFFFFu;
}

static WD_ALWAYS_INLINE wdAnimPoseGeneratorCommandType GetCommandType(wdAnimPoseGeneratorCommandID id)
{
  return static_cast<wdAnimPoseGeneratorCommandType>(static_cast<wdUInt32>(id) >> 24u);
}

wdAnimPoseGeneratorCommandSampleTrack& wdAnimPoseGenerator::AllocCommandSampleTrack(wdUInt32 uiDeterministicID)
{
  auto& cmd = m_CommandsSampleTrack.ExpandAndGetRef();
  cmd.m_Type = wdAnimPoseGeneratorCommandType::SampleTrack;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsSampleTrack.GetCount() - 1);
  cmd.m_LocalPoseOutput = m_LocalPoseCounter++;
  cmd.m_uiUniqueID = uiDeterministicID;

  return cmd;
}

wdAnimPoseGeneratorCommandCombinePoses& wdAnimPoseGenerator::AllocCommandCombinePoses()
{
  auto& cmd = m_CommandsCombinePoses.ExpandAndGetRef();
  cmd.m_Type = wdAnimPoseGeneratorCommandType::CombinePoses;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsCombinePoses.GetCount() - 1);
  cmd.m_LocalPoseOutput = m_LocalPoseCounter++;

  return cmd;
}

wdAnimPoseGeneratorCommandLocalToModelPose& wdAnimPoseGenerator::AllocCommandLocalToModelPose()
{
  auto& cmd = m_CommandsLocalToModelPose.ExpandAndGetRef();
  cmd.m_Type = wdAnimPoseGeneratorCommandType::LocalToModelPose;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsLocalToModelPose.GetCount() - 1);
  cmd.m_ModelPoseOutput = m_ModelPoseCounter++;

  return cmd;
}

wdAnimPoseGeneratorCommandModelPoseToOutput& wdAnimPoseGenerator::AllocCommandModelPoseToOutput()
{
  auto& cmd = m_CommandsModelPoseToOutput.ExpandAndGetRef();
  cmd.m_Type = wdAnimPoseGeneratorCommandType::ModelPoseToOutput;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsModelPoseToOutput.GetCount() - 1);

  return cmd;
}

wdAnimPoseGeneratorCommandSampleEventTrack& wdAnimPoseGenerator::AllocCommandSampleEventTrack()
{
  auto& cmd = m_CommandsSampleEventTrack.ExpandAndGetRef();
  cmd.m_Type = wdAnimPoseGeneratorCommandType::SampleEventTrack;
  cmd.m_CommandID = CreateCommandID(cmd.m_Type, m_CommandsSampleEventTrack.GetCount() - 1);

  return cmd;
}

wdAnimPoseGenerator::wdAnimPoseGenerator() = default;

wdAnimPoseGenerator::~wdAnimPoseGenerator()
{
  for (wdUInt32 i = 0; i < m_SamplingCaches.GetCount(); ++i)
  {
    WD_DEFAULT_DELETE(m_SamplingCaches.GetValue(i));
  }
  m_SamplingCaches.Clear();
}

void wdAnimPoseGenerator::Validate() const
{
  WD_ASSERT_DEV(m_CommandsModelPoseToOutput.GetCount() <= 1, "Only one output node may exist");

  for (auto& cmd : m_CommandsSampleTrack)
  {
    WD_ASSERT_DEV(cmd.m_hAnimationClip.IsValid(), "Invalid animation clips are not allowed.");
    //WD_ASSERT_DEV(cmd.m_Inputs.IsEmpty(), "Track samplers can't have inputs.");
    WD_ASSERT_DEV(cmd.m_LocalPoseOutput != wdInvalidIndex, "Output pose not allocated.");
  }

  for (auto& cmd : m_CommandsCombinePoses)
  {
    //WD_ASSERT_DEV(cmd.m_Inputs.GetCount() >= 1, "Must combine at least one pose.");
    WD_ASSERT_DEV(cmd.m_LocalPoseOutput != wdInvalidIndex, "Output pose not allocated.");
    WD_ASSERT_DEV(cmd.m_Inputs.GetCount() == cmd.m_InputWeights.GetCount(), "Number of inputs and weights must match.");

    for (auto id : cmd.m_Inputs)
    {
      auto type = GetCommand(id).GetType();
      WD_ASSERT_DEV(type == wdAnimPoseGeneratorCommandType::SampleTrack || type == wdAnimPoseGeneratorCommandType::CombinePoses, "Unsupported input type");
    }
  }

  for (auto& cmd : m_CommandsLocalToModelPose)
  {
    WD_ASSERT_DEV(cmd.m_Inputs.GetCount() == 1, "Exactly one input must be provided.");
    WD_ASSERT_DEV(cmd.m_ModelPoseOutput != wdInvalidIndex, "Output pose not allocated.");

    for (auto id : cmd.m_Inputs)
    {
      auto type = GetCommand(id).GetType();
      WD_ASSERT_DEV(type == wdAnimPoseGeneratorCommandType::SampleTrack || type == wdAnimPoseGeneratorCommandType::CombinePoses, "Unsupported input type");
    }
  }

  for (auto& cmd : m_CommandsModelPoseToOutput)
  {
    WD_ASSERT_DEV(cmd.m_Inputs.GetCount() == 1, "Exactly one input must be provided.");

    for (auto id : cmd.m_Inputs)
    {
      auto type = GetCommand(id).GetType();
      WD_ASSERT_DEV(type == wdAnimPoseGeneratorCommandType::LocalToModelPose, "Unsupported input type");
    }
  }

  for (auto& cmd : m_CommandsSampleEventTrack)
  {
    WD_ASSERT_DEV(cmd.m_hAnimationClip.IsValid(), "Invalid animation clips are not allowed.");
  }
}

const wdAnimPoseGeneratorCommand& wdAnimPoseGenerator::GetCommand(wdAnimPoseGeneratorCommandID id) const
{
  return const_cast<wdAnimPoseGenerator*>(this)->GetCommand(id);
}

wdAnimPoseGeneratorCommand& wdAnimPoseGenerator::GetCommand(wdAnimPoseGeneratorCommandID id)
{
  WD_ASSERT_DEV(id != wdInvalidIndex, "Invalid command ID");

  switch (GetCommandType(id))
  {
    case wdAnimPoseGeneratorCommandType::SampleTrack:
      return m_CommandsSampleTrack[GetCommandIndex(id)];

    case wdAnimPoseGeneratorCommandType::CombinePoses:
      return m_CommandsCombinePoses[GetCommandIndex(id)];

    case wdAnimPoseGeneratorCommandType::LocalToModelPose:
      return m_CommandsLocalToModelPose[GetCommandIndex(id)];

    case wdAnimPoseGeneratorCommandType::ModelPoseToOutput:
      return m_CommandsModelPoseToOutput[GetCommandIndex(id)];

    case wdAnimPoseGeneratorCommandType::SampleEventTrack:
      return m_CommandsSampleEventTrack[GetCommandIndex(id)];

      WD_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  WD_REPORT_FAILURE("Invalid command ID");
  return m_CommandsSampleTrack[0];
}

wdArrayPtr<wdMat4> wdAnimPoseGenerator::GeneratePose(const wdGameObject* pSendAnimationEventsTo /*= nullptr*/)
{
  Validate();

  for (auto& cmd : m_CommandsModelPoseToOutput)
  {
    Execute(cmd, pSendAnimationEventsTo);
  }

  auto pPose = m_OutputPose;

  // TODO: clear temp data

  return pPose;
}

void wdAnimPoseGenerator::Execute(wdAnimPoseGeneratorCommand& cmd, const wdGameObject* pSendAnimationEventsTo)
{
  if (cmd.m_bExecuted)
    return;

  // TODO: validate for circular dependencies
  cmd.m_bExecuted = true;

  for (auto id : cmd.m_Inputs)
  {
    Execute(GetCommand(id), pSendAnimationEventsTo);
  }

  // TODO: build a task graph and execute multi-threaded

  switch (cmd.GetType())
  {
    case wdAnimPoseGeneratorCommandType::SampleTrack:
      ExecuteCmd(static_cast<wdAnimPoseGeneratorCommandSampleTrack&>(cmd), pSendAnimationEventsTo);
      break;

    case wdAnimPoseGeneratorCommandType::CombinePoses:
      ExecuteCmd(static_cast<wdAnimPoseGeneratorCommandCombinePoses&>(cmd));
      break;

    case wdAnimPoseGeneratorCommandType::LocalToModelPose:
      ExecuteCmd(static_cast<wdAnimPoseGeneratorCommandLocalToModelPose&>(cmd));
      break;

    case wdAnimPoseGeneratorCommandType::ModelPoseToOutput:
      ExecuteCmd(static_cast<wdAnimPoseGeneratorCommandModelPoseToOutput&>(cmd));
      break;

    case wdAnimPoseGeneratorCommandType::SampleEventTrack:
      ExecuteCmd(static_cast<wdAnimPoseGeneratorCommandSampleEventTrack&>(cmd), pSendAnimationEventsTo);
      break;

      WD_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

void wdAnimPoseGenerator::ExecuteCmd(wdAnimPoseGeneratorCommandSampleTrack& cmd, const wdGameObject* pSendAnimationEventsTo)
{
  wdResourceLock<wdAnimationClipResource> pResource(cmd.m_hAnimationClip, wdResourceAcquireMode::BlockTillLoaded);

  const ozz::animation::Animation& ozzAnim = pResource->GetDescriptor().GetMappedOzzAnimation(*m_pSkeleton);

  cmd.m_bAdditive = pResource->GetDescriptor().m_bAdditive;

  auto transforms = AcquireLocalPoseTransforms(cmd.m_LocalPoseOutput);

  auto& pSampler = m_SamplingCaches[cmd.m_uiUniqueID];

  if (pSampler == nullptr)
  {
    pSampler = WD_DEFAULT_NEW(ozz::animation::SamplingJob::Context);
  }

  if (pSampler->max_tracks() != ozzAnim.num_tracks())
  {
    pSampler->Resize(ozzAnim.num_tracks());
  }

  ozz::animation::SamplingJob job;
  job.animation = &ozzAnim;
  job.context = pSampler;
  job.ratio = cmd.m_fNormalizedSamplePos;
  job.output = ozz::span<ozz::math::SoaTransform>(transforms.GetPtr(), transforms.GetCount());

  if (!job.Validate())
    return;

  WD_ASSERT_DEBUG(job.Validate(), "");
  job.Run();

  SampleEventTrack(pResource.GetPointer(), cmd.m_EventSampling, pSendAnimationEventsTo, cmd.m_fPreviousNormalizedSamplePos, cmd.m_fNormalizedSamplePos);
}

void wdAnimPoseGenerator::ExecuteCmd(wdAnimPoseGeneratorCommandCombinePoses& cmd)
{
  auto transforms = AcquireLocalPoseTransforms(cmd.m_LocalPoseOutput);

  wdHybridArray<ozz::animation::BlendingJob::Layer, 8> bl;
  wdHybridArray<ozz::animation::BlendingJob::Layer, 8> blAdd;

  for (wdUInt32 i = 0; i < cmd.m_Inputs.GetCount(); ++i)
  {
    const auto& cmdIn = GetCommand(cmd.m_Inputs[i]);

    if (cmdIn.GetType() == wdAnimPoseGeneratorCommandType::SampleEventTrack)
      continue;

    ozz::animation::BlendingJob::Layer* layer = nullptr;

    switch (cmdIn.GetType())
    {
      case wdAnimPoseGeneratorCommandType::SampleTrack:
      {
        if (static_cast<const wdAnimPoseGeneratorCommandSampleTrack&>(cmdIn).m_bAdditive)
        {
          layer = &blAdd.ExpandAndGetRef();
        }
        else
        {
          layer = &bl.ExpandAndGetRef();
        }

        auto transform = AcquireLocalPoseTransforms(static_cast<const wdAnimPoseGeneratorCommandSampleTrack&>(cmdIn).m_LocalPoseOutput);
        layer->transform = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
      }
      break;

      case wdAnimPoseGeneratorCommandType::CombinePoses:
      {
        layer = &bl.ExpandAndGetRef();

        auto transform = AcquireLocalPoseTransforms(static_cast<const wdAnimPoseGeneratorCommandCombinePoses&>(cmdIn).m_LocalPoseOutput);
        layer->transform = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
      }
      break;

        WD_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    layer->weight = cmd.m_InputWeights[i];

    if (cmd.m_InputBoneWeights.GetCount() > i && !cmd.m_InputBoneWeights[i].IsEmpty())
    {
      layer->joint_weights = ozz::span(cmd.m_InputBoneWeights[i].GetPtr(), cmd.m_InputBoneWeights[i].GetEndPtr());
    }
  }

  ozz::animation::BlendingJob job;
  job.threshold = 1.0f;
  job.layers = ozz::span<const ozz::animation::BlendingJob::Layer>(begin(bl), end(bl));
  job.additive_layers = ozz::span<const ozz::animation::BlendingJob::Layer>(begin(blAdd), end(blAdd));
  job.rest_pose = m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().joint_rest_poses();
  job.output = ozz::span<ozz::math::SoaTransform>(transforms.GetPtr(), transforms.GetCount());
  WD_ASSERT_DEBUG(job.Validate(), "");
  job.Run();
}

void wdAnimPoseGenerator::ExecuteCmd(wdAnimPoseGeneratorCommandLocalToModelPose& cmd)
{
  ozz::animation::LocalToModelJob job;

  const auto& cmdIn = GetCommand(cmd.m_Inputs[0]);

  switch (cmdIn.GetType())
  {
    case wdAnimPoseGeneratorCommandType::SampleTrack:
    {
      auto transform = AcquireLocalPoseTransforms(static_cast<const wdAnimPoseGeneratorCommandSampleTrack&>(cmdIn).m_LocalPoseOutput);
      job.input = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
    }
    break;

    case wdAnimPoseGeneratorCommandType::CombinePoses:
    {
      auto transform = AcquireLocalPoseTransforms(static_cast<const wdAnimPoseGeneratorCommandCombinePoses&>(cmdIn).m_LocalPoseOutput);
      job.input = ozz::span<const ozz::math::SoaTransform>(transform.GetPtr(), transform.GetCount());
    }
    break;

      WD_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (cmd.m_pSendLocalPoseMsgTo)
  {
    wdMsgAnimationPosePreparing msg;
    msg.m_pSkeleton = &m_pSkeleton->GetDescriptor().m_Skeleton;
    msg.m_LocalTransforms = wdMakeArrayPtr(const_cast<ozz::math::SoaTransform*>(job.input.data()), (wdUInt32)job.input.size());

    cmd.m_pSendLocalPoseMsgTo->SendMessageRecursive(msg);
  }

  auto transforms = AcquireModelPoseTransforms(cmd.m_ModelPoseOutput);

  job.output = ozz::span<ozz::math::Float4x4>(reinterpret_cast<ozz::math::Float4x4*>(transforms.GetPtr()), transforms.GetCount());
  job.skeleton = &m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();
  WD_ASSERT_DEBUG(job.Validate(), "");
  job.Run();
}

void wdAnimPoseGenerator::ExecuteCmd(wdAnimPoseGeneratorCommandModelPoseToOutput& cmd)
{
  const auto& cmdIn = GetCommand(cmd.m_Inputs[0]);

  switch (cmdIn.GetType())
  {
    case wdAnimPoseGeneratorCommandType::LocalToModelPose:
      m_OutputPose = AcquireModelPoseTransforms(static_cast<const wdAnimPoseGeneratorCommandLocalToModelPose&>(cmdIn).m_ModelPoseOutput);
      break;

      WD_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

void wdAnimPoseGenerator::ExecuteCmd(wdAnimPoseGeneratorCommandSampleEventTrack& cmd, const wdGameObject* pSendAnimationEventsTo)
{
  wdResourceLock<wdAnimationClipResource> pResource(cmd.m_hAnimationClip, wdResourceAcquireMode::BlockTillLoaded);

  SampleEventTrack(pResource.GetPointer(), cmd.m_EventSampling, pSendAnimationEventsTo, cmd.m_fPreviousNormalizedSamplePos, cmd.m_fNormalizedSamplePos);
}

void wdAnimPoseGenerator::SampleEventTrack(const wdAnimationClipResource* pResource, wdAnimPoseEventTrackSampleMode mode, const wdGameObject* pSendAnimationEventsTo, float fPrevPos, float fCurPos)
{
  const auto& et = pResource->GetDescriptor().m_EventTrack;

  if (mode == wdAnimPoseEventTrackSampleMode::None || et.IsEmpty())
    return;

  const wdTime duration = pResource->GetDescriptor().GetDuration();

  const wdTime tPrev = fPrevPos * duration;
  const wdTime tNow = fCurPos * duration;
  const wdTime tStart = wdTime::Zero();
  const wdTime tEnd = duration + wdTime::Seconds(1.0); // sampling position is EXCLUSIVE

  wdHybridArray<wdHashedString, 16> events;

  switch (mode)
  {
    case wdAnimPoseEventTrackSampleMode::OnlyBetween:
      et.Sample(tPrev, tNow, events);
      break;

    case wdAnimPoseEventTrackSampleMode::LoopAtEnd:
      et.Sample(tPrev, tEnd, events);
      et.Sample(tStart, tNow, events);
      break;

    case wdAnimPoseEventTrackSampleMode::LoopAtStart:
      et.Sample(tPrev, tStart, events);
      et.Sample(tStart, tNow, events);
      break;

    case wdAnimPoseEventTrackSampleMode::BounceAtEnd:
      et.Sample(tPrev, tEnd, events);
      et.Sample(tEnd, tNow, events);
      break;

    case wdAnimPoseEventTrackSampleMode::BounceAtStart:
      et.Sample(tPrev, tStart, events);
      et.Sample(tStart, tNow, events);
      break;

      WD_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  wdMsgGenericEvent msg;

  for (const auto& hs : events)
  {
    msg.m_sMessage = hs;

    pSendAnimationEventsTo->SendEventMessage(msg, nullptr);
  }
}

wdArrayPtr<ozz::math::SoaTransform> wdAnimPoseGenerator::AcquireLocalPoseTransforms(wdAnimPoseGeneratorLocalPoseID id)
{
  m_UsedLocalTransforms.EnsureCount(id + 1);

  if (m_UsedLocalTransforms[id].IsEmpty())
  {
    using T = ozz::math::SoaTransform;
    const wdUInt32 num = m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().num_soa_joints();
    m_UsedLocalTransforms[id] = WD_NEW_ARRAY(wdFrameAllocator::GetCurrentAllocator(), T, num);
  }

  return m_UsedLocalTransforms[id];
}

wdArrayPtr<wdMat4> wdAnimPoseGenerator::AcquireModelPoseTransforms(wdAnimPoseGeneratorModelPoseID id)
{
  m_UsedModelTransforms.EnsureCount(id + 1);

  m_UsedModelTransforms[id].SetCountUninitialized(m_pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton().num_joints());

  return m_UsedModelTransforms[id];
}


WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_AnimPoseGenerator);
