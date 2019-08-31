#include "compat.h"
#include "keyboard.h"
#include "control.h"
#include "init.h"
#include "runlist.h"
#include "switch.h"
#include "object.h"
#include "aistuff.h"
#include "player.h"
#include "mummy.h"
#include "move.h"
#include "ra.h"
#include "view.h"
#include "runlist.h"
#include "engine.h"
#include "sound.h"
#include "exhumed.h"
#include "config.h"
#include "items.h"
#include "light.h"
#include "map.h"
#include "menu.h"
#include "lighting.h"
#include "anims.h"
#include "input.h"
#include "util.h"
#include <stdio.h>
#include <string.h>

enum
{
    kTagRamses = 61,
};

int ototalclock = 0;

int initx, inity, initz;
short inita, initsect;

short nCurChunkNum = 0;

short nBodyGunSprite[50];
int movefifoend;
int movefifopos;

short nCurBodyGunNum;

short SectSoundSect[kMaxSectors] = { 0 };
short SectSound[kMaxSectors]     = { 0 };
short SectFlag[kMaxSectors]      = { 0 };
int   SectDepth[kMaxSectors]     = { 0 };
int   SectAbove[kMaxSectors]     = { 0 };
short SectDamage[kMaxSectors]    = { 0 };
short SectSpeed[kMaxSectors]     = { 0 };
int   SectBelow[kMaxSectors]     = { 0 };


uint8_t bIsVersion6 = kTrue;

// definitions for map version 6 structures
#pragma pack(1)

// 37 bytes
struct Sector_6
{
    uint16_t wallptr, wallnum;
    short ceilingpicnum, floorpicnum;
    short ceilingheinum, floorheinum;
    int ceilingz, floorz;
    int8_t ceilingshade, floorshade;
    uint8_t ceilingxpanning, floorxpanning;
    uint8_t ceilingypanning, floorypanning;
    uint8_t ceilingstat, floorstat;
    uint8_t ceilingpal, floorpal;
    uint8_t visibility;
    short lotag, hitag, extra;
};

struct Wall_6
{
    int x, y;
    short point2, nextsector, nextwall;
    short picnum, overpicnum;
    int8_t shade;
    uint8_t pal;
    short cstat;
    uint8_t xrepeat, yrepeat, xpanning, ypanning;
    short lotag, hitag, extra;
};

// 43 bytes
struct Sprite_6
{
    int x, y, z;
    short cstat;
    int8_t shade;
    uint8_t pal, clipdist;
    uint8_t xrepeat, yrepeat;
    int8_t xoffset, yoffset;
    short picnum, ang, xvel, yvel, zvel, owner;
    short sectnum, statnum;
    short lotag, hitag, extra;
};

#pragma pack()

static Sector_6 sector_6[1024];
static Wall_6   wall_6[8192];
static Sprite_6 sprite_6[4096];



uint8_t LoadLevel(int nMap)
{
    char fileName_1[80];
    char fileName_2[32];

    initspritelists();

//	nMap = 1;

//	sprintf(fileName_2, "queen");
    sprintf(fileName_2, "lev%d", nMap);
//	sprintf(fileName_2, "sentry");
//	sprintf(fileName_2, "bjd");
//	sprintf(fileName_2, "door");
//	sprintf(fileName_2, "ceil");
//	sprintf(fileName_2, "scarab");
//	sprintf(fileName_2, "guns");
//	sprintf(fileName_2, "wasp");
//	sprintf(fileName_2, "items");

    fileName_1[0] = '\0';
    strcat(fileName_1, fileName_2);
    strcat(fileName_1, ".map");

    // init stuff
    {
        StopAllSounds();
        nCreaturesLeft = 0;
        nFreeze = 0;
        nSpiritSprite = -1;

        InitLion();
        InitRexs();
        InitSets();
        InitQueens();
        InitRoachs();
        InitWasps();
        InitRats();
        InitBullets();
        InitWeapons();
        InitGrenades();
        InitAnims();
        InitSnakes();
        InitFishes();
        InitLights();
        InitMap();
        InitBubbles();
        InitObjects();
        InitLava();
        InitPushBlocks();
        InitAnubis();
        InitSpider();
        InitMummy();
        InitScorp();
        InitPlayer();
        InitItems();
        InitInput();

        if (nMap == kMap20) {
            InitEnergyTile();
        }
    }

    if (nMap > 15)
    {
        nSwitchSound = 35;
        nStoneSound = 23;
        nElevSound = 51;
        nStopSound = 35;
    }
    else
    {
        nSwitchSound = 33;
        nStoneSound = 23;
        nElevSound = 23;
        nStopSound = 66;
    }

    if (nMap < 0) {
        return kFalse;
    }

#if 0
    {
        // going to load the map without loadboard() - to take care of version 6 to 7 map conversion
        //int hFile = kopen4load(fileName_1, 1);
        int hFile = kopen4load(fileName_1, 0);
        //	int hFile = open(fileName_1, O_BINARY | O_RDONLY);
        if (hFile == -1) {
            return kFalse;
        }

        int version;

        kread(hFile, &version, sizeof(version));
        if (version != 6) {
            bIsVersion6 = kFalse;
        }

        initspritelists();

        memset(show2dsector, 0, sizeof(show2dsector));
        memset(show2dsprite, 0, sizeof(show2dsprite));
        memset(show2dwall,   0, sizeof(show2dwall));

        // replacement for build's loadboard()
        kread(hFile, &initx, 4);
        kread(hFile, &inity, 4);
        kread(hFile, &initz, 4);
        kread(hFile, &inita, 2);
        kread(hFile, &initsect, 2);

        // sectors
        short nSectors;
        kread(hFile, &nSectors, sizeof(nSectors));

        if (bIsVersion6) {
            kread(hFile, sector_6, sizeof(Sector_6) * nSectors);
        }
        else {
            kread(hFile, sector, sizeof(SECTOR) * nSectors);
        }

        // walls
        short nWalls;
        kread(hFile, &nWalls, sizeof(nWalls));

        if (bIsVersion6) {
            kread(hFile, wall_6, sizeof(Wall_6) * nWalls);
        }
        else {
            kread(hFile, wall, sizeof(WALL) * nWalls);
        }

        // sprites
        short nSprites;
        kread(hFile, &nSprites, sizeof(nSprites));

        if (bIsVersion6) {
            kread(hFile, sprite_6, sizeof(Sprite_6) * nSprites);
        }
        else {
            kread(hFile, sprite, sizeof(SPRITE) * nSprites);
        }

        // set engine variables
        numsectors = nSectors;
        numwalls = nWalls;

        // load in our version 6 structs to the engines v7 structs if required
        if (bIsVersion6)
        {
            for (int nSector = 0; nSector < nSectors; nSector++)
            {
                sector[nSector].ceilingz = sector_6[nSector].ceilingz;
                sector[nSector].floorz = sector_6[nSector].floorz;
                sector[nSector].wallptr = sector_6[nSector].wallptr;
                sector[nSector].wallnum = sector_6[nSector].wallnum;
                sector[nSector].ceilingpicnum = sector_6[nSector].ceilingpicnum;
                sector[nSector].ceilingheinum = Max(Min(((int)sector_6[nSector].ceilingheinum) << 5, 32767), -32768);

                if ((sector_6[nSector].ceilingstat & 2) == 0) {
                    sector[nSector].ceilingheinum = 0;
                }

                sector[nSector].ceilingshade = sector_6[nSector].ceilingshade;
                sector[nSector].ceilingpal = sector_6[nSector].ceilingpal;
                sector[nSector].ceilingxpanning = sector_6[nSector].ceilingxpanning;
                sector[nSector].ceilingypanning = sector_6[nSector].ceilingypanning;
                sector[nSector].floorpicnum = sector_6[nSector].floorpicnum;
                sector[nSector].floorheinum = Max(Min(((int)sector_6[nSector].floorheinum) << 5, 32767), -32768);

                if ((sector_6[nSector].floorstat & 2) == 0) {
                    sector[nSector].floorheinum = 0;
                }

                sector[nSector].floorshade = sector_6[nSector].floorshade;
                sector[nSector].floorpal = sector_6[nSector].floorpal;
                sector[nSector].floorxpanning = sector_6[nSector].floorxpanning;
                sector[nSector].floorypanning = sector_6[nSector].floorypanning;
                sector[nSector].ceilingstat = sector_6[nSector].ceilingstat;
                sector[nSector].floorstat = sector_6[nSector].floorstat;
                sector[nSector].visibility = sector_6[nSector].visibility;
                sector[nSector].filler = 0;
                sector[nSector].lotag = sector_6[nSector].lotag;
                sector[nSector].hitag = sector_6[nSector].hitag;
                sector[nSector].extra = sector_6[nSector].extra;
            }

            for (int nWall = 0; nWall < nWalls; nWall++)
            {
                wall[nWall].x = wall_6[nWall].x;
                wall[nWall].y = wall_6[nWall].y;
                wall[nWall].point2 = wall_6[nWall].point2;
                wall[nWall].nextwall = wall_6[nWall].nextwall;
                wall[nWall].nextsector = wall_6[nWall].nextsector;
                wall[nWall].cstat = wall_6[nWall].cstat;
                wall[nWall].picnum = wall_6[nWall].picnum;
                wall[nWall].overpicnum = wall_6[nWall].overpicnum;
                wall[nWall].shade = wall_6[nWall].shade;
                wall[nWall].pal = wall_6[nWall].pal;
                wall[nWall].xrepeat = wall_6[nWall].xrepeat;
                wall[nWall].yrepeat = wall_6[nWall].yrepeat;
                wall[nWall].xpanning = wall_6[nWall].xpanning;
                wall[nWall].ypanning = wall_6[nWall].ypanning;
                wall[nWall].lotag = wall_6[nWall].lotag;
                wall[nWall].hitag = wall_6[nWall].hitag;
                wall[nWall].extra = wall_6[nWall].extra;
            }

            for (int nSprite = 0; nSprite < nSprites; nSprite++)
            {
                sprite[nSprite].x = sprite_6[nSprite].x;
                sprite[nSprite].y = sprite_6[nSprite].y;
                sprite[nSprite].z = sprite_6[nSprite].z;
                sprite[nSprite].cstat = sprite_6[nSprite].cstat;
                sprite[nSprite].picnum = sprite_6[nSprite].picnum;
                sprite[nSprite].shade = sprite_6[nSprite].shade;
                sprite[nSprite].pal = sprite_6[nSprite].pal;
                sprite[nSprite].clipdist = sprite_6[nSprite].clipdist;
                sprite[nSprite].filler = 0;
                sprite[nSprite].xrepeat = sprite_6[nSprite].xrepeat;
                sprite[nSprite].yrepeat = sprite_6[nSprite].yrepeat;
                sprite[nSprite].xoffset = sprite_6[nSprite].xoffset;
                sprite[nSprite].yoffset = sprite_6[nSprite].yoffset;
                sprite[nSprite].sectnum = sprite_6[nSprite].sectnum;
                sprite[nSprite].statnum = sprite_6[nSprite].statnum;
                sprite[nSprite].ang = sprite_6[nSprite].ang;
                sprite[nSprite].owner = sprite_6[nSprite].owner;
                sprite[nSprite].xvel = sprite_6[nSprite].xvel;
                sprite[nSprite].yvel = sprite_6[nSprite].yvel;
                sprite[nSprite].zvel = sprite_6[nSprite].zvel;
                sprite[nSprite].lotag = sprite_6[nSprite].lotag;
                sprite[nSprite].hitag = sprite_6[nSprite].hitag;
                sprite[nSprite].extra = sprite_6[nSprite].extra;
            }
        }

        for (int nSprite = 0; nSprite < nSprites; nSprite++) {
            insertsprite(sprite[nSprite].sectnum, sprite[nSprite].statnum);
        }

        updatesector(initx, inity, &initsect);

        kclose(hFile);
        hFile = -1;
    }
    // loadboard has finished
#endif
    vec3_t startPos;
    int status = engineLoadBoard(fileName_1, 0, &startPos, &inita, &initsect);
    if (status == -2)
        status = engineLoadBoardV5V6(fileName_1, 0, &startPos, &inita, &initsect);
    initx = startPos.x;
    inity = startPos.y;
    initz = startPos.z;

#ifdef YAX_ENABLE
    yax_update(1);
#endif

    int i;

    for (i = 0; i < kMaxPlayers; i++)
    {
        PlayerList[i].nSprite = -1;
    }

    psky_t* pSky = tileSetupSky(0);

    pSky->tileofs[0] = 0;
    pSky->tileofs[1] = 0;
    pSky->tileofs[2] = 0;
    pSky->tileofs[3] = 0;
    pSky->yoffs = 256;
    pSky->lognumtiles = 2;
    pSky->horizfrac = 65536;
    pSky->yscale = 65536;
    parallaxtype = 2;
    g_visibility = 2048;
    flash = 0;
    precache();

    LoadObjects();

    levelnum = nMap;

    // TEMP - show full 2D map// TEMP
    for (i = 0; i < (kMaxWalls >> 3); i++)
        show2dwall[i] = 0xFF;
    for (i = 0; i < (kMaxSprites >> 3); i++)
        show2dsprite[i] = 0xFF;
    for (i = 0; i < numsectors; i++) {
        show2dsector[i >> 3] |= (1 << (i & 7));
    }

    return kTrue;
}

void ResetEngine()
{
    uint8_t blankPal[768];

    memset(blankPal, 0, sizeof(blankPal));
    MySetPalette(blankPal);

    SetOverscan(kenpal);

    EraseScreen(-1);

    resettiming();

    totalclock  = 0;
    ototalclock = totalclock;
    localclock  = totalclock;

    numframes = 0;
}

void InstallEngine()
{
    initgroupfile("stuff.dat");

    char *cwd;

    if (g_modDir[0] != '/' && (cwd = buildvfs_getcwd(NULL, 0)))
    {
        buildvfs_chdir(g_modDir);
        if (artLoadFiles("tiles000.art", MAXCACHE1DSIZE) < 0)
        {
            buildvfs_chdir(cwd);
            if (artLoadFiles("tiles000.art", MAXCACHE1DSIZE) < 0)
                bail2dos("Failed loading art.");
        }
        buildvfs_chdir(cwd);
#ifndef __ANDROID__ //This crashes on *some* Android devices. Small onetime memory leak. TODO fix above function
        Xfree(cwd);
#endif
    }
    else if (artLoadFiles("tiles000.art",MAXCACHE1DSIZE) < 0)
        bail2dos("Failed loading art.");

    // TEMP

    //nScreenWidth *= 2;
    //nScreenHeight *= 2;
    bHiRes = kTrue;
    // TEMP

    if (engineInit())
    {
        wm_msgbox("Fatal Engine Initialization Error",
                  "There was a problem initializing the engine: %s\n\nThe application will now close.", engineerrstr);
        //TODO:
        //G_Cleanup();
        ERRprintf("G_Startup: There was a problem initializing the engine: %s\n", engineerrstr);
        exit(6);
    }
    if (videoSetGameMode(gSetup.fullscreen, gSetup.xdim, gSetup.ydim, gSetup.bpp, 0) < 0)
    {
        initprintf("Failure setting video mode %dx%dx%d %s! Trying next mode...\n", gSetup.xdim, gSetup.ydim,
                    gSetup.bpp, gSetup.fullscreen ? "fullscreen" : "windowed");

        int resIdx = 0;

        for (int i=0; i < validmodecnt; i++)
        {
            if (validmode[i].xdim == gSetup.xdim && validmode[i].ydim == gSetup.ydim)
            {
                resIdx = i;
                break;
            }
        }

        int const savedIdx = resIdx;
        int bpp = gSetup.bpp;

        while (videoSetGameMode(0, validmode[resIdx].xdim, validmode[resIdx].ydim, bpp, 0) < 0)
        {
            initprintf("Failure setting video mode %dx%dx%d windowed! Trying next mode...\n",
                        validmode[resIdx].xdim, validmode[resIdx].ydim, bpp);

            if (++resIdx == validmodecnt)
            {
                if (bpp == 8)
                    bail2dos("Fatal error: unable to set any video mode!");

                resIdx = savedIdx;
                bpp = 8;
            }
        }

        gSetup.xdim = validmode[resIdx].xdim;
        gSetup.ydim = validmode[resIdx].ydim;
        gSetup.bpp  = bpp;
    }

    LoadPaletteLookups();
    MyLoadPalette();
}

void RemoveEngine()
{
    engineUnInit();
    uninitgroupfile();
}

void SetBelow(short nCurSector, short nBelowSector)
{
    SectBelow[nCurSector] = nBelowSector;
}

void SetAbove(short nCurSector, short nAboveSector)
{
    SectAbove[nCurSector] = nAboveSector;
}

void SnapSectors(short nSectorA, short nSectorB, short b)
{
    // edx - nSectorA
    // eax - nSectorB

    short nWallA = sector[nSectorA].wallptr;
    short nWallB = sector[nSectorB].wallptr;

    short num1 = sector[nSectorA].wallnum;
    short num2 = sector[nSectorB].wallnum;

    int nCount = 0;

    while (num1 > nCount)
    {
        short dx = nWallB;
        
        int esi = 0x7FFFFFF;
        int edi = esi;

        int x = wall[nWallA].x;
        int y = wall[nWallA].y;

        int var_14 = 0;

        int nCount2 = 0; 

        while (nCount2 < num2)
        {
            int eax = x - wall[dx].x;
            int ebx = y - wall[dx].y;

            if (eax < 0) {
                eax = -eax;
            }

            int var_38 = eax;

            if (ebx < 0) {
                ebx = -ebx;
            }

            int var_3C = ebx;

            var_38 += var_3C;

            eax = esi;
            if (eax < 0) {
                eax = -eax;
            }

            var_3C = eax;

            eax = edi;
//			int var_34 = edi;
            if (eax < 0) {
                eax = -eax;
            }

            int var_34 = eax;

            var_34 += var_3C;

            if (var_38 < var_34) 
            {
                esi = x - wall[dx].x;
                edi = y - wall[dx].y;
                var_14 = dx;
            }

            dx++;
            nCount2++;
        }

        dragpoint(var_14, wall[var_14].x + esi, wall[var_14].y + edi, 0);

        nCount++;
        nWallA++;
    }

    if (b) {
        sector[nSectorB].ceilingz = sector[nSectorA].floorz;
    }

    if (SectFlag[nSectorA] & 0x1000) {
        SnapBobs(nSectorA, nSectorB);
    }
}

void InitSectFlag()
{
    for (int i = 0; i < kMaxSectors; i++)
    {
        SectSoundSect[i] = -1;
        SectSound[i] = -1;
        SectAbove[i] = -1;
        SectBelow[i] = -1;
        SectDepth[i] = 0;
        SectFlag[i]  = 0;
        SectSpeed[i] = 0;
        SectDamage[i] = 0;
    }
}

void ProcessSpriteTag(short nSprite, short lotag, short hitag)
{
    int nChannel = runlist_AllocChannel(hitag % 1000);
//	int ebp = nChannel;

    int nHitag2 = hitag / 1000;

    int nLotag2 = lotag / 1000;
    if (nLotag2 == 0) {
        nLotag2 = 1;
    }

    if (lotag > 1000) {
        int blahgh = 123;
    }

    // this value can change in the below code but we also need to retain the original hitag value
    int nVal = hitag;

    if (lotag >= 900 && lotag <= 949)
    {
        ProcessTrailSprite(nSprite, lotag, hitag);
        return;
    }

    // handle tags 6 to 60
    switch (lotag)
    {
        case 8: // M-60 ammo belt
        {
            nVal = 3 * (hitag / 3);
            // fall through to 6,7 etc
        }
        case 6:
        case 7:
        case 9:
        case 10:
        case 11:
        case 15:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 26:
        case 28:
        case 29:
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37:
        case 39:
        case 40:
        case 41:
        case 42:
        case 43:
        case 44:
        case 45:
        case 46:
        case 47:
        case 48:
        case 49:
        case 50:
        case 51:
        case 52:
        case 53:
        case 54:
        case 55:
        case 56:
        case 57:
        case 58:
        case 60:
        {
            sprite[nSprite].hitag = nVal;
            changespritestat(nSprite, lotag + 900);
            sprite[nSprite].cstat &= 0xFEFE;
            BuildItemAnim(nSprite);
            return;
        }
        case 12: // berry twig
        {
            sprite[nSprite].hitag = 40;
            changespritestat(nSprite, lotag + 900);
            sprite[nSprite].cstat &= 0xFEFE;
            BuildItemAnim(nSprite);
            return;
        }
        case 13: // blood bowl
        {
            sprite[nSprite].hitag = 160;
            changespritestat(nSprite, lotag + 900);
            sprite[nSprite].cstat &= 0xFEFE;
            BuildItemAnim(nSprite);
            return;
        }
        case 14: // venom bowl
        {
            sprite[nSprite].hitag = -200;
            changespritestat(nSprite, lotag + 900);
            sprite[nSprite].cstat &= 0xFEFE;
            BuildItemAnim(nSprite);
            return;
        }

        case 16:
            // reserved
            mydeletesprite(nSprite);
            return;

        case 25:
        case 59:
        {
            // extra life or checkpoint scarab. Delete for multiplayer
            if (nNetPlayerCount != 0)
            {
                mydeletesprite(nSprite);
                return;
            }
            else
            {
                sprite[nSprite].hitag = nVal;
                changespritestat(nSprite, lotag + 900);
                sprite[nSprite].cstat &= 0xFEFE;
                BuildItemAnim(nSprite);
                return;
            }
        }
        case 27:
        {
            sprite[nSprite].hitag = 1;
            changespritestat(nSprite, 9 + 900);
            sprite[nSprite].cstat &= 0xFEFE;
            BuildItemAnim(nSprite);
            return;
        }

        case 38: // raw energy
        {
            nVal++;
            nVal--; // CHECKME ??
            sprite[nSprite].hitag = nVal;
            changespritestat(nSprite, lotag + 900);
            sprite[nSprite].cstat &= 0xFEFE;
            BuildItemAnim(nSprite);
            return;
        }
    }

    int v6 = lotag % 1000;

    if (!bNoCreatures || v6 < 100 || v6 > 118)
    {
        if (v6 > 999) {
            mydeletesprite(nSprite);
            return;
        }

        switch (v6)
        {
            case 999:
            {
                AddFlicker(sprite[nSprite].sectnum, nLotag2);
                break;
            }
            case 998:
            {
                AddGlow(sprite[nSprite].sectnum, nLotag2);
                break;
            }
            case 118: // Anubis with drum
            {
                if (bNoCreatures) {
                    mydeletesprite(nSprite);
                    return;
                }

                BuildAnubis(nSprite, 0, 0, 0, 0, 0, 1);
                return;
            }
            case 117:
            {
                if (bNoCreatures) {
                    mydeletesprite(nSprite);
                    return;
                }

                BuildWasp(nSprite, 0, 0, 0, 0, 0);
                return;
            }
            case 116:
            {
                BuildRat(nSprite, 0, 0, 0, 0, -1);
                return;
            }
            case 115: // Rat (eating)
            {
                BuildRat(nSprite, 0, 0, 0, 0, 0);
                return;
            }
            case 113:
            {
                BuildQueen(nSprite, 0, 0, 0, 0, 0, nChannel);
                return;
            }
            case 112:
            {
                BuildScorp(nSprite, 0, 0, 0, 0, 0, nChannel);
                return;
            }
            case 111:
            {
                if (bNoCreatures) {
                    mydeletesprite(nSprite);
                    return;
                }

                BuildSet(nSprite, 0, 0, 0, 0, 0, nChannel);
                return;
            }
            case 108:
            {
                if (bNoCreatures) {
                    mydeletesprite(nSprite);
                    return;
                }

                BuildLava(nSprite, 0, 0, 0, 0, 0, nChannel);
                return;
            }
            case 107:
            {
                if (bNoCreatures) {
                    mydeletesprite(nSprite);
                    return;
                }

                BuildRex(nSprite, 0, 0, 0, 0, 0, nChannel);
                return;
            }
            case 106: 
            {
                if (bNoCreatures) {
                    mydeletesprite(nSprite);
                    return;
                }

                BuildFish(nSprite, 0, 0, 0, 0, 0);
                return;
            }
            case 105:
            {
                if (bNoCreatures) {
                    mydeletesprite(nSprite);
                    return;
                }

                BuildSpider(nSprite, 0, 0, 0, 0, 0);
                return;
            }
            case 104:
            {
                if (bNoCreatures) {
                    mydeletesprite(nSprite);
                    return;
                }

                BuildRoach(1, nSprite, 0, 0, 0, 0, 0);
                return;
            }
            case 103:
            {
                if (bNoCreatures) {
                    mydeletesprite(nSprite);
                    return;
                }

                BuildRoach(0, nSprite, 0, 0, 0, 0, 0);
                return;
            }
            case 102:
            {
                if (bNoCreatures) {
                    mydeletesprite(nSprite);
                    return;
                }

                BuildLion(nSprite, 0, 0, 0, 0, 0);
                return;
            }
            case 101:
            {
                if (bNoCreatures) {
                    mydeletesprite(nSprite);
                    return;
                }

                BuildMummy(nSprite, 0, 0, 0, 0, 0);
                return;
            }
            case 100:
            {
                if (bNoCreatures) {
                    mydeletesprite(nSprite);
                    return;
                }

                BuildAnubis(nSprite, 0, 0, 0, 0, 0, 0);
                return;
            }
            case 99: // underwater type 2
            {
                short nSector = sprite[nSprite].sectnum;
                SetAbove(nSector, hitag);
                SectFlag[nSector] |= kSectUnderwater;

                mydeletesprite(nSprite);
                return;
            }
            case 98:
            {
                short nSector = sprite[nSprite].sectnum;
                SetBelow(nSector, hitag);
                SnapSectors(nSector, hitag, 1);

                mydeletesprite(nSprite);
                return;
            }
            case 97:
            {
                AddSectorBob(sprite[nSprite].sectnum, hitag, 1);

                mydeletesprite(nSprite);
                return;
            }
            case 96: // Lava sector
            {
                hitag /= 4; // hitag is damage level?
                if (hitag == 0) {
                    hitag = 1;
                }

                short nSector = sprite[nSprite].sectnum;

                SectDamage[nSector] = hitag;
                SectFlag[nSector] |= kSectLava;

                mydeletesprite(nSprite);
                return;
            }
            case 95:
            {
                AddSectorBob(sprite[nSprite].sectnum, hitag, 0);

                mydeletesprite(nSprite);
                return;
            }
            case 94: // water
            {
                short nSector = sprite[nSprite].sectnum;
                SectDepth[nSector] = hitag << 8;
                
                mydeletesprite(nSprite);
                return;
            }
            case 93:
            {
                BuildBubbleMachine(nSprite);
                return;
            }
            case 90:
            {
                BuildObject(nSprite, 3, hitag);
                return;
            }
            case 79:
            case 89:
            {
                short nSector = sprite[nSprite].sectnum;

                SectSpeed[nSector] = nLotag2;
                SectFlag[nSector] |= sprite[nSprite].ang;

                mydeletesprite(nSprite);
                return;
            }
            case 88:
            {
                AddFlow(nSprite, nLotag2, 0);

                mydeletesprite(nSprite);
                return;
            }
            case 80: // underwater
            {
                short nSector = sprite[nSprite].sectnum;
                SectFlag[nSector] |= kSectUnderwater;

                mydeletesprite(nSprite);
                return;
            }
            case 78:
            {
                AddFlow(nSprite, nLotag2, 1);

                short nSector = sprite[nSprite].sectnum;
                SectFlag[nSector] |= 0x8000;

                mydeletesprite(nSprite);
                return;
            }
            case 77:
            {
                int nArrow = BuildArrow(nSprite, nLotag2);

                runlist_AddRunRec(sRunChannels[nChannel].a, nArrow);
                return;
            }
            case 76: // Explosion Trigger (Exploding Fire Cauldron)
            {
                BuildObject(nSprite, 0, hitag);
                return;
            }
            case 75: // Explosion Target (Cauldrons, fireballs and grenades will destroy nearby 75 sprites)
            {
                BuildObject(nSprite, 1, hitag);
                return;
            }
            case 71:
            {
                int nFireball = BuildFireBall(nSprite, hitag, nLotag2);

                runlist_AddRunRec(sRunChannels[nChannel].a, nFireball);
                return;
            }
            case 70:
            {
                BuildDrip(nSprite);
                return;
            }
            case 63:
            {
                changespritestat(nSprite, 405);
                sprite[nSprite].cstat = 0x8000;
                return;
            }
            case 62:
            {
                nNetStartSprite[nNetStartSprites] = nSprite;
                sprite[nSprite].cstat = 0x8000;

                nNetStartSprites++;
                return;
            }
            case kTagRamses: // Ramses head
            {
                nSpiritSprite = nSprite;
                sprite[nSprite].cstat |= 0x8000;
                return;
            }
            default: // TODO - checkme!
            {
                mydeletesprite(nSprite);
                return;
            }
        }
    }

    mydeletesprite(nSprite);
}

void ExamineSprites()
{
    nNetStartSprites = 0;
    nCurStartSprite = 0;

    for (int nSprite = 0; nSprite < kMaxSprites; nSprite++)
    {
        int nStatus = sprite[nSprite].statnum;
        if (!nStatus)
        {
            short lotag = sprite[nSprite].lotag;
            short hitag = sprite[nSprite].hitag;

            if ((nStatus < kMaxStatus) && lotag)
            {
                sprite[nSprite].lotag = 0;
                sprite[nSprite].hitag = 0;

                ProcessSpriteTag(nSprite, lotag, hitag);
            }
            else
            {
                changespritestat(nSprite, 0);
            }
        }
    }

    if (nNetPlayerCount)
    {
        int nSprite = insertsprite(initsect, 0);
        sprite[nSprite].x = initx;
        sprite[nSprite].y = inity;
        sprite[nSprite].z = initz;
        sprite[nSprite].cstat = 0x8000;
        nNetStartSprite[nNetStartSprites] = nSprite;
        nNetStartSprites++;
    }
}

void LoadObjects()
{
    runlist_InitRun();
    runlist_InitChan();
    InitLink();
    InitPoint();
    InitSlide();
    InitSwitch();
    InitElev();
    InitWallFace();
    InitTimeSlot();
    InitSectFlag();

    for (int nSector = 0; nSector < numsectors; nSector++)
    {
        short hitag = sector[nSector].hitag;
        short lotag = sector[nSector].lotag;

        sector[nSector].hitag = 0;
        sector[nSector].lotag = 0;
        sector[nSector].extra = -1;

        if (hitag || lotag)
        {
            sector[nSector].lotag = runlist_HeadRun() + 1;
            sector[nSector].hitag = lotag;

            runlist_ProcessSectorTag(nSector, lotag, hitag);
        }
    }

    for (int nWall = 0; nWall < numwalls; nWall++)
    {
        wall[nWall].extra = -1;

        short lotag = wall[nWall].lotag;
        short hitag = wall[nWall].hitag;

        wall[nWall].lotag = 0;

        if (hitag || lotag)
        {
            wall[nWall].lotag = runlist_HeadRun() + 1;
            runlist_ProcessWallTag(nWall, lotag, hitag);
        }
    }

    ExamineSprites();
    PostProcess();
    InitRa();
    InitChunks();

    for (int nSprite = 0; nSprite < kMaxSprites; nSprite++)
    {
        runlist_ChangeChannel(nSprite, 0);
        runlist_ReadyChannel(nSprite);
    }

    nCamerax = initx;
    nCameray = inity;
    nCameraz = initz;
}

int myloadconfig()
{
    FILE *fp = fopen("psa.ini", "rb");

    if (fp == NULL)
    {
        gFXVolume = 200;
        nGamma = 2;
        gMusicVolume = 200;
        bFullScreen = 0;
        mysetbrightness((uint8_t)nGamma);

        lMouseSens = 8;
        return -1;
    }

    fread(&gFXVolume,    sizeof(gFXVolume), 1, fp);
    fread(&gMusicVolume, sizeof(gMusicVolume), 1, fp);
    fread(&screensize,   sizeof(screensize), 1, fp);
    fread(&bFullScreen,  sizeof(bFullScreen), 1, fp);
    fread(&nGamma,       sizeof(nGamma), 1, fp);
    fread(&lMouseSens,   sizeof(lMouseSens), 1, fp);

    if (bHiRes) {
        screensize *= 2;
    }

    if (screensize > xdim || screensize < xdim >> 2) {
        screensize = xdim;
    }

    fclose(fp);

    if (gFXVolume > 255) {
        gFXVolume = 125;
    }

    if (gMusicVolume > 255) {
        gMusicVolume = 125;
    }

    return 1;
}

int mysaveconfig()
{
    FILE *fp = fopen("psa.ini", "wb");
    if (fp == NULL) {
        return -1;
    }

    fwrite(&gFXVolume,    sizeof(gFXVolume), 1, fp);
    fwrite(&gMusicVolume, sizeof(gMusicVolume), 1, fp);

    short nSize = screensize;

    if (bHiRes)
    {
        nSize = screensize / 2;
    }

    fwrite(&nSize,       sizeof(nSize), 1, fp);
    fwrite(&bFullScreen, sizeof(bFullScreen), 1, fp);
    fwrite(&nGamma,      sizeof(nGamma), 1, fp);
    fwrite(&lMouseSens,  sizeof(lMouseSens), 1, fp);

    fclose(fp);
    return 1;
}
