#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Meshes/InstancedMeshComponent.h>
#include <RendererCore/Pipeline/InstanceDataProvider.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
WD_BEGIN_STATIC_REFLECTED_TYPE(wdMeshInstanceData, wdNoBase, 1, wdRTTIDefaultAllocator<wdMeshInstanceData>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("LocalPosition", GetLocalPosition, SetLocalPosition)->AddAttributes(new wdSuffixAttribute(" m")),
    WD_ACCESSOR_PROPERTY("LocalRotation", GetLocalRotation, SetLocalRotation),
    WD_ACCESSOR_PROPERTY("LocalScaling", GetLocalScaling, SetLocalScaling)->AddAttributes(new wdDefaultValueAttribute(wdVec3(1.0f, 1.0f, 1.0f))),

    WD_MEMBER_PROPERTY("Color", m_color)
  }
  WD_END_PROPERTIES;

  WD_BEGIN_ATTRIBUTES
  {
    new wdTransformManipulatorAttribute("LocalPosition", "LocalRotation", "LocalScaling"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_STATIC_REFLECTED_TYPE
// clang-format on

void wdMeshInstanceData::SetLocalPosition(wdVec3 vPosition)
{
  m_transform.m_vPosition = vPosition;
}
wdVec3 wdMeshInstanceData::GetLocalPosition() const
{
  return m_transform.m_vPosition;
}

void wdMeshInstanceData::SetLocalRotation(wdQuat qRotation)
{
  m_transform.m_qRotation = qRotation;
}

wdQuat wdMeshInstanceData::GetLocalRotation() const
{
  return m_transform.m_qRotation;
}

void wdMeshInstanceData::SetLocalScaling(wdVec3 vScaling)
{
  m_transform.m_vScale = vScaling;
}

wdVec3 wdMeshInstanceData::GetLocalScaling() const
{
  return m_transform.m_vScale;
}

static const wdTypeVersion s_MeshInstanceDataVersion = 1;
wdResult wdMeshInstanceData::Serialize(wdStreamWriter& ref_writer) const
{
  ref_writer.WriteVersion(s_MeshInstanceDataVersion);

  ref_writer << m_transform;
  ref_writer << m_color;

  return WD_SUCCESS;
}

wdResult wdMeshInstanceData::Deserialize(wdStreamReader& ref_reader)
{
  /*auto version = */ ref_reader.ReadVersion(s_MeshInstanceDataVersion);

  ref_reader >> m_transform;
  ref_reader >> m_color;

  return WD_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdInstancedMeshRenderData, 1, wdRTTIDefaultAllocator<wdInstancedMeshRenderData>)
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void wdInstancedMeshRenderData::FillBatchIdAndSortingKey()
{
  FillBatchIdAndSortingKeyInternal(m_pExplicitInstanceData->m_hInstanceDataBuffer.GetInternalID().m_Data);
}

//////////////////////////////////////////////////////////////////////////////////////

wdInstancedMeshComponentManager::wdInstancedMeshComponentManager(wdWorld* pWorld)
  : SUPER(pWorld)
{
}

void wdInstancedMeshComponentManager::EnqueueUpdate(const wdInstancedMeshComponent* pComponent) const
{
  wdUInt64 uiCurrentFrame = wdRenderWorld::GetFrameCounter();
  if (pComponent->m_uiEnqueuedFrame == uiCurrentFrame)
    return;

  WD_LOCK(m_Mutex);
  if (pComponent->m_uiEnqueuedFrame == uiCurrentFrame)
    return;

  auto instanceData = pComponent->GetInstanceData();
  if (instanceData.IsEmpty())
    return;

  m_RequireUpdate.PushBack({pComponent->GetHandle(), instanceData});
  pComponent->m_uiEnqueuedFrame = uiCurrentFrame;
}

void wdInstancedMeshComponentManager::OnRenderEvent(const wdRenderWorldRenderEvent& e)
{
  if (e.m_Type != wdRenderWorldRenderEvent::Type::BeginRender)
    return;

  WD_LOCK(m_Mutex);

  if (m_RequireUpdate.IsEmpty())
    return;

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();
  wdGALPass* pGALPass = pDevice->BeginPass("Update Instanced Mesh Data");

  wdRenderContext* pRenderContext = wdRenderContext::GetDefaultInstance();
  pRenderContext->BeginCompute(pGALPass);

  for (const auto& componentToUpdate : m_RequireUpdate)
  {
    wdInstancedMeshComponent* pComp = nullptr;
    if (!TryGetComponent(componentToUpdate.m_hComponent, pComp))
      continue;

    if (pComp->m_pExplicitInstanceData)
    {
      wdUInt32 uiOffset = 0;
      auto instanceData = pComp->m_pExplicitInstanceData->GetInstanceData(componentToUpdate.m_InstanceData.GetCount(), uiOffset);
      instanceData.CopyFrom(componentToUpdate.m_InstanceData);

      pComp->m_pExplicitInstanceData->UpdateInstanceData(pRenderContext, instanceData.GetCount());
    }
  }

  pRenderContext->EndCompute();
  pDevice->EndPass(pGALPass);

  m_RequireUpdate.Clear();
}

void wdInstancedMeshComponentManager::Initialize()
{
  SUPER::Initialize();

  wdRenderWorld::GetRenderEvent().AddEventHandler(wdMakeDelegate(&wdInstancedMeshComponentManager::OnRenderEvent, this));
}

void wdInstancedMeshComponentManager::Deinitialize()
{
  wdRenderWorld::GetRenderEvent().RemoveEventHandler(wdMakeDelegate(&wdInstancedMeshComponentManager::OnRenderEvent, this));

  SUPER::Deinitialize();
}

//////////////////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_COMPONENT_TYPE(wdInstancedMeshComponent, 1, wdComponentMode::Static)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("Mesh", GetMeshFile, SetMeshFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Mesh_Static")),
    WD_ACCESSOR_PROPERTY("MainColor", GetColor, SetColor)->AddAttributes(new wdExposeColorAlphaAttribute()),
    WD_ARRAY_ACCESSOR_PROPERTY("Materials", Materials_GetCount, Materials_GetValue, Materials_SetValue, Materials_Insert, Materials_Remove)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Material")),

    WD_ARRAY_ACCESSOR_PROPERTY("InstanceData", Instances_GetCount, Instances_GetValue, Instances_SetValue, Instances_Insert, Instances_Remove),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_MESSAGEHANDLERS
  {
    WD_MESSAGE_HANDLER(wdMsgExtractGeometry, OnMsgExtractGeometry),
    WD_MESSAGE_HANDLER(wdMsgExtractRenderData, OnMsgExtractRenderData),
  }
  WD_END_MESSAGEHANDLERS;
}
WD_END_COMPONENT_TYPE
// clang-format on

wdInstancedMeshComponent::wdInstancedMeshComponent() = default;
wdInstancedMeshComponent::~wdInstancedMeshComponent() = default;

void wdInstancedMeshComponent::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  inout_stream.GetStream().WriteArray(m_RawInstancedData).IgnoreResult();
}

void wdInstancedMeshComponent::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  inout_stream.GetStream().ReadArray(m_RawInstancedData).IgnoreResult();
}

void wdInstancedMeshComponent::OnActivated()
{
  SUPER::OnActivated();

  WD_ASSERT_DEV(m_pExplicitInstanceData == nullptr, "Instance data must not be initialized at this point");
  m_pExplicitInstanceData = WD_DEFAULT_NEW(wdInstanceData);
}

void wdInstancedMeshComponent::OnDeactivated()
{
  WD_DEFAULT_DELETE(m_pExplicitInstanceData);
  m_pExplicitInstanceData = nullptr;

  SUPER::OnDeactivated();
}

void wdInstancedMeshComponent::OnMsgExtractGeometry(wdMsgExtractGeometry& ref_msg) {}

wdResult wdInstancedMeshComponent::GetLocalBounds(wdBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, wdMsgUpdateLocalBounds& ref_msg)
{
  wdBoundingBoxSphere singleBounds;
  if (m_hMesh.IsValid())
  {
    wdResourceLock<wdMeshResource> pMesh(m_hMesh, wdResourceAcquireMode::AllowLoadingFallback);
    singleBounds = pMesh->GetBounds();

    for (const auto& instance : m_RawInstancedData)
    {
      auto instanceBounds = singleBounds;
      instanceBounds.Transform(instance.m_transform.GetAsMat4());

      ref_bounds.ExpandToInclude(instanceBounds);
    }

    return WD_SUCCESS;
  }

  return WD_FAILURE;
}

void wdInstancedMeshComponent::OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const
{
  if (!m_hMesh.IsValid())
    return;

  SUPER::OnMsgExtractRenderData(msg);

  static_cast<const wdInstancedMeshComponentManager*>(GetOwningManager())->EnqueueUpdate(this);
}

wdMeshRenderData* wdInstancedMeshComponent::CreateRenderData() const
{
  auto pRenderData = wdCreateRenderDataForThisFrame<wdInstancedMeshRenderData>(GetOwner());

  if (m_pExplicitInstanceData)
  {
    pRenderData->m_pExplicitInstanceData = m_pExplicitInstanceData;
    pRenderData->m_uiExplicitInstanceCount = m_RawInstancedData.GetCount();
  }

  return pRenderData;
}

wdUInt32 wdInstancedMeshComponent::Instances_GetCount() const
{
  return m_RawInstancedData.GetCount();
}

wdMeshInstanceData wdInstancedMeshComponent::Instances_GetValue(wdUInt32 uiIndex) const
{
  return m_RawInstancedData[uiIndex];
}

void wdInstancedMeshComponent::Instances_SetValue(wdUInt32 uiIndex, wdMeshInstanceData value)
{
  m_RawInstancedData[uiIndex] = value;

  TriggerLocalBoundsUpdate();
}

void wdInstancedMeshComponent::Instances_Insert(wdUInt32 uiIndex, wdMeshInstanceData value)
{
  m_RawInstancedData.Insert(value, uiIndex);

  TriggerLocalBoundsUpdate();
}

void wdInstancedMeshComponent::Instances_Remove(wdUInt32 uiIndex)
{
  m_RawInstancedData.RemoveAtAndCopy(uiIndex);

  TriggerLocalBoundsUpdate();
}

wdArrayPtr<wdPerInstanceData> wdInstancedMeshComponent::GetInstanceData() const
{
  if (!m_pExplicitInstanceData || m_RawInstancedData.IsEmpty())
    return wdArrayPtr<wdPerInstanceData>();

  auto instanceData = WD_NEW_ARRAY(wdFrameAllocator::GetCurrentAllocator(), wdPerInstanceData, m_RawInstancedData.GetCount());

  const wdTransform ownerTransform = GetOwner()->GetGlobalTransform();

  float fBoundingSphereRadius = 1.0f;

  if (m_hMesh.IsValid())
  {
    wdResourceLock<wdMeshResource> pMesh(m_hMesh, wdResourceAcquireMode::AllowLoadingFallback);
    fBoundingSphereRadius = pMesh->GetBounds().GetSphere().m_fRadius;
  }

  for (wdUInt32 i = 0; i < m_RawInstancedData.GetCount(); ++i)
  {
    const wdTransform globalTransform = ownerTransform * m_RawInstancedData[i].m_transform;
    const wdMat4 objectToWorld = globalTransform.GetAsMat4();

    instanceData[i].ObjectToWorld = objectToWorld;

    if (m_RawInstancedData[i].m_transform.ContainsUniformScale())
    {
      instanceData[i].ObjectToWorldNormal = objectToWorld;
    }
    else
    {
      wdMat3 mInverse = objectToWorld.GetRotationalPart();
      mInverse.Invert(0.0f).IgnoreResult();
      // we explicitly ignore the return value here (success / failure)
      // because when we have a scale of 0 (which happens temporarily during editing) that would be annoying

      wdShaderTransform shaderT;
      shaderT = mInverse.GetTranspose();
      instanceData[i].ObjectToWorldNormal = shaderT;
    }

    instanceData[i].GameObjectID = GetUniqueIdForRendering();
    instanceData[i].BoundingSphereRadius = fBoundingSphereRadius * m_RawInstancedData[i].m_transform.GetMaxScale();

    instanceData[i].Color = m_Color * m_RawInstancedData[i].m_color;
  }

  return instanceData;
}


WD_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_InstancedMeshComponent);
