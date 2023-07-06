#pragma once

#include <Foundation/Math/Mat4.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererCore/Rasterizer/Thirdparty/Occluder.h>
#include <RendererCore/RendererCoreDLL.h>

class wdGeometry;

class WD_RENDERERCORE_DLL wdRasterizerObject : public wdRefCounted
{
  WD_DISALLOW_COPY_AND_ASSIGN(wdRasterizerObject);

public:
  wdRasterizerObject();
  ~wdRasterizerObject();

  /// \brief If an object with the given name has been created before, it is returned, otherwise nullptr is returned.
  ///
  /// Use this to quickly query for an existing object. Call CreateMesh() in case the object doesn't exist yet.
  static wdSharedPtr<const wdRasterizerObject> GetObject(wdStringView sUniqueName);

  /// \brief Creates a box object with the specified dimensions. If such a box was created before, the same pointer is returned.
  static wdSharedPtr<const wdRasterizerObject> CreateBox(const wdVec3& vFullExtents);

  /// \brief Creates an object with the given geometry. If an object with the same name was created before, that pointer is returned instead.
  ///
  /// It is assumed that the same name will only be used for identical geometry.
  static wdSharedPtr<const wdRasterizerObject> CreateMesh(wdStringView sUniqueName, const wdGeometry& geometry);

private:
  void CreateMesh(const wdGeometry& geometry);

  friend class wdRasterizerView;
  Occluder m_Occluder;

  static wdMutex s_Mutex;
  static wdMap<wdString, wdSharedPtr<wdRasterizerObject>> s_Objects;
};
