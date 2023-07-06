#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/RendererFoundationDLL.h>

const wdUInt8 wdGALIndexType::s_Size[wdGALIndexType::ENUM_COUNT] = {
  0,               // None
  sizeof(wdInt16), // UShort
  sizeof(wdInt32)  // UInt
};

const char* wdGALShaderStage::Names[ENUM_COUNT] = {
  "VertexShader",
  "HullShader",
  "DomainShader",
  "GeometryShader",
  "PixelShader",
  "ComputeShader",
};

WD_BEGIN_STATIC_REFLECTED_ENUM(wdGALMSAASampleCount, 1)
  WD_ENUM_CONSTANTS(
    wdGALMSAASampleCount::None, wdGALMSAASampleCount::TwoSamples, wdGALMSAASampleCount::FourSamples, wdGALMSAASampleCount::EightSamples)
WD_END_STATIC_REFLECTED_ENUM;

WD_STATICLINK_FILE(RendererFoundation, RendererFoundation_Basics);
