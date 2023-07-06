
#pragma once

#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>

struct wdMeshBufferResourceDescriptor;

struct wdMeshNormalPrecision
{
  using StorageType = wdUInt8;

  enum Enum
  {
    _10Bit,
    _16Bit,
    _32Bit,

    Default = _10Bit
  };

  /// \brief Convert mesh normal precision to actual resource format used for normals
  static wdGALResourceFormat::Enum ToResourceFormatNormal(Enum value);

  /// \brief Convert mesh normal precision to actual resource format used for tangents
  static wdGALResourceFormat::Enum ToResourceFormatTangent(Enum value);
};

WD_DECLARE_REFLECTABLE_TYPE(WD_RENDERERCORE_DLL, wdMeshNormalPrecision);

struct wdMeshTexCoordPrecision
{
  using StorageType = wdUInt8;

  enum Enum
  {
    _16Bit,
    _32Bit,

    Default = _16Bit
  };

  /// \brief Convert mesh texcoord precision to actual resource format
  static wdGALResourceFormat::Enum ToResourceFormat(Enum value);
};

WD_DECLARE_REFLECTABLE_TYPE(WD_RENDERERCORE_DLL, wdMeshTexCoordPrecision);

struct WD_RENDERERCORE_DLL wdMeshBufferUtils
{
  static wdResult EncodeNormal(const wdVec3& vNormal, wdArrayPtr<wdUInt8> dest, wdMeshNormalPrecision::Enum normalPrecision);
  static wdResult EncodeTangent(const wdVec3& vTangent, float fTangentSign, wdArrayPtr<wdUInt8> dest, wdMeshNormalPrecision::Enum tangentPrecision);
  static wdResult EncodeTexCoord(const wdVec2& vTexCoord, wdArrayPtr<wdUInt8> dest, wdMeshTexCoordPrecision::Enum texCoordPrecision);

  static wdResult EncodeNormal(const wdVec3& vNormal, wdArrayPtr<wdUInt8> dest, wdGALResourceFormat::Enum destFormat);
  static wdResult EncodeTangent(const wdVec3& vTangent, float fTangentSign, wdArrayPtr<wdUInt8> dest, wdGALResourceFormat::Enum destFormat);
  static wdResult EncodeTexCoord(const wdVec2& vTexCoord, wdArrayPtr<wdUInt8> dest, wdGALResourceFormat::Enum destFormat);

  static wdResult DecodeNormal(wdArrayPtr<const wdUInt8> source, wdVec3& ref_vDestNormal, wdMeshNormalPrecision::Enum normalPrecision);
  static wdResult DecodeTangent(
    wdArrayPtr<const wdUInt8> source, wdVec3& ref_vDestTangent, float& ref_fDestBiTangentSign, wdMeshNormalPrecision::Enum tangentPrecision);
  static wdResult DecodeTexCoord(wdArrayPtr<const wdUInt8> source, wdVec2& ref_vDestTexCoord, wdMeshTexCoordPrecision::Enum texCoordPrecision);

  static wdResult DecodeNormal(wdArrayPtr<const wdUInt8> source, wdGALResourceFormat::Enum sourceFormat, wdVec3& ref_vDestNormal);
  static wdResult DecodeTangent(
    wdArrayPtr<const wdUInt8> source, wdGALResourceFormat::Enum sourceFormat, wdVec3& ref_vDestTangent, float& ref_fDestBiTangentSign);
  static wdResult DecodeTexCoord(wdArrayPtr<const wdUInt8> source, wdGALResourceFormat::Enum sourceFormat, wdVec2& ref_vDestTexCoord);

  // low level conversion functions
  static wdResult EncodeFromFloat(const float fSource, wdArrayPtr<wdUInt8> dest, wdGALResourceFormat::Enum destFormat);
  static wdResult EncodeFromVec2(const wdVec2& vSource, wdArrayPtr<wdUInt8> dest, wdGALResourceFormat::Enum destFormat);
  static wdResult EncodeFromVec3(const wdVec3& vSource, wdArrayPtr<wdUInt8> dest, wdGALResourceFormat::Enum destFormat);
  static wdResult EncodeFromVec4(const wdVec4& vSource, wdArrayPtr<wdUInt8> dest, wdGALResourceFormat::Enum destFormat);

  static wdResult DecodeToFloat(wdArrayPtr<const wdUInt8> source, wdGALResourceFormat::Enum sourceFormat, float& ref_fDest);
  static wdResult DecodeToVec2(wdArrayPtr<const wdUInt8> source, wdGALResourceFormat::Enum sourceFormat, wdVec2& ref_vDest);
  static wdResult DecodeToVec3(wdArrayPtr<const wdUInt8> source, wdGALResourceFormat::Enum sourceFormat, wdVec3& ref_vDest);
  static wdResult DecodeToVec4(wdArrayPtr<const wdUInt8> source, wdGALResourceFormat::Enum sourceFormat, wdVec4& ref_vDest);

  /// \brief Helper function to get the position stream from the given mesh buffer descriptor
  static wdResult GetPositionStream(const wdMeshBufferResourceDescriptor& meshBufferDesc, const wdVec3*& out_pPositions, wdUInt32& out_uiElementStride);

  /// \brief Helper function to get the position and normal stream from the given mesh buffer descriptor
  static wdResult GetPositionAndNormalStream(const wdMeshBufferResourceDescriptor& meshBufferDesc, const wdVec3*& out_pPositions, const wdUInt8*& out_pNormals, wdGALResourceFormat::Enum& out_normalFormat, wdUInt32& out_uiElementStride);
};

#include <RendererCore/Meshes/Implementation/MeshBufferUtils_inl.h>
