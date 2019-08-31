
#ifndef __engine_h__
#define __engine_h__

#include "compat.h"
#include "build.h"
#include "pragmas.h"
#include "typedefs.h"

#define kMaxTiles	6144
#define kMaxSprites 4096
#define kMaxSectors 1024
#define kMaxWalls   8192
#define kMaxTiles   6144
#define kMaxVoxels	4096

enum
{
	kStatIgnited = 404
};

#define kMaxSpritesOnscreen		1024

#define kMaxPalookups 256
#define kMaxStatus   1024
#define MAXPSKYTILES 256

inline int Sin(int angle)
{
    return sintable[angle & 0x7FF];
}

int movesprite(short spritenum, int dx, int dy, int dz, int ceildist, int flordist, unsigned int clipmask);
void overwritesprite(int thex, int they, short tilenum, signed char shade, char stat, char dapalnum);
void precache();
void resettiming();
void printext(int x, int y, const char* buffer, short tilenum, char invisiblecol);
void kensetpalette(unsigned char *vgapal);

#endif
