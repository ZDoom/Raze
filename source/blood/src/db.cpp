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
#include "m_crc32.h"
#include "md4.h"

//#include "actor.h"
#include "globals.h"
#include "db.h"
#include "iob.h"
#include "eventq.h"
#include "nnexts.h"

BEGIN_BLD_NS

bool gModernMap = false;
unsigned short gStatCount[kMaxStatus + 1];

XSPRITE xsprite[kMaxXSprites];
XSECTOR xsector[kMaxXSectors];
XWALL xwall[kMaxXWalls];

SPRITEHIT gSpriteHit[kMaxXSprites];

int xvel[kMaxSprites], yvel[kMaxSprites], zvel[kMaxSprites];

#ifdef POLYMER
PolymerLight_t gPolymerLight[kMaxSprites];
#endif

char qsprite_filler[kMaxSprites], qsector_filler[kMaxSectors];

int gVisibility;

void dbCrypt(char *pPtr, int nLength, int nKey)
{
    for (int i = 0; i < nLength; i++)
    {
        pPtr[i] = pPtr[i] ^ nKey;
        nKey++;
    }
}

#ifdef POLYMER

void DeleteLight(int32_t s)
{
    if (gPolymerLight[s].lightId >= 0)
        polymer_deletelight(gPolymerLight[s].lightId);
    gPolymerLight[s].lightId = -1;
    gPolymerLight[s].lightptr = NULL;
}


void G_Polymer_UnInit(void)
{
    int32_t i;

    for (i = 0; i < kMaxSprites; i++)
        DeleteLight(i);
}
#endif


void InsertSpriteSect(int nSprite, int nSector)
{
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    dassert(nSector >= 0 && nSector < kMaxSectors);
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
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    int nSector = sprite[nSprite].sectnum;
    dassert(nSector >= 0 && nSector < kMaxSectors);
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
    sprite[nSprite].sectnum = -1;
}

void InsertSpriteStat(int nSprite, int nStat)
{
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    dassert(nStat >= 0 && nStat <= kMaxStatus);
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
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    int nStat = sprite[nSprite].statnum;
    dassert(nStat >= 0 && nStat <= kMaxStatus);
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
    sprite[nSprite].statnum = kStatNothing;
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
    int const nMaxSprites = bVanilla ? 4096 : kMaxSprites;
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
    dassert(nSprite < kMaxSprites);
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

#ifdef POLYMER
    gPolymerLight[nSprite].lightId = -1;
#endif

    Numsprites++;

    return nSprite;
}

int qinsertsprite(short nSector, short nStat) // Replace
{
    return InsertSprite(nSector, nStat);
}

int DeleteSprite(int nSprite)
{
#ifdef POLYMER
    if (gPolymerLight[nSprite].lightptr != NULL && videoGetRenderMode() == REND_POLYMER)
        DeleteLight(nSprite);
#endif
    if (sprite[nSprite].extra > 0)
    {
        dbDeleteXSprite(sprite[nSprite].extra);
    }
    dassert(sprite[nSprite].statnum >= 0 && sprite[nSprite].statnum < kMaxStatus);
    RemoveSpriteStat(nSprite);
    dassert(sprite[nSprite].sectnum >= 0 && sprite[nSprite].sectnum < kMaxSectors);
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
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    dassert(nSector >= 0 && nSector < kMaxSectors);
    dassert(sprite[nSprite].sectnum >= 0 && sprite[nSprite].sectnum < kMaxSectors);
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
    dassert(nSprite >= 0 && nSprite < kMaxSprites);
    dassert(nStatus >= 0 && nStatus < kMaxStatus);
    dassert(sprite[nSprite].statnum >= 0 && sprite[nSprite].statnum < kMaxStatus);
    dassert(sprite[nSprite].sectnum >= 0 && sprite[nSprite].sectnum < kMaxSectors);
    RemoveSpriteStat(nSprite);
    InsertSpriteStat(nSprite, nStatus);
    return 0;
}

int qchangespritestat(short nSprite, short nStatus)
{
    return ChangeSpriteStat(nSprite, nStatus);
}

unsigned short nextXSprite[kMaxXSprites];
unsigned short nextXWall[kMaxXWalls];
unsigned short nextXSector[kMaxXSectors];

void InitFreeList(unsigned short *pList, int nCount)
{
    for (int i = 1; i < nCount; i++)
    {
        pList[i] = i-1;
    }
    pList[0] = nCount - 1;
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
        ThrowError("Out of free XSprites");
    }
    memset(&xsprite[nXSprite], 0, sizeof(XSPRITE));
    if (!bVanilla)
        memset(&gSpriteHit[nXSprite], 0, sizeof(SPRITEHIT));
    xsprite[nXSprite].reference = nSprite;
    sprite[nSprite].extra = nXSprite;
    return nXSprite;
}

void dbDeleteXSprite(int nXSprite)
{
    dassert(xsprite[nXSprite].reference >= 0);
    dassert(sprite[xsprite[nXSprite].reference].extra == nXSprite);
    InsertFree(nextXSprite, nXSprite);
    sprite[xsprite[nXSprite].reference].extra = -1;
    xsprite[nXSprite].reference = -1;
}

unsigned short dbInsertXWall(int nWall)
{
    int nXWall = nextXWall[0];
    nextXWall[0] = nextXWall[nXWall];
    if (nXWall == 0)
    {
        ThrowError("Out of free XWalls");
    }
    memset(&xwall[nXWall], 0, sizeof(XWALL));
    xwall[nXWall].reference = nWall;
    wall[nWall].extra = nXWall;
    return nXWall;
}

void dbDeleteXWall(int nXWall)
{
    dassert(xwall[nXWall].reference >= 0);
    InsertFree(nextXWall, nXWall);
    wall[xwall[nXWall].reference].extra = -1;
    xwall[nXWall].reference = -1;
}

unsigned short dbInsertXSector(int nSector)
{
    int nXSector = nextXSector[0];
    nextXSector[0] = nextXSector[nXSector];
    if (nXSector == 0)
    {
        ThrowError("Out of free XSectors");
    }
    memset(&xsector[nXSector], 0, sizeof(XSECTOR));
    xsector[nXSector].reference = nSector;
    sector[nSector].extra = nXSector;
    return nXSector;
}

void dbDeleteXSector(int nXSector)
{
    dassert(xsector[nXSector].reference >= 0);
    InsertFree(nextXSector, nXSector);
    sector[xsector[nXSector].reference].extra = -1;
    xsector[nXSector].reference = -1;
}

void dbXSpriteClean(void)
{
    for (int i = 0; i < kMaxSprites; i++)
    {
        int nXSprite = sprite[i].extra;
        if (nXSprite == 0)
        {
            sprite[i].extra = -1;
        }
        if (sprite[i].statnum < kMaxStatus && nXSprite > 0)
        {
            dassert(nXSprite < kMaxXSprites);
            if (xsprite[nXSprite].reference != i)
            {
                int nXSprite2 = dbInsertXSprite(i);
                memcpy(&xsprite[nXSprite2], &xsprite[nXSprite], sizeof(XSPRITE));
                xsprite[nXSprite2].reference = i;
            }
        }
    }
    for (int i = 1; i < kMaxXSprites; i++)
    {
        int nSprite = xsprite[i].reference;
        if (nSprite >= 0)
        {
            dassert(nSprite < kMaxSprites);
            if (sprite[nSprite].statnum >= kMaxStatus || sprite[nSprite].extra != i)
            {
                InsertFree(nextXSprite, i);
                xsprite[i].reference = -1;
            }
        }
    }
}

void dbXWallClean(void)
{
    for (int i = 0; i < numwalls; i++)
    {
        int nXWall = wall[i].extra;
        if (nXWall == 0)
        {
            wall[i].extra = -1;
        }
        if (nXWall > 0)
        {
            dassert(nXWall < kMaxXWalls);
            if (xwall[nXWall].reference == -1)
            {
                wall[i].extra = -1;
            }
            else
            {
                xwall[nXWall].reference = i;
            }
        }
    }
    for (int i = 0; i < numwalls; i++)
    {
        int nXWall = wall[i].extra;
        if (nXWall > 0)
        {
            dassert(nXWall < kMaxXWalls);
            if (xwall[nXWall].reference != i)
            {
                int nXWall2 = dbInsertXWall(i);
                memcpy(&xwall[nXWall2], &xwall[nXWall], sizeof(XWALL));
                xwall[nXWall2].reference = i;
            }
        }
    }
    for (int i = 1; i < kMaxXWalls; i++)
    {
        int nWall = xwall[i].reference;
        if (nWall >= 0)
        {
            dassert(nWall < kMaxWalls);
            if (nWall >= numwalls || wall[nWall].extra != i)
            {
                InsertFree(nextXWall, i);
                xwall[i].reference = -1;
            }
        }
    }
}

void dbXSectorClean(void)
{


    for (int i = 0; i < numsectors; i++)
    {
        int nXSector = sector[i].extra;
        if (nXSector == 0)
        {
            sector[i].extra = -1;
        }
        if (nXSector > 0)
        {
            dassert(nXSector < kMaxXSectors);
            if (xsector[nXSector].reference == -1)
            {
                sector[i].extra = -1;
            }
            else
            {
                xsector[nXSector].reference = i;
            }
        }
    }
    for (int i = 0; i < numsectors; i++)
    {
        int nXSector = sector[i].extra;
        if (nXSector > 0)
        {
            dassert(nXSector < kMaxXSectors);
            if (xsector[nXSector].reference != i)
            {
                int nXSector2 = dbInsertXSector(i);
                memcpy(&xsector[nXSector2], &xsector[nXSector], sizeof(XSECTOR));
                xsector[nXSector2].reference = i;
            }
        }
    }
    for (int i = 1; i < kMaxXSectors; i++)
    {
        int nSector = xsector[i].reference;
        if (nSector >= 0)
        {
            dassert(nSector < kMaxSectors);
            if (nSector >= numsectors || sector[nSector].extra != i)
            {
                InsertFree(nextXSector, i);
                xsector[i].reference = -1;
            }
        }
    }
}

void dbInit(void)
{
    InitFreeList(nextXSprite, kMaxXSprites);
    for (int i = 1; i < kMaxXSprites; i++)
    {
        xsprite[i].reference = -1;
    }
    InitFreeList(nextXWall, kMaxXWalls);
    for (int i = 1; i < kMaxXWalls; i++)
    {
        xwall[i].reference = -1;
    }
    InitFreeList(nextXSector, kMaxXSectors);
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

bool byte_1A76C6, byte_1A76C7, byte_1A76C8;

MAPHEADER2 byte_19AE44;

unsigned int dbReadMapCRC(const char *pPath)
{
    byte_1A76C7 = 0;
    byte_1A76C8 = 0;

	DICTNODE* pNode;
    pNode = gSysRes.Lookup(pPath, "MAP");
    if (!pNode)
    {
        char name2[BMAX_PATH];
        Bstrncpy(name2, pPath, BMAX_PATH);
        ChangeExtension(name2, "");
        pNode = gSysRes.Lookup(name2, "MAP");
    }

    if (!pNode)
    {
        Printf("Error opening map file %s", pPath);
        return -1;
    }
    char *pData = (char*)gSysRes.Lock(pNode);

    int nSize = pNode->Size();
    MAPSIGNATURE header;
    IOBuffer(nSize, pData).Read(&header, 6);
#if B_BIG_ENDIAN == 1
    header.version = B_LITTLE16(header.version);
#endif
    if (memcmp(header.signature, "BLM\x1a", 4))
    {
        ThrowError("Map file corrupted");
    }
    if ((header.version & 0xff00) == 0x600)
    {
    }
    else if ((header.version & 0xff00) == 0x700)
    {
        byte_1A76C8 = 1;
    }
    else
    {
        ThrowError("Map file is wrong version");
    }
    unsigned int nCRC = *(unsigned int*)(pData+nSize-4);
    gSysRes.Unlock(pNode);
    return nCRC;
}

int gMapRev, gSongId, gSkyCount;
//char byte_19AE44[128];
const int nXSectorSize = 60;
const int nXSpriteSize = 56;
const int nXWallSize = 24;

int dbLoadMap(const char *pPath, int *pX, int *pY, int *pZ, short *pAngle, short *pSector, unsigned int *pCRC) {
    int16_t tpskyoff[256];
    show2dsector.Zero();
    memset(show2dwall, 0, sizeof(show2dwall));
    memset(show2dsprite, 0, sizeof(show2dsprite));
    #ifdef NOONE_EXTENSIONS
    gModernMap = false;
    #endif

#ifdef USE_OPENGL
    Polymost_prepare_loadboard();
#endif

	DICTNODE* pNode;

    pNode = gSysRes.Lookup(pPath, "MAP");
    if (!pNode)
    {
        char name2[BMAX_PATH];
        Bstrncpy(name2, pPath, BMAX_PATH);
        ChangeExtension(name2, "");
        pNode = gSysRes.Lookup(name2, "MAP");
    }

    if (!pNode)
    {
        Printf("Error opening map file %s", pPath);
        return -1;
    }
    char *pData = (char*)gSysRes.Lock(pNode);
    int nSize = pNode->Size();
    MAPSIGNATURE header;
    IOBuffer IOBuffer1 = IOBuffer(nSize, pData);
    IOBuffer1.Read(&header, 6);
#if B_BIG_ENDIAN == 1
    header.version = B_LITTLE16(header.version);
#endif
    if (memcmp(header.signature, "BLM\x1a", 4))
    {
        Printf("Map file corrupted");
        gSysRes.Unlock(pNode);
        return -1;
    }
    byte_1A76C8 = 0;
    if ((header.version & 0xff00) == 0x700) {
        byte_1A76C8 = 1;
        
        #ifdef NOONE_EXTENSIONS
        // indicate if the map requires modern features to work properly
        // for maps wich created in PMAPEDIT BETA13 or higher versions. Since only minor version changed,
        // the map is still can be loaded with vanilla BLOOD / MAPEDIT and should work in other ports too.
        if ((header.version & 0x00ff) == 0x001) gModernMap = true;
        #endif

    } else {
        Printf("Map file is wrong version");
        gSysRes.Unlock(pNode);
        return -1;
    }

    MAPHEADER mapHeader;
    IOBuffer1.Read(&mapHeader,37/* sizeof(mapHeader)*/);
    if (mapHeader.at16 != 0 && mapHeader.at16 != 0x7474614d && mapHeader.at16 != 0x4d617474) {
        dbCrypt((char*)&mapHeader, sizeof(mapHeader), 0x7474614d);
        byte_1A76C7 = 1;
    }

#if B_BIG_ENDIAN == 1
    mapHeader.at0 = B_LITTLE32(mapHeader.at0);
    mapHeader.at4 = B_LITTLE32(mapHeader.at4);
    mapHeader.at8 = B_LITTLE32(mapHeader.at8);
    mapHeader.atc = B_LITTLE16(mapHeader.atc);
    mapHeader.ate = B_LITTLE16(mapHeader.ate);
    mapHeader.at10 = B_LITTLE16(mapHeader.at10);
    mapHeader.at12 = B_LITTLE32(mapHeader.at12);
    mapHeader.at16 = B_LITTLE32(mapHeader.at16);
    mapHeader.at1b = B_LITTLE32(mapHeader.at1b);
    mapHeader.at1f = B_LITTLE16(mapHeader.at1f);
    mapHeader.at21 = B_LITTLE16(mapHeader.at21);
    mapHeader.at23 = B_LITTLE16(mapHeader.at23);
#endif

    psky_t *pSky = tileSetupSky(0);
    pSky->horizfrac = 65536;

    *pX = mapHeader.at0;
    *pY = mapHeader.at4;
    *pZ = mapHeader.at8;
    *pAngle = mapHeader.atc;
    *pSector = mapHeader.ate;
    pSky->lognumtiles = mapHeader.at10;
    gVisibility = g_visibility = mapHeader.at12;
    gSongId = mapHeader.at16;
    if (byte_1A76C8)
    {
        if (mapHeader.at16 == 0x7474614d || mapHeader.at16 == 0x4d617474)
        {
            byte_1A76C6 = 1;
        }
        else if (!mapHeader.at16)
        {
            byte_1A76C6 = 0;
        }
        else
        {
            Printf("Corrupted Map file");
            gSysRes.Unlock(pNode);
            return -1;
        }
    }
    else if (mapHeader.at16)
    {
        Printf("Corrupted Map file");
        gSysRes.Unlock(pNode);
        return -1;
    }
    parallaxtype = mapHeader.at1a;
    gMapRev = mapHeader.at1b;
    numsectors = mapHeader.at1f;
    numwalls = mapHeader.at21;
    dbInit();
    if (byte_1A76C8)
    {
        IOBuffer1.Read(&byte_19AE44, 128);
        dbCrypt((char*)&byte_19AE44, 128, numwalls);
#if B_BIG_ENDIAN == 1
        byte_19AE44.at40 = B_LITTLE32(byte_19AE44.at40);
        byte_19AE44.at44 = B_LITTLE32(byte_19AE44.at44);
        byte_19AE44.at48 = B_LITTLE32(byte_19AE44.at48);
#endif
    }
    else
    {
        memset(&byte_19AE44, 0, 128);
    }
    gSkyCount = 1<<pSky->lognumtiles;
    IOBuffer1.Read(tpskyoff, gSkyCount*sizeof(tpskyoff[0]));
    if (byte_1A76C8)
    {
        dbCrypt((char*)tpskyoff, gSkyCount*sizeof(tpskyoff[0]), gSkyCount*2);
    }
    for (int i = 0; i < ClipHigh(gSkyCount, MAXPSKYTILES); i++)
    {
        pSky->tileofs[i] = B_LITTLE16(tpskyoff[i]);
    }
    for (int i = 0; i < numsectors; i++)
    {
        sectortype *pSector = &sector[i];
        IOBuffer1.Read(pSector, sizeof(sectortype));
        if (byte_1A76C8)
        {
            dbCrypt((char*)pSector, sizeof(sectortype), gMapRev*sizeof(sectortype));
        }
#if B_BIG_ENDIAN == 1
        pSector->wallptr = B_LITTLE16(pSector->wallptr);
        pSector->wallnum = B_LITTLE16(pSector->wallnum);
        pSector->ceilingz = B_LITTLE32(pSector->ceilingz);
        pSector->floorz = B_LITTLE32(pSector->floorz);
        pSector->ceilingstat = B_LITTLE16(pSector->ceilingstat);
        pSector->floorstat = B_LITTLE16(pSector->floorstat);
        pSector->ceilingpicnum = B_LITTLE16(pSector->ceilingpicnum);
        pSector->ceilingheinum = B_LITTLE16(pSector->ceilingheinum);
        pSector->floorpicnum = B_LITTLE16(pSector->floorpicnum);
        pSector->floorheinum = B_LITTLE16(pSector->floorheinum);
        pSector->type = B_LITTLE16(pSector->type);
        pSector->hitag = B_LITTLE16(pSector->hitag);
        pSector->extra = B_LITTLE16(pSector->extra);
#endif
        qsector_filler[i] = pSector->fogpal;
        pSector->fogpal = 0;
        if (sector[i].extra > 0)
        {
            char pBuffer[nXSectorSize];
            int nXSector = dbInsertXSector(i);
            XSECTOR *pXSector = &xsector[nXSector];
            memset(pXSector, 0, sizeof(XSECTOR));
            int nCount;
            if (!byte_1A76C8)
            {
                nCount = nXSectorSize;
            }
            else
            {
                nCount = byte_19AE44.at48;
            }
            dassert(nCount <= nXSectorSize);
            IOBuffer1.Read(pBuffer, nCount);
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
            pXSector->unused2 = bitReader.readUnsigned(1);
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
            pXSector->ceilXPanFrac = bitReader.readUnsigned(8);
            pXSector->ceilYPanFrac = bitReader.readUnsigned(8);
            pXSector->floorXPanFrac = bitReader.readUnsigned(8);
            pXSector->damageType = bitReader.readUnsigned(3);
            pXSector->floorpal = bitReader.readUnsigned(4);
            pXSector->floorYPanFrac = bitReader.readUnsigned(8);
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
            xsector[sector[i].extra].busy = xsector[sector[i].extra].state<<16;

        }
    }
    for (int i = 0; i < numwalls; i++)
    {
        walltype *pWall = &wall[i];
        IOBuffer1.Read(pWall, sizeof(walltype));
        if (byte_1A76C8)
        {
            dbCrypt((char*)pWall, sizeof(walltype), (gMapRev*sizeof(sectortype)) | 0x7474614d);
        }
#if B_BIG_ENDIAN == 1
        pWall->x = B_LITTLE32(pWall->x);
        pWall->y = B_LITTLE32(pWall->y);
        pWall->point2 = B_LITTLE16(pWall->point2);
        pWall->nextwall = B_LITTLE16(pWall->nextwall);
        pWall->nextsector = B_LITTLE16(pWall->nextsector);
        pWall->cstat = B_LITTLE16(pWall->cstat);
        pWall->picnum = B_LITTLE16(pWall->picnum);
        pWall->overpicnum = B_LITTLE16(pWall->overpicnum);
        pWall->type = B_LITTLE16(pWall->type);
        pWall->hitag = B_LITTLE16(pWall->hitag);
        pWall->extra = B_LITTLE16(pWall->extra);
#endif
        if (wall[i].extra > 0)
        {
            char pBuffer[nXWallSize];
            int nXWall = dbInsertXWall(i);
            XWALL *pXWall = &xwall[nXWall];
            memset(pXWall, 0, sizeof(XWALL));
            int nCount;
            if (!byte_1A76C8)
            {
                nCount = nXWallSize;
            }
            else
            {
                nCount = byte_19AE44.at44;
            }
            dassert(nCount <= nXWallSize);
            IOBuffer1.Read(pBuffer, nCount);
            BitReader bitReader(pBuffer, nCount);
            pXWall->reference = bitReader.readSigned(14);
            pXWall->state = bitReader.readUnsigned(1);
            pXWall->busy = bitReader.readUnsigned(17);
            pXWall->data = bitReader.readSigned(16);
            pXWall->txID = bitReader.readUnsigned(10);
            pXWall->unused1 = bitReader.readUnsigned(6);
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
            pXWall->unused2 = bitReader.readUnsigned(2);
            pXWall->xpanFrac = bitReader.readUnsigned(8);
            pXWall->ypanFrac = bitReader.readUnsigned(8);
            pXWall->locked = bitReader.readUnsigned(1);
            pXWall->dudeLockout = bitReader.readUnsigned(1);
            pXWall->unused3 = bitReader.readUnsigned(4);
            pXWall->unused4 = bitReader.readUnsigned(32);
            xwall[wall[i].extra].reference = i;
            xwall[wall[i].extra].busy = xwall[wall[i].extra].state << 16;

        }
    }
    initspritelists();
    for (int i = 0; i < mapHeader.at23; i++)
    {
        RemoveSpriteStat(i);
        spritetype *pSprite = &sprite[i];
        IOBuffer1.Read(pSprite, sizeof(spritetype));
        if (byte_1A76C8)
        {
            dbCrypt((char*)pSprite, sizeof(spritetype), (gMapRev*sizeof(spritetype)) | 0x7474614d);
        }
#if B_BIG_ENDIAN == 1
        pSprite->x = B_LITTLE32(pSprite->x);
        pSprite->y = B_LITTLE32(pSprite->y);
        pSprite->z = B_LITTLE32(pSprite->z);
        pSprite->cstat = B_LITTLE16(pSprite->cstat);
        pSprite->picnum = B_LITTLE16(pSprite->picnum);
        pSprite->sectnum = B_LITTLE16(pSprite->sectnum);
        pSprite->statnum = B_LITTLE16(pSprite->statnum);
        pSprite->ang = B_LITTLE16(pSprite->ang);
        pSprite->owner = B_LITTLE16(pSprite->owner);
        pSprite->index = B_LITTLE16(pSprite->index);
        pSprite->yvel = B_LITTLE16(pSprite->yvel);
        pSprite->inittype = B_LITTLE16(pSprite->inittype);
        pSprite->type = B_LITTLE16(pSprite->type);
        pSprite->flags = B_LITTLE16(pSprite->hitag);
        pSprite->extra = B_LITTLE16(pSprite->extra);
#endif
        InsertSpriteSect(i, sprite[i].sectnum);
        InsertSpriteStat(i, sprite[i].statnum);
        Numsprites++;
        sprite[i].index = i;
        qsprite_filler[i] = pSprite->blend;
        pSprite->blend = 0;
        if (sprite[i].extra > 0)
        {
            char pBuffer[nXSpriteSize];
            int nXSprite = dbInsertXSprite(i);
            XSPRITE *pXSprite = &xsprite[nXSprite];
            memset(pXSprite, 0, sizeof(XSPRITE));
            int nCount;
            if (!byte_1A76C8)
            {
                nCount = nXSpriteSize;
            }
            else
            {
                nCount = byte_19AE44.at40;
            }
            dassert(nCount <= nXSpriteSize);
            IOBuffer1.Read(pBuffer, nCount);
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
            xsprite[sprite[i].extra].busy = xsprite[sprite[i].extra].state << 16;
            if (!byte_1A76C8) {
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
    unsigned int nCRC;
    IOBuffer1.Read(&nCRC, 4);
#if B_BIG_ENDIAN == 1
    nCRC = B_LITTLE32(nCRC);
#endif
    md4once((unsigned char*)pData, nSize, g_loadedMapHack.md4);
    if (Bcrc32(pData, nSize-4, 0) != nCRC)
    {
        Printf("Map File does not match CRC");
        gSysRes.Unlock(pNode);
        return -1;
    }
    if (pCRC)
        *pCRC = nCRC;
    gSysRes.Unlock(pNode);
    PropagateMarkerReferences();
    if (byte_1A76C8)
    {
        if (gSongId == 0x7474614d || gSongId == 0x4d617474)
        {
            byte_1A76C6 = 1;
        }
        else if (!gSongId)
        {
            byte_1A76C6 = 0;
        }
        else
        {
            Printf("Corrupted Map file");
            gSysRes.Unlock(pNode);
            return -1;
        }
    }
    else if (gSongId != 0)
    {
        Printf("Corrupted Map file");
        gSysRes.Unlock(pNode);
        return -1;
    }

#ifdef POLYMER
    if (videoGetRenderMode() == REND_POLYMER)
        polymer_loadboard();
#endif

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

    g_loadedMapVersion = 7;

    return 0;
}

int32_t qloadboard(const char* filename, char flags, vec3_t* dapos, int16_t* daang, int16_t* dacursectnum)
{
    // NUKE-TODO: implement flags, see mapedit.cpp
    return dbLoadMap(filename, &dapos->x, &dapos->y, &dapos->z, (short*)daang, (short*)dacursectnum, NULL);
}


END_BLD_NS
