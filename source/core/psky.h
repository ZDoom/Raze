#pragma once

#include <stdint.h>

enum
{
	DEFAULTPSKY = -1,
    MAXPSKYTILES = 16,
};

struct SkyDefinition
{
	int64_t 		crc32;			// CRC32 of the master tile
	int 			tilenum;
	int				baselineofs;
	float			scale;
	int				lognumtiles;
	int16_t			offsets[MAXPSKYTILES];
	
	int 			pmoffset; // offset for Polymost, should go away.
};

void addSky(SkyDefinition& sky, int tilenum);
void addSkyCRC(SkyDefinition& sky, int64_t crc32);
void SetSkyOverride(float scale, int bits);
SkyDefinition getSky(int tilenum);
void defineSky(int tilenum, int lognumtiles, const int16_t *tileofs, int yoff = 0, float yscale = 1.f, int yoff2 = 0x7fffffff);

