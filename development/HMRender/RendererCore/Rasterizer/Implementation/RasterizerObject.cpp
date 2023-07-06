#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <RendererCore/Rasterizer/Thirdparty/Occluder.h>
#include <RendererCore/Rasterizer/Thirdparty/VectorMath.h>

wdMutex wdRasterizerObject::s_Mutex;
wdMap<wdString, wdSharedPtr<wdRasterizerObject>> wdRasterizerObject::s_Objects;

wdRasterizerObject::wdRasterizerObject() = default;
wdRasterizerObject::~wdRasterizerObject() = default;

#if WD_ENABLED(WD_RASTERIZER_SUPPORTED)

// needed for wdHybridArray below
WD_DEFINE_AS_POD_TYPE(__m128);

void wdRasterizerObject::CreateMesh(const wdGeometry& geo)
{
  wdHybridArray<__m128, 64, wdAlignedAllocatorWrapper> vertices;
  vertices.Reserve(geo.GetPolygons().GetCount() * 4);

  Aabb bounds;

  auto addVtx = [&](wdVec3 vtxPos) {
    wdSimdVec4f v;
    v.Load<4>(vtxPos.GetAsPositionVec4().GetData());
    vertices.PushBack(v.m_v);
  };

  for (const auto& poly : geo.GetPolygons())
  {
    const wdUInt32 uiNumVertices = poly.m_Vertices.GetCount();
    wdUInt32 uiQuadVtx = 0;

    // ignore complex polygons entirely
    if (uiNumVertices > 4)
      continue;

    for (wdUInt32 i = 0; i < uiNumVertices; ++i)
    {
      if (uiQuadVtx == 4)
      {
        // TODO: restart next quad (also flip this one's front face)
        break;
      }

      const wdUInt32 vtxIdx = poly.m_Vertices[i];

      addVtx(geo.GetVertices()[vtxIdx].m_vPosition);

      bounds.include(vertices.PeekBack());
      ++uiQuadVtx;
    }

    // if the polygon is a triangle, duplicate the last vertex to make it a degenerate quad
    if (uiQuadVtx == 3)
    {
      vertices.PushBack(vertices.PeekBack());
      ++uiQuadVtx;
    }

    if (uiQuadVtx == 4)
    {
      const wdUInt32 n = vertices.GetCount();

      // swap two vertices in the quad to flip the front face (different convention between EZ and the rasterizer)
      wdMath::Swap(vertices[n - 1], vertices[n - 3]);
    }

    WD_ASSERT_DEV(uiQuadVtx == 4, "Degenerate polygon encountered");
  }

  // pad vertices to 32 for proper alignment during baking
  while (vertices.GetCount() % 32 != 0)
  {
    vertices.PushBack(vertices[0]);
  }

  m_Occluder.bake(vertices.GetData(), vertices.GetCount(), bounds.m_min, bounds.m_max);
}

wdSharedPtr<const wdRasterizerObject> wdRasterizerObject::GetObject(wdStringView sUniqueName)
{
  WD_LOCK(s_Mutex);

  auto it = s_Objects.Find(sUniqueName);

  if (it.IsValid())
    return it.Value();

  return nullptr;
}

wdSharedPtr<const wdRasterizerObject> wdRasterizerObject::CreateBox(const wdVec3& vFullExtents)
{
  WD_LOCK(s_Mutex);

  wdStringBuilder sName;
  sName.Format("Box-{}-{}-{}", vFullExtents.x, vFullExtents.y, vFullExtents.z);

  wdSharedPtr<wdRasterizerObject>& pObj = s_Objects[sName];

  if (pObj == nullptr)
  {
    pObj = WD_NEW(wdFoundation::GetAlignedAllocator(), wdRasterizerObject);

    wdGeometry geometry;
    geometry.AddBox(vFullExtents, false, {});

    pObj->CreateMesh(geometry);
  }

  return pObj;
}

wdSharedPtr<const wdRasterizerObject> wdRasterizerObject::CreateMesh(wdStringView sUniqueName, const wdGeometry& geometry)
{
  WD_LOCK(s_Mutex);

  wdSharedPtr<wdRasterizerObject>& pObj = s_Objects[sUniqueName];

  if (pObj == nullptr)
  {
    pObj = WD_NEW(wdFoundation::GetAlignedAllocator(), wdRasterizerObject);

    pObj->CreateMesh(geometry);
  }

  return pObj;
}

#else

void wdRasterizerObject::CreateMesh(const wdGeometry& geo)
{
}

wdSharedPtr<const wdRasterizerObject> wdRasterizerObject::GetObject(wdStringView sUniqueName)
{
  return nullptr;
}

wdSharedPtr<const wdRasterizerObject> wdRasterizerObject::CreateBox(const wdVec3& vFullExtents)
{
  return nullptr;
}

wdSharedPtr<const wdRasterizerObject> wdRasterizerObject::CreateMesh(wdStringView sUniqueName, const wdGeometry& geometry)
{
  return nullptr;
}

#endif


WD_STATICLINK_FILE(RendererCore, RendererCore_Rasterizer_Implementation_RasterizerObject);
