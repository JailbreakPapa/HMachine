#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>

#include <Foundation/Types/RefCounted.h>
#include <ozz/base/maths/soa_transform.h>

class wdAnimGraph;
class wdStreamWriter;
class wdStreamReader;
struct wdAnimGraphPinDataBoneWeights;
struct wdAnimGraphPinDataLocalTransforms;
struct wdAnimGraphPinDataModelTransforms;

struct wdAnimGraphSharedBoneWeights : public wdRefCounted
{
  wdDynamicArray<ozz::math::SimdFloat4, wdAlignedAllocatorWrapper> m_Weights;
};

using wdAnimPoseGeneratorLocalPoseID = wdUInt32;
using wdAnimPoseGeneratorModelPoseID = wdUInt32;
using wdAnimPoseGeneratorCommandID = wdUInt32;

class WD_RENDERERCORE_DLL wdAnimGraphPin : public wdReflectedClass
{
  WD_ADD_DYNAMIC_REFLECTION(wdAnimGraphPin, wdReflectedClass);

public:
  enum Type : wdUInt8
  {
    Invalid,
    Trigger,
    Number,
    BoneWeights,
    LocalPose,
    ModelPose,
    // EXTEND THIS if a new type is introduced

    ENUM_COUNT
  };

  bool IsConnected() const
  {
    return m_iPinIndex != -1;
  }

  wdResult Serialize(wdStreamWriter& inout_stream) const;
  wdResult Deserialize(wdStreamReader& inout_stream);

protected:
  wdInt16 m_iPinIndex = -1;
  wdUInt8 m_uiNumConnections = 0;
};

class WD_RENDERERCORE_DLL wdAnimGraphInputPin : public wdAnimGraphPin
{
  WD_ADD_DYNAMIC_REFLECTION(wdAnimGraphInputPin, wdAnimGraphPin);

public:
};

class WD_RENDERERCORE_DLL wdAnimGraphOutputPin : public wdAnimGraphPin
{
  WD_ADD_DYNAMIC_REFLECTION(wdAnimGraphOutputPin, wdAnimGraphPin);

public:
};

//////////////////////////////////////////////////////////////////////////

class WD_RENDERERCORE_DLL wdAnimGraphTriggerInputPin : public wdAnimGraphInputPin
{
  WD_ADD_DYNAMIC_REFLECTION(wdAnimGraphTriggerInputPin, wdAnimGraphInputPin);

public:
  bool IsTriggered(wdAnimGraph& ref_graph) const;
  bool AreAllTriggered(wdAnimGraph& ref_graph) const;
};

class WD_RENDERERCORE_DLL wdAnimGraphTriggerOutputPin : public wdAnimGraphOutputPin
{
  WD_ADD_DYNAMIC_REFLECTION(wdAnimGraphTriggerOutputPin, wdAnimGraphOutputPin);

public:
  /// \brief Sets this output pin to the triggered or untriggered state for this frame.
  ///
  /// All pin states are reset before every graph update, so this only needs to be called
  /// when a pin should be set to the triggered state, but then it must be called every frame.
  void SetTriggered(wdAnimGraph& ref_graph, bool bTriggered);
};

//////////////////////////////////////////////////////////////////////////

class WD_RENDERERCORE_DLL wdAnimGraphNumberInputPin : public wdAnimGraphInputPin
{
  WD_ADD_DYNAMIC_REFLECTION(wdAnimGraphNumberInputPin, wdAnimGraphInputPin);

public:
  double GetNumber(wdAnimGraph& ref_graph, double fFallback = 0.0) const;
};

class WD_RENDERERCORE_DLL wdAnimGraphNumberOutputPin : public wdAnimGraphOutputPin
{
  WD_ADD_DYNAMIC_REFLECTION(wdAnimGraphNumberOutputPin, wdAnimGraphOutputPin);

public:
  void SetNumber(wdAnimGraph& ref_graph, double value);
};

//////////////////////////////////////////////////////////////////////////

class WD_RENDERERCORE_DLL wdAnimGraphBoneWeightsInputPin : public wdAnimGraphInputPin
{
  WD_ADD_DYNAMIC_REFLECTION(wdAnimGraphBoneWeightsInputPin, wdAnimGraphInputPin);

public:
  wdAnimGraphPinDataBoneWeights* GetWeights(wdAnimGraph& ref_graph) const;
};

class WD_RENDERERCORE_DLL wdAnimGraphBoneWeightsOutputPin : public wdAnimGraphOutputPin
{
  WD_ADD_DYNAMIC_REFLECTION(wdAnimGraphBoneWeightsOutputPin, wdAnimGraphOutputPin);

public:
  void SetWeights(wdAnimGraph& ref_graph, wdAnimGraphPinDataBoneWeights* pWeights);
};

//////////////////////////////////////////////////////////////////////////

class WD_RENDERERCORE_DLL wdAnimGraphLocalPoseInputPin : public wdAnimGraphInputPin
{
  WD_ADD_DYNAMIC_REFLECTION(wdAnimGraphLocalPoseInputPin, wdAnimGraphInputPin);

public:
  wdAnimGraphPinDataLocalTransforms* GetPose(wdAnimGraph& ref_graph) const;
};

class WD_RENDERERCORE_DLL wdAnimGraphLocalPoseMultiInputPin : public wdAnimGraphInputPin
{
  WD_ADD_DYNAMIC_REFLECTION(wdAnimGraphLocalPoseMultiInputPin, wdAnimGraphInputPin);

public:
  void GetPoses(wdAnimGraph& ref_graph, wdDynamicArray<wdAnimGraphPinDataLocalTransforms*>& out_poses) const;
};

class WD_RENDERERCORE_DLL wdAnimGraphLocalPoseOutputPin : public wdAnimGraphOutputPin
{
  WD_ADD_DYNAMIC_REFLECTION(wdAnimGraphLocalPoseOutputPin, wdAnimGraphOutputPin);

public:
  void SetPose(wdAnimGraph& ref_graph, wdAnimGraphPinDataLocalTransforms* pPose);
};

//////////////////////////////////////////////////////////////////////////

class WD_RENDERERCORE_DLL wdAnimGraphModelPoseInputPin : public wdAnimGraphInputPin
{
  WD_ADD_DYNAMIC_REFLECTION(wdAnimGraphModelPoseInputPin, wdAnimGraphInputPin);

public:
  wdAnimGraphPinDataModelTransforms* GetPose(wdAnimGraph& ref_graph) const;
};

class WD_RENDERERCORE_DLL wdAnimGraphModelPoseOutputPin : public wdAnimGraphOutputPin
{
  WD_ADD_DYNAMIC_REFLECTION(wdAnimGraphModelPoseOutputPin, wdAnimGraphOutputPin);

public:
  void SetPose(wdAnimGraph& ref_graph, wdAnimGraphPinDataModelTransforms* pPose);
};
