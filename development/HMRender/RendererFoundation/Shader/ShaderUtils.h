#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>

class wdShaderUtils
{
public:
  WD_ALWAYS_INLINE static wdUInt32 Float3ToRGB10(wdVec3 value)
  {
    const wdVec3 unsignedValue = value * 0.5f + wdVec3(0.5f);

    const wdUInt32 r = wdMath::Clamp(static_cast<wdUInt32>(unsignedValue.x * 1023.0f + 0.5f), 0u, 1023u);
    const wdUInt32 g = wdMath::Clamp(static_cast<wdUInt32>(unsignedValue.y * 1023.0f + 0.5f), 0u, 1023u);
    const wdUInt32 b = wdMath::Clamp(static_cast<wdUInt32>(unsignedValue.z * 1023.0f + 0.5f), 0u, 1023u);

    return r | (g << 10) | (b << 20);
  }

  WD_ALWAYS_INLINE static wdUInt32 PackFloat16intoUint(wdFloat16 x, wdFloat16 y)
  {
    const wdUInt32 r = x.GetRawData();
    const wdUInt32 g = y.GetRawData();

    return r | (g << 16);
  }

  WD_ALWAYS_INLINE static wdUInt32 Float2ToRG16F(wdVec2 value)
  {
    const wdUInt32 r = wdFloat16(value.x).GetRawData();
    const wdUInt32 g = wdFloat16(value.y).GetRawData();

    return r | (g << 16);
  }

  WD_ALWAYS_INLINE static void Float4ToRGBA16F(wdVec4 value, wdUInt32& out_uiRG, wdUInt32& out_uiBA)
  {
    out_uiRG = Float2ToRG16F(wdVec2(value.x, value.y));
    out_uiBA = Float2ToRG16F(wdVec2(value.z, value.w));
  }

  enum class wdBuiltinShaderType
  {
    CopyImage,
    CopyImageArray,
    DownscaleImage,
    DownscaleImageArray,
  };

  struct wdBuiltinShader
  {
    wdGALShaderHandle m_hActiveGALShader;
    wdGALBlendStateHandle m_hBlendState;
    wdGALDepthStencilStateHandle m_hDepthStencilState;
    wdGALRasterizerStateHandle m_hRasterizerState;
  };

  WD_RENDERERFOUNDATION_DLL static wdDelegate<void(wdBuiltinShaderType type, wdBuiltinShader& out_shader)> g_RequestBuiltinShaderCallback;

  WD_ALWAYS_INLINE static void RequestBuiltinShader(wdBuiltinShaderType type, wdBuiltinShader& out_shader)
  {
    g_RequestBuiltinShaderCallback(type, out_shader);
  }
};
WD_DEFINE_AS_POD_TYPE(wdShaderUtils::wdBuiltinShaderType);
