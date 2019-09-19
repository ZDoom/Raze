//-------------------------------------------------------------------------
/*
Copyright (C) 2010-2019 EDuke32 developers and contributors
Copyright (C) 2019 Nuke.YKT

This file is part of NBlood.

NBlood is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
//-------------------------------------------------------------------------
#pragma once
#include "baselayer.h"
#include "build.h"
#include "cache1d.h"
#include "common.h"
#include "pragmas.h"
#include "misc.h"
#include "network.h"

extern int g_useCwd;

#ifndef APPNAME
#define APPNAME "NBlood"
#endif

#ifndef APPBASENAME
#define APPBASENAME "nblood"
#endif

#define BYTEVERSION 102
#define EXEVERSION 101

void _SetErrorLoc(const char *pzFile, int nLine);
void _ThrowError(const char *pzFormat, ...);
void __dassert(const char *pzExpr, const char *pzFile, int nLine);

#define ThrowError(...) \
	{ \
		_SetErrorLoc(__FILE__,__LINE__); \
		_ThrowError(__VA_ARGS__); \
	}

#define dassert(x) if (!(x)) __dassert(#x,__FILE__,__LINE__)

#define kMaxSectors 1024
#define kMaxWalls 8192
#define kMaxSprites 4096

#define kMaxTiles MAXTILES
#define kMaxStatus MAXSTATUS
#define kMaxPlayers 8
#define kMaxViewSprites maxspritesonscreen

#define kMaxVoxels MAXVOXELS

#define kTicRate 120
#define kTicsPerFrame 4
#define kTicsPerSec (kTicRate/kTicsPerFrame)

// NUKE-TODO:
#define OSDTEXT_DEFAULT   "^00"
#define OSDTEXT_DARKRED   "^00"
#define OSDTEXT_GREEN     "^00"
#define OSDTEXT_RED       "^00"
#define OSDTEXT_YELLOW    "^00"

#define OSDTEXT_BRIGHT    "^S0"

#define OSD_ERROR OSDTEXT_DARKRED OSDTEXT_BRIGHT

enum BLOOD_GLOBALFLAGS {
    BLOOD_FORCE_WIDELOADSCREEN = 1<<0,
};

enum searchpathtypes_t {
    SEARCHPATH_REMOVE = 1<<0,
};

extern char *g_grpNamePtr;

extern int loaddefinitions_game(const char *fn, int32_t preload);

extern void G_AddSearchPaths(void);
extern void G_CleanupSearchPaths(void);

extern void G_ExtPreInit(int32_t argc, char const * const * argv);
extern void G_ExtInit(void);

void G_LoadGroupsInDir(const char *dirname);
void G_DoAutoload(const char *dirname);
extern void G_LoadGroups(int32_t autoload);

#define G_ModDirSnprintf(buf, size, basename, ...)                                                                                          \
    (((g_modDir[0] != '/') ? Bsnprintf(buf, size, "%s/" basename, g_modDir, ##__VA_ARGS__) : Bsnprintf(buf, size, basename, ##__VA_ARGS__)) \
     >= ((int32_t)size) - 1)

#define G_ModDirSnprintfLite(buf, size, basename) \
    ((g_modDir[0] != '/') ? Bsnprintf(buf, size, "%s/%s", g_modDir, basename) : Bsnprintf(buf, size, "%s", basename))

static inline void G_HandleAsync(void)
{
    handleevents();
    netGetPackets();
}

#if defined HAVE_FLAC || defined HAVE_VORBIS
# define FORMAT_UPGRADE_ELIGIBLE
extern int32_t S_OpenAudio(const char *fn, char searchfirst, uint8_t ismusic);
#else
# define S_OpenAudio(fn, searchfirst, ismusic) kopen4loadfrommod(fn, searchfirst)
#endif

#pragma pack(push,1)

#if 0
struct sectortype
{
    short wallptr, wallnum;
    int ceilingz, floorz;
    unsigned short ceilingstat, floorstat;
    short ceilingpicnum, ceilingheinum;
    signed char ceilingshade;
    char ceilingpal, ceilingxpanning, ceilingypanning;
    short floorpicnum, floorheinum;
    signed char floorshade;
    char floorpal, floorxpanning, floorypanning;
    char visibility, filler;
    unsigned short lotag;
    short hitag, extra;
};

struct walltype
{
    int x, y;
    short point2, nextwall, nextsector;
    unsigned short cstat;
    short picnum, overpicnum;
    signed char shade;
    char pal, xrepeat, yrepeat, xpanning, ypanning;
    short lotag, hitag, extra;
};

struct spritetype
{
    int x, y, z;
    short cstat, picnum;
    signed char shade;
    char pal, clipdist, filler;
    unsigned char xrepeat, yrepeat;
    signed char xoffset, yoffset;
    short sectnum, statnum;
    short ang, owner, index, yvel, zvel;
    short type, hitag, extra;
};

struct PICANM {
    unsigned int animframes : 5;
    unsigned int at0_5 : 1;
    unsigned int animtype : 2;
    signed int xoffset : 8;
    signed int yoffset : 8;
    unsigned int animspeed : 4;
    unsigned int at3_4 : 3; // type
    unsigned int at3_7 : 1; // filler
};
#endif

struct LOCATION {
    int x, y, z;
    int ang;
};

struct POINT2D {
    int x, y;
};

struct POINT3D {
    int x, y, z;
};

struct VECTOR2D {
    int dx, dy;
};

struct Aim {
    int dx, dy, dz;
};

#pragma pack(pop)

extern char qsprite_filler[], qsector_filler[];

inline int ksgnf(float f)
{
    if (f < 0)
        return -1;
    if (f > 0)
        return 1;
    return 0;
}

inline int IncBy(int a, int b)
{
    a += b;
    int q = a % b;
    a -= q;
    if (q < 0)
        a -= b;
    return a;
}

inline int DecBy(int a, int b)
{
    a--;
    int q = a % b;
    a -= q;
    if (q < 0)
        a -= b;
    return a;
}

#if 0
inline float IncByF(float a, float b)
{
    a += b;
    float q = fmod(a, b);
    a -= q;
    if (q < 0)
        a -= b;
    return a;
}

inline float DecByF(float a, float b)
{
    //a--;
    a -= fabs(b)*0.001;
    float q = fmod(a, b);
    a -= q;
    if (q < 0)
        a -= b;
    return a;
}
#endif

inline int ClipLow(int a, int b)
{
    if (a < b)
        return b;
    return a;
}

inline int ClipHigh(int a, int b)
{
    if (a >= b)
        return b;
    return a;
}

inline int ClipRange(int a, int b, int c)
{
    if (a < b)
        return b;
    if (a > c)
        return c;
    return a;
}

inline float ClipRangeF(float a, float b, float c)
{
    if (a < b)
        return b;
    if (a > c)
        return c;
    return a;
}

inline int interpolate(int a, int b, int c)
{
    return a+mulscale16(b-a,c);
}

inline int interpolateang(int a, int b, int c)
{
    return a+mulscale16(((b-a+1024)&2047)-1024, c);
}

inline fix16_t interpolateangfix16(fix16_t a, fix16_t b, int c)
{
    return a+mulscale16(((b-a+0x4000000)&0x7ffffff)-0x4000000, c);
}

inline char Chance(int a1)
{
    return wrand() < (a1>>1);
}

inline unsigned int Random(int a1)
{
    return mulscale(wrand(), a1, 15);
}

inline int Random2(int a1)
{
    return mulscale(wrand(), a1, 14)-a1;
}

inline int Random3(int a1)
{
    return mulscale(wrand()+wrand(), a1, 15) - a1;
}

inline unsigned int QRandom(int a1)
{
    return mulscale(qrand(), a1, 15);
}

inline int QRandom2(int a1)
{
    return mulscale(qrand(), a1, 14)-a1;
}

inline void SetBitString(char *pArray, int nIndex)
{
    pArray[nIndex>>3] |= 1<<(nIndex&7);
}

inline void ClearBitString(char *pArray, int nIndex)
{
    pArray[nIndex >> 3] &= ~(1 << (nIndex & 7));
}

inline char TestBitString(char *pArray, int nIndex)
{
    return pArray[nIndex>>3] & (1<<(nIndex&7));
}

inline int scale(int a1, int a2, int a3, int a4, int a5)
{
    return a4 + (a5-a4) * (a1-a2) / (a3-a2);
}

inline int mulscale16r(int a, int b)
{
    int64_t acc = 1<<(16-1);
    acc += ((int64_t)a) * b;
    return (int)(acc>>16);
}

inline int mulscale30r(int a, int b)
{
    int64_t acc = 1<<(30-1);
    acc += ((int64_t)a) * b;
    return (int)(acc>>30);
}

inline int dmulscale30r(int a, int b, int c, int d)
{
    int64_t acc = 1<<(30-1);
    acc += ((int64_t)a) * b;
    acc += ((int64_t)c) * d;
    return (int)(acc>>30);
}

inline int approxDist(int dx, int dy)
{
    dx = klabs(dx);
    dy = klabs(dy);
    if (dx > dy)
        dy = (3*dy)>>3;
    else
        dx = (3*dx)>>3;
    return dx+dy;
}

class Rect {
public:
    int x1, y1, x2, y2;
    Rect(int _x1, int _y1, int _x2, int _y2)
    {
        x1 = _x1; y1 = _y1; x2 = _x2; y2 = _y2;
    }
    bool isValid(void) const
    {
        return x1 < x2 && y1 < y2;
    }
    char isEmpty(void) const
    {
        return !(x1 < x2 && y1 < y2);
    }
    bool operator!(void) const
    {
        return isEmpty();
    }

    Rect & operator&=(Rect &pOther)
    {
        x1 = ClipLow(x1, pOther.x1);
        y1 = ClipLow(y1, pOther.y1);
        x2 = ClipHigh(x2, pOther.x2);
        y2 = ClipHigh(y2, pOther.y2);
        return *this;
    }
};

class BitReader {
public:
    int nBitPos;
    int nSize;
    char *pBuffer;
    BitReader(char *_pBuffer, int _nSize, int _nBitPos) { pBuffer = _pBuffer; nSize = _nSize; nBitPos = _nBitPos; nSize -= nBitPos>>3; }
    BitReader(char *_pBuffer, int _nSize) { pBuffer = _pBuffer; nSize = _nSize; nBitPos = 0; }
    int readBit()
    {
        if (nSize <= 0)
            ThrowError("Buffer overflow");
        int bit = ((*pBuffer)>>nBitPos)&1;
        if (++nBitPos >= 8)
        {
            nBitPos = 0;
            pBuffer++;
            nSize--;
        }
        return bit;
    }
    void skipBits(int nBits)
    {
        nBitPos += nBits;
        pBuffer += nBitPos>>3;
        nSize -= nBitPos>>3;
        nBitPos &= 7;
        if ((nSize == 0 && nBitPos > 0) || nSize < 0)
            ThrowError("Buffer overflow");
    }
    unsigned int readUnsigned(int nBits)
    {
        unsigned int n = 0;
        dassert(nBits <= 32);
        for (int i = 0; i < nBits; i++)
            n += readBit()<<i;
        return n;
    }
    int readSigned(int nBits)
    {
        dassert(nBits <= 32);
        int n = (int)readUnsigned(nBits);
        n <<= 32-nBits;
        n >>= 32-nBits;
        return n;
    }
};

class BitWriter {
public:
    int nBitPos;
    int nSize;
    char *pBuffer;
    BitWriter(char *_pBuffer, int _nSize, int _nBitPos) { pBuffer = _pBuffer; nSize = _nSize; nBitPos = _nBitPos; memset(pBuffer, 0, nSize); nSize -= nBitPos>>3; }
    BitWriter(char *_pBuffer, int _nSize) { pBuffer = _pBuffer; nSize = _nSize; nBitPos = 0; memset(pBuffer, 0, nSize); }
    void writeBit(int bit)
    {
        if (nSize <= 0)
            ThrowError("Buffer overflow");
        *pBuffer |= bit<<nBitPos;
        if (++nBitPos >= 8)
        {
            nBitPos = 0;
            pBuffer++;
            nSize--;
        }
    }
    void skipBits(int nBits)
    {
        nBitPos += nBits;
        pBuffer += nBitPos>>3;
        nSize -= nBitPos>>3;
        nBitPos &= 7;
        if ((nSize == 0 && nBitPos > 0) || nSize < 0)
            ThrowError("Buffer overflow");
    }
    void write(int nValue, int nBits)
    {
        dassert(nBits <= 32);
        for (int i = 0; i < nBits; i++)
            writeBit((nValue>>i)&1);
    }
};

