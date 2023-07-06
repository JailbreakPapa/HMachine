#pragma once

#include <Foundation/Math/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

struct wdMsgQueryAnimationSkeleton;

using wdVisualizeSkeletonComponentManager = wdComponentManagerSimple<class wdSkeletonComponent, wdComponentUpdateType::Always, wdBlockStorageType::Compact>;

class WD_RENDERERCORE_DLL wdSkeletonComponent : public wdRenderComponent
{
  WD_DECLARE_COMPONENT_TYPE(wdSkeletonComponent, wdRenderComponent, wdVisualizeSkeletonComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // wdComponent

public:
  virtual void SerializeComponent(wdWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(wdWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;


  //////////////////////////////////////////////////////////////////////////
  // wdRenderComponent

public:
  virtual wdResult GetLocalBounds(wdBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, wdMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // wdSkeletonComponent

public:
  wdSkeletonComponent();
  ~wdSkeletonComponent();

  void SetSkeletonFile(const char* szFile); // [ property ]
  const char* GetSkeletonFile() const;      // [ property ]

  void SetSkeleton(const wdSkeletonResourceHandle& hResource);
  const wdSkeletonResourceHandle& GetSkeleton() const { return m_hSkeleton; }

  void SetBonesToHighlight(const char* szFilter); // [ property ]
  const char* GetBonesToHighlight() const;        // [ property ]

  void VisualizeSkeletonDefaultState();

  bool m_bVisualizeBones = true;
  bool m_bVisualizeColliders = false;
  bool m_bVisualizeJoints = false;
  bool m_bVisualizeSwingLimits = false;
  bool m_bVisualizeTwistLimits = false;

protected:
  void Update();
  void OnAnimationPoseUpdated(wdMsgAnimationPoseUpdated& msg); // [ msg handler ]

  void BuildSkeletonVisualization(wdMsgAnimationPoseUpdated& msg);
  void BuildColliderVisualization(wdMsgAnimationPoseUpdated& msg);
  void BuildJointVisualization(wdMsgAnimationPoseUpdated& msg);

  void OnQueryAnimationSkeleton(wdMsgQueryAnimationSkeleton& msg);
  wdDebugRenderer::Line& AddLine(const wdVec3& vStart, const wdVec3& vEnd, const wdColor& color);

  wdSkeletonResourceHandle m_hSkeleton;
  wdTransform m_RootTransform = wdTransform::IdentityTransform();
  wdUInt32 m_uiSkeletonChangeCounter = 0;
  wdString m_sBonesToHighlight;

  wdBoundingBox m_MaxBounds;
  wdDynamicArray<wdDebugRenderer::Line> m_LinesSkeleton;

  struct SphereShape
  {
    wdTransform m_Transform;
    wdBoundingSphere m_Shape;
    wdColor m_Color;
  };

  struct BoxShape
  {
    wdTransform m_Transform;
    wdBoundingBox m_Shape;
    wdColor m_Color;
  };

  struct CapsuleShape
  {
    wdTransform m_Transform;
    float m_fLength;
    float m_fRadius;
    wdColor m_Color;
  };

  struct AngleShape
  {
    wdTransform m_Transform;
    wdColor m_Color;
    wdAngle m_StartAngle;
    wdAngle m_EndAngle;
  };

  struct ConeLimitShape
  {
    wdTransform m_Transform;
    wdColor m_Color;
    wdAngle m_Angle1;
    wdAngle m_Angle2;
  };

  struct CylinderShape
  {
    wdTransform m_Transform;
    wdColor m_Color;
    float m_fRadius1;
    float m_fRadius2;
    float m_fLength;
  };

  wdDynamicArray<SphereShape> m_SpheresShapes;
  wdDynamicArray<BoxShape> m_BoxShapes;
  wdDynamicArray<CapsuleShape> m_CapsuleShapes;
  wdDynamicArray<AngleShape> m_AngleShapes;
  wdDynamicArray<ConeLimitShape> m_ConeLimitShapes;
  wdDynamicArray<CylinderShape> m_CylinderShapes;
};
