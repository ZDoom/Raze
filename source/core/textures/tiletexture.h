#pragma once

#include <stdint.h>
#include "image.h"
#include "textureid.h"
#include "tarray.h"

const uint8_t* GetRawPixels(FTextureID texid);
uint8_t* GetWritablePixels(FTextureID texid, bool reload = false);
FImageSource* createDummyTile(int width, int height);
void InitArtFiles(TArray<FString>& addart);
void GetArtImages(TArray<FImageSource*>& array, TArray<unsigned>& picanm);

enum
{
	MAXTILES = 30720,
	MAXUSERTILES = (MAXTILES - 16)  // reserve 16 tiles at the end
};
