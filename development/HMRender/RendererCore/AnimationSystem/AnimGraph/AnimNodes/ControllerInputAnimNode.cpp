#include <RendererCore/RendererCorePCH.h>

#include <Core/Input/InputManager.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/ControllerInputAnimNode.h>

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdControllerInputAnimNode, 1, wdRTTIDefaultAllocator<wdControllerInputAnimNode>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("LeftTrigger", m_LeftTrigger)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("LeftShoulder", m_LeftShoulder)->AddAttributes(new wdHiddenAttribute()),

    WD_MEMBER_PROPERTY("LeftStickX", m_LeftStickX)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("LeftStickY", m_LeftStickY)->AddAttributes(new wdHiddenAttribute()),

    WD_MEMBER_PROPERTY("PadLeft", m_PadLeft)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("PadRight", m_PadRight)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("PadUp", m_PadUp)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("PadDown", m_PadDown)->AddAttributes(new wdHiddenAttribute()),

    WD_MEMBER_PROPERTY("RightTrigger", m_RightTrigger)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("RightShoulder", m_RightShoulder)->AddAttributes(new wdHiddenAttribute()),

    WD_MEMBER_PROPERTY("RightStickX", m_RightStickX)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("RightStickY", m_RightStickY)->AddAttributes(new wdHiddenAttribute()),

    WD_MEMBER_PROPERTY("ButtonA", m_ButtonA)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("ButtonB", m_ButtonB)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("ButtonX", m_ButtonX)->AddAttributes(new wdHiddenAttribute()),
    WD_MEMBER_PROPERTY("ButtonY", m_ButtonY)->AddAttributes(new wdHiddenAttribute()),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Input"),
    new wdColorAttribute(wdColorScheme::DarkUI(wdColorScheme::Pink)),
    new wdTitleAttribute("XBox Controller"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdResult wdControllerInputAnimNode::SerializeNode(wdStreamWriter& stream) const
{
  stream.WriteVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  WD_SUCCEED_OR_RETURN(m_ButtonA.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_ButtonB.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_ButtonX.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_ButtonY.Serialize(stream));

  WD_SUCCEED_OR_RETURN(m_LeftStickX.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_LeftStickY.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_RightStickX.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_RightStickY.Serialize(stream));

  WD_SUCCEED_OR_RETURN(m_LeftTrigger.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_RightTrigger.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_LeftShoulder.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_RightShoulder.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_PadLeft.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_PadRight.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_PadUp.Serialize(stream));
  WD_SUCCEED_OR_RETURN(m_PadDown.Serialize(stream));

  return WD_SUCCESS;
}

wdResult wdControllerInputAnimNode::DeserializeNode(wdStreamReader& stream)
{
  stream.ReadVersion(1);

  WD_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  WD_SUCCEED_OR_RETURN(m_ButtonA.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_ButtonB.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_ButtonX.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_ButtonY.Deserialize(stream));

  WD_SUCCEED_OR_RETURN(m_LeftStickX.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_LeftStickY.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_RightStickX.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_RightStickY.Deserialize(stream));

  WD_SUCCEED_OR_RETURN(m_LeftTrigger.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_RightTrigger.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_LeftShoulder.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_RightShoulder.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_PadLeft.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_PadRight.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_PadUp.Deserialize(stream));
  WD_SUCCEED_OR_RETURN(m_PadDown.Deserialize(stream));

  return WD_SUCCESS;
}

void wdControllerInputAnimNode::Step(wdAnimGraph& graph, wdTime tDiff, const wdSkeletonResource* pSkeleton, wdGameObject* pTarget)
{
  float fValue1 = 0.0f;
  float fValue2 = 0.0f;

  wdInputManager::GetInputSlotState(wdInputSlot_Controller0_LeftStick_NegX, &fValue1);
  wdInputManager::GetInputSlotState(wdInputSlot_Controller0_LeftStick_PosX, &fValue2);
  m_LeftStickX.SetNumber(graph, -fValue1 + fValue2);

  wdInputManager::GetInputSlotState(wdInputSlot_Controller0_LeftStick_NegY, &fValue1);
  wdInputManager::GetInputSlotState(wdInputSlot_Controller0_LeftStick_PosY, &fValue2);
  m_LeftStickY.SetNumber(graph, -fValue1 + fValue2);

  wdInputManager::GetInputSlotState(wdInputSlot_Controller0_RightStick_NegX, &fValue1);
  wdInputManager::GetInputSlotState(wdInputSlot_Controller0_RightStick_PosX, &fValue2);
  m_RightStickX.SetNumber(graph, -fValue1 + fValue2);

  wdInputManager::GetInputSlotState(wdInputSlot_Controller0_RightStick_NegY, &fValue1);
  wdInputManager::GetInputSlotState(wdInputSlot_Controller0_RightStick_PosY, &fValue2);
  m_RightStickY.SetNumber(graph, -fValue1 + fValue2);

  wdInputManager::GetInputSlotState(wdInputSlot_Controller0_ButtonA, &fValue1);
  m_ButtonA.SetTriggered(graph, fValue1 > 0);

  wdInputManager::GetInputSlotState(wdInputSlot_Controller0_ButtonB, &fValue1);
  m_ButtonB.SetTriggered(graph, fValue1 > 0);

  wdInputManager::GetInputSlotState(wdInputSlot_Controller0_ButtonX, &fValue1);
  m_ButtonX.SetTriggered(graph, fValue1 > 0);

  wdInputManager::GetInputSlotState(wdInputSlot_Controller0_ButtonY, &fValue1);
  m_ButtonY.SetTriggered(graph, fValue1 > 0);

  wdInputManager::GetInputSlotState(wdInputSlot_Controller0_LeftShoulder, &fValue1);
  m_LeftShoulder.SetTriggered(graph, fValue1 > 0);
  wdInputManager::GetInputSlotState(wdInputSlot_Controller0_LeftTrigger, &fValue1);
  m_LeftTrigger.SetNumber(graph, fValue1);

  wdInputManager::GetInputSlotState(wdInputSlot_Controller0_RightShoulder, &fValue1);
  m_RightShoulder.SetTriggered(graph, fValue1 > 0);
  wdInputManager::GetInputSlotState(wdInputSlot_Controller0_RightTrigger, &fValue1);
  m_RightTrigger.SetNumber(graph, fValue1);

  wdInputManager::GetInputSlotState(wdInputSlot_Controller0_PadLeft, &fValue1);
  m_PadLeft.SetTriggered(graph, fValue1 > 0);
  wdInputManager::GetInputSlotState(wdInputSlot_Controller0_PadRight, &fValue1);
  m_PadRight.SetTriggered(graph, fValue1 > 0);
  wdInputManager::GetInputSlotState(wdInputSlot_Controller0_PadUp, &fValue1);
  m_PadUp.SetTriggered(graph, fValue1 > 0);
  wdInputManager::GetInputSlotState(wdInputSlot_Controller0_PadDown, &fValue1);
  m_PadDown.SetTriggered(graph, fValue1 > 0);
}


WD_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_ControllerInputAnimNode);
