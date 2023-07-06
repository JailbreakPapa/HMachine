#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Math/Vec3.h>

struct WD_CORE_DLL wdAmbientCubeBasis
{
  enum
  {
    PosX = 0,
    NegX,
    PosY,
    NegY,
    PosZ,
    NegZ,

    NumDirs = 6
  };

  static wdVec3 s_Dirs[NumDirs];
};

template <typename T>
struct wdAmbientCube
{
  WD_DECLARE_POD_TYPE();

  wdAmbientCube();

  template <typename U>
  wdAmbientCube(const wdAmbientCube<U>& other);

  template <typename U>
  void operator=(const wdAmbientCube<U>& other);

  bool operator==(const wdAmbientCube& other) const;
  bool operator!=(const wdAmbientCube& other) const;

  void AddSample(const wdVec3& vDir, const T& value);

  T Evaluate(const wdVec3& vNormal) const;

  wdResult Serialize(wdStreamWriter& inout_stream) const;
  wdResult Deserialize(wdStreamReader& inout_stream);

  T m_Values[wdAmbientCubeBasis::NumDirs];
};

#include <Core/Graphics/Implementation/AmbientCubeBasis_inl.h>
