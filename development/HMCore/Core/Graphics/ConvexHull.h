#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>

/// \brief Computes convex hulls for 3D meshes.
///
/// By default it will also simplify the result to a reasonable degree,
/// to reduce complexity and vertex/triangle count.
///
/// Currently there is an upper limit of 16384 vertices to accept meshes.
/// Everything larger than that will not be processed.
class WD_CORE_DLL wdConvexHullGenerator
{
public:
  struct Face
  {
    WD_DECLARE_POD_TYPE();

    wdUInt16 m_uiVertexIdx[3];
  };

  wdConvexHullGenerator();
  ~wdConvexHullGenerator();

  /// \brief Used to remove degenerate and unnecessary triangles that have corners with very little angle change.
  /// Ie. specifying 10 degree, means that all triangle corners must have at least a 10 degree change (and inner angle of 170 degree).
  /// Default is 22 degree.
  void SetSimplificationMinTriangleAngle(wdAngle angle) { m_MinTriangleAngle = angle; }

  /// \brief Used to remove vertices that do not contribute much to the silhouette.
  /// Vertices whose adjacent triangle normals do not differ by more than angle, will be discarded.
  /// Default is 5 degree.
  void SetSimplificationFlatVertexNormalThreshold(wdAngle angle) { m_FlatVertexNormalThreshold = angle; }

  /// \brief The minimum triangle edge length. Every edge shorter than this will be discarded and replaced by a single vertex at the
  /// average position.
  /// \note The length is not in 'mesh space' coordinates, but instead in 'unit cube space'.
  /// That means, every mesh is scaled to fit into a cube of size [-1; +1] for each axis. Thus the exact scale of the mesh does not matter
  /// when setting this value. Default is 0.05.
  void SetSimplificationMinTriangleEdgeLength(double fLen) { m_fMinTriangleEdgeLength = fLen; }

  /// \brief Generates the convex hull. Simplifies the mesh according to the previously specified parameters.
  wdResult Build(const wdArrayPtr<const wdVec3> vertices);

  /// \brief When Build() was successful this can be called to retrieve the resulting vertices and triangles.
  void Retrieve(wdDynamicArray<wdVec3>& out_vertices, wdDynamicArray<Face>& out_faces);

  /// \brief Same as Retrieve() but only returns the vertices.
  void RetrieveVertices(wdDynamicArray<wdVec3>& out_vertices);

private:
  wdResult ComputeCenterAndScale(const wdArrayPtr<const wdVec3> vertices);
  wdResult StoreNormalizedVertices(const wdArrayPtr<const wdVec3> vertices);
  void StoreTriangle(wdUInt16 i, wdUInt16 j, wdUInt16 k);
  wdResult InitializeHull();
  wdResult ComputeHull();
  bool IsInside(wdUInt32 vtxId) const;
  void RemoveVisibleFaces(wdUInt32 vtxId);
  void PatchHole(wdUInt32 vtxId);
  bool PruneFlatVertices(double fNormalThreshold);
  bool PruneDegenerateTriangles(double fMaxCosAngle);
  bool PruneSmallTriangles(double fMaxEdgeLen);
  wdResult ProcessVertices(const wdArrayPtr<const wdVec3> vertices);

  struct TwoSet
  {
    WD_ALWAYS_INLINE TwoSet()
    {
      a = 0xFFFF;
      b = 0xFFFF;
    }
    WD_ALWAYS_INLINE void Add(wdUInt16 x) { (a == 0xFFFF ? a : b) = x; }
    WD_ALWAYS_INLINE bool Contains(wdUInt16 x) { return a == x || b == x; }
    WD_ALWAYS_INLINE void Remove(wdUInt16 x) { (a == x ? a : b) = 0xFFFF; }
    WD_ALWAYS_INLINE int GetSize() { return (a != 0xFFFF) + (b != 0xFFFF); }

    wdUInt16 a, b;
  };

  struct Triangle
  {
    wdVec3d m_vNormal;
    double m_fPlaneDistance;
    wdUInt16 m_uiVertexIdx[3];
    bool m_bFlip;
    bool m_bIsDegenerate;
  };

  // used for mesh simplification
  wdAngle m_MinTriangleAngle = wdAngle::Degree(22.0f);
  wdAngle m_FlatVertexNormalThreshold = wdAngle::Degree(5);
  double m_fMinTriangleEdgeLength = 0.05;

  wdVec3d m_vCenter;
  double m_fScale;

  wdVec3d m_vInside;

  // all the 'good' vertices (no duplicates)
  // normalized to be within a unit-cube
  wdDynamicArray<wdVec3d> m_Vertices;

  // Will be resized to Square(m_Vertices.GetCount())
  // Index [i * m_Vertices.GetCount() + j] indicates which (up to two) other points
  // combine with the edge i and j to make a triangle in the hull.  Only defined when i < j.
  wdDynamicArray<TwoSet> m_Edges;

  wdDeque<Triangle> m_Triangles;
};
