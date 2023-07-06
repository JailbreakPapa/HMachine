#pragma once

#include <Texture/Image/Image.h>

class wdColorLinear16f;

WD_TEXTURE_DLL void wdDecompressBlockBC1(const wdUInt8* pSource, wdColorBaseUB* pTarget, bool bForceFourColorMode);
WD_TEXTURE_DLL void wdDecompressBlockBC4(const wdUInt8* pSource, wdUInt8* pTarget, wdUInt32 uiStride, wdUInt8 uiBias);
WD_TEXTURE_DLL void wdDecompressBlockBC6(const wdUInt8* pSource, wdColorLinear16f* pTarget, bool bIsSigned);
WD_TEXTURE_DLL void wdDecompressBlockBC7(const wdUInt8* pSource, wdColorBaseUB* pTarget);

WD_TEXTURE_DLL void wdUnpackPaletteBC4(wdUInt32 ui0, wdUInt32 ui1, wdUInt32* pAlphas);
