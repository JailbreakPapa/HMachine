#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Types/Bitflags.h>

/// \brief This struct defines the different states a key can be in.
///        All keys always go through the states 'Pressed' and 'Released', even if they are active for only one frame.
///        A key is 'Down' when it is pressed for at least two frames. It is 'Up' when it is not pressed for at least two frames.
struct WD_CORE_DLL wdKeyState
{
  enum Enum
  {
    Up,       ///< Key is not pressed at all.
    Released, ///< Key has just been released this frame.
    Pressed,  ///< Key has just been pressed down this frame.
    Down      ///< Key is pressed down for longer than one frame now.
  };

  /// \brief Computes the new key state from a previous key state and whether it is currently pressed or not.
  static wdKeyState::Enum GetNewKeyState(wdKeyState::Enum prevState, bool bKeyDown);
};

// clang-format off
// off for the entire file

/// \brief These flags are specified when registering an input slot (by a device), to define some capabilities and restrictions of the hardware.
///
/// By default you do not need to use these flags at all. However, when presenting the user with a list of 'possible' buttons to press to map to an
/// action, these flags can be used to filter out unwanted slots.
/// For example you can filter out mouse movements by requiring that the input slot must be pressable or may not represent any axis.
/// You an additionally also use the prefix of the input slot name, to filter out all touch input slots etc. if necessary.
struct wdInputSlotFlags
{
  using StorageType = wdUInt16;

  enum Enum
  {
    None                      = 0,

    ReportsRelativeValues     = WD_BIT(0),  ///< The input slot reports delta values (e.g. a mouse move), instead of absolute values.
    ValueBinaryZeroOrOne      = WD_BIT(1),  ///< The input slot will either be zero or one. Used for all buttons and keys.
    ValueRangeZeroToOne       = WD_BIT(2),  ///< The input slot has analog values between zero and one. Used for analog axis like the xbox triggers or thumb-sticks.
    ValueRangeZeroToInf       = WD_BIT(3),  ///< The input slot has unbounded values larger than zero. Used for all absolute positions, such as the mouse position.
    Pressable                 = WD_BIT(4),  ///< The slot can be pressed (e.g. a key). This is not possible for an axis, such as the mouse or an analog stick.
    Holdable                  = WD_BIT(5),  ///< The user can hold down the key. Possible for buttons, but not for axes or for wheels such as the mouse wheel.
    HalfAxis                  = WD_BIT(6),  ///< The input slot represents one half of the actually possible data. Used for all axes (pos / neg mouse movement, thumb-sticks).
    FullAxis                  = WD_BIT(7),  ///< The input slot represents one full axis. Mostly used for devices that report absolute values, such as the mouse position or touch input positions (values between zero and one) 
    RequiresDeadZone          = WD_BIT(8),  ///< The input slot represents hardware that should use a dead zone, otherwise it might fire prematurely. Mostly used on thumb-sticks and trigger buttons.
    ValuesAreNonContinuous    = WD_BIT(9),  ///< The values of the slot can jump around randomly, ie. the user can input arbitrary values, like the position on a touchpad
    ActivationDependsOnOthers = WD_BIT(10), ///< Whether this slot can be activated depends on whether certain other slots are active. This is the case for touch-points which are numbered depending on how many other touch-points are already active.
    NeverTimeScale            = WD_BIT(11), ///< When this flag is specified, data from the input slot will never be scaled by the input update time difference. Important for mouse deltas and such.


    // Some predefined sets of flags for the most common use cases
    IsButton                  =                         ValueBinaryZeroOrOne | Pressable | Holdable,
    IsMouseWheel              = ReportsRelativeValues | ValueRangeZeroToInf  | Pressable |            HalfAxis |                    NeverTimeScale,
    IsAnalogTrigger           =                         ValueRangeZeroToOne  | Pressable | Holdable | FullAxis | RequiresDeadZone,
    IsMouseAxisPosition       =                         ValueRangeZeroToOne  |                        FullAxis |                    NeverTimeScale,
    IsMouseAxisMove           = ReportsRelativeValues | ValueRangeZeroToInf  |                        HalfAxis |                    NeverTimeScale,
    IsAnalogStick             =                         ValueRangeZeroToOne  |             Holdable | HalfAxis | RequiresDeadZone,
    IsDoubleClick             =                         ValueBinaryZeroOrOne | Pressable |                                          NeverTimeScale,
    IsTouchPosition           =                         ValueRangeZeroToOne  |                        FullAxis |                    NeverTimeScale | ValuesAreNonContinuous,
    IsTouchPoint              =                         ValueBinaryZeroOrOne | Pressable | Holdable |                                                ActivationDependsOnOthers,
    IsDPad                    =                         ValueBinaryZeroOrOne | Pressable | Holdable | HalfAxis,
    IsTrackedValue            =                         ValueRangeZeroToInf  |                        HalfAxis |                    NeverTimeScale | ValuesAreNonContinuous,

    Default                   = None
  };

  struct Bits
  {
    StorageType ReportsRelativeValues     : 1;
    StorageType ValueBinaryZeroOrOne      : 1;
    StorageType ValueRangeZeroToOne       : 1;
    StorageType ValueRangeZeroToInf       : 1;
    StorageType Pressable                 : 1;
    StorageType Holdable                  : 1;
    StorageType HalfAxis                  : 1;
    StorageType FullAxis                  : 1;
    StorageType RequiresDeadZone          : 1;
    StorageType ValuesAreNonContinuous    : 1;
    StorageType ActivationDependsOnOthers : 1;
  };
};

WD_DECLARE_FLAGS_OPERATORS(wdInputSlotFlags);

#define wdInputSlot_None                  ""

//
// Touchpads
//

#define wdInputSlot_TouchPoint0           "touchpoint_0"
#define wdInputSlot_TouchPoint0_PositionX "touchpoint_0_position_x"
#define wdInputSlot_TouchPoint0_PositionY "touchpoint_0_position_y"

#define wdInputSlot_TouchPoint1           "touchpoint_1"
#define wdInputSlot_TouchPoint1_PositionX "touchpoint_1_position_x"
#define wdInputSlot_TouchPoint1_PositionY "touchpoint_1_position_y"

#define wdInputSlot_TouchPoint2           "touchpoint_2"
#define wdInputSlot_TouchPoint2_PositionX "touchpoint_2_position_x"
#define wdInputSlot_TouchPoint2_PositionY "touchpoint_2_position_y"

#define wdInputSlot_TouchPoint3           "touchpoint_3"
#define wdInputSlot_TouchPoint3_PositionX "touchpoint_3_position_x"
#define wdInputSlot_TouchPoint3_PositionY "touchpoint_3_position_y"

#define wdInputSlot_TouchPoint4           "touchpoint_4"
#define wdInputSlot_TouchPoint4_PositionX "touchpoint_4_position_x"
#define wdInputSlot_TouchPoint4_PositionY "touchpoint_4_position_y"

#define wdInputSlot_TouchPoint5           "touchpoint_5"
#define wdInputSlot_TouchPoint5_PositionX "touchpoint_5_position_x"
#define wdInputSlot_TouchPoint5_PositionY "touchpoint_5_position_y"

#define wdInputSlot_TouchPoint6           "touchpoint_6"
#define wdInputSlot_TouchPoint6_PositionX "touchpoint_6_position_x"
#define wdInputSlot_TouchPoint6_PositionY "touchpoint_6_position_y"

#define wdInputSlot_TouchPoint7           "touchpoint_7"
#define wdInputSlot_TouchPoint7_PositionX "touchpoint_7_position_x"
#define wdInputSlot_TouchPoint7_PositionY "touchpoint_7_position_y"

#define wdInputSlot_TouchPoint8           "touchpoint_8"
#define wdInputSlot_TouchPoint8_PositionX "touchpoint_8_position_x"
#define wdInputSlot_TouchPoint8_PositionY "touchpoint_8_position_y"

#define wdInputSlot_TouchPoint9           "touchpoint_9"
#define wdInputSlot_TouchPoint9_PositionX "touchpoint_9_position_x"
#define wdInputSlot_TouchPoint9_PositionY "touchpoint_9_position_y"

//
// Standard Controllers
//

#define wdInputSlot_Controller0_ButtonA         "controller0_button_a"
#define wdInputSlot_Controller0_ButtonB         "controller0_button_b"
#define wdInputSlot_Controller0_ButtonX         "controller0_button_x"
#define wdInputSlot_Controller0_ButtonY         "controller0_button_y"
#define wdInputSlot_Controller0_ButtonStart     "controller0_button_start"
#define wdInputSlot_Controller0_ButtonBack      "controller0_button_back"
#define wdInputSlot_Controller0_LeftShoulder    "controller0_left_shoulder"
#define wdInputSlot_Controller0_RightShoulder   "controller0_right_shoulder"
#define wdInputSlot_Controller0_LeftTrigger     "controller0_left_trigger"
#define wdInputSlot_Controller0_RightTrigger    "controller0_right_trigger"
#define wdInputSlot_Controller0_PadUp           "controller0_pad_up"
#define wdInputSlot_Controller0_PadDown         "controller0_pad_down"
#define wdInputSlot_Controller0_PadLeft         "controller0_pad_left"
#define wdInputSlot_Controller0_PadRight        "controller0_pad_right"
#define wdInputSlot_Controller0_LeftStick       "controller0_left_stick"
#define wdInputSlot_Controller0_RightStick      "controller0_right_stick"
#define wdInputSlot_Controller0_LeftStick_NegX  "controller0_leftstick_negx"
#define wdInputSlot_Controller0_LeftStick_PosX  "controller0_leftstick_posx"
#define wdInputSlot_Controller0_LeftStick_NegY  "controller0_leftstick_negy"
#define wdInputSlot_Controller0_LeftStick_PosY  "controller0_leftstick_posy"
#define wdInputSlot_Controller0_RightStick_NegX "controller0_rightstick_negx"
#define wdInputSlot_Controller0_RightStick_PosX "controller0_rightstick_posx"
#define wdInputSlot_Controller0_RightStick_NegY "controller0_rightstick_negy"
#define wdInputSlot_Controller0_RightStick_PosY "controller0_rightstick_posy"

#define wdInputSlot_Controller1_ButtonA         "controller1_button_a"
#define wdInputSlot_Controller1_ButtonB         "controller1_button_b"
#define wdInputSlot_Controller1_ButtonX         "controller1_button_x"
#define wdInputSlot_Controller1_ButtonY         "controller1_button_y"
#define wdInputSlot_Controller1_ButtonStart     "controller1_button_start"
#define wdInputSlot_Controller1_ButtonBack      "controller1_button_back"
#define wdInputSlot_Controller1_LeftShoulder    "controller1_left_shoulder"
#define wdInputSlot_Controller1_RightShoulder   "controller1_right_shoulder"
#define wdInputSlot_Controller1_LeftTrigger     "controller1_left_trigger"
#define wdInputSlot_Controller1_RightTrigger    "controller1_right_trigger"
#define wdInputSlot_Controller1_PadUp           "controller1_pad_up"
#define wdInputSlot_Controller1_PadDown         "controller1_pad_down"
#define wdInputSlot_Controller1_PadLeft         "controller1_pad_left"
#define wdInputSlot_Controller1_PadRight        "controller1_pad_right"
#define wdInputSlot_Controller1_LeftStick       "controller1_left_stick"
#define wdInputSlot_Controller1_RightStick      "controller1_right_stick"
#define wdInputSlot_Controller1_LeftStick_NegX  "controller1_leftstick_negx"
#define wdInputSlot_Controller1_LeftStick_PosX  "controller1_leftstick_posx"
#define wdInputSlot_Controller1_LeftStick_NegY  "controller1_leftstick_negy"
#define wdInputSlot_Controller1_LeftStick_PosY  "controller1_leftstick_posy"
#define wdInputSlot_Controller1_RightStick_NegX "controller1_rightstick_negx"
#define wdInputSlot_Controller1_RightStick_PosX "controller1_rightstick_posx"
#define wdInputSlot_Controller1_RightStick_NegY "controller1_rightstick_negy"
#define wdInputSlot_Controller1_RightStick_PosY "controller1_rightstick_posy"

#define wdInputSlot_Controller2_ButtonA         "controller2_button_a"
#define wdInputSlot_Controller2_ButtonB         "controller2_button_b"
#define wdInputSlot_Controller2_ButtonX         "controller2_button_x"
#define wdInputSlot_Controller2_ButtonY         "controller2_button_y"
#define wdInputSlot_Controller2_ButtonStart     "controller2_button_start"
#define wdInputSlot_Controller2_ButtonBack      "controller2_button_back"
#define wdInputSlot_Controller2_LeftShoulder    "controller2_left_shoulder"
#define wdInputSlot_Controller2_RightShoulder   "controller2_right_shoulder"
#define wdInputSlot_Controller2_LeftTrigger     "controller2_left_trigger"
#define wdInputSlot_Controller2_RightTrigger    "controller2_right_trigger"
#define wdInputSlot_Controller2_PadUp           "controller2_pad_up"
#define wdInputSlot_Controller2_PadDown         "controller2_pad_down"
#define wdInputSlot_Controller2_PadLeft         "controller2_pad_left"
#define wdInputSlot_Controller2_PadRight        "controller2_pad_right"
#define wdInputSlot_Controller2_LeftStick       "controller2_left_stick"
#define wdInputSlot_Controller2_RightStick      "controller2_right_stick"
#define wdInputSlot_Controller2_LeftStick_NegX  "controller2_leftstick_negx"
#define wdInputSlot_Controller2_LeftStick_PosX  "controller2_leftstick_posx"
#define wdInputSlot_Controller2_LeftStick_NegY  "controller2_leftstick_negy"
#define wdInputSlot_Controller2_LeftStick_PosY  "controller2_leftstick_posy"
#define wdInputSlot_Controller2_RightStick_NegX "controller2_rightstick_negx"
#define wdInputSlot_Controller2_RightStick_PosX "controller2_rightstick_posx"
#define wdInputSlot_Controller2_RightStick_NegY "controller2_rightstick_negy"
#define wdInputSlot_Controller2_RightStick_PosY "controller2_rightstick_posy"

#define wdInputSlot_Controller3_ButtonA         "controller3_button_a"
#define wdInputSlot_Controller3_ButtonB         "controller3_button_b"
#define wdInputSlot_Controller3_ButtonX         "controller3_button_x"
#define wdInputSlot_Controller3_ButtonY         "controller3_button_y"
#define wdInputSlot_Controller3_ButtonStart     "controller3_button_start"
#define wdInputSlot_Controller3_ButtonBack      "controller3_button_back"
#define wdInputSlot_Controller3_LeftShoulder    "controller3_left_shoulder"
#define wdInputSlot_Controller3_RightShoulder   "controller3_right_shoulder"
#define wdInputSlot_Controller3_LeftTrigger     "controller3_left_trigger"
#define wdInputSlot_Controller3_RightTrigger    "controller3_right_trigger"
#define wdInputSlot_Controller3_PadUp           "controller3_pad_up"
#define wdInputSlot_Controller3_PadDown         "controller3_pad_down"
#define wdInputSlot_Controller3_PadLeft         "controller3_pad_left"
#define wdInputSlot_Controller3_PadRight        "controller3_pad_right"
#define wdInputSlot_Controller3_LeftStick       "controller3_left_stick"
#define wdInputSlot_Controller3_RightStick      "controller3_right_stick"
#define wdInputSlot_Controller3_LeftStick_NegX  "controller3_leftstick_negx"
#define wdInputSlot_Controller3_LeftStick_PosX  "controller3_leftstick_posx"
#define wdInputSlot_Controller3_LeftStick_NegY  "controller3_leftstick_negy"
#define wdInputSlot_Controller3_LeftStick_PosY  "controller3_leftstick_posy"
#define wdInputSlot_Controller3_RightStick_NegX "controller3_rightstick_negx"
#define wdInputSlot_Controller3_RightStick_PosX "controller3_rightstick_posx"
#define wdInputSlot_Controller3_RightStick_NegY "controller3_rightstick_negy"
#define wdInputSlot_Controller3_RightStick_PosY "controller3_rightstick_posy"

//
// Keyboard
//

#define wdInputSlot_KeyLeft           "keyboard_left"
#define wdInputSlot_KeyRight          "keyboard_right"
#define wdInputSlot_KeyUp             "keyboard_up"
#define wdInputSlot_KeyDown           "keyboard_down"
#define wdInputSlot_KeyEscape         "keyboard_escape"
#define wdInputSlot_KeySpace          "keyboard_space"
#define wdInputSlot_KeyBackspace      "keyboard_backspace"
#define wdInputSlot_KeyReturn         "keyboard_return"
#define wdInputSlot_KeyTab            "keyboard_tab"
#define wdInputSlot_KeyLeftShift      "keyboard_left_shift"
#define wdInputSlot_KeyRightShift     "keyboard_right_shift"
#define wdInputSlot_KeyLeftCtrl       "keyboard_left_ctrl"
#define wdInputSlot_KeyRightCtrl      "keyboard_right_ctrl"
#define wdInputSlot_KeyLeftAlt        "keyboard_left_alt"
#define wdInputSlot_KeyRightAlt       "keyboard_right_alt"
#define wdInputSlot_KeyLeftWin        "keyboard_left_win"
#define wdInputSlot_KeyRightWin       "keyboard_right_win"
#define wdInputSlot_KeyBracketOpen    "keyboard_bracket_open"
#define wdInputSlot_KeyBracketClose   "keyboard_bracket_close"
#define wdInputSlot_KeySemicolon      "keyboard_semicolon"
#define wdInputSlot_KeyApostrophe     "keyboard_apostrophe"
#define wdInputSlot_KeySlash          "keyboard_slash"
#define wdInputSlot_KeyEquals         "keyboard_equals"
#define wdInputSlot_KeyTilde          "keyboard_tilde"
#define wdInputSlot_KeyHyphen         "keyboard_hyphen"
#define wdInputSlot_KeyComma          "keyboard_comma"
#define wdInputSlot_KeyPeriod         "keyboard_period"
#define wdInputSlot_KeyBackslash      "keyboard_backslash"
#define wdInputSlot_KeyPipe           "keyboard_pipe"
#define wdInputSlot_Key1              "keyboard_1"
#define wdInputSlot_Key2              "keyboard_2"
#define wdInputSlot_Key3              "keyboard_3"
#define wdInputSlot_Key4              "keyboard_4"
#define wdInputSlot_Key5              "keyboard_5"
#define wdInputSlot_Key6              "keyboard_6"
#define wdInputSlot_Key7              "keyboard_7"
#define wdInputSlot_Key8              "keyboard_8"
#define wdInputSlot_Key9              "keyboard_9"
#define wdInputSlot_Key0              "keyboard_0"
#define wdInputSlot_KeyNumpad1        "keyboard_numpad_1"
#define wdInputSlot_KeyNumpad2        "keyboard_numpad_2"
#define wdInputSlot_KeyNumpad3        "keyboard_numpad_3"
#define wdInputSlot_KeyNumpad4        "keyboard_numpad_4"
#define wdInputSlot_KeyNumpad5        "keyboard_numpad_5"
#define wdInputSlot_KeyNumpad6        "keyboard_numpad_6"
#define wdInputSlot_KeyNumpad7        "keyboard_numpad_7"
#define wdInputSlot_KeyNumpad8        "keyboard_numpad_8"
#define wdInputSlot_KeyNumpad9        "keyboard_numpad_9"
#define wdInputSlot_KeyNumpad0        "keyboard_numpad_0"
#define wdInputSlot_KeyA              "keyboard_a"
#define wdInputSlot_KeyB              "keyboard_b"
#define wdInputSlot_KeyC              "keyboard_c"
#define wdInputSlot_KeyD              "keyboard_d"
#define wdInputSlot_KeyE              "keyboard_e"
#define wdInputSlot_KeyF              "keyboard_f"
#define wdInputSlot_KeyG              "keyboard_g"
#define wdInputSlot_KeyH              "keyboard_h"
#define wdInputSlot_KeyI              "keyboard_i"
#define wdInputSlot_KeyJ              "keyboard_j"
#define wdInputSlot_KeyK              "keyboard_k"
#define wdInputSlot_KeyL              "keyboard_l"
#define wdInputSlot_KeyM              "keyboard_m"
#define wdInputSlot_KeyN              "keyboard_n"
#define wdInputSlot_KeyO              "keyboard_o"
#define wdInputSlot_KeyP              "keyboard_p"
#define wdInputSlot_KeyQ              "keyboard_q"
#define wdInputSlot_KeyR              "keyboard_r"
#define wdInputSlot_KeyS              "keyboard_s"
#define wdInputSlot_KeyT              "keyboard_t"
#define wdInputSlot_KeyU              "keyboard_u"
#define wdInputSlot_KeyV              "keyboard_v"
#define wdInputSlot_KeyW              "keyboard_w"
#define wdInputSlot_KeyX              "keyboard_x"
#define wdInputSlot_KeyY              "keyboard_y"
#define wdInputSlot_KeyZ              "keyboard_z"
#define wdInputSlot_KeyF1             "keyboard_f1"
#define wdInputSlot_KeyF2             "keyboard_f2"
#define wdInputSlot_KeyF3             "keyboard_f3"
#define wdInputSlot_KeyF4             "keyboard_f4"
#define wdInputSlot_KeyF5             "keyboard_f5"
#define wdInputSlot_KeyF6             "keyboard_f6"
#define wdInputSlot_KeyF7             "keyboard_f7"
#define wdInputSlot_KeyF8             "keyboard_f8"
#define wdInputSlot_KeyF9             "keyboard_f9"
#define wdInputSlot_KeyF10            "keyboard_f10"
#define wdInputSlot_KeyF11            "keyboard_f11"
#define wdInputSlot_KeyF12            "keyboard_f12"
#define wdInputSlot_KeyHome           "keyboard_home"
#define wdInputSlot_KeyEnd            "keyboard_end"
#define wdInputSlot_KeyDelete         "keyboard_delete"
#define wdInputSlot_KeyInsert         "keyboard_insert"
#define wdInputSlot_KeyPageUp         "keyboard_page_up"
#define wdInputSlot_KeyPageDown       "keyboard_page_down"
#define wdInputSlot_KeyNumLock        "keyboard_numlock"
#define wdInputSlot_KeyNumpadPlus     "keyboard_numpad_plus"
#define wdInputSlot_KeyNumpadMinus    "keyboard_numpad_minus"
#define wdInputSlot_KeyNumpadStar     "keyboard_numpad_star"
#define wdInputSlot_KeyNumpadSlash    "keyboard_numpad_slash"
#define wdInputSlot_KeyNumpadPeriod   "keyboard_numpad_period"
#define wdInputSlot_KeyNumpadEnter    "keyboard_numpad_enter"
#define wdInputSlot_KeyCapsLock       "keyboard_capslock"
#define wdInputSlot_KeyPrint          "keyboard_print"
#define wdInputSlot_KeyScroll         "keyboard_scroll"
#define wdInputSlot_KeyPause          "keyboard_pause"
#define wdInputSlot_KeyApps           "keyboard_apps"
#define wdInputSlot_KeyPrevTrack      "keyboard_prev_track"
#define wdInputSlot_KeyNextTrack      "keyboard_next_track"
#define wdInputSlot_KeyPlayPause      "keyboard_play_pause"
#define wdInputSlot_KeyStop           "keyboard_stop"
#define wdInputSlot_KeyVolumeUp       "keyboard_volume_up"
#define wdInputSlot_KeyVolumeDown     "keyboard_volume_down"
#define wdInputSlot_KeyMute           "keyboard_mute"

//
// Mouse
//

#define wdInputSlot_MouseWheelUp      "mouse_wheel_up"
#define wdInputSlot_MouseWheelDown    "mouse_wheel_down"
#define wdInputSlot_MouseMoveNegX     "mouse_move_negx"
#define wdInputSlot_MouseMovePosX     "mouse_move_posx"
#define wdInputSlot_MouseMoveNegY     "mouse_move_negy"
#define wdInputSlot_MouseMovePosY     "mouse_move_posy"
#define wdInputSlot_MouseButton0      "mouse_button_0"
#define wdInputSlot_MouseButton1      "mouse_button_1"
#define wdInputSlot_MouseButton2      "mouse_button_2"
#define wdInputSlot_MouseButton3      "mouse_button_3"
#define wdInputSlot_MouseButton4      "mouse_button_4"
#define wdInputSlot_MouseDblClick0    "mouse_button_0_doubleclick"
#define wdInputSlot_MouseDblClick1    "mouse_button_1_doubleclick"
#define wdInputSlot_MouseDblClick2    "mouse_button_2_doubleclick"
#define wdInputSlot_MousePositionX    "mouse_position_x"
#define wdInputSlot_MousePositionY    "mouse_position_y"

//
// Spatial Input Data (Tracked Hands or Controllers)
//

#define wdInputSlot_Spatial_Hand0_Tracked "spatial_hand0_tracked"
#define wdInputSlot_Spatial_Hand0_Pressed "spatial_hand0_pressed"
#define wdInputSlot_Spatial_Hand0_PositionPosX "spatial_hand0_position_posx"
#define wdInputSlot_Spatial_Hand0_PositionPosY "spatial_hand0_position_posy"
#define wdInputSlot_Spatial_Hand0_PositionPosZ "spatial_hand0_position_posz"
#define wdInputSlot_Spatial_Hand0_PositionNegX "spatial_hand0_position_negx"
#define wdInputSlot_Spatial_Hand0_PositionNegY "spatial_hand0_position_negy"
#define wdInputSlot_Spatial_Hand0_PositionNegZ "spatial_hand0_position_negz"

#define wdInputSlot_Spatial_Hand1_Tracked "spatial_hand1_tracked"
#define wdInputSlot_Spatial_Hand1_Pressed "spatial_hand1_pressed"
#define wdInputSlot_Spatial_Hand1_PositionPosX "spatial_hand1_position_posx"
#define wdInputSlot_Spatial_Hand1_PositionPosY "spatial_hand1_position_posy"
#define wdInputSlot_Spatial_Hand1_PositionPosZ "spatial_hand1_position_posz"
#define wdInputSlot_Spatial_Hand1_PositionNegX "spatial_hand1_position_negx"
#define wdInputSlot_Spatial_Hand1_PositionNegY "spatial_hand1_position_negy"
#define wdInputSlot_Spatial_Hand1_PositionNegZ "spatial_hand1_position_negz"

#define wdInputSlot_Spatial_Head_PositionPosX "spatial_head_position_posx"
#define wdInputSlot_Spatial_Head_PositionPosY "spatial_head_position_posy"
#define wdInputSlot_Spatial_Head_PositionPosZ "spatial_head_position_posz"
#define wdInputSlot_Spatial_Head_PositionNegX "spatial_head_position_negx"
#define wdInputSlot_Spatial_Head_PositionNegY "spatial_head_position_negy"
#define wdInputSlot_Spatial_Head_PositionNegZ "spatial_head_position_negz"

#define wdInputSlot_Spatial_Head_ForwardPosX "spatial_head_forward_posx"
#define wdInputSlot_Spatial_Head_ForwardPosY "spatial_head_forward_posy"
#define wdInputSlot_Spatial_Head_ForwardPosZ "spatial_head_forward_posz"
#define wdInputSlot_Spatial_Head_ForwardNegX "spatial_head_forward_negx"
#define wdInputSlot_Spatial_Head_ForwardNegY "spatial_head_forward_negy"
#define wdInputSlot_Spatial_Head_ForwardNegZ "spatial_head_forward_negz"

#define wdInputSlot_Spatial_Head_UpPosX "spatial_head_up_posx"
#define wdInputSlot_Spatial_Head_UpPosY "spatial_head_up_posy"
#define wdInputSlot_Spatial_Head_UpPosZ "spatial_head_up_posz"
#define wdInputSlot_Spatial_Head_UpNegX "spatial_head_up_negx"
#define wdInputSlot_Spatial_Head_UpNegY "spatial_head_up_negy"
#define wdInputSlot_Spatial_Head_UpNegZ "spatial_head_up_negz"

