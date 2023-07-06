#include <Core/CorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Math/Quat.h>
#include <mikktspace/mikktspace.h>

bool wdGeometry::GeoOptions::IsFlipWindingNecessary() const
{
  return m_Transform.GetRotationalPart().GetDeterminant() < 0;
}

bool wdGeometry::Vertex::operator<(const wdGeometry::Vertex& rhs) const
{
  if (m_vPosition != rhs.m_vPosition)
    return m_vPosition < rhs.m_vPosition;

  if (m_vNormal != rhs.m_vNormal)
    return m_vNormal < rhs.m_vNormal;

  if (m_vTangent != rhs.m_vTangent)
    return m_vTangent < rhs.m_vTangent;

  if (m_fBiTangentSign != rhs.m_fBiTangentSign)
    return m_fBiTangentSign < rhs.m_fBiTangentSign;

  if (m_vTexCoord != rhs.m_vTexCoord)
    return m_vTexCoord < rhs.m_vTexCoord;

  if (m_Color != rhs.m_Color)
    return m_Color < rhs.m_Color;

  if (m_BoneIndices != rhs.m_BoneIndices)
    return m_BoneIndices < rhs.m_BoneIndices;

  return m_BoneWeights < rhs.m_BoneWeights;
}

bool wdGeometry::Vertex::operator==(const wdGeometry::Vertex& rhs) const
{
  return m_vPosition == rhs.m_vPosition &&
         m_vNormal == rhs.m_vNormal &&
         m_vTangent == rhs.m_vTangent &&
         m_fBiTangentSign == rhs.m_fBiTangentSign &&
         m_vTexCoord == rhs.m_vTexCoord &&
         m_Color == rhs.m_Color &&
         m_BoneIndices == rhs.m_BoneIndices &&
         m_BoneWeights == rhs.m_BoneWeights;
}

void wdGeometry::Polygon::FlipWinding()
{
  const wdUInt32 uiCount = m_Vertices.GetCount();
  const wdUInt32 uiHalfCount = uiCount / 2;
  for (wdUInt32 i = 0; i < uiHalfCount; i++)
  {
    wdMath::Swap(m_Vertices[i], m_Vertices[uiCount - i - 1]);
  }
}

void wdGeometry::Clear()
{
  m_Vertices.Clear();
  m_Polygons.Clear();
  m_Lines.Clear();
}

wdUInt32 wdGeometry::AddVertex(const wdVec3& vPos, const wdVec3& vNormal, const wdVec2& vTexCoord, const wdColor& color, const wdVec4U16& vBoneIndices /*= wdVec4U16::ZeroVector()*/, const wdColorLinearUB& boneWeights /*= wdColorLinearUB(255, 0, 0, 0)*/)
{
  Vertex& v = m_Vertices.ExpandAndGetRef();
  v.m_vPosition = vPos;
  v.m_vNormal = vNormal;
  v.m_vTexCoord = vTexCoord;
  v.m_Color = color;
  v.m_BoneIndices = vBoneIndices;
  v.m_BoneWeights = boneWeights;

  return m_Vertices.GetCount() - 1;
}

void wdGeometry::AddPolygon(const wdArrayPtr<wdUInt32>& vertices, bool bFlipWinding)
{
  WD_ASSERT_DEV(vertices.GetCount() >= 3, "Polygon must have at least 3 vertices, not {0}", vertices.GetCount());

  for (wdUInt32 v = 0; v < vertices.GetCount(); ++v)
  {
    WD_ASSERT_DEV(vertices[v] < m_Vertices.GetCount(), "Invalid vertex index {0}, geometry only has {1} vertices", vertices[v], m_Vertices.GetCount());
  }

  m_Polygons.ExpandAndGetRef().m_Vertices = vertices;

  if (bFlipWinding)
  {
    m_Polygons.PeekBack().FlipWinding();
  }
}

void wdGeometry::AddLine(wdUInt32 uiStartVertex, wdUInt32 uiEndVertex)
{
  WD_ASSERT_DEV(uiStartVertex < m_Vertices.GetCount(), "Invalid vertex index {0}, geometry only has {1} vertices", uiStartVertex, m_Vertices.GetCount());
  WD_ASSERT_DEV(uiEndVertex < m_Vertices.GetCount(), "Invalid vertex index {0}, geometry only has {1} vertices", uiEndVertex, m_Vertices.GetCount());

  Line l;
  l.m_uiStartVertex = uiStartVertex;
  l.m_uiEndVertex = uiEndVertex;

  m_Lines.PushBack(l);
}


void wdGeometry::TriangulatePolygons(wdUInt32 uiMaxVerticesInPolygon /*= 3*/)
{
  WD_ASSERT_DEV(uiMaxVerticesInPolygon >= 3, "Can't triangulate polygons that are already triangles.");
  uiMaxVerticesInPolygon = wdMath::Max<wdUInt32>(uiMaxVerticesInPolygon, 3);

  const wdUInt32 uiNumPolys = m_Polygons.GetCount();

  for (wdUInt32 p = 0; p < uiNumPolys; ++p)
  {
    const auto& poly = m_Polygons[p];

    const wdUInt32 uiNumVerts = poly.m_Vertices.GetCount();
    if (uiNumVerts > uiMaxVerticesInPolygon)
    {
      for (wdUInt32 v = 2; v < uiNumVerts; ++v)
      {
        auto& tri = m_Polygons.ExpandAndGetRef();
        tri.m_vNormal = poly.m_vNormal;
        tri.m_Vertices.SetCountUninitialized(3);
        tri.m_Vertices[0] = poly.m_Vertices[0];
        tri.m_Vertices[1] = poly.m_Vertices[v - 1];
        tri.m_Vertices[2] = poly.m_Vertices[v];
      }

      m_Polygons.RemoveAtAndSwap(p);
    }
  }
}

void wdGeometry::ComputeFaceNormals()
{
  for (wdUInt32 p = 0; p < m_Polygons.GetCount(); ++p)
  {
    Polygon& poly = m_Polygons[p];

    const wdVec3& v1 = m_Vertices[poly.m_Vertices[0]].m_vPosition;
    const wdVec3& v2 = m_Vertices[poly.m_Vertices[1]].m_vPosition;
    const wdVec3& v3 = m_Vertices[poly.m_Vertices[2]].m_vPosition;

    poly.m_vNormal.CalculateNormal(v1, v2, v3).IgnoreResult();
  }
}

void wdGeometry::ComputeSmoothVertexNormals()
{
  // reset all vertex normals
  for (wdUInt32 v = 0; v < m_Vertices.GetCount(); ++v)
  {
    m_Vertices[v].m_vNormal.SetZero();
  }

  // add face normal of all adjacent faces to each vertex
  for (wdUInt32 p = 0; p < m_Polygons.GetCount(); ++p)
  {
    Polygon& poly = m_Polygons[p];

    for (wdUInt32 v = 0; v < poly.m_Vertices.GetCount(); ++v)
    {
      m_Vertices[poly.m_Vertices[v]].m_vNormal += poly.m_vNormal;
    }
  }

  // normalize all vertex normals
  for (wdUInt32 v = 0; v < m_Vertices.GetCount(); ++v)
  {
    m_Vertices[v].m_vNormal.NormalizeIfNotZero(wdVec3(0, 1, 0)).IgnoreResult();
  }
}

struct TangentContext
{
  TangentContext(wdGeometry* pGeom)
    : m_pGeom(pGeom)
  {
    m_Polygons = m_pGeom->GetPolygons();
  }

  static int getNumFaces(const SMikkTSpaceContext* pContext)
  {
    TangentContext& context = *static_cast<TangentContext*>(pContext->m_pUserData);
    return context.m_pGeom->GetPolygons().GetCount();
  }
  static int getNumVerticesOfFace(const SMikkTSpaceContext* pContext, const int iFace)
  {
    TangentContext& context = *static_cast<TangentContext*>(pContext->m_pUserData);
    return context.m_pGeom->GetPolygons()[iFace].m_Vertices.GetCount();
  }
  static void getPosition(const SMikkTSpaceContext* pContext, float pPosOut[], const int iFace, const int iVert)
  {
    TangentContext& context = *static_cast<TangentContext*>(pContext->m_pUserData);
    wdUInt32 iVertexIndex = context.m_pGeom->GetPolygons()[iFace].m_Vertices[iVert];
    const wdVec3& pos = context.m_pGeom->GetVertices()[iVertexIndex].m_vPosition;
    pPosOut[0] = pos.x;
    pPosOut[1] = pos.y;
    pPosOut[2] = pos.z;
  }
  static void getNormal(const SMikkTSpaceContext* pContext, float pNormOut[], const int iFace, const int iVert)
  {
    TangentContext& context = *static_cast<TangentContext*>(pContext->m_pUserData);
    wdUInt32 iVertexIndex = context.m_pGeom->GetPolygons()[iFace].m_Vertices[iVert];
    const wdVec3& normal = context.m_pGeom->GetVertices()[iVertexIndex].m_vNormal;
    pNormOut[0] = normal.x;
    pNormOut[1] = normal.y;
    pNormOut[2] = normal.z;
  }
  static void getTexCoord(const SMikkTSpaceContext* pContext, float pTexcOut[], const int iFace, const int iVert)
  {
    TangentContext& context = *static_cast<TangentContext*>(pContext->m_pUserData);
    wdUInt32 iVertexIndex = context.m_pGeom->GetPolygons()[iFace].m_Vertices[iVert];
    const wdVec2& tex = context.m_pGeom->GetVertices()[iVertexIndex].m_vTexCoord;
    pTexcOut[0] = tex.x;
    pTexcOut[1] = tex.y;
  }
  static void setTSpaceBasic(const SMikkTSpaceContext* pContext, const float pTangent[], const float fSign, const int iFace, const int iVert)
  {
    TangentContext& context = *static_cast<TangentContext*>(pContext->m_pUserData);
    wdUInt32 iVertexIndex = context.m_pGeom->GetPolygons()[iFace].m_Vertices[iVert];
    wdGeometry::Vertex v = context.m_pGeom->GetVertices()[iVertexIndex];
    v.m_vTangent.x = pTangent[0];
    v.m_vTangent.y = pTangent[1];
    v.m_vTangent.z = pTangent[2];
    v.m_fBiTangentSign = fSign;

    bool existed = false;
    auto it = context.m_VertMap.FindOrAdd(v, &existed);
    if (!existed)
    {
      it.Value() = context.m_Vertices.GetCount();
      context.m_Vertices.PushBack(v);
    }
    wdUInt32 iNewVertexIndex = it.Value();
    context.m_Polygons[iFace].m_Vertices[iVert] = iNewVertexIndex;
  }

  static void setTSpace(const SMikkTSpaceContext* pContext, const float pTangent[], const float pBiTangent[], const float fMagS, const float fMagT, const tbool isOrientationPreserving, const int iFace, const int iVert)
  {
    int i = 0;
    (void)i;
  }

  wdGeometry* m_pGeom;
  wdMap<wdGeometry::Vertex, wdUInt32> m_VertMap;
  wdDeque<wdGeometry::Vertex> m_Vertices;
  wdDeque<wdGeometry::Polygon> m_Polygons;
};

void wdGeometry::ComputeTangents()
{
  for (wdUInt32 i = 0; i < m_Polygons.GetCount(); ++i)
  {
    if (m_Polygons[i].m_Vertices.GetCount() > 4)
    {
      wdLog::Error("Tangent generation does not support polygons with more than 4 vertices");
      break;
    }
  }

  SMikkTSpaceInterface sMikkTInterface;
  sMikkTInterface.m_getNumFaces = &TangentContext::getNumFaces;
  sMikkTInterface.m_getNumVerticesOfFace = &TangentContext::getNumVerticesOfFace;
  sMikkTInterface.m_getPosition = &TangentContext::getPosition;
  sMikkTInterface.m_getNormal = &TangentContext::getNormal;
  sMikkTInterface.m_getTexCoord = &TangentContext::getTexCoord;
  sMikkTInterface.m_setTSpaceBasic = &TangentContext::setTSpaceBasic;
  sMikkTInterface.m_setTSpace = &TangentContext::setTSpace;
  TangentContext context(this);

  SMikkTSpaceContext sMikkTContext;
  sMikkTContext.m_pInterface = &sMikkTInterface;
  sMikkTContext.m_pUserData = &context;

  genTangSpaceDefault(&sMikkTContext);
  m_Polygons = std::move(context.m_Polygons);
  m_Vertices = std::move(context.m_Vertices);
}

void wdGeometry::ValidateTangents(float fEpsilon)
{
  for (auto& vertex : m_Vertices)
  {
    // checking for orthogonality to the normal and for squared unit length (standard case) or 3 (magic number for binormal inversion)
    if (!wdMath::IsEqual(vertex.m_vNormal.GetLengthSquared(), 1.f, fEpsilon) || !wdMath::IsEqual(vertex.m_vNormal.Dot(vertex.m_vTangent), 0.f, fEpsilon) || !(wdMath::IsEqual(vertex.m_vTangent.GetLengthSquared(), 1.f, fEpsilon) || wdMath::IsEqual(vertex.m_vTangent.GetLengthSquared(), 3.f, fEpsilon)))
    {
      vertex.m_vTangent.SetZero();
    }
  }
}

wdUInt32 wdGeometry::CalculateTriangleCount() const
{
  const wdUInt32 numPolys = m_Polygons.GetCount();
  wdUInt32 numTris = 0;

  for (wdUInt32 p = 0; p < numPolys; ++p)
  {
    numTris += m_Polygons[p].m_Vertices.GetCount() - 2;
  }

  return numTris;
}

void wdGeometry::SetAllVertexBoneIndices(const wdVec4U16& vBoneIndices, wdUInt32 uiFirstVertex)
{
  for (wdUInt32 v = uiFirstVertex; v < m_Vertices.GetCount(); ++v)
    m_Vertices[v].m_BoneIndices = vBoneIndices;
}

void wdGeometry::SetAllVertexColor(const wdColor& color, wdUInt32 uiFirstVertex)
{
  for (wdUInt32 v = uiFirstVertex; v < m_Vertices.GetCount(); ++v)
    m_Vertices[v].m_Color = color;
}


void wdGeometry::SetAllVertexTexCoord(const wdVec2& vTexCoord, wdUInt32 uiFirstVertex /*= 0*/)
{
  for (wdUInt32 v = uiFirstVertex; v < m_Vertices.GetCount(); ++v)
    m_Vertices[v].m_vTexCoord = vTexCoord;
}

void wdGeometry::TransformVertices(const wdMat4& mTransform, wdUInt32 uiFirstVertex)
{
  if (mTransform.IsIdentity(wdMath::SmallEpsilon<float>()))
    return;

  for (wdUInt32 v = uiFirstVertex; v < m_Vertices.GetCount(); ++v)
  {
    m_Vertices[v].m_vPosition = mTransform.TransformPosition(m_Vertices[v].m_vPosition);
    m_Vertices[v].m_vNormal = mTransform.TransformDirection(m_Vertices[v].m_vNormal);
  }
}

void wdGeometry::Transform(const wdMat4& mTransform, bool bTransformPolyNormals)
{
  TransformVertices(mTransform, 0);

  if (bTransformPolyNormals)
  {
    for (wdUInt32 p = 0; p < m_Polygons.GetCount(); ++p)
    {
      m_Polygons[p].m_vNormal = mTransform.TransformDirection(m_Polygons[p].m_vNormal);
    }
  }
}

void wdGeometry::Merge(const wdGeometry& other)
{
  const wdUInt32 uiVertexOffset = m_Vertices.GetCount();

  for (wdUInt32 v = 0; v < other.m_Vertices.GetCount(); ++v)
  {
    m_Vertices.PushBack(other.m_Vertices[v]);
  }

  for (wdUInt32 p = 0; p < other.m_Polygons.GetCount(); ++p)
  {
    m_Polygons.PushBack(other.m_Polygons[p]);
    Polygon& poly = m_Polygons.PeekBack();

    for (wdUInt32 pv = 0; pv < poly.m_Vertices.GetCount(); ++pv)
    {
      poly.m_Vertices[pv] += uiVertexOffset;
    }
  }

  for (wdUInt32 l = 0; l < other.m_Lines.GetCount(); ++l)
  {
    Line line;
    line.m_uiStartVertex = other.m_Lines[l].m_uiStartVertex + uiVertexOffset;
    line.m_uiEndVertex = other.m_Lines[l].m_uiEndVertex + uiVertexOffset;

    m_Lines.PushBack(line);
  }
}

void wdGeometry::AddRectXY(const wdVec2& vSize, wdUInt32 uiTesselationX, wdUInt32 uiTesselationY, const GeoOptions& options)
{
  if (uiTesselationX == 0)
    uiTesselationX = 1;
  if (uiTesselationY == 0)
    uiTesselationY = 1;

  const wdVec2 halfSize = vSize * 0.5f;
  const bool bFlipWinding = options.IsFlipWindingNecessary();

  const wdVec2 sizeFraction = vSize.CompDiv(wdVec2(static_cast<float>(uiTesselationX), static_cast<float>(uiTesselationY)));

  for (wdUInt32 vy = 0; vy < uiTesselationY + 1; ++vy)
  {
    for (wdUInt32 vx = 0; vx < uiTesselationX + 1; ++vx)
    {
      const wdVec2 tc((float)vx / (float)uiTesselationX, (float)vy / (float)uiTesselationY);

      AddVertex(wdVec3(-halfSize.x + vx * sizeFraction.x, -halfSize.y + vy * sizeFraction.y, 0), wdVec3(0, 0, 1), tc, options);
    }
  }

  wdUInt32 idx[4];

  wdUInt32 uiFirstIndex = 0;

  for (wdUInt32 vy = 0; vy < uiTesselationY; ++vy)
  {
    for (wdUInt32 vx = 0; vx < uiTesselationX; ++vx)
    {

      idx[0] = uiFirstIndex;
      idx[1] = uiFirstIndex + 1;
      idx[2] = uiFirstIndex + uiTesselationX + 2;
      idx[3] = uiFirstIndex + uiTesselationX + 1;

      AddPolygon(idx, bFlipWinding);

      ++uiFirstIndex;
    }

    ++uiFirstIndex;
  }
}

void wdGeometry::AddBox(const wdVec3& vFullExtents, bool bExtraVerticesForTexturing, const GeoOptions& options)
{
  const wdVec3 halfSize = vFullExtents * 0.5f;
  const bool bFlipWinding = options.IsFlipWindingNecessary();

  if (bExtraVerticesForTexturing)
  {
    wdUInt32 idx[4];

    {
      idx[0] = AddVertex(wdVec3(-halfSize.x, -halfSize.y, +halfSize.z), wdVec3(0, 0, 1), wdVec2(0, 1), options);
      idx[1] = AddVertex(wdVec3(+halfSize.x, -halfSize.y, +halfSize.z), wdVec3(0, 0, 1), wdVec2(0, 0), options);
      idx[2] = AddVertex(wdVec3(+halfSize.x, +halfSize.y, +halfSize.z), wdVec3(0, 0, 1), wdVec2(1, 0), options);
      idx[3] = AddVertex(wdVec3(-halfSize.x, +halfSize.y, +halfSize.z), wdVec3(0, 0, 1), wdVec2(1, 1), options);
      AddPolygon(idx, bFlipWinding);
    }

    {
      idx[0] = AddVertex(wdVec3(-halfSize.x, +halfSize.y, -halfSize.z), wdVec3(0, 0, -1), wdVec2(1, 0), options);
      idx[1] = AddVertex(wdVec3(+halfSize.x, +halfSize.y, -halfSize.z), wdVec3(0, 0, -1), wdVec2(1, 1), options);
      idx[2] = AddVertex(wdVec3(+halfSize.x, -halfSize.y, -halfSize.z), wdVec3(0, 0, -1), wdVec2(0, 1), options);
      idx[3] = AddVertex(wdVec3(-halfSize.x, -halfSize.y, -halfSize.z), wdVec3(0, 0, -1), wdVec2(0, 0), options);
      AddPolygon(idx, bFlipWinding);
    }

    {
      idx[0] = AddVertex(wdVec3(-halfSize.x, -halfSize.y, -halfSize.z), wdVec3(-1, 0, 0), wdVec2(0, 1), options);
      idx[1] = AddVertex(wdVec3(-halfSize.x, -halfSize.y, +halfSize.z), wdVec3(-1, 0, 0), wdVec2(0, 0), options);
      idx[2] = AddVertex(wdVec3(-halfSize.x, +halfSize.y, +halfSize.z), wdVec3(-1, 0, 0), wdVec2(1, 0), options);
      idx[3] = AddVertex(wdVec3(-halfSize.x, +halfSize.y, -halfSize.z), wdVec3(-1, 0, 0), wdVec2(1, 1), options);
      AddPolygon(idx, bFlipWinding);
    }

    {
      idx[0] = AddVertex(wdVec3(+halfSize.x, +halfSize.y, -halfSize.z), wdVec3(1, 0, 0), wdVec2(0, 1), options);
      idx[1] = AddVertex(wdVec3(+halfSize.x, +halfSize.y, +halfSize.z), wdVec3(1, 0, 0), wdVec2(0, 0), options);
      idx[2] = AddVertex(wdVec3(+halfSize.x, -halfSize.y, +halfSize.z), wdVec3(1, 0, 0), wdVec2(1, 0), options);
      idx[3] = AddVertex(wdVec3(+halfSize.x, -halfSize.y, -halfSize.z), wdVec3(1, 0, 0), wdVec2(1, 1), options);
      AddPolygon(idx, bFlipWinding);
    }

    {
      idx[0] = AddVertex(wdVec3(+halfSize.x, -halfSize.y, -halfSize.z), wdVec3(0, -1, 0), wdVec2(0, 1), options);
      idx[1] = AddVertex(wdVec3(+halfSize.x, -halfSize.y, +halfSize.z), wdVec3(0, -1, 0), wdVec2(0, 0), options);
      idx[2] = AddVertex(wdVec3(-halfSize.x, -halfSize.y, +halfSize.z), wdVec3(0, -1, 0), wdVec2(1, 0), options);
      idx[3] = AddVertex(wdVec3(-halfSize.x, -halfSize.y, -halfSize.z), wdVec3(0, -1, 0), wdVec2(1, 1), options);
      AddPolygon(idx, bFlipWinding);
    }

    {
      idx[0] = AddVertex(wdVec3(-halfSize.x, +halfSize.y, -halfSize.z), wdVec3(0, +1, 0), wdVec2(0, 1), options);
      idx[1] = AddVertex(wdVec3(-halfSize.x, +halfSize.y, +halfSize.z), wdVec3(0, +1, 0), wdVec2(0, 0), options);
      idx[2] = AddVertex(wdVec3(+halfSize.x, +halfSize.y, +halfSize.z), wdVec3(0, +1, 0), wdVec2(1, 0), options);
      idx[3] = AddVertex(wdVec3(+halfSize.x, +halfSize.y, -halfSize.z), wdVec3(0, +1, 0), wdVec2(1, 1), options);
      AddPolygon(idx, bFlipWinding);
    }
  }
  else
  {
    wdUInt32 idx[8];

    idx[0] = AddVertex(wdVec3(-halfSize.x, -halfSize.y, halfSize.z), wdVec3(0, 0, 1), wdVec2(0), options);
    idx[1] = AddVertex(wdVec3(halfSize.x, -halfSize.y, halfSize.z), wdVec3(0, 0, 1), wdVec2(0), options);
    idx[2] = AddVertex(wdVec3(halfSize.x, halfSize.y, halfSize.z), wdVec3(0, 0, 1), wdVec2(0), options);
    idx[3] = AddVertex(wdVec3(-halfSize.x, halfSize.y, halfSize.z), wdVec3(0, 0, 1), wdVec2(0), options);

    idx[4] = AddVertex(wdVec3(-halfSize.x, -halfSize.y, -halfSize.z), wdVec3(0, 0, -1), wdVec2(0), options);
    idx[5] = AddVertex(wdVec3(halfSize.x, -halfSize.y, -halfSize.z), wdVec3(0, 0, -1), wdVec2(0), options);
    idx[6] = AddVertex(wdVec3(halfSize.x, halfSize.y, -halfSize.z), wdVec3(0, 0, -1), wdVec2(0), options);
    idx[7] = AddVertex(wdVec3(-halfSize.x, halfSize.y, -halfSize.z), wdVec3(0, 0, -1), wdVec2(0), options);

    wdUInt32 poly[4];

    poly[0] = idx[0];
    poly[1] = idx[1];
    poly[2] = idx[2];
    poly[3] = idx[3];
    AddPolygon(poly, bFlipWinding);

    poly[0] = idx[1];
    poly[1] = idx[5];
    poly[2] = idx[6];
    poly[3] = idx[2];
    AddPolygon(poly, bFlipWinding);

    poly[0] = idx[5];
    poly[1] = idx[4];
    poly[2] = idx[7];
    poly[3] = idx[6];
    AddPolygon(poly, bFlipWinding);

    poly[0] = idx[4];
    poly[1] = idx[0];
    poly[2] = idx[3];
    poly[3] = idx[7];
    AddPolygon(poly, bFlipWinding);

    poly[0] = idx[4];
    poly[1] = idx[5];
    poly[2] = idx[1];
    poly[3] = idx[0];
    AddPolygon(poly, bFlipWinding);

    poly[0] = idx[3];
    poly[1] = idx[2];
    poly[2] = idx[6];
    poly[3] = idx[7];
    AddPolygon(poly, bFlipWinding);
  }
}

void wdGeometry::AddLineBox(const wdVec3& vSize, const GeoOptions& options)
{
  const wdVec3 halfSize = vSize * 0.5f;

  AddVertex(wdVec3(-halfSize.x, -halfSize.y, halfSize.z), wdVec3(0, 0, 1), wdVec2(0), options);
  AddVertex(wdVec3(halfSize.x, -halfSize.y, halfSize.z), wdVec3(0, 0, 1), wdVec2(0), options);
  AddVertex(wdVec3(halfSize.x, halfSize.y, halfSize.z), wdVec3(0, 0, 1), wdVec2(0), options);
  AddVertex(wdVec3(-halfSize.x, halfSize.y, halfSize.z), wdVec3(0, 0, 1), wdVec2(0), options);

  AddVertex(wdVec3(-halfSize.x, -halfSize.y, -halfSize.z), wdVec3(0, 0, -1), wdVec2(0), options);
  AddVertex(wdVec3(halfSize.x, -halfSize.y, -halfSize.z), wdVec3(0, 0, -1), wdVec2(0), options);
  AddVertex(wdVec3(halfSize.x, halfSize.y, -halfSize.z), wdVec3(0, 0, -1), wdVec2(0), options);
  AddVertex(wdVec3(-halfSize.x, halfSize.y, -halfSize.z), wdVec3(0, 0, -1), wdVec2(0), options);

  AddLine(0, 1);
  AddLine(1, 2);
  AddLine(2, 3);
  AddLine(3, 0);

  AddLine(4, 5);
  AddLine(5, 6);
  AddLine(6, 7);
  AddLine(7, 4);

  AddLine(0, 4);
  AddLine(1, 5);
  AddLine(2, 6);
  AddLine(3, 7);
}

void wdGeometry::AddLineBoxCorners(const wdVec3& vSize, float fCornerFraction, const GeoOptions& options)
{
  WD_ASSERT_DEV(fCornerFraction >= 0.0f && fCornerFraction <= 1.0f, "A fraction value of {0} is invalid", wdArgF(fCornerFraction, 2));

  fCornerFraction *= 0.5f;
  const wdVec3 halfSize = vSize * 0.5f;

  AddVertex(wdVec3(-halfSize.x, -halfSize.y, halfSize.z), wdVec3(0, 0, 1), wdVec2(0), options);
  AddVertex(wdVec3(halfSize.x, -halfSize.y, halfSize.z), wdVec3(0, 0, 1), wdVec2(0), options);
  AddVertex(wdVec3(halfSize.x, halfSize.y, halfSize.z), wdVec3(0, 0, 1), wdVec2(0), options);
  AddVertex(wdVec3(-halfSize.x, halfSize.y, halfSize.z), wdVec3(0, 0, 1), wdVec2(0), options);

  AddVertex(wdVec3(-halfSize.x, -halfSize.y, -halfSize.z), wdVec3(0, 0, -1), wdVec2(0), options);
  AddVertex(wdVec3(halfSize.x, -halfSize.y, -halfSize.z), wdVec3(0, 0, -1), wdVec2(0), options);
  AddVertex(wdVec3(halfSize.x, halfSize.y, -halfSize.z), wdVec3(0, 0, -1), wdVec2(0), options);
  AddVertex(wdVec3(-halfSize.x, halfSize.y, -halfSize.z), wdVec3(0, 0, -1), wdVec2(0), options);

  for (wdUInt32 c = 0; c < 8; ++c)
  {
    const wdVec3& op = m_Vertices[c].m_vPosition;

    const wdVec3 op1 = wdVec3(op.x, op.y, -wdMath::Sign(op.z) * wdMath::Abs(op.z));
    const wdVec3 op2 = wdVec3(op.x, -wdMath::Sign(op.y) * wdMath::Abs(op.y), op.z);
    const wdVec3 op3 = wdVec3(-wdMath::Sign(op.x) * wdMath::Abs(op.x), op.y, op.z);

    const wdUInt32 ix1 = AddVertex(wdMath::Lerp(op, op1, fCornerFraction), m_Vertices[c].m_vPosition, m_Vertices[c].m_vTexCoord, options);
    const wdUInt32 ix2 = AddVertex(wdMath::Lerp(op, op2, fCornerFraction), m_Vertices[c].m_vPosition, m_Vertices[c].m_vTexCoord, options);
    const wdUInt32 ix3 = AddVertex(wdMath::Lerp(op, op3, fCornerFraction), m_Vertices[c].m_vPosition, m_Vertices[c].m_vTexCoord, options);

    AddLine(c, ix1);
    AddLine(c, ix2);
    AddLine(c, ix3);
  }
}

void wdGeometry::AddPyramid(const wdVec3& vSize, bool bCap, const GeoOptions& options)
{
  const wdVec3 halfSize = vSize * 0.5f;
  const bool bFlipWinding = options.IsFlipWindingNecessary();
  wdUInt32 quad[4];

  quad[0] = AddVertex(wdVec3(-halfSize.x, halfSize.y, 0), wdVec3(-1, 1, 0).GetNormalized(), wdVec2(0), options);
  quad[1] = AddVertex(wdVec3(halfSize.x, halfSize.y, 0), wdVec3(1, 1, 0).GetNormalized(), wdVec2(0), options);
  quad[2] = AddVertex(wdVec3(halfSize.x, -halfSize.y, 0), wdVec3(1, -1, 0).GetNormalized(), wdVec2(0), options);
  quad[3] = AddVertex(wdVec3(-halfSize.x, -halfSize.y, 0), wdVec3(-1, -1, 0).GetNormalized(), wdVec2(0), options);

  const wdUInt32 tip = AddVertex(wdVec3(0, 0, vSize.z), wdVec3(0, 0, 1), wdVec2(0), options);

  if (bCap)
  {
    AddPolygon(quad, bFlipWinding);
  }

  wdUInt32 tri[3];

  tri[0] = quad[1];
  tri[1] = quad[0];
  tri[2] = tip;
  AddPolygon(tri, bFlipWinding);

  tri[0] = quad[2];
  tri[1] = quad[1];
  tri[2] = tip;
  AddPolygon(tri, bFlipWinding);

  tri[0] = quad[3];
  tri[1] = quad[2];
  tri[2] = tip;
  AddPolygon(tri, bFlipWinding);

  tri[0] = quad[0];
  tri[1] = quad[3];
  tri[2] = tip;
  AddPolygon(tri, bFlipWinding);
}

void wdGeometry::AddGeodesicSphere(float fRadius, wdUInt8 uiSubDivisions, const GeoOptions& options)
{
  const bool bFlipWinding = options.IsFlipWindingNecessary();
  struct Triangle
  {
    Triangle(wdUInt32 ui1, wdUInt32 ui2, wdUInt32 ui3)
    {
      m_uiIndex[0] = ui1;
      m_uiIndex[1] = ui2;
      m_uiIndex[2] = ui3;
    }

    wdUInt32 m_uiIndex[3];
  };

  struct Edge
  {
    Edge() = default;

    Edge(wdUInt32 uiId1, wdUInt32 uiId2)
    {
      m_uiVertex[0] = wdMath::Min(uiId1, uiId2);
      m_uiVertex[1] = wdMath::Max(uiId1, uiId2);
    }

    bool operator<(const Edge& rhs) const
    {
      if (m_uiVertex[0] < rhs.m_uiVertex[0])
        return true;
      if (m_uiVertex[0] > rhs.m_uiVertex[0])
        return false;
      return m_uiVertex[1] < rhs.m_uiVertex[1];
    }

    bool operator==(const Edge& rhs) const { return m_uiVertex[0] == rhs.m_uiVertex[0] && m_uiVertex[1] == rhs.m_uiVertex[1]; }

    wdUInt32 m_uiVertex[2];
  };

  const wdUInt32 uiFirstVertex = m_Vertices.GetCount();

  wdInt32 iCurrentList = 0;
  wdDeque<Triangle> Tris[2];
  wdVec4U16 boneIndices(options.m_uiBoneIndex, 0, 0, 0);

  // create icosahedron
  {
    wdMat3 mRotX, mRotZ, mRotZh;
    mRotX.SetRotationMatrixX(wdAngle::Degree(360.0f / 6.0f));
    mRotZ.SetRotationMatrixZ(wdAngle::Degree(-360.0f / 5.0f));
    mRotZh.SetRotationMatrixZ(wdAngle::Degree(-360.0f / 10.0f));

    wdUInt32 vert[12];
    wdVec3 vDir(0, 0, 1);

    vDir.Normalize();
    vert[0] = AddVertex(vDir * fRadius, vDir, wdVec2::ZeroVector(), options.m_Color, boneIndices);

    vDir = mRotX * vDir;

    for (wdInt32 i = 0; i < 5; ++i)
    {
      vDir.Normalize();
      vert[1 + i] = AddVertex(vDir * fRadius, vDir, wdVec2::ZeroVector(), options.m_Color, boneIndices);
      vDir = mRotZ * vDir;
    }

    vDir = mRotX * vDir;
    vDir = mRotZh * vDir;

    for (wdInt32 i = 0; i < 5; ++i)
    {
      vDir.Normalize();
      vert[6 + i] = AddVertex(vDir * fRadius, vDir, wdVec2::ZeroVector(), options.m_Color, boneIndices);
      vDir = mRotZ * vDir;
    }

    vDir.Set(0, 0, -1);
    vDir.Normalize();
    vert[11] = AddVertex(vDir * fRadius, vDir, wdVec2::ZeroVector(), options.m_Color, boneIndices);


    Tris[0].PushBack(Triangle(vert[0], vert[2], vert[1]));
    Tris[0].PushBack(Triangle(vert[0], vert[3], vert[2]));
    Tris[0].PushBack(Triangle(vert[0], vert[4], vert[3]));
    Tris[0].PushBack(Triangle(vert[0], vert[5], vert[4]));
    Tris[0].PushBack(Triangle(vert[0], vert[1], vert[5]));

    Tris[0].PushBack(Triangle(vert[1], vert[2], vert[6]));
    Tris[0].PushBack(Triangle(vert[2], vert[3], vert[7]));
    Tris[0].PushBack(Triangle(vert[3], vert[4], vert[8]));
    Tris[0].PushBack(Triangle(vert[4], vert[5], vert[9]));
    Tris[0].PushBack(Triangle(vert[5], vert[1], vert[10]));

    Tris[0].PushBack(Triangle(vert[2], vert[7], vert[6]));
    Tris[0].PushBack(Triangle(vert[3], vert[8], vert[7]));
    Tris[0].PushBack(Triangle(vert[4], vert[9], vert[8]));
    Tris[0].PushBack(Triangle(vert[5], vert[10], vert[9]));
    Tris[0].PushBack(Triangle(vert[6], vert[10], vert[1]));

    Tris[0].PushBack(Triangle(vert[7], vert[11], vert[6]));
    Tris[0].PushBack(Triangle(vert[8], vert[11], vert[7]));
    Tris[0].PushBack(Triangle(vert[9], vert[11], vert[8]));
    Tris[0].PushBack(Triangle(vert[10], vert[11], vert[9]));
    Tris[0].PushBack(Triangle(vert[6], vert[11], vert[10]));
  }

  wdMap<Edge, wdUInt32> NewVertices;

  // subdivide the icosahedron n times (splitting every triangle into 4 new triangles)
  for (wdUInt32 div = 0; div < uiSubDivisions; ++div)
  {
    // switch the last result and the new result
    const wdInt32 iPrevList = iCurrentList;
    iCurrentList = (iCurrentList + 1) % 2;

    Tris[iCurrentList].Clear();
    NewVertices.Clear();

    for (wdUInt32 tri = 0; tri < Tris[iPrevList].GetCount(); ++tri)
    {
      wdUInt32 uiVert[3] = {Tris[iPrevList][tri].m_uiIndex[0], Tris[iPrevList][tri].m_uiIndex[1], Tris[iPrevList][tri].m_uiIndex[2]};

      Edge Edges[3] = {Edge(uiVert[0], uiVert[1]), Edge(uiVert[1], uiVert[2]), Edge(uiVert[2], uiVert[0])};

      wdUInt32 uiNewVert[3];

      // split each edge of the triangle in half
      for (wdUInt32 i = 0; i < 3; ++i)
      {
        // do not split an edge that was split before, we want shared vertices everywhere
        if (NewVertices.Find(Edges[i]).IsValid())
          uiNewVert[i] = NewVertices[Edges[i]];
        else
        {
          const wdVec3 vCenter = (m_Vertices[Edges[i].m_uiVertex[0]].m_vPosition + m_Vertices[Edges[i].m_uiVertex[1]].m_vPosition).GetNormalized();
          uiNewVert[i] = AddVertex(vCenter * fRadius, vCenter, wdVec2::ZeroVector(), options.m_Color, boneIndices);

          NewVertices[Edges[i]] = uiNewVert[i];
        }
      }

      // now we turn one triangle into 4 smaller ones
      Tris[iCurrentList].PushBack(Triangle(uiVert[0], uiNewVert[0], uiNewVert[2]));
      Tris[iCurrentList].PushBack(Triangle(uiNewVert[0], uiVert[1], uiNewVert[1]));
      Tris[iCurrentList].PushBack(Triangle(uiNewVert[1], uiVert[2], uiNewVert[2]));

      Tris[iCurrentList].PushBack(Triangle(uiNewVert[0], uiNewVert[1], uiNewVert[2]));
    }
  }

  // add the final list of triangles to the output
  for (wdUInt32 tri = 0; tri < Tris[iCurrentList].GetCount(); ++tri)
  {
    AddPolygon(Tris[iCurrentList][tri].m_uiIndex, bFlipWinding);
  }

  // finally apply the user transformation on the new vertices
  TransformVertices(options.m_Transform, uiFirstVertex);
}

void wdGeometry::AddCylinder(float fRadiusTop, float fRadiusBottom, float fPositiveLength, float fNegativeLength, bool bCapTop, bool bCapBottom, wdUInt16 uiSegments, const GeoOptions& options, wdAngle fraction /*= wdAngle::Degree(360.0f)*/)
{
  WD_ASSERT_DEV(uiSegments >= 3, "Cannot create a cylinder with only {0} segments", uiSegments);
  WD_ASSERT_DEV(fraction.GetDegree() >= -0.01f, "A cylinder cannot be built with more less than 0 degree");
  WD_ASSERT_DEV(fraction.GetDegree() <= 360.01f, "A cylinder cannot be built with more than 360 degree");

  fraction = wdMath::Clamp(fraction, wdAngle(), wdAngle::Degree(360.0f));

  const bool bFlipWinding = options.IsFlipWindingNecessary();
  const bool bIsFraction = fraction.GetDegree() < 360.0f;
  const wdAngle fDegStep = wdAngle::Degree(fraction.GetDegree() / uiSegments);

  const wdVec3 vTopCenter(0, 0, fPositiveLength);
  const wdVec3 vBottomCenter(0, 0, -fNegativeLength);

  // cylinder wall
  {
    wdHybridArray<wdUInt32, 512> VertsTop;
    wdHybridArray<wdUInt32, 512> VertsBottom;

    for (wdInt32 i = 0; i <= uiSegments; ++i)
    {
      const wdAngle deg = (float)i * fDegStep;

      float fU = 4.0f - deg.GetDegree() / 90.0f;

      const float fX = wdMath::Cos(deg);
      const float fY = wdMath::Sin(deg);

      const wdVec3 vDir(fX, fY, 0);

      VertsTop.PushBack(AddVertex(vTopCenter + vDir * fRadiusTop, vDir, wdVec2(fU, 0), options));
      VertsBottom.PushBack(AddVertex(vBottomCenter + vDir * fRadiusBottom, vDir, wdVec2(fU, 1), options));
    }

    for (wdUInt32 i = 1; i <= uiSegments; ++i)
    {
      wdUInt32 quad[4];
      quad[0] = VertsBottom[i - 1];
      quad[1] = VertsBottom[i];
      quad[2] = VertsTop[i];
      quad[3] = VertsTop[i - 1];


      AddPolygon(quad, bFlipWinding);
    }
  }

  // walls for fractional cylinders
  if (bIsFraction)
  {
    const wdVec3 vDir0(1, 0, 0);
    const wdVec3 vDir1(wdMath::Cos(fraction), wdMath::Sin(fraction), 0);

    wdUInt32 quad[4];

    const wdVec3 vNrm0 = -wdVec3(0, 0, 1).CrossRH(vDir0).GetNormalized();
    quad[0] = AddVertex(vTopCenter + vDir0 * fRadiusTop, vNrm0, wdVec2(0, 0), options);
    quad[1] = AddVertex(vTopCenter, vNrm0, wdVec2(1, 0), options);
    quad[2] = AddVertex(vBottomCenter, vNrm0, wdVec2(1, 1), options);
    quad[3] = AddVertex(vBottomCenter + vDir0 * fRadiusBottom, vNrm0, wdVec2(0, 1), options);


    AddPolygon(quad, bFlipWinding);

    const wdVec3 vNrm1 = wdVec3(0, 0, 1).CrossRH(vDir1).GetNormalized();
    quad[0] = AddVertex(vTopCenter, vNrm1, wdVec2(0, 0), options);
    quad[1] = AddVertex(vTopCenter + vDir1 * fRadiusTop, vNrm1, wdVec2(1, 0), options);
    quad[2] = AddVertex(vBottomCenter + vDir1 * fRadiusBottom, vNrm1, wdVec2(1, 1), options);
    quad[3] = AddVertex(vBottomCenter, vNrm1, wdVec2(0, 1), options);

    AddPolygon(quad, bFlipWinding);
  }

  if (bCapBottom)
  {
    wdHybridArray<wdUInt32, 512> VertsBottom;

    if (bIsFraction)
    {
      const wdUInt32 uiCenterVtx = AddVertex(vBottomCenter, wdVec3(0, 0, -1), wdVec2(0), options);

      for (wdInt32 i = uiSegments; i >= 0; --i)
      {
        const wdAngle deg = (float)i * fDegStep;

        const float fX = wdMath::Cos(deg);
        const float fY = wdMath::Sin(deg);

        const wdVec3 vDir(fX, fY, 0);

        AddVertex(vBottomCenter + vDir * fRadiusBottom, wdVec3(0, 0, -1), wdVec2(fY, fX), options);
      }

      VertsBottom.SetCountUninitialized(3);
      VertsBottom[0] = uiCenterVtx;

      for (wdUInt32 i = 0; i < uiSegments; ++i)
      {
        VertsBottom[1] = uiCenterVtx + i + 1;
        VertsBottom[2] = uiCenterVtx + i + 2;

        AddPolygon(VertsBottom, bFlipWinding);
      }
    }
    else
    {
      for (wdInt32 i = uiSegments - 1; i >= 0; --i)
      {
        const wdAngle deg = (float)i * fDegStep;

        const float fX = wdMath::Cos(deg);
        const float fY = wdMath::Sin(deg);

        const wdVec3 vDir(fX, fY, 0);

        VertsBottom.PushBack(AddVertex(vBottomCenter + vDir * fRadiusBottom, wdVec3(0, 0, -1), wdVec2(fY, fX), options));
      }

      AddPolygon(VertsBottom, bFlipWinding);
    }
  }

  if (bCapTop)
  {
    wdHybridArray<wdUInt32, 512> VertsTop;

    if (bIsFraction)
    {
      const wdUInt32 uiCenterVtx = AddVertex(vTopCenter, wdVec3(0, 0, 1), wdVec2(0), options);

      for (wdInt32 i = 0; i <= uiSegments; ++i)
      {
        const wdAngle deg = (float)i * fDegStep;

        const float fX = wdMath::Cos(deg);
        const float fY = wdMath::Sin(deg);

        const wdVec3 vDir(fX, fY, 0);

        AddVertex(vTopCenter + vDir * fRadiusTop, wdVec3(0, 0, 1), wdVec2(fY, -fX), options);
      }

      VertsTop.SetCountUninitialized(3);
      VertsTop[0] = uiCenterVtx;

      for (wdUInt32 i = 0; i < uiSegments; ++i)
      {
        VertsTop[1] = uiCenterVtx + i + 1;
        VertsTop[2] = uiCenterVtx + i + 2;

        AddPolygon(VertsTop, bFlipWinding);
      }
    }
    else
    {
      for (wdInt32 i = 0; i < uiSegments; ++i)
      {
        const wdAngle deg = (float)i * fDegStep;

        const float fX = wdMath::Cos(deg);
        const float fY = wdMath::Sin(deg);

        const wdVec3 vDir(fX, fY, 0);

        VertsTop.PushBack(AddVertex(vTopCenter + vDir * fRadiusTop, wdVec3(0, 0, 1), wdVec2(fY, -fX), options));
      }

      AddPolygon(VertsTop, bFlipWinding);
    }
  }
}

void wdGeometry::AddCylinderOnePiece(float fRadiusTop, float fRadiusBottom, float fPositiveLength, float fNegativeLength, wdUInt16 uiSegments, const GeoOptions& options)
{
  WD_ASSERT_DEV(uiSegments >= 3, "Cannot create a cylinder with only {0} segments", uiSegments);

  const bool bFlipWinding = options.IsFlipWindingNecessary();
  const wdAngle fDegStep = wdAngle::Degree(360.0f / uiSegments);

  const wdVec3 vTopCenter(0, 0, fPositiveLength);
  const wdVec3 vBottomCenter(0, 0, -fNegativeLength);

  // cylinder wall
  {
    wdHybridArray<wdUInt32, 512> VertsTop;
    wdHybridArray<wdUInt32, 512> VertsBottom;

    for (wdInt32 i = 0; i < uiSegments; ++i)
    {
      const wdAngle deg = (float)i * fDegStep;

      float fU = 4.0f - deg.GetDegree() / 90.0f;

      const float fX = wdMath::Cos(deg);
      const float fY = wdMath::Sin(deg);

      const wdVec3 vDir(fX, fY, 0);

      VertsTop.PushBack(AddVertex(vTopCenter + vDir * fRadiusTop, vDir, wdVec2(fU, 0), options));
      VertsBottom.PushBack(AddVertex(vBottomCenter + vDir * fRadiusBottom, vDir, wdVec2(fU, 1), options));
    }

    for (wdUInt32 i = 1; i <= uiSegments; ++i)
    {
      wdUInt32 quad[4];
      quad[0] = VertsBottom[i - 1];
      quad[1] = VertsBottom[i % uiSegments];
      quad[2] = VertsTop[i % uiSegments];
      quad[3] = VertsTop[i - 1];

      AddPolygon(quad, bFlipWinding);
    }

    AddPolygon(VertsTop, bFlipWinding);
    AddPolygon(VertsBottom, !bFlipWinding);
  }
}

void wdGeometry::AddCone(float fRadius, float fHeight, bool bCap, wdUInt16 uiSegments, const GeoOptions& options)
{
  WD_ASSERT_DEV(uiSegments >= 3, "Cannot create a cone with only {0} segments", uiSegments);

  const bool bFlipWinding = options.IsFlipWindingNecessary();

  wdHybridArray<wdUInt32, 512> VertsBottom;

  const wdAngle fDegStep = wdAngle::Degree(360.0f / uiSegments);

  const wdUInt32 uiTip = AddVertex(wdVec3(0, 0, fHeight), wdVec3(0, 0, 1), wdVec2(0), options);

  for (wdInt32 i = uiSegments - 1; i >= 0; --i)
  {
    const wdAngle deg = (float)i * fDegStep;

    wdVec3 vDir(wdMath::Cos(deg), wdMath::Sin(deg), 0);

    VertsBottom.PushBack(AddVertex(vDir * fRadius, vDir, wdVec2(0), options));
  }

  wdUInt32 uiPrevSeg = uiSegments - 1;

  for (wdUInt32 i = 0; i < uiSegments; ++i)
  {
    wdUInt32 tri[3];
    tri[0] = VertsBottom[uiPrevSeg];
    tri[1] = uiTip;
    tri[2] = VertsBottom[i];

    uiPrevSeg = i;

    AddPolygon(tri, bFlipWinding);
  }

  if (bCap)
  {
    AddPolygon(VertsBottom, bFlipWinding);
  }
}

void wdGeometry::AddSphere(float fRadius, wdUInt16 uiSegments, wdUInt16 uiStacks, const GeoOptions& options)
{
  WD_ASSERT_DEV(uiSegments >= 3, "Sphere must have at least 3 segments");
  WD_ASSERT_DEV(uiStacks >= 2, "Sphere must have at least 2 stacks");

  const bool bFlipWinding = options.IsFlipWindingNecessary();
  const wdAngle fDegreeDiffSegments = wdAngle::Degree(360.0f / (float)(uiSegments));
  const wdAngle fDegreeDiffStacks = wdAngle::Degree(180.0f / (float)(uiStacks));

  const wdUInt32 uiFirstVertex = m_Vertices.GetCount();

  // first create all the vertex positions
  for (wdUInt32 st = 1; st < uiStacks; ++st)
  {
    const wdAngle fDegreeStack = wdAngle::Degree(-90.0f + (st * fDegreeDiffStacks.GetDegree()));
    const float fCosDS = wdMath::Cos(fDegreeStack);
    const float fSinDS = wdMath::Sin(fDegreeStack);
    const float fY = -fSinDS * fRadius;

    const float fV = (float)st / (float)uiStacks;

    for (wdUInt32 sp = 0; sp < uiSegments + 1u; ++sp)
    {
      float fU = ((float)sp / (float)(uiSegments)) * 2.0f;

      const wdAngle fDegree = (float)sp * fDegreeDiffSegments;

      wdVec3 vPos;
      vPos.x = wdMath::Cos(fDegree) * fRadius * fCosDS;
      vPos.y = -wdMath::Sin(fDegree) * fRadius * fCosDS;
      vPos.z = fY;

      wdVec3 vNormal = vPos;
      vNormal.NormalizeIfNotZero(wdVec3(0, 0, 1)).IgnoreResult();
      AddVertex(vPos, vNormal, wdVec2(fU, fV), options);
    }
  }

  wdUInt32 tri[3];
  wdUInt32 quad[4];

  // now create the top cone
  for (wdUInt32 p = 0; p < uiSegments; ++p)
  {
    float fU = ((p + 0.5f) / (float)(uiSegments)) * 2.0f;

    tri[0] = AddVertex(wdVec3(0, 0, fRadius), wdVec3(0, 0, 1), wdVec2(fU, 0), options);
    tri[1] = uiFirstVertex + p + 1;
    tri[2] = uiFirstVertex + p;

    AddPolygon(tri, bFlipWinding);
  }

  // now create the stacks in the middle
  for (wdUInt16 st = 0; st < uiStacks - 2; ++st)
  {
    const wdUInt32 uiRowBottom = (uiSegments + 1) * st;
    const wdUInt32 uiRowTop = (uiSegments + 1) * (st + 1);

    for (wdInt32 i = 0; i < uiSegments; ++i)
    {
      quad[0] = uiFirstVertex + (uiRowTop + i + 1);
      quad[1] = uiFirstVertex + (uiRowTop + i);
      quad[2] = uiFirstVertex + (uiRowBottom + i);
      quad[3] = uiFirstVertex + (uiRowBottom + i + 1);

      AddPolygon(quad, bFlipWinding);
    }
  }

  const wdInt32 iTopStack = (uiSegments + 1) * (uiStacks - 2);

  // now create the bottom cone
  for (wdUInt32 p = 0; p < uiSegments; ++p)
  {
    float fU = ((p + 0.5f) / (float)(uiSegments)) * 2.0f;

    tri[0] = AddVertex(wdVec3(0, 0, -fRadius), wdVec3(0, 0, -1), wdVec2(fU, 1), options);
    tri[1] = uiFirstVertex + (iTopStack + p);
    tri[2] = uiFirstVertex + (iTopStack + p + 1);

    AddPolygon(tri, bFlipWinding);
  }
}

void wdGeometry::AddHalfSphere(float fRadius, wdUInt16 uiSegments, wdUInt16 uiStacks, bool bCap, const GeoOptions& options)
{
  WD_ASSERT_DEV(uiSegments >= 3, "Sphere must have at least 3 segments");
  WD_ASSERT_DEV(uiStacks >= 1, "Sphere must have at least 1 stacks");

  const bool bFlipWinding = options.IsFlipWindingNecessary();
  const wdAngle fDegreeDiffSegments = wdAngle::Degree(360.0f / (float)(uiSegments));
  const wdAngle fDegreeDiffStacks = wdAngle::Degree(90.0f / (float)(uiStacks));

  const wdUInt32 uiFirstVertex = m_Vertices.GetCount();

  // first create all the vertex positions
  for (wdUInt32 st = 0; st < uiStacks; ++st)
  {
    const wdAngle fDegreeStack = wdAngle::Degree(-90.0f + ((st + 1) * fDegreeDiffStacks.GetDegree()));
    const float fCosDS = wdMath::Cos(fDegreeStack);
    const float fSinDS = wdMath::Sin(fDegreeStack);
    const float fY = -fSinDS * fRadius;

    const float fV = (float)(st + 1) / (float)uiStacks;

    for (wdUInt32 sp = 0; sp <= uiSegments; ++sp)
    {
      float fU = ((float)sp / (float)(uiSegments)) * 2.0f;

      if (fU > 1.0f)
        fU = 2.0f - fU;

      // the vertices for the bottom disk
      const wdAngle fDegree = (float)sp * fDegreeDiffSegments;

      wdVec3 vPos;
      vPos.x = wdMath::Cos(fDegree) * fRadius * fCosDS;
      vPos.y = wdMath::Sin(fDegree) * fRadius * fCosDS;
      vPos.z = fY;

      AddVertex(vPos, vPos.GetNormalized(), wdVec2(fU, fV), options);
    }
  }

  wdUInt32 uiTopVertex = AddVertex(wdVec3(0, 0, fRadius), wdVec3(0, 0, 1), wdVec2(0.0f), options);

  wdUInt32 tri[3];
  wdUInt32 quad[4];

  // now create the top cone
  for (wdUInt32 p = 0; p < uiSegments; ++p)
  {
    tri[0] = uiTopVertex;
    tri[1] = uiFirstVertex + p;
    tri[2] = uiFirstVertex + ((p + 1) % (uiSegments + 1));

    AddPolygon(tri, bFlipWinding);
  }

  // now create the stacks in the middle

  for (wdUInt16 st = 0; st < uiStacks - 1; ++st)
  {
    const wdUInt32 uiRowBottom = (uiSegments + 1) * st;
    const wdUInt32 uiRowTop = (uiSegments + 1) * (st + 1);

    for (wdInt32 i = 0; i < uiSegments; ++i)
    {
      quad[0] = uiFirstVertex + (uiRowTop + ((i + 1) % (uiSegments + 1)));
      quad[1] = uiFirstVertex + (uiRowBottom + ((i + 1) % (uiSegments + 1)));
      quad[2] = uiFirstVertex + (uiRowBottom + i);
      quad[3] = uiFirstVertex + (uiRowTop + i);

      AddPolygon(quad, bFlipWinding);
    }
  }

  if (bCap)
  {
    wdHybridArray<wdUInt32, 256> uiCap;

    for (wdUInt32 i = uiTopVertex - 1; i >= uiTopVertex - uiSegments; --i)
      uiCap.PushBack(i);

    AddPolygon(uiCap, bFlipWinding);
  }
}

void wdGeometry::AddCapsule(float fRadius, float fHeight, wdUInt16 uiSegments, wdUInt16 uiStacks, const GeoOptions& options)
{
  WD_ASSERT_DEV(uiSegments >= 3, "Capsule must have at least 3 segments");
  WD_ASSERT_DEV(uiStacks >= 1, "Capsule must have at least 1 stacks");
  WD_ASSERT_DEV(fHeight >= 0.0f, "Height must be positive");

  const bool bFlipWinding = options.IsFlipWindingNecessary();
  const wdAngle fDegreeDiffStacks = wdAngle::Degree(90.0f / (float)(uiStacks));

  const wdUInt32 uiFirstVertex = m_Vertices.GetCount();

  // first create all the vertex positions
  const float fDegreeStepSlices = 360.0f / (float)(uiSegments);

  float fOffset = fHeight * 0.5f;

  // for (wdUInt32 h = 0; h < 2; ++h)
  {
    for (wdUInt32 st = 0; st < uiStacks; ++st)
    {
      const wdAngle fDegreeStack = wdAngle::Degree(-90.0f + ((st + 1) * fDegreeDiffStacks.GetDegree()));
      const float fCosDS = wdMath::Cos(fDegreeStack);
      const float fSinDS = wdMath::Sin(fDegreeStack);
      const float fY = -fSinDS * fRadius;

      for (wdUInt32 sp = 0; sp < uiSegments; ++sp)
      {
        const wdAngle fDegree = wdAngle::Degree(sp * fDegreeStepSlices);

        wdVec3 vPos;
        vPos.x = wdMath::Cos(fDegree) * fRadius * fCosDS;
        vPos.z = fY + fOffset;
        vPos.y = wdMath::Sin(fDegree) * fRadius * fCosDS;

        AddVertex(vPos, vPos.GetNormalized(), wdVec2(0), options);
      }
    }

    fOffset -= fHeight;

    for (wdUInt32 st = 0; st < uiStacks; ++st)
    {
      const wdAngle fDegreeStack = wdAngle::Degree(0.0f - (st * fDegreeDiffStacks.GetDegree()));
      const float fCosDS = wdMath::Cos(fDegreeStack);
      const float fSinDS = wdMath::Sin(fDegreeStack);
      const float fY = fSinDS * fRadius;

      for (wdUInt32 sp = 0; sp < uiSegments; ++sp)
      {
        const wdAngle fDegree = wdAngle::Degree(sp * fDegreeStepSlices);

        wdVec3 vPos;
        vPos.x = wdMath::Cos(fDegree) * fRadius * fCosDS;
        vPos.z = fY + fOffset;
        vPos.y = wdMath::Sin(fDegree) * fRadius * fCosDS;

        AddVertex(vPos, vPos.GetNormalized(), wdVec2(0), options);
      }
    }
  }

  wdUInt32 uiTopVertex = AddVertex(wdVec3(0, 0, fRadius + fHeight * 0.5f), wdVec3(0, 0, 1), wdVec2(0), options);
  wdUInt32 uiBottomVertex = AddVertex(wdVec3(0, 0, -fRadius - fHeight * 0.5f), wdVec3(0, 0, -1), wdVec2(0), options);

  wdUInt32 tri[3];
  wdUInt32 quad[4];

  // now create the top cone
  for (wdUInt32 p = 0; p < uiSegments; ++p)
  {
    tri[0] = uiTopVertex;
    tri[2] = uiFirstVertex + ((p + 1) % uiSegments);
    tri[1] = uiFirstVertex + p;

    AddPolygon(tri, bFlipWinding);
  }

  // now create the stacks in the middle
  wdUInt16 uiMaxStacks = static_cast<wdUInt16>(uiStacks * 2 - 1);
  for (wdUInt16 st = 0; st < uiMaxStacks; ++st)
  {
    const wdUInt32 uiRowBottom = uiSegments * st;
    const wdUInt32 uiRowTop = uiSegments * (st + 1);

    for (wdInt32 i = 0; i < uiSegments; ++i)
    {
      quad[0] = uiFirstVertex + (uiRowTop + ((i + 1) % uiSegments));
      quad[3] = uiFirstVertex + (uiRowTop + i);
      quad[2] = uiFirstVertex + (uiRowBottom + i);
      quad[1] = uiFirstVertex + (uiRowBottom + ((i + 1) % uiSegments));

      AddPolygon(quad, bFlipWinding);
    }
  }

  const wdInt32 iBottomStack = uiSegments * (uiStacks * 2 - 1);

  // now create the bottom cone
  for (wdUInt32 p = 0; p < uiSegments; ++p)
  {
    tri[0] = uiBottomVertex;
    tri[2] = uiFirstVertex + (iBottomStack + p);
    tri[1] = uiFirstVertex + (iBottomStack + ((p + 1) % uiSegments));

    AddPolygon(tri, bFlipWinding);
  }
}

void wdGeometry::AddTorus(float fInnerRadius, float fOuterRadius, wdUInt16 uiSegments, wdUInt16 uiSegmentDetail, bool bExtraVerticesForTexturing, const GeoOptions& options)
{
  WD_ASSERT_DEV(fInnerRadius < fOuterRadius, "Inner radius must be smaller than outer radius. Doh!");
  WD_ASSERT_DEV(uiSegments >= 3, "Invalid number of segments.");
  WD_ASSERT_DEV(uiSegmentDetail >= 3, "Invalid segment detail value.");

  const bool bFlipWinding = options.IsFlipWindingNecessary();
  const float fCylinderRadius = (fOuterRadius - fInnerRadius) * 0.5f;
  const float fLoopRadius = fInnerRadius + fCylinderRadius;

  const wdAngle fAngleStepSegment = wdAngle::Degree(360.0f / uiSegments);
  const wdAngle fAngleStepCylinder = wdAngle::Degree(360.0f / uiSegmentDetail);

  const wdUInt16 uiFirstVertex = static_cast<wdUInt16>(m_Vertices.GetCount());

  const wdUInt16 uiNumSegments = bExtraVerticesForTexturing ? uiSegments + 1 : uiSegments;
  const wdUInt16 uiNumSegmentDetail = bExtraVerticesForTexturing ? uiSegmentDetail + 1 : uiSegmentDetail;

  // this is the loop for the torus ring
  for (wdUInt16 seg = 0; seg < uiNumSegments; ++seg)
  {
    float fU = ((float)seg / (float)uiSegments) * 2.0f;

    const wdAngle fAngle = seg * fAngleStepSegment;

    const float fSinAngle = wdMath::Sin(fAngle);
    const float fCosAngle = wdMath::Cos(fAngle);

    const wdVec3 vLoopPos = wdVec3(fSinAngle, fCosAngle, 0) * fLoopRadius;

    // this is the loop to go round the cylinder
    for (wdUInt16 p = 0; p < uiNumSegmentDetail; ++p)
    {
      float fV = (float)p / (float)uiSegmentDetail;

      const wdAngle fCylinderAngle = p * fAngleStepCylinder;

      const wdVec3 vDir(wdMath::Cos(fCylinderAngle) * fSinAngle, wdMath::Cos(fCylinderAngle) * fCosAngle, wdMath::Sin(fCylinderAngle));

      const wdVec3 vPos = vLoopPos + fCylinderRadius * vDir;

      AddVertex(vPos, vDir, wdVec2(fU, fV), options.m_Color, options.m_uiBoneIndex, options.m_Transform);
    }
  }

  if (bExtraVerticesForTexturing)
  {
    for (wdUInt16 seg = 0; seg < uiSegments; ++seg)
    {
      const wdUInt16 rs0 = uiFirstVertex + seg * (uiSegmentDetail + 1);
      const wdUInt16 rs1 = uiFirstVertex + (seg + 1) * (uiSegmentDetail + 1);

      for (wdUInt16 p = 0; p < uiSegmentDetail; ++p)
      {
        wdUInt32 quad[4];
        quad[0] = rs1 + p;
        quad[3] = rs1 + p + 1;
        quad[2] = rs0 + p + 1;
        quad[1] = rs0 + p;

        AddPolygon(quad, bFlipWinding);
      }
    }
  }
  else
  {
    wdUInt16 prevRing = (uiSegments - 1);

    for (wdUInt16 seg = 0; seg < uiSegments; ++seg)
    {
      const wdUInt16 thisRing = seg;

      const wdUInt16 prevRingFirstVtx = uiFirstVertex + (prevRing * uiSegmentDetail);
      wdUInt16 prevRingPrevVtx = prevRingFirstVtx + (uiSegmentDetail - 1);

      const wdUInt16 thisRingFirstVtx = uiFirstVertex + (thisRing * uiSegmentDetail);
      wdUInt16 thisRingPrevVtx = thisRingFirstVtx + (uiSegmentDetail - 1);

      for (wdUInt16 p = 0; p < uiSegmentDetail; ++p)
      {
        const wdUInt16 prevRingThisVtx = prevRingFirstVtx + p;
        const wdUInt16 thisRingThisVtx = thisRingFirstVtx + p;

        wdUInt32 quad[4];

        quad[0] = prevRingPrevVtx;
        quad[1] = prevRingThisVtx;
        quad[2] = thisRingThisVtx;
        quad[3] = thisRingPrevVtx;

        AddPolygon(quad, bFlipWinding);

        prevRingPrevVtx = prevRingThisVtx;
        thisRingPrevVtx = thisRingThisVtx;
      }

      prevRing = thisRing;
    }
  }
}

void wdGeometry::AddTexturedRamp(const wdVec3& vSize, const GeoOptions& options)
{
  const wdVec3 halfSize = vSize * 0.5f;
  const bool bFlipWinding = options.IsFlipWindingNecessary();
  wdUInt32 idx[4];
  wdUInt32 idx3[3];

  {
    wdVec3 vNormal = wdVec3(-halfSize.z, 0, halfSize.x).GetNormalized();
    idx[0] = AddVertex(wdVec3(-halfSize.x, -halfSize.y, -halfSize.z), vNormal, wdVec2(0, 1), options);
    idx[1] = AddVertex(wdVec3(+halfSize.x, -halfSize.y, +halfSize.z), vNormal, wdVec2(0, 0), options);
    idx[2] = AddVertex(wdVec3(+halfSize.x, +halfSize.y, +halfSize.z), vNormal, wdVec2(1, 0), options);
    idx[3] = AddVertex(wdVec3(-halfSize.x, +halfSize.y, -halfSize.z), vNormal, wdVec2(1, 1), options);
    AddPolygon(idx, bFlipWinding);
  }

  {
    idx[0] = AddVertex(wdVec3(-halfSize.x, +halfSize.y, -halfSize.z), wdVec3(0, 0, -1), wdVec2(1, 0), options);
    idx[1] = AddVertex(wdVec3(+halfSize.x, +halfSize.y, -halfSize.z), wdVec3(0, 0, -1), wdVec2(1, 1), options);
    idx[2] = AddVertex(wdVec3(+halfSize.x, -halfSize.y, -halfSize.z), wdVec3(0, 0, -1), wdVec2(0, 1), options);
    idx[3] = AddVertex(wdVec3(-halfSize.x, -halfSize.y, -halfSize.z), wdVec3(0, 0, -1), wdVec2(0, 0), options);
    AddPolygon(idx, bFlipWinding);
  }

  {
    idx[0] = AddVertex(wdVec3(+halfSize.x, +halfSize.y, -halfSize.z), wdVec3(1, 0, 0), wdVec2(0, 1), options);
    idx[1] = AddVertex(wdVec3(+halfSize.x, +halfSize.y, +halfSize.z), wdVec3(1, 0, 0), wdVec2(0, 0), options);
    idx[2] = AddVertex(wdVec3(+halfSize.x, -halfSize.y, +halfSize.z), wdVec3(1, 0, 0), wdVec2(1, 0), options);
    idx[3] = AddVertex(wdVec3(+halfSize.x, -halfSize.y, -halfSize.z), wdVec3(1, 0, 0), wdVec2(1, 1), options);
    AddPolygon(idx, bFlipWinding);
  }

  {
    idx3[0] = AddVertex(wdVec3(+halfSize.x, -halfSize.y, -halfSize.z), wdVec3(0, -1, 0), wdVec2(0, 1), options);
    idx3[1] = AddVertex(wdVec3(+halfSize.x, -halfSize.y, +halfSize.z), wdVec3(0, -1, 0), wdVec2(0, 0), options);
    idx3[2] = AddVertex(wdVec3(-halfSize.x, -halfSize.y, -halfSize.z), wdVec3(0, -1, 0), wdVec2(1, 1), options);
    AddPolygon(idx3, bFlipWinding);
  }

  {
    idx3[0] = AddVertex(wdVec3(-halfSize.x, +halfSize.y, -halfSize.z), wdVec3(0, +1, 0), wdVec2(0, 1), options);
    idx3[1] = AddVertex(wdVec3(+halfSize.x, +halfSize.y, +halfSize.z), wdVec3(0, +1, 0), wdVec2(1, 0), options);
    idx3[2] = AddVertex(wdVec3(+halfSize.x, +halfSize.y, -halfSize.z), wdVec3(0, +1, 0), wdVec2(1, 1), options);
    AddPolygon(idx3, bFlipWinding);
  }
}

void wdGeometry::AddStairs(const wdVec3& vSize, wdUInt32 uiNumSteps, wdAngle curvature, bool bSmoothSloped, const GeoOptions& options)
{
  const bool bFlipWinding = options.IsFlipWindingNecessary();

  curvature = wdMath::Clamp(curvature, -wdAngle::Degree(360), wdAngle::Degree(360));
  const wdAngle curveStep = curvature / (float)uiNumSteps;

  const float fStepDiv = 1.0f / uiNumSteps;
  const float fStepDepth = vSize.x / uiNumSteps;
  const float fStepHeight = vSize.z / uiNumSteps;

  wdVec3 vMoveFwd(fStepDepth, 0, 0);
  const wdVec3 vMoveUp(0, 0, fStepHeight);
  wdVec3 vMoveUpFwd(fStepDepth, 0, fStepHeight);

  wdVec3 vBaseL0(-vSize.x * 0.5f, -vSize.y * 0.5f, -vSize.z * 0.5f);
  wdVec3 vBaseL1(-vSize.x * 0.5f, +vSize.y * 0.5f, -vSize.z * 0.5f);
  wdVec3 vBaseR0 = vBaseL0 + vMoveFwd;
  wdVec3 vBaseR1 = vBaseL1 + vMoveFwd;

  wdVec3 vTopL0 = vBaseL0 + vMoveUp;
  wdVec3 vTopL1 = vBaseL1 + vMoveUp;
  wdVec3 vTopR0 = vBaseR0 + vMoveUp;
  wdVec3 vTopR1 = vBaseR1 + vMoveUp;

  wdVec3 vPrevTopR0 = vBaseL0;
  wdVec3 vPrevTopR1 = vBaseL1;

  float fTexU0 = 0;
  float fTexU1 = fStepDiv;

  wdVec3 vSideNormal0(0, 1, 0);
  wdVec3 vSideNormal1(0, 1, 0);
  wdVec3 vStepFrontNormal(-1, 0, 0);

  wdQuat qRot;
  qRot.SetFromAxisAndAngle(wdVec3(0, 0, 1), curveStep);

  for (wdUInt32 step = 0; step < uiNumSteps; ++step)
  {
    {
      const wdVec3 vAvg = (vTopL0 + vTopL1 + vTopR0 + vTopR1) / 4.0f;

      vTopR0 = vAvg + qRot * (vTopR0 - vAvg);
      vTopR1 = vAvg + qRot * (vTopR1 - vAvg);
      vBaseR0 = vAvg + qRot * (vBaseR0 - vAvg);
      vBaseR1 = vAvg + qRot * (vBaseR1 - vAvg);

      vMoveFwd = qRot * vMoveFwd;
      vMoveUpFwd = vMoveFwd;
      vMoveUpFwd.z = fStepHeight;

      vSideNormal1 = qRot * vSideNormal1;
    }

    if (bSmoothSloped)
    {
      // don't care about exact normals for the top surfaces
      vTopL0 = vPrevTopR0;
      vTopL1 = vPrevTopR1;
    }

    wdUInt32 poly[4];

    // top
    poly[0] = AddVertex(vTopL0, wdVec3(0, 0, 1), wdVec2(fTexU0, 0), options);
    poly[3] = AddVertex(vTopL1, wdVec3(0, 0, 1), wdVec2(fTexU0, 1), options);
    poly[1] = AddVertex(vTopR0, wdVec3(0, 0, 1), wdVec2(fTexU1, 0), options);
    poly[2] = AddVertex(vTopR1, wdVec3(0, 0, 1), wdVec2(fTexU1, 1), options);
    AddPolygon(poly, bFlipWinding);

    // bottom
    poly[0] = AddVertex(vBaseL0, wdVec3(0, 0, -1), wdVec2(fTexU0, 0), options);
    poly[1] = AddVertex(vBaseL1, wdVec3(0, 0, -1), wdVec2(fTexU0, 1), options);
    poly[3] = AddVertex(vBaseR0, wdVec3(0, 0, -1), wdVec2(fTexU1, 0), options);
    poly[2] = AddVertex(vBaseR1, wdVec3(0, 0, -1), wdVec2(fTexU1, 1), options);
    AddPolygon(poly, bFlipWinding);

    // step front
    if (!bSmoothSloped)
    {
      poly[0] = AddVertex(vPrevTopR0, wdVec3(-1, 0, 0), wdVec2(0, fTexU0), options);
      poly[3] = AddVertex(vPrevTopR1, wdVec3(-1, 0, 0), wdVec2(1, fTexU0), options);
      poly[1] = AddVertex(vTopL0, wdVec3(-1, 0, 0), wdVec2(0, fTexU1), options);
      poly[2] = AddVertex(vTopL1, wdVec3(-1, 0, 0), wdVec2(1, fTexU1), options);
      AddPolygon(poly, bFlipWinding);
    }

    // side 1
    poly[0] = AddVertex(vBaseL0, -vSideNormal0, wdVec2(fTexU0, 0), options);
    poly[1] = AddVertex(vBaseR0, -vSideNormal1, wdVec2(fTexU1, 0), options);
    poly[3] = AddVertex(vTopL0, -vSideNormal0, wdVec2(fTexU0, fTexU1), options);
    poly[2] = AddVertex(vTopR0, -vSideNormal1, wdVec2(fTexU1, fTexU1), options);
    AddPolygon(poly, bFlipWinding);

    // side 2
    poly[0] = AddVertex(vBaseL1, vSideNormal0, wdVec2(fTexU0, 0), options);
    poly[3] = AddVertex(vBaseR1, vSideNormal1, wdVec2(fTexU1, 0), options);
    poly[1] = AddVertex(vTopL1, vSideNormal0, wdVec2(fTexU0, fTexU1), options);
    poly[2] = AddVertex(vTopR1, vSideNormal1, wdVec2(fTexU1, fTexU1), options);
    AddPolygon(poly, bFlipWinding);

    vPrevTopR0 = vTopR0;
    vPrevTopR1 = vTopR1;

    vBaseL0 = vBaseR0;
    vBaseL1 = vBaseR1;
    vBaseR0 += vMoveFwd;
    vBaseR1 += vMoveFwd;

    vTopL0 = vTopR0 + vMoveUp;
    vTopL1 = vTopR1 + vMoveUp;
    vTopR0 += vMoveUpFwd;
    vTopR1 += vMoveUpFwd;

    fTexU0 = fTexU1;
    fTexU1 += fStepDiv;

    vSideNormal0 = vSideNormal1;
    vStepFrontNormal = qRot * vStepFrontNormal;
  }

  // back
  {
    wdUInt32 poly[4];
    poly[0] = AddVertex(vBaseL0, -vStepFrontNormal, wdVec2(0, 0), options);
    poly[1] = AddVertex(vBaseL1, -vStepFrontNormal, wdVec2(1, 0), options);
    poly[3] = AddVertex(vPrevTopR0, -vStepFrontNormal, wdVec2(0, 1), options);
    poly[2] = AddVertex(vPrevTopR1, -vStepFrontNormal, wdVec2(1, 1), options);
    AddPolygon(poly, bFlipWinding);
  }
}


void wdGeometry::AddArch(const wdVec3& vSize, wdUInt32 uiNumSegments, float fThickness, wdAngle angle, bool bMakeSteps, bool bSmoothBottom, bool bSmoothTop, bool bCapTopAndBottom, const GeoOptions& options)
{
  // sanitize input values
  {
    if (angle.GetRadian() == 0.0f)
      angle = wdAngle::Degree(360);

    angle = wdMath::Clamp(angle, wdAngle::Degree(-360.0f), wdAngle::Degree(360.0f));

    fThickness = wdMath::Clamp(fThickness, 0.01f, wdMath::Min(vSize.x, vSize.y) * 0.45f);

    bSmoothBottom = bMakeSteps && bSmoothBottom;
    bSmoothTop = bMakeSteps && bSmoothTop;
  }

  bool bFlipWinding = options.IsFlipWindingNecessary();

  if (angle.GetRadian() < 0)
    bFlipWinding = !bFlipWinding;

  const wdAngle angleStep = angle / (float)uiNumSegments;
  const float fScaleX = vSize.x * 0.5f;
  const float fScaleY = vSize.y * 0.5f;
  const float fHalfHeight = vSize.z * 0.5f;
  const float fStepHeight = vSize.z / (float)uiNumSegments;

  float fBottomZ = -fHalfHeight;
  float fTopZ = +fHalfHeight;

  if (bMakeSteps)
  {
    fTopZ = fBottomZ + fStepHeight;
  }

  // mutable variables
  wdAngle nextAngle;
  wdVec3 vCurDirOutwards, vNextDirOutwards;
  wdVec3 vCurBottomOuter, vCurBottomInner, vCurTopOuter, vCurTopInner;
  wdVec3 vNextBottomOuter, vNextBottomInner, vNextTopOuter, vNextTopInner;

  // Setup first round
  {
    vNextDirOutwards.Set(wdMath::Cos(nextAngle), wdMath::Sin(nextAngle), 0);
    vNextBottomOuter.Set(wdMath::Cos(nextAngle) * fScaleX, wdMath::Sin(nextAngle) * fScaleY, fBottomZ);
    vNextTopOuter.Set(vNextBottomOuter.x, vNextBottomOuter.y, fTopZ);

    const wdVec3 vNextThickness = vNextDirOutwards * fThickness;
    vNextBottomInner = vNextBottomOuter - vNextThickness;
    vNextTopInner = vNextTopOuter - vNextThickness;

    if (bSmoothBottom)
    {
      vNextBottomInner.z += fStepHeight * 0.5f;
      vNextBottomOuter.z += fStepHeight * 0.5f;
    }

    if (bSmoothTop)
    {
      vNextTopInner.z += fStepHeight * 0.5f;
      vNextTopOuter.z += fStepHeight * 0.5f;
    }
  }

  const bool isFullCircle = wdMath::Abs(angle.GetRadian()) >= wdAngle::Degree(360).GetRadian();

  const float fOuterUstep = 3.0f / uiNumSegments;
  for (wdUInt32 segment = 0; segment < uiNumSegments; ++segment)
  {
    // step values
    {
      nextAngle = angleStep * (segment + 1.0f);

      vCurDirOutwards = vNextDirOutwards;

      vCurBottomOuter = vNextBottomOuter;
      vCurBottomInner = vNextBottomInner;
      vCurTopOuter = vNextTopOuter;
      vCurTopInner = vNextTopInner;

      vNextDirOutwards.Set(wdMath::Cos(nextAngle), wdMath::Sin(nextAngle), 0);

      vNextBottomOuter.Set(vNextDirOutwards.x * fScaleX, vNextDirOutwards.y * fScaleY, fBottomZ);
      vNextTopOuter.Set(vNextBottomOuter.x, vNextBottomOuter.y, fTopZ);

      const wdVec3 vNextThickness = vNextDirOutwards * fThickness;
      vNextBottomInner = vNextBottomOuter - vNextThickness;
      vNextTopInner = vNextTopOuter - vNextThickness;

      if (bSmoothBottom)
      {
        vCurBottomInner.z -= fStepHeight;
        vCurBottomOuter.z -= fStepHeight;

        vNextBottomInner.z += fStepHeight * 0.5f;
        vNextBottomOuter.z += fStepHeight * 0.5f;
      }

      if (bSmoothTop)
      {
        vCurTopInner.z -= fStepHeight;
        vCurTopOuter.z -= fStepHeight;

        vNextTopInner.z += fStepHeight * 0.5f;
        vNextTopOuter.z += fStepHeight * 0.5f;
      }
    }

    const float fCurOuterU = segment * fOuterUstep;
    const float fNextOuterU = (1 + segment) * fOuterUstep;

    wdUInt32 poly[4];

    // Outside
    {
      poly[0] = AddVertex(vCurBottomOuter, vCurDirOutwards, wdVec2(fCurOuterU, 0), options);
      poly[1] = AddVertex(vNextBottomOuter, vNextDirOutwards, wdVec2(fNextOuterU, 0), options);
      poly[3] = AddVertex(vCurTopOuter, vCurDirOutwards, wdVec2(fCurOuterU, 1), options);
      poly[2] = AddVertex(vNextTopOuter, vNextDirOutwards, wdVec2(fNextOuterU, 1), options);
      AddPolygon(poly, bFlipWinding);
    }

    // Inside
    {
      poly[0] = AddVertex(vCurBottomInner, -vCurDirOutwards, wdVec2(fCurOuterU, 0), options);
      poly[3] = AddVertex(vNextBottomInner, -vNextDirOutwards, wdVec2(fNextOuterU, 0), options);
      poly[1] = AddVertex(vCurTopInner, -vCurDirOutwards, wdVec2(fCurOuterU, 1), options);
      poly[2] = AddVertex(vNextTopInner, -vNextDirOutwards, wdVec2(fNextOuterU, 1), options);
      AddPolygon(poly, bFlipWinding);
    }

    // Bottom
    if (bCapTopAndBottom)
    {
      poly[0] = AddVertex(vCurBottomInner, wdVec3(0, 0, -1), vCurBottomInner.GetAsVec2(), options);
      poly[1] = AddVertex(vNextBottomInner, wdVec3(0, 0, -1), vNextBottomInner.GetAsVec2(), options);
      poly[3] = AddVertex(vCurBottomOuter, wdVec3(0, 0, -1), vCurBottomOuter.GetAsVec2(), options);
      poly[2] = AddVertex(vNextBottomOuter, wdVec3(0, 0, -1), vNextBottomOuter.GetAsVec2(), options);
      AddPolygon(poly, bFlipWinding);
    }

    // Top
    if (bCapTopAndBottom)
    {
      poly[0] = AddVertex(vCurTopInner, wdVec3(0, 0, 1), vCurTopInner.GetAsVec2(), options);
      poly[3] = AddVertex(vNextTopInner, wdVec3(0, 0, 1), vNextTopInner.GetAsVec2(), options);
      poly[1] = AddVertex(vCurTopOuter, wdVec3(0, 0, 1), vCurTopOuter.GetAsVec2(), options);
      poly[2] = AddVertex(vNextTopOuter, wdVec3(0, 0, 1), vNextTopOuter.GetAsVec2(), options);
      AddPolygon(poly, bFlipWinding);
    }

    // Front
    if (bMakeSteps || (!isFullCircle && segment == 0))
    {
      const wdVec3 vNormal = (bFlipWinding ? -1.0f : 1.0f) * vCurDirOutwards.CrossRH(wdVec3(0, 0, 1));
      poly[0] = AddVertex(vCurBottomInner, vNormal, wdVec2(0, 0), options);
      poly[1] = AddVertex(vCurBottomOuter, vNormal, wdVec2(1, 0), options);
      poly[3] = AddVertex(vCurTopInner, vNormal, wdVec2(0, 1), options);
      poly[2] = AddVertex(vCurTopOuter, vNormal, wdVec2(1, 1), options);
      AddPolygon(poly, bFlipWinding);
    }

    // Back
    if (bMakeSteps || (!isFullCircle && segment == uiNumSegments - 1))
    {
      const wdVec3 vNormal = (bFlipWinding ? -1.0f : 1.0f) * -vNextDirOutwards.CrossRH(wdVec3(0, 0, 1));
      poly[0] = AddVertex(vNextBottomInner, vNormal, wdVec2(0, 0), options);
      poly[3] = AddVertex(vNextBottomOuter, vNormal, wdVec2(1, 0), options);
      poly[1] = AddVertex(vNextTopInner, vNormal, wdVec2(0, 1), options);
      poly[2] = AddVertex(vNextTopOuter, vNormal, wdVec2(1, 1), options);
      AddPolygon(poly, bFlipWinding);
    }

    if (bMakeSteps)
    {
      vNextTopOuter.z += fStepHeight;
      vNextTopInner.z += fStepHeight;
      vNextBottomOuter.z += fStepHeight;
      vNextBottomInner.z += fStepHeight;

      fBottomZ = fTopZ;
      fTopZ += fStepHeight;
    }
  }
}

WD_STATICLINK_FILE(Core, Core_Graphics_Implementation_Geometry);
