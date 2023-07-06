#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Utilities/Progress.h>
#include <RendererCore/BakedProbes/BakedProbesComponent.h>
#include <RendererCore/BakedProbes/BakedProbesWorldModule.h>
#include <RendererCore/BakedProbes/ProbeTreeSectorResource.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/Pass.h>
#include <RendererFoundation/Resources/Texture.h>

struct wdBakedProbesComponent::RenderDebugViewTask : public wdTask
{
  RenderDebugViewTask()
  {
    ConfigureTask("BakingDebugView", wdTaskNesting::Never);
  }

  virtual void Execute() override
  {
    WD_ASSERT_DEV(m_PixelData.GetCount() == m_uiWidth * m_uiHeight, "Pixel data must be pre-allocated");

    wdProgress progress;
    progress.m_Events.AddEventHandler([this](const wdProgressEvent& e) {
      if (e.m_Type != wdProgressEvent::Type::CancelClicked)
      {
        if (HasBeenCanceled())
        {
          e.m_pProgressbar->UserClickedCancel();
        }
        m_bHasNewData = true;
      }
    });

    if (m_pBakingInterface->RenderDebugView(*m_pWorld, m_InverseViewProjection, m_uiWidth, m_uiHeight, m_PixelData, progress).Succeeded())
    {
      m_bHasNewData = true;
    }
  }

  wdBakingInterface* m_pBakingInterface = nullptr;

  const wdWorld* m_pWorld = nullptr;
  wdMat4 m_InverseViewProjection = wdMat4::IdentityMatrix();
  wdUInt32 m_uiWidth = 0;
  wdUInt32 m_uiHeight = 0;
  wdDynamicArray<wdColorGammaUB> m_PixelData;

  bool m_bHasNewData = false;
};

//////////////////////////////////////////////////////////////////////////


wdBakedProbesComponentManager::wdBakedProbesComponentManager(wdWorld* pWorld)
  : wdSettingsComponentManager<wdBakedProbesComponent>(pWorld)
{
}

wdBakedProbesComponentManager::~wdBakedProbesComponentManager() = default;

void wdBakedProbesComponentManager::Initialize()
{
  {
    auto desc = WD_CREATE_MODULE_UPDATE_FUNCTION_DESC(wdBakedProbesComponentManager::RenderDebug, this);

    this->RegisterUpdateFunction(desc);
  }

  wdRenderWorld::GetRenderEvent().AddEventHandler(wdMakeDelegate(&wdBakedProbesComponentManager::OnRenderEvent, this));

  CreateDebugResources();
}

void wdBakedProbesComponentManager::Deinitialize()
{
  wdRenderWorld::GetRenderEvent().RemoveEventHandler(wdMakeDelegate(&wdBakedProbesComponentManager::OnRenderEvent, this));
}

void wdBakedProbesComponentManager::RenderDebug(const wdWorldModule::UpdateContext& updateContext)
{
  if (wdBakedProbesComponent* pComponent = GetSingletonComponent())
  {
    if (pComponent->GetShowDebugOverlay())
    {
      pComponent->RenderDebugOverlay();
    }
  }
}

void wdBakedProbesComponentManager::OnRenderEvent(const wdRenderWorldRenderEvent& e)
{
  if (e.m_Type != wdRenderWorldRenderEvent::Type::BeginRender)
    return;

  if (wdBakedProbesComponent* pComponent = GetSingletonComponent())
  {
    auto& task = pComponent->m_pRenderDebugViewTask;
    if (task != nullptr && task->m_bHasNewData)
    {
      task->m_bHasNewData = false;

      wdGALDevice* pGALDevice = wdGALDevice::GetDefaultDevice();
      wdGALPass* pGALPass = pGALDevice->BeginPass("BakingDebugViewUpdate");
      auto pCommandEncoder = pGALPass->BeginCompute();

      wdBoundingBoxu32 destBox;
      destBox.m_vMin.SetZero();
      destBox.m_vMax = wdVec3U32(task->m_uiWidth, task->m_uiHeight, 1);

      wdGALSystemMemoryDescription sourceData;
      sourceData.m_pData = task->m_PixelData.GetData();
      sourceData.m_uiRowPitch = task->m_uiWidth * sizeof(wdColorGammaUB);

      pCommandEncoder->UpdateTexture(pComponent->m_hDebugViewTexture, wdGALTextureSubresource(), destBox, sourceData);

      pGALPass->EndCompute(pCommandEncoder);
      pGALDevice->EndPass(pGALPass);
    }
  }
}

void wdBakedProbesComponentManager::CreateDebugResources()
{
  if (!m_hDebugSphere.IsValid())
  {
    wdGeometry geom;
    geom.AddSphere(0.3f, 32, 16);

    const char* szBufferResourceName = "IrradianceProbeDebugSphereBuffer";
    wdMeshBufferResourceHandle hMeshBuffer = wdResourceManager::GetExistingResource<wdMeshBufferResource>(szBufferResourceName);
    if (!hMeshBuffer.IsValid())
    {
      wdMeshBufferResourceDescriptor desc;
      desc.AddStream(wdGALVertexAttributeSemantic::Position, wdGALResourceFormat::XYZFloat);
      desc.AddStream(wdGALVertexAttributeSemantic::Normal, wdGALResourceFormat::XYZFloat);
      desc.AllocateStreamsFromGeometry(geom, wdGALPrimitiveTopology::Triangles);

      hMeshBuffer = wdResourceManager::GetOrCreateResource<wdMeshBufferResource>(szBufferResourceName, std::move(desc), szBufferResourceName);
    }

    const char* szMeshResourceName = "IrradianceProbeDebugSphere";
    m_hDebugSphere = wdResourceManager::GetExistingResource<wdMeshResource>(szMeshResourceName);
    if (!m_hDebugSphere.IsValid())
    {
      wdMeshResourceDescriptor desc;
      desc.UseExistingMeshBuffer(hMeshBuffer);
      desc.AddSubMesh(geom.CalculateTriangleCount(), 0, 0);
      desc.ComputeBounds();

      m_hDebugSphere = wdResourceManager::GetOrCreateResource<wdMeshResource>(szMeshResourceName, std::move(desc), szMeshResourceName);
    }
  }

  if (!m_hDebugMaterial.IsValid())
  {
    m_hDebugMaterial = wdResourceManager::LoadResource<wdMaterialResource>(
      "{ 4d15c716-a8e9-43d4-9424-43174403fb94 }"); // IrradianceProbeVisualization.wdMaterialAsset
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_COMPONENT_TYPE(wdBakedProbesComponent, 1, wdComponentMode::Static)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("Settings", m_Settings),
    WD_ACCESSOR_PROPERTY("ShowDebugOverlay", GetShowDebugOverlay, SetShowDebugOverlay)->AddAttributes(new wdGroupAttribute("Debug")),
    WD_ACCESSOR_PROPERTY("ShowDebugProbes", GetShowDebugProbes, SetShowDebugProbes),
    WD_ACCESSOR_PROPERTY("UseTestPosition", GetUseTestPosition, SetUseTestPosition),
    WD_ACCESSOR_PROPERTY("TestPosition", GetTestPosition, SetTestPosition)
  }
  WD_END_PROPERTIES;
  WD_BEGIN_MESSAGEHANDLERS
  {
    WD_MESSAGE_HANDLER(wdMsgUpdateLocalBounds, OnUpdateLocalBounds),
    WD_MESSAGE_HANDLER(wdMsgExtractRenderData, OnExtractRenderData),
  }
  WD_END_MESSAGEHANDLERS;
  WD_BEGIN_FUNCTIONS
  {
    WD_FUNCTION_PROPERTY(OnObjectCreated),
  }
  WD_END_FUNCTIONS;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Rendering/Baking"),
    new wdLongOpAttribute("wdLongOpProxy_BakeScene"),
    new wdTransformManipulatorAttribute("TestPosition"),
    new wdInDevelopmentAttribute(wdInDevelopmentAttribute::Phase::Beta),
  }
  WD_END_ATTRIBUTES;
}
WD_END_COMPONENT_TYPE
// clang-format on

wdBakedProbesComponent::wdBakedProbesComponent() = default;
wdBakedProbesComponent::~wdBakedProbesComponent() = default;

void wdBakedProbesComponent::OnActivated()
{
  auto pModule = GetWorld()->GetOrCreateModule<wdBakedProbesWorldModule>();
  pModule->SetProbeTreeResourcePrefix(m_sProbeTreeResourcePrefix);

  GetOwner()->UpdateLocalBounds();

  SUPER::OnActivated();
}

void wdBakedProbesComponent::OnDeactivated()
{
  if (m_pRenderDebugViewTask != nullptr)
  {
    wdTaskSystem::CancelTask(m_pRenderDebugViewTask).IgnoreResult();
  }

  GetOwner()->UpdateLocalBounds();

  SUPER::OnDeactivated();
}

void wdBakedProbesComponent::SetShowDebugOverlay(bool bShow)
{
  m_bShowDebugOverlay = bShow;

  if (bShow && m_pRenderDebugViewTask == nullptr)
  {
    m_pRenderDebugViewTask = WD_DEFAULT_NEW(RenderDebugViewTask);
  }
}

void wdBakedProbesComponent::SetShowDebugProbes(bool bShow)
{
  if (m_bShowDebugProbes != bShow)
  {
    m_bShowDebugProbes = bShow;

    if (IsActiveAndInitialized())
    {
      wdRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
    }
  }
}

void wdBakedProbesComponent::SetUseTestPosition(bool bUse)
{
  if (m_bUseTestPosition != bUse)
  {
    m_bUseTestPosition = bUse;

    if (IsActiveAndInitialized())
    {
      wdRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
    }
  }
}

void wdBakedProbesComponent::SetTestPosition(const wdVec3& vPos)
{
  m_vTestPosition = vPos;

  if (IsActiveAndInitialized())
  {
    wdRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

void wdBakedProbesComponent::OnUpdateLocalBounds(wdMsgUpdateLocalBounds& ref_msg)
{
  ref_msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? wdDefaultSpatialDataCategories::RenderDynamic : wdDefaultSpatialDataCategories::RenderStatic);
}

void wdBakedProbesComponent::OnExtractRenderData(wdMsgExtractRenderData& ref_msg) const
{
  if (!m_bShowDebugProbes)
    return;

  // Don't trigger probe rendering in shadow or reflection views.
  if (ref_msg.m_pView->GetCameraUsageHint() == wdCameraUsageHint::Shadow ||
      ref_msg.m_pView->GetCameraUsageHint() == wdCameraUsageHint::Reflection)
    return;

  auto pModule = GetWorld()->GetModule<wdBakedProbesWorldModule>();
  if (!pModule->HasProbeData())
    return;

  const wdGameObject* pOwner = GetOwner();
  auto pManager = static_cast<const wdBakedProbesComponentManager*>(GetOwningManager());

  auto addProbeRenderData = [&](const wdVec3& vPosition, wdCompressedSkyVisibility skyVisibility, wdRenderData::Caching::Enum caching) {
    wdTransform transform = wdTransform::IdentityTransform();
    transform.m_vPosition = vPosition;

    wdColor encodedSkyVisibility = wdColor::Black;
    encodedSkyVisibility.r = *reinterpret_cast<const float*>(&skyVisibility);

    wdMeshRenderData* pRenderData = wdCreateRenderDataForThisFrame<wdMeshRenderData>(pOwner);
    {
      pRenderData->m_GlobalTransform = transform;
      pRenderData->m_GlobalBounds.SetInvalid();
      pRenderData->m_hMesh = pManager->m_hDebugSphere;
      pRenderData->m_hMaterial = pManager->m_hDebugMaterial;
      pRenderData->m_Color = encodedSkyVisibility;
      pRenderData->m_uiSubMeshIndex = 0;
      pRenderData->m_uiUniqueID = wdRenderComponent::GetUniqueIdForRendering(this, 0);

      pRenderData->FillBatchIdAndSortingKey();
    }

    ref_msg.AddRenderData(pRenderData, wdDefaultRenderDataCategories::SimpleOpaque, caching);
  };

  if (m_bUseTestPosition)
  {
    wdBakedProbesWorldModule::ProbeIndexData indexData;
    if (pModule->GetProbeIndexData(m_vTestPosition, wdVec3::UnitZAxis(), indexData).Failed())
      return;

    if (true)
    {
      wdResourceLock<wdProbeTreeSectorResource> pProbeTree(pModule->m_hProbeTree, wdResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pProbeTree.GetAcquireResult() != wdResourceAcquireResult::Final)
        return;

      for (wdUInt32 i = 0; i < WD_ARRAY_SIZE(indexData.m_probeIndices); ++i)
      {
        wdVec3 pos = pProbeTree->GetProbePositions()[indexData.m_probeIndices[i]];
        wdDebugRenderer::DrawCross(ref_msg.m_pView->GetHandle(), pos, 0.5f, wdColor::Yellow);

        pos.z += 0.5f;
        wdDebugRenderer::Draw3DText(ref_msg.m_pView->GetHandle(), wdFmt("Weight: {}", indexData.m_probeWeights[i]), pos, wdColor::Yellow);
      }
    }

    wdCompressedSkyVisibility skyVisibility = wdBakingUtils::CompressSkyVisibility(pModule->GetSkyVisibility(indexData));

    addProbeRenderData(m_vTestPosition, skyVisibility, wdRenderData::Caching::Never);
  }
  else
  {
    wdResourceLock<wdProbeTreeSectorResource> pProbeTree(pModule->m_hProbeTree, wdResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pProbeTree.GetAcquireResult() != wdResourceAcquireResult::Final)
      return;

    auto probePositions = pProbeTree->GetProbePositions();
    auto skyVisibility = pProbeTree->GetSkyVisibility();

    for (wdUInt32 uiProbeIndex = 0; uiProbeIndex < probePositions.GetCount(); ++uiProbeIndex)
    {
      addProbeRenderData(probePositions[uiProbeIndex], skyVisibility[uiProbeIndex], wdRenderData::Caching::IfStatic);
    }
  }
}

void wdBakedProbesComponent::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  wdStreamWriter& s = inout_stream.GetStream();

  if (m_Settings.Serialize(s).Failed())
    return;

  s << m_sProbeTreeResourcePrefix;
  s << m_bShowDebugOverlay;
  s << m_bShowDebugProbes;
  s << m_bUseTestPosition;
  s << m_vTestPosition;
}

void wdBakedProbesComponent::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const wdUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  wdStreamReader& s = inout_stream.GetStream();

  if (m_Settings.Deserialize(s).Failed())
    return;

  s >> m_sProbeTreeResourcePrefix;
  s >> m_bShowDebugOverlay;
  s >> m_bShowDebugProbes;
  s >> m_bUseTestPosition;
  s >> m_vTestPosition;
}

void wdBakedProbesComponent::RenderDebugOverlay()
{
  wdView* pView = wdRenderWorld::GetViewByUsageHint(wdCameraUsageHint::MainView, wdCameraUsageHint::EditorView);
  if (pView == nullptr)
    return;

  wdBakingInterface* pBakingInterface = wdSingletonRegistry::GetSingletonInstance<wdBakingInterface>();
  if (pBakingInterface == nullptr)
  {
    wdDebugRenderer::Draw2DText(pView->GetHandle(), "Baking Plugin not loaded", wdVec2I32(10, 10), wdColor::OrangeRed);
    return;
  }

  wdGALDevice* pDevice = wdGALDevice::GetDefaultDevice();

  wdRectFloat viewport = pView->GetViewport();
  wdUInt32 uiWidth = static_cast<wdUInt32>(wdMath::Ceil(viewport.width / 3.0f));
  wdUInt32 uiHeight = static_cast<wdUInt32>(wdMath::Ceil(viewport.height / 3.0f));

  wdMat4 inverseViewProjection = pView->GetInverseViewProjectionMatrix(wdCameraEye::Left);

  if (m_pRenderDebugViewTask->m_InverseViewProjection != inverseViewProjection ||
      m_pRenderDebugViewTask->m_uiWidth != uiWidth || m_pRenderDebugViewTask->m_uiHeight != uiHeight)
  {
    wdTaskSystem::CancelTask(m_pRenderDebugViewTask).IgnoreResult();

    m_pRenderDebugViewTask->m_pBakingInterface = pBakingInterface;
    m_pRenderDebugViewTask->m_pWorld = GetWorld();
    m_pRenderDebugViewTask->m_InverseViewProjection = inverseViewProjection;
    m_pRenderDebugViewTask->m_uiWidth = uiWidth;
    m_pRenderDebugViewTask->m_uiHeight = uiHeight;
    m_pRenderDebugViewTask->m_PixelData.SetCount(uiWidth * uiHeight, wdColor::Red);
    m_pRenderDebugViewTask->m_bHasNewData = false;

    wdTaskSystem::StartSingleTask(m_pRenderDebugViewTask, wdTaskPriority::LongRunning);
  }

  wdUInt32 uiTextureWidth = 0;
  wdUInt32 uiTextureHeight = 0;
  if (const wdGALTexture* pTexture = pDevice->GetTexture(m_hDebugViewTexture))
  {
    uiTextureWidth = pTexture->GetDescription().m_uiWidth;
    uiTextureHeight = pTexture->GetDescription().m_uiHeight;
  }

  if (uiTextureWidth != uiWidth || uiTextureHeight != uiHeight)
  {
    if (!m_hDebugViewTexture.IsInvalidated())
    {
      pDevice->DestroyTexture(m_hDebugViewTexture);
    }

    wdGALTextureCreationDescription desc;
    desc.m_uiWidth = uiWidth;
    desc.m_uiHeight = uiHeight;
    desc.m_Format = wdGALResourceFormat::RGBAUByteNormalizedsRGB;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hDebugViewTexture = pDevice->CreateTexture(desc);
  }

  wdRectFloat rectInPixel = wdRectFloat(10.0f, 10.0f, static_cast<float>(uiWidth), static_cast<float>(uiHeight));

  wdDebugRenderer::Draw2DRectangle(pView->GetHandle(), rectInPixel, 0.0f, wdColor::White, pDevice->GetDefaultResourceView(m_hDebugViewTexture));
}

void wdBakedProbesComponent::OnObjectCreated(const wdAbstractObjectNode& node)
{
  wdStringBuilder sPrefix;
  sPrefix.Format(":project/AssetCache/Generated/{0}", node.GetGuid());

  m_sProbeTreeResourcePrefix.Assign(sPrefix);
}


WD_STATICLINK_FILE(RendererCore, RendererCore_BakedProbes_Implementation_BakedProbesComponent);
