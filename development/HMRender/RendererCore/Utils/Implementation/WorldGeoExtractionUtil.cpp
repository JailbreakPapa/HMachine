#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Core/World/World.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

WD_IMPLEMENT_MESSAGE_TYPE(wdMsgExtractGeometry);
WD_BEGIN_DYNAMIC_REFLECTED_TYPE(wdMsgExtractGeometry, 1, wdRTTIDefaultAllocator<wdMsgExtractGeometry>)
WD_END_DYNAMIC_REFLECTED_TYPE;

void wdWorldGeoExtractionUtil::ExtractWorldGeometry(MeshObjectList& ref_objects, const wdWorld& world, ExtractionMode mode, wdTagSet* pExcludeTags /*= nullptr*/)
{
  WD_PROFILE_SCOPE("ExtractWorldGeometry");
  WD_LOG_BLOCK("ExtractWorldGeometry", world.GetName());

  wdMsgExtractGeometry msg;
  msg.m_Mode = mode;
  msg.m_pMeshObjects = &ref_objects;

  WD_LOCK(world.GetReadMarker());

  for (auto it = world.GetObjects(); it.IsValid(); ++it)
  {
    if (pExcludeTags != nullptr && it->GetTags().IsAnySet(*pExcludeTags))
      continue;

    it->SendMessage(msg);
  }
}

void wdWorldGeoExtractionUtil::ExtractWorldGeometry(MeshObjectList& ref_objects, const wdWorld& world, ExtractionMode mode, const wdDeque<wdGameObjectHandle>& selection)
{
  WD_PROFILE_SCOPE("ExtractWorldGeometry");
  WD_LOG_BLOCK("ExtractWorldGeometry", world.GetName());

  wdMsgExtractGeometry msg;
  msg.m_Mode = mode;
  msg.m_pMeshObjects = &ref_objects;

  WD_LOCK(world.GetReadMarker());

  for (wdGameObjectHandle hObject : selection)
  {
    const wdGameObject* pObject;
    if (!world.TryGetObject(hObject, pObject))
      continue;

    pObject->SendMessage(msg);
  }
}

void wdWorldGeoExtractionUtil::WriteWorldGeometryToOBJ(const char* szFile, const MeshObjectList& objects, const wdMat3& mTransform)
{
  WD_LOG_BLOCK("Write World Geometry to OBJ", szFile);

  wdFileWriter file;
  if (file.Open(szFile).Failed())
  {
    wdLog::Error("Failed to open file for writing: '{0}'", szFile);
    return;
  }

  wdMat4 transform = wdMat4::IdentityMatrix();
  transform.SetRotationalPart(mTransform);

  wdStringBuilder line;

  line = "\n\n# vertices\n\n";
  file.WriteBytes(line.GetData(), line.GetElementCount()).IgnoreResult();

  wdUInt32 uiVertexOffset = 0;
  wdDeque<wdUInt32> indices;

  for (const MeshObject& object : objects)
  {
    wdResourceLock<wdCpuMeshResource> pCpuMesh(object.m_hMeshResource, wdResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pCpuMesh.GetAcquireResult() != wdResourceAcquireResult::Final)
    {
      continue;
    }

    const auto& meshBufferDesc = pCpuMesh->GetDescriptor().MeshBufferDesc();

    const wdVec3* pPositions = nullptr;
    wdUInt32 uiElementStride = 0;
    if (wdMeshBufferUtils::GetPositionStream(meshBufferDesc, pPositions, uiElementStride).Failed())
    {
      continue;
    }

    wdMat4 finalTransform = transform * object.m_GlobalTransform.GetAsMat4();

    // write out all vertices
    for (wdUInt32 i = 0; i < meshBufferDesc.GetVertexCount(); ++i)
    {
      const wdVec3 pos = finalTransform.TransformPosition(*pPositions);

      line.Format("v {0} {1} {2}\n", wdArgF(pos.x, 8), wdArgF(pos.y, 8), wdArgF(pos.z, 8));
      file.WriteBytes(line.GetData(), line.GetElementCount()).IgnoreResult();

      pPositions = wdMemoryUtils::AddByteOffset(pPositions, uiElementStride);
    }

    // collect all indices
    bool flip = wdGraphicsUtils::IsTriangleFlipRequired(finalTransform.GetRotationalPart());

    if (meshBufferDesc.HasIndexBuffer())
    {
      if (meshBufferDesc.Uses32BitIndices())
      {
        const wdUInt32* pTypedIndices = reinterpret_cast<const wdUInt32*>(meshBufferDesc.GetIndexBufferData().GetPtr());

        for (wdUInt32 p = 0; p < meshBufferDesc.GetPrimitiveCount(); ++p)
        {
          indices.PushBack(pTypedIndices[p * 3 + (flip ? 2 : 0)] + uiVertexOffset);
          indices.PushBack(pTypedIndices[p * 3 + 1] + uiVertexOffset);
          indices.PushBack(pTypedIndices[p * 3 + (flip ? 0 : 2)] + uiVertexOffset);
        }
      }
      else
      {
        const wdUInt16* pTypedIndices = reinterpret_cast<const wdUInt16*>(meshBufferDesc.GetIndexBufferData().GetPtr());

        for (wdUInt32 p = 0; p < meshBufferDesc.GetPrimitiveCount(); ++p)
        {
          indices.PushBack(pTypedIndices[p * 3 + (flip ? 2 : 0)] + uiVertexOffset);
          indices.PushBack(pTypedIndices[p * 3 + 1] + uiVertexOffset);
          indices.PushBack(pTypedIndices[p * 3 + (flip ? 0 : 2)] + uiVertexOffset);
        }
      }
    }
    else
    {
      for (wdUInt32 v = 0; v < meshBufferDesc.GetVertexCount(); ++v)
      {
        indices.PushBack(uiVertexOffset + v);
      }
    }

    uiVertexOffset += meshBufferDesc.GetVertexCount();
  }

  line = "\n\n# triangles\n\n";
  file.WriteBytes(line.GetData(), line.GetElementCount()).IgnoreResult();

  for (wdUInt32 i = 0; i < indices.GetCount(); i += 3)
  {
    // indices are 1 based in obj
    line.Format("f {0} {1} {2}\n", indices[i + 0] + 1, indices[i + 1] + 1, indices[i + 2] + 1);
    file.WriteBytes(line.GetData(), line.GetElementCount()).IgnoreResult();
  }

  wdLog::Success("Wrote world geometry to '{0}'", file.GetFilePathAbsolute().GetView());
}

//////////////////////////////////////////////////////////////////////////

void wdMsgExtractGeometry::AddMeshObject(const wdTransform& transform, wdCpuMeshResourceHandle hMeshResource)
{
  m_pMeshObjects->PushBack({transform, hMeshResource});
}

void wdMsgExtractGeometry::AddBox(const wdTransform& transform, wdVec3 vExtents)
{
  const char* szResourceName = "CpuMesh-UnitBox";
  wdCpuMeshResourceHandle hBoxMesh = wdResourceManager::GetExistingResource<wdCpuMeshResource>(szResourceName);
  if (hBoxMesh.IsValid() == false)
  {
    wdGeometry geom;
    geom.AddBox(wdVec3(1), false);
    geom.TriangulatePolygons();
    geom.ComputeTangents();

    wdMeshResourceDescriptor desc;
    desc.SetMaterial(0, "{ 1c47ee4c-0379-4280-85f5-b8cda61941d2 }"); // Data/Base/Materials/Common/Pattern.wdMaterialAsset

    desc.MeshBufferDesc().AddCommonStreams();
    desc.MeshBufferDesc().AllocateStreamsFromGeometry(geom, wdGALPrimitiveTopology::Triangles);

    desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);

    desc.ComputeBounds();

    hBoxMesh = wdResourceManager::GetOrCreateResource<wdCpuMeshResource>(szResourceName, std::move(desc), szResourceName);
  }

  auto& meshObject = m_pMeshObjects->ExpandAndGetRef();
  meshObject.m_GlobalTransform = transform;
  meshObject.m_GlobalTransform.m_vScale *= vExtents;
  meshObject.m_hMeshResource = hBoxMesh;
}

WD_STATICLINK_FILE(RendererCore, RendererCore_Utils_Implementation_WorldGeoExtractionUtil);
