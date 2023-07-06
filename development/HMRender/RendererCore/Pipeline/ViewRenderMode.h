#pragma once

#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Pipeline/Declarations.h>

struct WD_RENDERERCORE_DLL wdViewRenderMode
{
  using StorageType = wdUInt8;

  enum Enum
  {
    None,
    WireframeColor,
    WireframeMonochrome,
    DiffuseLitOnly,
    SpecularLitOnly,
    LightCount,
    DecalCount,
    TexCoordsUV0,
    TexCoordsUV1,
    VertexColors0,
    VertexColors1,
    VertexNormals,
    VertexTangents,
    PixelNormals,
    DiffuseColor,
    DiffuseColorRange,
    SpecularColor,
    EmissiveColor,
    Roughness,
    Occlusion,
    Depth,
    StaticVsDynamic,
    BoneWeights,

    ENUM_COUNT,

    Default = None
  };

  static wdTempHashedString GetPermutationValue(Enum renderMode);
  static int GetRenderPassForShader(Enum renderMode);
  static void GetDebugText(Enum renderMode, wdStringBuilder& out_sDebugText);
};
WD_DECLARE_REFLECTABLE_TYPE(WD_RENDERERCORE_DLL, wdViewRenderMode);
