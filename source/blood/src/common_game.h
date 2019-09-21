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

#define BLOODWIDESCREENDEF "blood_widescreen.def"

#define BYTEVERSION 102
#define EXEVERSION 101

void _SetErrorLoc(const char *pzFile, int nLine);
void _ThrowError(const char *pzFormat, ...);
void __dassert(const char *pzExpr, const char *pzFile, int nLine);
void QuitGame(void);

#define ThrowError(...) \
	{ \
		_SetErrorLoc(__FILE__,__LINE__); \
		_ThrowError(__VA_ARGS__); \
	}

#define dassert(x) if (!(x)) __dassert(#x,__FILE__,__LINE__)

#define kMaxSectors MAXSECTORS
#define kMaxWalls MAXWALLS
#define kMaxSprites MAXSPRITES

#define kMaxTiles MAXTILES
#define kMaxStatus MAXSTATUS
#define kMaxPlayers 8
#define kMaxViewSprites maxspritesonscreen

#define kMaxVoxels MAXVOXELS

#define kTicRate 120
#define kTicsPerFrame 4
#define kTicsPerSec (kTicRate/kTicsPerFrame)

#define TILTBUFFER 4078

#define kExplodeMax 8

#define kDudeBase 200
#define kDudePlayer1 231
#define kDudePlayer8 238
#define kDudeMax 260
#define kMissileBase 300
#define kMissileMax 318
#define kThingBase 400
#define kThingMax 436

#define kMaxPowerUps 51

#define kStatRespawn 8
#define kStatMarker 10
#define kStatGDXDudeTargetChanger 20
#define kStatFree 1024

#define kLensSize 80
#define kViewEffectMax 19

#define kNoTile -1


// defined by NoOne:
// -------------------------------
#define kMaxPAL 5

#define kItemBase 100
#define kWeaponItemBase 40
#define kItemMax 151

// marker sprite types
#define kMarkerSPStart 1
#define kMarkerMPStart 2
#define kMarkerOff 3
#define kMarkerOn 4
#define kMarkerAxis 5
#define kMarkerLowLink 6
#define kMarkerUpLink 7
#define kMarkerWarpDest 8
#define kMarkerUpWater 9
#define kMarkerLowWater 10
#define kMarkerUpStack 11
#define kMarkerLowStack 12
#define kMarkerUpGoo 13
#define kMarkerLowGoo 14
#define kMarkerPath 15

// sprite attributes
#define kHitagAutoAim 0x0008
#define kHitagRespawn 0x0010
#define kHitagFree 0x0020
#define kHitagSmoke 0x0100

// sprite physics attributes
#define kPhysMove 0x0001 // affected by movement physics
#define kPhysGravity 0x0002 // affected by gravity
#define kPhysFalling 0x0004 // currently in z-motion
// additional physics attributes for debris sprites
#define kPhysDebrisFly 0x0008 // *debris* affected by negative gravity (fly instead of falling, DO NOT mess with kHitagAutoAim)
#define kPhysDebrisVector 0x0400 // *debris* can be affected by vector weapons
#define kPhysDebrisExplode 0x0800 // *debris* can be affected by explosions

// *modern types only hitag*
#define kModernTypeFlag0 0x0
#define kModernTypeFlag1 0x1
#define kModernTypeFlag2 0x2
#define kModernTypeFlag3 0x3

// sector types 
#define kSecBase 600
#define kSecZMotion kSectorBase
#define kSecZSprite 602
#define kSecWarp 603
#define kSecTeleport 604
#define kSecPath 612
#define kSecRotateStep 613
#define kSecSlideMarked 614
#define kSecRotateMarked 615
#define kSecSlide 616
#define kSecRotate 617
#define kSecDamage 618
#define kSecCounter 619
#define kSecMax 620

// sector cstat
#define kSecCParallax 0x01
#define kSecCSloped 0x02
#define kSecCSwapXY 0x04
#define kSecCExpand 0x08
#define kSecCFlipX 0x10
#define kSecCFlipY 0x20
#define kSecCFlipMask 0x34
#define kSecCRelAlign 0x40
#define kSecCFloorShade 0x8000

// switch types
#define kSwitchBase 20
#define kSwitchToggle 20
#define kSwitchOneWay 21
#define kSwitchCombo 22
#define kSwitchPadlock 23
#define kSwitchMax 24

// projectile types
#define kProjectileEctoSkull 307

// custom level end
#define kGDXChannelEndLevelCustom 6

// GDX types
#define kGDXTypeBase 24
#define kGDXCustomDudeSpawn 24
#define kGDXRandomTX 25
#define kGDXSequentialTX 26
#define kGDXSeqSpawner 27
#define kGDXObjPropertiesChanger 28
#define kGDXObjPicnumChanger 29
#define kGDXObjSizeChanger 31
#define kGDXDudeTargetChanger 33
#define kGDXSectorFXChanger 34
#define kGDXObjDataChanger 35
#define kGDXSpriteDamager 36
#define kGDXObjDataAccumulator 37
#define kGDXEffectSpawner 38
#define kGDXWindGenerator 39
#define kModernConcussSprite 712

#define kGDXThingTNTProx 433 // detects only players
#define kGDXThingThrowableRock 434 // does small damage if hits target
#define kGDXThingCustomDudeLifeLeech 435 // the same as normal, except it aims in specified target
#define kCustomDude 254
#define kCustomDudeBurning 255

#define kGDXItemMapLevel 150 // once picked up, draws whole minimap

// ai state types
#define kAiStateOther -1
#define kAiStateIdle 0
#define kAiStateGenIdle 1
#define kAiStateMove 2
#define kAiStateSearch 3
#define kAiStateChase 4
#define kAiStateRecoil 5


#define kAng5 28
#define kAng15 85
#define kAng30 170
#define kAng45 256
#define kAng60 341
#define kAng90 512
#define kAng120 682
#define kAng180 1024
#define kAng360 2048


// -------------------------------

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

extern void G_SetupGlobalPsky(void);

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
    int x0, y0, x1, y1;
    Rect(int _x0, int _y0, int _x1, int _y1)
    {
        x0 = _x0; y0 = _y0; x1 = _x1; y1 = _y1;
    }
    bool isValid(void) const
    {
        return x0 < x1 && y0 < y1;
    }
    char isEmpty(void) const
    {
        return !isValid();
    }
    bool operator!(void) const
    {
        return isEmpty();
    }

    Rect & operator&=(Rect &pOther)
    {
        x0 = ClipLow(x0, pOther.x0);
        y0 = ClipLow(y0, pOther.y0);
        x1 = ClipHigh(x1, pOther.x1);
        y1 = ClipHigh(y1, pOther.y1);
        return *this;
    }

    void offset(int dx, int dy)
    {
        x0 += dx;
        y0 += dy;
        x1 += dx;
        y1 += dy;
    }

    int height()
    {
        return y1 - y0;
    }

    int width()
    {
        return x1 - x0;
    }

    bool inside(Rect& other)
    {
        return (x0 <= other.x0 && x1 >= other.x1 && y0 <= other.y0 && y1 >= other.y1);
    }

    bool inside(int x, int y)
    {
        return (x0 <= x && x1 > x && y0 <= y && y1 > y);
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

