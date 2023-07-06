#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Math/Float16.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>

namespace
{
  template <wdUInt32 Bits>
  WD_ALWAYS_INLINE wdUInt32 ColorFloatToUNorm(float value)
  {
    // Implemented according to
    // https://docs.microsoft.com/en-us/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    if (wdMath::IsNaN(value))
    {
      return 0;
    }
    else
    {
      float fMaxValue = ((1 << Bits) - 1);
      return static_cast<wdUInt32>(wdMath::Saturate(value) * fMaxValue + 0.5f);
    }
  }

  template <wdUInt32 Bits>
  constexpr inline float ColorUNormToFloat(wdUInt32 value)
  {
    // Implemented according to
    // https://docs.microsoft.com/en-us/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    wdUInt32 uiMaxValue = ((1 << Bits) - 1);
    float fMaxValue = ((1 << Bits) - 1);
    return (value & uiMaxValue) * (1.0f / fMaxValue);
  }
} // namespace

// clang-format off
WD_BEGIN_STATIC_REFLECTED_ENUM(wdMeshNormalPrecision, 1)
  WD_ENUM_CONSTANT(wdMeshNormalPrecision::_10Bit),
  WD_ENUM_CONSTANT(wdMeshNormalPrecision::_16Bit),
  WD_ENUM_CONSTANT(wdMeshNormalPrecision::_32Bit),
WD_END_STATIC_REFLECTED_ENUM;

WD_BEGIN_STATIC_REFLECTED_ENUM(wdMeshTexCoordPrecision, 1)
  WD_ENUM_CONSTANT(wdMeshTexCoordPrecision::_16Bit),
  WD_ENUM_CONSTANT(wdMeshTexCoordPrecision::_32Bit),
WD_END_STATIC_REFLECTED_ENUM;
// clang-format on

// static
wdResult wdMeshBufferUtils::EncodeFromFloat(const float fSource, wdArrayPtr<wdUInt8> dest, wdGALResourceFormat::Enum destFormat)
{
  WD_ASSERT_DEBUG(dest.GetCount() >= wdGALResourceFormat::GetBitsPerElement(destFormat) / 8, "Destination buffer is too small");

  switch (destFormat)
  {
    case wdGALResourceFormat::RFloat:
      *reinterpret_cast<float*>(dest.GetPtr()) = fSource;
      return WD_SUCCESS;
    case wdGALResourceFormat::RHalf:
      *reinterpret_cast<wdFloat16*>(dest.GetPtr()) = fSource;
      return WD_SUCCESS;
    default:
      return WD_FAILURE;
  }
}

// static
wdResult wdMeshBufferUtils::EncodeFromVec2(const wdVec2& vSource, wdArrayPtr<wdUInt8> dest, wdGALResourceFormat::Enum destFormat)
{
  WD_ASSERT_DEBUG(dest.GetCount() >= wdGALResourceFormat::GetBitsPerElement(destFormat) / 8, "Destination buffer is too small");

  switch (destFormat)
  {
    case wdGALResourceFormat::RGFloat:
      *reinterpret_cast<wdVec2*>(dest.GetPtr()) = vSource;
      return WD_SUCCESS;

    case wdGALResourceFormat::RGHalf:
      *reinterpret_cast<wdFloat16Vec2*>(dest.GetPtr()) = vSource;
      return WD_SUCCESS;

    default:
      return WD_FAILURE;
  }
}

// static
wdResult wdMeshBufferUtils::EncodeFromVec3(const wdVec3& vSource, wdArrayPtr<wdUInt8> dest, wdGALResourceFormat::Enum destFormat)
{
  WD_ASSERT_DEBUG(dest.GetCount() >= wdGALResourceFormat::GetBitsPerElement(destFormat) / 8, "Destination buffer is too small");

  switch (destFormat)
  {
    case wdGALResourceFormat::RGBFloat:
      *reinterpret_cast<wdVec3*>(dest.GetPtr()) = vSource;
      return WD_SUCCESS;

    case wdGALResourceFormat::RGBAUShortNormalized:
      reinterpret_cast<wdUInt16*>(dest.GetPtr())[0] = wdMath::ColorFloatToShort(vSource.x);
      reinterpret_cast<wdUInt16*>(dest.GetPtr())[1] = wdMath::ColorFloatToShort(vSource.y);
      reinterpret_cast<wdUInt16*>(dest.GetPtr())[2] = wdMath::ColorFloatToShort(vSource.z);
      reinterpret_cast<wdUInt16*>(dest.GetPtr())[3] = 0;
      return WD_SUCCESS;

    case wdGALResourceFormat::RGBAShortNormalized:
      reinterpret_cast<wdInt16*>(dest.GetPtr())[0] = wdMath::ColorFloatToSignedShort(vSource.x);
      reinterpret_cast<wdInt16*>(dest.GetPtr())[1] = wdMath::ColorFloatToSignedShort(vSource.y);
      reinterpret_cast<wdInt16*>(dest.GetPtr())[2] = wdMath::ColorFloatToSignedShort(vSource.z);
      reinterpret_cast<wdInt16*>(dest.GetPtr())[3] = 0;
      return WD_SUCCESS;

    case wdGALResourceFormat::RGB10A2UIntNormalized:
      *reinterpret_cast<wdUInt32*>(dest.GetPtr()) = ColorFloatToUNorm<10>(vSource.x);
      *reinterpret_cast<wdUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<10>(vSource.y) << 10;
      *reinterpret_cast<wdUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<10>(vSource.z) << 20;
      return WD_SUCCESS;

    case wdGALResourceFormat::RGBAUByteNormalized:
      dest.GetPtr()[0] = wdMath::ColorFloatToByte(vSource.x);
      dest.GetPtr()[1] = wdMath::ColorFloatToByte(vSource.y);
      dest.GetPtr()[2] = wdMath::ColorFloatToByte(vSource.z);
      dest.GetPtr()[3] = 0;
      return WD_SUCCESS;

    case wdGALResourceFormat::RGBAByteNormalized:
      dest.GetPtr()[0] = wdMath::ColorFloatToSignedByte(vSource.x);
      dest.GetPtr()[1] = wdMath::ColorFloatToSignedByte(vSource.y);
      dest.GetPtr()[2] = wdMath::ColorFloatToSignedByte(vSource.z);
      dest.GetPtr()[3] = 0;
      return WD_SUCCESS;
    default:
      return WD_FAILURE;
  }
}

// static
wdResult wdMeshBufferUtils::EncodeFromVec4(const wdVec4& vSource, wdArrayPtr<wdUInt8> dest, wdGALResourceFormat::Enum destFormat)
{
  WD_ASSERT_DEBUG(dest.GetCount() >= wdGALResourceFormat::GetBitsPerElement(destFormat) / 8, "Destination buffer is too small");

  switch (destFormat)
  {
    case wdGALResourceFormat::RGBAFloat:
      *reinterpret_cast<wdVec4*>(dest.GetPtr()) = vSource;
      return WD_SUCCESS;

    case wdGALResourceFormat::RGBAHalf:
      *reinterpret_cast<wdFloat16Vec4*>(dest.GetPtr()) = vSource;
      return WD_SUCCESS;

    case wdGALResourceFormat::RGBAUShortNormalized:
      reinterpret_cast<wdUInt16*>(dest.GetPtr())[0] = wdMath::ColorFloatToShort(vSource.x);
      reinterpret_cast<wdUInt16*>(dest.GetPtr())[1] = wdMath::ColorFloatToShort(vSource.y);
      reinterpret_cast<wdUInt16*>(dest.GetPtr())[2] = wdMath::ColorFloatToShort(vSource.z);
      reinterpret_cast<wdUInt16*>(dest.GetPtr())[3] = wdMath::ColorFloatToShort(vSource.w);
      return WD_SUCCESS;

    case wdGALResourceFormat::RGBAShortNormalized:
      reinterpret_cast<wdInt16*>(dest.GetPtr())[0] = wdMath::ColorFloatToSignedShort(vSource.x);
      reinterpret_cast<wdInt16*>(dest.GetPtr())[1] = wdMath::ColorFloatToSignedShort(vSource.y);
      reinterpret_cast<wdInt16*>(dest.GetPtr())[2] = wdMath::ColorFloatToSignedShort(vSource.z);
      reinterpret_cast<wdInt16*>(dest.GetPtr())[3] = wdMath::ColorFloatToSignedShort(vSource.w);
      return WD_SUCCESS;

    case wdGALResourceFormat::RGB10A2UIntNormalized:
      *reinterpret_cast<wdUInt32*>(dest.GetPtr()) = ColorFloatToUNorm<10>(vSource.x);
      *reinterpret_cast<wdUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<10>(vSource.y) << 10;
      *reinterpret_cast<wdUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<10>(vSource.z) << 20;
      *reinterpret_cast<wdUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<2>(vSource.w) << 30;
      return WD_SUCCESS;

    case wdGALResourceFormat::RGBAUByteNormalized:
      dest.GetPtr()[0] = wdMath::ColorFloatToByte(vSource.x);
      dest.GetPtr()[1] = wdMath::ColorFloatToByte(vSource.y);
      dest.GetPtr()[2] = wdMath::ColorFloatToByte(vSource.z);
      dest.GetPtr()[3] = wdMath::ColorFloatToByte(vSource.w);
      return WD_SUCCESS;

    case wdGALResourceFormat::RGBAByteNormalized:
      dest.GetPtr()[0] = wdMath::ColorFloatToSignedByte(vSource.x);
      dest.GetPtr()[1] = wdMath::ColorFloatToSignedByte(vSource.y);
      dest.GetPtr()[2] = wdMath::ColorFloatToSignedByte(vSource.z);
      dest.GetPtr()[3] = wdMath::ColorFloatToSignedByte(vSource.w);
      return WD_SUCCESS;

    default:
      return WD_FAILURE;
  }
}

// static
wdResult wdMeshBufferUtils::DecodeToFloat(wdArrayPtr<const wdUInt8> source, wdGALResourceFormat::Enum sourceFormat, float& ref_fDest)
{
  WD_ASSERT_DEBUG(source.GetCount() >= wdGALResourceFormat::GetBitsPerElement(sourceFormat) / 8, "Source buffer is too small");

  switch (sourceFormat)
  {
    case wdGALResourceFormat::RFloat:
      ref_fDest = *reinterpret_cast<const float*>(source.GetPtr());
      return WD_SUCCESS;
    case wdGALResourceFormat::RHalf:
      ref_fDest = *reinterpret_cast<const wdFloat16*>(source.GetPtr());
      return WD_SUCCESS;
    default:
      return WD_FAILURE;
  }
}

// static
wdResult wdMeshBufferUtils::DecodeToVec2(wdArrayPtr<const wdUInt8> source, wdGALResourceFormat::Enum sourceFormat, wdVec2& ref_vDest)
{
  WD_ASSERT_DEBUG(source.GetCount() >= wdGALResourceFormat::GetBitsPerElement(sourceFormat) / 8, "Source buffer is too small");

  switch (sourceFormat)
  {
    case wdGALResourceFormat::RGFloat:
      ref_vDest = *reinterpret_cast<const wdVec2*>(source.GetPtr());
      return WD_SUCCESS;
    case wdGALResourceFormat::RGHalf:
      ref_vDest = *reinterpret_cast<const wdFloat16Vec2*>(source.GetPtr());
      return WD_SUCCESS;
    default:
      return WD_FAILURE;
  }
}

// static
wdResult wdMeshBufferUtils::DecodeToVec3(wdArrayPtr<const wdUInt8> source, wdGALResourceFormat::Enum sourceFormat, wdVec3& ref_vDest)
{
  WD_ASSERT_DEBUG(source.GetCount() >= wdGALResourceFormat::GetBitsPerElement(sourceFormat) / 8, "Source buffer is too small");

  switch (sourceFormat)
  {
    case wdGALResourceFormat::RGBFloat:
      ref_vDest = *reinterpret_cast<const wdVec3*>(source.GetPtr());
      return WD_SUCCESS;

    case wdGALResourceFormat::RGBAUShortNormalized:
      ref_vDest.x = wdMath::ColorShortToFloat(reinterpret_cast<const wdUInt16*>(source.GetPtr())[0]);
      ref_vDest.y = wdMath::ColorShortToFloat(reinterpret_cast<const wdUInt16*>(source.GetPtr())[1]);
      ref_vDest.z = wdMath::ColorShortToFloat(reinterpret_cast<const wdUInt16*>(source.GetPtr())[2]);
      return WD_SUCCESS;

    case wdGALResourceFormat::RGBAShortNormalized:
      ref_vDest.x = wdMath::ColorSignedShortToFloat(reinterpret_cast<const wdInt16*>(source.GetPtr())[0]);
      ref_vDest.y = wdMath::ColorSignedShortToFloat(reinterpret_cast<const wdInt16*>(source.GetPtr())[1]);
      ref_vDest.z = wdMath::ColorSignedShortToFloat(reinterpret_cast<const wdInt16*>(source.GetPtr())[2]);
      return WD_SUCCESS;

    case wdGALResourceFormat::RGB10A2UIntNormalized:
      ref_vDest.x = ColorUNormToFloat<10>(*reinterpret_cast<const wdUInt32*>(source.GetPtr()));
      ref_vDest.y = ColorUNormToFloat<10>(*reinterpret_cast<const wdUInt32*>(source.GetPtr()) >> 10);
      ref_vDest.z = ColorUNormToFloat<10>(*reinterpret_cast<const wdUInt32*>(source.GetPtr()) >> 20);
      return WD_SUCCESS;

    case wdGALResourceFormat::RGBAUByteNormalized:
      ref_vDest.x = wdMath::ColorByteToFloat(source.GetPtr()[0]);
      ref_vDest.y = wdMath::ColorByteToFloat(source.GetPtr()[1]);
      ref_vDest.z = wdMath::ColorByteToFloat(source.GetPtr()[2]);
      return WD_SUCCESS;

    case wdGALResourceFormat::RGBAByteNormalized:
      ref_vDest.x = wdMath::ColorSignedByteToFloat(source.GetPtr()[0]);
      ref_vDest.y = wdMath::ColorSignedByteToFloat(source.GetPtr()[1]);
      ref_vDest.z = wdMath::ColorSignedByteToFloat(source.GetPtr()[2]);
      return WD_SUCCESS;
    default:
      return WD_FAILURE;
  }
}

// static
wdResult wdMeshBufferUtils::DecodeToVec4(wdArrayPtr<const wdUInt8> source, wdGALResourceFormat::Enum sourceFormat, wdVec4& ref_vDest)
{
  WD_ASSERT_DEBUG(source.GetCount() >= wdGALResourceFormat::GetBitsPerElement(sourceFormat) / 8, "Source buffer is too small");

  switch (sourceFormat)
  {
    case wdGALResourceFormat::RGBAFloat:
      ref_vDest = *reinterpret_cast<const wdVec4*>(source.GetPtr());
      return WD_SUCCESS;

    case wdGALResourceFormat::RGBAHalf:
      ref_vDest = *reinterpret_cast<const wdFloat16Vec4*>(source.GetPtr());
      return WD_SUCCESS;

    case wdGALResourceFormat::RGBAUShortNormalized:
      ref_vDest.x = wdMath::ColorShortToFloat(reinterpret_cast<const wdUInt16*>(source.GetPtr())[0]);
      ref_vDest.y = wdMath::ColorShortToFloat(reinterpret_cast<const wdUInt16*>(source.GetPtr())[1]);
      ref_vDest.z = wdMath::ColorShortToFloat(reinterpret_cast<const wdUInt16*>(source.GetPtr())[2]);
      ref_vDest.w = wdMath::ColorShortToFloat(reinterpret_cast<const wdUInt16*>(source.GetPtr())[3]);
      return WD_SUCCESS;

    case wdGALResourceFormat::RGBAShortNormalized:
      ref_vDest.x = wdMath::ColorSignedShortToFloat(reinterpret_cast<const wdInt16*>(source.GetPtr())[0]);
      ref_vDest.y = wdMath::ColorSignedShortToFloat(reinterpret_cast<const wdInt16*>(source.GetPtr())[1]);
      ref_vDest.z = wdMath::ColorSignedShortToFloat(reinterpret_cast<const wdInt16*>(source.GetPtr())[2]);
      ref_vDest.w = wdMath::ColorSignedShortToFloat(reinterpret_cast<const wdInt16*>(source.GetPtr())[3]);
      return WD_SUCCESS;

    case wdGALResourceFormat::RGB10A2UIntNormalized:
      ref_vDest.x = ColorUNormToFloat<10>(*reinterpret_cast<const wdUInt32*>(source.GetPtr()));
      ref_vDest.y = ColorUNormToFloat<10>(*reinterpret_cast<const wdUInt32*>(source.GetPtr()) >> 10);
      ref_vDest.z = ColorUNormToFloat<10>(*reinterpret_cast<const wdUInt32*>(source.GetPtr()) >> 20);
      ref_vDest.w = ColorUNormToFloat<2>(*reinterpret_cast<const wdUInt32*>(source.GetPtr()) >> 30);
      return WD_SUCCESS;

    case wdGALResourceFormat::RGBAUByteNormalized:
      ref_vDest.x = wdMath::ColorByteToFloat(source.GetPtr()[0]);
      ref_vDest.y = wdMath::ColorByteToFloat(source.GetPtr()[1]);
      ref_vDest.z = wdMath::ColorByteToFloat(source.GetPtr()[2]);
      ref_vDest.w = wdMath::ColorByteToFloat(source.GetPtr()[3]);
      return WD_SUCCESS;

    case wdGALResourceFormat::RGBAByteNormalized:
      ref_vDest.x = wdMath::ColorSignedByteToFloat(source.GetPtr()[0]);
      ref_vDest.y = wdMath::ColorSignedByteToFloat(source.GetPtr()[1]);
      ref_vDest.z = wdMath::ColorSignedByteToFloat(source.GetPtr()[2]);
      ref_vDest.w = wdMath::ColorSignedByteToFloat(source.GetPtr()[3]);
      return WD_SUCCESS;

    default:
      return WD_FAILURE;
  }
}

// static
wdResult wdMeshBufferUtils::GetPositionStream(const wdMeshBufferResourceDescriptor& meshBufferDesc, const wdVec3*& out_pPositions, wdUInt32& out_uiElementStride)
{
  const wdVertexDeclarationInfo& vdi = meshBufferDesc.GetVertexDeclaration();
  const wdUInt8* pRawVertexData = meshBufferDesc.GetVertexBufferData().GetPtr();

  const wdVec3* pPositions = nullptr;

  for (wdUInt32 vs = 0; vs < vdi.m_VertexStreams.GetCount(); ++vs)
  {
    if (vdi.m_VertexStreams[vs].m_Semantic == wdGALVertexAttributeSemantic::Position)
    {
      if (vdi.m_VertexStreams[vs].m_Format != wdGALResourceFormat::RGBFloat)
      {
        wdLog::Error("Unsupported vertex position format {0}", (int)vdi.m_VertexStreams[vs].m_Format);
        return WD_FAILURE; // other position formats are not supported
      }

      pPositions = reinterpret_cast<const wdVec3*>(pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset);
    }
  }

  if (pPositions == nullptr)
  {
    wdLog::Error("No position stream found");
    return WD_FAILURE;
  }

  out_pPositions = pPositions;
  out_uiElementStride = meshBufferDesc.GetVertexDataSize();
  return WD_SUCCESS;
}

// static
wdResult wdMeshBufferUtils::GetPositionAndNormalStream(const wdMeshBufferResourceDescriptor& meshBufferDesc, const wdVec3*& out_pPositions, const wdUInt8*& out_pNormals, wdGALResourceFormat::Enum& out_normalFormat, wdUInt32& out_uiElementStride)
{
  const wdVertexDeclarationInfo& vdi = meshBufferDesc.GetVertexDeclaration();
  const wdUInt8* pRawVertexData = meshBufferDesc.GetVertexBufferData().GetPtr();

  const wdVec3* pPositions = nullptr;
  const wdUInt8* pNormals = nullptr;
  wdGALResourceFormat::Enum normalFormat = wdGALResourceFormat::Invalid;

  for (wdUInt32 vs = 0; vs < vdi.m_VertexStreams.GetCount(); ++vs)
  {
    if (vdi.m_VertexStreams[vs].m_Semantic == wdGALVertexAttributeSemantic::Position)
    {
      if (vdi.m_VertexStreams[vs].m_Format != wdGALResourceFormat::RGBFloat)
      {
        wdLog::Error("Unsupported vertex position format {0}", (int)vdi.m_VertexStreams[vs].m_Format);
        return WD_FAILURE; // other position formats are not supported
      }

      pPositions = reinterpret_cast<const wdVec3*>(pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset);
    }
    else if (vdi.m_VertexStreams[vs].m_Semantic == wdGALVertexAttributeSemantic::Normal)
    {
      pNormals = pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset;
      normalFormat = vdi.m_VertexStreams[vs].m_Format;
    }
  }

  if (pPositions == nullptr || pNormals == nullptr)
  {
    wdLog::Error("No position and normal stream found");
    return WD_FAILURE;
  }

  wdUInt8 dummySource[16] = {};
  wdVec3 vNormal;
  if (DecodeNormal(wdMakeArrayPtr(dummySource), normalFormat, vNormal).Failed())
  {
    wdLog::Error("Unsupported vertex normal format {0}", normalFormat);
    return WD_FAILURE;
  }

  out_pPositions = pPositions;
  out_pNormals = pNormals;
  out_normalFormat = normalFormat;
  out_uiElementStride = meshBufferDesc.GetVertexDataSize();
  return WD_SUCCESS;
}


WD_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshBufferUtils);
