#pragma once

#include "textureid.h"

void PrecacheHardwareTextures(int nTile);
void markTextureForPrecache(FTextureID texid, int palnum = 0);
void markTextureForPrecache(const char* texname, int palnum = 0);
void markVoxelForPrecache(int voxnum);
void precacheMarkedTiles();
void precacheMap();
