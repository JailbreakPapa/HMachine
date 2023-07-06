#include <RendererCore/RendererCorePCH.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/Texture2DResource.h>


wdCameraComponentManager::wdCameraComponentManager(wdWorld* pWorld)
  : wdComponentManager<wdCameraComponent, wdBlockStorageType::Compact>(pWorld)
{
  wdRenderWorld::s_CameraConfigsModifiedEvent.AddEventHandler(wdMakeDelegate(&wdCameraComponentManager::OnCameraConfigsChanged, this));
}

wdCameraComponentManager::~wdCameraComponentManager()
{
  wdRenderWorld::s_CameraConfigsModifiedEvent.RemoveEventHandler(wdMakeDelegate(&wdCameraComponentManager::OnCameraConfigsChanged, this));
}

void wdCameraComponentManager::Initialize()
{
  auto desc = WD_CREATE_MODULE_UPDATE_FUNCTION_DESC(wdCameraComponentManager::Update, this);
  desc.m_Phase = UpdateFunctionDesc::Phase::PostTransform;

  this->RegisterUpdateFunction(desc);

  wdRenderWorld::s_ViewCreatedEvent.AddEventHandler(wdMakeDelegate(&wdCameraComponentManager::OnViewCreated, this));
}

void wdCameraComponentManager::Deinitialize()
{
  wdRenderWorld::s_ViewCreatedEvent.RemoveEventHandler(wdMakeDelegate(&wdCameraComponentManager::OnViewCreated, this));

  SUPER::Deinitialize();
}

void wdCameraComponentManager::Update(const wdWorldModule::UpdateContext& context)
{
  for (auto hCameraComponent : m_ModifiedCameras)
  {
    wdCameraComponent* pCameraComponent = nullptr;
    if (!TryGetComponent(hCameraComponent, pCameraComponent))
    {
      continue;
    }

    if (wdView* pView = wdRenderWorld::GetViewByUsageHint(pCameraComponent->GetUsageHint(), wdCameraUsageHint::None, GetWorld()))
    {
      pCameraComponent->ApplySettingsToView(pView);
    }

    pCameraComponent->m_bIsModified = false;
  }

  m_ModifiedCameras.Clear();

  for (auto hCameraComponent : m_RenderTargetCameras)
  {
    wdCameraComponent* pCameraComponent = nullptr;
    if (!TryGetComponent(hCameraComponent, pCameraComponent))
    {
      continue;
    }

    pCameraComponent->UpdateRenderTargetCamera();
  }

  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized() && it->m_bShowStats && it->GetUsageHint() == wdCameraUsageHint::MainView)
    {
      if (wdView* pView = wdRenderWorld::GetViewByUsageHint(wdCameraUsageHint::MainView, wdCameraUsageHint::EditorView, GetWorld()))
      {
        it->ShowStats(pView);
      }
    }
  }
}

void wdCameraComponentManager::ReinitializeAllRenderTargetCameras()
{
  WD_LOCK(GetWorld()->GetWriteMarker());

  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized())
    {
      it->DeactivateRenderToTexture();
      it->ActivateRenderToTexture();
    }
  }
}

const wdCameraComponent* wdCameraComponentManager::GetCameraByUsageHint(wdCameraUsageHint::Enum usageHint) const
{
  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized() && it->GetUsageHint() == usageHint)
    {
      return it;
    }
  }

  return nullptr;
}

wdCameraComponent* wdCameraComponentManager::GetCameraByUsageHint(wdCameraUsageHint::Enum usageHint)
{
  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    if (it->IsActiveAndInitialized() && it->GetUsageHint() == usageHint)
    {
      return it;
    }
  }

  return nullptr;
}

void wdCameraComponentManager::AddRenderTargetCamera(wdCameraComponent* pComponent)
{
  m_RenderTargetCameras.PushBack(pComponent->GetHandle());
}

void wdCameraComponentManager::RemoveRenderTargetCamera(wdCameraComponent* pComponent)
{
  m_RenderTargetCameras.RemoveAndSwap(pComponent->GetHandle());
}

void wdCameraComponentManager::OnViewCreated(wdView* pView)
{
  // Mark all cameras as modified so the new view gets the proper settings
  for (auto it = GetComponents(); it.IsValid(); ++it)
  {
    it->MarkAsModified(this);
  }
}

void wdCameraComponentManager::OnCameraConfigsChanged(void* dummy)
{
  ReinitializeAllRenderTargetCameras();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
WD_BEGIN_COMPONENT_TYPE(wdCameraComponent, 10, wdComponentMode::Static)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("EditorShortcut", m_iEditorShortcut)->AddAttributes(new wdDefaultValueAttribute(-1), new wdClampValueAttribute(-1, 9)),
    WD_ENUM_ACCESSOR_PROPERTY("UsageHint", wdCameraUsageHint, GetUsageHint, SetUsageHint),
    WD_ENUM_ACCESSOR_PROPERTY("Mode", wdCameraMode, GetCameraMode, SetCameraMode),
    WD_ACCESSOR_PROPERTY("RenderTarget", GetRenderTargetFile, SetRenderTargetFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Texture_Target", wdDependencyFlags::Package)),
    WD_ACCESSOR_PROPERTY("RenderTargetOffset", GetRenderTargetRectOffset, SetRenderTargetRectOffset)->AddAttributes(new wdClampValueAttribute(wdVec2(0.0f), wdVec2(0.9f))),
    WD_ACCESSOR_PROPERTY("RenderTargetSize", GetRenderTargetRectSize, SetRenderTargetRectSize)->AddAttributes(new wdDefaultValueAttribute(wdVec2(1.0f)), new wdClampValueAttribute(wdVec2(0.1f), wdVec2(1.0f))),
    WD_ACCESSOR_PROPERTY("NearPlane", GetNearPlane, SetNearPlane)->AddAttributes(new wdDefaultValueAttribute(0.25f), new wdClampValueAttribute(0.01f, 4.0f)),
    WD_ACCESSOR_PROPERTY("FarPlane", GetFarPlane, SetFarPlane)->AddAttributes(new wdDefaultValueAttribute(1000.0f), new wdClampValueAttribute(5.0, 10000.0f)),
    WD_ACCESSOR_PROPERTY("FOV", GetFieldOfView, SetFieldOfView)->AddAttributes(new wdDefaultValueAttribute(60.0f), new wdClampValueAttribute(1.0f, 170.0f)),
    WD_ACCESSOR_PROPERTY("Dimensions", GetOrthoDimension, SetOrthoDimension)->AddAttributes(new wdDefaultValueAttribute(10.0f), new wdClampValueAttribute(0.01f, 10000.0f)),
    WD_SET_MEMBER_PROPERTY("IncludeTags", m_IncludeTags)->AddAttributes(new wdTagSetWidgetAttribute("Default")),
    WD_SET_MEMBER_PROPERTY("ExcludeTags", m_ExcludeTags)->AddAttributes(new wdTagSetWidgetAttribute("Default")),
    WD_ACCESSOR_PROPERTY("CameraRenderPipeline", GetRenderPipelineEnum, SetRenderPipelineEnum)->AddAttributes(new wdDynamicStringEnumAttribute("CameraPipelines")),
    WD_ACCESSOR_PROPERTY("Aperture", GetAperture, SetAperture)->AddAttributes(new wdDefaultValueAttribute(1.0f), new wdClampValueAttribute(1.0f, 32.0f), new wdSuffixAttribute(" f-stop(s)")),
    WD_ACCESSOR_PROPERTY("ShutterTime", GetShutterTime, SetShutterTime)->AddAttributes(new wdDefaultValueAttribute(wdTime::Seconds(1.0)), new wdClampValueAttribute(wdTime::Seconds(1.0f / 100000.0f), wdTime::Seconds(600.0f))),
    WD_ACCESSOR_PROPERTY("ISO", GetISO, SetISO)->AddAttributes(new wdDefaultValueAttribute(100.0f), new wdClampValueAttribute(50.0f, 64000.0f)),
    WD_ACCESSOR_PROPERTY("ExposureCompensation", GetExposureCompensation, SetExposureCompensation)->AddAttributes(new wdClampValueAttribute(-32.0f, 32.0f)),
    WD_MEMBER_PROPERTY("ShowStats", m_bShowStats),
    //WD_ACCESSOR_PROPERTY_READ_ONLY("EV100", GetEV100),
    //WD_ACCESSOR_PROPERTY_READ_ONLY("FinalExposure", GetExposure),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Rendering"),
    new wdDirectionVisualizerAttribute(wdBasisAxis::PositiveX, 1.0f, wdColor::DarkSlateBlue),
    new wdCameraVisualizerAttribute("Mode", "FOV", "Dimensions", "NearPlane", "FarPlane"),
  }
  WD_END_ATTRIBUTES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdCameraComponent::wdCameraComponent() = default;
wdCameraComponent::~wdCameraComponent() = default;

void wdCameraComponent::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_UsageHint.GetValue();
  s << m_Mode.GetValue();
  s << m_fNearPlane;
  s << m_fFarPlane;
  s << m_fPerspectiveFieldOfView;
  s << m_fOrthoDimension;

  // Version 2 till 7
  // s << m_hRenderPipeline;

  // Version 3
  s << m_fAperture;
  s << static_cast<float>(m_ShutterTime.GetSeconds());
  s << m_fISO;
  s << m_fExposureCompensation;

  // Version 4
  m_IncludeTags.Save(s);
  m_ExcludeTags.Save(s);

  // Version 6
  s << m_hRenderTarget;

  // Version 7
  s << m_vRenderTargetRectOffset;
  s << m_vRenderTargetRectSize;

  // Version 8
  s << m_sRenderPipeline;

  // Version 10
  s << m_bShowStats;
}

void wdCameraComponent::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const wdUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  wdCameraUsageHint::StorageType usage;
  s >> usage;
  if (uiVersion == 1 && usage > wdCameraUsageHint::MainView)
    usage = wdCameraUsageHint::None;
  m_UsageHint.SetValue(usage);

  wdCameraMode::StorageType cam;
  s >> cam;
  m_Mode.SetValue(cam);

  s >> m_fNearPlane;
  s >> m_fFarPlane;
  s >> m_fPerspectiveFieldOfView;
  s >> m_fOrthoDimension;

  if (uiVersion >= 2 && uiVersion <= 7)
  {
    wdRenderPipelineResourceHandle m_hRenderPipeline;
    s >> m_hRenderPipeline;
  }

  if (uiVersion >= 3)
  {
    s >> m_fAperture;
    float shutterTime;
    s >> shutterTime;
    m_ShutterTime = wdTime::Seconds(shutterTime);
    s >> m_fISO;
    s >> m_fExposureCompensation;
  }

  if (uiVersion >= 4)
  {
    m_IncludeTags.Load(s, wdTagRegistry::GetGlobalRegistry());
    m_ExcludeTags.Load(s, wdTagRegistry::GetGlobalRegistry());
  }

  if (uiVersion >= 6)
  {
    s >> m_hRenderTarget;
  }

  if (uiVersion >= 7)
  {
    s >> m_vRenderTargetRectOffset;
    s >> m_vRenderTargetRectSize;
  }

  if (uiVersion >= 8)
  {
    s >> m_sRenderPipeline;
  }

  if (uiVersion >= 10)
  {
    s >> m_bShowStats;
  }

  MarkAsModified();
}

void wdCameraComponent::UpdateRenderTargetCamera()
{
  if (!m_bRenderTargetInitialized)
    return;

  // recreate everything, if the view got invalidated in between
  if (m_hRenderTargetView.IsInvalidated())
  {
    DeactivateRenderToTexture();
    ActivateRenderToTexture();
  }

  wdView* pView = nullptr;
  if (!wdRenderWorld::TryGetView(m_hRenderTargetView, pView))
    return;

  ApplySettingsToView(pView);

  if (m_Mode == wdCameraMode::PerspectiveFixedFovX || m_Mode == wdCameraMode::PerspectiveFixedFovY)
    m_RenderTargetCamera.SetCameraMode(GetCameraMode(), m_fPerspectiveFieldOfView, m_fNearPlane, m_fFarPlane);
  else
    m_RenderTargetCamera.SetCameraMode(GetCameraMode(), m_fOrthoDimension, m_fNearPlane, m_fFarPlane);

  m_RenderTargetCamera.LookAt(
    GetOwner()->GetGlobalPosition(), GetOwner()->GetGlobalPosition() + GetOwner()->GetGlobalDirForwards(), GetOwner()->GetGlobalDirUp());
}

void wdCameraComponent::ShowStats(wdView* pView)
{
  if (!m_bShowStats)
    return;

  // draw stats
  {
    const wdStringView sName = GetOwner()->GetName();

    wdStringBuilder sb;
    sb.Format("Camera '{0}':\nEV100: {1}, Exposure: {2}", sName.IsEmpty() ? pView->GetName() : sName, GetEV100(), GetExposure());
    wdDebugRenderer::DrawInfoText(pView->GetHandle(), wdDebugRenderer::ScreenPlacement::TopLeft, "CamStats", sb, wdColor::White);
  }

  // draw frustum
  {
    const wdGameObject* pOwner = GetOwner();
    wdVec3 vPosition = pOwner->GetGlobalPosition();
    wdVec3 vForward = pOwner->GetGlobalDirForwards();
    wdVec3 vUp = pOwner->GetGlobalDirUp();

    const wdMat4 viewMatrix = wdGraphicsUtils::CreateLookAtViewMatrix(vPosition, vPosition + vForward, vUp);

    wdMat4 projectionMatrix = pView->GetProjectionMatrix(wdCameraEye::Left); // todo: Stereo support
    wdMat4 viewProjectionMatrix = projectionMatrix * viewMatrix;

    wdFrustum frustum;
    frustum.SetFrustum(viewProjectionMatrix);

    // TODO: limit far plane to 10 meters

    wdDebugRenderer::DrawLineFrustum(GetWorld(), frustum, wdColor::LimeGreen);
  }
}

void wdCameraComponent::SetUsageHint(wdEnum<wdCameraUsageHint> val)
{
  if (val == m_UsageHint)
    return;

  DeactivateRenderToTexture();

  m_UsageHint = val;

  ActivateRenderToTexture();

  MarkAsModified();
}

void wdCameraComponent::SetRenderTargetFile(const char* szFile)
{
  DeactivateRenderToTexture();

  if (!wdStringUtils::IsNullOrEmpty(szFile))
  {
    m_hRenderTarget = wdResourceManager::LoadResource<wdRenderToTexture2DResource>(szFile);
  }
  else
  {
    m_hRenderTarget.Invalidate();
  }

  ActivateRenderToTexture();

  MarkAsModified();
}

const char* wdCameraComponent::GetRenderTargetFile() const
{
  if (!m_hRenderTarget.IsValid())
    return "";

  return m_hRenderTarget.GetResourceID();
}

void wdCameraComponent::SetRenderTargetRectOffset(wdVec2 value)
{
  DeactivateRenderToTexture();

  m_vRenderTargetRectOffset.x = wdMath::Clamp(value.x, 0.0f, 0.9f);
  m_vRenderTargetRectOffset.y = wdMath::Clamp(value.y, 0.0f, 0.9f);

  ActivateRenderToTexture();
}

void wdCameraComponent::SetRenderTargetRectSize(wdVec2 value)
{
  DeactivateRenderToTexture();

  m_vRenderTargetRectSize.x = wdMath::Clamp(value.x, 0.1f, 1.0f);
  m_vRenderTargetRectSize.y = wdMath::Clamp(value.y, 0.1f, 1.0f);

  ActivateRenderToTexture();
}

void wdCameraComponent::SetCameraMode(wdEnum<wdCameraMode> val)
{
  if (val == m_Mode)
    return;
  m_Mode = val;

  MarkAsModified();
}


void wdCameraComponent::SetNearPlane(float fVal)
{
  if (fVal == m_fNearPlane)
    return;
  m_fNearPlane = fVal;

  MarkAsModified();
}


void wdCameraComponent::SetFarPlane(float fVal)
{
  if (fVal == m_fFarPlane)
    return;
  m_fFarPlane = fVal;

  MarkAsModified();
}


void wdCameraComponent::SetFieldOfView(float fVal)
{
  if (fVal == m_fPerspectiveFieldOfView)
    return;
  m_fPerspectiveFieldOfView = fVal;

  MarkAsModified();
}


void wdCameraComponent::SetOrthoDimension(float fVal)
{
  if (fVal == m_fOrthoDimension)
    return;
  m_fOrthoDimension = fVal;

  MarkAsModified();
}

wdRenderPipelineResourceHandle wdCameraComponent::GetRenderPipeline() const
{
  return m_hCachedRenderPipeline;
}

const char* wdCameraComponent::GetRenderPipelineEnum() const
{
  return m_sRenderPipeline.GetData();
}

void wdCameraComponent::SetRenderPipelineEnum(const char* szFile)
{
  DeactivateRenderToTexture();

  m_sRenderPipeline.Assign(szFile);

  ActivateRenderToTexture();

  MarkAsModified();
}

void wdCameraComponent::SetAperture(float fAperture)
{
  if (m_fAperture == fAperture)
    return;
  m_fAperture = fAperture;

  MarkAsModified();
}

void wdCameraComponent::SetShutterTime(wdTime shutterTime)
{
  if (m_ShutterTime == shutterTime)
    return;
  m_ShutterTime = shutterTime;

  MarkAsModified();
}

void wdCameraComponent::SetISO(float fISO)
{
  if (m_fISO == fISO)
    return;
  m_fISO = fISO;

  MarkAsModified();
}

void wdCameraComponent::SetExposureCompensation(float fEC)
{
  if (m_fExposureCompensation == fEC)
    return;
  m_fExposureCompensation = fEC;

  MarkAsModified();
}

float wdCameraComponent::GetEV100() const
{
  // From: course_notes_moving_frostbite_to_pbr.pdf
  // EV number is defined as:
  // 2^ EV_s = N^2 / t and EV_s = EV_100 + log2 (S /100)
  // This gives
  // EV_s = log2 (N^2 / t)
  // EV_100 + log2 (S /100) = log2 (N^2 / t)
  // EV_100 = log2 (N^2 / t) - log2 (S /100)
  // EV_100 = log2 (N^2 / t . 100 / S)
  return wdMath::Log2((m_fAperture * m_fAperture) / m_ShutterTime.AsFloatInSeconds() * 100.0f / m_fISO) - m_fExposureCompensation;
}

float wdCameraComponent::GetExposure() const
{
  // Compute the maximum luminance possible with H_sbs sensitivity
  // maxLum = 78 / ( S * q ) * N^2 / t
  // = 78 / ( S * q ) * 2^ EV_100
  // = 78 / (100 * 0.65) * 2^ EV_100
  // = 1.2 * 2^ EV
  // Reference : http://en.wikipedia.org/wiki/Film_speed
  float maxLuminance = 1.2f * wdMath::Pow2(GetEV100());
  return 1.0f / maxLuminance;
}

void wdCameraComponent::ApplySettingsToView(wdView* pView) const
{
  if (m_UsageHint == wdCameraUsageHint::None)
    return;

  float fFovOrDim = m_fPerspectiveFieldOfView;
  if (m_Mode == wdCameraMode::OrthoFixedWidth || m_Mode == wdCameraMode::OrthoFixedHeight)
  {
    fFovOrDim = m_fOrthoDimension;
  }

  wdCamera* pCamera = pView->GetCamera();
  pCamera->SetCameraMode(m_Mode, fFovOrDim, m_fNearPlane, wdMath::Max(m_fNearPlane + 0.00001f, m_fFarPlane));
  pCamera->SetExposure(GetExposure());

  pView->m_IncludeTags = m_IncludeTags;
  pView->m_ExcludeTags = m_ExcludeTags;

  const wdTag& tagEditor = wdTagRegistry::GetGlobalRegistry().RegisterTag("Editor");
  pView->m_ExcludeTags.Set(tagEditor);

  if (m_hCachedRenderPipeline.IsValid())
  {
    pView->SetRenderPipelineResource(m_hCachedRenderPipeline);
  }
}

void wdCameraComponent::ResourceChangeEventHandler(const wdResourceEvent& e)
{
  switch (e.m_Type)
  {
    case wdResourceEvent::Type::ResourceExists:
    case wdResourceEvent::Type::ResourceCreated:
      return;

    case wdResourceEvent::Type::ResourceDeleted:
    case wdResourceEvent::Type::ResourceContentUnloading:
    case wdResourceEvent::Type::ResourceContentUpdated:
      // triggers a recreation of the view
      wdRenderWorld::DeleteView(m_hRenderTargetView);
      m_hRenderTargetView.Invalidate();
      break;

    default:
      break;
  }
}

void wdCameraComponent::MarkAsModified()
{
  if (!m_bIsModified)
  {
    GetWorld()->GetComponentManager<wdCameraComponentManager>()->m_ModifiedCameras.PushBack(GetHandle());
    m_bIsModified = true;
  }
}


void wdCameraComponent::MarkAsModified(wdCameraComponentManager* pCameraManager)
{
  if (!m_bIsModified)
  {
    pCameraManager->m_ModifiedCameras.PushBack(GetHandle());
    m_bIsModified = true;
  }
}

void wdCameraComponent::ActivateRenderToTexture()
{
  if (m_UsageHint != wdCameraUsageHint::RenderTarget)
    return;

  if (m_bRenderTargetInitialized || !m_hRenderTarget.IsValid() || m_sRenderPipeline.IsEmpty() || !IsActiveAndInitialized())
    return;

  wdResourceLock<wdRenderToTexture2DResource> pRenderTarget(m_hRenderTarget, wdResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pRenderTarget.GetAcquireResult() != wdResourceAcquireResult::Final)
  {
    return;
  }

  // query the render pipeline to use
  if (const auto* pConfig = wdRenderWorld::FindCameraConfig(m_sRenderPipeline))
  {
    m_hCachedRenderPipeline = pConfig->m_hRenderPipeline;
  }

  if (!m_hCachedRenderPipeline.IsValid())
    return;

  m_bRenderTargetInitialized = true;

  WD_ASSERT_DEV(m_hRenderTargetView.IsInvalidated(), "Render target view is already created");

  wdStringBuilder name;
  name.Format("Camera RT: {0}", GetOwner()->GetName());

  wdView* pRenderTargetView = nullptr;
  m_hRenderTargetView = wdRenderWorld::CreateView(name, pRenderTargetView);

  pRenderTargetView->SetRenderPipelineResource(m_hCachedRenderPipeline);

  pRenderTargetView->SetWorld(GetWorld());
  pRenderTargetView->SetCamera(&m_RenderTargetCamera);

  pRenderTarget->m_ResourceEvents.AddEventHandler(wdMakeDelegate(&wdCameraComponent::ResourceChangeEventHandler, this));

  wdGALRenderTargets renderTargets;
  renderTargets.m_hRTs[0] = pRenderTarget->GetGALTexture();
  pRenderTargetView->SetRenderTargets(renderTargets);

  const float maxSizeX = 1.0f - m_vRenderTargetRectOffset.x;
  const float maxSizeY = 1.0f - m_vRenderTargetRectOffset.y;

  const float resX = (float)pRenderTarget->GetWidth();
  const float resY = (float)pRenderTarget->GetHeight();

  const float width = resX * wdMath::Min(maxSizeX, m_vRenderTargetRectSize.x);
  const float height = resY * wdMath::Min(maxSizeY, m_vRenderTargetRectSize.y);

  const float offsetX = m_vRenderTargetRectOffset.x * resX;
  const float offsetY = m_vRenderTargetRectOffset.y * resY;

  pRenderTargetView->SetViewport(wdRectFloat(offsetX, offsetY, width, height));

  pRenderTarget->AddRenderView(m_hRenderTargetView);

  GetWorld()->GetComponentManager<wdCameraComponentManager>()->AddRenderTargetCamera(this);
}

void wdCameraComponent::DeactivateRenderToTexture()
{
  if (!m_bRenderTargetInitialized)
    return;

  m_bRenderTargetInitialized = false;
  m_hCachedRenderPipeline.Invalidate();

  WD_ASSERT_DEBUG(m_hRenderTarget.IsValid(), "Render Target should be valid");

  if (m_hRenderTarget.IsValid())
  {
    wdResourceLock<wdRenderToTexture2DResource> pRenderTarget(m_hRenderTarget, wdResourceAcquireMode::BlockTillLoaded);
    pRenderTarget->RemoveRenderView(m_hRenderTargetView);

    pRenderTarget->m_ResourceEvents.RemoveEventHandler(wdMakeDelegate(&wdCameraComponent::ResourceChangeEventHandler, this));
  }

  if (!m_hRenderTargetView.IsInvalidated())
  {
    wdRenderWorld::DeleteView(m_hRenderTargetView);
    m_hRenderTargetView.Invalidate();
  }

  GetWorld()->GetComponentManager<wdCameraComponentManager>()->RemoveRenderTargetCamera(this);
}

void wdCameraComponent::OnActivated()
{
  SUPER::OnActivated();

  ActivateRenderToTexture();
}

void wdCameraComponent::OnDeactivated()
{
  DeactivateRenderToTexture();

  SUPER::OnDeactivated();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class wdCameraComponentPatch_4_5 : public wdGraphPatch
{
public:
  wdCameraComponentPatch_4_5()
    : wdGraphPatch("wdCameraComponent", 5)
  {
  }

  virtual void Patch(wdGraphPatchContext& ref_context, wdAbstractObjectGraph* pGraph, wdAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Usage Hint", "UsageHint");
    pNode->RenameProperty("Near Plane", "NearPlane");
    pNode->RenameProperty("Far Plane", "FarPlane");
    pNode->RenameProperty("Include Tags", "IncludeTags");
    pNode->RenameProperty("Exclude Tags", "ExcludeTags");
    pNode->RenameProperty("Render Pipeline", "RenderPipeline");
    pNode->RenameProperty("Shutter Time", "ShutterTime");
    pNode->RenameProperty("Exposure Compensation", "ExposureCompensation");
  }
};

wdCameraComponentPatch_4_5 g_wdCameraComponentPatch_4_5;

//////////////////////////////////////////////////////////////////////////

class wdCameraComponentPatch_8_9 : public wdGraphPatch
{
public:
  wdCameraComponentPatch_8_9()
    : wdGraphPatch("wdCameraComponent", 9)
  {
  }

  virtual void Patch(wdGraphPatchContext& ref_context, wdAbstractObjectGraph* pGraph, wdAbstractObjectNode* pNode) const override
  {
    // convert the "ShutterTime" property from float to wdTime
    if (auto pProp = pNode->FindProperty("ShutterTime"))
    {
      if (pProp->m_Value.IsA<float>())
      {
        const float shutterTime = pProp->m_Value.Get<float>();
        pProp->m_Value = wdTime::Seconds(shutterTime);
      }
    }
  }
};

wdCameraComponentPatch_8_9 g_wdCameraComponentPatch_8_9;


WD_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_CameraComponent);
