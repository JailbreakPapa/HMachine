#pragma once

#include <Core/World/ComponentManager.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Types/RangeView.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

class wdSkeletonPoseComponentManager : public wdComponentManager<class wdSkeletonPoseComponent, wdBlockStorageType::Compact>
{
public:
  using SUPER = wdComponentManager<wdSkeletonPoseComponent, wdBlockStorageType::Compact>;

  wdSkeletonPoseComponentManager(wdWorld* pWorld)
    : SUPER(pWorld)
  {
  }

  void Update(const wdWorldModule::UpdateContext& context);
  void EnqueueUpdate(wdComponentHandle hComponent);

private:
  mutable wdMutex m_Mutex;
  wdDeque<wdComponentHandle> m_RequireUpdate;

protected:
  virtual void Initialize() override;
};

//////////////////////////////////////////////////////////////////////////

struct wdSkeletonPoseMode
{
  using StorageType = wdUInt8;

  enum Enum
  {
    CustomPose,
    RestPose,
    Disabled,
    Default = CustomPose
  };
};

WD_DECLARE_REFLECTABLE_TYPE(WD_RENDERERCORE_DLL, wdSkeletonPoseMode);


class WD_RENDERERCORE_DLL wdSkeletonPoseComponent : public wdComponent
{
  WD_DECLARE_COMPONENT_TYPE(wdSkeletonPoseComponent, wdComponent, wdSkeletonPoseComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // wdComponent

public:
  virtual void SerializeComponent(wdWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(wdWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // wdSkeletonPoseComponent

public:
  wdSkeletonPoseComponent();
  ~wdSkeletonPoseComponent();

  void SetSkeletonFile(const char* szFile); // [ property ]
  const char* GetSkeletonFile() const;      // [ property ]

  void SetSkeleton(const wdSkeletonResourceHandle& hResource);
  const wdSkeletonResourceHandle& GetSkeleton() const { return m_hSkeleton; }

  wdEnum<wdSkeletonPoseMode> GetPoseMode() const { return m_PoseMode; }
  void SetPoseMode(wdEnum<wdSkeletonPoseMode> mode);

  void ResendPose();

  const wdRangeView<const char*, wdUInt32> GetBones() const;   // [ property ] (exposed bones)
  void SetBone(const char* szKey, const wdVariant& value);     // [ property ] (exposed bones)
  void RemoveBone(const char* szKey);                          // [ property ] (exposed bones)
  bool GetBone(const char* szKey, wdVariant& out_value) const; // [ property ] (exposed bones)

protected:
  void Update();
  void SendRestPose();
  void SendCustomPose();

  float m_fDummy = 0;
  wdUInt8 m_uiResendPose = 0;
  wdSkeletonResourceHandle m_hSkeleton;
  wdArrayMap<wdHashedString, wdExposedBone> m_Bones; // [ property ]
  wdEnum<wdSkeletonPoseMode> m_PoseMode;             // [ property ]
};
