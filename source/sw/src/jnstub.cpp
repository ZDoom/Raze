//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2005 - 3D Realms Entertainment

This file is NOT part of Shadow Warrior version 1.2
However, it is either an older version of a file that is, or is
some test code written during the development of Shadow Warrior.
This file is provided purely for educational interest.

Shadow Warrior is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

Prepared for public release: 03/28/2005 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

#include "build.h"
#include "editor.h"
#include "cache1d.h"

#include "keys.h"
#include "names2.h"
#include "game.h"
#include "tags.h"
#include "pal.h"

#include "common.h"
#include "common_game.h"

#include "colormap.h"


const char* AppProperName = "Wangulator";
const char* AppTechnicalName = "wangulator";

#if defined(_WIN32)
#define DEFAULT_GAME_EXEC "voidsw.exe"
#define DEFAULT_GAME_LOCAL_EXEC "voidsw.exe"
#else
#define DEFAULT_GAME_EXEC "voidsw"
#define DEFAULT_GAME_LOCAL_EXEC "./voidsw"
#endif

const char *DefaultGameExec = DEFAULT_GAME_EXEC;
const char *DefaultGameLocalExec = DEFAULT_GAME_LOCAL_EXEC;

#define SETUPFILENAME "wangulator.cfg"
const char *defaultsetupfilename = SETUPFILENAME;
char setupfilename[BMAX_PATH] = SETUPFILENAME;


#define M_RED 102
#define M_BLUE 198


void ToggleFAF(void);       // brooms.c
void FAF_AfterDrawRooms(void);  // brooms.c
void ResetBuildFAF(void);   // brooms.c


// Jim's Vars

#define BUILD_DEV_VER   0   // True if this is the developer version of build.


// Idle time counter
//int  idleclock=0;        // How much time is spent not touching the keyboard.
//int    slackerclock=0;       // Accumulated no keyboard time, adds every 30 secs
//int    oldtotalclock=0;

// Sprite lists
int numsprite[MAXSPRITES], multisprite[MAXSPRITES];

// Next available tag tracking vars
short siNextTag = 1;                    // Shows next available tag if there
// is an open gap in tagging
short siNextEndTag = 1;                 // Shows the hightest possible next
// tag

int loaded_numwalls;
// Boolean flags used for sprite searching
SWBOOL bFindPicNum = TRUE;                // Default
SWBOOL bFindHiTag = FALSE;
SWBOOL bFindLowTag = FALSE;
SWBOOL bVoxelsOn = TRUE;                  // Turn voxels on by default
SWBOOL bSpinBobVoxels = TRUE;             // Do twizzly stuff to voxels
SWBOOL bAutoSize = TRUE;                  // Autosizing on/off

int nextvoxid = 0;

// Globals used to hold current sprite type being searched for.
short FindPicNum = 0;
short FindSpriteNum = 0;

// My Function Prototypes
void ContextHelp(short spritenum);
void LoadKVXFromScript(const char *filename);

//void LogUserTime( SWBOOL bIsLoggingIn );

// voxelarray format is:
//      spritenumber, voxelnumber
extern int aVoxelArray[MAXTILES];

// Ken ALT highlighted array
extern short highlightsector[MAXSECTORS];
extern short highlightsectorcnt;


int DispMono = FALSE;

//
// KENS setup variables
//

#define MAX_STAG_INFO 1024
typedef struct
{
    char name[64];
    int flags;
} STAG_INFO, *STAG_INFOp;

STAG_INFO StagInfo[MAX_STAG_INFO];

void PrintStatus(const char *string, int num, char x, char y, char color);

#define NUMOPTIONS 8
char option[NUMOPTIONS] = {0, 0, 0, 0, 0, 0, 1, 0};
char default_buildkeys[NUMBUILDKEYS] =
{
    0xc8, 0xd0, 0xcb, 0xcd, 0x2a, 0x9d, 0x1d, 0x39,
    0x1e, 0x2c, 0xd1, 0xc9, 0x33, 0x34,
    0x9c, 0x1c, 0xd, 0xc, 0xf, 0x45
};

#define MODE_3D 200

extern short pointhighlight, linehighlight;
extern short asksave;
short ExtSectorTag[MAXSECTORS][4];
static char tempbuf[256];
char ds[256];

enum
{
    CAPTION_NONE,
    CAPTION_DEFAULT,
    CAPTION_NAMES,
    CAPTION_MOST,
    CAPTION_ALL,
    CAPTION_MAX,
};
short CaptionMode = CAPTION_NAMES;


// RIGHT ALT selection key
extern short highlightsector[MAXSECTORS];
extern short highlightsectorcnt;

// RIGHT SHIFT selection key
#define SPRITE_FLAG 16384
extern short highlight[MAXWALLS+MAXSPRITES];       // sprite nums are + 16348
extern short highlightcnt;

// Variables copied with the tab key
extern int32_t temppicnum, tempcstat, templotag, temphitag, tempextra;

void SectorMoveFloorZ(int);
void SectorMoveCeilingZ(int);

void BuildStagTable(void);
void Message(const char *string, char color);
void ShowMessage(void);
void ShadeMenu(void);
void FindNextTag(void);
void ShowNextTag(void);
void FindSprite(short picnum, short findspritenum);
void FindNextSprite(short picnum);
void SetClipdist2D(void);
void DrawClipBox(short spritenum);

//printext16 parameters:
//printext16(int xpos, int ypos, short col, short backcol,
//           char name[82], char fontsize)
//  xpos 0-639   (top left)
//  ypos 0-479   (top left)
//  col 0-15
//  backcol 0-15, -1 is transparent background
//  name
//  fontsize 0=8*8, 1=3*5

//drawline16 parameters:
// drawline16(int x1, int y1, int x2, int y2, char col)
//  x1, x2  0-639
//  y1, y2  0-143  (status bar is 144 high, origin is top-left of STATUS BAR)
//  col     0-15

//Detecting 2D / 3D mode:
//   qsetmode is 200 in 3D mode
//   qsetmode is 350/480 in 2D mode
//
//You can read these variables when F5-F8 is pressed in 3D mode only:
//
//   If (searchstat == 0)  WALL        searchsector=sector, searchwall=wall
//   If (searchstat == 1)  CEILING     searchsector=sector
//   If (searchstat == 2)  FLOOR       searchsector=sector
//   If (searchstat == 3)  SPRITE      searchsector=sector, searchwall=sprite
//   If (searchstat == 4)  MASKED WALL searchsector=sector, searchwall=wall
//
//   searchsector is the sector of the selected item for all 5 searchstat's
//
//   searchwall is undefined if searchstat is 1 or 2
//   searchwall is the wall if searchstat = 0 or 4
//   searchwall is the sprite if searchstat = 3 (Yeah, I know - it says wall,
//                                      but trust me, it's the sprite number)

void
ResetKeys(void)
{
    unsigned i;

    for (i = 0; i < SIZ(keystatus); i++)
    {
        KEY_PRESSED(i) = 0;
    }
}

void ExtPreCheckKeys(void)
{
    ToggleFAF();
}


// Toggle sprites on/off.  Good for frame rate checks.
SWBOOL DebugActorFreeze = 0;

void
ToggleSprites()
{
    spritetype *tspr;


    DebugActorFreeze++;
    if (DebugActorFreeze > 2)
        DebugActorFreeze = 0;

    // Don't show any sprites, period
    if (DebugActorFreeze == 2)
    {
        short i;

        for (i = 0, tspr = &sprite[0]; i < MAXSPRITES; i++, tspr++)
        {
            SET(tspr->cstat, CSTAT_SPRITE_INVISIBLE);
//                if (TEST(tspr->cstat, CSTAT_SPRITE_BLOCK))
//                {
//                    SET(tspr->extra, SPRX_BLOCK);
//                    RESET(tspr->cstat, CSTAT_SPRITE_BLOCK);
//                }
        }
    }


    // Show all sprites except actors and ST's
    if (DebugActorFreeze == 1)
    {
        short i;

        for (i = 0, tspr = &sprite[0]; i < MAXSPRITES; i++, tspr++)
        {
            switch (tspr->picnum)
            {
            case COOLIE_RUN_R0:
            case ZOMBIE_RUN_R0:
            case NINJA_RUN_R0:
            case SERP_RUN_R0:
            case LAVA_RUN_R0:
            case SKEL_RUN_R0:
            case GORO_RUN_R0:
            case HORNET_RUN_R0:
            case SKULL_R0:
            case RIPPER_RUN_R0:
            case 2307:                  // ST1
            case 2308:                  // ST2
            case 2309:                  // QJ
            case 2310:                  // QJD
            case 2311:                  // QSJ
            case 2312:                  // QSCN
            case 2313:                  // QEXIT
                SET(tspr->cstat, CSTAT_SPRITE_INVISIBLE);
//                      if (TEST(tspr->cstat, CSTAT_SPRITE_BLOCK))
//                      {
//                          SET(tspr->extra, SPRX_BLOCK);
//                          RESET(tspr->cstat, CSTAT_SPRITE_BLOCK);
//                      }
                break;
            }
        }
    }


    // Show all sprites
    if (DebugActorFreeze == FALSE)
    {
        short i;

        for (i = 0, tspr = &sprite[0]; i < MAXSPRITES; i++, tspr++)
        {
            RESET(tspr->cstat, CSTAT_SPRITE_INVISIBLE);
//                if (TEST(tspr->extra, SPRX_BLOCK))
//                    SET(tspr->cstat, CSTAT_SPRITE_BLOCK);
        }
    }
}


void
DoAutoSize(uspritetype *tspr)
{
    short i;

    if (!bAutoSize)
        return;

    switch (tspr->picnum)
    {
    case ICON_STAR:                     // 1793
        break;
    case ICON_UZI:                      // 1797
        tspr->xrepeat = 43;
        tspr->yrepeat = 40;
        break;
    case ICON_UZIFLOOR:         // 1807
        tspr->xrepeat = 43;
        tspr->yrepeat = 40;
        break;
    case ICON_LG_UZI_AMMO:              // 1799
        break;
    case ICON_HEART:                    // 1824
        break;
    case ICON_HEART_LG_AMMO:            // 1820
        break;
    case ICON_GUARD_HEAD:               // 1814
        break;
    case ICON_FIREBALL_LG_AMMO: // 3035
        break;
    case ICON_ROCKET:                   // 1843
        break;
    case ICON_SHOTGUN:                  // 1794
        tspr->xrepeat = 57;
        tspr->yrepeat = 58;
        break;
    case ICON_LG_ROCKET:                // 1796
        break;
    case ICON_LG_SHOTSHELL:             // 1823
        break;
    case ICON_MICRO_GUN:                // 1818
        break;
    case ICON_MICRO_BATTERY:            // 1800
        break;
    case ICON_GRENADE_LAUNCHER: // 1817
        tspr->xrepeat = 54;
        tspr->yrepeat = 52;
        break;
    case ICON_LG_GRENADE:               // 1831
        break;
    case ICON_LG_MINE:                  // 1842
        break;
    case ICON_RAIL_GUN:         // 1811
        tspr->xrepeat = 50;
        tspr->yrepeat = 54;
        break;
    case ICON_RAIL_AMMO:                // 1812
        break;
    case ICON_SM_MEDKIT:                // 1802
        break;
    case ICON_MEDKIT:                   // 1803
        break;
    case ICON_CHEMBOMB:
        tspr->xrepeat = 64;
        tspr->yrepeat = 47;
        break;
    case ICON_FLASHBOMB:
        tspr->xrepeat = 32;
        tspr->yrepeat = 34;
        break;
    case ICON_NUKE:
        break;
    case ICON_CALTROPS:
        tspr->xrepeat = 37;
        tspr->yrepeat = 30;
        break;
    case ICON_BOOSTER:                  // 1810
        tspr->xrepeat = 30;
        tspr->yrepeat = 38;
        break;
    case ICON_HEAT_CARD:                // 1819
        tspr->xrepeat = 46;
        tspr->yrepeat = 47;
        break;
    case ICON_REPAIR_KIT:               // 1813
        break;
    case ICON_EXPLOSIVE_BOX:            // 1801
        break;
    case ICON_ENVIRON_SUIT:             // 1837
        break;
    case ICON_FLY:                      // 1782
        break;
    case ICON_CLOAK:                    // 1826
        break;
    case ICON_NIGHT_VISION:             // 3031
        tspr->xrepeat = 59;
        tspr->yrepeat = 71;
        break;
    case ICON_NAPALM:                   // 3046
        break;
    case ICON_RING:                     // 3050
        break;
    case ICON_RINGAMMO:         // 3054
        break;
    case ICON_NAPALMAMMO:               // 3058
        break;
    case ICON_GRENADE:                  // 3059
        break;
    case ICON_ARMOR:                    // 3030
        tspr->xrepeat = 82;
        tspr->yrepeat = 84;
        break;
    case BLUE_KEY:                      // 1766
        break;
    case RED_KEY:                       // 1770
        break;
    case GREEN_KEY:                     // 1774
        break;
    case YELLOW_KEY:                    // 1778
        break;
    case BLUE_CARD:
    case RED_CARD:
    case GREEN_CARD:
    case YELLOW_CARD:
        tspr->xrepeat = 36;
        tspr->yrepeat = 33;
        break;
    case GOLD_SKELKEY:
    case SILVER_SKELKEY:
    case BRONZE_SKELKEY:
    case RED_SKELKEY:
        tspr->xrepeat = 39;
        tspr->yrepeat = 45;
        break;
    case SKEL_LOCKED:
    case SKEL_UNLOCKED:
        tspr->xrepeat = 47;
        tspr->yrepeat = 40;
        break;
    case RAMCARD_LOCKED:
    case RAMCARD_UNLOCKED:
    case CARD_LOCKED:
    case CARD_UNLOCKED:
        break;
    default:
        break;
    }
}

// Rotation angles for sprites
short rotang = 0;

void
ExtAnalyzeSprites(int32_t ourx, int32_t oury, int32_t oura, int32_t smoothr)
{
    int i, currsprite;
    uspritetype *tspr;

    UNREFERENCED_PARAMETER(ourx);
    UNREFERENCED_PARAMETER(oury);
    UNREFERENCED_PARAMETER(oura);
    UNREFERENCED_PARAMETER(smoothr);

    rotang += 4;
    if (rotang > 2047)
        rotang = 0;

    for (i = 0, tspr = &tsprite[0]; i < spritesortcnt; i++, tspr++)
    {

        // Take care of autosizing
        DoAutoSize(tspr);

        tspr->shade += 6;
        if (sector[tspr->sectnum].ceilingstat & 1)
            tspr->shade += sector[tspr->sectnum].ceilingshade;
        else
            tspr->shade += sector[tspr->sectnum].floorshade;

        if (tspr->picnum == ICON_ARMOR)
        {
            if (tspr->pal != 19)        // Red
                tspr->pal = 17;         // Gray
        }

        // Check for voxels
        if (bVoxelsOn)
        {
            if (bSpinBobVoxels)
            {
                switch (tspr->picnum)
                {
                case ICON_STAR: // 1793
                case ICON_UZI:          // 1797
                case ICON_UZIFLOOR:     // 1807
                case ICON_LG_UZI_AMMO:  // 1799
                case ICON_HEART:        // 1824
                case ICON_HEART_LG_AMMO:        // 1820
                case ICON_GUARD_HEAD:   // 1814
                case ICON_FIREBALL_LG_AMMO:     // 3035
                case ICON_ROCKET:       // 1843
                case ICON_SHOTGUN:      // 1794
                case ICON_LG_ROCKET:    // 1796
                case ICON_LG_SHOTSHELL: // 1823
                case ICON_MICRO_GUN:    // 1818
                case ICON_MICRO_BATTERY:        // 1800
                case ICON_GRENADE_LAUNCHER:     // 1817
                case ICON_LG_GRENADE:   // 1831
                case ICON_LG_MINE:      // 1842
                case ICON_RAIL_GUN:     // 1811
                case ICON_RAIL_AMMO:    // 1812
                case ICON_SM_MEDKIT:    // 1802
                case ICON_MEDKIT:       // 1803
                case ICON_BOOSTER:      // 1810
                case ICON_HEAT_CARD:    // 1819
                case ICON_REPAIR_KIT:   // 1813
                case ICON_EXPLOSIVE_BOX:        // 1801
                case ICON_ENVIRON_SUIT: // 1837
                case ICON_FLY:          // 1782
                case ICON_CLOAK:        // 1826
                case ICON_NIGHT_VISION: // 3031
                case ICON_NAPALM:       // 3046
                case ICON_RING: // 3050
                // case ICON_GOROAMMO:       // 3035
                // case ICON_HEARTAMMO:      // 1820
                case ICON_RINGAMMO:     // 3054
                case ICON_NAPALMAMMO:   // 3058
                case ICON_GRENADE:      // 3059
                // case ICON_OXYGEN:         // 1800
                case ICON_ARMOR:        // 3030
                case BLUE_KEY:          // 1766
                case RED_KEY:           // 1770
                case GREEN_KEY: // 1774
                case YELLOW_KEY:        // 1778
                case ICON_CHEMBOMB:
                case ICON_FLASHBOMB:
                case ICON_NUKE:
                case ICON_CALTROPS:
                    tspr->ang = rotang;
                    // tspr->z+=(sintable[(rotang*2)%2047]/16);
                    break;
                }
            }

            if (aVoxelArray[tspr->picnum] >= 0)
            {

                // Turn on voxels
                tspr->picnum = aVoxelArray[tspr->picnum];       // Get the voxel number
                tspr->cstat |= 48;      // Set stat to voxelize sprite
            }
        }
    }

}

uint8_t*
BKeyPressed(void)
{
    uint8_t* k;

    for (k = (uint8_t*) & KEY_PRESSED(0); k < (uint8_t*) &KEY_PRESSED(SIZ(keystatus)); k++)
    {
        if (*k)
            return k;
    }

    return NULL;
}

uint8_t*
KeyPressedRange(uint8_t* kb, uint8_t* ke)
{
    uint8_t* k;

    for (k = kb; k <= ke; k++)
    {
        if (*k)
            return k;
    }

    return NULL;
}

void
ResetKeyRange(uint8_t* kb, uint8_t* ke)
{
    uint8_t* k;

    for (k = kb; k <= ke; k++)
    {
        *k = 0;
    }
}

#if 0
void
ExtInit(void)
{
    int i, fil;

    initgroupfile(G_GrpFile());
    if ((fil = open("setup.dat", O_BINARY | O_RDWR, S_IREAD)) != -1)
    {
        read(fil, &option[0], NUMOPTIONS);
        read(fil, &default_buildkeys[0], NUMKEYS);
        memcpy((void *) buildkeys, (void *) default_buildkeys, NUMKEYS);     // Trick to make build
        // use setup.dat keys
        close(fil);
    }
    if (option[4] > 0)
        option[4] = 0;
    initmouse();

    initengine();
    vidoption = option[0];
    xdim = vesares[option[6] & 15][0];
    ydim = vesares[option[6] & 15][1];

#if 0
    switch (option[0])
    {
    case 0:
        initengine(0, chainxres[option[6] & 15], chainyres[option[6] >> 4]);
        break;
    case 1:
        initengine(1, vesares[option[6] & 15][0], vesares[option[6] & 15][1]);
        break;
    default:
        initengine(option[0], 320L, 200L);
    }
#endif

    InitPalette();

    kensplayerheight = 58;
    zmode = 0;

}

#endif

const char *startwin_labeltext = "Starting Build Editor for Shadow Warrior...";

const char *ExtGetVer(void)
{
    return s_buildRev;
}

void ExtSetupMapFilename(const char *mapname)
{
    UNREFERENCED_PARAMETER(mapname);
}

int32_t ExtPreInit(int32_t argc,char const * const * argv)
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    OSD_SetLogFile("wangulator.log");
    OSD_SetVersion(AppProperName,0,2);
    initprintf("%s %s\n", AppProperName, s_buildRev);
    PrintBuildInfo();

    return 0;
}

int
ExtInit(void)
{
    int rv = 0;

#ifndef BUILD_DEV_VER
    char ch;

    printf("\n------------------------------------------------------------------------------\n");
    printf("BUILD.EXE for Shadow Warrior\n\n");
    printf("Copyright (c) 1993 - 1997, 3D Realms Entertainment.\n");
    printf("\n");
    printf("IMPORTANT:  This editor and associated tools and utilities are NOT\n");
    printf("shareware and may NOT be freely distributed to any BBS, CD, floppy, or\n");
    printf("any other media.  These tools may NOT be sold or repackaged for sale in\n");
    printf("a commercial product.\n");
    printf("\n");
    printf("Any levels created with these editors and tools may only be used with the\n");
    printf("full (registered) copy of Shadow Warrior, and not the shareware version.\n");
    printf("Please refer to LICENSE.DOC for further information on levels created with\n");
    printf("BUILD.EXE.\n");
    printf("\n");
    printf("Press <Y> if you have read and accepted the terms of LICENSE.DOC,\n");
    printf("or any other key to abort the program. \n");
    printf("\n");
    ch = getch();

    if (ch == 'y' || ch == 'Y')         // if user press Y
    {
#endif


    int i, fil;

    // Store user log in time
    //LogUserTime(TRUE);              // Send true because user is logging
    // in.

#ifdef _WIN32
    if (!access("user_profiles_enabled", F_OK))
#endif
    {
        char cwd[BMAX_PATH];
        char *homedir;
        int asperr;

#if defined(__linux) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
        addsearchpath("/usr/share/games/jfsw");
        addsearchpath("/usr/local/share/games/jfsw");
#elif defined(__APPLE__)
        addsearchpath("/Library/Application Support/JFShadowWarrior");
#endif
        if (getcwd(cwd,BMAX_PATH)) addsearchpath(cwd);
        if ((homedir = Bgethomedir()))
        {
            Bsnprintf(cwd,sizeof(cwd),"%s/"
#if defined(_WIN32)
                      "JFShadowWarrior"
#elif defined(__APPLE__)
                      "Library/Application Support/JFShadowWarrior"
#else
                      ".jfsw"
#endif
                      ,homedir);
            asperr = addsearchpath(cwd);
            if (asperr == -2)
            {
                if (Bmkdir(cwd,S_IRWXU) == 0) asperr = addsearchpath(cwd);
                else asperr = -1;
            }
            if (asperr == 0)
                chdir(cwd);
            free(homedir);
        }
    }

    if (g_grpNamePtr == NULL)
    {
        const char *cp = getenv("SWGRP");
        if (cp)
        {
            clearGrpNamePtr();
            g_grpNamePtr = dup_filename(cp);
            initprintf("Using \"%s\" as main GRP file\n", g_grpNamePtr);
        }
    }
    /*
        if ((fil = open("setup.dat", O_BINARY | O_RDWR, S_IREAD)) != -1)
            {
            read(fil, &option[0], NUMOPTIONS);
            read(fil, &default_buildkeys[0], NUMKEYS);
            memcpy((void *) buildkeys, (void *) default_buildkeys, NUMKEYS); // Trick to make build
            // use setup.dat keys
            close(fil);
            }
        */
    bpp = 8;
    if (loadsetup("build.cfg") < 0) buildputs("Configuration file not found, using defaults.\n"), rv = 1;
    Bmemcpy((void *)buildkeys,(void *)default_buildkeys,NUMBUILDKEYS);       //Trick to make build use setup.dat keys
    if (option[4] > 0)
        option[4] = 0;

    kensplayerheight = 58;
    zmode = 0;

#ifndef BUILD_DEV_VER
}                                   // end user press Y
else
{
    printf("------------------------------------------------------------------------------\n");
    exit(0);
}
#endif
    return rv;
}

int32_t ExtPostStartupWindow(void)
{
    initgroupfile(G_GrpFile());

    if (engineInit())
    {
        wm_msgbox("Build Engine Initialisation Error",
                  "There was a problem initialising the Build engine: %s", engineerrstr);
        return -1;
    }

    InitPalette();
    SW_InitMultiPsky();

    return 0;
}

void ExtPostInit(void)
{
}

void
ExtUnInit(void)
{
    uninitgroupfile();
    writesetup("build.cfg");
    // Store user log in time
    //LogUserTime(FALSE);                 // FALSE means user is logging out
    // now.
}

void
SetSpriteExtra(void)
{
    SPRITEp sp;
    int i;

#define DEFAULT_SKILL 2

    // for (sp = sprite; sp < &sprite[MAXSPRITES]; sp++)
    for (sp = sprite; sp < &sprite[MAXSPRITES]; sp++)
    {
        if (sp->picnum == ST1)
        {
            if (sp->owner == -1)
                sp->owner = 0;
        }
        else
        {
            sp->owner = -1;
        }

        if (sp->extra == -1)
        {
            sp->extra = 0;
            SET(sp->extra, DEFAULT_SKILL);
        }
    }

    // loaded_numwalls is what numwalls is after a load
    // only new walls get their extra's set
    if (loaded_numwalls != numwalls)
    {
        for (i = 0; i < numwalls; i++)
        {
            if (wall[i].extra != 0)
                wall[i].extra = 0;
        }
    }
    loaded_numwalls = numwalls;
}

void
ResetSpriteFound(void)
{
    SPRITEp sp;

    for (sp = sprite; sp < &sprite[MAXSPRITES]; sp++)
    {
        RESET(sp->extra, SPRX_FOUND);
    }
}


// imported from allen code
void
Keys3D(void)
{
    int i;

    // 'PGUP - Move a floor/ceiling sector up 8 pixels.
    if (keystatus[KEYSC_RCTRL] && keystatus[KEYSC_PGUP])
    {
        switch (searchstat)
        {
        case 1:                 // Ceiling
            sector[searchsector].ceilingz -= (1024 * 8);
            keystatus[KEYSC_PGUP] = 0;
            break;
        case 2:                 // Floor
            sector[searchsector].floorz -= (1024 * 8);
            keystatus[KEYSC_PGUP] = 0;
            break;
        }

    }

    // 'PGDN - Move a floor/ceiling sector down 8 pixels.
    if (keystatus[KEYSC_RCTRL] && keystatus[KEYSC_PGDN])
    {
        switch (searchstat)
        {
        case 1:                 // Ceiling
            sector[searchsector].ceilingz += (1024 * 8);
            keystatus[KEYSC_PGDN] = 0;
            break;
        case 2:                 // Floor
            sector[searchsector].floorz += (1024 * 8);
            keystatus[KEYSC_PGDN] = 0;
            break;
        }
    }



    // R - Set relative mode on a floor/ceiling.
    if (!keystatus[KEYSC_ENTER] && !keystatus[KEYSC_QUOTE] && keystatus[KEYSC_R])
    {
        switch (searchstat)
        {
        case 0:
        case 4:                 // Wall
            break;
        case 1:                 // Ceiling
            if (sector[searchsector].ceilingstat & 65)
                Message("Ceiling Relative OFF", M_RED);
            else
                Message("Ceiling Relative ON", M_BLUE);
            break;
        case 2:                 // Floor
            if (sector[searchsector].floorstat & 65)
                Message("Floor Relative OFF", M_RED);
            else
                Message("Floor Relative ON", M_BLUE);
            break;
        case 3:                 // Sprite
            break;
        }
    }

    // '+  = Shade down a floor ceiling while keeping sprites constant
    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_GPLUS])
    {
        SWBOOL dospriteshade = FALSE;

        keystatus[KEYSC_GPLUS] = 0;

        switch (searchstat)
        {
        case 1:
            if (sector[searchsector].ceilingstat & 1)
                dospriteshade = TRUE;
            if (sector[searchsector].ceilingshade > -128)
                sector[searchsector].ceilingshade--;
            break;
        case 2:
            if (!(sector[searchsector].ceilingstat & 1))
                dospriteshade = TRUE;
            if (sector[searchsector].floorshade > -128)
                sector[searchsector].floorshade--;
            break;
        default:
            break;
        }

        if (dospriteshade)
        {
            for (i = 0; i < MAXSPRITES; i++)
            {
                if (sprite[i].sectnum == searchsector)
                {
                    if (sprite[i].shade < 127)
                        sprite[i].shade++;
                }
            }
        }
    }

    // '-  = Shade down a floor ceiling while keeping sprites constant
    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_GMINUS])
    {
        SWBOOL dospriteshade = FALSE;

        keystatus[KEYSC_GMINUS] = 0;

        switch (searchstat)
        {
        case 1:
            if (sector[searchsector].ceilingstat & 1)
                dospriteshade = TRUE;
            if (sector[searchsector].ceilingshade < 127)
                sector[searchsector].ceilingshade++;
            break;
        case 2:
            if (!(sector[searchsector].ceilingstat & 1))
                dospriteshade = TRUE;
            if (sector[searchsector].floorshade < 127)
                sector[searchsector].floorshade++;
            break;
        default:
            break;
        }

        if (dospriteshade)
        {
            for (i = 0; i < MAXSPRITES; i++)
            {
                if (sprite[i].sectnum == searchsector)
                {
                    if (sprite[i].shade > -128)
                        sprite[i].shade--;
                }
            }
        }
    }


    // 'ENTER - Copies only the bitmap in the copy buffer to
    // wall/ceiling/floor/sprite,
    // whatever the cursor is pointing to. Does not copy x/y repeats etc.,
    // like ENTER does.
    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_ENTER])       // ' ENTER
    {
        printext256(0, 0, 15, 0, "Put Graphic ONLY", 0);
        keystatus[KEYSC_ENTER] = 0;
        switch (searchstat)
        {
        case 0:
            wall[searchwall].picnum = temppicnum;
            break;
        case 1:
            sector[searchsector].ceilingpicnum = temppicnum;
            break;
        case 2:
            sector[searchsector].floorpicnum = temppicnum;
            break;
        case 3:
            sprite[searchwall].picnum = temppicnum;
            break;
        case 4:
            wall[searchwall].overpicnum = temppicnum;
            break;
        }
    }

    // ;S
    if (keystatus[KEYSC_SEMI] && keystatus[KEYSC_S])    // ; S
    {
        keystatus[KEYSC_S] = 0;
        switch (searchstat)
        {

        case 0:
        case 4:
            for (i = 0; i < MAXWALLS; i++)
            {
                if (wall[i].picnum == temppicnum)
                {
                    wall[i].shade = tempshade;
                }
            }
            break;
        case 1:
        case 2:
            for (i = 0; i < MAXSECTORS; i++)
            {
                if (searchstat)
                    if (sector[i].ceilingpicnum == temppicnum)
                    {
                        sector[i].ceilingshade = tempshade;
                    }
                if (searchstat == 2)
                    if (sector[i].floorpicnum == temppicnum)
                    {
                        sector[i].floorshade = tempshade;
                    }
            }
            break;
        case 3:
            for (i = 0; i < MAXSPRITES; i++)
            {
                if (sprite[i].picnum == temppicnum)
                {
                    sprite[i].shade = tempshade;
                }
            }
            break;
        }
    }

    // 'C - Does a global tile replacement using bitmap only, no x/y repeat,
    // etc....
    // Works for walls, sectors, or sprites.
    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_C])   // ' C
    {
        short searchpicnum = 0;
        short w, start_wall, end_wall, currsector;

        keystatus[KEYSC_C] = 0;

        switch (searchstat)
        {

        case 0:
        case 4:
            searchpicnum = wall[searchwall].picnum;
            if (highlightsectorcnt <= 0)
            {
                for (i = 0; i < MAXWALLS; i++)
                {
                    if (wall[i].picnum == searchpicnum)
                    {
                        wall[i].picnum = temppicnum;
                    }
                }
            }
            else
            {
                for (i = 0; i < highlightsectorcnt; i++)
                {
                    currsector = highlightsector[i];
                    start_wall = sector[currsector].wallptr;
                    end_wall = start_wall + sector[currsector].wallnum;

                    for (w = start_wall; w < end_wall; w++)
                    {
                        if (wall[w].picnum == searchpicnum)
                            wall[w].picnum = temppicnum;
                    }
                }
            }
            break;
        case 1:
            if (highlightsectorcnt <= 0)
            {
                searchpicnum = sector[searchsector].ceilingpicnum;
                for (i = 0; i < MAXSECTORS; i++)
                {
                    if (sector[i].ceilingpicnum == searchpicnum)
                    {
                        sector[i].ceilingpicnum = temppicnum;
                    }
                }
            }
            else
            {
                for (i = 0; i < highlightsectorcnt; i++)
                {
                    currsector = highlightsector[i];

                    if (sector[currsector].ceilingpicnum == searchpicnum)
                        sector[currsector].ceilingpicnum = temppicnum;
                }
            }
            break;
        case 2:
            searchpicnum = sector[searchsector].floorpicnum;
            if (highlightsectorcnt <= 0)
            {
                for (i = 0; i < MAXSECTORS; i++)
                {
                    if (sector[i].floorpicnum == searchpicnum)
                    {
                        sector[i].floorpicnum = temppicnum;
                    }
                }
            }
            else
            {
                for (i = 0; i < highlightsectorcnt; i++)
                {
                    currsector = highlightsector[i];

                    if (sector[currsector].floorpicnum == searchpicnum)
                        sector[currsector].floorpicnum = temppicnum;
                }
            }
            break;
        case 3:
            searchpicnum = sprite[searchwall].picnum;

            if (highlightsectorcnt <= 0)
            {
                for (i = 0; i < MAXSPRITES; i++)
                {
                    if (sprite[i].picnum == searchpicnum)
                        sprite[i].picnum = temppicnum;
                }
            }
            break;
        default:
            break;
        }
    }

    // 'T - Set's the low tag of a wall/sector/sprite.
    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_T])   // ' T
    {
        keystatus[KEYSC_T] = 0;
        switch (searchstat)
        {
        case 0:
        case 4:
            strcpy(tempbuf, "Wall lotag: ");
            wall[searchwall].lotag =
                getnumber256(tempbuf, wall[searchwall].lotag, 65536L, 1);
            break;
        case 1:
        case 2:
            strcpy(tempbuf, "Sector lotag: ");
            sector[searchsector].lotag =
                getnumber256(tempbuf, sector[searchsector].lotag, 65536L, 1);
            break;
        case 3:
            strcpy(tempbuf, "Sprite lotag: ");
            sprite[searchwall].lotag =
                getnumber256(tempbuf, sprite[searchwall].lotag, 65536L, 1);
            // Find the next lotag
            if (sprite[searchwall].picnum == ST1)
            {
                FindNextTag();
                ShowNextTag();
            }

            break;
        }
    }

    // 'H - Sets the high tag of a wall/sector/sprite.
    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_H])   // ' H
    {
        keystatus[KEYSC_H] = 0;
        switch (searchstat)
        {
        case 0:
        case 4:
            strcpy(tempbuf, "Wall hitag: ");
            wall[searchwall].hitag =
                getnumber256(tempbuf, wall[searchwall].hitag, 65536L, 1);
            break;
        case 1:
        case 2:
            strcpy(tempbuf, "Sector hitag: ");
            sector[searchsector].hitag =
                getnumber256(tempbuf, sector[searchsector].hitag, 65536L, 1);
            break;
        case 3:
            strcpy(tempbuf, "Sprite hitag: ");
            sprite[searchwall].hitag =
                getnumber256(tempbuf, sprite[searchwall].hitag, 65536L, 1);
            break;
        }
    }

    // 'S - Sets the shade of a wall/sector/sprite using an entered input
    // value
    // between 0-65536.
    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_S])   // ' S
    {
        keystatus[KEYSC_S] = 0;
        switch (searchstat)
        {
        case 0:
        case 4:
            strcpy(tempbuf, "Wall shade: ");
            wall[searchwall].shade =
                getnumber256(tempbuf, wall[searchwall].shade, 65536L, 1);
            break;
        case 1:
        case 2:
            strcpy(tempbuf, "Sector shade: ");
            if (searchstat == 1)
                sector[searchsector].ceilingshade =
                    getnumber256(tempbuf, sector[searchsector].ceilingshade, 65536L, 1);
            if (searchstat == 2)
                sector[searchsector].floorshade =
                    getnumber256(tempbuf, sector[searchsector].floorshade, 65536L, 1);
            break;
        case 3:
            strcpy(tempbuf, "Sprite shade: ");
            sprite[searchwall].shade =
                getnumber256(tempbuf, sprite[searchwall].shade, 65536L, 1);
            break;
        }
    }

    // 'V - Sets sector visibility on a sector using an input value between
    // 0-65536.
    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_V])   // ' V
    {
        keystatus[KEYSC_V] = 0;
        switch (searchstat)
        {
        case 1:
        case 2:
            strcpy(tempbuf, "Sector visibility: ");
            sector[searchsector].visibility =
                getnumber256(tempbuf, sector[searchsector].visibility, 65536L, 1);
            break;
        }
    }

    // 'X - Toggles voxel sprites on/off
    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_X])   // ' X
    {
        keystatus[KEYSC_X] = 0;

        bVoxelsOn = !bVoxelsOn;
    }

    // 'Z - Toggles voxel rotation on/off
    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_Z])   // ' Z
    {
        keystatus[KEYSC_Z] = 0;

        bSpinBobVoxels = !bSpinBobVoxels;
    }

    // 'A - Toggles sprite autosizing on/off
    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_A])   // ' A
    {
        keystatus[KEYSC_A] = 0;

        bAutoSize = !bAutoSize;
    }

    // 'M - Toggles sprites on/off
    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_M])   // ' M
    {
        keystatus[KEYSC_M] = 0;

        ToggleSprites();
    }

    // 'P - Will copy palette to all sectors highlighted with R-Alt key
    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_P])   // ' P
    {
        short w, start_wall, end_wall, currsector;

        keystatus[KEYSC_P] = 0;

        for (i = 0; i < highlightsectorcnt; i++)
        {
            currsector = highlightsector[i];
            sector[currsector].ceilingpal = temppal;
            sector[currsector].floorpal = temppal;
            // Do all the walls in the sector
            start_wall = sector[currsector].wallptr;
            end_wall = start_wall + sector[currsector].wallnum;

            for (w = start_wall; w < end_wall; w++)
            {
                wall[w].pal = temppal;
            }
        }
    }

    // ;P - Will copy palette to all sectors highlighted with R-Alt key
    if (keystatus[KEYSC_SEMI] && keystatus[KEYSC_P])   // ; P
    {
        short w, start_wall, end_wall, currsector;

        keystatus[KEYSC_P] = 0;

        for (i = 0; i < highlightsectorcnt; i++)
        {
            currsector = highlightsector[i];

            if ((sector[currsector].ceilingpal == temppal && temppal != 0) || (temppal == 0 && sector[currsector].ceilingpal != 0))
                sector[currsector].ceilingshade = 127;
            if ((sector[currsector].floorpal == temppal && temppal != 0) || (temppal == 0 && sector[currsector].ceilingpal != 0))
                sector[currsector].floorshade = 127;

            // Do all the walls in the sector
            start_wall = sector[currsector].wallptr;
            end_wall = start_wall + sector[currsector].wallnum;

            for (w = start_wall; w < end_wall; w++)
            {
                if ((wall[w].pal == temppal && temppal != 0) || (temppal == 0 && wall[w].pal != 0))
                    wall[w].shade = 127;
            }
        }
    }

    ShowMessage();
}                                   // end Keys3D()

// Used to help print out the item status list
void
PrintStatus(const char *string, int num, char x, char y, char color)
{
    sprintf(tempbuf, "%s %d", string, num);
    printext16(x * 8, ydim16+y * 8, color, -1, tempbuf, 0);
}


static int32_t sw_getnumber256(const char *namestart, int32_t num, int32_t maxnumber, char sign)
{
    return _getnumber256(namestart, num, maxnumber, sign, NULL);
}
static int32_t sw_getnumber16(const char *namestart, int32_t num, int32_t maxnumber, char sign)
{
    return _getnumber16(namestart, num, maxnumber, sign, NULL);
}
static void sw_printmessage256(const char *text)
{
    printmessage256(0, 0, text);
}
static void sw_printmessage16(const char *text)
{
    lastpm16time = (int32_t) totalclock;
    _printmessage16("%s", text);
}

void
MoreKeys(short searchstat, short searchwall, short searchsector, short pointhighlight)
{

    typedef int32_t GET_NUM_FUNC (const char *, int32_t, int32_t, char);
    typedef GET_NUM_FUNC *GET_NUM_FUNCp;
    typedef void PRINT_MSG_FUNC (const char *);
    typedef PRINT_MSG_FUNC *PRINT_MSG_FUNCp;
    SPRITEp sp;

    int i;
    int value;

    GET_NUM_FUNCp getnumber;
    PRINT_MSG_FUNCp printmessage;

    if (qsetmode == MODE_3D)
    {
        getnumber = sw_getnumber256;
        printmessage = sw_printmessage256;
    }
    else
    {
        getnumber = sw_getnumber16;
        printmessage = sw_printmessage16;

        if (TEST(pointhighlight, SPRITE_FLAG))
        {
            searchstat = 3;
            searchwall = RESET(pointhighlight, SPRITE_FLAG);
        }
        else
        {
            // for now make search stat invalid
            searchstat = 5;
        }
    }

    sp = &sprite[searchwall];



    if (KEY_PRESSED(KEYSC_RALT) && KEY_PRESSED(KEYSC_RCTRL))
    {
        if (KEY_PRESSED(KEYSC_KPMINUS))
        {
            KEY_PRESSED(KEYSC_KPMINUS) = 0;
            g_visibility = g_visibility - (g_visibility >> 3);

            if (g_visibility < 0)
                g_visibility = 16348;
        }
        else if (KEY_PRESSED(KEYSC_KPPLUS))
        {
            KEY_PRESSED(KEYSC_KPPLUS) = 0;
            g_visibility = g_visibility + (g_visibility >> 3);

            if (g_visibility > 16348)
                g_visibility = 0;
        }
    }


    if (keystatus[KEYSC_QUOTE])
    {

        if (keystatus[KEYSC_K])   // ' K
        {
            short data;
            SPRITEp sp = &sprite[searchwall];

            keystatus[KEYSC_K] = 0;
            switch (searchstat)
            {
            case 3:
                data = TEST(sp->extra, SPRX_SKILL);

                //data = getnumber256(tempbuf, data, 65536L);
                data++; // Toggle

                if (data > 3)
                    data = 0;

                RESET(sp->extra, SPRX_SKILL);
                SET(sp->extra, data);
                break;
            }
        }

        if (keystatus[KEYSC_RSHIFT] || keystatus[KEYSC_LSHIFT])
        {
            if (keystatus[KEYSC_1])
            {
                keystatus[KEYSC_1] = 0;
                keystatus[KEYSC_SEMI] = 0;

                switch (searchstat)
                {
                case 3:
                    sprintf(tempbuf, "Sprite tag 11 (shade) (snum = %d): ", searchwall);
                    SPRITE_TAG11(searchwall) =
                        getnumber(tempbuf, SPRITE_TAG11(searchwall), 65536L, 0);
                    break;
                }

                printmessage(" ");
            }

            if (keystatus[KEYSC_2])
            {
                keystatus[KEYSC_2] = 0;
                keystatus[KEYSC_SEMI] = 0;

                switch (searchstat)
                {
                case 3:
                    strcpy(tempbuf, "Sprite tag 12 (pal): ");
                    SPRITE_TAG12(searchwall) =
                        getnumber(tempbuf, SPRITE_TAG12(searchwall), 65536L, 0);
                    break;
                }

                printmessage(" ");
            }

            if (keystatus[KEYSC_3])
            {
                keystatus[KEYSC_3] = 0;

                switch (searchstat)
                {
                case 3:
                    strcpy(tempbuf, "Sprite tag 13 (xoffset/yoffset): ");
                    i = getnumber(tempbuf, SPRITE_TAG13(searchwall), 65536L, 0);
                    SET_SPRITE_TAG13(searchwall, i);
                    break;
                }

                printmessage(" ");
            }

            if (keystatus[KEYSC_4])
            {
                keystatus[KEYSC_4] = 0;

                switch (searchstat)
                {
                case 3:
                    strcpy(tempbuf, "Sprite tag 14 (xrepeat/yrepeat): ");
                    i = getnumber(tempbuf, SPRITE_TAG14(searchwall), 65536L, 0);
                    SET_SPRITE_TAG14(searchwall, i);
                    break;
                }

                printmessage(" ");
            }

            if (keystatus[KEYSC_5])
            {
                keystatus[KEYSC_5] = 0;

                switch (searchstat)
                {
                case 3:
                    strcpy(tempbuf, "Sprite tag 15 (z): ");
                    SPRITE_TAG15(searchwall) =
                        getnumber(tempbuf, SPRITE_TAG15(searchwall), 65536L, 0);
                    break;
                }

                printmessage(" ");
            }
        }

        if (keystatus[KEYSC_1])
        {
            keystatus[KEYSC_1] = 0;

            switch (searchstat)
            {
            case 0:
            case 4:
                strcpy(tempbuf, "Wall tag 1 (hitag): ");
                wall[searchwall].hitag =
                    getnumber(tempbuf, wall[searchwall].hitag, 65536L, 0);
                break;
            case 1:
            case 2:
                strcpy(tempbuf, "Sector tag 1 (hitag): ");
                sector[searchsector].hitag =
                    getnumber(tempbuf, sector[searchsector].hitag, 65536L, 0);
                break;
            case 3:
                strcpy(tempbuf, "Sprite tag 1 (hitag): ");
                SPRITE_TAG1(searchwall) =
                    getnumber(tempbuf, SPRITE_TAG1(searchwall), 65536L, 0);
                break;
            }

            printmessage(" ");
        }

        if (keystatus[KEYSC_2])
        {
            keystatus[KEYSC_2] = 0;

            switch (searchstat)
            {
            case 0:
            case 4:
                strcpy(tempbuf, "Wall tag 2 (lotag): ");
                wall[searchwall].lotag =
                    getnumber(tempbuf, wall[searchwall].lotag, 65536L, 0);
                break;
            case 1:
            case 2:
                strcpy(tempbuf, "Sector tag 2 (lotag): ");
                sector[searchsector].lotag =
                    getnumber(tempbuf, sector[searchsector].lotag, 65536L, 0);
                break;
            case 3:
                strcpy(tempbuf, "Sprite tag 2 (lotag): ");
                SPRITE_TAG2(searchwall) =
                    getnumber(tempbuf, SPRITE_TAG2(searchwall), 65536L, 0);
                // Find the next lotag
                if (sprite[searchwall].picnum == ST1)
                {
                    FindNextTag();
                    ShowNextTag();
                }
                break;
            }
            printmessage(" ");
        }

        if (keystatus[KEYSC_3])
        {
            keystatus[KEYSC_3] = 0;

            switch (searchstat)
            {
            case 0:
            case 4:
                strcpy(tempbuf, "Wall tag 3 (xpanning): ");
                wall[searchwall].xpanning =
                    getnumber(tempbuf, wall[searchwall].xpanning, 65536L, 0);
                break;
            case 1:
            case 2:
                strcpy(tempbuf, "Sector tag 3 (ceilingxpanning): ");
                sector[searchsector].ceilingxpanning =
                    getnumber(tempbuf, sector[searchsector].ceilingxpanning, 65536L, 0);
                break;
            case 3:
                strcpy(tempbuf, "Sprite tag 3 (clipdist) : ");
                SPRITE_TAG3(searchwall) =
                    getnumber(tempbuf, SPRITE_TAG3(searchwall), 65536L, 0);

                break;
            }
            printmessage(" ");
        }

        if (keystatus[KEYSC_4])
        {
            keystatus[KEYSC_4] = 0;

            switch (searchstat)
            {
            case 0:
            case 4:
                strcpy(tempbuf, "Wall tag 4 (ypanning): ");
                wall[searchwall].ypanning =
                    getnumber(tempbuf, wall[searchwall].ypanning, 65536L, 0);
                break;
            case 1:
            case 2:
                strcpy(tempbuf, "Sector tag 4 (ceilingypanning): ");
                sector[searchsector].ceilingypanning =
                    getnumber(tempbuf, sector[searchsector].ceilingypanning, 65536L, 0);
                break;
            case 3:
                strcpy(tempbuf, "Sprite tag 4 (ang) : ");
                SPRITE_TAG4(searchwall) =
                    getnumber(tempbuf, SPRITE_TAG4(searchwall), 65536L, 0);
                break;
            }
            printmessage(" ");
        }

        if (keystatus[KEYSC_5])
        {
            keystatus[KEYSC_5] = 0;

            switch (searchstat)
            {
            case 0:
            case 4:
                break;
            case 1:
            case 2:
                strcpy(tempbuf, "Sector tag 5 (floorxpanning): ");
                sector[searchsector].floorxpanning =
                    getnumber(tempbuf, sector[searchsector].floorxpanning, 65536L, 0);
                break;
            case 3:
                strcpy(tempbuf, "Sprite tag 5 (xvel) : ");
                SPRITE_TAG5(searchwall) =
                    getnumber(tempbuf, SPRITE_TAG5(searchwall), 65536L, 0);
                break;
            }
            printmessage(" ");
        }

        if (keystatus[KEYSC_6])
        {
            keystatus[KEYSC_6] = 0;

            switch (searchstat)
            {
            case 0:
            case 4:
                break;
            case 1:
            case 2:
                strcpy(tempbuf, "Sector tag 6 (floorypanning): ");
                sector[searchsector].floorypanning =
                    getnumber(tempbuf, sector[searchsector].floorypanning, 65536L, 0);
                break;
            case 3:
                strcpy(tempbuf, "Sprite tag 6 (yvel) : ");
                SPRITE_TAG6(searchwall) =
                    getnumber(tempbuf, SPRITE_TAG6(searchwall), 65536L, 0);
                break;
            }
            printmessage(" ");
        }

        if (keystatus[KEYSC_7])
        {
            keystatus[KEYSC_7] = 0;

            switch (searchstat)
            {
            case 0:
            case 4:
                break;
            case 1:
            case 2:
                strcpy(tempbuf, "Sector tag 7 (floorypanning): ");
                sector[searchsector].floorypanning =
                    getnumber(tempbuf, sector[searchsector].floorypanning, 65536L, 0);
                break;
            case 3:
                strcpy(tempbuf, "Sprite tag 7 (zvel 1) <0-255> : ");
                SPRITE_TAG7(searchwall) =
                    getnumber(tempbuf, SPRITE_TAG7(searchwall), 65536L, 0);
                break;
            }
            printmessage(" ");
        }

        if (keystatus[KEYSC_8])
        {
            keystatus[KEYSC_8] = 0;

            switch (searchstat)
            {
            case 0:
            case 4:
                break;
            case 1:
            case 2:
                break;
            case 3:
                strcpy(tempbuf, "Sprite tag 8 (zvel 2) <0-255> : ");
                SPRITE_TAG8(searchwall) =
                    getnumber(tempbuf, SPRITE_TAG8(searchwall), 65536L, 0);
                break;
            }
            printmessage(" ");
        }

        if (keystatus[KEYSC_9])
        {
            keystatus[KEYSC_9] = 0;

            switch (searchstat)
            {
            case 0:
            case 4:
                break;
            case 1:
            case 2:
                break;
            case 3:
                strcpy(tempbuf, "Sprite tag 9 (owner 1) <0-255> : ");
                SPRITE_TAG9(searchwall) =
                    getnumber(tempbuf, SPRITE_TAG9(searchwall), 65536L, 0);
                break;
            }
            printmessage(" ");
        }

        if (keystatus[KEYSC_0])
        {
            keystatus[KEYSC_0] = 0;

            switch (searchstat)
            {
            case 0:
            case 4:
                break;
            case 1:
            case 2:
                break;
            case 3:
                strcpy(tempbuf, "Sprite tag 10 (owner 2) <0-255> : ");
                SPRITE_TAG10(searchwall) =
                    getnumber(tempbuf, SPRITE_TAG10(searchwall), 65536L, 0);
                break;
            }
            printmessage(" ");
        }

    }

    if (!keystatus[KEYSC_SEMI])
        return;

    if (keystatus[KEYSC_RSHIFT] || keystatus[KEYSC_LSHIFT])
    {
        if (keystatus[KEYSC_1])
        {
            keystatus[KEYSC_1] = 0;

            switch (searchstat)
            {
            case 3:
                strcpy(tempbuf, "Boolean Sprite tag 11 (0 or 1): ");
                value = !!TEST_BOOL11(sp);
                value = getnumber(tempbuf, value, 65536L, 0);

                if (value)
                    SET_BOOL11(sp);
                else
                    RESET_BOOL11(sp);

                break;
            }

            printmessage(" ");
        }
    }
    else
    {
        if (keystatus[KEYSC_1])
        {
            keystatus[KEYSC_1] = 0;

            switch (searchstat)
            {
            case 3:
                strcpy(tempbuf, "Boolean Sprite tag 1 (0 or 1): ");
                value = !!TEST_BOOL1(sp);
                value = getnumber(tempbuf, value, 65536L, 0);

                if (value)
                    SET_BOOL1(sp);
                else
                    RESET_BOOL1(sp);

                break;
            }

            printmessage(" ");
        }

        if (keystatus[KEYSC_2])
        {
            keystatus[KEYSC_2] = 0;

            switch (searchstat)
            {
            case 3:
                strcpy(tempbuf, "Boolean Sprite tag 2 (0 or 1): ");
                value = !!TEST_BOOL2(sp);
                value = getnumber(tempbuf, value, 65536L, 0);

                if (value)
                    SET_BOOL2(sp);
                else
                    RESET_BOOL2(sp);

                break;
            }

            printmessage(" ");
        }


        if (keystatus[KEYSC_3])
        {
            keystatus[KEYSC_3] = 0;

            switch (searchstat)
            {
            case 3:
                strcpy(tempbuf, "Boolean Sprite tag 3 (0 or 1): ");
                value = !!TEST_BOOL3(sp);
                value = getnumber(tempbuf, value, 65536L, 0);

                if (value)
                    SET_BOOL3(sp);
                else
                    RESET_BOOL3(sp);

                break;
            }

            printmessage(" ");
        }

        if (keystatus[KEYSC_4])
        {
            keystatus[KEYSC_4] = 0;

            switch (searchstat)
            {
            case 3:
                strcpy(tempbuf, "Boolean Sprite tag 4 (0 or 1): ");
                value = !!TEST_BOOL4(sp);
                value = getnumber(tempbuf, value, 65536L, 0);

                if (value)
                    SET_BOOL4(sp);
                else
                    RESET_BOOL4(sp);

                break;
            }

            printmessage(" ");
        }

        if (keystatus[KEYSC_5])
        {
            keystatus[KEYSC_5] = 0;

            switch (searchstat)
            {
            case 3:
                strcpy(tempbuf, "Boolean Sprite tag 5 (0 or 1): ");
                value = !!TEST_BOOL5(sp);
                value = getnumber(tempbuf, value, 65536L, 0);

                if (value)
                    SET_BOOL5(sp);
                else
                    RESET_BOOL5(sp);

                break;
            }

            printmessage(" ");
        }

        if (keystatus[KEYSC_6])
        {
            keystatus[KEYSC_6] = 0;

            switch (searchstat)
            {
            case 3:
                strcpy(tempbuf, "Boolean Sprite tag 6 (0 or 1): ");
                value = !!TEST_BOOL6(sp);
                value = getnumber(tempbuf, value, 65536L, 0);

                if (value)
                    SET_BOOL6(sp);
                else
                    RESET_BOOL6(sp);

                break;
            }

            printmessage(" ");
        }

        if (keystatus[KEYSC_7])
        {
            keystatus[KEYSC_7] = 0;

            switch (searchstat)
            {
            case 3:
                strcpy(tempbuf, "Boolean Sprite tag 7 (0 or 1): ");
                value = !!TEST_BOOL7(sp);
                value = getnumber(tempbuf, value, 65536L, 0);

                if (value)
                    SET_BOOL7(sp);
                else
                    RESET_BOOL7(sp);

                break;
            }

            printmessage(" ");
        }

        if (keystatus[KEYSC_8])
        {
            keystatus[KEYSC_8] = 0;

            switch (searchstat)
            {
            case 3:
                strcpy(tempbuf, "Boolean Sprite tag 8 (0 or 1): ");
                value = !!TEST_BOOL8(sp);
                value = getnumber(tempbuf, value, 65536L, 0);

                if (value)
                    SET_BOOL8(sp);
                else
                    RESET_BOOL8(sp);

                break;
            }

            printmessage(" ");
        }

        if (keystatus[KEYSC_9])
        {
            keystatus[KEYSC_9] = 0;

            switch (searchstat)
            {
            case 3:
                strcpy(tempbuf, "Boolean Sprite tag 9 (0 or 1): ");
                value = !!TEST_BOOL9(sp);
                value = getnumber(tempbuf, value, 65536L, 0);

                if (value)
                    SET_BOOL9(sp);
                else
                    RESET_BOOL9(sp);

                break;
            }

            printmessage(" ");
        }

        if (keystatus[KEYSC_0])
        {
            keystatus[KEYSC_0] = 0;

            switch (searchstat)
            {
            case 3:
                strcpy(tempbuf, "Boolean Sprite tag 10 (0 or 1): ");
                value = !!TEST_BOOL10(sp);
                value = getnumber(tempbuf, value, 65536L, 0);

                if (value)
                    SET_BOOL10(sp);
                else
                    RESET_BOOL10(sp);

                break;
            }

            printmessage(" ");
        }
    }
}


#define COINCURSOR 2440

int intro;
void
ExtCheckKeysNotice(void)
{
#if 0
    if (qsetmode == 200)                // In 3D mode
    {
        if (intro < 600)
        {
            intro++;
            sprintf(tempbuf, BUILD_VER_MSG);
            printext256(46 * 8, (24 * 8) - 1, 0, -1, tempbuf, 1);
            printext256(46 * 8, 24 * 8, 15, -1, tempbuf, 1);
            rotatesprite((320 - 8) << 16, (200 - 8) << 16, 64 << 9, 0, COINCURSOR + (((4 - totalclock >> 3)) & 7), 0, 0, 0, 0, 0, xdim - 1, ydim - 1);
        }
    }
#endif
}                                   // end

void
ExtCheckKeys(void)
{
    extern short f_c;

//  int ticdiff=0;

    // Display BUILD notice
    ExtCheckKeysNotice();

    FAF_AfterDrawRooms();

    // try it every time through the loop
    SetSpriteExtra();
    sector[0].extra = g_visibility;

//  if(!BKeyPressed())
//      ticdiff = totalclock-oldtotalclock;  // Difference in tics from last time

//  oldtotalclock = totalclock; // Set old clock to new time

//  slackerclock += ticdiff;
//  ticdiff = 0;            // Set it back to 0!


    if (qsetmode == 200)                // In 3D mode
    {
#define AVERAGEFRAMES 16
        static int frameval[AVERAGEFRAMES], framecnt = 0;
        int i;

        i = (int32_t) totalclock;
        if (i != frameval[framecnt])
        {
            sprintf(tempbuf, "%d", ((120 * AVERAGEFRAMES) / (i - frameval[framecnt])) + f_c);
            printext256(0L, 0L, 1, -1, tempbuf, 1);
            frameval[framecnt] = i;
        }
        framecnt = ((framecnt + 1) & (AVERAGEFRAMES - 1));

    }

    MoreKeys(searchstat, searchwall, searchsector, pointhighlight);

    if (qsetmode == 200)                // In 3D mode
    {
        Keys3D();

        if (KEY_PRESSED(KEYSC_W))
        {
            KEY_PRESSED(KEYSC_W) = 0;

            switch (searchstat)
            {
            case 0:
            case 4:
                wall[searchwall].picnum = temppicnum;
                break;
            case 1:
                sector[searchsector].ceilingpicnum = temppicnum;
                break;
            case 2:
                sector[searchsector].floorpicnum = temppicnum;
                break;
            case 3:
                sprite[searchwall].picnum = temppicnum;
                break;
            }
        }

        // calling this twice seems to speed up the movement
        editinput();
        editinput();
    }
    else
    {

        if (KEY_PRESSED(KEYSC_QUOTE) && KEY_PRESSED(KEYSC_M))
        {
            KEY_PRESSED(KEYSC_M) = FALSE;
            ShadeMenu();
        }

        // greater than & less than keys
//        if (KEY_PRESSED(KEYSC_COMMA) || KEY_PRESSED(KEYSC_PERIOD))
//            {
//            KEY_PRESSED(KEYSC_COMMA) = KEY_PRESSED(KEYSC_PERIOD) = FALSE;
//            }

        // c for clip boxes
        // will find out
        if (KEY_PRESSED(KEYSC_QUOTE) && KEY_PRESSED(KEYSC_V))
        {
            KEY_PRESSED(KEYSC_V) = FALSE;
            CaptionMode++;
            if (CaptionMode >= CAPTION_MAX)
                CaptionMode = 0;
        }

        if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_D])
        {
            keystatus[KEYSC_D] = 0;
            SetClipdist2D();
        }

    }
}

void
ExtLoadMap(const char *mapname)
{
    SPRITEp sp;
    int i;

    BuildStagTable();

    SetSpriteExtra();


#if 0
    Old visibility New visibility
    8->16384
    9->8192
    10->4096
    11->2048
    12->1024
    13->512
    14->256
    15->128
#endif

    // if in valid range set visiblity for the map
    if (sector[0].extra != -1 && sector[0].extra > 0 && sector[0].extra < 16384)
        g_visibility = sector[0].extra;

    else
        // if NOT in valid range set a default
        g_visibility = 2;

    // Load up voxel sprites from voxel script file
    //LoadKVXFromScript("swvoxfil.txt");
    loaded_numwalls = numwalls;
}

void
ExtSaveMap(const char *mapname)
{
    SPRITEp sp;
    int i;

    SetSpriteExtra();
    ResetSpriteFound();

    for (i = 0; i < MAXWALLS; i++)
    {
        if (wall[i].extra != 0)
            wall[i].extra = 0;
    }
}

const char *
ExtGetSectorCaption(short sectnum)
{
    if ((sector[sectnum].lotag | sector[sectnum].hitag) == 0)
    {
        tempbuf[0] = 0;
    }
    else
    {
        sprintf(tempbuf, "%d,%d", TrackerCast(sector[sectnum].hitag),
                TrackerCast(sector[sectnum].lotag));
    }
    return tempbuf;
}

const char *
ExtGetWallCaption(short wallnum)
{
    if ((wall[wallnum].lotag | wall[wallnum].hitag) == 0)
    {
        tempbuf[0] = 0;
    }
    else
    {
        sprintf(tempbuf, "%d,%d", TrackerCast(wall[wallnum].hitag),
                TrackerCast(wall[wallnum].lotag));
    }
    return tempbuf;
}

const char *
ExtGetSpriteCaption(short spritenum)
{
    SPRITEp sp = &sprite[spritenum];
    const char *p = "";
    char name[66];
    char tp[30];
    char multi_str[30] = "";
    int16_t data;

    data = TEST(sp->extra, SPRX_SKILL);

    // Non ST1 sprites that are tagged like them
    if (TEST_BOOL1(sp) && sp->picnum != ST1)
    {
        if ((uint16_t)SP_TAG1(sp) > 1006)
            sprintf(name, "Invalid Tag");
        else
            sprintf(name, "%s,", StagInfo[SP_TAG1(sp)].name);   // This page faults if
        // invalid #

        p = name;
    }
    else
        switch (sp->picnum)
        {
        case ST1:
            if ((uint16_t)SP_TAG1(sp) > 1006)
                sprintf(name, "Invalid Tag");
            else
                sprintf(name, "*%s,", StagInfo[SP_TAG1(sp)].name);
            p = name;
            break;
        case ST_QUICK_JUMP:
            p = "QJ,";
            break;
        case ST_QUICK_JUMP_DOWN:
            p = "QJD,";
            break;
        case ST_QUICK_SUPER_JUMP:
            p = "QSJ,";
            break;
        case ST_QUICK_EXIT:
            p = "QEXIT,";
            break;
        case ST_QUICK_SCAN:
            p = "QSCAN,";
            break;
        case ST_QUICK_OPERATE:
            p = "QOPERATE,";
            break;
        case ST_QUICK_DUCK:
            p = "QDUCK,";
            break;
        case ST_QUICK_DEFEND:
            p = "QDEFEND,";
            break;
        case NINJA_RUN_R0:
            p = "NINJA,";
            break;
        case GORO_RUN_R0:
            p = "GAURD,";
            break;
        case COOLIE_RUN_R0:
            p = "COOLIE,";
            break;
        case COOLG_RUN_R0:
            p = "GHOST,";
            break;
        case RIPPER_RUN_R0:
            p = "RIPPER,";
            break;
        case SKEL_RUN_R0:
            p = "SKEL,";
            break;
        case HORNET_RUN_R0:
            p = "HORNET,";
            break;
        case SERP_RUN_R0:
            p = "SERP,";
            break;
        case SKULL_R0:
            p = "SKULL,";
            break;

        case ICON_STAR:
            p = "STAR,";
            break;
        case ICON_LG_MINE:
            p = "LG_MINE,";
            break;
        case ICON_GRENADE_LAUNCHER:
            p = "GRENADE_LAUNCHER,";
            break;
        case ICON_LG_GRENADE:
            p = "LG_GRENADE,";
            break;
        case ICON_MICRO_GUN:
            p = "MICRO_GUN,";
            break;
        case ICON_MICRO_BATTERY:
            p = "MICRO_BATTERY,";
            break;
        case ICON_SHOTGUN:
            p = "RIOT_GUN,";
            break;
        case ICON_LG_SHOTSHELL:
            p = "LG_SHOTSHELL,";
            break;
        case ICON_ROCKET:
            p = "ROCKET,";
            break;
        case ICON_LG_ROCKET:
            p = "LG_ROCKET,";
            break;
        case ICON_UZI:
            p = "UZI,";
            break;
        case ICON_UZIFLOOR:
            p = "UZI_FLOOR,";
            break;
        case ICON_LG_UZI_AMMO:
            p = "LG_UZI_AMMO,";
            break;
        case ICON_GUARD_HEAD:
            p = "FIRE,";
            break;
        case ICON_HEART:
            p = "HEART,";
            break;
        case ICON_HEART_LG_AMMO:
            p = "HEART_LG_AMMO,";
            break;

        case ICON_SPELL:
            p = "SPELL,";
            break;
        case ICON_EXPLOSIVE_BOX:
            p = "CRATE,";
            break;
        case ICON_SM_MEDKIT:
            p = "FIRST_AID,";
            break;
        case ICON_MEDKIT:
            p = "MEDKIT,";
            break;
        case ICON_CHEMBOMB:
            p = "CHEM_BOMB,";
            break;
        case ICON_FLASHBOMB:
            p = "FLASH_BOMB,";
            break;
        case ICON_NUKE:
            p = "NUKE_BOMB,";
            break;
        case ICON_CALTROPS:
            p = "CALTROPS,";
            break;
        case ICON_BOOSTER:
            p = "BOT_HEALTH,";
            break;
        case ICON_HEAT_CARD:
            p = "HEAT_SEEKERS,";
            break;
//    case ICON_ENVIRON_SUIT:
//        p = "EVIRON_SUIT,";
//        break;
        case ICON_CLOAK:
            p = "SMOKE_BOMB,";
            break;
        case ICON_FLY:
            p = "FLY,";
            break;
        case ICON_GOROAMMO:
            p = "GAURDAMMO,";
            break;
        case ICON_RAIL_GUN:
            p = "RAIL_GUN,";
            break;
        case ICON_RAIL_AMMO:
            p = "RAIL_AMMO,";
            break;
        case ICON_REPAIR_KIT:
            p = "REPAIR_KIT,";
            break;
        /*
         * case ICON_GAUNTAMMO: p = "GAUNTAMMO,"; break;
         */
        case ICON_RINGAMMO:
            p = "RINGAMMO,";
            break;
        case ICON_NAPALMAMMO:
            p = "NAPALMAMMO,";
            break;
        case ICON_GRENADE:
            p = "GRENADE,";
            break;
        case ICON_NIGHT_VISION:
            p = "NIGHT_VISION,";
            break;
        case ICON_ARMOR:
            p = "ARMOR,";
            break;
        case SKEL_LOCKED:
            p = "SKELETON KEY LOCK,";
            switch (sp->hitag)
            {
            case 1:
                sp->pal = PALETTE_PLAYER9;
                break;
            case 2:
                sp->pal = PALETTE_PLAYER7;
                break;
            case 3:
                sp->pal = PALETTE_PLAYER6;
                break;
            case 4:
                sp->pal = PALETTE_PLAYER4;
                break;
            case 5:
                sp->pal = PALETTE_PLAYER4;
                break;
            case 6:
                sp->pal = PALETTE_PLAYER1;
                break;
            case 7:
                sp->pal = PALETTE_PLAYER8;
                break;
            case 8:
                sp->pal = PALETTE_PLAYER9;
                break;
            }
            break;
        case RAMCARD_LOCKED:
            p = "RAM CARD LOCK,";
            switch (sp->hitag)
            {
            case 1:
                sp->pal = PALETTE_PLAYER9;
                break;
            case 2:
                sp->pal = PALETTE_PLAYER7;
                break;
            case 3:
                sp->pal = PALETTE_PLAYER6;
                break;
            case 4:
                sp->pal = PALETTE_PLAYER4;
                break;
            case 5:
                sp->pal = PALETTE_PLAYER4;
                break;
            case 6:
                sp->pal = PALETTE_PLAYER1;
                break;
            case 7:
                sp->pal = PALETTE_PLAYER8;
                break;
            case 8:
                sp->pal = PALETTE_PLAYER9;
                break;
            }
            break;
        case CARD_LOCKED:
            p = "CARD KEY LOCK,";
            switch (sp->hitag)
            {
            case 1:
                sp->pal = PALETTE_PLAYER9;
                break;
            case 2:
                sp->pal = PALETTE_PLAYER7;
                break;
            case 3:
                sp->pal = PALETTE_PLAYER6;
                break;
            case 4:
                sp->pal = PALETTE_PLAYER4;
                break;
            case 5:
                sp->pal = PALETTE_PLAYER4;
                break;
            case 6:
                sp->pal = PALETTE_PLAYER1;
                break;
            case 7:
                sp->pal = PALETTE_PLAYER8;
                break;
            case 8:
                sp->pal = PALETTE_PLAYER9;
                break;
            }
            break;
        case RED_KEY:
            p = "RED KEY,";
            sp->pal = PALETTE_PLAYER9;  // Hard set the palette
            break;
        case BLUE_KEY:
            p = "BLUE KEY,";
            sp->pal = PALETTE_PLAYER7;  // Hard set the palette
            break;
        case GREEN_KEY:
            p = "GREEN KEY,";
            sp->pal = PALETTE_PLAYER6;  // Hard set the palette
            break;
        case YELLOW_KEY:
            p = "YEL KEY,";
            sp->pal = PALETTE_PLAYER4;  // Hard set the palette
            break;
        case RED_CARD:
            p = "RED CARD,";
            sp->pal = PALETTE_PLAYER9;  // Hard set the palette
            break;
        case BLUE_CARD:
            p = "BLUE CARD,";
            sp->pal = PALETTE_PLAYER7;  // Hard set the palette
            break;
        case GREEN_CARD:
            p = "GREEN CARD,";
            sp->pal = PALETTE_PLAYER6;  // Hard set the palette
            break;
        case YELLOW_CARD:
            p = "YEL CARD,";
            sp->pal = PALETTE_PLAYER4;  // Hard set the palette
            break;
        case SILVER_SKELKEY:
            p = "SILVER SKELKEY,";
            sp->pal = PALETTE_PLAYER1;  // Hard set the palette
            break;
        case GOLD_SKELKEY:
            p = "GOLD SKELKEY,";
            sp->pal = PALETTE_PLAYER4;  // Hard set the palette
            break;
        case BRONZE_SKELKEY:
            p = "BRONZE SKELKEY,";
            sp->pal = PALETTE_PLAYER8;  // Hard set the palette
            break;
        case RED_SKELKEY:
            p = "RED SKELKEY,";
            sp->pal = PALETTE_PLAYER9;  // Hard set the palette
            break;
        case RED_KEY_STATUE:
            p = "RED STAT,";
            break;
        case BLUE_KEY_STATUE:
            p = "BLUE STAT,";
            break;
        case GREEN_KEY_STATUE:
            p = "GREEN STAT,";
            break;
        case YELLOW_KEY_STATUE:
            p = "YEL STAT,";
            break;
        default:
            p = "";
            break;
        }

    if (sp->picnum != ST1)
    {
        if (sp->lotag == TAG_SPRITE_HIT_MATCH)
        {
            p = "TAG_SPRITE_HIT_MATCH";
        }

        // multi
        if (TEST(sp->extra, SPRX_MULTI_ITEM))
            strcpy(multi_str, "MULTI,");
    }

    // track
    if (sp->picnum >= TRACK_SPRITE && sp->picnum < TRACK_SPRITE + 100)
    {
        sprintf(tp, "T%d,", sp->picnum - TRACK_SPRITE);
        p = tp;
    }

    switch (CaptionMode)
    {
    case CAPTION_NONE:
        tempbuf[0] = 0;
        break;

    case CAPTION_DEFAULT:
        if ((sprite[spritenum].lotag | sprite[spritenum].hitag) == 0)
            //tempbuf[0] = NULL;
            sprintf(tempbuf, "S:%d", data);
        else
            sprintf(tempbuf, "S:%d,%d,%d", data, TrackerCast(sprite[spritenum].hitag), TrackerCast(sprite[spritenum].lotag));
        break;


    case CAPTION_NAMES:
        // Show clip boxes in default mode
        if (sp->hitag == SO_CLIP_BOX)
            DrawClipBox(spritenum);
        if (sp->hitag == SECT_SO_CLIP_DIST)
            DrawClipBox(spritenum);

        if ((sprite[spritenum].lotag | sprite[spritenum].hitag) == 0)
            sprintf(tempbuf, "S:%d,%s%s", data, p, multi_str);
        else
            // name and numbers - name only prints if not null string
            sprintf(tempbuf, "%s%s%d,%d", p, multi_str, TrackerCast(sprite[spritenum].hitag), TrackerCast(sprite[spritenum].lotag));

        break;

    case CAPTION_MOST:
        if ((sprite[spritenum].lotag | sprite[spritenum].hitag) == 0)
            sprintf(tempbuf, "%s%s", p, multi_str);
        else
            sprintf(tempbuf, "%s%s%d,%d,%d,%d,%d,%d", p, multi_str,
                    TrackerCast(SPRITE_TAG1(spritenum)),
                    TrackerCast(SPRITE_TAG2(spritenum)),
                    TrackerCast(SPRITE_TAG3(spritenum)),
                    TrackerCast(SPRITE_TAG4(spritenum)),
                    TrackerCast(SPRITE_TAG5(spritenum)),
                    TrackerCast(SPRITE_TAG6(spritenum)));
        break;

    case CAPTION_ALL:
//            if (sp->hitag == SO_CLIP_BOX)
//                DrawClipBox(spritenum);

        if ((sprite[spritenum].lotag | sprite[spritenum].hitag) == 0)
            sprintf(tempbuf, "%s%s", p, multi_str);
        else
            sprintf(tempbuf, "%s%s%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", p, multi_str,
                    TrackerCast(SPRITE_TAG1(spritenum)),
                    TrackerCast(SPRITE_TAG2(spritenum)),
                    TrackerCast(SPRITE_TAG3(spritenum)),
                    TrackerCast(SPRITE_TAG4(spritenum)),
                    TrackerCast(SPRITE_TAG5(spritenum)),
                    TrackerCast(SPRITE_TAG6(spritenum)),
                    SPRITE_TAG7(spritenum),
                    SPRITE_TAG8(spritenum),
                    SPRITE_TAG9(spritenum),
                    SPRITE_TAG10(spritenum));
        break;
    }


    return tempbuf;
}

void
SectorMidPoint(short sectnum, int *xmid, int *ymid, int *zmid)
{
    short startwall, endwall, j;
    int xsum = 0, ysum = 0;
    WALLp wp;

    startwall = sector[sectnum].wallptr;
    endwall = startwall + sector[sectnum].wallnum - 1;

    for (wp = &wall[startwall], j = startwall; j <= endwall; wp++, j++)
    {
        xsum += wp->x;
        ysum += wp->y;
    }

    *xmid = xsum / (endwall - startwall + 1);
    *ymid = ysum / (endwall - startwall + 1);

    *zmid = DIV2(sector[sectnum].floorz + sector[sectnum].ceilingz);
}

void
DrawClipBox(short spritenum)
{
    int x = 0, y = 0, z;
    int radius;
    extern int zoom;

    if (sprite[spritenum].hitag == SO_CLIP_BOX)
    {
        x = mulscale14(sprite[spritenum].x - pos.x, zoom);
        y = mulscale14(sprite[spritenum].y - pos.y, zoom);
    }
    else if (sprite[spritenum].hitag == SECT_SO_CLIP_DIST)
    {
        SectorMidPoint(sprite[spritenum].sectnum,&x, &y, &z);
        x = mulscale14(x - pos.x, zoom);
        y = mulscale14(y - pos.y, zoom);
    }

    x += 320;
    y += 200;

    radius = mulscale14(sprite[spritenum].lotag, zoom);

#define BOX_COLOR 3
    // upper
    editorDraw2dLine(x - radius, y - radius, x + radius, y - radius, BOX_COLOR);
    // lower
    editorDraw2dLine(x - radius, y + radius, x + radius, y + radius, BOX_COLOR);
    // left
    editorDraw2dLine(x - radius, y - radius, x - radius, y + radius, BOX_COLOR);
    // right
    editorDraw2dLine(x + radius, y - radius, x + radius, y + radius, BOX_COLOR);
}

void
ExtShowSectorData(short sectnum)        // F5
{
    int i, x, y, x2;

    if (qsetmode == 200)                // In 3D mode
        return;

    clearmidstatbar16();                // Clear middle of status bar


//  if (keystatus[KEYSC_F5] )
//  {
    KEY_PRESSED(KEYSC_F5) = 0;
    for (i = 0; i < MAXSPRITES; i++)
        numsprite[i] = 0;
    for (i = 0; i < MAXSPRITES; i++)
        multisprite[i] = 0;
    for (i = 0; i < MAXSPRITES; i++)
    {
        if (sprite[i].statnum == 0)
        {
            if (TEST(sprite[i].extra, SPRX_MULTI_ITEM))
            {
                if (sprite[i].picnum == ICON_UZIFLOOR)
                    multisprite[ICON_UZI]++;
                else
                    multisprite[sprite[i].picnum]++;

            }
            else
            {
                if (sprite[i].picnum == ICON_UZIFLOOR)
                    numsprite[ICON_UZI]++;
                else
                    numsprite[sprite[i].picnum]++;
            }
        }
    }


    clearmidstatbar16();                // Clear middle of status bar

    x = 1;
    x2 = 14;
    y = 4;
    printext16(x * 8, ydim16+y * 8, 11, -1, "Item Count", 0);
    PrintStatus("10%Health=", numsprite[ICON_SM_MEDKIT], x, y + 2, 11);
    PrintStatus("", multisprite[ICON_SM_MEDKIT], x2, y + 2, 1);
    PrintStatus("HealthBot=", numsprite[ICON_BOOSTER], x, y + 3, 11);
    PrintStatus("", multisprite[ICON_BOOSTER], x2, y + 3, 1);
    PrintStatus("Armor    =", numsprite[ICON_ARMOR], x, y + 4, 11);
    PrintStatus("", multisprite[ICON_ARMOR], x2, y + 4, 1);

    x = 17;
    x2 = 30;
    y = 4;
    printext16(x * 8, ydim16+y * 8, 11, -1, "Inventory", 0);
    PrintStatus("Med-Kit  =", numsprite[ICON_MEDKIT], x, y + 2, 11);
    PrintStatus("", multisprite[ICON_MEDKIT], x2, y + 2, 1);
    PrintStatus("Bio_Suit =", numsprite[ICON_ENVIRON_SUIT], x, y + 3, 11);
    PrintStatus("", multisprite[ICON_ENVIRON_SUIT], x2, y + 3, 1);
    PrintStatus("NightGogs=", numsprite[ICON_NIGHT_VISION], x, y + 4, 11);
    PrintStatus("", multisprite[ICON_NIGHT_VISION], x2, y + 4, 1);
    PrintStatus("SmokeBomb=", numsprite[ICON_CLOAK], x, y + 5, 11);
    PrintStatus("", multisprite[ICON_CLOAK], x2, y + 5, 1);
    PrintStatus("Tool_Box =", numsprite[ICON_REPAIR_KIT], x, y + 6, 11);
    PrintStatus("", multisprite[ICON_REPAIR_KIT], x2, y + 6, 1);
    PrintStatus("Heat_Card=", numsprite[ICON_HEAT_CARD], x, y + 7, 11);
    PrintStatus("", multisprite[ICON_HEAT_CARD], x2, y + 7, 1);
    PrintStatus("FlashBomb=", numsprite[ICON_FLASHBOMB], x, y + 8, 11);
    PrintStatus("", multisprite[ICON_FLASHBOMB], x2, y + 8, 1);
    PrintStatus("Caltrops =", numsprite[ICON_CALTROPS], x, y + 9, 11);
    PrintStatus("", multisprite[ICON_CALTROPS], x2, y + 9, 1);

    x = 33;
    x2 = 46;
    y = 4;
    printext16(x * 8, ydim16+y * 8, 11, -1, "Weapon Count", 0);
    PrintStatus("Shuriken =", numsprite[ICON_STAR], x, y + 2, 11);
    PrintStatus("", multisprite[ICON_STAR], x2, y + 2, 1);
    PrintStatus("Uzi      =", numsprite[ICON_UZI], x, y + 3, 11);
    PrintStatus("", multisprite[ICON_UZI], x2, y + 3, 1);
    PrintStatus("Riot_Gun =", numsprite[ICON_SHOTGUN], x, y + 4, 11);
    PrintStatus("", multisprite[ICON_SHOTGUN], x2, y + 4, 1);
    PrintStatus("Misl_Bat =", numsprite[ICON_MICRO_GUN], x, y + 5, 11);
    PrintStatus("", multisprite[ICON_MICRO_GUN], x2, y + 5, 1);
    PrintStatus("40mm     =", numsprite[ICON_GRENADE_LAUNCHER], x, y + 6, 11);
    PrintStatus("", multisprite[ICON_GRENADE_LAUNCHER], x2, y + 6, 1);
    PrintStatus("Mines    =", numsprite[ICON_LG_MINE], x, y + 7, 11);
    PrintStatus("", multisprite[ICON_LG_MINE], x2, y + 7, 1);
    PrintStatus("Rail_Gun =", numsprite[ICON_RAIL_GUN], x, y + 8, 11);
    PrintStatus("", multisprite[ICON_RAIL_GUN], x2, y + 8, 1);
    PrintStatus("Evil Head=", numsprite[ICON_GUARD_HEAD], x, y + 9, 11);
    PrintStatus("", multisprite[ICON_GUARD_HEAD], x2, y + 9, 1);
    PrintStatus("Heart    =", numsprite[ICON_HEART], x, y + 10, 11);
    PrintStatus("", multisprite[ICON_HEART], x2, y + 10, 1);

    x = 49;
    x2 = 62;
    y = 4;
    printext16(x * 8, ydim16+y * 8, 11, -1, "Ammo Count", 0);
    PrintStatus("Bullets  =", numsprite[ICON_LG_UZI_AMMO], x, y + 2, 11);
    PrintStatus("", multisprite[ICON_LG_UZI_AMMO], x2, y + 2, 1);
    PrintStatus("ShotShell=", numsprite[ICON_LG_SHOTSHELL], x, y + 3, 11);
    PrintStatus("", multisprite[ICON_LG_SHOTSHELL], x2, y + 3, 1);
    PrintStatus("Rockets  =", numsprite[ICON_MICRO_BATTERY], x, y + 4, 11);
    PrintStatus("", multisprite[ICON_MICRO_BATTERY], x2, y + 4, 1);
    PrintStatus("40mmShell=", numsprite[ICON_LG_GRENADE], x, y + 5, 11);
    PrintStatus("", multisprite[ICON_LG_GRENADE], x2, y + 5, 1);
    PrintStatus("Rail_Pack=", numsprite[ICON_RAIL_AMMO], x, y + 6, 11);
    PrintStatus("", multisprite[ICON_RAIL_AMMO], x2, y + 6, 1);


    // Show next tags
    FindNextTag();
    ShowNextTag();
    /*
            printext16(65*8,ydim16+4*8,11,-1,"MISC",0);
            PrintStatus("Secrets =",secrets,65,6,11);
            printext16(65*8,ydim16+7*8,11,-1,"ACTORS",0);
            PrintStatus("Skill 1 =",totalactors1,65,8,11);
            PrintStatus("Skill 2 =",totalactors2,65,9,11);
            PrintStatus("Skill 3 =",totalactors3,65,10,11);
            PrintStatus("Skill 4 =",totalactors4,65,11,11);
            PrintStatus("Respawn =",totalrespawn,65,12,11);
    */
//  }

#if 0
    sprintf(tempbuf, "Sector %d", sectnum);
    printext16(8, ydim16+32, 11, -1, tempbuf, 0);

    printext16(8, ydim16+48, 11, -1, "8*8 font: ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz 0123456789", 0);
    printext16(8, ydim16+56, 11, -1, "3*5 font: ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz 0123456789", 1);

    drawline16(320, 68, 344, 80, 4);    // Draw house
    drawline16(344, 80, 344, 116, 4);
    drawline16(344, 116, 296, 116, 4);
    drawline16(296, 116, 296, 80, 4);
    drawline16(296, 80, 320, 68, 4);
#endif
}

void
ExtShowWallData(short wallnum)          // F6
{
    if (qsetmode == 200)                // In 3D mode
        return;


    clearmidstatbar16();                // Clear middle of status bar
    sprintf(tempbuf, "Wall %d", wallnum);
    printext16(8, ydim16+32, 11, -1, tempbuf, 0);
}

void
ExtShowSpriteData(short spritenum)      // F6
{
    if (qsetmode == 200)                // In 3D mode
        return;

    while (KEY_PRESSED(KEYSC_F6)) ;
    ResetKeys();
    ContextHelp(spritenum);             // Get context sensitive help


// OLD
//    clearmidstatbar16();
//    sprintf(tempbuf, "Sprite %d", spritenum);
//    printext16(8, ydim16+32, 11, -1, tempbuf, 0);
}

void
ExtEditSectorData(short sectnum)        // F7
{
    short key_num;
    SPRITEp sp;

    if (qsetmode == 200)                // In 3D mode
        return;


    sp = &sprite[FindSpriteNum];        // Set sprite to current spritenum

    clearmidstatbar16();                // Clear middle of status bar

    sprintf(tempbuf, "Current attributes for sprite being searched for:");
    printext16(8, ydim16+32, 11, -1, tempbuf, 0);

    sprintf(tempbuf, "PicNum = %d", FindPicNum);
    printext16(8, ydim16+32 + 16, 11, -1, tempbuf, 0);

    sprintf(tempbuf, "HiTag = %d", TrackerCast(sp->hitag));
    printext16(8, ydim16+32 + 24, 11, -1, tempbuf, 0);

    sprintf(tempbuf, "LowTag = %d", TrackerCast(sp->lotag));
    printext16(8, ydim16+32 + 32, 11, -1, tempbuf, 0);

    FindNextSprite(FindPicNum);
}

void
ExtEditWallData(short wallnum)          // F8
{
//    short nickdata;

    if (qsetmode == 200)                // In 3D mode
        return;

//    sprintf(tempbuf, "Wall (%ld): ", wallnum);
//    nickdata = 0;
//    nickdata = getnumber16(tempbuf, nickdata, 65536L);

//    printmessage16(" ");                 // Clear message box (top right of
    // status bar)
//    ExtShowWallData(wallnum);
}

void
ExtEditSpriteData(short spritenum)      // F8
{
    uint8_t* key;
    short data;
    SPRITEp sp;

    SetSpriteExtra();

    sp = &sprite[spritenum];


    if (qsetmode == 200)                // In 3D mode
        return;

    clearmidstatbar16();                // Clear middle of status bar
    printext16(8, ydim16+32, 11, -1, "(1)  Skill Level - 0=Easy 1=Normal 2=Hard 3=Crazy", 0);
    printext16(8, ydim16+32 + 8, 11, -1, "(2)  Multi-Player Item Toggle", 0);
    printext16(8, ydim16+32 + 16, 11, -1, "(3)  Find Sprite", 0);
    printext16(8, ydim16+32 + 24, 11, -1, "(4)  Dbug Toggle (* Programming use only *) ", 0);
    videoShowFrame(1);

    while (KEY_PRESSED(KEYSC_F8)) handleevents();

    ResetKeys();

    while ((key = BKeyPressed()) == NULL) handleevents();

    if (key == (uint8_t*)&KEY_PRESSED(KEYSC_1) || key == (uint8_t*)&KEY_PRESSED(KEYSC_F8))
    {
        *key = FALSE;

        sprintf(tempbuf, "Sprite (%d) Skill Level (0-3) : ", spritenum);

        data = TEST(sp->extra, SPRX_SKILL);

        data = getnumber16(tempbuf, data, 65536L, 1);

        if (data > 3)
            data = 3;

        RESET(sp->extra, SPRX_SKILL);
        SET(sp->extra, data);
    }
    else if (key == (uint8_t*)&KEY_PRESSED(KEYSC_2) || key == (uint8_t*)&KEY_PRESSED(KEYSC_F9))
    {
        *key = FALSE;

        FLIP(sprite[spritenum].extra, SPRX_MULTI_ITEM);
    }
    else if (key == (uint8_t*)&KEY_PRESSED(KEYSC_3) || key == (uint8_t*)&KEY_PRESSED(KEYSC_F10))
    {
        *key = FALSE;

        do
        {
DISPLAY:
            clearmidstatbar16();
            printext16(8, ydim16+32, 11, -1, "Toggle Sprite Seach Criteria.  ESC quits.", 0);

            printext16(8, ydim16+32 + 16, 11, -1, "(1) Use PicNum in search: ", 0);
            if (bFindPicNum)
                printext16(8 + 240, ydim16+32 + 16, 11, -1, "TRUE", 0);
            else
                printext16(8 + 240, ydim16+32 + 16, 11, -1, "FALSE", 0);

            printext16(8, ydim16+32 + 24, 11, -1, "(2) Use HiTag in search: ", 0);
            if (bFindHiTag)
                printext16(8 + 240, ydim16+32 + 24, 11, -1, "TRUE", 0);
            else
                printext16(8 + 240, ydim16+32 + 24, 11, -1, "FALSE", 0);

            printext16(8, ydim16+32 + 32, 11, -1, "(3) Use LowTag in search: ", 0);
            if (bFindLowTag)
                printext16(8 + 240, ydim16+32 + 32, 11, -1, "TRUE", 0);
            else
                printext16(8 + 240, ydim16+32 + 32, 11, -1, "FALSE", 0);
            videoShowFrame(1);

            // Disallow invalid settings
            if (!bFindPicNum && !bFindHiTag && !bFindLowTag)
            {
                bFindPicNum = TRUE;
                goto DISPLAY;
            }

            while (KEY_PRESSED(KEYSC_1) || KEY_PRESSED(KEYSC_2) || KEY_PRESSED(KEYSC_3)
                   || KEY_PRESSED(KEYSC_4)) handleevents();

            ResetKeys();

            while ((key = BKeyPressed()) == NULL) handleevents();

            if (key == (uint8_t*)&KEY_PRESSED(KEYSC_1))
            {
                *key = FALSE;
                bFindPicNum = !bFindPicNum;
            }
            else if (key == (uint8_t*)&KEY_PRESSED(KEYSC_2))
            {
                *key = FALSE;
                bFindHiTag = !bFindHiTag;
            }
            else if (key == (uint8_t*)&KEY_PRESSED(KEYSC_3))
            {
                *key = FALSE;
                bFindLowTag = !bFindLowTag;
            }

        }
        while ((KEY_PRESSED(0x1c) == 0) && (KEY_PRESSED(0x1) == 0));    // Enter, ESC
        KEY_PRESSED(0x1c) = 0;
        KEY_PRESSED(0x1) = 0;

        FindSprite(sprite[spritenum].picnum, spritenum);
    }
    else if (key == (uint8_t*)&KEY_PRESSED(KEYSC_4))
    {
        *key = FALSE;
        FLIP(sprite[spritenum].extra, SPRX_BLOCK);
    }

    printmessage16(" ");

    clearmidstatbar16();                // Clear middle of status bar

    sprintf(tempbuf, "Current attributes for selected sprite:");
    printext16(8, ydim16+32, 11, -1, tempbuf, 0);

    sprintf(tempbuf, "     Skill = %d", TEST(sp->extra, SPRX_SKILL));
    printext16(8, ydim16+32 + 16, 11, -1, tempbuf, 0);

    sprintf(tempbuf, "     Multi Item = %d", !!TEST(sp->extra, SPRX_MULTI_ITEM));
    printext16(8, ydim16+32 + 24, 11, -1, tempbuf, 0);

    sprintf(tempbuf, "     Debug = %d", !!TEST(sp->extra, SPRX_BLOCK));
    printext16(8, ydim16+32 + 32, 11, -1, tempbuf, 0);

}

void
PlaxSetShade(void)
{
    short data;
    short shade;
    int i, count = 0;

    if (qsetmode == 200)                // In 3D mode
        return;

    sprintf(tempbuf, "Plax Sky set shade to #: ");
    shade = getnumber16(tempbuf, 0, 65536L, 1);

    if (shade == 0)
        return;

    for (i = 0, count = 0; i < numsectors; i++)
    {
        if (TEST(sector[i].ceilingstat, CEILING_STAT_PLAX))
        {
            sector[i].ceilingshade = shade;
            count++;
        }
    }

    printmessage16(" ");

    clearmidstatbar16();                // Clear middle of status bar

    sprintf(tempbuf, "%d Plax Sky shades set.", count);
    printext16(8, ydim16+32, 11, -1, tempbuf, 0);
}

void
PlaxAdjustShade(void)
{
    short data;
    short shade;
    int i, count = 0;

    if (qsetmode == 200)                // In 3D mode
        return;

    sprintf(tempbuf, "Plax Sky adjust shade by (+10000 for negative): ");
    shade = getnumber16(tempbuf, 0, 65536L, 1);

    if (shade == 0)
        return;

    if (shade > 10000)
    {
        shade = -(shade - 10000);
    }


    for (i = 0; i < numsectors; i++)
    {
        if (TEST(sector[i].ceilingstat, CEILING_STAT_PLAX))
        {
            sector[i].ceilingshade += shade;
        }
    }

    printmessage16(" ");

    clearmidstatbar16();                // Clear middle of status bar

    sprintf(tempbuf, "%d Plax Sky shades adjusted.", count);
    printext16(8, ydim16+32, 11, -1, tempbuf, 0);
}

void
AdjustShade(void)
{
    short data;
    short shade;
    int i, count;
    short SpriteNum, NextSprite;

    if (qsetmode == 200)                // In 3D mode
        return;

    sprintf(tempbuf, "Adjust amount (+10000 for negative): ");
    shade = getnumber16(tempbuf, 0, 65536L, 1);

    if (shade == 0)
        return;

    if (shade > 10000)
    {
        shade = -(shade - 10000);
    }

    if (highlightsectorcnt > -1)
    {
        short startwall, endwall;
        short i, j = 0;

        for (i = 0; i < highlightsectorcnt; i++)
        {
            sector[highlightsector[i]].floorshade += shade;
            sector[highlightsector[i]].ceilingshade += shade;

            TRAVERSE_SPRITE_SECT(headspritesect[highlightsector[i]], SpriteNum, NextSprite)
            {
                sprite[SpriteNum].shade += shade;
            }

            startwall = sector[highlightsector[i]].wallptr;
            endwall = startwall + sector[highlightsector[i]].wallnum - 1;

            for (j = startwall; j <= endwall; j++)
            {
                if (!TEST(wall[j].extra, 0x1))
                {
                    SET(wall[j].extra, 0x1);
                    wall[j].shade += shade;
                }

                if (!TEST(wall[wall[j].nextwall].extra, 0x1))
                {
                    SET(wall[wall[j].nextwall].extra, 0x1);
                    wall[wall[j].nextwall].shade += shade;
                }

            }
        }

        for (i = 0; i < MAXWALLS; i++)
        {
            RESET(wall[j].extra, 0x1);
        }

    }
    else
    {
        for (i = count = 0; i < numwalls; i++)
        {
            wall[i].shade += shade;
        }

        for (i = 0; i < numsectors; i++)
        {
            sector[i].floorshade += shade;
            sector[i].ceilingshade += shade;

            TRAVERSE_SPRITE_SECT(headspritesect[i], SpriteNum, NextSprite)
            {
                sprite[SpriteNum].shade += shade;
            }
        }
    }

    printmessage16(" ");

    clearmidstatbar16();                // Clear middle of status bar
}


void
SetClipdist2D(void)
{
    short dist;
    int i;
    short num;

    if (qsetmode == 200)                // In 3D mode
        return;

    if (highlightcnt <= -1)
        return;

    sprintf(tempbuf, "Sprite Clipdist: ");
    dist = getnumber16(tempbuf, 0, 65536L, 1);

    for (i = 0; i < highlightcnt; i++)
    {
        if (TEST(highlight[i], SPRITE_FLAG))
        {
            num = RESET(highlight[i], SPRITE_FLAG);
            sprite[highlight[i]].clipdist = dist;
        }
    }

    printmessage16(" ");

    clearmidstatbar16();                // Clear middle of status bar
}

void
AdjustVisibility(void)
{
    short data;
    short vis;
    int i, count = 0;

    if (qsetmode == 200)                // In 3D mode
        return;

    sprintf(tempbuf, "Adjust non-zero vis sectors by (+10000 for neg): ");
    vis = getnumber16(tempbuf, 0, 65536L, 1);

    if (vis == 0)
        return;

    if (vis > 10000)
    {
        vis = -(vis - 10000);
    }

    if (highlightsectorcnt > -1)
    {
        short i, j;

        for (i = 0; i < highlightsectorcnt; i++)
        {
            if (sector[highlightsector[i]].visibility != 0)
            {
                count++;
                sector[highlightsector[i]].visibility += vis;
            }
        }
    }
    else
        for (i = 0; i < numsectors; i++)
        {
            if (sector[i].visibility != 0)
            {
                count++;
                sector[i].visibility += vis;
            }
        }

    printmessage16(" ");

    clearmidstatbar16();                // Clear middle of status bar

    sprintf(tempbuf, "%d Vis adjusted.", count);
    printext16(8, ydim16+32, 11, -1, tempbuf, 0);
}

void
FindSprite(short picnum, short findspritenum)
{
    int i, count;
    short SpriteNum, NextSprite;
    SPRITEp sp;

    SWBOOL bFoundPicNum, bFoundHiTag, bFoundLowTag, bFoundIt;


    if (qsetmode == 200)                // In 3D mode
        return;

    if (picnum == 0)
    {
        sprintf(tempbuf, "Find sprite (tile number): ");
        picnum = getnumber16(tempbuf, 0, 65536L, 1);
    }

    FindPicNum = picnum;
    FindSpriteNum = findspritenum;

    ResetSpriteFound();

    // go to the first one
    for (i = 0; i < numsectors; i++)
    {
        TRAVERSE_SPRITE_SECT(headspritesect[i], SpriteNum, NextSprite)
        {
            sp = &sprite[SpriteNum];

            // Reset search status
            bFoundIt = TRUE;
            bFoundPicNum = bFoundHiTag = bFoundLowTag = FALSE;

            if (bFindPicNum)
            {
                if (sp->picnum == picnum)
                    bFoundPicNum = TRUE;
                bFoundIt = bFoundIt & bFoundPicNum;
            }
            if (bFindHiTag)
            {
                if (sp->hitag == sprite[FindSpriteNum].hitag)
                    bFoundHiTag = TRUE;
                bFoundIt = bFoundIt & bFoundHiTag;
            }
            if (bFindLowTag)
            {
                if (sp->lotag == sprite[FindSpriteNum].lotag)
                    bFoundLowTag = TRUE;
                bFoundIt = bFoundIt & bFoundLowTag;
            }

            if (bFoundIt)
            {
                SET(sp->extra, SPRX_FOUND);
                cursectnum = sp->sectnum;
                pos.x = sp->x;
                pos.y = sp->y;
                pos.z = sp->z - kensplayerheight;
                return;
            }

        }
    }

    printmessage16(" ");

    clearmidstatbar16();                // Clear middle of status bar
}

void
FindNextSprite(short picnum)
{
    int i, count;
    short SpriteNum, NextSprite;
    SPRITEp sp;
    short animlen;

    SWBOOL bFoundPicNum, bFoundHiTag, bFoundLowTag, bFoundIt;

    if (qsetmode == 200)                // In 3D mode
        return;

    for (i = 0; i < numsectors; i++)
    {
        TRAVERSE_SPRITE_SECT(headspritesect[i], SpriteNum, NextSprite)
        {
            sp = &sprite[SpriteNum];

            // Reset search status
            bFoundIt = TRUE;
            bFoundPicNum = bFoundHiTag = bFoundLowTag = FALSE;

            if (bFindPicNum)
            {
                if (sp->picnum == picnum)
                    bFoundPicNum = TRUE;
                bFoundIt = bFoundIt & bFoundPicNum;
            }
            if (bFindHiTag)
            {
                if (sp->hitag == sprite[FindSpriteNum].hitag)
                    bFoundHiTag = TRUE;
                bFoundIt = bFoundIt & bFoundHiTag;
            }
            if (bFindLowTag)
            {
                if (sp->lotag == sprite[FindSpriteNum].lotag)
                    bFoundLowTag = TRUE;
                bFoundIt = bFoundIt & bFoundLowTag;
            }

            if (bFoundIt && !TEST(sp->extra, SPRX_FOUND))
            {
                SET(sp->extra, SPRX_FOUND);
                cursectnum = sp->sectnum;
                pos.x = sp->x;
                pos.y = sp->y;
                pos.z = sp->z - kensplayerheight;
                return;
            }
        }
    }
}


// Array of no-no ST1 tags that should not be considered for match tags.
short tagcheck[] = {0, 1, 3, 5, 16, 23, 25, 27, 29, 30, 31, 32, 33, 34, 37, 38, 39, 42,
                    45, 46, 47, 48, 49, 50, 51, 52, 53, 55, 56, 62, 64, 65, 66, 68, 71,
                    72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 90, 94, 97, 98, 99, 100,
                    101, 102, 135, 110, 111, 112, 113, 114, 115, 9002};

SWBOOL
DoMatchCheck(SPRITEp sp)
{
    unsigned i;

    // Don't F up the next tag with weird tag 2 cases that aren't match tags!
    for (i = 0; i < sizeof(tagcheck); i++)
        if (tagcheck[i] == sp->hitag)
            return TRUE;                // This ST1 tag is evil!  Don't
    // consider it!

    return FALSE;                       // This ST1 tag if fine.
}

void
ShowNextTag(void)
{
    if (qsetmode == 200)                // In 3D mode
        return;

    printmessage16(" ");
    printmessage16("Next tag = %d", siNextEndTag);
}

void
FindNextTag(void)
{
    int i, count, j;
    short SpriteNum, NextSprite;
    short siNextFind;                   // Next tag that SHOULD be found
    SPRITEp sp;

    if (qsetmode == 200)                // In 3D mode
        return;

    siNextTag = siNextEndTag = 0;       // Reset tags for new search
    siNextFind = 0;

    // go to the first one
    for (i = 0; i < numsectors; i++)
    {
        TRAVERSE_SPRITE_SECT(headspritesect[i], SpriteNum, NextSprite)
        {
            sp = &sprite[SpriteNum];

            // If it's not an ST1 sprite, blow past it
            if (sp->picnum != ST1)
                continue;

            // Check for evil tags
            if (DoMatchCheck(sp))
                continue;

            // Show the highest possible next tag
            if (sp->lotag >= siNextEndTag)
                siNextEndTag = sp->lotag + 1;

        }
    }
}


void
ShadeMenu(void)                         // F8
{
    uint8_t* key;

    if (qsetmode == 200)                // In 3D mode
        return;

    clearmidstatbar16();                // Clear middle of status bar
    printext16(8, ydim16+32, 11, -1, "(1) Plax Set ", 0);
    printext16(8, ydim16+32 + 8, 11, -1, "(2) Plax Adjust ", 0);
    printext16(8, ydim16+32 + 16, 11, -1, "(3) Shade Adjust ", 0);
    printext16(8, ydim16+32 + 24, 11, -1, "(4) Visibility ", 0);

    ResetKeys();

    while ((key = BKeyPressed()) == NULL) ;

    if (key == (uint8_t*)&KEY_PRESSED(KEYSC_1))
    {
        *key = FALSE;
        PlaxSetShade();
    }
    else if (key == (uint8_t*)&KEY_PRESSED(KEYSC_2))
    {
        *key = FALSE;
        PlaxAdjustShade();
    }
    else if (key == (uint8_t*)&KEY_PRESSED(KEYSC_3))
    {
        *key = FALSE;
        AdjustShade();
    }
    else if (key == (uint8_t*)&KEY_PRESSED(KEYSC_4))
    {
        *key = FALSE;
        AdjustVisibility();
    }
}

void faketimerhandler(void)
{
    timerUpdate();
}

//Just thought you might want my getnumber16 code
/*
getnumber16(char namestart[80], short num, int maxnumber)
{
        char buffer[80];
        int j, k, n, danum, oldnum;

        danum = (int)num;
        oldnum = danum;
        while ((KEY_PRESSED(0x1c) != 2) && (KEY_PRESSED(0x1) == 0))  //Enter, ESC
        {
                sprintf(&buffer,"%s%ld_ ",namestart,danum);
                printmessage16(buffer);

                for(j=2;j<=11;j++)                //Scan numbers 0-9
                        if (KEY_PRESSED(j) > 0)
                        {
                                KEY_PRESSED(j) = 0;
                                k = j-1;
                                if (k == 10) k = 0;
                                n = (danum*10)+k;
                                if (n < maxnumber) danum = n;
                        }
                if (KEY_PRESSED(0xe) > 0)    // backspace
                {
                        danum /= 10;
                        KEY_PRESSED(0xe) = 0;
                }
                if (KEY_PRESSED(0x1c) == 1)   //L. enter
                {
                        oldnum = danum;
                        KEY_PRESSED(0x1c) = 2;
                        asksave = 1;
                }
        }
        KEY_PRESSED(0x1c) = 0;
        KEY_PRESSED(0x1) = 0;
        return((short)oldnum);
}
*/

static char messagecolor = 31;
static unsigned short messagedelay = 0;
static char messagebuf[1024];

void
Message(const char *string, char color)
{
    sprintf(messagebuf, string, 0);
    messagedelay = 512;
    messagecolor = color;
}

void
ShowMessage(void)
{
    if (messagedelay < 1)
        return;
    messagedelay--;
    printext256(1 * 4, 1 * 8, 1, 0, messagebuf, 1);
}

void
ResetSprites()
{
    short i;
    spritetype *tspr;

    DebugActorFreeze = 0;

    for (i = 0, tspr = &sprite[0]; i < MAXSPRITES; i++, tspr++)
    {
        RESET(tspr->cstat, CSTAT_SPRITE_INVISIBLE);
//        if (TEST(tspr->extra, SPRX_BLOCK))
//          SET(tspr->cstat, CSTAT_SPRITE_BLOCK);
    }

}

static char kvxloaded = 0;
void ExtPreLoadMap(void)
{
    ResetSprites();
    if (!kvxloaded) LoadKVXFromScript("swvoxfil.txt"), kvxloaded = 1;
}


int32_t ExtPreSaveMap(void)
{
    ResetBuildFAF();
    ResetSprites();

    return 0;
}

void
dsprintf(char *str, char *format, ...)
{
    va_list arglist;

    va_start(arglist, format);
    vsprintf(str, format, arglist);
    va_end(arglist);
}

void
dsprintf_null(char *str, const char *format, ...)
{
    va_list arglist;
}


void
BuildStagTable(void)
{
#define MAKE_STAG_TABLE
#include "stag.h"
#undef  MAKE_STAG_TABLE
}

#include "m32script.h"

void M32RunScript(const char *s) { UNREFERENCED_PARAMETER(s); }
void G_Polymer_UnInit(void) { }
void SetGamePalette(int32_t j) { UNREFERENCED_PARAMETER(j); }

int32_t AmbienceToggle, MixRate, ParentalLock;

int32_t taglab_linktags(int32_t spritep, int32_t num)
{
    int32_t link = 0;

    g_iReturnVar = link;
    VM_OnEvent(EVENT_LINKTAGS, spritep ? num : -1);
    link = g_iReturnVar;

    return link;
}

int32_t taglab_getnextfreetag(int32_t *duetoptr)
{
    int32_t i, nextfreetag=1;
    int32_t obj = -1;

    for (i=0; i<MAXSPRITES; i++)
    {
        int32_t tag;

        if (sprite[i].statnum == MAXSTATUS)
            continue;

        tag = select_sprite_tag(i);

        if (tag != INT32_MIN && nextfreetag <= tag)
        {
            nextfreetag = tag+1;
            obj = 32768 + i;
        }
    }

    for (i=0; i<numwalls; i++)
    {
        int32_t lt = taglab_linktags(0, i);

        if ((lt&1) && nextfreetag <= wall[i].lotag)
            nextfreetag = wall[i].lotag+1, obj = i;
        if ((lt&2) && nextfreetag <= wall[i].hitag)
            nextfreetag = wall[i].hitag+1, obj = i;
    }

    if (duetoptr != NULL)
        *duetoptr = obj;

    if (nextfreetag < 32768)
        return nextfreetag;

    return 0;
}

int32_t S_InvalidSound(int32_t num) {
    UNREFERENCED_PARAMETER(num); return 1;
};
int32_t S_CheckSoundPlaying(int32_t i, int32_t num) {
    UNREFERENCED_PARAMETER(i); UNREFERENCED_PARAMETER(num); return 0;
};
int32_t S_SoundsPlaying(int32_t i) {
    UNREFERENCED_PARAMETER(i); return -1;
}
int32_t S_SoundFlags(int32_t num) {
    UNREFERENCED_PARAMETER(num); return 0;
};
int32_t A_PlaySound(uint32_t num, int32_t i) {
    UNREFERENCED_PARAMETER(num); UNREFERENCED_PARAMETER(i); return 0;
};
void S_StopSound(int32_t num) {
    UNREFERENCED_PARAMETER(num);
};
void S_StopAllSounds(void) { }
