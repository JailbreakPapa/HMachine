#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Math/Frustum.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
WD_BEGIN_STATIC_REFLECTED_ENUM(wdCameraUsageHint, 1)
  WD_ENUM_CONSTANT(wdCameraUsageHint::None),
  WD_ENUM_CONSTANT(wdCameraUsageHint::MainView),
  WD_ENUM_CONSTANT(wdCameraUsageHint::EditorView),
  WD_ENUM_CONSTANT(wdCameraUsageHint::RenderTarget),
  WD_ENUM_CONSTANT(wdCameraUsageHint::Culling),
  WD_ENUM_CONSTANT(wdCameraUsageHint::Thumbnail),
WD_END_STATIC_REFLECTED_ENUM;

WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdView, 1, wdRTTINoAllocator)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("RenderTarget0", m_PinRenderTarget0),
    WD_MEMBER_PROPERTY("RenderTarget1", m_PinRenderTarget1),
    WD_MEMBER_PROPERTY("RenderTarget2", m_PinRenderTarget2),
    WD_MEMBER_PROPERTY("RenderTarget3", m_PinRenderTarget3),
    WD_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
  }
  WD_END_PROPERTIES;
}
WD_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

wdView::wdView()
{
  m_pExtractTask = WD_DEFAULT_NEW(wdDelegateTask<void>, "", wdMakeDelegate(&wdView::ExtractData, this));
}

wdView::~wdView() = default;

void wdView::SetName(wdStringView sName)
{
  m_sName.Assign(sName);

  wdStringBuilder sb = sName;
  sb.Append(".ExtractData");
  m_pExtractTask->ConfigureTask(sb, wdTaskNesting::Maybe);
}

void wdView::SetWorld(wdWorld* pWorld)
{
  if (m_pWorld != pWorld)
  {
    m_pWorld = pWorld;

    wdRenderWorld::ResetRenderDataCache(*this);
  }
}

void wdView::SetSwapChain(wdGALSwapChainHandle hSwapChain)
{
  if (m_Data.m_hSwapChain != hSwapChain)
  {
    // Swap chain and render target setup are mutually exclusive.
    m_Data.m_hSwapChain = hSwapChain;
    m_Data.m_renderTargets = wdGALRenderTargets();
    if (m_pRenderPipeline)
    {
      wdRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());
    }
  }
}

void wdView::SetRenderTargets(const wdGALRenderTargets& renderTargets)
{
  if (m_Data.m_renderTargets != renderTargets)
  {
    // Swap chain and render target setup are mutually exclusive.
    m_Data.m_hSwapChain = wdGALSwapChainHandle();
    m_Data.m_renderTargets = renderTargets;
    if (m_pRenderPipeline)
    {
      wdRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());
    }
  }
}

const wdGALRenderTargets& wdView::GetActiveRenderTargets() const
{
  if (const wdGALSwapChain* pSwapChain = wdGALDevice::GetDefaultDevice()->GetSwapChain(m_Data.m_hSwapChain))
  {
    return pSwapChain->GetRenderTargets();
  }
  return m_Data.m_renderTargets;
}

void wdView::SetRenderPipelineResource(wdRenderPipelineResourceHandle hPipeline)
{
  if (hPipeline == m_hRenderPipeline)
  {
    return;
  }

  m_uiRenderPipelineResourceDescriptionCounter = 0;
  m_hRenderPipeline = hPipeline;

  if (m_pRenderPipeline == nullptr)
  {
    EnsureUpToDate();
  }
}

wdRenderPipelineResourceHandle wdView::GetRenderPipelineResource() const
{
  return m_hRenderPipeline;
}

void wdView::SetCameraUsageHint(wdEnum<wdCameraUsageHint> val)
{
  m_Data.m_CameraUsageHint = val;
}

void wdView::SetViewRenderMode(wdEnum<wdViewRenderMode> value)
{
  m_Data.m_ViewRenderMode = value;
}

void wdView::SetViewport(const wdRectFloat& viewport)
{
  m_Data.m_ViewPortRect = viewport;

  UpdateViewData(wdRenderWorld::GetDataIndexForExtraction());
}

void wdView::ForceUpdate()
{
  if (m_pRenderPipeline)
  {
    wdRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());
  }
}

void wdView::ExtractData()
{
  WD_ASSERT_DEV(IsValid(), "Cannot extract data from an invalid view");

  wdRenderWorldExtractionEvent extractionEvent;
  extractionEvent.m_Type = wdRenderWorldExtractionEvent::Type::BeforeViewExtraction;
  extractionEvent.m_pView = this;
  extractionEvent.m_uiFrameCounter = wdRenderWorld::GetFrameCounter();
  wdRenderWorld::s_ExtractionEvent.Broadcast(extractionEvent);


  m_pRenderPipeline->m_sName = m_sName;
  m_pRenderPipeline->ExtractData(*this);

  extractionEvent.m_Type = wdRenderWorldExtractionEvent::Type::AfterViewExtraction;
  wdRenderWorld::s_ExtractionEvent.Broadcast(extractionEvent);
}

void wdView::ComputeCullingFrustum(wdFrustum& out_frustum) const
{
  const wdCamera* pCamera = GetCullingCamera();
  const float fViewportAspectRatio = m_Data.m_ViewPortRect.width / m_Data.m_ViewPortRect.height;

  wdMat4 viewMatrix = pCamera->GetViewMatrix();

  wdMat4 projectionMatrix;
  pCamera->GetProjectionMatrix(fViewportAspectRatio, projectionMatrix);

  out_frustum.SetFrustum(projectionMatrix * viewMatrix);
}

void wdView::SetShaderPermutationVariable(const char* szName, const char* szValue)
{
  wdHashedString sName;
  sName.Assign(szName);

  for (auto& var : m_PermutationVars)
  {
    if (var.m_sName == sName)
    {
      if (var.m_sValue != szValue)
      {
        var.m_sValue.Assign(szValue);
        m_bPermutationVarsDirty = true;
      }
      return;
    }
  }

  auto& var = m_PermutationVars.ExpandAndGetRef();
  var.m_sName = sName;
  var.m_sValue.Assign(szValue);
  m_bPermutationVarsDirty = true;
}

void wdView::SetRenderPassProperty(const char* szPassName, const char* szPropertyName, const wdVariant& value)
{
  SetProperty(m_PassProperties, szPassName, szPropertyName, value);
}

void wdView::SetExtractorProperty(const char* szPassName, const char* szPropertyName, const wdVariant& value)
{
  SetProperty(m_ExtractorProperties, szPassName, szPropertyName, value);
}

void wdView::SetRenderPassReadBackProperty(const char* szPassName, const char* szPropertyName, const wdVariant& value)
{
  SetReadBackProperty(m_PassReadBackProperties, szPassName, szPropertyName, value);
}

wdVariant wdView::GetRenderPassReadBackProperty(const char* szPassName, const char* szPropertyName)
{
  wdStringBuilder sKey(szPassName, "::", szPropertyName);

  auto it = m_PassReadBackProperties.Find(sKey);
  if (it.IsValid())
    return it.Value().m_Value;

  wdLog::Warning("Unknown read-back property '{0}::{1}'", szPassName, szPropertyName);
  return wdVariant();
}


bool wdView::IsRenderPassReadBackPropertyExisting(const char* szPassName, const char* szPropertyName) const
{
  wdStringBuilder sKey(szPassName, "::", szPropertyName);

  auto it = m_PassReadBackProperties.Find(sKey);
  return it.IsValid();
}

void wdView::UpdateViewData(wdUInt32 uiDataIndex)
{
  if (m_pRenderPipeline != nullptr)
  {
    m_pRenderPipeline->UpdateViewData(*this, uiDataIndex);
  }
}

void wdView::UpdateCachedMatrices() const
{
  const wdCamera* pCamera = GetCamera();

  bool bUpdateVP = false;

  if (m_uiLastCameraOrientationModification != pCamera->GetOrientationModificationCounter())
  {
    bUpdateVP = true;
    m_uiLastCameraOrientationModification = pCamera->GetOrientationModificationCounter();

    m_Data.m_ViewMatrix[0] = pCamera->GetViewMatrix(wdCameraEye::Left);
    m_Data.m_ViewMatrix[1] = pCamera->GetViewMatrix(wdCameraEye::Right);

    // Some of our matrices contain very small values so that the matrix inversion will fall below the default epsilon.
    // We pass zero as epsilon here since all view and projection matrices are invertible.
    m_Data.m_InverseViewMatrix[0] = m_Data.m_ViewMatrix[0].GetInverse(0.0f);
    m_Data.m_InverseViewMatrix[1] = m_Data.m_ViewMatrix[1].GetInverse(0.0f);
  }

  const float fViewportAspectRatio = m_Data.m_ViewPortRect.HasNonZeroArea() ? m_Data.m_ViewPortRect.width / m_Data.m_ViewPortRect.height : 1.0f;
  if (m_uiLastCameraSettingsModification != pCamera->GetSettingsModificationCounter() || m_fLastViewportAspectRatio != fViewportAspectRatio)
  {
    bUpdateVP = true;
    m_uiLastCameraSettingsModification = pCamera->GetSettingsModificationCounter();
    m_fLastViewportAspectRatio = fViewportAspectRatio;


    pCamera->GetProjectionMatrix(m_fLastViewportAspectRatio, m_Data.m_ProjectionMatrix[0], wdCameraEye::Left);
    m_Data.m_InverseProjectionMatrix[0] = m_Data.m_ProjectionMatrix[0].GetInverse(0.0f);

    pCamera->GetProjectionMatrix(m_fLastViewportAspectRatio, m_Data.m_ProjectionMatrix[1], wdCameraEye::Right);
    m_Data.m_InverseProjectionMatrix[1] = m_Data.m_ProjectionMatrix[1].GetInverse(0.0f);
  }

  if (bUpdateVP)
  {
    for (int i = 0; i < 2; ++i)
    {
      m_Data.m_ViewProjectionMatrix[i] = m_Data.m_ProjectionMatrix[i] * m_Data.m_ViewMatrix[i];
      m_Data.m_InverseViewProjectionMatrix[i] = m_Data.m_ViewProjectionMatrix[i].GetInverse(0.0f);
    }
  }
}

void wdView::EnsureUpToDate()
{
  if (m_hRenderPipeline.IsValid())
  {
    wdResourceLock<wdRenderPipelineResource> pPipeline(m_hRenderPipeline, wdResourceAcquireMode::BlockTillLoaded);

    wdUInt32 uiCounter = pPipeline->GetCurrentResourceChangeCounter();

    if (m_uiRenderPipelineResourceDescriptionCounter != uiCounter)
    {
      m_uiRenderPipelineResourceDescriptionCounter = uiCounter;

      m_pRenderPipeline = pPipeline->CreateRenderPipeline();
      wdRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());

      m_bPermutationVarsDirty = true;
      ResetAllPropertyStates(m_PassProperties);
      ResetAllPropertyStates(m_ExtractorProperties);
    }

    ApplyPermutationVars();
    ApplyRenderPassProperties();
    ApplyExtractorProperties();
  }
}

void wdView::ApplyPermutationVars()
{
  if (!m_bPermutationVarsDirty)
    return;

  m_pRenderPipeline->m_PermutationVars = m_PermutationVars;
  m_bPermutationVarsDirty = false;
}

void wdView::SetProperty(wdMap<wdString, PropertyValue>& map, const char* szPassName, const char* szPropertyName, const wdVariant& value)
{
  wdStringBuilder sKey(szPassName, "::", szPropertyName);

  bool bExisted = false;
  auto& prop = map.FindOrAdd(sKey, &bExisted).Value();

  if (!bExisted)
  {
    prop.m_sObjectName = szPassName;
    prop.m_sPropertyName = szPropertyName;
    prop.m_bIsValid = true;
  }

  prop.m_bIsDirty = true;
  prop.m_Value = value;
}


void wdView::SetReadBackProperty(wdMap<wdString, PropertyValue>& map, const char* szPassName, const char* szPropertyName, const wdVariant& value)
{
  wdStringBuilder sKey(szPassName, "::", szPropertyName);

  bool bExisted = false;
  auto& prop = map.FindOrAdd(sKey, &bExisted).Value();

  if (!bExisted)
  {
    prop.m_sObjectName = szPassName;
    prop.m_sPropertyName = szPropertyName;
    prop.m_bIsValid = true;
  }

  prop.m_bIsDirty = false;
  prop.m_Value = value;
}

void wdView::ReadBackPassProperties()
{
  wdHybridArray<wdRenderPipelinePass*, 16> passes;
  m_pRenderPipeline->GetPasses(passes);

  for (auto pPass : passes)
  {
    pPass->ReadBackProperties(this);
  }
}

void wdView::ResetAllPropertyStates(wdMap<wdString, PropertyValue>& map)
{
  for (auto it = map.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_bIsDirty = true;
    it.Value().m_bIsValid = true;
  }
}

void wdView::ApplyRenderPassProperties()
{
  for (auto it = m_PassProperties.GetIterator(); it.IsValid(); ++it)
  {
    auto& propertyValue = it.Value();

    if (!propertyValue.m_bIsValid || !propertyValue.m_bIsDirty)
      continue;

    propertyValue.m_bIsDirty = false;

    wdReflectedClass* pObject = nullptr;
    const char* szDot = propertyValue.m_sObjectName.FindSubString(".");
    if (szDot != nullptr)
    {
      WD_REPORT_FAILURE("Setting renderer properties is not possible anymore");
    }
    else
    {
      pObject = m_pRenderPipeline->GetPassByName(propertyValue.m_sObjectName);
    }

    if (pObject == nullptr)
    {
      wdLog::Error(
        "The render pass '{0}' does not exist. Property '{1}' cannot be applied.", propertyValue.m_sObjectName, propertyValue.m_sPropertyName);

      propertyValue.m_bIsValid = false;
      continue;
    }

    ApplyProperty(pObject, propertyValue, "render pass");
  }
}

void wdView::ApplyExtractorProperties()
{
  for (auto it = m_ExtractorProperties.GetIterator(); it.IsValid(); ++it)
  {
    if (!it.Value().m_bIsValid || !it.Value().m_bIsDirty)
      continue;

    it.Value().m_bIsDirty = false;

    wdExtractor* pExtractor = m_pRenderPipeline->GetExtractorByName(it.Value().m_sObjectName);
    if (pExtractor == nullptr)
    {
      wdLog::Error("The extractor '{0}' does not exist. Property '{1}' cannot be applied.", it.Value().m_sObjectName, it.Value().m_sPropertyName);

      it.Value().m_bIsValid = false;
      continue;
    }

    ApplyProperty(pExtractor, it.Value(), "extractor");
  }
}

void wdView::ApplyProperty(wdReflectedClass* pClass, PropertyValue& data, const char* szTypeName)
{
  wdAbstractProperty* pAbstractProperty = pClass->GetDynamicRTTI()->FindPropertyByName(data.m_sPropertyName);
  if (pAbstractProperty == nullptr)
  {
    wdLog::Error("The {0} '{1}' does not have a property called '{2}', it cannot be applied.", szTypeName, data.m_sObjectName, data.m_sPropertyName);

    data.m_bIsValid = false;
    return;
  }

  if (pAbstractProperty->GetCategory() != wdPropertyCategory::Member)
  {
    wdLog::Error("The {0} property '{1}::{2}' is not a member property, it cannot be applied.", szTypeName, data.m_sObjectName, data.m_sPropertyName);

    data.m_bIsValid = false;
    return;
  }

  wdReflectionUtils::SetMemberPropertyValue(static_cast<wdAbstractMemberProperty*>(pAbstractProperty), pClass, data.m_Value);
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_View);
