#pragma once

#include <stdint.h>
#include "textureid.h"

enum
{
    MAXPSKYTILES = 16,
};

struct SkyDefinition
{
	FTextureID		texid;
	int				baselineofs;
	float			scale;
	int				lognumtiles;
	int16_t			offsets[MAXPSKYTILES];
};

void addSky(SkyDefinition& sky, FTextureID texid);
void SetSkyOverride(float scale, int bits);
SkyDefinition getSky(FTextureID texid);
//void defineSky(FTextureID texid, int lognumtiles, const int16_t *tileofs, int yoff = 0, float yscale = 1.f, int yoff2 = 0x7fffffff);
void defineSky(const char* tilename, int lognumtiles, const int16_t* tileofs, int yoff = 0, float yscale = 1.f, int yoff2 = 0x7fffffff);

