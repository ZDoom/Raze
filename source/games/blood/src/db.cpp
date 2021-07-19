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

#include "ns.h"	// Must come before everything else!

#include "build.h"
#include "compat.h"
#include "common_game.h"
#include "zstring.h"
#include "m_crc32.h"
#include "md4.h"
#include "automap.h"
#include "raze_sound.h"
#include "gamefuncs.h"
#include "hw_sections.h"
#include "sectorgeometry.h"

#include "blood.h"

BEGIN_BLD_NS

DBloodActor bloodActors[kMaxSprites];


bool gModernMap = false;
unsigned short gStatCount[kMaxStatus + 1];

XSPRITE xsprite[kMaxXSprites];
XSECTOR xsector[kMaxXSectors];
XWALL xwall[kMaxXWalls];

SPRITEHIT gSpriteHit[kMaxXSprites];

int xvel[kMaxSprites], yvel[kMaxSprites], zvel[kMaxSprites];

unsigned short nextXSprite[kMaxXSprites];
int XWallsUsed, XSectorsUsed;



char qsector_filler[kMaxSectors];

int gVisibility;

void dbCrypt(char *pPtr, int nLength, int nKey)
{
    for (int i = 0; i < nLength; i++)
    {
        pPtr[i] = pPtr[i] ^ nKey;
        nKey++;
    }
}

void InsertSpriteSect(int nSprite, int nSector)
{
    assert(nSprite >= 0 && nSprite < kMaxSprites);
    assert(nSector >= 0 && nSector < kMaxSectors);
    int nOther = headspritesect[nSector];
    if (nOther >= 0)
    {
        prevspritesect[nSprite] = prevspritesect[nOther];
        nextspritesect[nSprite] = -1;
        nextspritesect[prevspritesect[nOther]] = nSprite;
        prevspritesect[nOther] = nSprite;
    }
    else
    {
        prevspritesect[nSprite] = nSprite;
        nextspritesect[nSprite] = -1;
        headspritesect[nSector] = nSprite;
    }
    sprite[nSprite].sectnum = nSector;
}

void RemoveSpriteSect(int nSprite)
{
    assert(nSprite >= 0 && nSprite < kMaxSprites);
    int nSector = sprite[nSprite].sectnum;
    assert(nSector >= 0 && nSector < kMaxSectors);
    int nOther = nextspritesect[nSprite];
    if (nOther < 0)
    {
        nOther = headspritesect[nSector];
    }
    prevspritesect[nOther] = prevspritesect[nSprite];
    if (headspritesect[nSector] != nSprite)
    {
        nextspritesect[prevspritesect[nSprite]] = nextspritesect[nSprite];
    }
    else
    {
        headspritesect[nSector] = nextspritesect[nSprite];
    }
    sprite[nSprite].sectnum = MAXSECTORS;
}

void InsertSpriteStat(int nSprite, int nStat)
{
    assert(nSprite >= 0 && nSprite < kMaxSprites);
    assert(nStat >= 0 && nStat <= kMaxStatus);
    int nOther = headspritestat[nStat];
    if (nOther >= 0)
    {
        prevspritestat[nSprite] = prevspritestat[nOther];
        nextspritestat[nSprite] = -1;
        nextspritestat[prevspritestat[nOther]] = nSprite;
        prevspritestat[nOther] = nSprite;
    }
    else
    {
        prevspritestat[nSprite] = nSprite;
        nextspritestat[nSprite] = -1;
        headspritestat[nStat] = nSprite;
    }
    sprite[nSprite].statnum = nStat;
    gStatCount[nStat]++;
}

void RemoveSpriteStat(int nSprite)
{
    assert(nSprite >= 0 && nSprite < kMaxSprites);
    int nStat = sprite[nSprite].statnum;
    assert(nStat >= 0 && nStat <= kMaxStatus);
    int nOther = nextspritestat[nSprite];
    if (nOther < 0)
    {
        nOther = headspritestat[nStat];
    }
    prevspritestat[nOther] = prevspritestat[nSprite];
    if (headspritestat[nStat] != nSprite)
    {
        nextspritestat[prevspritestat[nSprite]] = nextspritestat[nSprite];
    }
    else
    {
        headspritestat[nStat] = nextspritestat[nSprite];
    }
    sprite[nSprite].statnum = MAXSTATUS;
    gStatCount[nStat]--;
}

void qinitspritelists(void) // Replace
{
    for (short i = 0; i <= kMaxSectors; i++)
    {
        headspritesect[i] = -1;
    }
    for (short i = 0; i <= kMaxStatus; i++)
    {
        headspritestat[i] = -1;
    }
    int const nMaxSprites = kMaxSprites;
    for (short i = 0; i < nMaxSprites; i++)
    {
        sprite[i].sectnum = -1;
        sprite[i].index = -1;
        InsertSpriteStat(i, kMaxStatus);
    }
    memset(gStatCount, 0, sizeof(gStatCount));
    Numsprites = 0;
}

int InsertSprite(int nSector, int nStat)
{
    int nSprite = headspritestat[kMaxStatus];
    assert(nSprite < kMaxSprites);
    if (nSprite < 0)
    {
        return nSprite;
    }
    RemoveSpriteStat(nSprite);
    spritetype *pSprite = &sprite[nSprite];
    memset(&sprite[nSprite], 0, sizeof(spritetype));
    InsertSpriteStat(nSprite, nStat);
    InsertSpriteSect(nSprite, nSector);
    pSprite->cstat = 128;
    pSprite->clipdist = 32;
    pSprite->xrepeat = pSprite->yrepeat = 64;
    pSprite->owner = -1;
    pSprite->extra = -1;
    pSprite->index = nSprite;
    xvel[nSprite] = yvel[nSprite] = zvel[nSprite] = 0;

    Numsprites++;

    sprite[nSprite].time = leveltimer++;
    return nSprite;
}

int qinsertsprite(short nSector, short nStat) // Replace
{
    return InsertSprite(nSector, nStat);
}

int DeleteSprite(int nSprite)
{
    FVector3 pos = GetSoundPos(&sprite[nSprite].pos);
    soundEngine->RelinkSound(SOURCE_Actor, &sprite[nSprite], nullptr, &pos);

    if (sprite[nSprite].extra > 0)
    {
        dbDeleteXSprite(sprite[nSprite].extra);
    }
    assert(sprite[nSprite].statnum >= 0 && sprite[nSprite].statnum < kMaxStatus);
    RemoveSpriteStat(nSprite);
    assert(sprite[nSprite].sectnum >= 0 && sprite[nSprite].sectnum < kMaxSectors);
    RemoveSpriteSect(nSprite);
    InsertSpriteStat(nSprite, kMaxStatus);

    Numsprites--;

    return nSprite;
}

int qdeletesprite(short nSprite) // Replace
{
    return DeleteSprite(nSprite);
}

int ChangeSpriteSect(int nSprite, int nSector)
{
    assert(nSprite >= 0 && nSprite < kMaxSprites);
    assert(nSector >= 0 && nSector < kMaxSectors);
    assert(sprite[nSprite].sectnum >= 0 && sprite[nSprite].sectnum < kMaxSectors);
    RemoveSpriteSect(nSprite);
    InsertSpriteSect(nSprite, nSector);
    return 0;
}

int qchangespritesect(short nSprite, short nSector)
{
    return ChangeSpriteSect(nSprite, nSector);
}

int ChangeSpriteStat(int nSprite, int nStatus)
{
    assert(nSprite >= 0 && nSprite < kMaxSprites);
    assert(nStatus >= 0 && nStatus < kMaxStatus);
    assert(sprite[nSprite].statnum >= 0 && sprite[nSprite].statnum < kMaxStatus);
    assert(sprite[nSprite].sectnum >= 0 && sprite[nSprite].sectnum < kMaxSectors);
    RemoveSpriteStat(nSprite);
    InsertSpriteStat(nSprite, nStatus);
    return 0;
}

int qchangespritestat(short nSprite, short nStatus)
{
    return ChangeSpriteStat(nSprite, nStatus);
}

void InitFreeList(unsigned short *pList, int nCount)
{
    for (int i = 1; i < nCount; i++)
    {
        pList[i] = i-1;
    }
    pList[0] = nCount - 1;
}

void InitFreeList(unsigned short* pList, int nCount, FixedBitArray<MAXSPRITES>&used)
{
    int lastfree = 0;
    for (int i = 1; i < nCount; i++)
    {
        if (!used[i])
        {
            pList[i] = lastfree;
            lastfree = i;
        }
    }
    pList[0] = lastfree;
}

void InsertFree(unsigned short *pList, int nIndex)
{
    pList[nIndex] = pList[0];
    pList[0] = nIndex;
}

unsigned short dbInsertXSprite(int nSprite)
{
    int nXSprite = nextXSprite[0];
    nextXSprite[0] = nextXSprite[nXSprite];
    if (nXSprite == 0)
    {
        I_Error("Out of free XSprites");
    }
    memset(&xsprite[nXSprite], 0, sizeof(XSPRITE));
    memset(&gSpriteHit[nXSprite], 0, sizeof(SPRITEHIT));
    xsprite[nXSprite].reference = nSprite;
    sprite[nSprite].extra = nXSprite;
    return nXSprite;
}

void dbDeleteXSprite(int nXSprite)
{
    assert(xsprite[nXSprite].reference >= 0);
    assert(sprite[xsprite[nXSprite].reference].extra == nXSprite);
    InsertFree(nextXSprite, nXSprite);
    sprite[xsprite[nXSprite].reference].extra = -1;
    xsprite[nXSprite].reference = -1;
}

unsigned short dbInsertXWall(int nWall)
{
    int nXWall = XWallsUsed++;
    if (nXWall >= kMaxXWalls)
    {
        I_Error("Out of free XWalls");
    }
    memset(&xwall[nXWall], 0, sizeof(XWALL));
    xwall[nXWall].reference = nWall;
    wall[nWall].extra = nXWall;
    return nXWall;
}

unsigned short dbInsertXSector(int nSector)
{
    int nXSector = XSectorsUsed++;
    if (nXSector >= kMaxXSectors)
    {
        I_Error("Out of free XSectors");
    }
    memset(&xsector[nXSector], 0, sizeof(XSECTOR));
    xsector[nXSector].reference = nSector;
    sector[nSector].extra = nXSector;
    return nXSector;
}

void dbInit(void)
{
    InitFreeList(nextXSprite, kMaxXSprites);
    for (int i = 1; i < kMaxXSprites; i++)
    {
        xsprite[i].reference = -1;
    }
    XWallsUsed = XSectorsUsed = 1;  // 0 is not usable because it's the default for 'extra' and some code actually uses it to clobber the contents in here. :(
    for (int i = 1; i < kMaxXWalls; i++)
    {
        xwall[i].reference = -1;
    }
    for (int i = 1; i < kMaxXSectors; i++)
    {
        xsector[i].reference = -1;
    }
    initspritelists();
    for (int i = 0; i < kMaxSprites; i++)
    {
        sprite[i].cstat = 128;
    }
}

void PropagateMarkerReferences(void)
{
    int nSprite, nNextSprite;
    for (nSprite = headspritestat[kStatMarker]; nSprite != -1; nSprite = nNextSprite) {
        
        nNextSprite = nextspritestat[nSprite];
        
        switch (sprite[nSprite].type)  {
            case kMarkerOff:
            case kMarkerAxis:
            case kMarkerWarpDest: {
                int nOwner = sprite[nSprite].owner;
                if (nOwner >= 0 && nOwner < numsectors) {
                    int nXSector = sector[nOwner].extra;
                    if (nXSector > 0 && nXSector < kMaxXSectors) {
                        xsector[nXSector].marker0 = nSprite;
                        continue;
                    }
                }
            }
            break;
            case kMarkerOn: {
                int nOwner = sprite[nSprite].owner;
                if (nOwner >= 0 && nOwner < numsectors) {
                    int nXSector = sector[nOwner].extra;
                    if (nXSector > 0 && nXSector < kMaxXSectors) {
                        xsector[nXSector].marker1 = nSprite;
                        continue;
                    }
                }
            }
            break;
        }
        
        DeleteSprite(nSprite);
    }
}

bool drawtile2048, encrypted;

MAPHEADER2 byte_19AE44;

unsigned int dbReadMapCRC(const char *pPath)
{
    encrypted = 0;

    FString mapname = pPath;
    DefaultExtension(mapname, ".map");
    auto fr = fileSystem.OpenFileReader(mapname);

    if (!fr.isOpen())
    {
        Printf("Error opening map file %s", pPath);
        return -1;
    }

    MAPSIGNATURE header;
    fr.Read(&header, 6);
    if (memcmp(header.signature, "BLM\x1a", 4))
    {
        I_Error("%s: Map file corrupted.", mapname.GetChars());
    }
    int ver = LittleShort(header.version);
    if ((ver & 0xff00) == 0x600)
    {
    }
    else if ((ver & 0xff00) == 0x700)
    {
        encrypted = 1;
    }
    else
    {
        I_Error("%s: Map file is wrong version.", mapname.GetChars());
    }
    fr.Seek(-4, FileReader::SeekEnd);
    return fr.ReadInt32();
}

int gMapRev, gMattId, gSkyCount;
//char byte_19AE44[128];
const int nXSectorSize = 60;
const int nXSpriteSize = 56;
const int nXWallSize = 24;


#pragma pack(push, 1)
// This is the on-disk format. Only Blood still needs this for its retarded encryption that has to read this in as a block so that it can be decoded.
// Keep it local so that the engine's sprite type is no longer limited by file format restrictions.
struct spritetypedisk
{
    int32_t x, y, z;
    uint16_t cstat;
    int16_t picnum;
    int8_t shade;
    uint8_t pal, clipdist, detail;
    uint8_t xrepeat, yrepeat;
    int8_t xoffset, yoffset;
    int16_t sectnum, statnum;
    int16_t ang, owner;
    int16_t index, yvel, inittype;
    int16_t type;
    int16_t hitag;
    int16_t extra;
};

struct sectortypedisk
{
    int16_t wallptr, wallnum;
    int32_t ceilingz, floorz;
    uint16_t ceilingstat, floorstat;
    int16_t ceilingpicnum, ceilingheinum;
    int8_t ceilingshade;
    uint8_t ceilingpal, ceilingxpanning, ceilingypanning;
    int16_t floorpicnum, floorheinum;
    int8_t floorshade;
    uint8_t floorpal, floorxpanning, floorypanning;
    uint8_t visibility, fogpal;
    int16_t type;
    int16_t hitag;
    int16_t extra;
};

struct walltypedisk
{
    int32_t x, y;
    int16_t point2, nextwall, nextsector;
    uint16_t cstat;
    int16_t picnum, overpicnum;
    int8_t shade;
    uint8_t pal, xrepeat, yrepeat, xpanning, ypanning;
    int16_t type;
    int16_t hitag;
    int16_t extra;
};

#pragma pack(pop)


void dbLoadMap(const char *pPath, int *pX, int *pY, int *pZ, short *pAngle, short *pSector, unsigned int *pCRC) {
    int16_t tpskyoff[256];
    ClearAutomap();
    #ifdef NOONE_EXTENSIONS
    gModernMap = false;
    #endif

    memset(sector, 0, sizeof(*sector) * MAXSECTORS);
    memset(wall, 0, sizeof(*wall) * MAXWALLS);
    memset(sprite, 0, sizeof(*sector) * MAXSPRITES);

#ifdef USE_OPENGL
    Polymost::Polymost_prepare_loadboard();
#endif

    FString mapname = pPath;
    DefaultExtension(mapname, ".map");
    auto fr = fileSystem.OpenFileReader(mapname);

    if (!fr.isOpen())
    {
        I_Error("Error opening map file %s", mapname.GetChars());
    }
    MAPSIGNATURE header;
    fr.Read(&header, 6);
    if (memcmp(header.signature, "BLM\x1a", 4))
    {
        I_Error("%s: Map file corrupted", mapname.GetChars());
    }
    encrypted = 0;
    if ((LittleShort(header.version) & 0xff00) == 0x700) {
        encrypted = 1;
        
        #ifdef NOONE_EXTENSIONS
        // indicate if the map requires modern features to work properly
        // for maps wich created in PMAPEDIT BETA13 or higher versions. Since only minor version changed,
        // the map is still can be loaded with vanilla BLOOD / MAPEDIT and should work in other ports too.
        if ((header.version & 0x00ff) == 0x001) gModernMap = true;
        #endif

    } else {
        I_Error("%s: Map file is wrong version", mapname.GetChars());
    }

    MAPHEADER mapHeader;
    fr.Read(&mapHeader,37/* sizeof(mapHeader)*/);
    if (mapHeader.mattid != 0 && mapHeader.mattid != 0x7474614d && mapHeader.mattid != 0x4d617474) {
        dbCrypt((char*)&mapHeader, sizeof(mapHeader), 0x7474614d);
    }

    mapHeader.x = LittleLong(mapHeader.x);
    mapHeader.y = LittleLong(mapHeader.y);
    mapHeader.z = LittleLong(mapHeader.z);
    mapHeader.ang = LittleShort(mapHeader.ang);
    mapHeader.sect = LittleShort(mapHeader.sect);
    mapHeader.pskybits = LittleShort(mapHeader.pskybits);
    mapHeader.visibility = LittleLong(mapHeader.visibility);
    mapHeader.mattid = LittleLong(mapHeader.mattid);
    mapHeader.revision = LittleLong(mapHeader.revision);
    mapHeader.numsectors = LittleShort(mapHeader.numsectors);
    mapHeader.numwalls = LittleShort(mapHeader.numwalls);
    mapHeader.numsprites = LittleShort(mapHeader.numsprites);

    *pX = mapHeader.x;
    *pY = mapHeader.y;
    *pZ = mapHeader.z;
    *pAngle = mapHeader.ang;
    *pSector = mapHeader.sect;
    gVisibility = g_visibility = mapHeader.visibility;
    gMattId = mapHeader.mattid;
    if (encrypted)
    {
        if (mapHeader.mattid == 0x7474614d || mapHeader.mattid == 0x4d617474)
        {
            drawtile2048 = 1;
        }
        else if (!mapHeader.mattid)
        {
            drawtile2048 = 0;
        }
        else
        {
            I_Error("%s: Corrupted Map file", mapname.GetChars());
        }
    }
    else if (mapHeader.mattid)
    {
        I_Error("%s: Corrupted Map file", mapname.GetChars());
    }
    parallaxtype = mapHeader.parallax;
    gMapRev = mapHeader.revision;
    numsectors = mapHeader.numsectors;
    numwalls = mapHeader.numwalls;
    dbInit();
    if (encrypted)
    {
        fr.Read(&byte_19AE44, 128);
        dbCrypt((char*)&byte_19AE44, 128, numwalls);

        byte_19AE44.numxsprites = LittleLong(byte_19AE44.numxsprites);
        byte_19AE44.numxwalls = LittleLong(byte_19AE44.numxwalls);
        byte_19AE44.numxsectors = LittleLong(byte_19AE44.numxsectors);
    }
    else
    {
        memset(&byte_19AE44, 0, 128);
    }
    gSkyCount = 1<< mapHeader.pskybits;
    fr.Read(tpskyoff, gSkyCount*sizeof(tpskyoff[0]));
    if (encrypted)
    {
        dbCrypt((char*)tpskyoff, gSkyCount*sizeof(tpskyoff[0]), gSkyCount*2);
    }

    psky_t* pSky = tileSetupSky(DEFAULTPSKY);
    pSky->horizfrac = 65536;
    pSky->lognumtiles = mapHeader.pskybits;
    for (int i = 0; i < ClipHigh(gSkyCount, MAXPSKYTILES); i++)
    {
        pSky->tileofs[i] = LittleShort(tpskyoff[i]);
    }

    for (int i = 0; i < numsectors; i++)
    {
        sectortype *pSector = &sector[i];
        sectortypedisk load;
        fr.Read(&load, sizeof(sectortypedisk));
        if (encrypted)
        {
            dbCrypt((char*)&load, sizeof(sectortypedisk), gMapRev*sizeof(sectortypedisk));
        }
        pSector->wallptr = LittleShort(load.wallptr);
        pSector->wallnum = LittleShort(load.wallnum);
        pSector->ceilingz = LittleLong(load.ceilingz);
        pSector->floorz = LittleLong(load.floorz);
        pSector->ceilingstat = LittleShort(load.ceilingstat);
        pSector->floorstat = LittleShort(load.floorstat);
        pSector->ceilingpicnum = LittleShort(load.ceilingpicnum);
        pSector->ceilingheinum = LittleShort(load.ceilingheinum);
        pSector->floorpicnum = LittleShort(load.floorpicnum);
        pSector->floorheinum = LittleShort(load.floorheinum);
        pSector->type = LittleShort(load.type);
        pSector->hitag = LittleShort(load.hitag);
        pSector->extra = LittleShort(load.extra);
        pSector->ceilingshade = load.ceilingshade;
        pSector->ceilingpal = load.ceilingpal;
        pSector->ceilingxpan_ = load.ceilingxpanning;
        pSector->ceilingypan_ = load.ceilingypanning;
        pSector->floorshade = load.floorshade;
        pSector->floorpal = load.floorpal;
        pSector->floorxpan_ = load.floorxpanning;
        pSector->floorypan_ = load.floorypanning;
        pSector->visibility = load.visibility;
        qsector_filler[i] = load.fogpal;
        pSector->dirty = 255;
        pSector->exflags = 0;
        pSector->fogpal = 0;

        if (sector[i].extra > 0)
        {
            char pBuffer[nXSectorSize];
            int nXSector = dbInsertXSector(i);
            XSECTOR *pXSector = &xsector[nXSector];
            memset(pXSector, 0, sizeof(XSECTOR));
            int nCount;
            if (!encrypted)
            {
                nCount = nXSectorSize;
            }
            else
            {
                nCount = byte_19AE44.numxsectors;
            }
            assert(nCount <= nXSectorSize);
            fr.Read(pBuffer, nCount);
            BitReader bitReader(pBuffer, nCount);
            pXSector->reference = bitReader.readSigned(14);
            pXSector->state = bitReader.readUnsigned(1);
            pXSector->busy = bitReader.readUnsigned(17);
            pXSector->data = bitReader.readUnsigned(16);
            pXSector->txID = bitReader.readUnsigned(10);
            pXSector->busyWaveA = bitReader.readUnsigned(3);
            pXSector->busyWaveB = bitReader.readUnsigned(3);
            pXSector->rxID = bitReader.readUnsigned(10);
            pXSector->command = bitReader.readUnsigned(8);
            pXSector->triggerOn = bitReader.readUnsigned(1);
            pXSector->triggerOff = bitReader.readUnsigned(1);
            pXSector->busyTimeA = bitReader.readUnsigned(12);
            pXSector->waitTimeA = bitReader.readUnsigned(12);
            pXSector->restState = bitReader.readUnsigned(1);
            pXSector->interruptable = bitReader.readUnsigned(1);
            pXSector->amplitude = bitReader.readSigned(8);
            pXSector->freq = bitReader.readUnsigned(8);
            pXSector->reTriggerA = bitReader.readUnsigned(1);
            pXSector->reTriggerB = bitReader.readUnsigned(1);
            pXSector->phase = bitReader.readUnsigned(8);
            pXSector->wave = bitReader.readUnsigned(4);
            pXSector->shadeAlways = bitReader.readUnsigned(1);
            pXSector->shadeFloor = bitReader.readUnsigned(1);
            pXSector->shadeCeiling = bitReader.readUnsigned(1);
            pXSector->shadeWalls = bitReader.readUnsigned(1);
            pXSector->shade = bitReader.readSigned(8);
            pXSector->panAlways = bitReader.readUnsigned(1);
            pXSector->panFloor = bitReader.readUnsigned(1);
            pXSector->panCeiling = bitReader.readUnsigned(1);
            pXSector->Drag = bitReader.readUnsigned(1);
            pXSector->Underwater = bitReader.readUnsigned(1);
            pXSector->Depth = bitReader.readUnsigned(3);
            pXSector->panVel = bitReader.readUnsigned(8);
            pXSector->panAngle = bitReader.readUnsigned(11);
            pXSector->unused1 = bitReader.readUnsigned(1);
            pXSector->decoupled = bitReader.readUnsigned(1);
            pXSector->triggerOnce = bitReader.readUnsigned(1);
            pXSector->isTriggered = bitReader.readUnsigned(1);
            pXSector->Key = bitReader.readUnsigned(3);
            pXSector->Push = bitReader.readUnsigned(1);
            pXSector->Vector = bitReader.readUnsigned(1);
            pXSector->Reserved = bitReader.readUnsigned(1);
            pXSector->Enter = bitReader.readUnsigned(1);
            pXSector->Exit = bitReader.readUnsigned(1);
            pXSector->Wallpush = bitReader.readUnsigned(1);
            pXSector->color = bitReader.readUnsigned(1);
            /*pXSector->unused2 =*/ bitReader.readUnsigned(1);
            pXSector->busyTimeB = bitReader.readUnsigned(12);
            pXSector->waitTimeB = bitReader.readUnsigned(12);
            pXSector->stopOn = bitReader.readUnsigned(1);
            pXSector->stopOff = bitReader.readUnsigned(1);
            pXSector->ceilpal = bitReader.readUnsigned(4);
            pXSector->offCeilZ = bitReader.readSigned(32);
            pXSector->onCeilZ = bitReader.readSigned(32);
            pXSector->offFloorZ = bitReader.readSigned(32);
            pXSector->onFloorZ = bitReader.readSigned(32);
            pXSector->marker0 = bitReader.readUnsigned(16);
            pXSector->marker1 = bitReader.readUnsigned(16);
            pXSector->Crush = bitReader.readUnsigned(1);
            pSector->ceilingxpan_ += bitReader.readUnsigned(8) / 256.f;
            pSector->ceilingypan_ += bitReader.readUnsigned(8) / 256.f;
            pSector->floorxpan_ += bitReader.readUnsigned(8) / 256.f;
            pXSector->damageType = bitReader.readUnsigned(3);
            pXSector->floorpal = bitReader.readUnsigned(4);
            pSector->floorypan_ += bitReader.readUnsigned(8) / 256.f;
            pXSector->locked = bitReader.readUnsigned(1);
            pXSector->windVel = bitReader.readUnsigned(10);
            pXSector->windAng = bitReader.readUnsigned(11);
            pXSector->windAlways = bitReader.readUnsigned(1);
            pXSector->dudeLockout = bitReader.readUnsigned(1);
            pXSector->bobTheta = bitReader.readUnsigned(11);
            pXSector->bobZRange = bitReader.readUnsigned(5);
            pXSector->bobSpeed = bitReader.readSigned(12);
            pXSector->bobAlways = bitReader.readUnsigned(1);
            pXSector->bobFloor = bitReader.readUnsigned(1);
            pXSector->bobCeiling = bitReader.readUnsigned(1);
            pXSector->bobRotate = bitReader.readUnsigned(1);
            xsector[sector[i].extra].reference = i;
            xsector[sector[i].extra].busy = IntToFixed(xsector[sector[i].extra].state);

        }
    }
    for (int i = 0; i < numwalls; i++)
    {
        walltype *pWall = &wall[i];
        walltypedisk load;
        fr.Read(&load, sizeof(walltypedisk));
        if (encrypted)
        {
            dbCrypt((char*)&load, sizeof(walltypedisk), (gMapRev*sizeof(sectortypedisk)) | 0x7474614d);
        }
        pWall->x = LittleLong(load.x);
        pWall->y = LittleLong(load.y);
        pWall->point2 = LittleShort(load.point2);
        pWall->nextwall = LittleShort(load.nextwall);
        pWall->nextsector = LittleShort(load.nextsector);
        pWall->cstat = LittleShort(load.cstat);
        pWall->picnum = LittleShort(load.picnum);
        pWall->overpicnum = LittleShort(load.overpicnum);
        pWall->type = LittleShort(load.type);
        pWall->hitag = LittleShort(load.hitag);
        pWall->extra = LittleShort(load.extra);
        pWall->shade = load.shade;
        pWall->pal = load.pal;
        pWall->xrepeat = load.xrepeat;
        pWall->xpan_ = load.xpanning;
        pWall->yrepeat = load.yrepeat;
        pWall->ypan_ = load.ypanning;

        if (wall[i].extra > 0)
        {
            char pBuffer[nXWallSize];
            int nXWall = dbInsertXWall(i);
            XWALL *pXWall = &xwall[nXWall];
            memset(pXWall, 0, sizeof(XWALL));
            int nCount;
            if (!encrypted)
            {
                nCount = nXWallSize;
            }
            else
            {
                nCount = byte_19AE44.numxwalls;
            }
            assert(nCount <= nXWallSize);
            fr.Read(pBuffer, nCount);
            BitReader bitReader(pBuffer, nCount);
            pXWall->reference = bitReader.readSigned(14);
            pXWall->state = bitReader.readUnsigned(1);
            pXWall->busy = bitReader.readUnsigned(17);
            pXWall->data = bitReader.readSigned(16);
            pXWall->txID = bitReader.readUnsigned(10);
            bitReader.readUnsigned(6);
            pXWall->rxID = bitReader.readUnsigned(10);
            pXWall->command = bitReader.readUnsigned(8);
            pXWall->triggerOn = bitReader.readUnsigned(1);
            pXWall->triggerOff = bitReader.readUnsigned(1);
            pXWall->busyTime = bitReader.readUnsigned(12);
            pXWall->waitTime = bitReader.readUnsigned(12);
            pXWall->restState = bitReader.readUnsigned(1);
            pXWall->interruptable = bitReader.readUnsigned(1);
            pXWall->panAlways = bitReader.readUnsigned(1);
            pXWall->panXVel = bitReader.readSigned(8);
            pXWall->panYVel = bitReader.readSigned(8);
            pXWall->decoupled = bitReader.readUnsigned(1);
            pXWall->triggerOnce = bitReader.readUnsigned(1);
            pXWall->isTriggered = bitReader.readUnsigned(1);
            pXWall->key = bitReader.readUnsigned(3);
            pXWall->triggerPush = bitReader.readUnsigned(1);
            pXWall->triggerVector = bitReader.readUnsigned(1);
            pXWall->triggerTouch = bitReader.readUnsigned(1);
            bitReader.readUnsigned(2);
            pWall->xpan_ += bitReader.readUnsigned(8) / 256.f;
            pWall->ypan_ += bitReader.readUnsigned(8) / 256.f;
            pXWall->locked = bitReader.readUnsigned(1);
            pXWall->dudeLockout = bitReader.readUnsigned(1);
            bitReader.readUnsigned(4);
            bitReader.readUnsigned(32);
            xwall[wall[i].extra].reference = i;
            xwall[wall[i].extra].busy = IntToFixed(xwall[wall[i].extra].state);

        }
    }
    initspritelists();
    for (int i = 0; i < mapHeader.numsprites; i++)
    {
        RemoveSpriteStat(i);
        spritetypedisk load;
        spritetype *pSprite = &sprite[i];
        fr.Read(&load, sizeof(spritetypedisk)); // load into an intermediate buffer so that spritetype is no longer bound by file formats.
        if (encrypted) // What were these people thinking? :(
        {
            dbCrypt((char*)&load, sizeof(spritetypedisk), (gMapRev*sizeof(spritetypedisk)) | 0x7474614d);
        }

        pSprite->x = LittleLong(load.x);
        pSprite->y = LittleLong(load.y);
        pSprite->z = LittleLong(load.z);
        pSprite->cstat = LittleShort(load.cstat);
        pSprite->picnum = LittleShort(load.picnum);
        pSprite->sectnum = LittleShort(load.sectnum);
        pSprite->statnum = LittleShort(load.statnum);
        pSprite->ang = LittleShort(load.ang);
        pSprite->owner = LittleShort(load.owner);
        pSprite->index = LittleShort(load.index);
        pSprite->yvel = LittleShort(load.yvel);
        pSprite->inittype = LittleShort(load.inittype);
        pSprite->type = LittleShort(load.type);
        pSprite->flags = LittleShort(load.hitag);
        pSprite->extra = LittleShort(load.extra);
        pSprite->pal = load.pal;
        pSprite->clipdist = load.clipdist;
        pSprite->xrepeat = load.xrepeat;
        pSprite->yrepeat = load.yrepeat;
        pSprite->xoffset = load.xoffset;
        pSprite->yoffset = load.yoffset;
        pSprite->detail = load.detail;
        pSprite->shade = load.shade;
        pSprite->blend = 0;

        InsertSpriteSect(i, sprite[i].sectnum);
        InsertSpriteStat(i, sprite[i].statnum);
        Numsprites++;
        sprite[i].index = i;
        if (sprite[i].extra > 0)
        {
            char pBuffer[nXSpriteSize];
            int nXSprite = dbInsertXSprite(i);
            XSPRITE *pXSprite = &xsprite[nXSprite];
            memset(pXSprite, 0, sizeof(XSPRITE));
            int nCount;
            if (!encrypted)
            {
                nCount = nXSpriteSize;
            }
            else
            {
                nCount = byte_19AE44.numxsprites;
            }
            assert(nCount <= nXSpriteSize);
            fr.Read(pBuffer, nCount);
            BitReader bitReader(pBuffer, nCount);
            pXSprite->reference = bitReader.readSigned(14);
            pXSprite->state = bitReader.readUnsigned(1);
            pXSprite->busy = bitReader.readUnsigned(17);
            pXSprite->txID = bitReader.readUnsigned(10);
            pXSprite->rxID = bitReader.readUnsigned(10);
            pXSprite->command = bitReader.readUnsigned(8);
            pXSprite->triggerOn = bitReader.readUnsigned(1);
            pXSprite->triggerOff = bitReader.readUnsigned(1);
            pXSprite->wave = bitReader.readUnsigned(2);
            pXSprite->busyTime = bitReader.readUnsigned(12);
            pXSprite->waitTime = bitReader.readUnsigned(12);
            pXSprite->restState = bitReader.readUnsigned(1);
            pXSprite->Interrutable = bitReader.readUnsigned(1);
            pXSprite->unused1 = bitReader.readUnsigned(2);
            pXSprite->respawnPending = bitReader.readUnsigned(2);
            pXSprite->unused2 = bitReader.readUnsigned(1);
            pXSprite->lT = bitReader.readUnsigned(1);
            pXSprite->dropMsg = bitReader.readUnsigned(8);
            pXSprite->Decoupled = bitReader.readUnsigned(1);
            pXSprite->triggerOnce = bitReader.readUnsigned(1);
            pXSprite->isTriggered = bitReader.readUnsigned(1);
            pXSprite->key = bitReader.readUnsigned(3);
            pXSprite->Push = bitReader.readUnsigned(1);
            pXSprite->Vector = bitReader.readUnsigned(1);
            pXSprite->Impact = bitReader.readUnsigned(1);
            pXSprite->Pickup = bitReader.readUnsigned(1);
            pXSprite->Touch = bitReader.readUnsigned(1);
            pXSprite->Sight = bitReader.readUnsigned(1);
            pXSprite->Proximity = bitReader.readUnsigned(1);
            pXSprite->unused3 = bitReader.readUnsigned(2);
            pXSprite->lSkill = bitReader.readUnsigned(5);
            pXSprite->lS = bitReader.readUnsigned(1);
            pXSprite->lB = bitReader.readUnsigned(1);
            pXSprite->lC = bitReader.readUnsigned(1);
            pXSprite->DudeLockout = bitReader.readUnsigned(1);
            pXSprite->data1 = bitReader.readSigned(16);
            pXSprite->data2 = bitReader.readSigned(16);
            pXSprite->data3 = bitReader.readSigned(16);
            pXSprite->goalAng = bitReader.readUnsigned(11);
            pXSprite->dodgeDir = bitReader.readSigned(2);
            pXSprite->locked = bitReader.readUnsigned(1);
            pXSprite->medium = bitReader.readUnsigned(2);
            pXSprite->respawn = bitReader.readUnsigned(2);
            pXSprite->data4 = bitReader.readUnsigned(16);
            pXSprite->unused4 = bitReader.readUnsigned(6);
            pXSprite->lockMsg = bitReader.readUnsigned(8);
            pXSprite->health = bitReader.readUnsigned(12);
            pXSprite->dudeDeaf = bitReader.readUnsigned(1);
            pXSprite->dudeAmbush = bitReader.readUnsigned(1);
            pXSprite->dudeGuard = bitReader.readUnsigned(1);
            pXSprite->dudeFlag4 = bitReader.readUnsigned(1);
            pXSprite->target = bitReader.readSigned(16);
            pXSprite->targetX = bitReader.readSigned(32);
            pXSprite->targetY = bitReader.readSigned(32);
            pXSprite->targetZ = bitReader.readSigned(32);
            pXSprite->burnTime = bitReader.readUnsigned(16);
            pXSprite->burnSource = bitReader.readSigned(16);
            pXSprite->height = bitReader.readUnsigned(16);
            pXSprite->stateTimer = bitReader.readUnsigned(16);
            pXSprite->aiState = NULL;
            bitReader.skipBits(32);
            xsprite[sprite[i].extra].reference = i;
            xsprite[sprite[i].extra].busy = IntToFixed(xsprite[sprite[i].extra].state);
            if (!encrypted) {
                xsprite[sprite[i].extra].lT |= xsprite[sprite[i].extra].lB;
            }

            #ifdef NOONE_EXTENSIONS
            // indicate if the map requires modern features to work properly
            // for maps wich created in different editors (include vanilla MAPEDIT) or in PMAPEDIT version below than BETA13
            if (!gModernMap && pXSprite->rxID == kChannelMapModernize && pXSprite->rxID == pXSprite->txID && pXSprite->command == kCmdModernFeaturesEnable)
                gModernMap = true;
            #endif
        }
        if ((sprite[i].cstat & 0x30) == 0x30)
        {
            sprite[i].cstat &= ~0x30;
        }
    }
    unsigned int nCRC =  fr.ReadUInt32();

    fr.Seek(0, FileReader::SeekSet);
    auto buffer = fr.Read();
    uint8_t md4[16];
    md4once(buffer.Data(), buffer.Size(), md4);
    G_LoadMapHack(mapname, md4);

    if (CalcCRC32(buffer.Data(), buffer.Size() -4) != nCRC)
    {
        I_Error("%s: Map File does not match CRC", mapname.GetChars());
    }
    if (pCRC)
        *pCRC = nCRC;
    PropagateMarkerReferences();
    if (encrypted)
    {
        if (gMattId == 0x7474614d || gMattId == 0x4d617474)
        {
            drawtile2048 = 1;
        }
        else if (!gMattId)
        {
            drawtile2048 = 0;
        }
        else
        {
            I_Error("%s: Corrupted Map file", mapname.GetChars());
        }
    }
    else if (gMattId != 0)
    {
        I_Error("%s: Corrupted Map file", mapname.GetChars());
    }

    if ((header.version & 0xff00) == 0x600)
    {
        switch (header.version&0xff)
        {
        case 0:
            for (int i = 0; i < numsectors; i++)
            {
                sectortype *pSector = &sector[i];
                if (pSector->extra > 0)
                {
                    XSECTOR *pXSector = &xsector[pSector->extra];
                    pXSector->busyTimeB = pXSector->busyTimeA;
                    if (pXSector->busyTimeA > 0)
                    {
                        if (!pXSector->restState)
                        {
                            pXSector->reTriggerA = 1;
                        }
                        else
                        {
                            pXSector->waitTimeB = pXSector->busyTimeA;
                            pXSector->waitTimeA = 0;
                            pXSector->reTriggerB = 1;
                        }
                    }
                }
            }
            fallthrough__;
        case 1:
            for (int i = 0; i < numsectors; i++)
            {
                sectortype *pSector = &sector[i];
                if (pSector->extra > 0)
                {
                    XSECTOR *pXSector = &xsector[pSector->extra];
                    pXSector->freq >>= 1;
                }
            }
            fallthrough__;
        case 2:
            for (int i = 0; i < kMaxSprites; i++)
            {
            }
            break;
            
        }
    }

    setWallSectors();
    hw_BuildSections();
    sectorGeometry.SetSize(numsections);
    memcpy(wallbackup, wall, sizeof(wallbackup));
    memcpy(sectorbackup, sector, sizeof(sectorbackup));
}


END_BLD_NS

// only used by the backup loader.
void qloadboard(const char* filename, char flags, vec3_t* dapos, int16_t* daang, int16_t* dacursectnum)
{
    Blood::dbLoadMap(filename, &dapos->x, &dapos->y, &dapos->z, (short*)daang, (short*)dacursectnum, NULL);
    Blood::dbInit();    // clean up immediately.
}
