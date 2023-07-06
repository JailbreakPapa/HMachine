import __Log = require("TypeScript/wd/Log")
export import Log = __Log.Log;

import __Utils = require("TypeScript/wd/Utils")
export import Utils = __Utils.Utils;

import __Vec2 = require("TypeScript/wd/Vec2")
export import Vec2 = __Vec2.Vec2;

import __Vec3 = require("TypeScript/wd/Vec3")
export import Vec3 = __Vec3.Vec3;

import __Mat3 = require("TypeScript/wd/Mat3")
export import Mat3 = __Mat3.Mat3;

import __Mat4 = require("TypeScript/wd/Mat4")
export import Mat4 = __Mat4.Mat4;

import __Quat = require("TypeScript/wd/Quat")
export import Quat = __Quat.Quat;

import __Transform = require("TypeScript/wd/Transform")
export import Transform = __Transform.Transform;

import __Color = require("TypeScript/wd/Color")
export import Color = __Color.Color;

import __Time = require("TypeScript/wd/Time")
export import Time = __Time.Time;

import __Angle = require("TypeScript/wd/Angle")
export import Angle = __Angle.Angle;

import __GameObject = require("TypeScript/wd/GameObject")
export import GameObject = __GameObject.GameObject;

import __Component = require("TypeScript/wd/Component")
export import Component = __Component.Component;
export import TypescriptComponent = __Component.TypescriptComponent;
export import TickedTypescriptComponent = __Component.TickedTypescriptComponent;

import __World = require("TypeScript/wd/World")
export import World = __World.World;
export import GameObjectDesc = __World.GameObjectDesc;

import __Message = require("TypeScript/wd/Message")
export import Message = __Message.Message;
//export import EventMessage = __Message.EventMessage; // not supported yet, to write custom TS event messages

import __Debug = require("TypeScript/wd/Debug")
export import Debug = __Debug.Debug;

import __Random = require("TypeScript/wd/Random")
export import Random = __Random.Random;

import __Clock = require("TypeScript/wd/Clock")
export import Clock = __Clock.Clock;

import __Physics = require("TypeScript/wd/Physics")
export import Physics = __Physics.Physics;



// AUTO-GENERATED
import __AllComponents = require("./wd/AllComponents")
export import RenderComponent = __AllComponents.RenderComponent;
export import AlwaysVisibleComponent = __AllComponents.AlwaysVisibleComponent;
export import SettingsComponent = __AllComponents.SettingsComponent;
export import AmbientLightComponent = __AllComponents.AmbientLightComponent;
export import MeshComponentBase = __AllComponents.MeshComponentBase;
export import AnimatedMeshComponent = __AllComponents.AnimatedMeshComponent;
export import AnimationControllerComponent = __AllComponents.AnimationControllerComponent;
export import AreaDamageComponent = __AllComponents.AreaDamageComponent;
export import BakedProbesComponent = __AllComponents.BakedProbesComponent;
export import BakedProbesVolumeComponent = __AllComponents.BakedProbesVolumeComponent;
export import BeamComponent = __AllComponents.BeamComponent;
export import BlackboardComponent = __AllComponents.BlackboardComponent;
export import ReflectionProbeComponentBase = __AllComponents.ReflectionProbeComponentBase;
export import BoxReflectionProbeComponent = __AllComponents.BoxReflectionProbeComponent;
export import CameraComponent = __AllComponents.CameraComponent;
export import CameraShakeComponent = __AllComponents.CameraShakeComponent;
export import CameraShakeVolumeComponent = __AllComponents.CameraShakeVolumeComponent;
export import CameraShakeVolumeSphereComponent = __AllComponents.CameraShakeVolumeSphereComponent;
export import CharacterControllerComponent = __AllComponents.CharacterControllerComponent;
export import ClothSheetComponent = __AllComponents.ClothSheetComponent;
export import CollectionComponent = __AllComponents.CollectionComponent;
export import ColorAnimationComponent = __AllComponents.ColorAnimationComponent;
export import CommentComponent = __AllComponents.CommentComponent;
export import CustomMeshComponent = __AllComponents.CustomMeshComponent;
export import DebugTextComponent = __AllComponents.DebugTextComponent;
export import DecalComponent = __AllComponents.DecalComponent;
export import DeviceTrackingComponent = __AllComponents.DeviceTrackingComponent;
export import LightComponent = __AllComponents.LightComponent;
export import DirectionalLightComponent = __AllComponents.DirectionalLightComponent;
export import EventMessageHandlerComponent = __AllComponents.EventMessageHandlerComponent;
export import FakeRopeComponent = __AllComponents.FakeRopeComponent;
export import FogComponent = __AllComponents.FogComponent;
export import FollowPathComponent = __AllComponents.FollowPathComponent;
export import ForwardEventsToGameStateComponent = __AllComponents.ForwardEventsToGameStateComponent;
export import MeshComponent = __AllComponents.MeshComponent;
export import GizmoComponent = __AllComponents.GizmoComponent;
export import GrabbableItemComponent = __AllComponents.GrabbableItemComponent;
export import GreyBoxComponent = __AllComponents.GreyBoxComponent;
export import HeadBoneComponent = __AllComponents.HeadBoneComponent;
export import HeightfieldComponent = __AllComponents.HeightfieldComponent;
export import InputComponent = __AllComponents.InputComponent;
export import InstancedMeshComponent = __AllComponents.InstancedMeshComponent;
export import JointAttachmentComponent = __AllComponents.JointAttachmentComponent;
export import JointOverrideComponent = __AllComponents.JointOverrideComponent;
export import LineToComponent = __AllComponents.LineToComponent;
export import MarkerComponent = __AllComponents.MarkerComponent;
export import MoveToComponent = __AllComponents.MoveToComponent;
export import OccluderComponent = __AllComponents.OccluderComponent;
export import PathComponent = __AllComponents.PathComponent;
export import PathNodeComponent = __AllComponents.PathNodeComponent;
export import PlayerStartPointComponent = __AllComponents.PlayerStartPointComponent;
export import PointLightComponent = __AllComponents.PointLightComponent;
export import PowerConnectorComponent = __AllComponents.PowerConnectorComponent;
export import PrefabReferenceComponent = __AllComponents.PrefabReferenceComponent;
export import ProjectileComponent = __AllComponents.ProjectileComponent;
export import PropertyAnimComponent = __AllComponents.PropertyAnimComponent;
export import RaycastComponent = __AllComponents.RaycastComponent;
export import RenderTargetActivatorComponent = __AllComponents.RenderTargetActivatorComponent;
export import RopeRenderComponent = __AllComponents.RopeRenderComponent;
export import TransformComponent = __AllComponents.TransformComponent;
export import RotorComponent = __AllComponents.RotorComponent;
export import SensorComponent = __AllComponents.SensorComponent;
export import SensorConeComponent = __AllComponents.SensorConeComponent;
export import SensorCylinderComponent = __AllComponents.SensorCylinderComponent;
export import SensorSphereComponent = __AllComponents.SensorSphereComponent;
export import ShapeIconComponent = __AllComponents.ShapeIconComponent;
export import SimpleAnimationComponent = __AllComponents.SimpleAnimationComponent;
export import SimpleWindComponent = __AllComponents.SimpleWindComponent;
export import SkeletonComponent = __AllComponents.SkeletonComponent;
export import SkeletonPoseComponent = __AllComponents.SkeletonPoseComponent;
export import SkyBoxComponent = __AllComponents.SkyBoxComponent;
export import SkyLightComponent = __AllComponents.SkyLightComponent;
export import SliderComponent = __AllComponents.SliderComponent;
export import SpatialAnchorComponent = __AllComponents.SpatialAnchorComponent;
export import SpawnComponent = __AllComponents.SpawnComponent;
export import SphereReflectionProbeComponent = __AllComponents.SphereReflectionProbeComponent;
export import SpotLightComponent = __AllComponents.SpotLightComponent;
export import SpriteComponent = __AllComponents.SpriteComponent;
export import StageSpaceComponent = __AllComponents.StageSpaceComponent;
export import StateMachineComponent = __AllComponents.StateMachineComponent;
export import TimedDeathComponent = __AllComponents.TimedDeathComponent;
export import TriggerDelayModifierComponent = __AllComponents.TriggerDelayModifierComponent;
export import VisualScriptComponent = __AllComponents.VisualScriptComponent;
export import VisualizeHandComponent = __AllComponents.VisualizeHandComponent;
export import WindVolumeComponent = __AllComponents.WindVolumeComponent;
export import WindVolumeConeComponent = __AllComponents.WindVolumeConeComponent;
export import WindVolumeCylinderComponent = __AllComponents.WindVolumeCylinderComponent;
export import WindVolumeSphereComponent = __AllComponents.WindVolumeSphereComponent;



// AUTO-GENERATED
import __AllMessages = require("./wd/AllMessages")
export import EventMsgPathChanged = __AllMessages.EventMsgPathChanged;
export import EventMsgSetPowerInput = __AllMessages.EventMsgSetPowerInput;
export import MsgAnimationPosePreparing = __AllMessages.MsgAnimationPosePreparing;
export import MsgAnimationPoseProposal = __AllMessages.MsgAnimationPoseProposal;
export import MsgAnimationPoseUpdated = __AllMessages.MsgAnimationPoseUpdated;
export import MsgAnimationReachedEnd = __AllMessages.MsgAnimationReachedEnd;
export import MsgApplyRootMotion = __AllMessages.MsgApplyRootMotion;
export import MsgBlackboardEntryChanged = __AllMessages.MsgBlackboardEntryChanged;
export import MsgBuildStaticMesh = __AllMessages.MsgBuildStaticMesh;
export import MsgChildrenChanged = __AllMessages.MsgChildrenChanged;
export import MsgCollision = __AllMessages.MsgCollision;
export import MsgComponentInternalTrigger = __AllMessages.MsgComponentInternalTrigger;
export import MsgComponentsChanged = __AllMessages.MsgComponentsChanged;
export import MsgDamage = __AllMessages.MsgDamage;
export import MsgDeleteGameObject = __AllMessages.MsgDeleteGameObject;
export import MsgExtractGeometry = __AllMessages.MsgExtractGeometry;
export import MsgExtractOccluderData = __AllMessages.MsgExtractOccluderData;
export import MsgExtractRenderData = __AllMessages.MsgExtractRenderData;
export import MsgGenericEvent = __AllMessages.MsgGenericEvent;
export import MsgInputActionTriggered = __AllMessages.MsgInputActionTriggered;
export import MsgMoveCharacterController = __AllMessages.MsgMoveCharacterController;
export import MsgObjectGrabbed = __AllMessages.MsgObjectGrabbed;
export import MsgOnlyApplyToObject = __AllMessages.MsgOnlyApplyToObject;
export import MsgParentChanged = __AllMessages.MsgParentChanged;
export import MsgPhysicsAddForce = __AllMessages.MsgPhysicsAddForce;
export import MsgPhysicsAddImpulse = __AllMessages.MsgPhysicsAddImpulse;
export import MsgPhysicsJointBroke = __AllMessages.MsgPhysicsJointBroke;
export import MsgQueryAnimationSkeleton = __AllMessages.MsgQueryAnimationSkeleton;
export import MsgReleaseObjectGrab = __AllMessages.MsgReleaseObjectGrab;
export import MsgRetrieveBoneState = __AllMessages.MsgRetrieveBoneState;
export import MsgRopePoseUpdated = __AllMessages.MsgRopePoseUpdated;
export import MsgSensorDetectedObjectsChanged = __AllMessages.MsgSensorDetectedObjectsChanged;
export import MsgSetColor = __AllMessages.MsgSetColor;
export import MsgSetFloatParameter = __AllMessages.MsgSetFloatParameter;
export import MsgSetMeshMaterial = __AllMessages.MsgSetMeshMaterial;
export import MsgSetPlaying = __AllMessages.MsgSetPlaying;
export import MsgStateMachineStateChanged = __AllMessages.MsgStateMachineStateChanged;
export import MsgTransformChanged = __AllMessages.MsgTransformChanged;
export import MsgTriggerTriggered = __AllMessages.MsgTriggerTriggered;
export import MsgTypeScriptMsgProxy = __AllMessages.MsgTypeScriptMsgProxy;
export import MsgUpdateLocalBounds = __AllMessages.MsgUpdateLocalBounds;



// AUTO-GENERATED
import __AllEnums = require("./wd/AllEnums")
export import BasisAxis = __AllEnums.BasisAxis;
export import CameraMode = __AllEnums.CameraMode;
export import CameraUsageHint = __AllEnums.CameraUsageHint;
export import GreyBoxShape = __AllEnums.GreyBoxShape;
export import InputMessageGranularity = __AllEnums.InputMessageGranularity;
export import OnComponentFinishedAction = __AllEnums.OnComponentFinishedAction;
export import PathNodeTangentMode = __AllEnums.PathNodeTangentMode;
export import PropertyAnimMode = __AllEnums.PropertyAnimMode;
export import ReflectionProbeMode = __AllEnums.ReflectionProbeMode;
export import RootMotionMode = __AllEnums.RootMotionMode;
export import SetColorMode = __AllEnums.SetColorMode;
export import SkeletonPoseMode = __AllEnums.SkeletonPoseMode;
export import SpriteBlendMode = __AllEnums.SpriteBlendMode;
export import TriggerState = __AllEnums.TriggerState;
export import UpdateRate = __AllEnums.UpdateRate;
export import WindStrength = __AllEnums.WindStrength;
export import WindVolumeCylinderMode = __AllEnums.WindVolumeCylinderMode;
export import XRDeviceType = __AllEnums.XRDeviceType;
export import XRPoseLocation = __AllEnums.XRPoseLocation;
export import XRStageSpace = __AllEnums.XRStageSpace;
export import XRTransformSpace = __AllEnums.XRTransformSpace;



// AUTO-GENERATED
import __AllFlags = require("./wd/AllFlags")
export import ClothSheetFlags = __AllFlags.ClothSheetFlags;
export import PathComponentFlags = __AllFlags.PathComponentFlags;
export import PhysicsShapeType = __AllFlags.PhysicsShapeType;

