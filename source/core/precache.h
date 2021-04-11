#pragma once

void PrecacheHardwareTextures(int nTile);
void markTileForPrecache(int tilenum, int palnum);
void precacheMarkedTiles();
