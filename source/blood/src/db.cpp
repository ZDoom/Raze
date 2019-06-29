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
#include "build.h"
#ifdef POLYMER
#include "polymer.h"
#endif
#include "compat.h"
#include "common_game.h"
#include "crc32.h"

//#include "actor.h"
#include "globals.h"
#include "db.h"
#include "iob.h"

unsigned short gStatCount[kMaxStatus + 1];

XSPRITE xsprite[kMaxXSprites];
XSECTOR xsector[kMaxXSectors];
XWALL xwall[kMaxXWalls];

SPRITEHIT gSpriteHit[kMaxXSprites];

int xvel[kMaxSprites], yvel[kMaxSprites], zvel[kMaxSprites];

char qsprite_filler[kMaxSprites], qsector_filler[kMaxSectors];

int gVisibility;

const char *gItemText[] = {
    "Skull Key",
    "Eye Key",
    "Fire Key",
    "Dagger Key",
    "Spider Key",
    "Moon Key",
    "Key 7",
    "Doctor's Bag",
    "Medicine Pouch",
    "Life Essence",
    "Life Seed",
    "Red Potion",
    "Feather Fall",
    "Limited Invisibility",
    "INVULNERABILITY",
    "Boots of Jumping",
    "Raven Flight",
    "Guns Akimbo",
    "Diving Suit",
    "Gas mask",
    "Clone",
    "Crystal Ball",
    "Decoy",
    "Doppleganger",
    "Reflective shots",
    "Beast Vision",
    "ShadowCloak",
    "Rage shroom",
    "Delirium Shroom",
    "Grow shroom",
    "Shrink shroom",
    "Death mask",
    "Wine Goblet",
    "Wine Bottle",
    "Skull Grail",
    "Silver Grail",
    "Tome",
    "Black Chest",
    "Wooden Chest",
    "Asbestos Armor",
    "Basic Armor",
    "Body Armor",
    "Fire Armor",
    "Spirit Armor",
    "Super Armor",
    "Blue Team Base",
    "Red Team Base",
    "Blue Flag",
    "Red Flag",
    "DUMMY",
    "Level map",
};

const char *gAmmoText[] = {
    "Spray can",
    "Bundle of TNT*",
    "Bundle of TNT",
    "Case of TNT",
    "Proximity Detonator",
    "Remote Detonator",
    "Trapped Soul",
    "4 shotgun shells",
    "Box of shotgun shells",
    "A few bullets",
    "Voodoo Doll",
    "OBSOLETE",
    "Full drum of bullets",
    "Tesla Charge",
    "OBSOLETE",
    "OBSOLETE",
    "Flares",
    "OBSOLETE",
    "OBSOLETE",
    "Gasoline Can",
    NULL,
};

const char *gWeaponText[] = {
    "RANDOM",
    "Sawed-off",
    "Tommy Gun",
    "Flare Pistol",
    "Voodoo Doll",
    "Tesla Cannon",
    "Napalm Launcher",
    "Pitchfork",
    "Spray Can",
    "Dynamite",
    "Life Leech",
};



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
    sprite[nSprite].statnum = -1;
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

    Numsprites++;

    return nSprite;
}

int qinsertsprite(short nSector, short nStat) // Replace
{
    return InsertSprite(nSector, nStat);
}

int DeleteSprite(int nSprite)
{
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
    for (nSprite = headspritestat[10]; nSprite != -1; nSprite = nNextSprite)
    {
        nNextSprite = nextspritestat[nSprite];
        switch (sprite[nSprite].type)
        {
        case 8:
        {
            int nOwner = sprite[nSprite].owner;
            if (nOwner >= 0 && nOwner < numsectors)
            {
                int nXSector = sector[nOwner].extra;
                if (nXSector > 0 && nXSector < kMaxXSectors)
                {
                    xsector[nXSector].at2c_0 = nSprite;
                    continue;
                }
            }
            break;
        }
        case 3:
        {
            int nOwner = sprite[nSprite].owner;
            if (nOwner >= 0 && nOwner < numsectors)
            {
                int nXSector = sector[nOwner].extra;
                if (nXSector > 0 && nXSector < kMaxXSectors)
                {
                    xsector[nXSector].at2c_0 = nSprite;
                    continue;
                }
            }
            break;
        }
        case 4:
        {
            int nOwner = sprite[nSprite].owner;
            if (nOwner >= 0 && nOwner < numsectors)
            {
                int nXSector = sector[nOwner].extra;
                if (nXSector > 0 && nXSector < kMaxXSectors)
                {
                    xsector[nXSector].at2e_0 = nSprite;
                    continue;
                }
            }
            break;
        }
        case 5:
        {
            int nOwner = sprite[nSprite].owner;
            if (nOwner >= 0 && nOwner < numsectors)
            {
                int nXSector = sector[nOwner].extra;
                if (nXSector > 0 && nXSector < kMaxXSectors)
                {
                    xsector[nXSector].at2c_0 = nSprite;
                    continue;
                }
            }
            break;
        }
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
    DICTNODE *pNode = gSysRes.Lookup(pPath, "MAP");
    if (!pNode)
    {
        ThrowError("Error opening map file %s", pPath);
    }
    char *pData = (char*)gSysRes.Lock(pNode);
    int nSize = pNode->size;
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

int dbLoadMap(const char *pPath, int *pX, int *pY, int *pZ, short *pAngle, short *pSector, unsigned int *pCRC)
{
    int16_t tpskyoff[256];
    memset(show2dsector, 0, sizeof(show2dsector));
    memset(show2dwall, 0, sizeof(show2dwall));
    memset(show2dsprite, 0, sizeof(show2dsprite));
#ifdef USE_OPENGL
    Polymost_prepare_loadboard();
#endif
    {
        char name2[BMAX_PATH];
        Bstrncpy(name2, pPath, BMAX_PATH);
        Bstrupr(name2);
        DICTNODE* pNode = *gSysRes.Probe(name2, "MAP");
        if (pNode && pNode->flags & DICT_EXTERNAL)
        {
            gSysRes.RemoveNode(pNode);
        }
    }
    DICTNODE *pNode = gSysRes.Lookup(pPath, "MAP");
    if (!pNode)
    {
        char name2[BMAX_PATH];
        Bstrncpy(name2, pPath, BMAX_PATH);
        ChangeExtension(name2, "");
        pNode = gSysRes.Lookup(name2, "MAP");
    }
    if (!pNode)
    {
        initprintf("Error opening map file %s", pPath);
        return -1;
    }
    char *pData = (char*)gSysRes.Lock(pNode);
    int nSize = pNode->size;
    MAPSIGNATURE header;
    IOBuffer IOBuffer1 = IOBuffer(nSize, pData);
    IOBuffer1.Read(&header, 6);
#if B_BIG_ENDIAN == 1
    header.version = B_LITTLE16(header.version);
#endif
    if (memcmp(header.signature, "BLM\x1a", 4))
    {
        initprintf("Map file corrupted");
        gSysRes.Unlock(pNode);
        return -1;
    }
    byte_1A76C8 = 0;
    if ((header.version & 0xff00) == 0x600)
    {
    }
    else if ((header.version & 0xff00) == 0x700)
    {
        byte_1A76C8 = 1;
    }
    else
    {
        initprintf("Map file is wrong version");
        gSysRes.Unlock(pNode);
        return -1;
    }
    MAPHEADER mapHeader;
    IOBuffer1.Read(&mapHeader,37/* sizeof(mapHeader)*/);
    if (mapHeader.at16 != 0 && mapHeader.at16 != 0x7474614d && mapHeader.at16 != 0x4d617474)
    {
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
            initprintf("Corrupted Map file");
            gSysRes.Unlock(pNode);
            return -1;
        }
    }
    else if (mapHeader.at16)
    {
        initprintf("Corrupted Map file");
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
        pSector->lotag = B_LITTLE16(pSector->lotag);
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
            pXSector->at7_2 = bitReader.readUnsigned(3);
            pXSector->at7_5 = bitReader.readUnsigned(3);
            pXSector->rxID = bitReader.readUnsigned(10);
            pXSector->command = bitReader.readUnsigned(8);
            pXSector->triggerOn = bitReader.readUnsigned(1);
            pXSector->triggerOff = bitReader.readUnsigned(1);
            pXSector->busyTimeA = bitReader.readUnsigned(12);
            pXSector->waitTimeA = bitReader.readUnsigned(12);
            pXSector->atd_4 = bitReader.readUnsigned(1);
            pXSector->interruptable = bitReader.readUnsigned(1);
            pXSector->amplitude = bitReader.readSigned(8);
            pXSector->freq = bitReader.readUnsigned(8);
            pXSector->atf_6 = bitReader.readUnsigned(1);
            pXSector->atf_7 = bitReader.readUnsigned(1);
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
            pXSector->at16_3 = bitReader.readUnsigned(1);
            pXSector->decoupled = bitReader.readUnsigned(1);
            pXSector->triggerOnce = bitReader.readUnsigned(1);
            pXSector->at16_6 = bitReader.readUnsigned(1);
            pXSector->Key = bitReader.readUnsigned(3);
            pXSector->Push = bitReader.readUnsigned(1);
            pXSector->Vector = bitReader.readUnsigned(1);
            pXSector->Reserved = bitReader.readUnsigned(1);
            pXSector->Enter = bitReader.readUnsigned(1);
            pXSector->Exit = bitReader.readUnsigned(1);
            pXSector->Wallpush = bitReader.readUnsigned(1);
            pXSector->color = bitReader.readUnsigned(1);
            pXSector->at18_1 = bitReader.readUnsigned(1);
            pXSector->busyTimeB = bitReader.readUnsigned(12);
            pXSector->waitTimeB = bitReader.readUnsigned(12);
            pXSector->at1b_2 = bitReader.readUnsigned(1);
            pXSector->at1b_3 = bitReader.readUnsigned(1);
            pXSector->ceilpal = bitReader.readUnsigned(4);
            pXSector->at1c_0 = bitReader.readSigned(32);
            pXSector->at20_0 = bitReader.readSigned(32);
            pXSector->at24_0 = bitReader.readSigned(32);
            pXSector->at28_0 = bitReader.readSigned(32);
            pXSector->at2c_0 = bitReader.readUnsigned(16);
            pXSector->at2e_0 = bitReader.readUnsigned(16);
            pXSector->Crush = bitReader.readUnsigned(1);
            pXSector->at30_1 = bitReader.readUnsigned(8);
            pXSector->at31_1 = bitReader.readUnsigned(8);
            pXSector->at32_1 = bitReader.readUnsigned(8);
            pXSector->damageType = bitReader.readUnsigned(3);
            pXSector->floorpal = bitReader.readUnsigned(4);
            pXSector->at34_0 = bitReader.readUnsigned(8);
            pXSector->locked = bitReader.readUnsigned(1);
            pXSector->windVel = bitReader.readUnsigned(10);
            pXSector->windAng = bitReader.readUnsigned(11);
            pXSector->windAlways = bitReader.readUnsigned(1);
            pXSector->at37_7 = bitReader.readUnsigned(1);
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
        pWall->lotag = B_LITTLE16(pWall->lotag);
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
            pXWall->at7_2 = bitReader.readUnsigned(6);
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
            pXWall->triggerReserved = bitReader.readUnsigned(1);
            pXWall->at11_0 = bitReader.readUnsigned(2);
            pXWall->xpanFrac = bitReader.readUnsigned(8);
            pXWall->ypanFrac = bitReader.readUnsigned(8);
            pXWall->locked = bitReader.readUnsigned(1);
            pXWall->dudeLockout = bitReader.readUnsigned(1);
            pXWall->at13_4 = bitReader.readUnsigned(4);
            pXWall->at14_0 = bitReader.readUnsigned(32);
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
        pSprite->xvel = B_LITTLE16(pSprite->xvel);
        pSprite->yvel = B_LITTLE16(pSprite->yvel);
        pSprite->zvel = B_LITTLE16(pSprite->zvel);
        pSprite->lotag = B_LITTLE16(pSprite->lotag);
        pSprite->hitag = B_LITTLE16(pSprite->hitag);
        pSprite->extra = B_LITTLE16(pSprite->extra);
#endif
        // NoOne's extension bit
        if (pSprite->hitag&1)
        {
            pSprite->hitag &= ~1;
            pSprite->hitag |= kHitagExtBit;
        }
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
            pXSprite->atb_2 = bitReader.readUnsigned(2);
            pXSprite->respawnPending = bitReader.readUnsigned(2);
            pXSprite->atb_6 = bitReader.readUnsigned(1);
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
            pXSprite->ate_5 = bitReader.readUnsigned(2);
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
            pXSprite->at1a_2 = bitReader.readUnsigned(6);
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
            if (!byte_1A76C8)
            {
                xsprite[sprite[i].extra].lT |= xsprite[sprite[i].extra].lB;
            }
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
    if (Bcrc32(pData, nSize-4, 0) != nCRC)
    {
        initprintf("Map File does not match CRC");
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
            initprintf("Corrupted Map file");
            gSysRes.Unlock(pNode);
            return -1;
        }
    }
    else if (gSongId != 0)
    {
        initprintf("Corrupted Map file");
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
                        if (!pXSector->atd_4)
                        {
                            pXSector->atf_6 = 1;
                        }
                        else
                        {
                            pXSector->waitTimeB = pXSector->busyTimeA;
                            pXSector->waitTimeA = 0;
                            pXSector->atf_7 = 1;
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

#ifdef YAX_ENABLE
    yax_update(numyaxbunches > 0 ? 2 : 1);
#endif

    g_loadedMapVersion = 7;

    return 0;
}

int dbSaveMap(const char *pPath, int nX, int nY, int nZ, short nAngle, short nSector)
{
    char sMapExt[_MAX_PATH];
    char sBakExt[_MAX_PATH];
    int16_t tpskyoff[256];
    int nSpriteNum;
    psky_t *pSky = tileSetupSky(0);
    gSkyCount = 1<<pSky->lognumtiles;
    gMapRev++;
    nSpriteNum = 0;
    strcpy(sMapExt, pPath);
    ChangeExtension(sMapExt, ".MAP");
    int nSize = sizeof(MAPSIGNATURE)+sizeof(MAPHEADER);
    if (byte_1A76C8)
    {
        nSize += sizeof(MAPHEADER2);
    }
    for (int i = 0; i < gSkyCount; i++)
        tpskyoff[i] = pSky->tileofs[i];
    nSize += gSkyCount*sizeof(tpskyoff[0]);
    nSize += sizeof(sectortype)*numsectors;
    for (int i = 0; i < numsectors; i++)
    {
        if (sector[i].extra > 0)
        {
            nSize += nXSectorSize;
        }
    }
    nSize += sizeof(walltype)*numwalls;
    for (int i = 0; i < numwalls; i++)
    {
        if (wall[i].extra > 0)
        {
            nSize += nXWallSize;
        }
    }
    for (int i = 0; i < kMaxSprites; i++)
    {
        if (sprite[i].statnum < kMaxStatus)
        {
            nSpriteNum++;
            if (sprite[i].extra > 0)
            {
                nSize += nXSpriteSize;
            }
        }
    }
    nSize += sizeof(spritetype)*nSpriteNum;
    nSize += 4;
    char *pData = (char*)Xmalloc(nSize);
    IOBuffer IOBuffer1 = IOBuffer(nSize, pData);
    MAPSIGNATURE header;
    memcpy(&header, "BLM\x1a", 4);
    if (byte_1A76C8)
    {
        header.version = 0x700;
        byte_1A76C7 = 1;
    }
    else
    {
        header.version = 0x603;
        byte_1A76C7 = 0;
    }
    IOBuffer1.Write(&header, sizeof(header));
    MAPHEADER mapheader;
    mapheader.at0 = B_LITTLE32(nX);
    mapheader.at4 = B_LITTLE32(nY);
    mapheader.at8 = B_LITTLE32(nZ);
    mapheader.atc = B_LITTLE16(nAngle);
    mapheader.ate = B_LITTLE16(nSector);
    mapheader.at10 = B_LITTLE16(pSky->lognumtiles);
    mapheader.at12 = B_LITTLE32(gVisibility);
    if (byte_1A76C6)
    {
        gSongId = 0x7474614d;
    }
    else
    {
        gSongId = 0;
    }
    mapheader.at16 = B_LITTLE32(gSongId);
    mapheader.at1a = parallaxtype;
    mapheader.at1b = gMapRev;
    mapheader.at1f = B_LITTLE16(numsectors);
    mapheader.at21 = B_LITTLE16(numwalls);
    mapheader.at23 = B_LITTLE16(nSpriteNum);
    if (byte_1A76C7)
    {
        dbCrypt((char*)&mapheader, sizeof(MAPHEADER), 'ttaM');
    }
    IOBuffer1.Write(&mapheader, sizeof(MAPHEADER));
    if (byte_1A76C8)
    {
        Bstrcpy(byte_19AE44.at0, AppProperName);
        byte_19AE44.at48 = nXSectorSize;
        byte_19AE44.at44 = nXWallSize;
        byte_19AE44.at40 = nXSpriteSize;
        dbCrypt((char*)&byte_19AE44, sizeof(MAPHEADER2), numwalls);
        IOBuffer1.Write(&byte_19AE44, sizeof(MAPHEADER2));
        dbCrypt((char*)&byte_19AE44, sizeof(MAPHEADER2), numwalls);
    }
    if (byte_1A76C8)
    {
        dbCrypt((char*)tpskyoff, gSkyCount*sizeof(tpskyoff[0]), gSkyCount*sizeof(tpskyoff[0]));
    }
    IOBuffer1.Write(tpskyoff, gSkyCount*sizeof(tpskyoff[0]));
    if (byte_1A76C8)
    {
        dbCrypt((char*)tpskyoff, gSkyCount*sizeof(tpskyoff[0]), gSkyCount*sizeof(tpskyoff[0]));
    }
    for (int i = 0; i < numsectors; i++)
    {
        if (byte_1A76C8)
        {
            dbCrypt((char*)&sector[i], sizeof(sectortype), gMapRev*sizeof(sectortype));
        }
        IOBuffer1.Write(&sector[i], sizeof(sectortype));
        if (byte_1A76C8)
        {
            dbCrypt((char*)&sector[i], sizeof(sectortype), gMapRev*sizeof(sectortype));
        }
        if (sector[i].extra > 0)
        {
            char pBuffer[nXSectorSize];
            BitWriter bitWriter(pBuffer, nXSectorSize);
            XSECTOR* pXSector = &xsector[sector[i].extra];
            bitWriter.write(pXSector->reference, 14);
            bitWriter.write(pXSector->state, 1);
            bitWriter.write(pXSector->busy, 17);
            bitWriter.write(pXSector->data, 16);
            bitWriter.write(pXSector->txID, 10);
            bitWriter.write(pXSector->at7_2, 3);
            bitWriter.write(pXSector->at7_5, 3);
            bitWriter.write(pXSector->rxID, 10);
            bitWriter.write(pXSector->command, 8);
            bitWriter.write(pXSector->triggerOn, 1);
            bitWriter.write(pXSector->triggerOff, 1);
            bitWriter.write(pXSector->busyTimeA, 12);
            bitWriter.write(pXSector->waitTimeA, 12);
            bitWriter.write(pXSector->atd_4, 1);
            bitWriter.write(pXSector->interruptable, 1);
            bitWriter.write(pXSector->amplitude, 8);
            bitWriter.write(pXSector->freq, 8);
            bitWriter.write(pXSector->atf_6, 1);
            bitWriter.write(pXSector->atf_7, 1);
            bitWriter.write(pXSector->phase, 8);
            bitWriter.write(pXSector->wave, 4);
            bitWriter.write(pXSector->shadeAlways, 1);
            bitWriter.write(pXSector->shadeFloor, 1);
            bitWriter.write(pXSector->shadeCeiling, 1);
            bitWriter.write(pXSector->shadeWalls, 1);
            bitWriter.write(pXSector->shade, 8);
            bitWriter.write(pXSector->panAlways, 1);
            bitWriter.write(pXSector->panFloor, 1);
            bitWriter.write(pXSector->panCeiling, 1);
            bitWriter.write(pXSector->Drag, 1);
            bitWriter.write(pXSector->Underwater, 1);
            bitWriter.write(pXSector->Depth, 3);
            bitWriter.write(pXSector->panVel, 8);
            bitWriter.write(pXSector->panAngle, 11);
            bitWriter.write(pXSector->at16_3, 1);
            bitWriter.write(pXSector->decoupled, 1);
            bitWriter.write(pXSector->triggerOnce, 1);
            bitWriter.write(pXSector->at16_6, 1);
            bitWriter.write(pXSector->Key, 3);
            bitWriter.write(pXSector->Push, 1);
            bitWriter.write(pXSector->Vector, 1);
            bitWriter.write(pXSector->Reserved, 1);
            bitWriter.write(pXSector->Enter, 1);
            bitWriter.write(pXSector->Exit, 1);
            bitWriter.write(pXSector->Wallpush, 1);
            bitWriter.write(pXSector->color, 1);
            bitWriter.write(pXSector->at18_1, 1);
            bitWriter.write(pXSector->busyTimeB, 12);
            bitWriter.write(pXSector->waitTimeB, 12);
            bitWriter.write(pXSector->at1b_2, 1);
            bitWriter.write(pXSector->at1b_3, 1);
            bitWriter.write(pXSector->ceilpal, 4);
            bitWriter.write(pXSector->at1c_0, 32);
            bitWriter.write(pXSector->at20_0, 32);
            bitWriter.write(pXSector->at24_0, 32);
            bitWriter.write(pXSector->at28_0, 32);
            bitWriter.write(pXSector->at2c_0, 16);
            bitWriter.write(pXSector->at2e_0, 16);
            bitWriter.write(pXSector->Crush, 1);
            bitWriter.write(pXSector->at30_1, 8);
            bitWriter.write(pXSector->at31_1, 8);
            bitWriter.write(pXSector->at32_1, 8);
            bitWriter.write(pXSector->damageType, 3);
            bitWriter.write(pXSector->floorpal, 4);
            bitWriter.write(pXSector->at34_0, 8);
            bitWriter.write(pXSector->locked, 1);
            bitWriter.write(pXSector->windVel, 10);
            bitWriter.write(pXSector->windAng, 11);
            bitWriter.write(pXSector->windAlways, 1);
            bitWriter.write(pXSector->at37_7, 1);
            bitWriter.write(pXSector->bobTheta, 11);
            bitWriter.write(pXSector->bobZRange, 5);
            bitWriter.write(pXSector->bobSpeed, 12);
            bitWriter.write(pXSector->bobAlways, 1);
            bitWriter.write(pXSector->bobFloor, 1);
            bitWriter.write(pXSector->bobCeiling, 1);
            bitWriter.write(pXSector->bobRotate, 1);
            IOBuffer1.Write(pBuffer, nXSectorSize);
        }
    }
    for (int i = 0; i < numwalls; i++)
    {
        if (byte_1A76C8)
        {
            dbCrypt((char*)&wall[i], sizeof(walltype), gMapRev*sizeof(sectortype) | 0x7474614d);
        }
        IOBuffer1.Write(&wall[i], sizeof(walltype));
        if (byte_1A76C8)
        {
            dbCrypt((char*)&wall[i], sizeof(walltype), gMapRev*sizeof(sectortype) | 0x7474614d);
        }
        if (wall[i].extra > 0)
        {
            char pBuffer[nXWallSize];
            BitWriter bitWriter(pBuffer, nXWallSize);
            XWALL* pXWall = &xwall[wall[i].extra];
            bitWriter.write(pXWall->reference, 14);
            bitWriter.write(pXWall->state, 1);
            bitWriter.write(pXWall->busy, 17);
            bitWriter.write(pXWall->data, 16);
            bitWriter.write(pXWall->txID, 10);
            bitWriter.write(pXWall->at7_2, 6);
            bitWriter.write(pXWall->rxID, 10);
            bitWriter.write(pXWall->command, 8);
            bitWriter.write(pXWall->triggerOn, 1);
            bitWriter.write(pXWall->triggerOff, 1);
            bitWriter.write(pXWall->busyTime, 12);
            bitWriter.write(pXWall->waitTime, 12);
            bitWriter.write(pXWall->restState, 1);
            bitWriter.write(pXWall->interruptable, 1);
            bitWriter.write(pXWall->panAlways, 1);
            bitWriter.write(pXWall->panXVel, 8);
            bitWriter.write(pXWall->panYVel, 8);
            bitWriter.write(pXWall->decoupled, 1);
            bitWriter.write(pXWall->triggerOnce, 1);
            bitWriter.write(pXWall->isTriggered, 1);
            bitWriter.write(pXWall->key, 3);
            bitWriter.write(pXWall->triggerPush, 1);
            bitWriter.write(pXWall->triggerVector, 1);
            bitWriter.write(pXWall->triggerReserved, 1);
            bitWriter.write(pXWall->at11_0, 2);
            bitWriter.write(pXWall->xpanFrac, 8);
            bitWriter.write(pXWall->ypanFrac, 8);
            bitWriter.write(pXWall->locked, 1);
            bitWriter.write(pXWall->dudeLockout, 1);
            bitWriter.write(pXWall->at13_4, 4);
            bitWriter.write(pXWall->at14_0, 32);
            IOBuffer1.Write(pBuffer, nXWallSize);
        }
    }
    for (int i = 0; i < kMaxSprites; i++)
    {
        if (sprite[i].statnum < kMaxStatus)
        {
            if (byte_1A76C8)
            {
                dbCrypt((char*)&sprite[i], sizeof(spritetype), gMapRev*sizeof(spritetype) | 'ttaM');
            }
            IOBuffer1.Write(&sprite[i], sizeof(spritetype));
            if (byte_1A76C8)
            {
                dbCrypt((char*)&sprite[i], sizeof(spritetype), gMapRev*sizeof(spritetype) | 'ttaM');
            }
            if (sprite[i].extra > 0)
            {
                char pBuffer[nXSpriteSize];
                BitWriter bitWriter(pBuffer, nXSpriteSize);
                XSPRITE* pXSprite = &xsprite[sprite[i].extra];
                bitWriter.write(pXSprite->reference, 14);
                bitWriter.write(pXSprite->state, 1);
                bitWriter.write(pXSprite->busy, 17);
                bitWriter.write(pXSprite->txID, 10);
                bitWriter.write(pXSprite->rxID, 10);
                bitWriter.write(pXSprite->command, 8);
                bitWriter.write(pXSprite->triggerOn, 1);
                bitWriter.write(pXSprite->triggerOff, 1);
                bitWriter.write(pXSprite->wave, 2);
                bitWriter.write(pXSprite->busyTime, 12);
                bitWriter.write(pXSprite->waitTime, 12);
                bitWriter.write(pXSprite->restState, 1);
                bitWriter.write(pXSprite->Interrutable, 1);
                bitWriter.write(pXSprite->atb_2, 2);
                bitWriter.write(pXSprite->respawnPending, 2);
                bitWriter.write(pXSprite->atb_6, 1);
                bitWriter.write(pXSprite->lT, 1);
                bitWriter.write(pXSprite->dropMsg, 8);
                bitWriter.write(pXSprite->Decoupled, 1);
                bitWriter.write(pXSprite->triggerOnce, 1);
                bitWriter.write(pXSprite->isTriggered, 1);
                bitWriter.write(pXSprite->key, 3);
                bitWriter.write(pXSprite->Push, 1);
                bitWriter.write(pXSprite->Vector, 1);
                bitWriter.write(pXSprite->Impact, 1);
                bitWriter.write(pXSprite->Pickup, 1);
                bitWriter.write(pXSprite->Touch, 1);
                bitWriter.write(pXSprite->Sight, 1);
                bitWriter.write(pXSprite->Proximity, 1);
                bitWriter.write(pXSprite->ate_5, 2);
                bitWriter.write(pXSprite->lSkill, 5);
                bitWriter.write(pXSprite->lS, 1);
                bitWriter.write(pXSprite->lB, 1);
                bitWriter.write(pXSprite->lC, 1);
                bitWriter.write(pXSprite->DudeLockout, 1);
                bitWriter.write(pXSprite->data1, 16);
                bitWriter.write(pXSprite->data2, 16);
                bitWriter.write(pXSprite->data3, 16);
                bitWriter.write(pXSprite->goalAng, 11);
                bitWriter.write(pXSprite->dodgeDir, 2);
                bitWriter.write(pXSprite->locked, 1);
                bitWriter.write(pXSprite->medium, 2);
                bitWriter.write(pXSprite->respawn, 2);
                bitWriter.write(pXSprite->data4, 16);
                bitWriter.write(pXSprite->at1a_2, 6);
                bitWriter.write(pXSprite->lockMsg, 8);
                bitWriter.write(pXSprite->health, 12);
                bitWriter.write(pXSprite->dudeDeaf, 1);
                bitWriter.write(pXSprite->dudeAmbush, 1);
                bitWriter.write(pXSprite->dudeGuard, 1);
                bitWriter.write(pXSprite->dudeFlag4, 1);
                bitWriter.write(pXSprite->target, 16);
                bitWriter.write(pXSprite->targetX, 32);
                bitWriter.write(pXSprite->targetY, 32);
                bitWriter.write(pXSprite->targetZ, 32);
                bitWriter.write(pXSprite->burnTime, 16);
                bitWriter.write(pXSprite->burnSource, 16);
                bitWriter.write(pXSprite->height, 16);
                bitWriter.write(pXSprite->stateTimer, 16);
                IOBuffer1.Write(pBuffer, nXSpriteSize);
            }
        }
    }
    unsigned long nCRC = Bcrc32(pData, nSize-4, 0);
    IOBuffer1.Write(&nCRC, 4);
    int nHandle = Bopen(sMapExt, BO_BINARY|BO_TRUNC|BO_CREAT|BO_WRONLY, BS_IREAD|BS_IWRITE);
    if (nHandle == -1)
    {
        initprintf("Couldn't open \"%s\" for writing: %s\n", sMapExt, strerror(errno));
        Bfree(pData);
        return -1;
    }
    if (Bwrite(nHandle, pData, nSize) != nSize)
    {
        initprintf("Couldn't write to \"%s\": %s\n", sMapExt, strerror(errno));
        Bclose(nHandle);
        Bfree(pData);
        return -1;
    }
    Bclose(nHandle);
    Bfree(pData);
    return 0;
#if 0
    char *pExt = strchr(sMapExt, '.');
    if (pExt)
    {
        *pExt = 0;
    }
    gSysRes.AddExternalResource(sMapExt, "MAP", nSize);
    DICTNODE *hMap = gSysRes.Lookup(sMapExt, "MAP");
    dassert(hMap != NULL);
#endif
}

int32_t qloadboard(const char* filename, char flags, vec3_t* dapos, int16_t* daang, int16_t* dacursectnum)
{
    // NUKE-TODO: implement flags, see mapedit.cpp
    return dbLoadMap(filename, &dapos->x, &dapos->y, &dapos->z, (short*)daang, (short*)dacursectnum, NULL);
}

int32_t qsaveboard(const char* filename, const vec3_t* dapos, int16_t daang, int16_t dacursectnum)
{
    // NUKE-TODO: see mapedit.cpp
    byte_1A76C6 = byte_1A76C8 = byte_1A76C7 = 1;
    return dbSaveMap(filename, dapos->x, dapos->y, dapos->z, daang, dacursectnum);
}
