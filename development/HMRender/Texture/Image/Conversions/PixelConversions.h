#pragma once

#include <Texture/Image/Image.h>

WD_TEXTURE_DLL wdColorBaseUB wdDecompressA4B4G4R4(wdUInt16 uiColor);
WD_TEXTURE_DLL wdColorBaseUB wdDecompressB4G4R4A4(wdUInt16 uiColor);
WD_TEXTURE_DLL wdColorBaseUB wdDecompressB5G6R5(wdUInt16 uiColor);
WD_TEXTURE_DLL wdColorBaseUB wdDecompressB5G5R5X1(wdUInt16 uiColor);
WD_TEXTURE_DLL wdColorBaseUB wdDecompressB5G5R5A1(wdUInt16 uiColor);
WD_TEXTURE_DLL wdColorBaseUB wdDecompressX1B5G5R5(wdUInt16 uiColor);
WD_TEXTURE_DLL wdColorBaseUB wdDecompressA1B5G5R5(wdUInt16 uiColor);
WD_TEXTURE_DLL wdUInt16 wdCompressA4B4G4R4(wdColorBaseUB color);
WD_TEXTURE_DLL wdUInt16 wdCompressB4G4R4A4(wdColorBaseUB color);
WD_TEXTURE_DLL wdUInt16 wdCompressB5G6R5(wdColorBaseUB color);
WD_TEXTURE_DLL wdUInt16 wdCompressB5G5R5X1(wdColorBaseUB color);
WD_TEXTURE_DLL wdUInt16 wdCompressB5G5R5A1(wdColorBaseUB color);
WD_TEXTURE_DLL wdUInt16 wdCompressX1B5G5R5(wdColorBaseUB color);
WD_TEXTURE_DLL wdUInt16 wdCompressA1B5G5R5(wdColorBaseUB color);
