#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/RendererCoreDLL.h>

#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/maths/soa_float.h>
#include <ozz/base/maths/soa_transform.h>

WD_DEFINE_AS_POD_TYPE(ozz::math::SoaTransform);

class wdSkeletonResource;
class wdAnimPoseGenerator;
class wdGameObject;

using wdAnimationClipResourceHandle = wdTypedResourceHandle<class wdAnimationClipResource>;

using wdAnimPoseGeneratorLocalPoseID = wdUInt32;
using wdAnimPoseGeneratorModelPoseID = wdUInt32;
using wdAnimPoseGeneratorCommandID = wdUInt32;

/// \brief The type of wdAnimPoseGeneratorCommand
enum class wdAnimPoseGeneratorCommandType
{
  Invalid,
  SampleTrack,
  CombinePoses,
  LocalToModelPose,
  ModelPoseToOutput,
  SampleEventTrack,
};

enum class wdAnimPoseEventTrackSampleMode : wdUInt8
{
  None,         ///< Don't sample the event track at all
  OnlyBetween,  ///< Sample the event track only between PrevSamplePos and SamplePos
  LoopAtEnd,    ///< Sample the event track between PrevSamplePos and End, then Start and SamplePos
  LoopAtStart,  ///< Sample the event track between PrevSamplePos and Start, then End and SamplePos
  BounceAtEnd,  ///< Sample the event track between PrevSamplePos and End, then End and SamplePos
  BounceAtStart ///< Sample the event track between PrevSamplePos and Start, then Start and SamplePos
};

/// \brief Base class for all pose generator commands
///
/// All commands have a unique command ID with which they are referenced.
/// All commands can have zero or N other commands set as *inputs*.
/// Every type of command only accepts certain types and amount of inputs.
///
/// The pose generation graph is built by allocating commands on the graph and then setting up
/// which command is an input to which other node.
/// A command can be an input to multiple other commands. It will be evaluated only once.
struct WD_RENDERERCORE_DLL wdAnimPoseGeneratorCommand
{
  wdHybridArray<wdAnimPoseGeneratorCommandID, 4> m_Inputs;

  wdAnimPoseGeneratorCommandID GetCommandID() const { return m_CommandID; }
  wdAnimPoseGeneratorCommandType GetType() const { return m_Type; }

private:
  friend class wdAnimPoseGenerator;

  bool m_bExecuted = false;
  wdAnimPoseGeneratorCommandID m_CommandID = wdInvalidIndex;
  wdAnimPoseGeneratorCommandType m_Type = wdAnimPoseGeneratorCommandType::Invalid;
};

/// \brief Samples an animation clip at a given time and optionally also its event track.
///
/// The command has to be added as an input to one of
/// * wdAnimPoseGeneratorCommandCombinePoses
/// * wdAnimPoseGeneratorCommandLocalToModelPose
///
/// If the event track shall be sampled as well, event messages are sent to the wdGameObject for which the pose is generated.
///
/// This command can optionally have input commands of type wdAnimPoseGeneratorCommandSampleEventTrack.
struct WD_RENDERERCORE_DLL wdAnimPoseGeneratorCommandSampleTrack final : public wdAnimPoseGeneratorCommand
{
  wdAnimationClipResourceHandle m_hAnimationClip;
  float m_fNormalizedSamplePos;
  float m_fPreviousNormalizedSamplePos;

  wdAnimPoseEventTrackSampleMode m_EventSampling = wdAnimPoseEventTrackSampleMode::None;

private:
  friend class wdAnimPoseGenerator;

  bool m_bAdditive = false;
  wdUInt32 m_uiUniqueID = 0;
  wdAnimPoseGeneratorLocalPoseID m_LocalPoseOutput = wdInvalidIndex;
};

/// \brief Combines all the local space poses that are given as input into one local pose.
///
/// The input commands must be of type
/// * wdAnimPoseGeneratorCommandSampleTrack
/// * wdAnimPoseGeneratorCommandCombinePoses
///
/// Every input pose gets both an overall weight, as well as optionally a per-bone weight mask.
/// If a per-bone mask is used, the respective input pose will only affect those bones.
struct WD_RENDERERCORE_DLL wdAnimPoseGeneratorCommandCombinePoses final : public wdAnimPoseGeneratorCommand
{
  wdHybridArray<float, 4> m_InputWeights;
  wdHybridArray<wdArrayPtr<const ozz::math::SimdFloat4>, 4> m_InputBoneWeights;

private:
  friend class wdAnimPoseGenerator;

  wdAnimPoseGeneratorLocalPoseID m_LocalPoseOutput = wdInvalidIndex;
};

/// \brief Accepts a single input in local space and converts it to model space.
///
/// The input command must be of type
/// * wdAnimPoseGeneratorCommandSampleTrack
/// * wdAnimPoseGeneratorCommandCombinePoses
struct WD_RENDERERCORE_DLL wdAnimPoseGeneratorCommandLocalToModelPose final : public wdAnimPoseGeneratorCommand
{
  wdGameObject* m_pSendLocalPoseMsgTo = nullptr;

private:
  friend class wdAnimPoseGenerator;

  wdAnimPoseGeneratorModelPoseID m_ModelPoseOutput = wdInvalidIndex;
};

/// \brief Accepts a single input command that outputs a model space pose and forwards it to the wdGameObject for which the pose is generated.
///
/// The input command must be of type
/// * wdAnimPoseGeneratorCommandLocalToModelPose
///
/// Every graph should have exactly one of these nodes. Commands that are not (indirectly) connected to an
/// output node will not be evaluated and won't have any effect.
struct WD_RENDERERCORE_DLL wdAnimPoseGeneratorCommandModelPoseToOutput final : public wdAnimPoseGeneratorCommand
{
};

/// \brief Samples the event track of an animation clip but doesn't generate an animation pose.
///
/// Commands of this type can be added as inputs to commands of type
/// * wdAnimPoseGeneratorCommandSampleTrack
/// * wdAnimPoseGeneratorCommandSampleEventTrack
///
/// They are used to sample event tracks only.
struct WD_RENDERERCORE_DLL wdAnimPoseGeneratorCommandSampleEventTrack final : public wdAnimPoseGeneratorCommand
{
  wdAnimationClipResourceHandle m_hAnimationClip;
  float m_fNormalizedSamplePos;
  float m_fPreviousNormalizedSamplePos;

  wdAnimPoseEventTrackSampleMode m_EventSampling = wdAnimPoseEventTrackSampleMode::None;

private:
  friend class wdAnimPoseGenerator;

  wdUInt32 m_uiUniqueID = 0;
};

class WD_RENDERERCORE_DLL wdAnimPoseGenerator final
{
public:
  wdAnimPoseGenerator();
  ~wdAnimPoseGenerator();

  void Reset(const wdSkeletonResource* pSkeleton);

  wdAnimPoseGeneratorCommandSampleTrack& AllocCommandSampleTrack(wdUInt32 uiDeterministicID);
  wdAnimPoseGeneratorCommandCombinePoses& AllocCommandCombinePoses();
  wdAnimPoseGeneratorCommandLocalToModelPose& AllocCommandLocalToModelPose();
  wdAnimPoseGeneratorCommandModelPoseToOutput& AllocCommandModelPoseToOutput();
  wdAnimPoseGeneratorCommandSampleEventTrack& AllocCommandSampleEventTrack();

  const wdAnimPoseGeneratorCommand& GetCommand(wdAnimPoseGeneratorCommandID id) const;
  wdAnimPoseGeneratorCommand& GetCommand(wdAnimPoseGeneratorCommandID id);

  wdArrayPtr<wdMat4> GeneratePose(const wdGameObject* pSendAnimationEventsTo);

private:
  void Validate() const;

  void Execute(wdAnimPoseGeneratorCommand& cmd, const wdGameObject* pSendAnimationEventsTo);
  void ExecuteCmd(wdAnimPoseGeneratorCommandSampleTrack& cmd, const wdGameObject* pSendAnimationEventsTo);
  void ExecuteCmd(wdAnimPoseGeneratorCommandCombinePoses& cmd);
  void ExecuteCmd(wdAnimPoseGeneratorCommandLocalToModelPose& cmd);
  void ExecuteCmd(wdAnimPoseGeneratorCommandModelPoseToOutput& cmd);
  void ExecuteCmd(wdAnimPoseGeneratorCommandSampleEventTrack& cmd, const wdGameObject* pSendAnimationEventsTo);
  void SampleEventTrack(const wdAnimationClipResource* pResource, wdAnimPoseEventTrackSampleMode mode, const wdGameObject* pSendAnimationEventsTo, float fPrevPos, float fCurPos);

  wdArrayPtr<ozz::math::SoaTransform> AcquireLocalPoseTransforms(wdAnimPoseGeneratorLocalPoseID id);
  wdArrayPtr<wdMat4> AcquireModelPoseTransforms(wdAnimPoseGeneratorModelPoseID id);

  const wdSkeletonResource* m_pSkeleton = nullptr;

  wdAnimPoseGeneratorLocalPoseID m_LocalPoseCounter = 0;
  wdAnimPoseGeneratorModelPoseID m_ModelPoseCounter = 0;

  wdArrayPtr<wdMat4> m_OutputPose;

  wdHybridArray<wdArrayPtr<ozz::math::SoaTransform>, 8> m_UsedLocalTransforms;
  wdHybridArray<wdDynamicArray<wdMat4, wdAlignedAllocatorWrapper>, 2> m_UsedModelTransforms;

  wdHybridArray<wdAnimPoseGeneratorCommandSampleTrack, 4> m_CommandsSampleTrack;
  wdHybridArray<wdAnimPoseGeneratorCommandCombinePoses, 1> m_CommandsCombinePoses;
  wdHybridArray<wdAnimPoseGeneratorCommandLocalToModelPose, 1> m_CommandsLocalToModelPose;
  wdHybridArray<wdAnimPoseGeneratorCommandModelPoseToOutput, 1> m_CommandsModelPoseToOutput;
  wdHybridArray<wdAnimPoseGeneratorCommandSampleEventTrack, 2> m_CommandsSampleEventTrack;

  wdArrayMap<wdUInt32, ozz::animation::SamplingJob::Context*> m_SamplingCaches;
};
