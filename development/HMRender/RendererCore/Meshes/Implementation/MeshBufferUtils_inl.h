
// static
WD_ALWAYS_INLINE wdGALResourceFormat::Enum wdMeshNormalPrecision::ToResourceFormatNormal(Enum value)
{
  return value == _10Bit ? wdGALResourceFormat::RGB10A2UIntNormalized
                         : (value == _16Bit ? wdGALResourceFormat::RGBAUShortNormalized : wdGALResourceFormat::XYZFloat);
}

// static
WD_ALWAYS_INLINE wdGALResourceFormat::Enum wdMeshNormalPrecision::ToResourceFormatTangent(Enum value)
{
  return value == _10Bit ? wdGALResourceFormat::RGB10A2UIntNormalized
                         : (value == _16Bit ? wdGALResourceFormat::RGBAUShortNormalized : wdGALResourceFormat::XYZWFloat);
}

//////////////////////////////////////////////////////////////////////////

// static
WD_ALWAYS_INLINE wdGALResourceFormat::Enum wdMeshTexCoordPrecision::ToResourceFormat(Enum value)
{
  return value == _16Bit ? wdGALResourceFormat::UVHalf : wdGALResourceFormat::UVFloat;
}

//////////////////////////////////////////////////////////////////////////

// static
WD_ALWAYS_INLINE wdResult wdMeshBufferUtils::EncodeNormal(const wdVec3& vNormal, wdArrayPtr<wdUInt8> dest, wdMeshNormalPrecision::Enum normalPrecision)
{
  return EncodeNormal(vNormal, dest, wdMeshNormalPrecision::ToResourceFormatNormal(normalPrecision));
}

// static
WD_ALWAYS_INLINE wdResult wdMeshBufferUtils::EncodeTangent(
  const wdVec3& vTangent, float fTangentSign, wdArrayPtr<wdUInt8> dest, wdMeshNormalPrecision::Enum tangentPrecision)
{
  return EncodeTangent(vTangent, fTangentSign, dest, wdMeshNormalPrecision::ToResourceFormatTangent(tangentPrecision));
}

// static
WD_ALWAYS_INLINE wdResult wdMeshBufferUtils::EncodeTexCoord(
  const wdVec2& vTexCoord, wdArrayPtr<wdUInt8> dest, wdMeshTexCoordPrecision::Enum texCoordPrecision)
{
  return EncodeTexCoord(vTexCoord, dest, wdMeshTexCoordPrecision::ToResourceFormat(texCoordPrecision));
}

// static
WD_ALWAYS_INLINE wdResult wdMeshBufferUtils::EncodeNormal(const wdVec3& vNormal, wdArrayPtr<wdUInt8> dest, wdGALResourceFormat::Enum destFormat)
{
  // we store normals in unsigned formats thus we need to map from -1..1 to 0..1 here
  return EncodeFromVec3(vNormal * 0.5f + wdVec3(0.5f), dest, destFormat);
}

// static
WD_ALWAYS_INLINE wdResult wdMeshBufferUtils::EncodeTangent(
  const wdVec3& vTangent, float fTangentSign, wdArrayPtr<wdUInt8> dest, wdGALResourceFormat::Enum destFormat)
{
  // make sure biTangentSign is either -1 or 1
  fTangentSign = (fTangentSign < 0.0f) ? -1.0f : 1.0f;

  // we store tangents in unsigned formats thus we need to map from -1..1 to 0..1 here
  return EncodeFromVec4(vTangent.GetAsVec4(fTangentSign) * 0.5f + wdVec4(0.5f), dest, destFormat);
}

// static
WD_ALWAYS_INLINE wdResult wdMeshBufferUtils::EncodeTexCoord(const wdVec2& vTexCoord, wdArrayPtr<wdUInt8> dest, wdGALResourceFormat::Enum destFormat)
{
  return EncodeFromVec2(vTexCoord, dest, destFormat);
}

// static
WD_ALWAYS_INLINE wdResult wdMeshBufferUtils::DecodeNormal(
  wdArrayPtr<const wdUInt8> source, wdVec3& ref_vDestNormal, wdMeshNormalPrecision::Enum normalPrecision)
{
  return DecodeNormal(source, wdMeshNormalPrecision::ToResourceFormatNormal(normalPrecision), ref_vDestNormal);
}

// static
WD_ALWAYS_INLINE wdResult wdMeshBufferUtils::DecodeTangent(
  wdArrayPtr<const wdUInt8> source, wdVec3& ref_vDestTangent, float& ref_fDestBiTangentSign, wdMeshNormalPrecision::Enum tangentPrecision)
{
  return DecodeTangent(source, wdMeshNormalPrecision::ToResourceFormatTangent(tangentPrecision), ref_vDestTangent, ref_fDestBiTangentSign);
}

// static
WD_ALWAYS_INLINE wdResult wdMeshBufferUtils::DecodeTexCoord(
  wdArrayPtr<const wdUInt8> source, wdVec2& ref_vDestTexCoord, wdMeshTexCoordPrecision::Enum texCoordPrecision)
{
  return DecodeTexCoord(source, wdMeshTexCoordPrecision::ToResourceFormat(texCoordPrecision), ref_vDestTexCoord);
}

// static
WD_ALWAYS_INLINE wdResult wdMeshBufferUtils::DecodeNormal(
  wdArrayPtr<const wdUInt8> source, wdGALResourceFormat::Enum sourceFormat, wdVec3& ref_vDestNormal)
{
  wdVec3 tempNormal;
  WD_SUCCEED_OR_RETURN(DecodeToVec3(source, sourceFormat, tempNormal));
  ref_vDestNormal = tempNormal * 2.0f - wdVec3(1.0f);
  return WD_SUCCESS;
}

// static
WD_ALWAYS_INLINE wdResult wdMeshBufferUtils::DecodeTangent(
  wdArrayPtr<const wdUInt8> source, wdGALResourceFormat::Enum sourceFormat, wdVec3& ref_vDestTangent, float& ref_fDestBiTangentSign)
{
  wdVec4 tempTangent;
  WD_SUCCEED_OR_RETURN(DecodeToVec4(source, sourceFormat, tempTangent));
  ref_vDestTangent = tempTangent.GetAsVec3() * 2.0f - wdVec3(1.0f);
  ref_fDestBiTangentSign = tempTangent.w * 2.0f - 1.0f;
  return WD_SUCCESS;
}

// static
WD_ALWAYS_INLINE wdResult wdMeshBufferUtils::DecodeTexCoord(
  wdArrayPtr<const wdUInt8> source, wdGALResourceFormat::Enum sourceFormat, wdVec2& ref_vDestTexCoord)
{
  return DecodeToVec2(source, sourceFormat, ref_vDestTexCoord);
}
