#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/RenderTargetActivatorComponent.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
WD_BEGIN_COMPONENT_TYPE(wdRenderTargetActivatorComponent, 1, wdComponentMode::Static)
{
  WD_BEGIN_PROPERTIES
  {
    WD_ACCESSOR_PROPERTY("RenderTarget", GetRenderTargetFile, SetRenderTargetFile)->AddAttributes(new wdAssetBrowserAttribute("CompatibleAsset_Texture_Target", wdDependencyFlags::Package)),
  }
  WD_END_PROPERTIES;
  WD_BEGIN_ATTRIBUTES
  {
    new wdCategoryAttribute("Rendering"),
  }
  WD_END_ATTRIBUTES;
  WD_BEGIN_MESSAGEHANDLERS
  {
    WD_MESSAGE_HANDLER(wdMsgExtractRenderData, OnMsgExtractRenderData),
  }
  WD_END_MESSAGEHANDLERS;
}
WD_END_COMPONENT_TYPE;
// clang-format on

wdRenderTargetActivatorComponent::wdRenderTargetActivatorComponent() = default;
wdRenderTargetActivatorComponent::~wdRenderTargetActivatorComponent() = default;

void wdRenderTargetActivatorComponent::SerializeComponent(wdWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  wdStreamWriter& s = inout_stream.GetStream();

  s << m_hRenderTarget;
}

void wdRenderTargetActivatorComponent::DeserializeComponent(wdWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const wdUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  wdStreamReader& s = inout_stream.GetStream();

  s >> m_hRenderTarget;
}

wdResult wdRenderTargetActivatorComponent::GetLocalBounds(wdBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, wdMsgUpdateLocalBounds& ref_msg)
{
  if (m_hRenderTarget.IsValid())
  {
    ref_bounds = wdBoundingSphere(wdVec3::ZeroVector(), 0.1f);
    return WD_SUCCESS;
  }

  return WD_FAILURE;
}

void wdRenderTargetActivatorComponent::OnMsgExtractRenderData(wdMsgExtractRenderData& msg) const
{
  // only add render target views from main views
  // otherwise every shadow casting light source would activate a render target
  if (msg.m_pView->GetCameraUsageHint() != wdCameraUsageHint::MainView && msg.m_pView->GetCameraUsageHint() != wdCameraUsageHint::EditorView)
    return;

  if (!m_hRenderTarget.IsValid())
    return;

  wdResourceLock<wdRenderToTexture2DResource> pRenderTarget(m_hRenderTarget, wdResourceAcquireMode::BlockTillLoaded);

  for (auto hView : pRenderTarget->GetAllRenderViews())
  {
    wdRenderWorld::AddViewToRender(hView);
  }
}

void wdRenderTargetActivatorComponent::SetRenderTarget(const wdRenderToTexture2DResourceHandle& hResource)
{
  m_hRenderTarget = hResource;

  TriggerLocalBoundsUpdate();
}

void wdRenderTargetActivatorComponent::SetRenderTargetFile(const char* szFile)
{
  wdRenderToTexture2DResourceHandle hResource;

  if (!wdStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = wdResourceManager::LoadResource<wdRenderToTexture2DResource>(szFile);
  }

  SetRenderTarget(hResource);
}

const char* wdRenderTargetActivatorComponent::GetRenderTargetFile() const
{
  if (!m_hRenderTarget.IsValid())
    return "";

  return m_hRenderTarget.GetResourceID();
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_RenderTargetActivatorComponent);
