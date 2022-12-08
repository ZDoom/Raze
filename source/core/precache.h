#pragma once

#include "textureid.h"

void PrecacheHardwareTextures(int nTile);
void markTextureForPrecache(FTextureID texid, int palnum);
void markTextureForPrecache(const char* texname);
void markVoxelForPrecache(int voxnum);
void precacheMarkedTiles();
void precacheMap();
