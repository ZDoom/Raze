#pragma once

void PrecacheHardwareTextures(int nTile);
void markTileForPrecache(int tilenum, int palnum);
void markTextureForPrecache(const char* texname);
void markVoxelForPrecache(int voxnum);
void precacheMarkedTiles();
