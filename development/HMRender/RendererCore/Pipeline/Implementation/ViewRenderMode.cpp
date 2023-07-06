#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/ViewRenderMode.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/GlobalConstants.h>

// clang-format off
WD_BEGIN_STATIC_REFLECTED_ENUM(wdViewRenderMode, 1)
  WD_ENUM_CONSTANT(wdViewRenderMode::None)->AddAttributes(new wdGroupAttribute("Default")),
  WD_ENUM_CONSTANT(wdViewRenderMode::WireframeColor)->AddAttributes(new wdGroupAttribute("Wireframe")),
  WD_ENUM_CONSTANT(wdViewRenderMode::WireframeMonochrome),
  WD_ENUM_CONSTANT(wdViewRenderMode::DiffuseLitOnly)->AddAttributes(new wdGroupAttribute("Lighting")),
  WD_ENUM_CONSTANT(wdViewRenderMode::SpecularLitOnly),
  WD_ENUM_CONSTANT(wdViewRenderMode::LightCount)->AddAttributes(new wdGroupAttribute("Performance")),
  WD_ENUM_CONSTANT(wdViewRenderMode::DecalCount),
  WD_ENUM_CONSTANT(wdViewRenderMode::StaticVsDynamic),
  WD_ENUM_CONSTANT(wdViewRenderMode::TexCoordsUV0)->AddAttributes(new wdGroupAttribute("TexCoords")),
  WD_ENUM_CONSTANT(wdViewRenderMode::TexCoordsUV1),
  WD_ENUM_CONSTANT(wdViewRenderMode::VertexColors0)->AddAttributes(new wdGroupAttribute("VertexColors")),
  WD_ENUM_CONSTANT(wdViewRenderMode::VertexColors1),
  WD_ENUM_CONSTANT(wdViewRenderMode::VertexNormals)->AddAttributes(new wdGroupAttribute("Normals")),
  WD_ENUM_CONSTANT(wdViewRenderMode::VertexTangents),
  WD_ENUM_CONSTANT(wdViewRenderMode::PixelNormals),
  WD_ENUM_CONSTANT(wdViewRenderMode::DiffuseColor)->AddAttributes(new wdGroupAttribute("PixelColors")),
  WD_ENUM_CONSTANT(wdViewRenderMode::DiffuseColorRange),
  WD_ENUM_CONSTANT(wdViewRenderMode::SpecularColor),
  WD_ENUM_CONSTANT(wdViewRenderMode::EmissiveColor),
  WD_ENUM_CONSTANT(wdViewRenderMode::Roughness)->AddAttributes(new wdGroupAttribute("Surface")),
  WD_ENUM_CONSTANT(wdViewRenderMode::Occlusion),
  WD_ENUM_CONSTANT(wdViewRenderMode::Depth),
  WD_ENUM_CONSTANT(wdViewRenderMode::BoneWeights)->AddAttributes(new wdGroupAttribute("Animation")),
WD_END_STATIC_REFLECTED_ENUM;
// clang-format on

// static
wdTempHashedString wdViewRenderMode::GetPermutationValue(Enum renderMode)
{
  if (renderMode >= WireframeColor && renderMode <= WireframeMonochrome)
  {
    return "RENDER_PASS_WIREFRAME";
  }
  else if (renderMode >= DiffuseLitOnly && renderMode < ENUM_COUNT)
  {
    return "RENDER_PASS_EDITOR";
  }

  return "";
}

// static
int wdViewRenderMode::GetRenderPassForShader(Enum renderMode)
{
  switch (renderMode)
  {
    case wdViewRenderMode::None:
      return -1;

    case wdViewRenderMode::WireframeColor:
      return WIREFRAME_RENDER_PASS_COLOR;

    case wdViewRenderMode::WireframeMonochrome:
      return WIREFRAME_RENDER_PASS_MONOCHROME;

    case wdViewRenderMode::DiffuseLitOnly:
      return EDITOR_RENDER_PASS_DIFFUSE_LIT_ONLY;

    case wdViewRenderMode::SpecularLitOnly:
      return EDITOR_RENDER_PASS_SPECULAR_LIT_ONLY;

    case wdViewRenderMode::LightCount:
      return EDITOR_RENDER_PASS_LIGHT_COUNT;

    case wdViewRenderMode::DecalCount:
      return EDITOR_RENDER_PASS_DECAL_COUNT;

    case wdViewRenderMode::TexCoordsUV0:
      return EDITOR_RENDER_PASS_TEXCOORDS_UV0;

    case wdViewRenderMode::TexCoordsUV1:
      return EDITOR_RENDER_PASS_TEXCOORDS_UV1;

    case wdViewRenderMode::VertexColors0:
      return EDITOR_RENDER_PASS_VERTEX_COLORS0;

    case wdViewRenderMode::VertexColors1:
      return EDITOR_RENDER_PASS_VERTEX_COLORS1;

    case wdViewRenderMode::VertexNormals:
      return EDITOR_RENDER_PASS_VERTEX_NORMALS;

    case wdViewRenderMode::VertexTangents:
      return EDITOR_RENDER_PASS_VERTEX_TANGENTS;

    case wdViewRenderMode::PixelNormals:
      return EDITOR_RENDER_PASS_PIXEL_NORMALS;

    case wdViewRenderMode::DiffuseColor:
      return EDITOR_RENDER_PASS_DIFFUSE_COLOR;

    case wdViewRenderMode::DiffuseColorRange:
      return EDITOR_RENDER_PASS_DIFFUSE_COLOR_RANGE;

    case wdViewRenderMode::SpecularColor:
      return EDITOR_RENDER_PASS_SPECULAR_COLOR;

    case wdViewRenderMode::EmissiveColor:
      return EDITOR_RENDER_PASS_EMISSIVE_COLOR;

    case wdViewRenderMode::Roughness:
      return EDITOR_RENDER_PASS_ROUGHNESS;

    case wdViewRenderMode::Occlusion:
      return EDITOR_RENDER_PASS_OCCLUSION;

    case wdViewRenderMode::Depth:
      return EDITOR_RENDER_PASS_DEPTH;

    case wdViewRenderMode::StaticVsDynamic:
      return EDITOR_RENDER_PASS_STATIC_VS_DYNAMIC;

    case wdViewRenderMode::BoneWeights:
      return EDITOR_RENDER_PASS_BONE_WEIGHTS;

    default:
      WD_ASSERT_NOT_IMPLEMENTED;
      return -1;
  }
}

// static
void wdViewRenderMode::GetDebugText(Enum renderMode, wdStringBuilder& out_sDebugText)
{
  if (renderMode == DiffuseColorRange)
  {
    out_sDebugText = "Pure magenta means the diffuse color is too dark, pure green means it is too bright.";
  }
  else if (renderMode == StaticVsDynamic)
  {
    out_sDebugText = "Static objects are shown in green, dynamic objects are shown in red.";
  }
}



WD_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_ViewRenderMode);
