#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/BakedProbes/BakingUtils.h>

wdVec3 wdBakingUtils::FibonacciSphere(wdUInt32 uiSampleIndex, wdUInt32 uiNumSamples)
{
  float offset = 2.0f / uiNumSamples;
  float increment = wdMath::Pi<float>() * (3.0f - wdMath::Sqrt(5.0f));

  float y = ((uiSampleIndex * offset) - 1) + (offset / 2);
  float r = wdMath::Sqrt(1 - y * y);

  wdAngle phi = wdAngle::Radian(((uiSampleIndex + 1) % uiNumSamples) * increment);

  float x = wdMath::Cos(phi) * r;
  float z = wdMath::Sin(phi) * r;

  return wdVec3(x, y, z);
}

static wdUInt32 s_BitsPerDir[wdAmbientCubeBasis::NumDirs] = {5, 5, 5, 5, 6, 6};

wdCompressedSkyVisibility wdBakingUtils::CompressSkyVisibility(const wdAmbientCube<float>& skyVisibility)
{
  wdCompressedSkyVisibility result = 0;
  wdUInt32 uiOffset = 0;
  for (wdUInt32 i = 0; i < wdAmbientCubeBasis::NumDirs; ++i)
  {
    float maxValue = static_cast<float>((1u << s_BitsPerDir[i]) - 1u);
    wdUInt32 compressedDir = static_cast<wdUInt8>(wdMath::Saturate(skyVisibility.m_Values[i]) * maxValue + 0.5f);
    result |= (compressedDir << uiOffset);
    uiOffset += s_BitsPerDir[i];
  }

  return result;
}

void wdBakingUtils::DecompressSkyVisibility(wdCompressedSkyVisibility compressedSkyVisibility, wdAmbientCube<float>& out_skyVisibility)
{
  wdUInt32 uiOffset = 0;
  for (wdUInt32 i = 0; i < wdAmbientCubeBasis::NumDirs; ++i)
  {
    wdUInt32 maxValue = (1u << s_BitsPerDir[i]) - 1u;
    out_skyVisibility.m_Values[i] = static_cast<float>((compressedSkyVisibility >> uiOffset) & maxValue) * (1.0f / maxValue);
    uiOffset += s_BitsPerDir[i];
  }
}


WD_STATICLINK_FILE(RendererCore, RendererCore_BakedProbes_Implementation_BakingUtils);
