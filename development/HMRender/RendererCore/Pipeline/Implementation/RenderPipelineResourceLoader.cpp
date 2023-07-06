#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Serialization/BinarySerializer.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>

////////////////////////////////////////////////////////////////////////
// wdDocumentNodeManager Internal
////////////////////////////////////////////////////////////////////////

struct RenderPipelineResourceLoaderConnectionInternal
{
  wdUuid m_Source;
  wdUuid m_Target;
  wdString m_SourcePin;
  wdString m_TargetPin;
};
WD_DECLARE_REFLECTABLE_TYPE(WD_NO_LINKAGE, RenderPipelineResourceLoaderConnectionInternal);

// clang-format off
WD_BEGIN_STATIC_REFLECTED_TYPE(RenderPipelineResourceLoaderConnectionInternal, wdNoBase, 1, wdRTTIDefaultAllocator<RenderPipelineResourceLoaderConnectionInternal>)
{
  WD_BEGIN_PROPERTIES
  {
    WD_MEMBER_PROPERTY("Connection::Source", m_Source),
    WD_MEMBER_PROPERTY("Connection::Target", m_Target),
    WD_MEMBER_PROPERTY("Connection::SourcePin", m_SourcePin),    
    WD_MEMBER_PROPERTY("Connection::TargetPin", m_TargetPin),
  }
  WD_END_PROPERTIES;
}
WD_END_STATIC_REFLECTED_TYPE;
// clang-format on

void wdRenderPipelineRttiConverterContext::Clear()
{
  wdRttiConverterContext::Clear();

  m_pRenderPipeline = nullptr;
}

wdInternal::NewInstance<void> wdRenderPipelineRttiConverterContext::CreateObject(const wdUuid& guid, const wdRTTI* pRtti)
{
  WD_ASSERT_DEBUG(pRtti != nullptr, "Object type is unknown");

  if (pRtti->IsDerivedFrom<wdRenderPipelinePass>())
  {
    if (!pRtti->GetAllocator()->CanAllocate())
    {
      wdLog::Error("Failed to create wdRenderPipelinePass because '{0}' cannot allocate!", pRtti->GetTypeName());
      return nullptr;
    }

    auto pass = pRtti->GetAllocator()->Allocate<wdRenderPipelinePass>();
    m_pRenderPipeline->AddPass(pass);

    RegisterObject(guid, pRtti, pass);
    return pass;
  }
  else if (pRtti->IsDerivedFrom<wdExtractor>())
  {
    if (!pRtti->GetAllocator()->CanAllocate())
    {
      wdLog::Error("Failed to create wdExtractor because '{0}' cannot allocate!", pRtti->GetTypeName());
      return nullptr;
    }

    auto extractor = pRtti->GetAllocator()->Allocate<wdExtractor>();
    m_pRenderPipeline->AddExtractor(extractor);

    RegisterObject(guid, pRtti, extractor);
    return extractor;
  }
  else
  {
    return wdRttiConverterContext::CreateObject(guid, pRtti);
  }
}

void wdRenderPipelineRttiConverterContext::DeleteObject(const wdUuid& guid)
{
  wdRttiConverterObject object = GetObjectByGUID(guid);
  const wdRTTI* pRtti = object.m_pType;
  WD_ASSERT_DEBUG(pRtti != nullptr, "Object does not exist!");
  if (pRtti->IsDerivedFrom<wdRenderPipelinePass>())
  {
    wdRenderPipelinePass* pPass = static_cast<wdRenderPipelinePass*>(object.m_pObject);

    UnregisterObject(guid);
    m_pRenderPipeline->RemovePass(pPass);
  }
  else if (pRtti->IsDerivedFrom<wdExtractor>())
  {
    wdExtractor* pExtractor = static_cast<wdExtractor*>(object.m_pObject);

    UnregisterObject(guid);
    m_pRenderPipeline->RemoveExtractor(pExtractor);
  }
  else
  {
    wdRttiConverterContext::DeleteObject(guid);
  }
}

// static
wdInternal::NewInstance<wdRenderPipeline> wdRenderPipelineResourceLoader::CreateRenderPipeline(const wdRenderPipelineResourceDescriptor& desc)
{
  auto pPipeline = WD_DEFAULT_NEW(wdRenderPipeline);
  wdRenderPipelineRttiConverterContext context;
  context.m_pRenderPipeline = pPipeline;

  wdRawMemoryStreamReader memoryReader(desc.m_SerializedPipeline);

  wdAbstractObjectGraph graph;
  wdAbstractGraphBinarySerializer::Read(memoryReader, &graph);

  wdRttiConverterReader rttiConverter(&graph, &context);

  auto& nodes = graph.GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto pNode = it.Value();
    wdRTTI* pType = wdRTTI::FindTypeByName(pNode->GetType());
    if (pType && pType->IsDerivedFrom<wdRenderPipelinePass>())
    {
      auto pPass = rttiConverter.CreateObjectFromNode(pNode);
      if (!pPass)
      {
        wdLog::Error("Failed to deserialize wdRenderPipelinePass!");
      }
    }
    else if (pType && pType->IsDerivedFrom<wdExtractor>())
    {
      auto pExtractor = rttiConverter.CreateObjectFromNode(pNode);
      if (!pExtractor)
      {
        wdLog::Error("Failed to deserialize wdExtractor!");
      }
    }
  }

  auto pType = wdGetStaticRTTI<RenderPipelineResourceLoaderConnectionInternal>();
  wdStringBuilder tmp;

  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    const wdUuid& guid = pNode->GetGuid();

    if (wdStringUtils::IsEqual(pNode->GetNodeName(), "Connection") == false)
      continue;

    RenderPipelineResourceLoaderConnectionInternal data;
    rttiConverter.ApplyPropertiesToObject(pNode, pType, &data);

    auto objectSource = context.GetObjectByGUID(data.m_Source);
    if (objectSource.m_pObject == nullptr || !objectSource.m_pType->IsDerivedFrom<wdRenderPipelinePass>())
    {
      wdLog::Error("Failed to retrieve connection target '{0}' with pin '{1}'", wdConversionUtils::ToString(guid, tmp), data.m_TargetPin);
      continue;
    }

    auto objectTarget = context.GetObjectByGUID(data.m_Target);
    if (objectTarget.m_pObject == nullptr || !objectTarget.m_pType->IsDerivedFrom<wdRenderPipelinePass>())
    {
      wdLog::Error("Failed to retrieve connection target '{0}' with pin '{1}'", wdConversionUtils::ToString(guid, tmp), data.m_TargetPin);
      continue;
    }

    wdRenderPipelinePass* pSource = static_cast<wdRenderPipelinePass*>(objectSource.m_pObject);
    wdRenderPipelinePass* pTarget = static_cast<wdRenderPipelinePass*>(objectTarget.m_pObject);

    if (!pPipeline->Connect(pSource, data.m_SourcePin, pTarget, data.m_TargetPin))
    {
      wdLog::Error("Failed to connect '{0}'::'{1}' to '{2}'::'{3}'!", pSource->GetName(), data.m_SourcePin, pTarget->GetName(), data.m_TargetPin);
    }
  }

  return pPipeline;
}

// static
void wdRenderPipelineResourceLoader::CreateRenderPipelineResourceDescriptor(const wdRenderPipeline* pPipeline, wdRenderPipelineResourceDescriptor& ref_desc)
{
  wdRenderPipelineRttiConverterContext context;

  wdAbstractObjectGraph graph;

  wdRttiConverterWriter rttiConverter(&graph, &context, false, true);

  wdHybridArray<const wdRenderPipelinePass*, 16> passes;
  pPipeline->GetPasses(passes);

  // Need to serialize all passes first so we have guids for each to be referenced in the connections.
  for (auto pPass : passes)
  {
    wdUuid guid;
    guid.CreateNewUuid();
    context.RegisterObject(guid, pPass->GetDynamicRTTI(), const_cast<wdRenderPipelinePass*>(pPass));
    rttiConverter.AddObjectToGraph(const_cast<wdRenderPipelinePass*>(pPass));
  }
  wdHybridArray<const wdExtractor*, 16> extractors;
  pPipeline->GetExtractors(extractors);
  for (auto pExtractor : extractors)
  {
    wdUuid guid;
    guid.CreateNewUuid();
    context.RegisterObject(guid, pExtractor->GetDynamicRTTI(), const_cast<wdExtractor*>(pExtractor));
    rttiConverter.AddObjectToGraph(const_cast<wdExtractor*>(pExtractor));
  }

  auto pType = wdGetStaticRTTI<RenderPipelineResourceLoaderConnectionInternal>();
  auto& nodes = graph.GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    auto objectSoure = context.GetObjectByGUID(pNode->GetGuid());

    if (objectSoure.m_pObject == nullptr || !objectSoure.m_pType->IsDerivedFrom<wdRenderPipelinePass>())
    {
      continue;
    }
    wdRenderPipelinePass* pSource = static_cast<wdRenderPipelinePass*>(objectSoure.m_pObject);

    RenderPipelineResourceLoaderConnectionInternal data;
    data.m_Source = pNode->GetGuid();

    auto outputs = pSource->GetOutputPins();
    for (const wdRenderPipelineNodePin* pPinSource : outputs)
    {
      data.m_SourcePin = pSource->GetPinName(pPinSource).GetView();

      const wdRenderPipelinePassConnection* pConnection = pPipeline->GetOutputConnection(pSource, pSource->GetPinName(pPinSource));
      if (!pConnection)
        continue;

      for (const wdRenderPipelineNodePin* pPinTarget : pConnection->m_Inputs)
      {
        data.m_Target = context.GetObjectGUID(pPinTarget->m_pParent->GetDynamicRTTI(), pPinTarget->m_pParent);
        data.m_TargetPin = pPinTarget->m_pParent->GetPinName(pPinTarget).GetView();

        wdUuid connectionGuid;
        connectionGuid.CreateNewUuid();
        context.RegisterObject(connectionGuid, pType, &data);
        rttiConverter.AddObjectToGraph(pType, &data, "Connection");
      }
    }
  }

  wdMemoryStreamContainerWrapperStorage<wdDynamicArray<wdUInt8>> storage(&ref_desc.m_SerializedPipeline);

  wdMemoryStreamWriter memoryWriter(&storage);
  wdAbstractGraphBinarySerializer::Write(memoryWriter, &graph);
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelineResourceLoader);
