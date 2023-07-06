#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <RendererCore/Meshes/MeshComponentBase.h>

struct wdPerInstanceData;
struct wdRenderWorldRenderEvent;
class wdInstancedMeshComponent;
struct wdMsgExtractGeometry;
class wdStreamWriter;
class wdStreamReader;

struct WD_RENDERERCORE_DLL wdMeshInstanceData
{
  void SetLocalPosition(wdVec3 vPosition);
  wdVec3 GetLocalPosition() const;

  void SetLocalRotation(wdQuat qRotation);
  wdQuat GetLocalRotation() const;

  void SetLocalScaling(wdVec3 vScaling);
  wdVec3 GetLocalScaling() const;

  wdResult Serialize(wdStreamWriter& ref_writer) const;
  wdResult Deserialize(wdStreamReader& ref_reader);

  wdTransform m_transform;

  wdColor m_color;
};

WD_DECLARE_REFLECTABLE_TYPE(WD_RENDERERCORE_DLL, wdMeshInstanceData);

//////////////////////////////////////////////////////////////////////////

class WD_RENDERERCORE_DLL wdInstancedMeshRenderData : public wdMeshRenderData
{
  WD_ADD_DYNAMIC_REFLECTION(wdInstancedMeshRenderData, wdMeshRenderData);

public:
  virtual void FillBatchIdAndSortingKey() override;

  wdInstanceData* m_pExplicitInstanceData = nullptr;
  wdUInt32 m_uiExplicitInstanceCount = 0;
};

//////////////////////////////////////////////////////////////////////////

class WD_RENDERERCORE_DLL wdInstancedMeshComponentManager : public wdComponentManager<class wdInstancedMeshComponent, wdBlockStorageType::Compact>
{
public:
  using SUPER = wdComponentManager<wdInstancedMeshComponent, wdBlockStorageType::Compact>;

  wdInstancedMeshComponentManager(wdWorld* pWorld);

  void EnqueueUpdate(const wdInstancedMeshComponent* pComponent) const;

private:
  struct ComponentToUpdate
  {
    wdComponentHandle m_hComponent;
    wdArrayPtr<wdPerInstanceData> m_InstanceData;
  };

  mutable wdMutex m_Mutex;
  mutable wdDeque<ComponentToUpdate> m_RequireUpdate;

protected:
  void OnRenderEvent(const wdRenderWorldRenderEvent& e);

  virtual void Initialize() override;
  virtual void Deinitialize() override;
};

class WD_RENDERERCORE_DLL wdInstancedMeshComponent : public wdMeshComponentBase
{
  WD_DECLARE_COMPONENT_TYPE(wdInstancedMeshComponent, wdMeshComponentBase, wdInstancedMeshComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // wdComponent

public:
  virtual void SerializeComponent(wdWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(wdWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // wdRenderComponent

public:
  virtual wdResult GetLocalBounds(wdBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, wdMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // wdMeshComponentBase

protected:
  virtual wdMeshRenderData* CreateRenderData() const override;


  //////////////////////////////////////////////////////////////////////////
  // wdInstancedMeshComponent

public:
  wdInstancedMeshComponent();
  ~wdInstancedMeshComponent();

  /// \brief Extracts the render geometry for export etc.
  void OnMsgExtractGeometry(wdMsgExtractGeometry& ref_msg); // [ msg handler ]

protected:
  void OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const;

  wdUInt32 Instances_GetCount() const;                                 // [ property ]
  wdMeshInstanceData Instances_GetValue(wdUInt32 uiIndex) const;       // [ property ]
  void Instances_SetValue(wdUInt32 uiIndex, wdMeshInstanceData value); // [ property ]
  void Instances_Insert(wdUInt32 uiIndex, wdMeshInstanceData value);   // [ property ]
  void Instances_Remove(wdUInt32 uiIndex);                             // [ property ]

  wdArrayPtr<wdPerInstanceData> GetInstanceData() const;

  // Unpacked, reflected instance data for editing and ease of access
  wdDynamicArray<wdMeshInstanceData> m_RawInstancedData;

  wdInstanceData* m_pExplicitInstanceData = nullptr;

  mutable wdUInt64 m_uiEnqueuedFrame = wdUInt64(-1);
};
