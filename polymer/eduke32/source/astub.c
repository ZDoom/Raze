//-------------------------------------------------------------------------
/*
Copyright (C) 2010 EDuke32 developers and contributors

This file is part of EDuke32.

EDuke32 is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
//-------------------------------------------------------------------------

#include "compat.h"
#include "build.h"
#include "editor.h"
#include "pragmas.h"
#include "baselayer.h"
#include "names.h"
#include "osd.h"
#include "osdfuncs.h"
#include "cache1d.h"

#include "mapster32.h"
#include "keys.h"

#include "keyboard.h"
#include "scriptfile.h"
#include "crc32.h"

#include "sounds_mapster32.h"
#include "fx_man.h"

#include "macros.h"
#include "quicklz.h"

#include "m32script.h"
#include "m32def.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

#include <signal.h>

#define BUILDDATE " 20100521"

static int32_t floor_over_floor;

// static char *startwin_labeltext = "Starting Mapster32...";
static char setupfilename[]= "mapster32.cfg";
static char defaultduke3dgrp[BMAX_PATH] = "duke3d.grp";
static char *g_grpNamePtr = defaultduke3dgrp;
static int32_t fixmapbeforesaving = 0;
static int32_t lastsave = -180*60;
static int32_t NoAutoLoad = 0;
static int32_t spnoclip=1;

static char default_tiles_cfg[] = "tiles.cfg";
static int32_t pathsearchmode_oninit;

// Sound in Mapster32
static char defaultgamecon[] = "game.con";
static char *gamecon = defaultgamecon;

#pragma pack(push,1)
sound_t g_sounds[MAXSOUNDS];
#pragma pack(pop)

static int16_t g_definedsndnum[MAXSOUNDS];  // maps parse order index to g_sounds index
static int16_t g_sndnum[MAXSOUNDS];  // maps current order index to g_sounds index
int32_t g_numsounds = 0;
static int32_t lastupdate, mousecol, mouseadd = 1, bstatus;

#if !defined(_WIN32)
static int32_t usecwd = 0;
#endif

static struct strllist
{
    struct strllist *next;
    char *str;
}
*CommandPaths = NULL, *CommandGrps = NULL;

const char *scripthist[SCRIPTHISTSIZ];
int32_t scripthistend = 0;

int32_t showambiencesounds=2;

//////////////////// Key stuff ////////////////////

#define eitherALT   (keystatus[KEYSC_LALT] || keystatus[KEYSC_RALT])
#define eitherCTRL  (keystatus[KEYSC_LCTRL] || keystatus[KEYSC_RCTRL])
#define eitherSHIFT (keystatus[KEYSC_LSHIFT] || keystatus[KEYSC_RSHIFT])

#define PRESSED_KEYSC(Key) (keystatus[KEYSC_##Key] && !(keystatus[KEYSC_##Key]=0))


//////////////////// Aiming ////////////////////
static const char *Typestr[] = { "Wall", "Ceiling", "Floor", "Sprite", "Wall" };
static const char *typestr[] = { "wall", "ceiling", "floor", "sprite", "wall" };
static const char *Typestr_wss[] = { "Wall", "Sector", "Sector", "Sprite", "Wall" };
/*static const char *typestr_wss[] = { "wall", "sector", "sector", "sprite", "wall" };*/

/** The following macros multiplex between identically named fields of sector/wall/sprite,
 * based on a macro parameter or the currently aimed at object (AIMED_ versions).
 * They can be used on either side of an assignment. */

// select wall, only makes a difference with walls that have 'swap bottom of walls' bit set
#define SELECT_WALL() (AIMING_AT_WALL ? searchbottomwall : searchwall)

#define SECFLD(i, Field)  (sector[i].Field)
#define WALFLD(i, Field)  (wall[i].Field)
#define SPRFLD(i, Field)  (sprite[i].Field)

// valid fields: z, stat, picnum, heinum, shade, pal, xpanning, ypanning
#define CEILINGFLOOR(iSec, Field) (*(AIMING_AT_CEILING ? &(sector[iSec].ceiling##Field) : &(sector[iSec].floor##Field)))
#define AIMED_CEILINGFLOOR(Field) CEILINGFLOOR(searchsector, Field)

#define AIMED_SEL_WALL(Field) WALFLD(SELECT_WALL(), Field)

// selects from wall proper or its mask
#define OVR_WALL(iWal, Field) (*(AIMING_AT_WALL ? &WALFLD(iWal, Field) : &(wall[iWal].over##Field)))
#define AIMED_SELOVR_WALL(Field) OVR_WALL(SELECT_WALL(), Field)

// the base macro to construct field multiplexing macros: wall and sector cases undetermined
#define MUXBASE(Field, SectorCase, WallCase) (*(AIMING_AT_CEILING_OR_FLOOR ? (SectorCase) : \
                                               (AIMING_AT_WALL_OR_MASK ? (WallCase) : \
                                                &SPRFLD(searchwall, Field))))

#define SFBASE_CF(Field, WallCase) MUXBASE(Field, &AIMED_CEILINGFLOOR(Field), WallCase)

#define SFBASE_(Field, WallCase) MUXBASE(Field, &SECFLD(searchsector,Field), WallCase)

#define AIMED(Field) SFBASE_(Field, &WALFLD(searchwall, Field))
#define AIMED_SEL(Field) SFBASE_(Field, &AIMED_SEL_WALL(Field))
//#define AIMED_CF(Field) SFBASE_CF(Field, &WALFLD(searchwall,Field))
#define AIMED_CF_SEL(Field) SFBASE_CF(Field, &AIMED_SEL_WALL(Field))

// OVR makes sense only with picnum
//#define AIMED_OVR_PICNUM  SFBASE_CF(picnum, &OVR_WALL(searchwall, picnum))
#define AIMED_SELOVR_PICNUM SFBASE_CF(picnum, &AIMED_SELOVR_WALL(picnum))


#define BTAG_MAX 65535


static const char *ONOFF_[] = {"OFF","ON"};
#define ONOFF(b) (ONOFF_[!!(b)])


static CACHE1D_FIND_REC *finddirs=NULL, *findfiles=NULL, *finddirshigh=NULL, *findfileshigh=NULL;
static int32_t numdirs=0, numfiles=0;
static int32_t currentlist=0;
static int32_t tsign, mouseaction=0, mouseax=0, mouseay=0;
static int32_t repeatcountx, repeatcounty;
static int32_t infobox=3; // bit0: current window, bit1: mouse pointer, the variable should be renamed

static char wallshades[MAXWALLS];
static char sectorshades[MAXSECTORS][2];
static char spriteshades[MAXSPRITES];
static char wallpals[MAXWALLS];
static char sectorpals[MAXSECTORS][2];
static char spritepals[MAXSPRITES];
static char wallflag[MAXWALLS];

// tile marking in tile selector for custom creation of tile groups
static int16_t tilemarked[(MAXTILES+7)>>3];

#ifdef POLYMER
static int16_t spritelightid[MAXSPRITES];
_prlight *spritelightptr[MAXSPRITES];
#endif
extern int32_t graphicsmode;

extern int32_t mskip;
extern int16_t capturecount;
extern int32_t editorgridextent;	// in engine.c
extern char game_executable[BMAX_PATH];

extern int32_t fillsector(int16_t sectnum, char fillcolor);

static void drawgradient()
{
    int32_t i, col = whitecol-21;
    begindrawing();
    for (i=ydim-STATUS2DSIZ+16; i<ydim && col>0; i++,col--)
        CLEARLINES2D(i, 1, (col<<24)|(col<<16)|(col<<8)|col);
    CLEARLINES2D(i, ydim-i, 0);
    enddrawing();
}

void message(const char *fmt, ...)
{
    char tmpstr[256];
    va_list va;

    va_start(va, fmt);
    Bvsnprintf(tmpstr, 256, fmt, va);
    va_end(va);

    Bstrcpy(getmessage,tmpstr);

    getmessageleng = Bstrlen(getmessage);
    getmessagetimeoff = totalclock+120*2;
    lastmessagetime = totalclock;

    if (!mouseaction)
    {
        Bstrcat(tmpstr,"\n");
        OSD_Printf(tmpstr);
    }
}

typedef struct _mapundo
{
    int32_t numsectors;
    int32_t numwalls;
    int32_t numsprites;

    sectortype *sectors;
    walltype *walls;
    spritetype *sprites;

    int32_t revision;

    uint32_t sectcrc, wallcrc, spritecrc;
    uint32_t sectsiz, wallsiz, spritesiz;

    struct _mapundo *next; // 'redo' loads this
    struct _mapundo *prev; // 'undo' loads this
} mapundo_t;

mapundo_t *mapstate = NULL;

int32_t map_revision = 1;

void create_map_snapshot(void)
{
    int32_t j;
    uint32_t tempcrc;

    /*
    if (mapstate->prev == NULL && mapstate->next != NULL) // should be the first map version
    mapstate = mapstate->next;
    */

    if (mapstate == NULL)
    {
        mapstate = (mapundo_t *)Bcalloc(1, sizeof(mapundo_t));
        mapstate->revision = map_revision = 1;
        mapstate->prev = mapstate->next = NULL;
    }
    else
    {
        if (mapstate->next != NULL)
        {
            mapundo_t *next = mapstate->next;
            next->prev = NULL;

            while (next->next)
                next = next->next;

            do
            {
                if (next->sectors && (next->prev == NULL || (next->sectcrc != next->prev->sectcrc)))
                    Bfree(next->sectors);
                if (next->walls && (next->prev == NULL || (next->wallcrc != next->prev->wallcrc)))
                    Bfree(next->walls);
                if (next->sprites && (next->prev == NULL || (next->spritecrc != next->prev->spritecrc)))
                    Bfree(next->sprites);
                if (!next->prev)
                {
                    Bfree(next);
                    break;
                }
                next = next->prev;
                Bfree(next->next);
            }
            while (next);
        }

        mapstate->next = (mapundo_t *)Bcalloc(1, sizeof(mapundo_t));
        mapstate->next->prev = mapstate;

        mapstate = mapstate->next;
        mapstate->revision = ++map_revision;
    }

    fixspritesectors();

    numsprites = 0;
    for (j=MAXSPRITES-1; j>=0; j--)
    {
        if (sprite[j].statnum != MAXSTATUS)
            numsprites++;
    }

    mapstate->numsectors = numsectors;
    mapstate->numwalls = numwalls;
    mapstate->numsprites = numsprites;

    tempcrc = crc32once((uint8_t *)&sector[0],sizeof(sectortype) * numsectors);


    if (numsectors)
    {
        if (mapstate->prev && mapstate->prev->sectcrc == tempcrc)
        {
            mapstate->sectors = mapstate->prev->sectors;
            mapstate->sectsiz = mapstate->prev->sectsiz;
            mapstate->sectcrc = tempcrc;
            /* OSD_Printf("found a match between undo sectors\n"); */
        }
        else
        {
            mapstate->sectors = (sectortype *)Bcalloc(1, sizeof(sectortype) * numsectors);
            mapstate->sectsiz = j = qlz_compress(&sector[0], (char *)&mapstate->sectors[0],
                                                 sizeof(sectortype) * numsectors, state_compress);
            mapstate->sectors = (sectortype *)Brealloc(mapstate->sectors, j);
            mapstate->sectcrc = tempcrc;
        }

        if (numwalls)
        {
            tempcrc = crc32once((uint8_t *)&wall[0],sizeof(walltype) * numwalls);


            if (mapstate->prev && mapstate->prev->wallcrc == tempcrc)
            {
                mapstate->walls = mapstate->prev->walls;
                mapstate->wallsiz = mapstate->prev->wallsiz;
                mapstate->wallcrc = tempcrc;
                /* OSD_Printf("found a match between undo walls\n"); */
            }
            else
            {
                mapstate->walls = (walltype *)Bcalloc(1, sizeof(walltype) * numwalls);
                mapstate->wallsiz = j = qlz_compress(&wall[0], (char *)&mapstate->walls[0],
                                                     sizeof(walltype) * numwalls, state_compress);
                mapstate->walls = (walltype *)Brealloc(mapstate->walls, j);
                mapstate->wallcrc = tempcrc;
            }
        }

        if (numsprites)
        {
            tempcrc = crc32once((uint8_t *)&sprite[0],sizeof(spritetype) * MAXSPRITES);

            if (mapstate->prev && mapstate->prev->spritecrc == tempcrc)
            {
                mapstate->sprites = mapstate->prev->sprites;
                mapstate->spritesiz = mapstate->prev->spritesiz;
                mapstate->spritecrc = tempcrc;
                /*OSD_Printf("found a match between undo sprites\n");*/
            }
            else
            {
                int32_t i = 0;
                spritetype *tspri = (spritetype *)Bcalloc(1, sizeof(spritetype) * numsprites),
                            *spri = &tspri[0];
                mapstate->sprites = (spritetype *)Bcalloc(1, sizeof(spritetype) * numsprites);

                for (j=0; j<MAXSPRITES && i < numsprites; j++)
                {
                    if (sprite[j].statnum != MAXSTATUS)
                    {
                        Bmemcpy(spri++,&sprite[j],sizeof(spritetype));
                        i++;
                    }
                }
                mapstate->spritesiz = j = qlz_compress(&tspri[0], (char *)&mapstate->sprites[0],
                                                       sizeof(spritetype) * numsprites, state_compress);
                mapstate->sprites = (spritetype *)Brealloc(mapstate->sprites, j);
                mapstate->spritecrc = tempcrc;
                Bfree(tspri);
            }
        }
    }
}

void map_undoredo_free(void)
{
    if (mapstate)
    {
        while (mapstate->next)
            mapstate = mapstate->next;

        while (mapstate->prev)
        {
            mapundo_t *state = mapstate->prev;
            if (mapstate->sectors && (mapstate->sectcrc != mapstate->prev->sectcrc)) Bfree(mapstate->sectors);
            if (mapstate->walls && (mapstate->wallcrc != mapstate->prev->wallcrc)) Bfree(mapstate->walls);
            if (mapstate->sprites && (mapstate->spritecrc != mapstate->prev->spritecrc)) Bfree(mapstate->sprites);
            Bfree(mapstate);
            mapstate = state;
        }

        if (mapstate->sectors) Bfree(mapstate->sectors);
        if (mapstate->walls) Bfree(mapstate->walls);
        if (mapstate->sprites) Bfree(mapstate->sprites);

        Bfree(mapstate);
        mapstate = NULL;
    }

    map_revision = 1;
}

int32_t map_undoredo(int32_t dir)
{
    int32_t i;

    if (mapstate == NULL) return 1;

    if (dir)
    {
        if (mapstate->next == NULL || !mapstate->next->numsectors) return 1;

        //        while (map_revision+1 != mapstate->revision && mapstate->next)
        mapstate = mapstate->next;
    }
    else
    {
        if (mapstate->prev == NULL || !mapstate->prev->numsectors) return 1;

        //        while (map_revision-1 != mapstate->revision && mapstate->prev)
        mapstate = mapstate->prev;
    }

    numsectors = mapstate->numsectors;
    numwalls = mapstate->numwalls;
    numsprites = mapstate->numsprites;
    map_revision = mapstate->revision;

    initspritelists();

    Bmemset(show2dsector, 0, sizeof(show2dsector));
    Bmemset(show2dsprite, 0, sizeof(show2dsprite));
    Bmemset(show2dwall, 0, sizeof(show2dwall));

    if (mapstate->numsectors)
    {
        qlz_decompress((const char *)&mapstate->sectors[0],  &sector[0], state_decompress);

        if (mapstate->numwalls)
            qlz_decompress((const char *)&mapstate->walls[0],  &wall[0], state_decompress);

        if (mapstate->numsprites)
            qlz_decompress((const char *)&mapstate->sprites[0],  &sprite[0], state_decompress);
    }

    updatenumsprites();

    for (i=0; i<numsprites; i++)
    {
        if ((sprite[i].cstat & 48) == 48) sprite[i].cstat &= ~48;
        insertsprite(sprite[i].sectnum,sprite[i].statnum);
    }

#ifdef POLYMER
    if (qsetmode == 200 && rendmode == 4)
        polymer_loadboard();
#endif
    return 0;
}

static const char *SpriteMode[]=
{
    "NONE",
    "SECTORS",
    "WALLS",
    "SPRITES",
    "ALL",
    "ITEMS ONLY",
    "CURRENT SPRITE ONLY",
    "ONLY SECTOREFFECTORS AND SECTORS",
    "NO SECTOREFFECTORS OR SECTORS"
};

#define MAXSKILL 5
static const char *SKILLMODE[MAXSKILL]=
{
    "Actor skill display: PIECE OF CAKE",
    "Actor skill display: LET'S ROCK",
    "Actor skill display: COME GET SOME",
    "Actor skill display: DAMN I'M GOOD",
    "Actor skill display: ALL SKILL LEVELS"
};

#define MAXNOSPRITES 4
static const char *SPRDSPMODE[MAXNOSPRITES]=
{
    "Sprite display: DISPLAY ALL SPRITES",
    "Sprite display: NO EFFECTORS",
    "Sprite display: NO ACTORS",
    "Sprite display: NO EFFECTORS OR ACTORS"
};

#define MAXHELP3D (int32_t)(sizeof(Help3d)/sizeof(Help3d[0]))
static const char *Help3d[]=
{
    "Mapster32 3D mode help",
    " ",
    " F1 = TOGGLE THIS HELP DISPLAY",
    " F2 = TOGGLE CLIPBOARD",
    " F3 = MOUSELOOK",
    " F6 = AUTOMATIC SECTOREFFECTOR HELP",
    " F7 = AUTOMATIC SECTOR TAG HELP",
    "",
    " ' A = TOGGLE AUTOSAVE",
    " ' D = CYCLE SPRITE SKILL DISPLAY",
    " ' R = TOGGLE FRAMERATE DISPLAY",
    " ' W = TOGGLE SPRITE DISPLAY",
    " ' X = MAP SHADE PREVIEW",
    " ' Y = TOGGLE PURPLE BACKGROUND",
    "",
    " ' T = CHANGE LOTAG",
    " ' H = CHANGE HITAG",
    " ' S = CHANGE SHADE",
    " ' M = CHANGE EXTRA",
    " ' V = CHANGE VISIBILITY",
    " ' L = CHANGE OBJECT COORDINATES",
    " ' C = CHANGE GLOBAL SHADE",
    "",
    " ' ENTER = PASTE GRAPHIC ONLY",
    " ' P & ; P = PASTE PALETTE TO ALL SELECTED SECTORS",
    " ; V = SET VISIBILITY ON ALL SELECTED SECTORS",
    " ' DEL = CSTAT=0",
    " CTRL-S = SAVE BOARD",
    " HOME = PGUP/PGDN MODIFIER (256 UNITS)",
    " END = PGUP/PGDN MODIFIER (512 UNITS)",
};

static void clearfilenames(void)
{
    klistfree(finddirs);
    klistfree(findfiles);
    finddirs = findfiles = NULL;
    numfiles = numdirs = 0;
}

static int32_t getfilenames(const char *path, const char *kind)
{
    CACHE1D_FIND_REC *r;

    clearfilenames();
    finddirs = klistpath(path,"*",CACHE1D_FIND_DIR);
    findfiles = klistpath(path,kind,CACHE1D_FIND_FILE);
    for (r = finddirs; r; r=r->next) numdirs++;
    for (r = findfiles; r; r=r->next) numfiles++;

    finddirshigh = finddirs;
    findfileshigh = findfiles;
    currentlist = 0;
    if (findfileshigh) currentlist = 1;

    return(0);
}

void ExtLoadMap(const char *mapname)
{
    int32_t i;
    int32_t sky=0;

    getmessageleng = 0;
    getmessagetimeoff = 0;

    Bstrcpy(levelname,mapname);
    pskyoff[0]=0;
    for (i=0; i<8; i++) pskyoff[i]=0;

    for (i=0; i<numsectors; i++)
    {
        switch (sector[i].ceilingpicnum)
        {
        case MOONSKY1 :
        case BIGORBIT1 : // orbit
        case LA : // la city
            sky=sector[i].ceilingpicnum;
            break;
        }
    }

    switch (sky)
    {
    case MOONSKY1 :
        //        earth          mountian   mountain         sun
        pskyoff[6]=1;
        pskyoff[1]=2;
        pskyoff[4]=2;
        pskyoff[2]=3;
        break;

    case BIGORBIT1 : // orbit
        //       earth1         2           3           moon/sun
        pskyoff[5]=1;
        pskyoff[6]=2;
        pskyoff[7]=3;
        pskyoff[2]=4;
        break;

    case LA : // la city
        //       earth1         2           3           moon/sun
        pskyoff[0]=1;
        pskyoff[1]=2;
        pskyoff[2]=1;
        pskyoff[3]=3;
        pskyoff[4]=4;
        pskyoff[5]=0;
        pskyoff[6]=2;
        pskyoff[7]=3;
        break;
    }

    pskybits=3;
    parallaxtype=0;
    Bsprintf(tempbuf, "Mapster32 - %s",mapname);

    map_undoredo_free();
    wm_setapptitle(tempbuf);
}

void ExtSaveMap(const char *mapname)
{
    UNREFERENCED_PARAMETER(mapname);
    saveboard("backup.map",&pos.x,&pos.y,&pos.z,&ang,&cursectnum);
}

static int32_t getTileGroup(const char *groupName)
{
    int32_t temp;
    for (temp = 0; temp < MAX_TILE_GROUPS; temp++)
    {
        if (s_TileGroups[temp].szText == NULL)
            return -1;

        if (!Bstrcmp(s_TileGroups[temp].szText, groupName))
            return temp;
    }
    return -1;
}

static int32_t tileInGroup(int32_t group, int32_t tilenum)
{
    // @todo Make a bitmap instead of doing this slow search..
    int32_t temp;
    if (group < 0 || group >= MAX_TILE_GROUPS || s_TileGroups[group].szText == NULL)
    {
        // group isn't valid.
        return 0;
    }
    for (temp=0; temp<s_TileGroups[group].nIds; temp++)
    {
        if (tilenum == s_TileGroups[group].pIds[temp])
            return 1;
    }
    return 0;
}

const char *ExtGetSectorType(int32_t lotag)
{
    static char tempbuf[64];

    Bmemset(tempbuf,0,sizeof(tempbuf));
    switch (lotag)
    {
    case 1:
        Bsprintf(tempbuf,"WATER (SE 7)");
        break;
    case 2:
        Bsprintf(tempbuf,"UNDERWATER (SE 7)");
        break;
    case 9:
        Bsprintf(tempbuf,"STAR TREK DOORS");
        break;
    case 15:
        Bsprintf(tempbuf,"ELEVATOR TRANSPORT (SE 17)");
        break;
    case 16:
        Bsprintf(tempbuf,"ELEVATOR PLATFORM DOWN");
        break;
    case 17:
        Bsprintf(tempbuf,"ELEVATOR PLATFORM UP");
        break;
    case 18:
        Bsprintf(tempbuf,"ELEVATOR DOWN");
        break;
    case 19:
        Bsprintf(tempbuf,"ELEVATOR UP");
        break;
    case 20:
        Bsprintf(tempbuf,"CEILING DOOR");
        break;
    case 21:
        Bsprintf(tempbuf,"FLOOR DOOR");
        break;
    case 22:
        Bsprintf(tempbuf,"SPLIT DOOR");
        break;
    case 23:
        Bsprintf(tempbuf,"SWING DOOR (SE 11)");
        break;
    case 25:
        Bsprintf(tempbuf,"SLIDE DOOR (SE 15)");
        break;
    case 26:
        Bsprintf(tempbuf,"SPLIT STAR TREK DOOR");
        break;
    case 27:
        Bsprintf(tempbuf,"BRIDGE (SE 20)");
        break;
    case 28:
        Bsprintf(tempbuf,"DROP FLOOR (SE 21)");
        break;
    case 29:
        Bsprintf(tempbuf,"TEETH DOOR (SE 22)");
        break;
    case 30:
        Bsprintf(tempbuf,"ROTATE RISE BRIDGE");
        break;
    case 31:
        Bsprintf(tempbuf,"2 WAY TRAIN (SE=30)");
        break;
    case 32767:
        Bsprintf(tempbuf,"SECRET ROOM");
        break;
    case -1:
        Bsprintf(tempbuf,"END OF LEVEL");
        break;
    default :
        if (lotag > 10000 && lotag < 32767)
            Bsprintf(tempbuf,"1 TIME SOUND");
        //        else  Bsprintf(tempbuf,"%hu",lotag);
        break;
    }
    return(tempbuf);
}

const char *ExtGetSectorCaption(int16_t sectnum)
{
    static char tempbuf[64];

    Bmemset(tempbuf, 0, sizeof(tempbuf));

    if (qsetmode != 200 && ((onnames!=1 && onnames!=4 && onnames!=7) || onnames==8))
        return tempbuf;

    if (qsetmode == 200 || (sector[sectnum].lotag|sector[sectnum].hitag))
    {
        Bstrcpy(lo, ExtGetSectorType(sector[sectnum].lotag));
        if (qsetmode != 200)
            Bsprintf(tempbuf,"%hu,%hu %s", sector[sectnum].hitag, sector[sectnum].lotag, lo);
        else
            Bsprintf(tempbuf,"%hu %s", sector[sectnum].lotag, lo);
    }
    return(tempbuf);
}

const char *ExtGetWallCaption(int16_t wallnum)
{
    static char tempbuf[64];

    Bmemset(tempbuf,0,sizeof(tempbuf));

    if (wall[wallnum].cstat & (1<<14))
    {
        Bsprintf(tempbuf,"%d", wallength(wallnum));
        wall[wallnum].cstat &= ~(1<<14);
        return(tempbuf);
    }

    if (!(onnames==2 || onnames==4))
    {
        tempbuf[0] = 0;
        return(tempbuf);
    }

    // HERE

    if ((wall[wallnum].lotag|wall[wallnum].hitag) == 0)
        tempbuf[0] = 0;
    else
        Bsprintf(tempbuf, "%hu,%hu", wall[wallnum].hitag, wall[wallnum].lotag);

    return(tempbuf);
} //end

const char *SectorEffectorTagText(int32_t lotag)
{
    static char tempbuf[64];

    static const char *tags[] =
    {
        "ROTATED SECTOR",                // 0
        "PIVOT SPRITE FOR SE 0",
        "EARTHQUAKE",
        "RANDOM LIGHTS AFTER SHOT OUT",
        "RANDOM LIGHTS",
        "(UNKNOWN)",                     // 5
        "SUBWAY",
        "TRANSPORT",
        "UP OPEN DOOR LIGHTS",
        "DOWN OPEN DOOR LIGHTS",
        "DOOR AUTO CLOSE (H=DELAY)",     // 10
        "ROTATE SECTOR DOOR",
        "LIGHT SWITCH",
        "EXPLOSIVE",
        "SUBWAY CAR",
        "SLIDE DOOR (ST 25)",            // 15
        "ROTATE REACTOR SECTOR",
        "ELEVATOR TRANSPORT (ST 15)",
        "INCREMENTAL SECTOR RISE/FALL",
        "CEILING FALL ON EXPLOSION",
        "BRIDGE (ST 27)",                // 20
        "DROP FLOOR (ST 28)",
        "TEETH DOOR (ST 29)",
        "1-WAY SE7 DESTINATION (H=SE 7)",
        "CONVAYER BELT",
        "ENGINE",                        // 25
        "(UNKNOWN)",
        "LIGHTNING (H= TILE#4890)",
        "CAMERA FOR PLAYBACK",
        "FLOAT",
        "2 WAY TRAIN (ST=31)",           // 30
        "FLOOR RISE",
        "CEILING FALL",
        "SPAWN JIB W/QUAKE",
    };

    Bmemset(tempbuf,0,sizeof(tempbuf));

    if (lotag>=0 && lotag<(int32_t)(sizeof(tags)/sizeof(tags[0])))
        Bsprintf(tempbuf, "%d: %s", lotag, tags[lotag]);
    else
        switch (lotag)
        {
        case 36:
            Bsprintf(tempbuf,"%d: SKRINK RAY SHOOTER",lotag);
            break;
        case 49:
            Bsprintf(tempbuf,"%d: POINT LIGHT",lotag);
            break;
        case 50:
            Bsprintf(tempbuf,"%d: SPOTLIGHT",lotag);
            break;
        default:
            Bsprintf(tempbuf,"%d: (UNKNOWN)",lotag);
            break;
        }

    return (tempbuf);
}

const char *MusicAndSFXTagText(int32_t lotag)
{
    static char tempbuf[16];

    Bmemset(tempbuf, 0, sizeof(tempbuf));

    if (g_numsounds <= 0)
        return tempbuf;

    if (lotag>0 && lotag<999 && g_sounds[lotag].definedname)
        return g_sounds[lotag].definedname;

    if (lotag>=1000 && lotag<2000)
        Bsprintf(tempbuf, "REVERB");
    return tempbuf;
}

const char *SectorEffectorText(int32_t spritenum)
{
    static char tempbuf[64];

    Bmemset(tempbuf, 0, sizeof(tempbuf));
    Bmemset(lo, 0, sizeof(lo));

    Bstrcpy(lo, SectorEffectorTagText(sprite[spritenum].lotag));
    if (!lo[5]) // tags are 5 chars or less
        SpriteName(spritenum, tempbuf);
    else
        Bsprintf(tempbuf, "SE %s",lo);

    return (tempbuf);
}

const char *ExtGetSpriteCaption(int16_t spritenum)
{
    static char tempbuf[1024];
    int32_t retfast = 0;

    Bmemset(tempbuf,0,sizeof(tempbuf));

    if (!(onnames>=3 && onnames<=8) || (onnames==7 && sprite[spritenum].picnum!=SECTOREFFECTOR))
        retfast = 1;
    if (onnames==5 && !tileInGroup(tilegroupItems, sprite[spritenum].picnum))
        retfast = 1;
    if (onnames==6 && sprite[spritenum].picnum != sprite[cursprite].picnum)
        retfast = 1;

    if (retfast)
        return(tempbuf);

    if ((sprite[spritenum].lotag|sprite[spritenum].hitag) == 0)
    {
        SpriteName(spritenum,lo);
        if (lo[0]!=0)
        {
            if (sprite[spritenum].pal==1) Bsprintf(tempbuf,"%s (MULTIPLAYER)",lo);
            else Bsprintf(tempbuf,"%s",lo);
        }
    }
    else if (sprite[spritenum].picnum==SECTOREFFECTOR)
    {
        if (onnames!=8)
        {
            Bsprintf(lo,"%s",SectorEffectorText(spritenum));
            Bsprintf(tempbuf,"%s, %hu",lo,sprite[spritenum].hitag);
        }
    }
    else
    {
        SpriteName(spritenum,lo);
        if (sprite[spritenum].extra != -1)
            Bsprintf(tempbuf,"%hu,%hu,%d %s",sprite[spritenum].hitag,sprite[spritenum].lotag,sprite[spritenum].extra,lo);
        else
            Bsprintf(tempbuf,"%hu,%hu %s",sprite[spritenum].hitag,sprite[spritenum].lotag,lo);
    }

    return(tempbuf);

} //end

//printext16 parameters:
//printext16(int32_t xpos, int32_t ypos, int16_t col, int16_t backcol,
//           char name[82], char fontsize)
//  xpos 0-639   (top left)
//  ypos 0-479   (top left)
//  col 0-15
//  backcol 0-15, -1 is transparent background
//  name
//  fontsize 0=8*8, 1=3*5

//drawline16 parameters:
// drawline16(int32_t x1, int32_t y1, int32_t x2, int32_t y2, char col)
//  x1, x2  0-639
//  y1, y2  0-143  (status bar is 144 high, origin is top-left of STATUS BAR)
//  col     0-15

void ExtShowSectorData(int16_t sectnum)   //F5
{
    int32_t x,x2,y;
    int32_t i,yi;
    int32_t secrets=0;
    int32_t totalactors1=0,totalactors2=0,totalactors3=0,totalactors4=0;
    int32_t totalrespawn=0;

    UNREFERENCED_PARAMETER(sectnum);
    if (qsetmode==200)
        return;

    for (i=0; i<numsectors; i++)
        secrets += (sector[i].lotag==32767);

    i = headspritestat[0];
    while (i != -1)
    {
        // Count all non-player actors.
        if (tileInGroup(tilegroupActors, sprite[i].picnum))
        {
            if (sprite[i].lotag<=1) totalactors1++;
            if (sprite[i].lotag<=2) totalactors2++;
            if (sprite[i].lotag<=3) totalactors3++;
            if (sprite[i].lotag<=4) totalactors4++;
        }
        if (sprite[i].picnum == RESPAWN) totalrespawn++;

        i = nextspritestat[i];
    }

    Bmemset(numsprite, 0, sizeof(numsprite));
    Bmemset(multisprite, 0, sizeof(numsprite));

    for (i=0; i<MAXSPRITES; i++)
    {
        if (sprite[i].statnum==0)
        {
            if (sprite[i].pal!=0)
                multisprite[sprite[i].picnum]++;
            else
                numsprite[sprite[i].picnum]++;
        }
    }

    clearmidstatbar16();             //Clear middle of status bar

    ydim -= 8;
    drawgradient();
    ydim += 8;

    printmessage16("Level %s",levelname);

#define PRSTAT(Str, Tiledef) \
    PrintStatus(Str, numsprite[Tiledef], x, y+yi, numsprite[Tiledef]?11:7); \
    PrintStatus("",multisprite[Tiledef], x2,y+yi, multisprite[Tiledef]?9:7); \
    yi++;

    ydim -= 8; // vvvvvv reset at end!!

    x=1; x2=14;
    y=4; yi=2;
    begindrawing();
    printext16(x*8, ydim-STATUS2DSIZ+y*8, editorcolors[11], -1, "Item Count", 0);
    enddrawing();

    PRSTAT("10%health=", COLA);
    PRSTAT("30%health=", SIXPAK);
    PRSTAT("Med-Kit  =", FIRSTAID);
    PRSTAT("Atom     =", ATOMICHEALTH);
    PRSTAT("Shields  =", SHIELD);

    x=17; x2=30;
    y=4; yi=2;
    begindrawing();
    printext16(x*8, ydim-STATUS2DSIZ+y*8, editorcolors[11], -1, "Inventory", 0);
    enddrawing();

    PRSTAT("Steroids =", STEROIDS);
    PRSTAT("Airtank  =", AIRTANK);
    PRSTAT("Jetpack  =", JETPACK);
    PRSTAT("Goggles  =", HEATSENSOR);
    PRSTAT("Boots    =", BOOTS);
    PRSTAT("HoloDuke =", HOLODUKE);
    PRSTAT("Multi D  =", APLAYER);

    x=33; x2=46;
    y=4; yi=2;
    begindrawing();
    printext16(x*8, ydim-STATUS2DSIZ+y*8, editorcolors[11], -1, "Weapon Count", 0);
    enddrawing();

    PRSTAT("Pistol   =", FIRSTGUNSPRITE);
    PRSTAT("Shotgun  =", SHOTGUNSPRITE);
    PRSTAT("Chaingun =", CHAINGUNSPRITE);
    PRSTAT("RPG      =", RPGSPRITE);
    PRSTAT("Pipe Bomb=", HEAVYHBOMB);
    PRSTAT("Shrinker =", SHRINKERSPRITE);
    PRSTAT("Devastatr=", DEVISTATORSPRITE);
    PRSTAT("Trip mine=", TRIPBOMBSPRITE);
    PRSTAT("Freezeray=", FREEZESPRITE);

    x=49; x2=62;
    y=4; yi=2;
    begindrawing();
    printext16(x*8,ydim-STATUS2DSIZ+y*8,editorcolors[11],-1,"Ammo Count",0);
    enddrawing();

    PRSTAT("Pistol   =", AMMO);
    PRSTAT("Shot     =", SHOTGUNAMMO);
    PRSTAT("Chain    =", BATTERYAMMO);
    PRSTAT("RPG Box  =", RPGAMMO);
    PRSTAT("Pipe Bomb=", HBOMBAMMO);
    PRSTAT("Shrinker =", CRYSTALAMMO);
    PRSTAT("Devastatr=", DEVISTATORAMMO);
    PRSTAT("Expander =", GROWAMMO);
    PRSTAT("Freezeray=", FREEZEAMMO);

    begindrawing();
    printext16(65*8, ydim-STATUS2DSIZ+4*8, editorcolors[11], -1, "MISC", 0);
    printext16(65*8, ydim-STATUS2DSIZ+8*8, editorcolors[11], -1, "ACTORS", 0);
    enddrawing();

#undef PRSTAT

    PrintStatus("Secrets =", secrets, 65, 6, 11);
    PrintStatus("Skill 1 =", totalactors1, 65, 10, 11);
    PrintStatus("Skill 2 =", totalactors2, 65, 11, 11);
    PrintStatus("Skill 3 =", totalactors3, 65, 12, 11);
    PrintStatus("Skill 4 =", totalactors4, 65, 13, 11);
    PrintStatus("Respawn =", totalrespawn, 65, 14, 11);

    ydim += 8; // ^^^^^^ see above!

}// end ExtShowSectorData

void ExtShowWallData(int16_t wallnum)       //F6
{
    int32_t i, runi, nextfreetag=0, total=0, x, y, yi;

    UNREFERENCED_PARAMETER(wallnum);

    if (qsetmode==200)
        return;

    for (i=0; i<MAXSPRITES; i++)
    {
        if (sprite[i].statnum==0)
            switch (sprite[i].picnum)
            {
                //LOTAG
            case ACTIVATOR:
            case ACTIVATORLOCKED:
            case TOUCHPLATE:
            case MASTERSWITCH:
            case RESPAWN:
            case ACCESSSWITCH:
            case SLOTDOOR:
            case LIGHTSWITCH:
            case SPACEDOORSWITCH:
            case SPACELIGHTSWITCH:
            case FRANKENSTINESWITCH:
            case MULTISWITCH:
            case DIPSWITCH:
            case DIPSWITCH2:
            case TECHSWITCH:
            case DIPSWITCH3:
            case ACCESSSWITCH2:
            case POWERSWITCH1:
            case LOCKSWITCH1:
            case POWERSWITCH2:
            case PULLSWITCH:
            case ALIENSWITCH:
                if (sprite[i].lotag>nextfreetag) nextfreetag = sprite[i].lotag+1;
                break;

                //HITAG
            case SEENINE:
            case OOZFILTER:
            case SECTOREFFECTOR:
                if (sprite[i].lotag==10 || sprite[i].lotag==27 || sprite[i].lotag==28 || sprite[i].lotag==29)
                    break;
                else if (sprite[i].hitag>nextfreetag) nextfreetag=sprite[i].hitag+1;
                break;
            default:
                break;

            }

    } // end sprite loop

    clearmidstatbar16();
    drawgradient();

    printmessage16("Level %s next tag %d", levelname, nextfreetag);


#define CASES_LIZTROOP \
    LIZTROOP: case LIZTROOPRUNNING : case LIZTROOPSTAYPUT: case LIZTROOPSHOOT: \
              case LIZTROOPJETPACK: case LIZTROOPONTOILET: case LIZTROOPDUCKING
#define CASES_BOSS1  BOSS1: case BOSS1STAYPUT: case BOSS1SHOOT: case BOSS1LOB: case BOSSTOP

    Bmemset(numsprite, 0, sizeof(numsprite));
    Bmemset(multisprite, 0, sizeof(multisprite));

    for (i=0; i<MAXSPRITES; i++)
    {
        if (sprite[i].statnum==0 && sprite[i].pal)
            switch (sprite[i].picnum)
            {
            case CASES_LIZTROOP:
                numsprite[LIZTROOP]++;
                break;
            case CASES_BOSS1:
                multisprite[BOSS1]++;
                break;
            case BOSS2:
                multisprite[BOSS2]++;
                break;
            case BOSS3:
                multisprite[BOSS3]++;
            default:
                break;
            }
    }

    //runi==0: Count Normal Actors,  runi==1: Count Respawn Actors
    for (runi=0; runi<2; runi++)
    {
        if (runi==2)
        {
            Bmemset(numsprite, 0, sizeof(numsprite));
            Bmemset(multisprite, 0, sizeof(multisprite));
        }

        for (i=0; i<MAXSPRITES; i++)
        {
            if (sprite[i].statnum==0 && ((runi==0 && sprite[i].pal==0) || (runi==1 && sprite[i].picnum==RESPAWN)))
            {
                int32_t pic = (runi==1) ? sprite[i].picnum : sprite[i].hitag;
                switch (sprite[i].picnum)
                {
                case CASES_LIZTROOP:
                    numsprite[LIZTROOP]++;
                    break;
                case PIGCOP: case PIGCOPSTAYPUT: case PIGCOPDIVE:
                    numsprite[PIGCOP]++;
                    break;
                case LIZMAN: case LIZMANSTAYPUT: case LIZMANSPITTING: case LIZMANFEEDING: case LIZMANJUMP:
                    numsprite[LIZMAN]++;
                    break;
                case CASES_BOSS1:
                    if (runi==0 || sprite[i].pal==0)
                        numsprite[BOSS1]++;
                    else
                        multisprite[BOSS1]++;
                    break;
                case COMMANDER:
                case COMMANDERSTAYPUT:
                    numsprite[COMMANDER]++;
                    break;
                case OCTABRAIN:
                case OCTABRAINSTAYPUT:
                    numsprite[OCTABRAIN]++;
                    break;
                case RECON: case DRONE: case ROTATEGUN: case EGG: case ORGANTIC: case GREENSLIME:
                case BOSS2: case BOSS3: case TANK: case NEWBEAST: case NEWBEASTSTAYPUT: case BOSS4:
                    numsprite[pic]++;
                default:
                    break;

                }// end switch
            }// end if
        }//end for

#undef CASES_LIZTROOP
#undef CASES_BOSS1

        total=0;
        for (i=0; i<MAXSPRITES; i++)
            total += numsprite[i];
        for (i=0; i<MAXSPRITES; i++)
            total += multisprite[i];

        x=2+runi*34;
        y=4;
        PrintStatus(runi==0?"Normal actors:":"Respawn actors:", total, x, y, 11);

#define PRSTAT(Str, Tiledef)  PrintStatus(Str, numsprite[Tiledef], x, y+(yi++), numsprite[Tiledef]?11:7);
        yi=1;

        PRSTAT(" Liztroop  =", LIZTROOP);
        PRSTAT(" Lizman    =", LIZMAN);
        PRSTAT(" Commander =", COMMANDER);
        PRSTAT(" Octabrain =", OCTABRAIN);
        PRSTAT(" PigCop    =", PIGCOP);
        PRSTAT(" Recon Car =", RECON);
        PRSTAT(" Drone     =", DRONE);
        PRSTAT(" Turret    =", ROTATEGUN);
        PRSTAT(" Egg       =", EGG);

        x+=17;
        yi=1;
        PRSTAT("Slimer    =", GREENSLIME);
        PRSTAT("Boss1     =", BOSS1);
        PrintStatus("MiniBoss1 =", multisprite[BOSS1], x, y+(yi++), multisprite[BOSS1]?11:7);
        PRSTAT("Boss2     =", BOSS2);
        PRSTAT("Boss3     =", BOSS3);
        PRSTAT("Riot Tank =", TANK);
        PRSTAT("Newbeast  =", NEWBEAST);
        PRSTAT("Boss4     =", BOSS4);
#undef PRSTAT
    }
}// end ExtShowWallData

static void Show2dText(char *name)
{
    int32_t fp,t;
    uint8_t x=0,y=4,xmax=0,xx=0,col=0;

    clearmidstatbar16();
    drawgradient();

    if ((fp=kopen4load(name,0)) == -1)
    {
        begindrawing();
        printext16(1*4,ydim-STATUS2DSIZ+4*8,editorcolors[11],-1,"ERROR: file not found.",0);
        enddrawing();
        return;
    }

    t=65;
    begindrawing();
    while (t!=EOF && col<5)
    {
        t = 0;
        if (kread(fp,&t,1)<=0)
            t = EOF;
        while (t!=EOF && t!='\n' && x<250)
        {
            tempbuf[x]=t;
            t = 0;
            if (kread(fp,&t,1)<=0) t = EOF;
            x++;
            if (x>xmax) xmax=x;
        }
        tempbuf[x]=0;
        printext16(xx*4,ydim-STATUS2DSIZ+(y*6)+2,editorcolors[11],-1,tempbuf,1);
        x=0;
        y++;
        if (y>18)
        {
            col++;
            y=6;
            xx+=xmax;
            xmax=0;
        }
    }
    enddrawing();

    kclose(fp);

}// end Show2dText

// PK_ vvvv
typedef struct helppage_
{
    int32_t numlines;
    char line[][80];  // C99 flexible array member
} helppage_t;

static helppage_t **helppage=NULL;
static int32_t numhelppages=0;

static int32_t emptyline(const char *start)
{
    int32_t i;
    for (i=0; i<80; i++)
    {
        if (start[i]=='\n' || !start[i]) break;
        if (start[i]!=' ' && start[i]!='\t' && start[i]!='\r')
            return 0;
    }
    return 1;
}

static int32_t newpage(const char *start)
{
    int32_t i;
    for (i=80-1; i>=0; i--)
    {
        if (start[i] == '^' && start[i+1] == 'P')
            return 1;
    }
    return 0;
}

#define IHELP_INITPAGES 32
#define IHELP_INITLINES 16

static void ReadHelpFile(const char *name)
{
    BFILE *fp;
    int32_t i, j, k, numallocpages;
    int32_t pos, charsread=0;
    helppage_t *hp;
    char skip=0;

    initprintf("Loading \"%s\"\n",name);

    if ((fp=fopenfrompath(name,"rb")) == NULL)
    {
        initprintf("Error initializing integrated help: file \"%s\" not found.\n", name);
        return;
    }

    helppage=Bmalloc(IHELP_INITPAGES * sizeof(helppage_t *));
    numallocpages=IHELP_INITPAGES;
    if (!helppage) goto HELPFILE_ERROR;

    i=0;
    while (!Bfeof(fp) && !ferror(fp))
    {
        while (!Bfeof(fp))    // skip empty lines
        {
            pos = ftell(fp);
            if (Bfgets(tempbuf, 80, fp) == NULL) break;
            charsread = ftell(fp)-pos;
            if (!newpage(tempbuf))
            {
                break;
            }
        }

        if (Bfeof(fp) || charsread<=0) break;

        hp=Bcalloc(1,sizeof(helppage_t) + IHELP_INITLINES*80);
        if (!hp) goto HELPFILE_ERROR;
        hp->numlines = IHELP_INITLINES;

        if (charsread == 79 && tempbuf[78]!='\n') skip=1;
        j=0;

        do
        {
            if (j >= hp->numlines)
            {
                hp=Brealloc(hp, sizeof(helppage_t) + 2*hp->numlines*80);
                if (!hp) goto HELPFILE_ERROR;
                hp->numlines *= 2;
            }

            // limit the line length to 78 chars and probably get rid of the CR
            if (charsread>0)
            {
                tempbuf[charsread-1]=0;
                if (tempbuf[charsread-2]==0x0d) tempbuf[charsread-2]=0;
            }

            Bmemcpy(hp->line[j], tempbuf, 80);

            for (k=charsread; k<80; k++) hp->line[j][k]=0;

            if (skip)
            {
                while (fgetc(fp)!='\n' && !Bfeof(fp)) /*skip rest of line*/;
                skip=0;
            }

            pos = ftell(fp);
            if (Bfgets(tempbuf, 80, fp) == NULL) break;
            charsread = ftell(fp)-pos;
            if (charsread == 79 && tempbuf[78]!='\n') skip=1;

            j++;

        }
        while (!newpage(tempbuf) && !Bfeof(fp) && charsread>0);

        hp=Brealloc(hp, sizeof(helppage_t) + j*80);
        if (!hp) goto HELPFILE_ERROR;
        hp->numlines=j;

        if (i >= numallocpages)
        {
            helppage = Brealloc(helppage, 2*numallocpages*sizeof(helppage_t *));
            numallocpages *= 2;
            if (!helppage) goto HELPFILE_ERROR;
        }
        helppage[i] = hp;
        i++;
    }

    helppage = Brealloc(helppage, i*sizeof(helppage_t *));
    if (!helppage) goto HELPFILE_ERROR;
    numhelppages = i;

    Bfclose(fp);
    return;

HELPFILE_ERROR:

    Bfclose(fp);
    initprintf("ReadHelpFile(): ERROR allocating memory.\n");
    return;
}

// why can't MSVC allocate an array of variable size?!
#define IHELP_NUMDISPLINES 110 // ((overridepm16y>>4)+(overridepm16y>>5)+(overridepm16y>>7)-2)
#define IHELP_PATLEN 45
extern int32_t overridepm16y;  // influences clearmidstatbar16()

static void IntegratedHelp()
{
    if (!helppage) return;

    overridepm16y = ydim;//3*STATUS2DSIZ;

    {
        int32_t i, j;
        static int32_t curhp=0, curline=0;
        int32_t highlighthp=-1, highlightline=-1, lasthighlighttime=0;
        char disptext[IHELP_NUMDISPLINES][80];
        char oldpattern[IHELP_PATLEN+1];

        Bmemset(oldpattern, 0, sizeof(char));
        //    clearmidstatbar16();

        while (keystatus[KEYSC_ESC]==0 && keystatus[KEYSC_Q]==0 && keystatus[KEYSC_F1]==0)
        {
            begindrawing();
            CLEARLINES2D(0, ydim, 0);
            enddrawing();

            idle_waitevent();
            if (handleevents())
                quitevent = 0;

            //        printmessage16("Help mode, press <Esc> to exit");

            if (PRESSED_KEYSC(T))    // goto table of contents
            {
                curhp=0;
                curline=0;
            }
            else if (PRESSED_KEYSC(G))    // goto arbitrary page
            {
                curhp=getnumber16("Goto page: ", 0, numhelppages-1, 0);
                curline=0;
            }
            else if (PRESSED_KEYSC(UP))    // scroll up
            {
                if (curline>0) curline--;
            }
            else if (PRESSED_KEYSC(DOWN))    // scroll down
            {
                if (curline + 32/*+IHELP_NUMDISPLINES*/ < helppage[curhp]->numlines) curline++;
            }
            else if (PRESSED_KEYSC(PGUP))    // scroll one page up
            {
                i=IHELP_NUMDISPLINES;
                while (i>0 && curline>0) i--, curline--;
            }
            else if (PRESSED_KEYSC(PGDN))    // scroll one page down
            {
                i=IHELP_NUMDISPLINES;
                while (i>0 && curline + 32/*+IHELP_NUMDISPLINES*/ < helppage[curhp]->numlines) i--, curline++;
            }
            else if (PRESSED_KEYSC(SPACE))   // goto next paragraph
            {
                for (i=curline, j=0; i < helppage[curhp]->numlines; i++)
                {
                    if (emptyline(helppage[curhp]->line[i])) { j=1; continue; }
                    if (j==1 && !emptyline(helppage[curhp]->line[i])) { j=2; break; }
                }
                if (j==2)
                {
                    if (i + 32 /*+IHELP_NUMDISPLINES*/ < helppage[curhp]->numlines)
                        curline=i;
                    else if (helppage[curhp]->numlines - 32/*-IHELP_NUMDISPLINES*/ > curline)
                        curline = helppage[curhp]->numlines - 32/*-IHELP_NUMDISPLINES*/;
                }
            }
            else if (PRESSED_KEYSC(BS))   // goto prev paragraph
            {
                for (i=curline-1, j=0; i>=0; i--)
                {
                    if (!emptyline(helppage[curhp]->line[i])) { j=1; continue; }
                    if (j==1 && emptyline(helppage[curhp]->line[i])) { j=2; break; }
                }
                if (j==2 || i==-1) curline=i+1;
            }
            else if (PRESSED_KEYSC(HOME))    // goto beginning of page
            {
                curline=0;
            }
            else if (PRESSED_KEYSC(END))    // goto end of page
            {
                if ((curline=helppage[curhp]->numlines - 32/*-IHELP_NUMDISPLINES*/) >= 0) /**/;
                else curline=0;
            }
            else if (PRESSED_KEYSC(LEFT) || PRESSED_KEYSC(LBRACK))    // prev page
            {
                if (curhp>0)
                {
                    curhp--;
                    curline=0;
                }
            }
            else if (PRESSED_KEYSC(RIGHT) || PRESSED_KEYSC(RBRACK))    // next page
            {
                if (curhp<numhelppages-1)
                {
                    curhp++;
                    curline=0;
                }
            }

            // based on 'save as' dialog in overheadeditor()
            else if (keystatus[KEYSC_S])    // text search
            {
                char ch, bad=0, pattern[IHELP_PATLEN+1];

                for (i=0; i<IHELP_PATLEN+1; i++) pattern[i]=0;

                i=0;
                bflushchars();
                while (bad == 0)
                {
                    _printmessage16("Search: %s_", pattern);
                    showframe(1);

                    idle_waitevent();

                    if (handleevents())
                        quitevent = 0;

                    ch = bgetchar();

                    if (keystatus[1]) bad = 1;
                    else if (ch == 13) bad = 2;
                    else if (ch > 0)
                    {
                        if (i > 0 && (ch == 8 || ch == 127))
                        {
                            i--;
                            pattern[i] = 0;
                        }
                        else if (i < IHELP_PATLEN && ch >= 32 && ch < 128)
                        {
                            pattern[i++] = ch;
                            pattern[i] = 0;
                        }
                    }
                }

                if (bad==1)
                {
                    keystatus[KEYSC_ESC] = keystatus[KEYSC_Q] = keystatus[KEYSC_F1] = 0;
                }

                if (bad==2)
                {
                    keystatus[KEYSC_ENTER] = 0;

                    for (i=curhp; i<numhelppages; i++)
                    {
                        for (j = (i==curhp)?(curline+1):0; j<helppage[i]->numlines; j++)
                        {
                            // entering an empty pattern will search with the last used pattern
                            if (strstr(helppage[i]->line[j], pattern[0]?pattern:oldpattern))
                            {
                                curhp = i;

                                if ((curline=j) <= helppage[i]->numlines - 32 /*-IHELP_NUMDISPLINES*/) /**/;
                                else if ((curline=helppage[i]->numlines- 32 /*-IHELP_NUMDISPLINES*/) >= 0) /**/;
                                else curline=0;

                                highlighthp = i;
                                highlightline = j;
                                lasthighlighttime = totalclock;
                                goto ENDFOR1;
                            }
                        }
                    }
ENDFOR1:
                    if (pattern[0])
                        Bmemcpy(oldpattern, pattern, IHELP_PATLEN+1);
                }
            }
            else    // '1'-'0' on the upper row
            {
                for (i=2; i<=11; i++)
                    if (keystatus[i]) break;
                if (i--<12 && i<numhelppages)
                {
                    curhp=i;
                    curline=0;
                }
            }

//            drawgradient();

            begindrawing();
            printext16(9, ydim2d-overridepm16y+9, editorcolors[4], -1, "Help Mode", 0);
            printext16(8, ydim2d-overridepm16y+8, editorcolors[12], -1, "Help Mode", 0);
            printext16(8 + 9*8 + 2*8, ydim2d-overridepm16y+8, editorcolors[15], -1, "(S:search)", 0);
            enddrawing();

            if (curhp < helppage[0]->numlines)
                _printmessage16("%s", helppage[0]->line[curhp]);
            else
                _printmessage16("%d. (Untitled page)", curhp);

            for (i=0; j=(curhp==0)?(i+curline+1):(i+curline),
                    i<IHELP_NUMDISPLINES && j<helppage[curhp]->numlines; i++)
            {
                if (ydim-overridepm16y+28+i*9+32 >= ydim)
                    break;
                Bmemcpy(disptext[i], helppage[curhp]->line[j], 80);
                printext16(8, ydim-overridepm16y+28+i*9, editorcolors[10],
                           (j==highlightline && curhp==highlighthp
                            && totalclock-lasthighlighttime<120*5) ?
                           editorcolors[1] : -1,
                           disptext[i], 0);
            }

            showframe(1);
        }

        overridepm16y = -1;
        //        i=ydim16;
        //        ydim16=ydim;
        //        drawline16(0,ydim-STATUS2DSIZ,xdim-1,ydim-STATUS2DSIZ,editorcolors[1]);
        //        ydim16=i;
        //        // printmessage16("");
        //        showframe(1);

        keystatus[KEYSC_ESC] = keystatus[KEYSC_Q] = keystatus[KEYSC_F1] = 0;
    }
}

#define SOUND_NUMDISPLINES IHELP_NUMDISPLINES
//extern char SoundToggle;

static int32_t compare_sounds_s(int16_t k1, int16_t k2)
{
    return (int32_t)k1 - (int32_t)k2;
}
static int32_t compare_sounds_d(int16_t k1, int16_t k2)
{
    sound_t *s1 = &g_sounds[k1], *s2 = &g_sounds[k2];
    char *n1 = s1->definedname, *n2 = s2->definedname;

    if (!n1 && !n2) return 0;
    if (!n1) return -1;
    if (!n2) return 1;
    return Bstrcasecmp(n1, n2);
}
static int32_t compare_sounds_f(int16_t k1, int16_t k2)
{
    sound_t *s1 = &g_sounds[k1], *s2 = &g_sounds[k2];
    char *n1 = s1->filename, *n2 = s2->filename;

    if (!n1 && !n2) return 0;
    if (!n1) return -1;
    if (!n2) return 1;
    return Bstrcasecmp(n1, n2);
}
static int32_t compare_sounds_1(int16_t k1, int16_t k2)
{
    return (g_sounds[k2].m&1) - (g_sounds[k1].m&1);
}
static int32_t compare_sounds_2(int16_t k1, int16_t k2)
{
    return (g_sounds[k2].m&2) - (g_sounds[k1].m&2);
}
static int32_t compare_sounds_3(int16_t k1, int16_t k2)
{
    return (g_sounds[k2].m&4) - (g_sounds[k1].m&4);
}
static int32_t compare_sounds_4(int16_t k1, int16_t k2)
{
    return (g_sounds[k2].m&8) - (g_sounds[k1].m&8);
}
static int32_t compare_sounds_5(int16_t k1, int16_t k2)
{
    return (g_sounds[k2].m&16) - (g_sounds[k1].m&16);
}


static int32_t sort_sounds(int32_t how)
{
    int32_t (*compare_sounds)(int16_t, int16_t) = NULL;

    int32_t ms, ofs, l, lb, r, rb, d, n, k1, k2;
    int16_t *src, *dst, *source, *dest, *tmp;

    n = g_numsounds;
    src = source = g_sndnum;
    dest = Bmalloc(sizeof(int16_t) * n);
    dst = dest;
    if (!dest) return -1;

    switch (how)
    {
    case 'g':  // restore original order
        Bmemcpy(g_sndnum, g_definedsndnum, sizeof(int16_t)*n);
        return 0;
    case 's':
        compare_sounds = compare_sounds_s;
        break;
    case 'd':
        compare_sounds = compare_sounds_d;
        break;
    case 'f':
        compare_sounds = compare_sounds_f;
        break;
    case '1':
        compare_sounds = compare_sounds_1;
        break;
    case '2':
        compare_sounds = compare_sounds_2;
        break;
    case '3':
        compare_sounds = compare_sounds_3;
        break;
    case '4':
        compare_sounds = compare_sounds_4;
        break;
    case '5':
        compare_sounds = compare_sounds_5;
        break;
    default:
        return -2;
    }

    for (ms=1; ms<n; ms*=2)
    {
        for (ofs=0; ofs<n; ofs+=2*ms)
        {
            l = ofs;
            r = ofs+ms;
            d = ofs;
            lb = min((l+ms), n);
            rb = min((r+ms), n);
            while (l < lb || r < rb)
            {
                if (l >= lb)
                {
                    dst[d++] = src[r++];
                    continue;
                }
                if (r >= rb)
                {
                    dst[d++] = src[l++];
                    continue;
                }
                k1 = src[l];
                k2 = src[r];
                if (compare_sounds(k1, k2) <= 0)
                {
                    dst[d++] = src[l++];
                    continue;
                }
                dst[d++] = src[r++];
            }
        }
        tmp = src;
        src = dst;
        dst = tmp;
    }
    if (src != source)
        Bmemcpy(source, src, sizeof(int16_t) * n);

    Bfree(dest);
    return 0;
}

static void SoundDisplay()
{
    if (g_numsounds <= 0) return;

    overridepm16y = ydim;//3*STATUS2DSIZ;

    {
        int32_t i, j;
        // cursnd is the first displayed line, cursnd+curofs is where the cursor is
        static int32_t cursnd=0, curofs=0;
        char disptext[SOUND_NUMDISPLINES][80];

        //        SoundToggle = 1;

        while (keystatus[KEYSC_ESC]==0 && keystatus[KEYSC_Q]==0 && keystatus[KEYSC_F2]==0
                && keystatus[buildkeys[BK_MODE2D_3D]]==0)  // quickjump to 3d mode
        {
            begindrawing();
            CLEARLINES2D(0, ydim16, 0);
            enddrawing();

            idle_waitevent();
            if (handleevents())
                quitevent = 0;

//            drawgradient();

            begindrawing();
            printext16(9, ydim2d-overridepm16y+9, editorcolors[4], -1, "Sound Index", 0);
            printext16(8, ydim2d-overridepm16y+8, editorcolors[12], -1, "Sound Index", 0);
            printext16(8 + 11*8 + 2*8, ydim2d-overridepm16y+8, editorcolors[15], -1, "(SPACE:play, S:sort)", 0);
            enddrawing();

            if (PRESSED_KEYSC(G))    // goto specified sound#
            {
                _printmessage16("                                                    ");
                j = getnumber16("Goto sound#: ", 0, g_numsounds-1, 0);
                for (i=0; i<g_numsounds; i++)
                    if (g_sndnum[i]==j)
                        break;
                if (i != g_numsounds)
                {
                    if (i<SOUND_NUMDISPLINES)
                        cursnd = 0, curofs = i;
                    else if (i>=g_numsounds- 32/*SOUND_NUMDISPLINES*/)
                        cursnd = g_numsounds-32/*SOUND_NUMDISPLINES*/, curofs = i-cursnd;
                    else
                        curofs = 32/*SOUND_NUMDISPLINES*//2, cursnd = i-curofs;
                }
            }
            else if (PRESSED_KEYSC(UP))    // scroll up
            {
                if (curofs>0) curofs--;
                else if (cursnd>0) cursnd--;
            }
            else if (PRESSED_KEYSC(DOWN))    // scroll down
            {
                if (curofs<32/*SOUND_NUMDISPLINES*/-1 && cursnd+curofs<g_numsounds-1)
                    curofs++;
                else if (cursnd+32/*SOUND_NUMDISPLINES*/ < g_numsounds)
                    cursnd++;
            }
            else if (PRESSED_KEYSC(PGUP))    // scroll one page up
            {
                i=SOUND_NUMDISPLINES;
                while (i>0 && curofs>0)
                    i--, curofs--;
                while (i>0 && cursnd>0)
                    i--, cursnd--;
            }
            else if (PRESSED_KEYSC(PGDN))    // scroll one page down
            {
                i=SOUND_NUMDISPLINES;

                while (i>0 && curofs<32/*SOUND_NUMDISPLINES*/-1 && cursnd+curofs<g_numsounds-1)
                    i--, curofs++;
                while (i>0 && cursnd+32/*SOUND_NUMDISPLINES*/ < g_numsounds)
                    i--, cursnd++;
            }
            else if (PRESSED_KEYSC(SPACE) || PRESSED_KEYSC(ENTER))   // play/stop sound
            {
                int32_t j = cursnd+curofs;
                int32_t k = g_sndnum[j];

                if (S_CheckSoundPlaying(0, k) > 0)
                    S_StopSound(k);
                else
                    S_PlaySound(k);
            }
            else if (PRESSED_KEYSC(HOME))    // goto first sound#
            {
                cursnd = curofs = 0;
            }
            else if (PRESSED_KEYSC(END))    // goto last sound#
            {
                if ((cursnd=g_numsounds-32/*SOUND_NUMDISPLINES*/) >= 0)
                    curofs=32/*SOUND_NUMDISPLINES*/-1;
                else
                {
                    cursnd = 0;
                    curofs = g_numsounds-1;
                }
            }

            _printmessage16("                          FILE NAME         PITCH RANGE  PRI FLAGS VOLUME");
            for (i=0; j=cursnd+i, i<SOUND_NUMDISPLINES && j<g_numsounds; i++)
            {
                int32_t l, m, k=g_sndnum[j];
                sound_t *snd=&g_sounds[k];
                char *cp;

                if (ydim-overridepm16y+28+i*9+32 >= ydim) break;

                Bsprintf(disptext[i],
                         "%4d .................... ................ %6d:%-6d %3d %c%c%c%c%c %6d",
                         //   5678901234567890X23456789012345678901234567
                         k, snd->ps, snd->pe, snd->pr,
                         snd->m&1 ? 'R':'-', snd->m&2 ? 'M':'-', snd->m&4 ? 'D':'-',
                         snd->m&8 ? 'P':'-', snd->m&16 ? 'G':'-', snd->vo);
                for (l = Bsnprintf(disptext[i]+5, 20, "%s", snd->definedname); l<20; l++)
                    disptext[i][5+l] = ' ';
                if (snd->filename)
                {
                    l = Bstrlen(snd->filename);
                    if (l<=16)
                        cp = snd->filename;
                    else
                        cp = snd->filename + l-15;
                    for (m = Bsnprintf(disptext[i]+26, 16, "%s", cp); m<16; m++)
                        disptext[i][26+m] = ' ';
                    if (l>16)
                        disptext[i][26] = disptext[i][27] = disptext[i][28] = '.';
                }

                printext16(8, ydim-overridepm16y+28+i*9,
                           keystatus[KEYSC_S]?editorcolors[8] : (S_CheckSoundPlaying(-1, k) ? editorcolors[2] : editorcolors[10]),
                           j==cursnd+curofs ? editorcolors[1] : -1,
                           disptext[i], 0);
            }

            if (keystatus[KEYSC_S])    // sorting
            {

                char ch, bad=0;

                _printmessage16("Sort by: (S)oundnum (D)ef (F)ile ori(g) or flags (12345)");
                showframe(1);

                i=0;
                bflushchars();
                while (bad == 0)
                {
                    idle_waitevent();
                    if (handleevents())
                        quitevent = 0;

                    ch = bgetchar();

                    if (keystatus[1]) bad = 1;

                    else if (ch == 's' || ch == 'd' || ch == 'f' || ch == 'g' ||
                             ch == '1' || ch == '2' || ch == '3' || ch == '4' || ch == '5')
                    {
                        bad = 2;
                        sort_sounds(ch);
                    }
                }

                if (bad==1)
                {
                    keystatus[KEYSC_ESC] = keystatus[KEYSC_Q] = keystatus[KEYSC_F2] = 0;
                }

                if (bad==2)
                {
                    keystatus[KEYSC_S] = keystatus[KEYSC_D] = keystatus[KEYSC_F] = 0;
                    keystatus[KEYSC_G] = keystatus[KEYSC_1] = keystatus[KEYSC_2] = 0;
                    keystatus[KEYSC_3] = keystatus[KEYSC_4] = keystatus[KEYSC_5] = 0;
                }
            }
            else
                showframe(1);
        }

        overridepm16y = -1;
        //        i=ydim16;
        //        ydim16=ydim;
        //        drawline16(0,ydim-STATUS2DSIZ,xdim-1,ydim-STATUS2DSIZ,editorcolors[1]);
        //        ydim16=i;
        //        // printmessage16("");
        //        showframe(1);

        FX_StopAllSounds();
        S_ClearSoundLocks();
        //        SoundToggle = 0;

        keystatus[KEYSC_ESC] = keystatus[KEYSC_Q] = keystatus[KEYSC_F2] = 0;
    }
}
// PK_ ^^^^

// from sector.c
static int32_t dist(spritetype *s1,spritetype *s2)
{
    int32_t x = klabs(s1->x-s2->x);
    int32_t y = klabs(s1->y-s2->y);
    int32_t z = klabs((s1->z-s2->z)>>4);

    if (x<y) swaplong(&x,&y);
    if (x<z) swaplong(&x,&z);

    {
        int32_t t = y + z;
        return (x - (x>>4) + (t>>2) + (t>>3));
    }
}

extern int32_t NumVoices;
extern int32_t g_numEnvSoundsPlaying;
int32_t AmbienceToggle = 1; //SoundToggle;
int32_t ParentalLock = 0;
#undef T1
#define T1 (s->filler)

// adapted from actors.c
static void M32_MoveFX(void)
{
    int32_t i, j;
    int32_t x, ht;
    spritetype *s;

    for (i=headspritestat[0]; i>=0; i=nextspritestat[i])
    {
        s = &sprite[i];

        if (s->picnum != MUSICANDSFX)
        {
            if (T1&1)
            {
                T1 &= (~1);
                S_StopEnvSound(s->lotag, i);
            }
        }
        else if (s->sectnum>=0)
        {
            ht = s->hitag;

            if (s->lotag < 999 && (unsigned)sector[s->sectnum].lotag < 9 &&
                    AmbienceToggle && sector[s->sectnum].floorz != sector[s->sectnum].ceilingz)
            {
                if ((g_sounds[s->lotag].m&2))
                {
                    x = dist((spritetype *)&pos,s);
                    if (x < ht && (T1&1) == 0 && FX_VoiceAvailable(g_sounds[s->lotag].pr-1))
                    {
                        if (g_numEnvSoundsPlaying == NumVoices)
                        {
                            for (j = headspritestat[0]; j >= 0; j = nextspritestat[j])
                            {
                                if (s->picnum == MUSICANDSFX && j != i && sprite[j].lotag < 999 &&
                                        (sprite[j].filler&1) == 1 && dist(&sprite[j],(spritetype *)&pos) > x)
                                {
                                    S_StopEnvSound(sprite[j].lotag,j);
                                    break;
                                }

                            }
                            if (j == -1) continue;
                        }
                        A_PlaySound(s->lotag,i);
                        T1 |= 1;
                    }
                    if (x >= ht && (T1&1) == 1)
                    {
                        T1 &= (~1);
                        S_StopEnvSound(s->lotag,i);
                    }
                }
            }
        }
    }
}
#undef T1


static void Show3dText(char *name)
{
    int32_t fp,t;
    uint8_t x=0,y=4,xmax=0,xx=0,col=0;

    if ((fp=kopen4load(name,0)) == -1)
    {
        begindrawing();
        printext256(1*4,4*8,whitecol,-1,"ERROR: file not found.",0);
        enddrawing();
        return;
    }
    t=65;
    begindrawing();
    while (t!=EOF && col<5)
    {
        t = 0;
        if (kread(fp,&t,1)<=0)
            t = EOF;
        while (t!=EOF && t!='\n' && x<250)
        {
            tempbuf[x]=t;
            t = 0;
            if (kread(fp,&t,1)<=0) t = EOF;
            x++;
            if (x>xmax) xmax=x;
        }
        tempbuf[x]=0;
        printext256(xx*4,(y*6)+2,whitecol,-1,tempbuf,1);
        x=0;
        y++;
        if (y>18)
        {
            col++;
            y=6;
            xx+=xmax;
            xmax=0;
        }
    }
    enddrawing();

    kclose(fp);
}// end Show3dText

///__ShowHelpText__

void ExtShowSpriteData(int16_t spritenum)   //F6
{
    UNREFERENCED_PARAMETER(spritenum);
    if (qsetmode != 200)
        Show2dText("sehelp.hlp");
    /*    if (qsetmode == 200)                // In 3D mode
    return;

    while (KEY_PRESSED(KEYSC_F6));
    ResetKeys();
    ContextHelp(spritenum);             // Get context sensitive help */
}// end ExtShowSpriteData

// Floor Over Floor (duke3d)

// If standing in sector with SE42 or SE44
// then draw viewing to SE41 and raise all =hi SE43 cielings.

// If standing in sector with SE43 or SE45
// then draw viewing to SE40 and lower all =hi SE42 floors.

static int32_t fofsizex = -1;
static int32_t fofsizey = -1;
#if 0
static void ResetFOFSize()
{
    if (fofsizex != -1) tilesizx[FOF] = fofsizex;
    if (fofsizey != -1) tilesizy[FOF] = fofsizey;
}
#endif
static void ExtSE40Draw(int32_t spnum,int32_t x,int32_t y,int32_t z,int16_t a,int16_t h)
{
    static int32_t tempsectorz[MAXSECTORS];
    static int32_t tempsectorpicnum[MAXSECTORS];

    int32_t i=0,j=0,k=0;
    int32_t floor1=0,floor2=0,ok=0,fofmode=0,draw_both=0;
    int32_t offx,offy,offz;

    if (sprite[spnum].ang!=512) return;

    // Things are a little different now, as we allow for masked transparent
    // floors and ceilings. So the FOF textures is no longer required
    //	if (!(gotpic[FOF>>3]&(1<<(FOF&7))))
    //		return;
    //	gotpic[FOF>>3] &= ~(1<<(FOF&7));

    if (tilesizx[562])
    {
        fofsizex = tilesizx[562];
        tilesizx[562] = 0;
    }
    if (tilesizy[562])
    {
        fofsizey = tilesizy[562];
        tilesizy[562] = 0;
    }

    floor1=spnum;

    if (sprite[spnum].lotag==42) fofmode=40;
    if (sprite[spnum].lotag==43) fofmode=41;
    if (sprite[spnum].lotag==44) fofmode=40;
    if (sprite[spnum].lotag==45) fofmode=41;

    // fofmode=sprite[spnum].lotag-2;

    // sectnum=sprite[j].sectnum;
    // sectnum=cursectnum;
    ok++;

    /*  recursive?
    for(j=0;j<MAXSPRITES;j++)
    {
    if(
    sprite[j].sectnum==sectnum &&
    sprite[j].picnum==1 &&
    sprite[j].lotag==110
    ) { DrawFloorOverFloor(j); break;}
    }
    */

    // if(ok==0) { Message("no fof",RED); return; }

    for (j=0; j<MAXSPRITES; j++)
    {
        if (sprite[j].picnum==1 && sprite[j].lotag==fofmode && sprite[j].hitag==sprite[floor1].hitag)
        {
            floor1=j;
            fofmode=sprite[j].lotag;
            ok++;
            break;
        }
    }
    // if(ok==1) { Message("no floor1",RED); return; }

    if (fofmode==40) k=41;
    else k=40;

    for (j=0; j<MAXSPRITES; j++)
    {
        if (sprite[j].picnum==1 && sprite[j].lotag==k && sprite[j].hitag==sprite[floor1].hitag)
        {
            floor2=j;
            ok++;
            break;
        }
    }

    i=floor1;
    offx=sprite[floor2].x-sprite[floor1].x;
    offy=sprite[floor2].y-sprite[floor1].y;
    offz=0;

    if (sprite[floor2].ang >= 1024)
        offz = sprite[floor2].z;
    else if (fofmode==41)
        offz = SPRITESEC(floor2).floorz;
    else
        offz = SPRITESEC(floor2).ceilingz;

    if (sprite[floor1].ang >= 1024)
        offz -= sprite[floor1].z;
    else if (fofmode==40)
        offz -= SPRITESEC(floor1).floorz;
    else
        offz -= SPRITESEC(floor1).ceilingz;

    // if(ok==2) { Message("no floor2",RED); return; }

    for (j=0; j<MAXSPRITES; j++) // raise ceiling or floor
    {
        if (sprite[j].picnum==1 && sprite[j].lotag==k+2 && sprite[j].hitag==sprite[floor1].hitag)
        {
            if (k==40)
            {
                tempsectorz[sprite[j].sectnum] = SPRITESEC(j).floorz;
                SPRITESEC(j).floorz += (((z-SPRITESEC(j).floorz)/32768)+1)*32768;
                tempsectorpicnum[sprite[j].sectnum] = SPRITESEC(j).floorpicnum;
                SPRITESEC(j).floorpicnum = 562;
            }
            else
            {
                tempsectorz[sprite[j].sectnum] = SPRITESEC(j).ceilingz;
                SPRITESEC(j).ceilingz += (((z-SPRITESEC(j).ceilingz)/32768)-1)*32768;
                tempsectorpicnum[sprite[j].sectnum] = SPRITESEC(j).ceilingpicnum;
                SPRITESEC(j).ceilingpicnum = 562;
            }
            draw_both = 1;
        }
    }

    drawrooms(x+offx,y+offy,z+offz,a,h,sprite[floor2].sectnum);
    ExtAnalyzeSprites();
    drawmasks();

    if (draw_both)
    {
        for (j=0; j<MAXSPRITES; j++) // restore ceiling or floor for the draw both sectors
        {
            if (sprite[j].picnum==SECTOREFFECTOR &&
                    sprite[j].lotag==k+2 && sprite[j].hitag==sprite[floor1].hitag)
            {
                if (k==40)
                {
                    SPRITESEC(j).floorz = tempsectorz[sprite[j].sectnum];
                    SPRITESEC(j).floorpicnum = tempsectorpicnum[sprite[j].sectnum];
                }
                else
                {
                    SPRITESEC(j).ceilingz = tempsectorz[sprite[j].sectnum];
                    SPRITESEC(j).ceilingpicnum = tempsectorpicnum[sprite[j].sectnum];
                }
            }// end if
        }// end for

        // Now re-draw
        drawrooms(x+offx,y+offy,z+offz,a,h,sprite[floor2].sectnum);
        ExtAnalyzeSprites();
        drawmasks();
    }

} // end SE40

static void SE40Code(int32_t x,int32_t y,int32_t z,int32_t a,int32_t h)
{
    int32_t i;

    i = 0;
    while (i<MAXSPRITES)
    {
        int32_t t = sprite[i].lotag;
        switch (t)
        {
            //            case 40:
            //            case 41:
            //                ExtSE40Draw(i,x,y,z,a,h);
            //                break;
        case 42:
        case 43:
        case 44:
        case 45:
            if (cursectnum == sprite[i].sectnum)
                ExtSE40Draw(i,x,y,z,a,h);
            break;
        }
        i++;
    }
}

void ExtEditSectorData(int16_t sectnum)    //F7
{
    //    if (qsetmode != 200) Show2dText("sthelp.hlp");
    if (qsetmode == 200)
        return;

    if (eitherALT)  //ALT
    {
        keystatus[KEYSC_F7] = 0;
        wallsprite=0;
        curwall = 0;
        curwallnum = 0;
        cursearchspritenum = 0;
        cursectornum=0;
        cursector_lotag = sector[sectnum].lotag;
        cursector_lotag = getnumber16("Enter search sector lotag : ", cursector_lotag, BTAG_MAX,0);
        _printmessage16("Search sector lotag %d",cursector_lotag);
    }
    else EditSectorData(sectnum);
}// end ExtEditSectorData

void ExtEditWallData(int16_t wallnum)       //F8
{
    if (qsetmode==200)
        return;

    if (eitherALT)  //ALT
    {
        wallsprite=1;
        curwall = wallnum;
        curwallnum = 0;
        cursearchspritenum = 0;
        cursectornum = 0;
        search_lotag = wall[curwall].lotag;
        search_hitag = wall[curwall].hitag;
        search_lotag = getnumber16("Enter wall search lotag : ", search_lotag, BTAG_MAX,0);
        search_hitag = getnumber16("Enter wall search hitag : ", search_hitag, BTAG_MAX,0);
        //    Bsprintf(tempbuf,"Current wall %d lo=%d hi=%d",
        //             curwall,wall[curwall].lotag,wall[curwall].hitag);
        _printmessage16("Search wall lo=%d hi=%d",search_lotag,search_hitag);
    }
    else EditWallData(wallnum);
}

static void GenericSpriteSearch(void);

void ExtEditSpriteData(int16_t spritenum)   //F8
{
    if (qsetmode==200)
        return;

    if (eitherALT)  //ALT
        GenericSpriteSearch();
#if 0
    {
        wallsprite=2;
        cursearchsprite = spritenum;
        curwallnum = 0;
        cursearchspritenum = 0;
        cursectornum = 0;
        search_lotag = sprite[cursearchsprite].lotag;
        search_hitag = sprite[cursearchsprite].hitag;
        search_lotag = getnumber16("Enter sprite search lotag : ", search_lotag, BTAG_MAX,0);
        search_hitag = getnumber16("Enter sprite search hitag : ", search_hitag, BTAG_MAX,0);
        printmessage16("Search sprite lo=%d hi=%d",search_lotag,search_hitag);
    }
#endif
    else EditSpriteData(spritenum);
}

static void PrintStatus(const char *string,int32_t num,char x,char y,char color)
{
    Bsprintf(tempbuf,"%s %d",string,num);
    begindrawing();
    printext16(x*8,ydim-STATUS2DSIZ+y*8,editorcolors[(int32_t)color],-1,tempbuf,0);
    enddrawing();
}

static inline void SpriteName(int16_t spritenum, char *lo2)
{
    Bstrcpy(lo2, names[sprite[spritenum].picnum]);
}// end SpriteName

static void ReadPaletteTable()
{
    int32_t i,j,fp;
    char lookup_num;

    for (j = 0; j < 256; j++)
        tempbuf[j] = j;

    for (i=1; i<MAXPALOOKUPS; i++)
        makepalookup(i,tempbuf,0,0,0,1);

    if ((fp=kopen4load("lookup.dat",0)) == -1)
    {
        if ((fp=kopen4load("lookup.dat",1)) == -1)
        {
            initprintf("LOOKUP.DAT not found\n");
            return;
        }
    }
    //    initprintf("Loading palette lookups... ");
    kread(fp,&num_tables,1);
    for (j=0; j<num_tables; j++)
    {
        kread(fp,&lookup_num,1);
        kread(fp,tempbuf,256);
        makepalookup(lookup_num,tempbuf,0,0,0,1);
    }
    for (j = 0; j < 256; j++)
        tempbuf[j] = j;

    num_tables++;
    makepalookup(num_tables, tempbuf, 15, 15, 15, 1);
    makepalookup(num_tables + 1, tempbuf, 15, 0, 0, 1);
    makepalookup(num_tables + 2, tempbuf, 0, 15, 0, 1);
    makepalookup(num_tables + 3, tempbuf, 0, 0, 15, 1);

    kread(fp,WATERpalette,768);
    kread(fp,SLIMEpalette,768);
    kread(fp,TITLEpalette,768);
    kread(fp,REALMSpalette,768);
    kread(fp,BOSS1palette,768);
    kclose(fp);
    //    initprintf("success.\n");
}// end ReadPaletteTable

static void ReadGamePalette()
{
    int32_t fp;
    if ((fp=kopen4load("palette.dat",0)) == -1)
        if ((fp=kopen4load("palette.dat",1)) == -1)
        {
            initprintf("!!! PALETTE.DAT NOT FOUND !!!\n");
            Bstrcpy(tempbuf, "Mapster32"VERSION BUILDDATE);
            wm_msgbox(tempbuf,"palette.dat not found");
            exit(0);
        }
    //    initprintf("Loading game palette... ");
    kread(fp,GAMEpalette,768);
    kclose(fp);
    //    initprintf("success.\n");
    ReadPaletteTable();
}

static void m32_showmouse(void)
{
    int32_t i, col;

    if (totalclock > lastupdate)
    {
        mousecol += mouseadd;
        if (mousecol >= 30 || mousecol <= 0)
        {
            mouseadd = -mouseadd;
            mousecol += mouseadd;
        }
        lastupdate = totalclock + 3;
    }

    switch (whitecol)
    {
    case 1:  // Shadow Warrior
        col = whitecol+mousecol;
        break;
    case 31: // Duke Nukem 3D
        col = whitecol-mousecol;
        break;
    default:
        col = whitecol;
        break;
    }

    if (col != whitecol)
    {
        for (i=((xdim > 640)?3:2); i<=((xdim > 640)?7:3); i++)
        {
            plotpixel(searchx+i,searchy,col);
            plotpixel(searchx-i,searchy,col);
            plotpixel(searchx,searchy-i,col);
            plotpixel(searchx,searchy+i,col);
        }

        for (i=1; i<=((xdim > 640)?2:1); i++)
        {
            plotpixel(searchx+i,searchy,whitecol);
            plotpixel(searchx-i,searchy,whitecol);
            plotpixel(searchx,searchy-i,whitecol);
            plotpixel(searchx,searchy+i,whitecol);
        }

        i = (xdim > 640)?8:4;

        plotpixel(searchx+i,searchy,0);
        plotpixel(searchx-i,searchy,0);
        plotpixel(searchx,searchy-i,0);
        plotpixel(searchx,searchy+i,0);
    }

    if (xdim > 640)
    {
        for (i=1; i<=4; i++)
        {
            plotpixel(searchx+i,searchy,whitecol);
            plotpixel(searchx-i,searchy,whitecol);
            plotpixel(searchx,searchy-i,whitecol);
            plotpixel(searchx,searchy+i,whitecol);
        }
    }
}

static int32_t AskIfSure(char *text)
{
    int32_t retval=1;

    if (qsetmode == 200)
    {
        begindrawing(); //{{{
        printext256(0,0,whitecol,0,text?text:"Are you sure you want to proceed?",0);
        enddrawing();   //}}}
    }
    else
    {
        _printmessage16(text?text:"Are you sure you want to proceed?");
    }

    showframe(1);

    while ((keystatus[KEYSC_ESC]|keystatus[KEYSC_ENTER]|keystatus[KEYSC_SPACE]|keystatus[KEYSC_N]) == 0)
    {
        idle_waitevent();

        if (handleevents())
        {
            if (quitevent)
            {
                retval = 1;
                break;
            }
        }

        if (PRESSED_KEYSC(Y) || PRESSED_KEYSC(ENTER))
        {
            retval = 0;
            break;
        }
    }

    if (PRESSED_KEYSC(ESC))
        retval = 1;

    return(retval);
}

static int32_t IsValidTile(int32_t idTile)
{
    return (idTile>=0 && idTile<MAXTILES) && (tilesizx[idTile] && tilesizy[idTile]);
}

static int32_t SelectAllTiles(int32_t iCurrentTile)
{
    int32_t i;

    if (iCurrentTile < localartlookupnum)
        iCurrentTile = localartlookup[iCurrentTile];
    else
        iCurrentTile = 0;

    localartlookupnum = MAXTILES;

    for (i = 0; i < MAXTILES; i++)
    {
        localartlookup[i] = i;
        localartfreq[i] = 0;
    }

    return iCurrentTile;
}

static int32_t OnGotoTile(int32_t iTile);
static int32_t OnSelectTile(int32_t iTile);
static int32_t OnSaveTileGroup();
static int32_t loadtilegroups(const char *fn);
static int32_t s_Zoom = INITIAL_ZOOM;
static int32_t s_TileZoom = 1;
static char tilesel_errmsg[128], tilesel_showerr=0;

static int32_t DrawTiles(int32_t iTopLeft, int32_t iSelected, int32_t nXTiles, int32_t nYTiles,
                         int32_t TileDim, int32_t offset, int32_t showmsg);

#define TMPERRMSG_SHOW(alsoOSD) do { \
    printext256(0, 0, whitecol, 0, tilesel_errmsg, 0); \
    if (alsoOSD) OSD_Printf("%s\n", tilesel_errmsg); \
    showframe(1); \
} while (0)

#define TMPERRMSG_PRINT(Msg, ...) do {  \
    Bsprintf(tilesel_errmsg, Msg, __VA_ARGS__); \
    TMPERRMSG_SHOW(1); \
    tilesel_showerr = 1; \
} while (0)

#define TMPERRMSG_RETURN(Msg, ...) do   \
{ \
    TMPERRMSG_PRINT(Msg, __VA_ARGS__);  \
    return 1; \
} while (0)


static int32_t m32gettile(int32_t idInitialTile)
{
    int32_t gap, temp, zoomsz;
    int32_t nXTiles, nYTiles, nDisplayedTiles;
    int32_t i;
    int32_t iTile, iTopLeftTile, iLastTile;
    int32_t idSelectedTile;
    int32_t scrollmode;
    int32_t mousedx, mousedy, mtile, omousex=searchx, omousey=searchy, moffset=0;

    int32_t noTilesMarked=1;
    int32_t mark_lastk = -1;

    // Enable following line for testing. I couldn't work out how to change vidmode on the fly
    // s_Zoom = NUM_ZOOMS - 1;

    idInitialTile = clamp(idInitialTile, 0, MAXTILES-1);

    // Ensure zoom not to big (which can happen if display size
    //   changes whilst Mapster is running)
    do
    {
        nXTiles = xdim / ZoomToThumbSize[s_Zoom];
        nYTiles = ydim / ZoomToThumbSize[s_Zoom];
        // Refuse to draw less than half of a row.
        if (ZoomToThumbSize[s_Zoom]/2 < 12) nYTiles--;
        nDisplayedTiles  = nXTiles * nYTiles;

        if (!nDisplayedTiles)
        {
            // Eh-up, resolution changed since we were last displaying tiles.
            s_Zoom--;
        }
    }
    while (!nDisplayedTiles);

    keystatus[KEYSC_V] = 0;

    for (i = 0; i < MAXTILES; i++)
    {
        localartfreq[i] = 0;
        localartlookup[i] = i;
    }

    iLastTile = iTile = idSelectedTile = idInitialTile;

    switch (searchstat)
    {
    case SEARCH_WALL:
        for (i = 0; i < numwalls; i++)
            localartfreq[ wall[i].picnum ]++;
        break;

    case SEARCH_CEILING:
    case SEARCH_FLOOR:
        for (i = 0; i < numsectors; i++)
        {
            localartfreq[ sector[i].ceilingpicnum ]++;
            localartfreq[ sector[i].floorpicnum ]++;
        }
        break;

    case SEARCH_SPRITE:
        for (i=0; i<MAXSPRITES; i++)
            localartfreq[ sprite[i].picnum ] += (sprite[i].statnum < MAXSTATUS);
        break;

    case SEARCH_MASKWALL:
        for (i = 0; i < numwalls; i++)
            localartfreq[ wall[i].overpicnum ]++;
        break;

    default :
        break;
    }


    //
    //	Sort tiles into frequency order
    //

    gap = MAXTILES / 2;

    do
    {
        for (i = 0; i < MAXTILES-gap; i++)
        {
            temp = i;

            while (localartfreq[temp]<localartfreq[temp+gap] && temp >= 0)
            {
                int32_t tempint;

                tempint = localartfreq[temp];
                localartfreq[temp] = localartfreq[temp+gap];
                localartfreq[temp+gap] = tempint;

                tempint = localartlookup[temp];
                localartlookup[temp] = localartlookup[temp+gap];
                localartlookup[temp+gap] = tempint;

                if (iTile == temp)
                    iTile = temp + gap;
                else if (iTile == temp + gap)
                    iTile = temp;

                temp -= gap;
            }
        }
        gap >>= 1;
    }
    while (gap > 0);

    //
    // Set up count of number of used tiles
    //

    localartlookupnum = 0;
    while (localartfreq[localartlookupnum] > 0)
        localartlookupnum++;

    //
    // Check : If no tiles used at all then switch to displaying all tiles
    //

    if (!localartfreq[0])
    {
        localartlookupnum = MAXTILES;

        for (i = 0; i < MAXTILES; i++)
        {
            localartlookup[i] = i;
            localartfreq[i] = 0; // Terrible bodge : zero tilefreq's not displayed in tile view. Still, when in Rome ... :-)
        }

        iTile = idInitialTile;
    }

    //
    //
    //

    iTopLeftTile = iTile - (iTile % nXTiles);
    iTopLeftTile = clamp(iTopLeftTile, 0, MAXTILES-nDisplayedTiles);

    zoomsz = ZoomToThumbSize[s_Zoom];

    searchx = ((iTile-iTopLeftTile)%nXTiles)*zoomsz + zoomsz/2;
    searchy = ((iTile-iTopLeftTile)/nXTiles)*zoomsz + zoomsz/2;

    ////////////////////////////////
    // Start of key handling code //
    ////////////////////////////////

    while ((keystatus[KEYSC_ENTER]|keystatus[KEYSC_ESC]|(bstatus&1)) == 0) // <- Presumably one of these is escape key ??
    {
        zoomsz = ZoomToThumbSize[s_Zoom];

        DrawTiles(iTopLeftTile, (iTile >= localartlookupnum) ? localartlookupnum-1 : iTile,
                  nXTiles, nYTiles, zoomsz, moffset,
                  (tilesel_showerr && (iTile==iLastTile || (tilesel_showerr=0))));

        iLastTile = iTile;

        idle_waitevent_timeout(500);
        // SDL seems to miss mousewheel events when rotated slowly.
        // These kludgy things seem to make it better, but I'm not sure.

        if (handleevents())
            quitevent = 0;

        getmousevalues(&mousedx,&mousedy,&bstatus);
        searchx += mousedx;
        searchy += mousedy;

        if (bstatus&2)
        {
            moffset += mousedy*2;
            searchy += mousedy;
            searchx -= mousedx;

            if ((moffset < 0 && iTopLeftTile > localartlookupnum-nDisplayedTiles-1)
                    || (moffset > 0 && iTopLeftTile==0))
            {
                moffset=0;
                searchy -= mousedy*2;
            }

            while (moffset > zoomsz)
            {
                iTopLeftTile -= nXTiles;
                moffset -= zoomsz;
            }
            while (moffset < -zoomsz)
            {
                iTopLeftTile += nXTiles;
                moffset += zoomsz;
            }
        }

        // Keep the pointer visible at all times.
        temp = min(zoomsz/2, 12);
        searchx = clamp(searchx, temp, xdim-temp);
        searchy = clamp(searchy, temp, ydim-temp);

        scrollmode = !(eitherCTRL^revertCTRL);
        if (bstatus&16 && scrollmode && iTopLeftTile > 0)
        {
            mouseb &= ~16;
            iTopLeftTile -= (nXTiles*scrollamount);
        }
        else if (bstatus&32 && scrollmode && iTopLeftTile < localartlookupnum-nDisplayedTiles-1)
        {
            mouseb &= ~32;
            iTopLeftTile += (nXTiles*scrollamount);
        }

        mtile = iTile = searchx/zoomsz + ((searchy-moffset)/zoomsz)*nXTiles + iTopLeftTile;
        while (iTile >= iTopLeftTile + nDisplayedTiles)
        {
            iTile -= nXTiles;
            mtile = iTile;
        }

        // These two lines are so obvious I don't need to comment them ...;-)
        synctics = totalclock-lockclock;
        lockclock += synctics;

        // Zoom in / out using numeric key pad's / and * keys
        if (((keystatus[KEYSC_gSLASH] || (!scrollmode && bstatus&16)) && s_Zoom<(signed)(NUM_ZOOMS-1))
                || ((keystatus[KEYSC_gSTAR]  || (!scrollmode && bstatus&32)) && s_Zoom>0))
        {
            if (PRESSED_KEYSC(gSLASH) || (!scrollmode && bstatus&16))
            {
                mouseb &= ~16;
                bstatus &= ~16;

                // Watch out : If editor window is small, then the next zoom level
                //  might get so large that even one tile might not fit !
                if (ZoomToThumbSize[s_Zoom+1]<=xdim && ZoomToThumbSize[s_Zoom+1]<=ydim)
                {
                    // Phew, plenty of room.
                    s_Zoom++;
                }
            }
            else
            {
                keystatus[KEYSC_gSTAR] = 0;
                mouseb &= ~32;
                bstatus &= ~32;
                s_Zoom--;
            }

            zoomsz = ZoomToThumbSize[s_Zoom];

            if (iTile >= localartlookupnum)
                iTile = localartlookupnum-1;

            // Calculate new num of tiles to display
            nXTiles = xdim / zoomsz;
            nYTiles = ydim / zoomsz;
            // Refuse to draw less than half of a row.
            if (zoomsz/2 < 12)
                nYTiles--;
            nDisplayedTiles  = nXTiles * nYTiles;

            // Determine if the top-left displayed tile needs to
            //   alter in order to display selected tile
            iTopLeftTile = iTile - (iTile % nXTiles);
            iTopLeftTile = clamp(iTopLeftTile, 0, MAXTILES - nDisplayedTiles);

            // scroll window so mouse points the same tile as it was before zooming
            iTopLeftTile -= searchx/zoomsz + ((searchy-moffset)/zoomsz)*nXTiles + iTopLeftTile-iTile;
        }

        if (PRESSED_KEYSC(LEFT))
        {
            if (eitherCTRL)  // same as HOME, for consistency with CTRL-UP/DOWN
                iTile = (iTile/nXTiles)*nXTiles;
            else
                iTile--;
        }

        if (PRESSED_KEYSC(RIGHT))
        {
            if (eitherCTRL)  // same as END, for consistency with CTRL-UP/DOWN
                iTile = ((iTile+nXTiles)/nXTiles)*nXTiles - 1;
            else
                iTile++;
        }

        if (PRESSED_KEYSC(UP))
        {
            if (eitherCTRL)
                while (iTile-nXTiles >= iTopLeftTile)
                    iTile -= nXTiles;
            else
                iTile -= nXTiles;
        }

        if (PRESSED_KEYSC(DOWN))
        {
            if (eitherCTRL)
                while (iTile+nXTiles < iTopLeftTile + nDisplayedTiles)
                    iTile += nXTiles;
            else
                iTile += nXTiles;
        }

        if (PRESSED_KEYSC(PGUP))
        {
            if (eitherCTRL)
                iTile = 0;
            else
                iTile -= nDisplayedTiles;
        }

        if (PRESSED_KEYSC(PGDN))
        {
            if (eitherCTRL)
                iTile = localartlookupnum-1;
            else
                iTile += nDisplayedTiles;
        }

        if (PRESSED_KEYSC(HOME))
        {
            if (eitherCTRL)
                iTile = iTopLeftTile;
            else
                iTile = (iTile/nXTiles)*nXTiles;
        }

        if (PRESSED_KEYSC(END))
        {
            if (eitherCTRL)
                iTile = iTopLeftTile + nDisplayedTiles - 1;
            else
                iTile = ((iTile+nXTiles)/nXTiles)*nXTiles - 1;
        }

        // 'V'  KEYPRESS
        if (PRESSED_KEYSC(V))
            iTile = SelectAllTiles(iTile);

        // 'G'  KEYPRESS - Goto frame
        if (PRESSED_KEYSC(G))
        {
            if (eitherCTRL)
            {
                if (OnSaveTileGroup() == 0)
                {
//                    iTile = SelectAllTiles(iTile);
                    Bmemset(tilemarked, 0, sizeof(tilemarked));
                    mark_lastk = -1;
                    noTilesMarked = 1;
                }
            }
            else
                iTile = OnGotoTile(iTile);
        }

        // 'U'  KEYPRESS : go straight to user defined art
        if (PRESSED_KEYSC(U))
        {
            SelectAllTiles(iTile);
            iTile = FIRST_USER_ART_TILE;
        }

        // 'A'  KEYPRESS : Go straight to start of Atomic edition's art
        if (PRESSED_KEYSC(A))
        {
            SelectAllTiles(iTile);
            iTile = FIRST_ATOMIC_TILE;
        }

        // 'E'  KEYPRESS : Go straight to start of extended art
        if (PRESSED_KEYSC(E))
        {
            SelectAllTiles(iTile);

            if (iTile == FIRST_EXTENDED_TILE)
                iTile = SECOND_EXTENDED_TILE;
            else iTile = FIRST_EXTENDED_TILE;
        }

        // 'T' KEYPRESS = Select from pre-defined tileset
        if (PRESSED_KEYSC(T))
            iTile = OnSelectTile(iTile);

        if (PRESSED_KEYSC(Z))
            s_TileZoom = !s_TileZoom;

        //
        // Ensure tilenum is within valid range
        //
        iTile = clamp(iTile, 0, min(MAXTILES-1, localartlookupnum+nDisplayedTiles-1));


        // 'S' KEYPRESS: search for named tile
        if (PRESSED_KEYSC(S))
        {
            static char laststr[MAXTILES] = "";
            const char *searchstr = getstring_simple("Search for tile name: ", laststr, MAXTILES-1);

            if (searchstr && searchstr[0])
            {
                int32_t i, i0, slen=Bstrlen(searchstr)-1;

                Bstrcpy(laststr, searchstr);
                i0 = localartlookup[iTile];

                for (i=(i0+1)%MAXTILES; i!=i0; i=(i+1)%MAXTILES)
                    if ((searchstr[0]=='^' && !Bstrncmp(names[i], searchstr+1, slen)) ||
                        (searchstr[0]!='^' && strstr(names[i], searchstr)))
                    {
                        SelectAllTiles(i);
                        iTile = i;
                        break;
                    }
            }
        }

        //
        //	Adjust top-left to ensure tilenum is within displayed range of tiles
        //

        while (iTile < iTopLeftTile - (moffset<0)?nXTiles:0)
            iTopLeftTile -= nXTiles;

        while (iTile >= iTopLeftTile + nDisplayedTiles)
            iTopLeftTile += nXTiles;

        iTopLeftTile = clamp(iTopLeftTile, 0, MAXTILES - nDisplayedTiles);


        // SPACE keypress: mark/unmark selected tile
        if (PRESSED_KEYSC(SPACE))
        {
            if (iTile < localartlookupnum && IsValidTile(localartlookup[iTile]))
            {
                if (keystatus[KEYSC_LCTRL] && keystatus[KEYSC_RSHIFT])
                {
                    Bmemset(tilemarked, 0, sizeof(tilemarked));
                    mark_lastk = -1;
                    noTilesMarked = 1;
                }
                else
                {
                    int32_t k=iTile, kend, dir;

                    if (noTilesMarked)
                    {
                        noTilesMarked = 0;
                        TMPERRMSG_PRINT("%s", "Beginning marking tiles. To group, press Ctrl-G. To reset, press LCtrl-RShift-SPACE.");
                    }

                    if (mark_lastk>=0 && eitherCTRL)
                    {
                        kend = mark_lastk;
                        dir = ksgn(mark_lastk-k);
                    }
                    else
                    {
                        kend = k;
                        dir = 0;
                    }

                    mark_lastk = k;

                    for (; dir==0 || dir*(kend-k)>=1; k+=dir)
                    {
                        tilemarked[localartlookup[k]>>3] ^= (1<<(localartlookup[k]&7));
                        if (dir==0)
                            break;
                    }
                }
            }
        }

        if ((keystatus[KEYSC_ENTER] || (bstatus&1)) == 0)   // uh ? Not escape key ?
        {
            idSelectedTile = idInitialTile;
        }
        else
        {
            if (iTile < localartlookupnum)
            {
                // Convert tile num from index to actual tile num
                idSelectedTile = localartlookup[iTile];

                // Check : if invalid tile selected, return original tile num
                if (!IsValidTile(idSelectedTile))
                    idSelectedTile = idInitialTile;
            }
            else
            {
                idSelectedTile = idInitialTile;
            }
        }
        if (mtile!=iTile) // if changed by keyboard, update mouse cursor
        {
            searchx = ((iTile-iTopLeftTile)%nXTiles) * zoomsz + zoomsz/2;
            searchy = ((iTile-iTopLeftTile)/nXTiles) * zoomsz + zoomsz/2 + moffset;
        }
    }

    searchx=omousex;
    searchy=omousey;

    keystatus[KEYSC_ESC] = 0;
    keystatus[KEYSC_ENTER] = 0;

    return idSelectedTile;

}

// Dir = 0 (zoom out) or 1 (zoom in)
//void OnZoomInOut( int32_t *pZoom, int32_t Dir /*0*/ )
//{
//}

static int32_t OnSaveTileGroup()
{
    int32_t i, n=0;
    char hotkey;
    const char *cp, *name;

    if (tile_groups==MAX_TILE_GROUPS)
        TMPERRMSG_RETURN("Cannot save tile group: maximum number of groups (%d) exceeded.", MAX_TILE_GROUPS);

    for (i=0; i<MAXTILES; i++)
        n += !!(tilemarked[i>>3]&(1<<(i&7)));

    if (n==0)
        TMPERRMSG_RETURN("%s", "Cannot save tile group: no tiles marked.");
    else if (n > MAX_TILE_GROUP_ENTRIES)
        TMPERRMSG_RETURN("Cannot save tile group: too many tiles in group. Have %d, max is %d.",
                  n, MAX_TILE_GROUP_ENTRIES);

    cp = getstring_simple("Hotkey for new group: ", "", 1);
    if (!cp || !*cp)
        return 1;

    hotkey = Btoupper(cp[0]);
    if (!isalpha(hotkey))
        TMPERRMSG_RETURN("%s", "Hotkey must be alphabetic.");

    for (i=0; i<tile_groups; i++)
        if (s_TileGroups[i].key1==hotkey || s_TileGroups[i].key2==Btolower(hotkey))
            TMPERRMSG_RETURN("Hotkey '%c' already in use by tile group `%s'.", hotkey, s_TileGroups[i].szText);

    name = getstring_simple("Name for new tile group: ", "", 0);
    if (!name || !*name)
        return 1;

    for (i=0; name[i]; i++)
        if (!(isalnum(name[i]) || name[i]==' ' || name[i]==','))
            TMPERRMSG_RETURN("%s", "Name may only consist of alphabetic, numeric and space characters.");

    {
        int32_t lasti=-1, col=0, j, k, opathsearchmode=pathsearchmode;
        BFILE *fp;

        pathsearchmode = pathsearchmode_oninit;  // use the same pathsearchmode as on init
        fp = fopenfrompath(default_tiles_cfg, "a");
        pathsearchmode = opathsearchmode;
        if (!fp)
            TMPERRMSG_RETURN("Could not open `%s' for writing: %s.", default_tiles_cfg, strerror(errno));
        if (fseek(fp, 0, BSEEK_END))  // seems to be necessary even though we fopen with "a"
            TMPERRMSG_RETURN("Could not seek to end of file `%s'.", default_tiles_cfg);

#define TTAB "\t"
#define TBITCHK(i) ((i)<MAXTILES && (tilemarked[(i)>>3]&(1<<((i)&7))))
        Bfprintf(fp, "\n");
        Bfprintf(fp, "tilegroup \"%s\"\n{\n", name);
        Bfprintf(fp, TTAB "hotkey \"%c\"\n\n", hotkey);

        if (!(s_TileGroups[tile_groups].pIds = Bmalloc(n * sizeof(s_TileGroups[tile_groups].pIds[0]))))
            TMPERRMSG_RETURN("%s", "Out of memory.");

        j = 0;
        // tileranges for consecutive runs of 3 or more tiles
        for (i=0; i<MAXTILES; i++)
        {
            if (lasti>=0 && !TBITCHK(i))
            {
                if (names[lasti][0] && names[i-1][0])
                    Bfprintf(fp, TTAB "tilerange %s %s\n", names[lasti], names[i-1]);
                else
                    Bfprintf(fp, TTAB "tilerange %d %d\n", lasti, i-1);

                for (k=lasti; k<i; k++)
                {
                    s_TileGroups[tile_groups].pIds[j++] = k;
                    tilemarked[k>>3] &= ~(1<<(k&7));
                }

                lasti = -1;
            }
            else if (lasti==-1 && TBITCHK(i))
            {
                if (TBITCHK(i+1) && TBITCHK(i+2))
                {
                    lasti = i;
                    i += 2;
                }
            }
        }
        if (lasti>=0 && lasti<=MAXTILES-3)
        {
            for (k=lasti; k<MAXTILES; k++)
            {
                s_TileGroups[tile_groups].pIds[j++] = k;
                tilemarked[k>>3] &= ~(1<<(k&7));
            }
            Bfprintf(fp, TTAB "tilerange %d %d\n", lasti, MAXTILES-1);
        }
        Bfprintf(fp, "\n");

        // throw them all in a tiles{...} group else
        Bfprintf(fp, TTAB "tiles\n" TTAB "{\n");
        for (i=0; i<MAXTILES; i++)
        {
            if (TBITCHK(i))
            {
                s_TileGroups[tile_groups].pIds[j++] = i;

                if (col==0)
                    Bfprintf(fp, TTAB TTAB), col+=8;

                if (names[i][0])
                    col+=Bfprintf(fp, "%s ", names[i]);
                else
                    col+=Bfprintf(fp, "%d ", i);

                if (col>80)
                {
                    Bfprintf(fp, "\n");
                    col = 0;
                }
            }
        }
        if (col>0)
            Bfprintf(fp, "\n");
        Bfprintf(fp, TTAB "}\n");
#undef TBITCHK
#undef TTAB
        Bfprintf(fp, "}\n");

        Bfclose(fp);

        if (!(s_TileGroups[tile_groups].szText = Bstrdup(name)))
        {
            Bfree(s_TileGroups[tile_groups].pIds);
            TMPERRMSG_RETURN("%s", "Out of memory.");
        }

        s_TileGroups[tile_groups].nIds = n;
        s_TileGroups[tile_groups].key1 = Btoupper(hotkey);
        s_TileGroups[tile_groups].key2 = Btolower(hotkey);
        s_TileGroups[tile_groups].color1 = s_TileGroups[tile_groups].color2 = 0;
        tile_groups++;

        TMPERRMSG_PRINT("%s", "Wrote and installed new tile group.");
    }

    return 0;
}


static int32_t OnGotoTile(int32_t iTile)
{
    int32_t iTemp, iNewTile;
    char ch;
    char szTemp[128];

    //Automatically press 'V'
    iTile = SelectAllTiles(iTile);

    bflushchars();

    iNewTile = iTemp = 0; //iTile; //PK

    while (keystatus[KEYSC_ESC] == 0)
    {
        if (handleevents())
            quitevent = 0;

        idle_waitevent();

        ch = bgetchar();

        Bsprintf(szTemp, "Goto tile: %d_ ", iNewTile);
        printext256(0, 0, whitecol, 0, szTemp, 0);
        showframe(1);

        if (ch >= '0' && ch <= '9')
        {
            iTemp = (iNewTile*10) + (ch-'0');
            if (iTemp < MAXTILES)
                iNewTile = iTemp;
        }
        else if (ch == 8)
        {
            iNewTile /= 10;
        }
        else if (ch == 13)
        {
            iTile = iNewTile;
            break;
        }
    }

    clearkeys();

    return iTile;
}


static int32_t LoadTileSet(const int32_t idCurrentTile, const int32_t *pIds, const int32_t nIds)
{
    int32_t iNewTile = 0;
    int32_t i;

    localartlookupnum = nIds;

    for (i = 0; i < localartlookupnum; i++)
    {
        localartlookup[i] = pIds[i];
        // REM : Could we still utilise localartfreq[] to mark
        //  which tiles are currently used in the map ? Set to 0xFFFF perhaps ?
        localartfreq[i] = 0;

        if (idCurrentTile == pIds[i])
            iNewTile = i;
    }

    return iNewTile;
}

static int32_t OnSelectTile(int32_t iTile)
{
    int32_t bDone = 0;
    int32_t i;
    char ch;

    if (tile_groups <= 0)
    {
        TMPERRMSG_PRINT("No tile groups loaded. Check for existence of `%s'.", default_tiles_cfg);
        return iTile;
    }

    SelectAllTiles(iTile);

    bflushchars();

    begindrawing();
    setpolymost2dview();
    clearview(0);

    //
    //	Await appropriate selection keypress.
    //

    bDone = 0;

    while (keystatus[KEYSC_ESC] == 0 && !bDone)
    {
        if (handleevents())
            quitevent = 0;

        idle_waitevent();

        //
        // Display the description strings for each available tile group
        //
        for (i = 0; i < tile_groups; i++)
        {
            if (s_TileGroups[i].szText != NULL)
            {
                if ((i+2)*16 > ydimgame) break;
                Bsprintf(tempbuf,"(%c) %s",s_TileGroups[i].key1,s_TileGroups[i].szText);
                printext256(10L, (i+1)*16, whitecol, -1, tempbuf, 0);
            }
        }
        showframe(1);

        ch = bgetchar();

        for (i = 0; i < tile_groups; i++)
        {
            if (s_TileGroups[i].pIds != NULL && s_TileGroups[i].key1)
                if ((ch == s_TileGroups[i].key1) || (ch == s_TileGroups[i].key2))
                {
                    iTile = LoadTileSet(iTile, s_TileGroups[i].pIds, s_TileGroups[i].nIds);
                    bDone = 1;
                }
        }
    }

    enddrawing();
    showframe(1);

    clearkeys();

    return iTile;
}

static const char *GetTilePixels(int32_t idTile)
{
    char *pPixelData = 0;

    if (idTile >= 0 && idTile < MAXTILES)
    {
        if (!waloff[idTile])
            loadtile(idTile);

        if (IsValidTile(idTile))
            pPixelData = (char *)waloff[idTile];
    }

    return pPixelData;
}

static int32_t DrawTiles(int32_t iTopLeft, int32_t iSelected, int32_t nXTiles, int32_t nYTiles,
                         int32_t TileDim, int32_t offset, int32_t showmsg)
{
    int32_t XTile, YTile;
    int32_t iTile, idTile;
    int32_t XPos, YPos;
    int32_t XOffset, YOffset;
    int32_t i, marked;
    const char *pRawPixels;
    int32_t TileSizeX, TileSizeY;
    int32_t DivInc,MulInc;
    char *pScreen;
    char szT[128];

    begindrawing();

    setpolymost2dview();

    clearview(0);

    for (YTile = 0-(offset>0); YTile < nYTiles+(offset<0)+1; YTile++)
    {
        for (XTile = 0; XTile < nXTiles; XTile++)
        {
            iTile = iTopLeft + XTile + (YTile * nXTiles);

            if (iTile>=0 && iTile < localartlookupnum)
            {
                idTile = localartlookup[ iTile ];

                // Get pointer to tile's raw pixel data
                pRawPixels = GetTilePixels(idTile);

                if (pRawPixels != NULL)
                {
                    XPos = XTile * TileDim;
                    YPos = YTile * TileDim+offset;

                    if (polymost_drawtilescreen(XPos, YPos, idTile, TileDim, s_TileZoom))
                    {
                        TileSizeX = tilesizx[ idTile ];
                        TileSizeY = tilesizy[ idTile ];

                        DivInc = 1;
                        MulInc = 1;

                        while ((TileSizeX/DivInc > TileDim) || (TileSizeY/DivInc) > TileDim)
                        {
                            DivInc++;
                        }

                        if (DivInc == 1 && s_TileZoom)
                        {
                            while ((TileSizeX*(MulInc+1)) <= TileDim && (TileSizeY*(MulInc+1)) <= TileDim)
                            {
                                MulInc++;
                            }
                        }

                        TileSizeX = (TileSizeX / DivInc) * MulInc;
                        TileSizeY = (TileSizeY / DivInc) * MulInc;

                        for (YOffset = 0; YOffset < TileSizeY; YOffset++)
                        {
                            int32_t y=YPos+YOffset;
                            if (y>=0 && y<ydim)
                            {
                                pScreen = (char *)ylookup[y]+XPos+frameplace;
                                for (XOffset = 0; XOffset < TileSizeX; XOffset++)
                                {
                                    pScreen[XOffset] = pRawPixels[((YOffset * DivInc) / MulInc) + (((XOffset * DivInc) / MulInc) * tilesizy[idTile])];
                                }
                            }
                        }
                    }

                    if (localartfreq[iTile] != 0 && YPos >= 0 && YPos <= ydim-20)
                    {
                        Bsprintf(szT, "%d", localartfreq[iTile]);
                        printext256(XPos, YPos, whitecol, -1, szT, 1);
                    }
                }

                marked = (IsValidTile(idTile) && tilemarked[idTile>>3]&(1<<(idTile&7)));

                //
                // Draw white box around currently selected tile or marked tile
                // p1=(x1, y1), p2=(x1+TileDim-1, y1+TileDim-1)
                //
                if (iTile == iSelected || marked)
                {
                    int32_t x1 = ((iTile-iTopLeft) % nXTiles)*TileDim;
                    int32_t y1 = ((iTile - ((iTile-iTopLeft) % nXTiles) - iTopLeft)/nXTiles)*TileDim + offset;
                    int32_t x2 = x1+TileDim-1;
                    int32_t y2 = y1+TileDim-1;

                    char markedcol = editorcolors[14];

                    setpolymost2dview();

                    y1=max(y1, 0);
                    y2=min(y2, ydim-1);

                    // box

                    {
                        int32_t xx[] = {x1, x1, x2, x2, x1};
                        int32_t yy[] = {y1, y2, y2, y1, y1};
                        plotlines2d(xx, yy, 5, iTile==iSelected ? whitecol : markedcol);
                    }
                    // cross
                    if (marked)
                    {
                        int32_t xx[] = {x1, x2};
                        int32_t yy[] = {y1, y2};

                        plotlines2d(xx, yy, 2, markedcol);
                        swaplong(&yy[0], &yy[1]);
                        plotlines2d(xx, yy, 2, markedcol);
                    }
                }
            }
        }
    }

    if (iSelected < 0 || iSelected >= MAXTILES)
        return 1;

    idTile = localartlookup[ iSelected ];

    // Draw info bar at bottom.

    // Clear out behind the text for improved visibility.
    //drawline256(0, (ydim-12)<<12, xdim<<12, (ydim-12)<<12, whitecol);
    for (i=ydim-12; i<ydim; i++)
        drawline256(0, i<<12, xdim<<12, i<<12, (ydim-i));

    // Tile number on left.
    Bsprintf(szT, "%d" , idTile);
    printext256(1, ydim-10, whitecol, -1, szT, 0);

    // Tile name on right.
    printext256(xdim-(Bstrlen(names[idTile])<<3)-1,ydim-10,whitecol,-1,names[idTile],0);

    // Tile dimensions.
    Bsprintf(szT,"%dx%d",tilesizx[idTile],tilesizy[idTile]);
    printext256(xdim>>2,ydim-10,whitecol,-1,szT,0);

    // EditArt animation flags.
    Bsprintf(szT,"%d, %d",(picanm[idTile]>>8)&0xFF,(picanm[idTile]>>16)&0xFF);
    printext256((xdim>>2)+100,ydim-10,whitecol,-1,szT,0);

    if (showmsg)
        TMPERRMSG_SHOW(0);

    m32_showmouse();

    enddrawing();
    showframe(1);

    return(0);

}

#undef TMPERRMSG_SHOW
#undef TMPERRMSG_PRINT
#undef TMPERRMSG_RETURN


static int32_t spriteonceilingz(int32_t searchwall)
{
    int32_t z=sprite[searchwall].z;

    z = getceilzofslope(searchsector,sprite[searchwall].x,sprite[searchwall].y);

    if (sprite[searchwall].cstat&128)
        z -= ((tilesizy[sprite[searchwall].picnum]*sprite[searchwall].yrepeat)<<1);
    if ((sprite[searchwall].cstat&48) != 32)
        z += ((tilesizy[sprite[searchwall].picnum]*sprite[searchwall].yrepeat)<<2);
    return z;
}

static int32_t spriteongroundz(int32_t searchwall)
{
    int32_t z=sprite[searchwall].z;

    z = getflorzofslope(searchsector,sprite[searchwall].x,sprite[searchwall].y);

    if (sprite[searchwall].cstat&128)
        z -= ((tilesizy[sprite[searchwall].picnum]*sprite[searchwall].yrepeat)<<1);
    return z;
}

#define WIND1X   3
#define WIND1Y 150
void drawtileinfo(char *title,int32_t x,int32_t y,int32_t picnum,int32_t shade,int32_t pal,int32_t cstat,int32_t lotag,int32_t hitag,int32_t extra)
{
    char buf[64];
    int32_t i,small = (xdimgame<=640);
    int32_t scale=65536;
    int32_t x1;
    int32_t oviewingrange=viewingrange, oyxaspect=yxaspect;

    i = ydimgame>>6;

    x1 = x+80;
    if (small)
        x1 /= 2;

    x1 *= 320.0/xdimgame;
    scale /= (max(tilesizx[picnum],tilesizy[picnum])/24.0);

    setaspect(65536L, (int32_t)divscale16(ydim*320L,xdim*200L));
    // +1024: prevents rotatesprite from setting aspect itself
    rotatesprite((x1+13)<<16,(y+11)<<16,scale,0, picnum,shade,pal, 2+1024, 0,0,xdim-1,ydim-1);
    setaspect(oviewingrange, oyxaspect);

    x *= xdimgame/320.0;
    y *= ydimgame/200.0;

    begindrawing();
    printext256(x+2,y+2,0,-1,title,small);
    printext256(x,y,255-13,-1,title,small);

#define DOPRINT(N) \
    printext256(x+2, y+2+i*N, 0, -1, buf, small); \
    printext256(x, y+i*N, whitecol, -1, buf, small);

    Bsprintf(buf,"Pic:%4d",picnum);
    DOPRINT(1);
    Bsprintf(buf,"Shd:%4d",shade);
    DOPRINT(2);
    Bsprintf(buf,"Pal:%4d",pal);
    DOPRINT(3);
    Bsprintf(buf,"Cst:%4d",cstat);
    DOPRINT(4);
    Bsprintf(buf,"Lot:%4d",lotag);
    DOPRINT(5);
    Bsprintf(buf,"Hit:%4d",hitag);
    DOPRINT(6);
    Bsprintf(buf,"Ext:%4d",extra);
    DOPRINT(7);
    enddrawing();
#undef DOPRINT
}
//int32_t snap=0;
//int32_t saveval1,saveval2,saveval3;

static inline void getnumber_dochar(char *ptr, int32_t num)
{
    *ptr = (char) num;
}

static inline void getnumber_doint16_t(int16_t *ptr, int32_t num)
{
    *ptr = (int16_t) num;
}

static inline void getnumber_doint32(int32_t *ptr, int32_t num)
{
    *ptr = (int32_t) num;
}

static inline void getnumber_doint64(int64_t *ptr, int32_t num)
{
    *ptr = (int64_t) num;
}

void getnumberptr256(char *namestart, void *num, int32_t bytes, int32_t maxnumber, char sign, void *(func)(int32_t))
{
    char buffer[80], ch;
    int32_t n, danum = 0, oldnum;

    switch (bytes)
    {
    case 1:
        danum = *(char *)num;
        break;
    case 2:
        danum = *(int16_t *)num;
        break;
    case 4:
        danum = *(int32_t *)num;
        break;
    case 8:
        danum = *(int64_t *)num;
        break;
    }

    oldnum = danum;
    bflushchars();
    while (keystatus[0x1] == 0)
    {
        if (handleevents())
            quitevent = 0;

        drawrooms(pos.x,pos.y,pos.z,ang,horiz,cursectnum);
#ifdef SUPERBUILD
        ExtAnalyzeSprites();
#endif
        drawmasks();
#ifdef POLYMER
        if (rendmode == 4 && searchit == 2)
        {
            polymer_editorpick();
            drawrooms(pos.x,pos.y,pos.z,ang,horiz,cursectnum);
            ExtAnalyzeSprites();
            drawmasks();
        }
#endif

        ch = bgetchar();

        if (keystatus[0x1]) break;

        clearkeys();

        mouseb = 0;
        searchx = osearchx;
        searchy = osearchy;

        ExtCheckKeys();

        Bsprintf(buffer,"%s%d",namestart,danum);
        if (totalclock & 32) Bstrcat(buffer,"_ ");
        printmessage256(0, 0, buffer);
        if (func != NULL)
        {
            Bsprintf(buffer,"%s",(char *)func((int32_t)danum));
            printmessage256(0, 9, buffer);
        }
        showframe(1);

        if (ch >= '0' && ch <= '9')
        {
            if (danum >= 0)
            {
                n = (danum*10)+(ch-'0');
                if (n <= maxnumber) danum = n;
            }
            else if (sign)
            {
                n = (danum*10)-(ch-'0');
                if (n >= -maxnumber) danum = n;
            }
        }
        else if (ch == 8 || ch == 127)  	// backspace
        {
            danum /= 10;
        }
        else if (ch == 13)
        {
            if (danum != oldnum)
                asksave = 1;
            oldnum = danum;
            break;
        }
        else if (ch == '-' && sign)  	// negate
        {
            danum = -danum;
        }
        switch (bytes)
        {
        case 1:
            getnumber_dochar(num, danum);
            break;
        case 2:
            getnumber_doint16_t(num, danum);
            break;
        case 4:
            getnumber_doint32(num, danum);
            break;
        case 8:
            getnumber_doint64(num, danum);
            break;
        }
    }
    clearkeys();

    lockclock = totalclock;  //Reset timing

    switch (bytes)
    {
    case 1:
        getnumber_dochar(num, oldnum);
        break;
    case 2:
        getnumber_doint16_t(num, oldnum);
        break;
    case 4:
        getnumber_doint32(num, oldnum);
        break;
    case 8:
        getnumber_doint64(num, oldnum);
        break;
    }
}

static void DoSpriteOrnament(int32_t i)
{
    int32_t j, hitw;
    hitdata_t hitinfo;

    hitscan((const vec3_t *)&sprite[i],sprite[i].sectnum,
            sintable[(sprite[i].ang+1536)&2047],
            sintable[(sprite[i].ang+1024)&2047],
            0,
            &hitinfo,CLIPMASK1);

    sprite[i].x = hitinfo.pos.x;
    sprite[i].y = hitinfo.pos.y;
    sprite[i].z = hitinfo.pos.z;
    changespritesect(i,hitinfo.hitsect);

    hitw = hitinfo.hitwall;

    if (hitw >= 0)
        sprite[i].ang = (getangle(POINT2(hitw).x-wall[hitw].x,
                                  POINT2(hitw).y-wall[hitw].y)+512)&2047;

    //Make sure sprite's in right sector
    if (inside(sprite[i].x,sprite[i].y,sprite[i].sectnum) == 0)
    {
        j = wall[hitw].point2;
        sprite[i].x -= ksgn(wall[j].y-wall[hitw].y);
        sprite[i].y += ksgn(wall[j].x-wall[hitw].x);
    }
}

#if 0
int64_t ldistsqr(spritetype *s1,spritetype *s2)
{
    return (((int64_t)(s2->x - s1->x))*((int64_t)(s2->x - s1->x)) +
            ((int64_t)(s2->y - s1->y))*((int64_t)(s2->y - s1->y)));
}
#endif

static void TextEntryMode(int16_t startspr)
{
    char ch, buffer[80], doingspace=0;
    int16_t daang = 0, t, alphidx, basetile, linebegspr, curspr, cursor;
    int32_t i, j, k, dax = 0, day = 0;
    static uint8_t hgap=0, vgap=4;
    static uint8_t spcgap[MAX_ALPHABETS], firstrun=1;
    spritetype *sp;

    int16_t *spritenums;
    int32_t stackallocsize=32, numletters=0;

    if (firstrun)
    {
        firstrun=0;
        for (i=0; i<numalphabets; i++)
            spcgap[i] = 0;
    }

    if (startspr<0 || startspr>=MAXSPRITES ||
            sprite[startspr].statnum == MAXSTATUS)
        return;

    if (numalphabets == 0)
    {
        message("Alphabet configuration not read.");
        return;
    }

    if ((sprite[startspr].cstat&16) == 0)
    {
        message("Must point at a wall-aligned text sprite.");
        return;
    }

    t = sprite[startspr].picnum;
    alphidx = -1;
    for (i=0; i<numalphabets; i++)
    {
        for (j=0; j<NUMPRINTABLES; j++)
            if (alphabets[i].pic[j] == t)
            {
                alphidx = i;
                basetile = t;
                if (spcgap[i] == 0)
                    spcgap[i] = 3*tilesizx[t]/2;
                goto ENDFOR1;
            }
    }
ENDFOR1:
    if (alphidx==-1)
    {
        message("Must point at a text sprite.");
        return;
    }

    curspr = linebegspr = startspr;

    t = sprite[startspr].picnum;
    sprite[startspr].xoffset = -(((picanm[t])>>8)&255);
    sprite[startspr].yoffset = -(((picanm[t])>>16)&255);

    spritenums = Bmalloc(stackallocsize * sizeof(int16_t));
    if (!spritenums) goto ERROR_NOMEMORY;

    cursor = insertsprite(sprite[startspr].sectnum,0);
    if (cursor < 0) goto ERROR_TOOMANYSPRITES;

    updatenumsprites();

    sp = &sprite[cursor];
    Bmemcpy(sp, &sprite[startspr], sizeof(spritetype));
    sp->yoffset = 0;
    sp->picnum = SMALLFNTCURSOR;
    sp->xrepeat = clamp(sp->xrepeat/tilesizx[sp->picnum], 2, 255);
    sp->yrepeat = clamp((sp->yrepeat*tilesizy[sprite[startspr].picnum])/tilesizy[sp->picnum], 4, 255);
    sp->pal = 0;
    sp->cstat = 18;

    bflushchars();
    while (keystatus[0x1] == 0)
    {
        if (handleevents())
            quitevent = 0;


        if (PRESSED_KEYSC(UP))  // vertical gap in pixels (32 x-units)
            vgap += (vgap<255);

        if (PRESSED_KEYSC(DOWN))
            vgap -= (vgap>0);

        if (PRESSED_KEYSC(RIGHT))  // horizontal gap in half pixels
            hgap += (hgap<255);

        if (PRESSED_KEYSC(LEFT))
            hgap -= (hgap>0);

        if (PRESSED_KEYSC(INSERT))  // space gap in half pixels
            spcgap[alphidx] += (spcgap[alphidx]<255);

        if (PRESSED_KEYSC(DELETE))
            spcgap[alphidx] -= (spcgap[alphidx]>1);

        if (PRESSED_KEYSC(HOME))  // shade
            sprite[linebegspr].shade += (sprite[linebegspr].shade<127);

        if (PRESSED_KEYSC(END))
            sprite[linebegspr].shade -= (sprite[linebegspr].shade>-128);

        if (PRESSED_KEYSC(PGUP))  // pal
            sprite[linebegspr].pal += (sprite[linebegspr].pal<255);

        if (PRESSED_KEYSC(PGDN))
            sprite[linebegspr].pal -= (sprite[linebegspr].pal>0);

        drawrooms(pos.x,pos.y,pos.z,ang,horiz,cursectnum);
#ifdef SUPERBUILD
        ExtAnalyzeSprites();
#endif
        drawmasks();
#ifdef POLYMER
        if (rendmode == 4 && searchit == 2)
        {
            polymer_editorpick();
            drawrooms(pos.x,pos.y,pos.z,ang,horiz,cursectnum);
            ExtAnalyzeSprites();
            drawmasks();
        }
#endif

        ch = bgetchar();

        if (keystatus[0x1]) break;

        clearkeys();

        mouseb = 0;
        searchx = osearchx;
        searchy = osearchy;

        ExtCheckKeys();

        printmessage256(0,0,"^251Text entry mode.^31 Navigation keys change vars.");
        Bsprintf(buffer, "Hgap=%d, Vgap=%d, SPCgap=%d, Shd=%d, Pal=%d",
                 hgap, vgap, spcgap[alphidx], sprite[linebegspr].shade, sprite[linebegspr].pal);
        printmessage256(0, 9, buffer);
        showframe(1);

        // ---
        sp = &sprite[curspr];
        if (!doingspace)
        {
            dax = sp->x; day = sp->y;
            daang = sp->ang;
        }

        j = sp->xrepeat*(hgap+tilesizx[sp->picnum]+2);
        {
            vec3_t vect;
            vect.x = dax + ((j*sintable[daang])>>17);
            vect.y = day - ((j*sintable[(daang+512)&2047])>>17);
            vect.z = sp->z;
            setsprite(cursor,&vect);
        }

        if (ch>=33 && ch<=126 && alphabets[alphidx].pic[ch-33] >= 0)
        {
            int16_t sect;

            // mapping char->tilenum
            t = alphabets[alphidx].pic[ch-33];
            j = sp->xrepeat*(hgap+tilesizx[sp->picnum]+tilesizx[t]);

            dax += (j*sintable[daang])>>17;
            day -= (j*sintable[(daang+512)&2047])>>17;
            dax += (j*sintable[(sprite[curspr].ang+2560)&2047])>>17;
            day += (j*sintable[(sprite[curspr].ang+2048)&2047])>>17;

            sect = sprite[curspr].sectnum;
            updatesector(dax,day,&sect);
            if (numsprites < MAXSPRITES && sect >= 0)
            {
                i = insertsprite(sect,0);
                Bmemcpy(&sprite[i], &sprite[linebegspr], sizeof(spritetype));
                sprite[i].x = dax, sprite[i].y = day;
                sprite[i].picnum = t;
                sprite[i].ang = daang;

                sprite[i].xoffset = -(((picanm[sprite[i].picnum])>>8)&255);
                sprite[i].yoffset = -(((picanm[sprite[i].picnum])>>16)&255);
                sprite[i].xoffset += alphabets[alphidx].xofs[(int32_t)ch-33];
                sprite[i].yoffset += alphabets[alphidx].yofs[(int32_t)ch-33];

                DoSpriteOrnament(i);

                for (k=0; k<MAXTILES; k++)
                    localartfreq[k] = 0;
                for (k=0; k<MAXSPRITES; k++)
                    if (sprite[k].statnum < MAXSTATUS)
                        localartfreq[sprite[k].picnum]++;

                curspr = i;
                doingspace = 0;

                updatenumsprites();
                asksave = 1;

                if (numletters >= stackallocsize)
                {
                    stackallocsize *= 2;
                    spritenums = Brealloc(spritenums, stackallocsize*sizeof(int16_t));
                    if (!spritenums) goto ERROR_NOMEMORY;
                }
                spritenums[numletters++] = i;
            }
        }
        else if (ch == 32)
        {
            dax += ((sp->xrepeat*spcgap[alphidx]*sintable[daang])>>17);
            day -= ((sp->xrepeat*spcgap[alphidx]*sintable[(daang+512)&2047])>>17);
            doingspace = 1;
        }
        else if (ch == 8)  	// backspace
        {
            if (doingspace)
                doingspace = 0;
            else if (numletters > 0)
            {
                int16_t last = spritenums[numletters-1];

                if (sprite[last].z != sprite[linebegspr].z)  // only "delete" line break
                {
                    sprite[linebegspr].z = sprite[last].z;
                    curspr = last;
                }
                else if (numletters > 1)
                {
                    int16_t sectolast = spritenums[numletters-2];

                    if (sprite[last].z == sprite[sectolast].z)
                        curspr = sectolast;
                    else  // if we delete the first letter on the line
                        curspr = linebegspr;

                    numletters--;
                    deletesprite(last);

                    updatenumsprites();
                    asksave = 1;
                }
                else
                {
                    numletters--;
                    deletesprite(last);
                    curspr = linebegspr;
                    updatenumsprites();
                    asksave = 1;
                }
            }
            else
            {
                sprite[linebegspr].z -= ((sprite[linebegspr].yrepeat*(vgap+tilesizy[basetile]))<<2);
                asksave = 1;
            }
        }
        else if (ch == 13)  // enter
        {
            sprite[linebegspr].z += ((sprite[linebegspr].yrepeat*(vgap+tilesizy[basetile]))<<2);
            curspr = linebegspr;
            doingspace = 0;
            asksave = 1;
        }
    }

ERROR_TOOMANYSPRITES:
    if (cursor < 0) message("Too many sprites in map!");
    else deletesprite(cursor);
    updatenumsprites();

ERROR_NOMEMORY:
    if (spritenums) Bfree(spritenums);
    else message("Out of memory!");

    clearkeys();

    lockclock = totalclock;  //Reset timing
}

static void mouseaction_movesprites(int32_t *sumxvect, int32_t *sumyvect, int32_t yangofs, int32_t mousexory)
{
    int32_t xvect,yvect, daxvect,dayvect, ii, spi;
    int32_t units, gridlock = (eitherCTRL && grid > 0 && grid < 9);
    spritetype *sp = &sprite[searchwall];
    int16_t tsect = sp->sectnum;
    vec3_t tvec = { sp->x, sp->y, sp->z };

    xvect = -((mousexory*(int32_t)sintable[(ang+yangofs+512)&2047])<<3);
    yvect = -((mousexory*(int32_t)sintable[(ang+yangofs)&2047])<<3);

    if (gridlock)
    {
        units = 1<<(11-grid);

        if ((tvec.x & (units-1)) || (tvec.y & (units-1)))
        {
            daxvect = ((tvec.x & ~(units-1)) - tvec.x)<<14;
            dayvect = ((tvec.y & ~(units-1)) - tvec.y)<<14;
        }
        else
        {
            units <<= 14;

            *sumxvect += xvect;
            *sumyvect += yvect;

            if (klabs(*sumxvect) >= units)
            {
                daxvect = ((*sumxvect)/units)*units;
                *sumxvect %= units;
            }
            else
                daxvect = 0;

            if (klabs(*sumyvect) >= units)
            {
                dayvect = ((*sumyvect)/units)*units;
                *sumyvect %= units;
            }
            else
                dayvect = 0;
        }
    }
    else
    {
        daxvect = xvect;
        dayvect = yvect;
    }

    if (highlightcnt<=0 || (show2dsprite[searchwall>>3] & (1<<(searchwall&7)))==0)
    {
        clipmove(&tvec, &tsect, daxvect,dayvect, sp->clipdist,64<<4,64<<4, spnoclip?1:CLIPMASK0);
        setsprite(searchwall, &tvec);
    }
    else
    {
        xvect = daxvect;
        yvect = dayvect;

        for (ii=0; ii<highlightcnt; ii++)
            if (highlight[ii]&16384)
            {
                spi = highlight[ii]&16383;
                Bmemcpy(&tvec, &sprite[spi], sizeof(vec3_t));
                tsect = sprite[spi].sectnum;
                clipmove(&tvec, &tsect, xvect,yvect, 128,64<<4,64<<4, spnoclip?1:CLIPMASK0);

                xvect = (tvec.x - sprite[spi].x)<<14;
                yvect = (tvec.y - sprite[spi].y)<<14;
            }
        for (ii=0; ii<highlightcnt; ii++)
            if (highlight[ii]&16384)
            {
                spi = highlight[ii]&16383;
                Bmemcpy(&tvec, &sprite[spi], sizeof(vec3_t));
                tsect = sprite[spi].sectnum;
                clipmove(&tvec, &tsect, xvect,yvect, 128,64<<4,64<<4, spnoclip?1:CLIPMASK0);
                setsprite(spi, &tvec);
            }
    }
}

static void Keys3d(void)
{
    int32_t i = 0, changedir,tsign; // ,count,nexti
    int32_t j, k, tempint = 0, hiz, loz;
    int32_t hihit, lohit;
    char smooshyalign=0, repeatpanalign=0; //, buffer[80];
    int16_t startwall, endwall, dasector; //, statnum=0;
    char tempbuf[128];

    /* start Mapster32 */

    if (g_numsounds > 0 && AmbienceToggle)
    {
        M32_MoveFX();
        S_Update();
    }

    if (usedcount && !helpon)
    {
#if 0
        if (!AIMING_AT_SPRITE)
        {
            count=0;
            for (i=0; i<numwalls; i++)
            {
                if (wall[i].picnum == temppicnum) count++;
                if (wall[i].overpicnum == temppicnum) count++;
            }
            for (i=0; i<numsectors; i++)
            {
                if (sector[i].ceilingpicnum == temppicnum) count++;
                if (sector[i].floorpicnum == temppicnum) count++;
            }
        }
        else
        {
            count=0;
            statnum=0;

            i = headspritestat[statnum];
            while (i != -1)
            {
                nexti = nextspritestat[i];
                if (sprite[i].picnum == temppicnum) count++;
                i = nexti;
            }
        }
#endif
        drawtileinfo("Clipboard",3,124,temppicnum,tempshade,temppal,tempcstat,templotag,temphitag,tempextra);
    }// end if usedcount

    if (searchsector > -1 && searchsector < numsectors)
    {
        char lines[8][64];
        int32_t dist, height1=0,height2=0,height3=0, num=0;
        int32_t x,y;
        int16_t w;

        if (infobox&1)
        {
            height2 = sector[searchsector].floorz - sector[searchsector].ceilingz;

            switch (searchstat)
            {
            case SEARCH_WALL:
            case SEARCH_MASKWALL:
                w = SELECT_WALL();
                drawtileinfo("Current", WIND1X,WIND1Y,
                             AIMING_AT_WALL ? wall[w].picnum : wall[w].overpicnum,
                             wall[w].shade, wall[w].pal, wall[searchwall].cstat,
                             wall[searchwall].lotag, wall[searchwall].hitag,wall[searchwall].extra);

                dist = wallength(searchwall);

                if (wall[searchwall].nextsector >= 0 && wall[searchwall].nextsector < numsectors)
                {
                    int32_t nextsect = wall[searchwall].nextsector;
                    height1 = sector[searchsector].floorz - sector[nextsect].floorz;
                    height2 = sector[nextsect].floorz - sector[nextsect].ceilingz;
                    height3 = sector[nextsect].ceilingz - sector[searchsector].ceilingz;
                }

                Bsprintf(lines[num++],"Panning: %d, %d", wall[w].xpanning, wall[w].ypanning);
                Bsprintf(lines[num++],"Repeat:  %d, %d", wall[searchwall].xrepeat, wall[searchwall].yrepeat);
                Bsprintf(lines[num++],"Overpic: %d", wall[searchwall].overpicnum);
                lines[num++][0]=0;

                if (getmessageleng)
                    break;

                Bsprintf(lines[num++],"^251Wall %d^31", searchwall);

                if (wall[searchwall].nextsector!=-1)
                    Bsprintf(lines[num++],"LoHeight:%d, HiHeight:%d, Length:%d",height1,height3,dist);
                else
                    Bsprintf(lines[num++],"Height:%d, Length:%d",height2,dist);
                break;

            case SEARCH_CEILING:
            case SEARCH_FLOOR:
                drawtileinfo("Current", WIND1X, WIND1Y, AIMED_CEILINGFLOOR(picnum), AIMED_CEILINGFLOOR(shade),
                             AIMED_CEILINGFLOOR(pal), AIMED_CEILINGFLOOR(stat),
                             sector[searchsector].lotag, sector[searchsector].hitag, sector[searchsector].extra);

                Bsprintf(lines[num++],"Panning:  %d, %d", AIMED_CEILINGFLOOR(xpanning), AIMED_CEILINGFLOOR(ypanning));
                Bsprintf(lines[num++],"%sZ: %d", Typestr[searchstat], AIMED_CEILINGFLOOR(z));
                Bsprintf(lines[num++],"Slope:    %d", AIMED_CEILINGFLOOR(heinum));
                lines[num++][0]=0;

                if (getmessageleng)
                    break;

                Bsprintf(lines[num++],"^251Sector %d^31 %s, Lotag:%s", searchsector, typestr[searchstat], ExtGetSectorCaption(searchsector));
                Bsprintf(lines[num++],"Height: %d, Visibility:%d", height2, sector[searchsector].visibility);
                break;

            case SEARCH_SPRITE:
                drawtileinfo("Current", WIND1X, WIND1Y, sprite[searchwall].picnum, sprite[searchwall].shade,
                             sprite[searchwall].pal, sprite[searchwall].cstat, sprite[searchwall].lotag,
                             sprite[searchwall].hitag, sprite[searchwall].extra);

                Bsprintf(lines[num++], "Repeat:  %d,%d", sprite[searchwall].xrepeat, sprite[searchwall].yrepeat);
                Bsprintf(lines[num++], "PosXY:   %d,%d", sprite[searchwall].x, sprite[searchwall].y);
                Bsprintf(lines[num++], "PosZ: ""   %d", sprite[searchwall].z);// prevents tab character
                lines[num++][0]=0;

                if (getmessageleng)
                    break;

                if (names[sprite[searchwall].picnum][0])
                {
                    if (sprite[searchwall].picnum==SECTOREFFECTOR)
                        Bsprintf(lines[num++],"^251Sprite %d^31 %s", searchwall, SectorEffectorText(searchwall));
                    else
                        Bsprintf(lines[num++],"^251Sprite %d^31 %s", searchwall, names[sprite[searchwall].picnum]);
                }
                else Bsprintf(lines[num++],"^251Sprite %d^31, picnum %d", searchwall, sprite[searchwall].picnum);

                Bsprintf(lines[num++], "Elevation:%d",
                         getflorzofslope(searchsector, sprite[searchwall].x, sprite[searchwall].y) - sprite[searchwall].z);
                break;
            }
        }

        x = WIND1X*(xdimgame/320.);
        y = WIND1Y*(ydimgame/200.);
        y += (ydimgame>>6)*8;

        if (getmessageleng)
        {
            while (num < 4)
                lines[num++][0] = 0;
            Bsprintf(lines[num++],"^251%s",getmessage);
        }

        begindrawing();
        for (i=0; i<num; i++)
        {
            printext256(x+2, y+2, 0, -1, lines[i], xdimgame<=640);
            printext256(x, y, whitecol, -1, lines[i], xdimgame<=640);
            y += ydimgame>>6;
        }
        enddrawing();
    }

    VM_OnEvent(EVENT_PREKEYS3D, -1);

    if (keystatus[KEYSC_QUOTE] && PRESSED_KEYSC(V)) // ' V
    {
        if (AIMING_AT_CEILING_OR_FLOOR)
            getnumberptr256("Sector visibility: ", &sector[searchsector].visibility, sizeof(sector[0].visibility), 255, 0, NULL);
    }

    if (keystatus[KEYSC_SEMI] && PRESSED_KEYSC(V))   // ; V
    {
        int16_t currsector;
        uint8_t visval;

        if (highlightsectorcnt == -1)
        {
            message("You didn't select any sectors!");
            return;
        }

        visval = getnumber256("Visibility of selected sectors: ", sector[searchsector].visibility, 255, 0);

        if (AskIfSure(0))
            return;

        for (i=0; i<highlightsectorcnt; i++)
        {
            currsector = highlightsector[i];
            sector[currsector].visibility = visval;
        }

        message("Visibility changed on all selected sectors");
    }

    if (PRESSED_KEYSC(V))  //V
    {
        if (ASSERT_AIMING)
        {
            int32_t oldtile = AIMED_SELOVR_PICNUM;

            tempint = m32gettile(oldtile);
            AIMED_SELOVR_PICNUM = tempint;

            if (AIMING_AT_MASKWALL && wall[searchwall].nextwall >= 0)
                NEXTWALL(searchwall).overpicnum = tempint;

            if (oldtile != tempint)
                asksave = 1;
        }
    }

    if (PRESSED_KEYSC(3))  /* 3 (toggle floor-over-floor (cduke3d only) */
    {
        floor_over_floor = !floor_over_floor;
        //        if (!floor_over_floor) ResetFOFSize();
        message("Floor-over-floor display %s",floor_over_floor?"enabled":"disabled");
    }

    if (PRESSED_KEYSC(F3))
    {
        mlook = !mlook;
        message("Mouselook: %s",mlook?"enabled":"disabled");
    }

    if (PRESSED_KEYSC(F4))
    {
        AmbienceToggle = !AmbienceToggle;
        message("Ambience sounds: %s",AmbienceToggle?"enabled":"disabled");
        if (!AmbienceToggle)
        {
            FX_StopAllSounds();
            S_ClearSoundLocks();
        }
    }

    // PK
    if (PRESSED_KEYSC(F5))
    {
        unrealedlook = !unrealedlook;
        message("UnrealEd mouse navigation: %s",unrealedlook?"enabled":"disabled");
    }

    if (keystatus[KEYSC_QUOTE] && PRESSED_KEYSC(DELETE)) // ' del
    {
        if (AIMING_AT_WALL_OR_MASK)
        {
            wall[searchwall].cstat = 0;
            message("Wall %d cstat = 0", searchwall);
        }
        else if (AIMING_AT_SPRITE)
        {
            sprite[searchwall].cstat = 0;
            message("Sprite %d cstat = 0", searchwall);
        }
    }

    // 'P and ;P - Will copy palette to all sectors/walls/sprites highlighted with R-Alt key
    if ((keystatus[KEYSC_QUOTE] || keystatus[KEYSC_SEMI]) && PRESSED_KEYSC(P))   // ' P  ; P
    {
        int16_t w, start_wall, end_wall, currsector;
        int8_t pal[4];

        if (highlightsectorcnt == -1)
        {
            message("You didn't select any sectors!");
            return;
        }

        if (keystatus[KEYSC_QUOTE])
        {
            pal[0] = getnumber256("Ceiling palette: ", -1, MAXPALOOKUPS, 1);
            pal[1] = getnumber256("Floor palette: ", -1, MAXPALOOKUPS, 1);
            pal[2] = getnumber256("Wall palette: ", -1, MAXPALOOKUPS, 1);
            pal[3] = getnumber256("Sprite palette: ", -1, MAXPALOOKUPS, 1);
        }
        else
        {
            pal[0] = getnumber256("Global palette: ", 0, MAXPALOOKUPS, 0);
            pal[1] = pal[2] = pal[3] = pal[0];
        }

        if (AskIfSure(0)) return;

        for (i = 0; i < highlightsectorcnt; i++)
        {
            currsector = highlightsector[i];

            if (pal[0] > -1)
                sector[currsector].ceilingpal = pal[0];
            if (pal[1] > -1)
                sector[currsector].floorpal = pal[1];

            // Do all the walls in the sector
            start_wall = sector[currsector].wallptr;
            end_wall = start_wall + sector[currsector].wallnum;

            if (pal[2] > -1)
                for (w = start_wall; w < end_wall; w++)
                    wall[w].pal = pal[2];

            if (pal[3] > -1)
            {
                for (k=0; k<highlightsectorcnt; k++)
                {
                    w = headspritesect[highlightsector[k]];
                    while (w >= 0)
                    {
                        j = nextspritesect[w];
                        sprite[w].pal = pal[3];
                        w = j;
                    }
                }
            }
        }

        message("Palettes changed");
    }

    if (PRESSED_KEYSC(DELETE))
    {
        if (AIMING_AT_SPRITE)
        {
            deletesprite(searchwall);
            updatenumsprites();
            message("Sprite %d deleted",searchwall);
            if (AmbienceToggle)
            {
                sprite[searchwall].filler &= (~1);
                S_StopEnvSound(sprite[searchwall].lotag, searchwall);
            }
            asksave = 1;
        }
    }

    if (PRESSED_KEYSC(F6))  //F6
    {
        autospritehelp = !autospritehelp;
        message("Automatic SECTOREFFECTOR help %s", autospritehelp?"enabled":"disabled");
    }
    if (PRESSED_KEYSC(F7))  //F7
    {
        autosecthelp = !autosecthelp;
        message("Automatic sector tag help %s", autosecthelp?"enabled":"disabled");
    }

    if (autospritehelp && helpon==0)
    {
        if (AIMING_AT_SPRITE && sprite[searchwall].picnum==SECTOREFFECTOR)
            Show3dText("sehelp.hlp");
        else if (AIMING_AT_CEILING_OR_FLOOR)
            Show3dText("sthelp.hlp");
    }

    if (AIMING_AT_WALL_OR_MASK && PRESSED_KEYSC(PERIOD))
    {
        AutoAlignWalls((int32_t)searchwall, 0);
        message("Wall %d autoalign", searchwall);
    }


    tsign = 0;
    tsign -= PRESSED_KEYSC(COMMA);
    tsign += PRESSED_KEYSC(PERIOD);

    if (tsign) // , . Search & fix panning to the left/right (3D)
    {
        if (AIMING_AT_SPRITE)
        {
            sprite[searchwall].ang += tsign<<(!eitherSHIFT*7);
            sprite[searchwall].ang &= 2047;
            message("Sprite %d angle: %d", searchwall, sprite[searchwall].ang);
        }
    }

    if (keystatus[KEYSC_QUOTE] && PRESSED_KEYSC(L)) // ' L
    {
        i = noclip;
        noclip = 1;

        switch (searchstat)
        {
        case SEARCH_CEILING:
            getnumberptr256("Sector ceilingz: ", &sector[searchsector].ceilingz,
                            sizeof(sector[0].ceilingz), 8388608, 1, NULL);
            if (!(sector[searchsector].ceilingstat&2))
            {
                sector[searchsector].ceilingstat |= 2;
                sector[searchsector].ceilingheinum = 0;
            }
            getnumberptr256("Sector ceiling slope: ", &sector[searchsector].ceilingheinum,
                            sizeof(sector[0].ceilingheinum), 65536, 1, NULL);
            break;
        case SEARCH_FLOOR:
            getnumberptr256("Sector floorz: ", &sector[searchsector].floorz,
                            sizeof(sector[0].floorz), 8388608, 1, NULL);
            if (!(sector[searchsector].floorstat&2))
            {
                sector[searchsector].floorheinum = 0;
                sector[searchsector].floorstat |= 2;
            }
            getnumberptr256("Sector floor slope: ", &sector[searchsector].floorheinum,
                            sizeof(sector[0].floorheinum), 65536, 1, NULL);
            break;

        case SEARCH_SPRITE:
            getnumberptr256("Sprite x: ", &sprite[searchwall].x, sizeof(sprite[0].x), 131072, 1, NULL);
            getnumberptr256("Sprite y: ", &sprite[searchwall].y, sizeof(sprite[0].y), 131072, 1, NULL);
            getnumberptr256("Sprite z: ", &sprite[searchwall].z, sizeof(sprite[0].z), 8388608, 1, NULL);
            getnumberptr256("Sprite angle: ", &sprite[searchwall].ang, sizeof(sprite[0].ang), 2047, 0, NULL);
            break;
        }

        if (sector[searchsector].ceilingheinum == 0)
            sector[searchsector].ceilingstat &= ~2;
        else
            sector[searchsector].ceilingstat |= 2;

        if (sector[searchsector].floorheinum == 0)
            sector[searchsector].floorstat &= ~2;
        else
            sector[searchsector].floorstat |= 2;

        asksave = 1;
        noclip = i;
    }


    getzrange(&pos, cursectnum, &hiz, &hihit, &loz, &lohit, 128L, CLIPMASK0);

    if (PRESSED_KEYSC(CAPS) || (keystatus[KEYSC_QUOTE] && PRESSED_KEYSC(Z)))  // CAPS LOCK
    {
        zmode = (zmode+1)%3;
        if (zmode == 1)
            zlock = (loz-pos.z)&0xfffffc00;
        switch (zmode)
        {
        case 0: message("Zmode = Gravity"); break;
        case 1: message("Zmode = Locked/Sector"); break;
        case 2: message("Zmode = Locked/Free"); break;
        }
    }

    if (keystatus[KEYSC_QUOTE] && PRESSED_KEYSC(M))  // 'M
    {
        if (ASSERT_AIMING)
        {
            Bsprintf(tempbuf, "%s extra: ", Typestr_wss[searchstat]);
            getnumberptr256(tempbuf, &AIMED(extra), sizeof(int16_t), BTAG_MAX, 1, NULL);
            asksave = 1;
        }
    }

    if (PRESSED_KEYSC(1) && ASSERT_AIMING)  // 1 (make 1-way wall)
    {
        if (!AIMING_AT_SPRITE)
        {
            wall[searchwall].cstat ^= 32;
            message("Wall %d one side masking bit %s", searchwall, ONOFF(wall[searchwall].cstat&32));
        }
        else
        {
            i = sprite[searchwall].cstat;
            i ^= 64;
            if ((i&48) == 32)
            {
                i &= ~8;
                if ((i&64) && pos.z>sprite[searchwall].z)
                    i |= 8;
            }
            message("Sprite %d one sided bit %s", searchwall, ONOFF(i&64));
            sprite[searchwall].cstat = i;
        }
        asksave = 1;
    }

    if (PRESSED_KEYSC(2))  // 2 (bottom wall swapping)
    {
        if (searchstat != SEARCH_SPRITE)
        {
            wall[searchwall].cstat ^= 2;
            message("Wall %d bottom texture swap bit %s", searchwall, ONOFF(wall[searchwall].cstat&2));
            asksave = 1;
        }
    }

    if (PRESSED_KEYSC(O))  // O (top/bottom orientation - for doors)
    {
        if (AIMING_AT_WALL_OR_MASK)
        {
            int16_t w = SELECT_WALL();
            wall[w].cstat ^= 4;
            message("Wall %d %s orientation", w, wall[w].cstat&4?"bottom":"top");
            asksave = 1;
        }
        else if (AIMING_AT_SPRITE)   // O (ornament onto wall) (2D)
        {
            DoSpriteOrnament(searchwall);
            message("Sprite %d ornament onto wall", searchwall);
            asksave = 1;
        }
    }

    if (PRESSED_KEYSC(M))  // M (masking walls)
    {
        if (!AIMING_AT_SPRITE && ASSERT_AIMING)
        {
            int16_t next = wall[searchwall].nextwall;

            if (next >= 0)
            {
                wall[searchwall].cstat ^= 16;
                message("Wall %d masking bit %s", searchwall, ONOFF(wall[searchwall].cstat&16));

                wall[searchwall].cstat &= ~8;

                if (wall[searchwall].cstat&16)
                {
                    if (!eitherSHIFT)
                    {
                        wall[next].cstat |= 8;           //auto other-side flip
                        wall[next].cstat |= 16;
                        wall[next].overpicnum = wall[searchwall].overpicnum;
                    }
                }
                else
                {
                    if (!eitherSHIFT)
                    {
                        wall[next].cstat &= ~8;         //auto other-side unflip
                        wall[next].cstat &= ~16;
                    }
                }

                wall[searchwall].cstat &= ~32;
                if (!eitherSHIFT)
                    wall[next].cstat &= ~32;

                asksave = 1;
            }
        }
    }

    if (PRESSED_KEYSC(H))  // H (hitscan sensitivity)
    {
        if (keystatus[KEYSC_QUOTE])
        {
            if (ASSERT_AIMING)
            {
                Bsprintf(tempbuf, "%s hitag: ", Typestr_wss[searchstat]);
                getnumberptr256(tempbuf, &AIMED(hitag), sizeof(int16_t), BTAG_MAX, 0, NULL);
            }
        }
        else
        {
            if (AIMING_AT_SPRITE)
            {
                sprite[searchwall].cstat ^= 256;
                message("Sprite %d hitscan sensitivity bit %s", searchwall, ONOFF(sprite[searchwall].cstat&256));
                asksave = 1;
            }
            else
            {
                wall[searchwall].cstat ^= 64;

                if (wall[searchwall].nextwall >= 0 && !eitherSHIFT)
                {
                    NEXTWALL(searchwall).cstat &= ~64;
                    NEXTWALL(searchwall).cstat |= (wall[searchwall].cstat&64);
                }
                message("Wall %d hitscan sensitivity bit %s", searchwall, ONOFF(wall[searchwall].cstat&64));

                asksave = 1;
            }
        }
    }

    smooshyalign = keystatus[KEYSC_gKP5];
    repeatpanalign = eitherSHIFT || (bstatus&2);

    if (mlook == 2)
        mlook = 0;

    if (!unrealedlook && (bstatus&4))
        mlook = 2;

    // PK: no btn: wheel changes shade
    if ((bstatus&(16|32) && !(bstatus&(1|2|4))) || keystatus[KEYSC_gMINUS] || keystatus[KEYSC_gPLUS])
    {
        //        if (bstatus&1)
        //            mlook = 2;
        tsign = 0;
        if (bstatus&32 || PRESSED_KEYSC(gMINUS))  // -
            tsign = 1;
        if (bstatus&16 || PRESSED_KEYSC(gPLUS))  // +
            tsign = -1;

        if (tsign)
        {
            mouseb &= ~(16|32);
            bstatus &= ~(16|32);

            if (eitherALT)  //ALT
            {
                if (eitherCTRL)  //CTRL
                {
                    if (tsign==1)
                        visibility <<= (visibility < 16384);
                    else
                        visibility >>= (visibility > 32);
                    message("Global visibility %d", visibility);
                }
                else
                {
                    k=eitherSHIFT?1:16;

                    if (highlightsectorcnt >= 0)
                        for (i=0; i<highlightsectorcnt; i++)
                            if (highlightsector[i] == searchsector)
                            {
                                while (k > 0)
                                {
                                    for (i=0; i<highlightsectorcnt; i++)
                                    {
                                        sector[highlightsector[i]].visibility += tsign;

                                        if (tsign==1)
                                        {
                                            if (sector[highlightsector[i]].visibility == 240)
                                                sector[highlightsector[i]].visibility = 239;
                                        }
                                        else if (sector[highlightsector[i]].visibility == 239)
                                            sector[highlightsector[i]].visibility = 240;
                                    }
                                    k--;
                                }
                                break;
                            }

                    while (k > 0)
                    {
                        sector[searchsector].visibility += tsign;

                        if (tsign==1)
                        {
                            if (sector[searchsector].visibility == 240)
                                sector[searchsector].visibility = 239;
                        }
                        else if (sector[searchsector].visibility == 239)
                            sector[searchsector].visibility = 240;
                        k--;
                    }
                    message("Sector %d visibility %d",searchsector,sector[searchsector].visibility);
                    asksave = 1;
                }
            }
            else  // if !eitherALT
            {
                k = 0;
                if (highlightsectorcnt >= 0)
                {
                    for (i=0; i<highlightsectorcnt; i++)
                        if (highlightsector[i] == searchsector)
                        {
                            k = 1;
                            break;
                        }
                }

                if (k == 0)
                {
                    if (ASSERT_AIMING)
                    {
                        AIMED_CF_SEL(shade) += tsign*(1+3*eitherCTRL);
                        message("%s %d shade %d", Typestr[searchstat], i, AIMED_CF_SEL(shade));
                    }
                }
                else
                {
                    for (i=0; i<highlightsectorcnt; i++)
                    {
                        dasector = highlightsector[i];

                        sector[dasector].ceilingshade += tsign;        //sector shade
                        sector[dasector].floorshade += tsign;

                        startwall = sector[dasector].wallptr;   //wall shade
                        endwall = startwall + sector[dasector].wallnum - 1;

                        for (j=startwall; j<=endwall; j++)
                            wall[j].shade += tsign;

                        j = headspritesect[dasector];           //sprite shade
                        while (j != -1)
                        {
                            sprite[j].shade += tsign;
                            j = nextspritesect[j];
                        }
                    }
                }
                asksave = 1;
            }
        }
    }

    // PK: lmb only & mousewheel, -, and +, cycle picnum
    if (keystatus[KEYSC_DASH] || keystatus[KEYSC_EQUAL] || (bstatus&(16|32) && (bstatus&1) && !(bstatus&2)))
    {
        if (ASSERT_AIMING)
        {
            j = i = (keystatus[KEYSC_EQUAL] || (bstatus&16)) ? 1 : -1;

            while (!tilesizx[AIMED_SELOVR_PICNUM] || !tilesizy[AIMED_SELOVR_PICNUM] || j)
            {
                AIMED_SELOVR_PICNUM += i;
                AIMED_SELOVR_PICNUM %= MAXTILES;
                j = 0;
            }

            asksave = 1;
        }
        keystatus[KEYSC_DASH] = keystatus[KEYSC_EQUAL] = 0;
        mouseb &= ~(16|32);
    }

    if (PRESSED_KEYSC(E))  // E (expand)
    {
        if (AIMING_AT_CEILING_OR_FLOOR)
        {
            AIMED_CEILINGFLOOR(stat) ^= 8;
            message("Sector %d %s texture expansion bit %s", searchsector, typestr[searchstat], ONOFF(sector[searchsector].ceilingstat&8));
            asksave = 1;
        }
    }

    if (PRESSED_KEYSC(R))  // R (relative alignment, rotation)
    {
        if (keystatus[KEYSC_QUOTE]) // FRAMERATE TOGGLE
        {
            framerateon = !framerateon;
            message("Show framerate %s", ONOFF(framerateon));
        }
        else
        {
            if (AIMING_AT_CEILING_OR_FLOOR)
            {
                AIMED_CEILINGFLOOR(stat) ^= 64;
                message("Sector %d %s texture relativity bit %s", searchsector, typestr[searchstat], ONOFF(AIMED_CEILINGFLOOR(stat)&64));
                asksave = 1;
            }
            else if (AIMING_AT_SPRITE)
            {
                static const char *aligntype[4] = { "view", "wall", "floor", "???" };

                i = sprite[searchwall].cstat;
                if ((i&48) < 32) i += 16;
                else i &= ~48;
                sprite[searchwall].cstat = i;

                message("Sprite %d now %s aligned", searchwall, aligntype[(i&48)/16]);
                asksave = 1;
            }
        }
    }

    if (PRESSED_KEYSC(F))  //F (Flip)
    {
        if (eitherALT)  //ALT-F (relative alignmment flip)
        {
            if (!AIMING_AT_SPRITE && ASSERT_AIMING)
            {
                setfirstwall(searchsector, searchwall);
                message("Sector %d first wall", searchsector);
                asksave = 1;
            }
        }
        else
        {
            if (AIMING_AT_WALL_OR_MASK)
            {
                static const int32_t next[4] = { 1, 3, 0, 2 };  // 0->1->3->2->0

                i = wall[searchbottomwall].cstat;
                i = ((i>>3)&1)+((i>>7)&2);    //3-x,8-y

                i = next[i];
                message("Wall %d flip %d",searchwall,i);

                i = ((i&1)<<3)+((i&2)<<7);
                wall[searchbottomwall].cstat &= ~0x0108;
                wall[searchbottomwall].cstat |= i;
                asksave = 1;
            }
            else if (AIMING_AT_CEILING_OR_FLOOR)  //8-way ceiling/floor flipping (bits 2,4,5)
            {
                static const int32_t next[8] = { 6, 7, 4, 5, 0, 1, 3, 2 };  // 0->6->3->5->1->7->2->4->0
                int16_t *stat = AIMING_AT_CEILING ? &sector[searchsector].ceilingstat : &sector[searchsector].floorstat;

                i = *stat;
                i = (i&0x4)+((i>>4)&3);
                i = next[i];
                message("Sector %d flip %d", searchsector, i);
                i = (i&0x4)+((i&3)<<4);
                *stat &= ~0x34;
                *stat |= i;
                asksave = 1;
            }
            else if (AIMING_AT_SPRITE)
            {
                i = sprite[searchwall].cstat;
                if (((i&48) == 32) && ((i&64) == 0))
                {
                    sprite[searchwall].cstat &= ~0xc;
                    sprite[searchwall].cstat |= ((i&4)^4);
                    message("Sprite %d flip bit %s",searchwall, ONOFF(sprite[searchwall].cstat&4));
                }
                else
                {
                    int32_t next[4] = { 1, 3, 0, 2 };  // 0->1->3->2->0
                    i = ((i>>2)&3);

                    i = next[i];
                    message("Sprite %d flip %d",searchwall,i);

                    sprite[searchwall].cstat &= ~0xc;
                    sprite[searchwall].cstat |= (i<<2);
                }
                asksave = 1;
            }
        }
    }


    if (keystatus[KEYSC_HOME])
        updownunits = 256;
    else if (keystatus[KEYSC_END])
        updownunits = 512;
    else
        updownunits = 1024;

    mouseaction=0;
    if (eitherALT && (bstatus&1))
    {
        mousex=0; mskip=1;
        if (mousey!=0)
        {
            updownunits=klabs(mousey*128);
            mouseaction=1;
        }
    }


    tsign = 0;
    if (ASSERT_AIMING)
    {
        // PK: PGUP/PGDN, rmb only & mwheel
        tsign -= (PRESSED_KEYSC(PGUP) || (mouseaction && mousey<0) || ((bstatus&18)==18 && !(bstatus&1)));
        tsign += (PRESSED_KEYSC(PGDN) || (mouseaction && mousey>0) || ((bstatus&34)==34 && !(bstatus&1)));
    }

    if (tsign)
    {
        k = 0;
        if (highlightsectorcnt >= 0)
        {
            for (i=0; i<highlightsectorcnt; i++)
                if (highlightsector[i] == searchsector)
                {
                    k = highlightsectorcnt;
                    break;
                }
        }

        if (AIMING_AT_WALL || AIMING_AT_CEILING)
        {
            int16_t sect = k ? highlightsector[0] :
                           ((AIMING_AT_WALL && wall[searchwall].nextsector>=0 && (eitherALT && !(bstatus&1))) ?
                            wall[searchwall].nextsector : searchsector);

            for (j=0; j<(k?k:1); j++, sect=highlightsector[j])
            {
                i = headspritesect[sect];
                while (i != -1)
                {
                    tempint = getceilzofslope(sect, sprite[i].x, sprite[i].y);
                    tempint += (tilesizy[sprite[i].picnum]*sprite[i].yrepeat)<<2;

                    if (sprite[i].cstat&128)
                        tempint += (tilesizy[sprite[i].picnum]*sprite[i].yrepeat)<<1;

                    if (sprite[i].z == tempint)
                        sprite[i].z += tsign * (updownunits << (eitherCTRL<<1));   // JBF 20031128

                    i = nextspritesect[i];
                }

                sector[sect].ceilingz += tsign * (updownunits << (eitherCTRL<<1));   // JBF 20031128
                message("Sector %d ceilingz = %d", sect, sector[sect].ceilingz);
            }
        }
        else if (AIMING_AT_FLOOR)
        {
            int16_t sect = k ? highlightsector[0] : searchsector;

            for (j=0; j<(k?k:1); j++, sect=highlightsector[j])
            {
                i = headspritesect[sect];
                while (i != -1)
                {
                    tempint = getflorzofslope(sect,sprite[i].x,sprite[i].y);

                    if (sprite[i].cstat&128)
                        tempint += ((tilesizy[sprite[i].picnum]*sprite[i].yrepeat)<<1);

                    if (sprite[i].z == tempint)
                        sprite[i].z += tsign * (updownunits << (eitherCTRL<<1));   // JBF 20031128

                    i = nextspritesect[i];
                }

                sector[sect].floorz += tsign * (updownunits << (eitherCTRL<<1)); // JBF 20031128
                message("Sector %d floorz = %d",sect,sector[sect].floorz);
            }
        }

        if (sector[searchsector].floorz < sector[searchsector].ceilingz)
        {
            if (tsign==-1)
                sector[searchsector].floorz = sector[searchsector].ceilingz;
            else
                sector[searchsector].ceilingz = sector[searchsector].floorz;
        }

        if (AIMING_AT_SPRITE)
        {
            if (eitherCTRL && !eitherALT)  //CTRL - put sprite on ceiling/floor
            {
                if (tsign==-1)
                    sprite[searchwall].z = spriteonceilingz(searchwall);
                else
                    sprite[searchwall].z = spriteongroundz(searchwall);
            }
            else
            {
                k = 0;
                if (highlightcnt >= 0)
                    for (i=0; i<highlightcnt; i++)
                        if (highlight[i] == searchwall+16384)
                        {
                            k = 1;
                            break;
                        }

                if (k == 0)
                {
                    sprite[searchwall].z += tsign * (updownunits << ((eitherCTRL && mouseaction)*3));
                    if (!spnoclip)
                        sprite[searchwall].z = max(sprite[searchwall].z, spriteonceilingz(searchwall));

                    message("Sprite %d z = %d", searchwall, sprite[searchwall].z);

                }
                else
                {
                    for (i=0; i<highlightcnt; i++)
                        if ((highlight[i]&0xc000) == 16384)
                        {
                            int16_t sp = highlight[i]&16383;

                            sprite[sp].z += tsign * updownunits;

                            if (!spnoclip)
                            {
                                if (tsign==-1)
                                    sprite[sp].z = max(sprite[sp].z, spriteonceilingz(sp));
                                else
                                    sprite[sp].z = min(sprite[sp].z, spriteongroundz(sp));
                            }
                        }

                    message("Sprites %s by %d units", tsign<0 ? "raised" : "lowered", updownunits);
//                    message("Sprite %d z = %d", highlight[i]&16383, sprite[highlight[i]&16383].z);
                }
            }
        }

        asksave = 1;
        mouseb &= ~(16|32);
    }

    /* end Mapster32 */

    //  DoWater(horiz);

    if (framerateon)
    {
        static int32_t FrameCount = 0;
        static int32_t LastCount = 0;
        static int32_t LastSec = 0;
        static int32_t LastMS = 0;

        int32_t ms = getticks();
        int32_t howlong = ms - LastMS;

        if (howlong >= 0)
        {
            int32_t thisSec = ms/1000;
            int32_t x = (xdim <= 640);
            int32_t chars = Bsprintf(tempbuf, "%2u ms (%3u fps)", howlong, LastCount);

            if (!x)
            {
                printext256(windowx2-(chars<<3)+1,windowy1+2,0,-1,tempbuf,x);
                printext256(windowx2-(chars<<3),windowy1+1,COLOR_WHITE,-1,tempbuf,x);
            }
            else
            {
                printext256(windowx2-(chars<<2)+1,windowy1+2,0,-1,tempbuf,x);
                printext256(windowx2-(chars<<2),windowy1+1,COLOR_WHITE,-1,tempbuf,x);
            }

            if (LastSec < thisSec)
            {
                LastCount = FrameCount / (thisSec - LastSec);
                LastSec = thisSec;
                FrameCount = 0;
            }
            FrameCount++;
        }
        LastMS = ms;
    }

    tempbuf[0] = 0;

    if ((bstatus&(4|2|1))==4 && !unrealedlook)  //PK
        Bsprintf(tempbuf,"VIEW");
    else if ((bstatus&(2|1))==2)
        Bsprintf(tempbuf,"Z%s", keystatus[KEYSC_HOME]?" 256":keystatus[KEYSC_END]?" 512":"");
    else if ((bstatus&(2|1))==1)
        Bsprintf(tempbuf,"LOCK");

    if (bstatus&1)
    {
        Bsprintf(tempbuf,"LOCK");
        switch (searchstat)
        {
        case SEARCH_WALL:
        case SEARCH_MASKWALL:
            if (eitherALT)
                Bsprintf(tempbuf,"CEILING Z %s", eitherCTRL?"512":"");
            else if (eitherSHIFT)
                Bsprintf(tempbuf,"PAN %s", eitherCTRL?"8":"");
            else if (eitherCTRL)
                Bsprintf(tempbuf,"SCALE");
            break;
        case SEARCH_CEILING:
        case SEARCH_FLOOR:
            if (eitherALT)
                Bsprintf(tempbuf,"%s Z %s", AIMING_AT_CEILING?"CEILING":"FLOOR", eitherCTRL?"512":"");
            else if (eitherSHIFT)
                Bsprintf(tempbuf,"PAN");
            else if (eitherCTRL)
                Bsprintf(tempbuf,"SLOPE");
            break;
        case SEARCH_SPRITE:
            if (eitherALT)
                Bsprintf(tempbuf,"MOVE Z %s", eitherCTRL?"1024":"");
            else if (eitherSHIFT)
                Bsprintf(tempbuf,"MOVE XY %s", eitherCTRL?"GRID":"");
            else if (eitherCTRL)
                Bsprintf(tempbuf,"SIZE");
            break;
        }
    }

    if (tempbuf[0] != 0)
    {
        i = (Bstrlen(tempbuf)<<3)+6;
        i = max((searchx+i)-(xdim-1), 0);

        j = max((searchy+16)-(ydim-1), 0);

        //            printext16(searchx+6-i,searchy+6-j,11,-1,tempbuf,0);
        printext256(searchx+4+2-i, searchy+4+2-j, 0,-1,tempbuf,!(xdimgame > 640));
        printext256(searchx+4-i,   searchy+4-j,   whitecol,-1,tempbuf,!(xdimgame > 640));

        //        printext256(searchx+4+2,searchy+4+2,0,-1,tempbuf,!(xdimgame > 640));
        //        printext256(searchx+4,searchy+4,whitecol,-1,tempbuf,!(xdimgame > 640));
    }
    if (helpon==1)
    {
        int32_t small = !(xdimgame > 640);
        for (i=0; i<MAXHELP3D; i++)
        {
            begindrawing();
            printext256(2, 8+(i*(8+!small))+2, 0, -1, Help3d[i], small);
            printext256(0, 8+(i*(8+!small)), whitecol, -1, Help3d[i], small);
            enddrawing();

            switch (i)
            {
            case 8:
                Bsprintf(tempbuf,"%d",autosave);
                break;
            case 9:
                Bsprintf(tempbuf,"%s",SKILLMODE[skill]);
                break;
            case 10:
                Bsprintf(tempbuf,"%d",framerateon);
                break;
            case 11:
                Bsprintf(tempbuf,"%s",SPRDSPMODE[nosprites]);
                break;
            case 12:
                Bsprintf(tempbuf,"%d",shadepreview);
                break;
            case 13:
                Bsprintf(tempbuf,"%d",purpleon);
                break;
            default :
                Bsprintf(tempbuf," ");
                break;
            }

            begindrawing();
            if (!Bstrcmp(tempbuf,"0"))
                Bsprintf(tempbuf,"OFF");
            else if (!Bstrcmp(tempbuf,"1"))
                Bsprintf(tempbuf,"ON");
            else if (!Bstrcmp(tempbuf,"2"))
                Bsprintf(tempbuf,"ON (2)");

            printext256((20+(!small * 20))*8+2, 8+(i*(8+!small))+2, 0, -1, tempbuf, small);
            printext256((20+(!small * 20))*8, 8+(i*(8+!small)), whitecol, -1, tempbuf, small);
            enddrawing();
        }
    }

    /* if(purpleon) {
    begindrawing();
    //          printext256(1*4,1*8,whitecol,-1,"Purple ON",0);
    message("Purple ON");
    enddrawing();
    }
    */

    if (sector[cursectnum].lotag==2)
    {
        if (sector[cursectnum].ceilingpicnum==FLOORSLIME)
            SetSLIMEPalette();
        else
            SetWATERPalette();
    }
    else SetGAMEPalette();

    if (keystatus[buildkeys[BK_MODE2D_3D]])  // Enter
    {
        SetGAMEPalette();
        FX_StopAllSounds();
        S_ClearSoundLocks();

#ifdef POLYMER
        for (i=0; i<MAXSPRITES; i++)
            if (spritelightptr[i] != NULL)
            {
                polymer_deletelight(spritelightid[i]);
                spritelightptr[i] = NULL;
            }
#endif
    }

    //Stick this in 3D part of ExtCheckKeys
    //Also choose your own key scan codes

    if (keystatus[KEYSC_QUOTE] && PRESSED_KEYSC(D)) // ' d
        /* { ShowHelpText("SectorEffector"); } */
    {
        skill = (skill+1)%MAXSKILL;
        message("%s", SKILLMODE[skill]);
        //        printext256(1*4,1*8,11,-1,tempbuf,0);
    }

    if (keystatus[KEYSC_QUOTE] && PRESSED_KEYSC(I))  // ' i
    {
        showinvisibility = !showinvisibility;
        message("Invisible sprite preview %s", showinvisibility?"enabled":"disabled");
    }

    if (keystatus[KEYSC_QUOTE] && PRESSED_KEYSC(X)) // ' x
    {
        shadepreview = !shadepreview;
        message("Map shade preview %s", shadepreview?"enabled":"disabled");

#ifdef POLYMER
        for (i=0; i<MAXSPRITES; i++)
            if (spritelightptr[i] != NULL)
            {
                polymer_deletelight(spritelightid[i]);
                spritelightptr[i] = NULL;
            }
#endif
    }


    /*    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_G]) // ' g <Unused>
    {
    keystatus[KEYSC_G] = 0;
    tabgraphic++;
    if (tabgraphic > 2) tabgraphic = 0;
    if (tabgraphic) message("Graphics ON");
    else message("Graphics OFF");
    }*/

    /*    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_R]) // ' r <Handled already>
    {
    keystatus[KEYSC_R] = 0;
    framerateon=!framerateon;
    if (framerateon) message("Framerate ON");
    else message("Framerate OFF");
    }*/

    if (keystatus[KEYSC_QUOTE] && PRESSED_KEYSC(W)) // ' w
    {
        nosprites = (nosprites+1)%MAXNOSPRITES;
        message("%s", SPRDSPMODE[nosprites]);
    }

    if (keystatus[KEYSC_QUOTE] && PRESSED_KEYSC(Y)) // ' y
    {
        purpleon = !purpleon;
        if (nosprites>3) nosprites=0;
        message("Purple %s", ONOFF(purpleon));
    }

    if (keystatus[KEYSC_QUOTE] && PRESSED_KEYSC(C)) // ' C
    {
        if (AIMING_AT_WALL_OR_MASK)
        {
            for (i=0; i<MAXWALLS; i++)
                if (wall[i].picnum==temppicnum)
                    wall[i].shade=tempshade;
        }
        else if (AIMING_AT_CEILING_OR_FLOOR)
        {
            for (i=0; i<MAXSECTORS; i++)
                if (CEILINGFLOOR(i, picnum)==temppicnum)
                    CEILINGFLOOR(i, shade)=tempshade;
        }
        else if (AIMING_AT_SPRITE)
        {
            for (i=0; i<MAXSPRITES; i++)
                if (sprite[i].picnum==temppicnum)
                    sprite[i].shade=tempshade;
        }

        if (ASSERT_AIMING)
        {
            message("%ss with picnum %d have shade of %d", Typestr[searchstat], temppicnum, tempshade);
            asksave = 1;
        }
    }

    if (keystatus[KEYSC_QUOTE] && PRESSED_KEYSC(T)) // ' T
    {
        if (AIMING_AT_WALL_OR_MASK)
        {
            wall[searchwall].lotag = getnumber256("Wall lotag: ", wall[searchwall].lotag, BTAG_MAX, 0);
        }
        else if (AIMING_AT_CEILING_OR_FLOOR)
        {
            sector[searchsector].lotag =
                _getnumber256("Sector lotag: ", sector[searchsector].lotag, BTAG_MAX, 0, (void *)ExtGetSectorType);
        }
        else if (AIMING_AT_SPRITE)
        {
            if (sprite[searchwall].picnum == SECTOREFFECTOR)
            {
                sprite[searchwall].lotag =
                    _getnumber256("Sprite lotag: ", sprite[searchwall].lotag, BTAG_MAX, 0, (void *)SectorEffectorTagText);
            }
            else if (sprite[searchwall].picnum == MUSICANDSFX)
            {
                int16_t oldtag = sprite[searchwall].lotag;

                sprite[searchwall].lotag =
                    _getnumber256("Sprite lotag: ", sprite[searchwall].lotag, BTAG_MAX, 0, (void *)MusicAndSFXTagText);

                if ((sprite[searchwall].filler&1) && sprite[searchwall].lotag != oldtag)
                {
                    sprite[searchwall].filler &= ~1;
                    S_StopEnvSound(oldtag, searchwall);
                }
            }
            else
                sprite[searchwall].lotag = getnumber256("Sprite lotag: ", sprite[searchwall].lotag, BTAG_MAX, 0);
        }
    }

    if (keystatus[KEYSC_QUOTE] && PRESSED_KEYSC(H)) // ' H
    {
        if (ASSERT_AIMING)
        {
            int16_t ohitag = AIMED(hitag);
            Bsprintf(tempbuf, "%s hitag: ", Typestr_wss[searchstat]);
            AIMED(hitag) = getnumber256(tempbuf, ohitag, BTAG_MAX,0);
            asksave |= (AIMED(hitag) != ohitag);
        }
    }

    if (keystatus[KEYSC_QUOTE] && PRESSED_KEYSC(S)) // ' S
    {
        if (ASSERT_AIMING)
        {
            int8_t oshade = AIMED_CF_SEL(shade);
            Bsprintf(tempbuf, "%s shade: ", Typestr[searchstat]);
            getnumberptr256(tempbuf, &AIMED_CF_SEL(shade), sizeof(int8_t), 128, 1, NULL);
            asksave |= (AIMED_CF_SEL(shade) != oshade);
        }
    }

    if (PRESSED_KEYSC(F2))  // F2
    {
        if (eitherCTRL || eitherSHIFT)
            infobox ^= (eitherSHIFT | ((eitherCTRL)<<1));
        else
            usedcount = !usedcount;
    }

#if 0
    if (keystatus[KEYSC_TAB])  // TAB : USED
    {
        //        usedcount=!usedcount;
        count=0;

        for (i=0; i<numwalls; i++)
        {
            if (wall[i].picnum == temppicnum) count++;
            if (wall[i].overpicnum == temppicnum) count++;
        }
        for (i=0; i<numsectors; i++)  // JBF 20040307: was numwalls, thanks Semicharm
        {
            if (sector[i].ceilingpicnum == temppicnum) count++;
            if (sector[i].floorpicnum == temppicnum) count++;
        }

        statnum = 0;        //status 1
        i = headspritestat[statnum];
        while (i != -1)
        {
            nexti = nextspritestat[i];

            //your code goes here
            //ex: printf("Sprite %d has a status of 1 (active)\n",i,statnum);

            if (sprite[i].picnum == temppicnum) count++;
            i = nexti;
        }
    }
#endif

    if (PRESSED_KEYSC(F1)) // F1
    {
        helpon = !helpon;
//        keystatus[KEYSC_H]=0;  // delete this line?
    }

    if (PRESSED_KEYSC(G)) // G
    {
        if (ASSERT_AIMING)
        {
            int16_t opicnum = AIMED_CF_SEL(picnum);
            static const char *Typestr_tmp[5] = { "Wall", "Sector ceiling", "Sector floor", "Sprite", "Masked wall" };
            Bsprintf(tempbuf, "%s picnum: ", Typestr_tmp[searchstat]);
            getnumberptr256(tempbuf, &AIMED_CF_SEL(picnum), sizeof(int16_t), MAXTILES-1, 0, NULL);
            asksave |= (opicnum != AIMED_CF_SEL(picnum));
        }
    }

    if (PRESSED_KEYSC(B))  // B (clip Blocking xor) (3D)
    {
        if (AIMING_AT_SPRITE)
        {
            sprite[searchwall].cstat ^= 1;
            //                                sprite[searchwall].cstat &= ~256;
            //                                sprite[searchwall].cstat |= ((sprite[searchwall].cstat&1)<<8);
            message("Sprite %d blocking bit %s", searchwall, ONOFF(sprite[searchwall].cstat&1));
            asksave = 1;
        }
        else if (ASSERT_AIMING)
        {
            wall[searchwall].cstat ^= 1;
            //                                wall[searchwall].cstat &= ~64;
            if ((wall[searchwall].nextwall >= 0) && !eitherSHIFT)
            {
                NEXTWALL(searchwall).cstat &= ~(1+64);
                NEXTWALL(searchwall).cstat |= (wall[searchwall].cstat&1);
            }

            message("Wall %d blocking bit %s", searchwall, ONOFF(wall[searchwall].cstat&1));
            asksave = 1;
        }
    }

    if (PRESSED_KEYSC(T))  // T (transluscence for sprites/masked walls)
    {
        if (AIMING_AT_CEILING_OR_FLOOR)   //Set masked/transluscent ceilings/floors
        {
            int32_t nexti[4] = { 128, 256, 384, 0 };
            int16_t *stat = &AIMED_CEILINGFLOOR(stat);
            i = (*stat&(128+256))>>7;
            i = nexti[i];
            *stat &= ~(128+256);
            *stat |= i;
            asksave = 1;
        }

        if (keystatus[KEYSC_QUOTE])
        {
            if (ASSERT_AIMING)
            {
                Bsprintf(tempbuf, "%s lotag: ", Typestr_wss[searchstat]);
                AIMED(lotag) = getnumber256(tempbuf, AIMED(lotag), BTAG_MAX, 0);
            }
        }
        else if (eitherCTRL)
        {
            if (AIMING_AT_SPRITE)
                TextEntryMode(searchwall);
        }
        else
        {
            if (AIMING_AT_SPRITE)
            {
                if ((sprite[searchwall].cstat&2) == 0)
                    sprite[searchwall].cstat |= 2;
                else if ((sprite[searchwall].cstat&512) == 0)
                    sprite[searchwall].cstat |= 512;
                else
                    sprite[searchwall].cstat &= ~(2+512);
                asksave = 1;
            }
            else if (AIMING_AT_MASKWALL)
            {
                if ((wall[searchwall].cstat&128) == 0)
                    wall[searchwall].cstat |= 128;
                else if ((wall[searchwall].cstat&512) == 0)
                    wall[searchwall].cstat |= 512;
                else
                    wall[searchwall].cstat &= ~(128+512);

                if (wall[searchwall].nextwall >= 0)
                {
                    NEXTWALL(searchwall).cstat &= ~(128+512);
                    NEXTWALL(searchwall).cstat |= (wall[searchwall].cstat&(128+512));
                }
                asksave = 1;
            }
        }
    }

    if (keystatus[KEYSC_QUOTE] && PRESSED_KEYSC(ENTER)) // ' ENTER
    {
        AIMED_SELOVR_PICNUM = temppicnum;
        message("Pasted picnum only");
    }

    i = 512;
    if (keystatus[KEYSC_RSHIFT]) i = 8;
    if (keystatus[KEYSC_LSHIFT]) i = 1;
    mouseaction=0;

    if (eitherCTRL && !eitherSHIFT && (bstatus&1) && AIMING_AT_CEILING_OR_FLOOR)
    {
        mousex=0; mskip=1;
        if (mousey)
        {
            i=klabs(mousey*2);
            mouseaction=1;
        }
    }

    tsign = 0;
    if (ASSERT_AIMING)
    {
        tsign -= (PRESSED_KEYSC(LBRACK) || (mouseaction && mousey<0));   // [
        tsign += (PRESSED_KEYSC(RBRACK) || (mouseaction && mousey>0));   // ]
    }

    if (tsign)
    {
        if (eitherALT)
        {
            int32_t ns=wall[searchwall].nextsector, sx=wall[searchwall].x, sy=wall[searchwall].y;

            if (ns >= 0 && !mouseaction)
            {
                if (AIMING_AT_CEILING || (tsign < 0 && AIMING_AT_WALL_OR_MASK))
                {
                    alignceilslope(searchsector, sx, sy, getceilzofslope(ns, sx, sy));
                    message("Sector %d align ceiling to wall %d", searchsector, searchwall);
                }
                if (AIMING_AT_FLOOR || (tsign > 0 && AIMING_AT_WALL_OR_MASK))
                {
                    alignflorslope(searchsector, sx, sy, getflorzofslope(ns, sx, sy));
                    message("Sector %d align floor to wall %d", searchsector, searchwall);
                }
            }
        }
        else
        {
            if (AIMING_AT_CEILING_OR_FLOOR)
            {
                if (!(AIMED_CEILINGFLOOR(stat)&2))
                    AIMED_CEILINGFLOOR(heinum) = 0;

                AIMED_CEILINGFLOOR(heinum) = clamp(AIMED_CEILINGFLOOR(heinum) + tsign*i, -32768, 32767);

                message("Sector %d ceiling slope = %d",searchsector,AIMED_CEILINGFLOOR(heinum));
            }
        }

        if (sector[searchsector].ceilingheinum == 0)
            sector[searchsector].ceilingstat &= ~2;
        else
            sector[searchsector].ceilingstat |= 2;

        if (sector[searchsector].floorheinum == 0)
            sector[searchsector].floorstat &= ~2;
        else
            sector[searchsector].floorstat |= 2;
        asksave = 1;
    }


    if ((bstatus&1) && eitherSHIFT)
        mskip=1;

    if ((bstatus&1) && eitherSHIFT && AIMING_AT_CEILING_OR_FLOOR && (mousex|mousey))
    {
        int32_t fw,x1,y1,x2,y2,stat,ma,a=0;

        stat = AIMING_AT_FLOOR ? sector[searchsector].floorstat : sector[searchsector].ceilingstat;
        if (stat&64) // align to first wall
        {
            fw=sector[searchsector].wallptr;
            x1=wall[fw].x,y1=wall[fw].y;
            x2=POINT2(fw).x,y2=POINT2(fw).y;
            a=getangle(x1-x2,y1-y2);
        }
        mouseax+=mousex; mouseay+=mousey;
        ma=getangle(mouseax,mouseay);
        ma+=ang-a;

        i = stat;
        i = (i&0x4)+((i>>4)&3);
        if (stat&64) // align to first wall
            switch (i)
            {
            case 0:break;
            case 1:ma=-ma; break;
            case 2:ma=1024-ma; break;
            case 3:ma+=1024; break;
            case 4:ma=-512-ma; break;
            case 5:ma+=512; break;
            case 6:ma-=512; break;
            case 7:ma=512-ma; break;
            }
        else
            switch (i)
            {
            case 0:ma=-ma; break;
            case 1:break;
            case 2:ma+=1024; break;
            case 3:ma=1024-ma; break;
            case 4:ma-=512; break;
            case 5:ma=512-ma; break;
            case 6:ma=-512-ma; break;
            case 7:ma+=512; break;
            }

        a = ksqrt(mouseax*mouseax + mouseay*mouseay);
        if (a)
        {
            int32_t mult = (stat&8) ? 8192 : 8192*2;
            x1 = -a*sintable[(ma+2048)&2047]/mult;
            y1 = -a*sintable[(ma+1536)&2047]/mult;

            if (x1||y1)
            {
                mouseax=0;
                mouseay=0;

                if (AIMING_AT_CEILING_OR_FLOOR)
                {
                    changedir = 1-2*(x1<0);
                    x1 = klabs(x1);

                    while (x1--)
                        AIMED_CEILINGFLOOR(xpanning) = changechar(AIMED_CEILINGFLOOR(xpanning),changedir,0,0);

                    changedir = 1-2*(y1<0);
                    y1 = klabs(y1);

                    while (y1--)
                        AIMED_CEILINGFLOOR(ypanning) = changechar(AIMED_CEILINGFLOOR(ypanning),changedir,0,0);

                    message("Sector %d %s panning: %d, %d", searchsector, typestr[searchstat],
                            AIMED_CEILINGFLOOR(xpanning), AIMED_CEILINGFLOOR(ypanning));
                }
                asksave=1;
            }
        }
        mousex=0;
        mousey=0;
    }


    smooshyalign = keystatus[KEYSC_gKP5];
    repeatpanalign = eitherSHIFT;

    ////////////////////
    updownunits=1;
    mouseaction=0;

    if (!mouseb)
    {
        mouseax=0;
        mouseay=0;
    }

    if ((bstatus&1) && searchstat != SEARCH_CEILING && searchstat != SEARCH_FLOOR)
    {
        if (eitherSHIFT)
        {
            mskip=1;
            if (mousex)
            {
                mouseaction = 1;
                mouseax += mousex;
                updownunits = klabs((int32_t)(mouseax/2.));
                if (updownunits)
                    mouseax=0;
            }
        }
        else if (eitherCTRL && !eitherALT)
        {
            mskip=1;
            if (mousex)
            {
                mouseaction = 2;
                repeatpanalign = 0;
                updownunits = klabs(mouseax+=mousex)/(16 - 12*AIMING_AT_SPRITE);
                if (updownunits)
                    mouseax=0;
            }
        }
    }

    if (keystatus[KEYSC_gLEFT] || keystatus[KEYSC_gRIGHT] || mouseaction) // 4 & 6 (keypad)
    {
        if (repeatcountx == 0 || repeatcountx > 32 || mouseaction)
        {
            changedir = 0;
            if (keystatus[KEYSC_gLEFT]  || mousex>0)
                changedir = -1;
            if (keystatus[KEYSC_gRIGHT] || mousex<0)
                changedir = 1;

            if (AIMING_AT_WALL_OR_MASK)
            {
                if (repeatpanalign == 0)
                {
                    while (updownunits--)
                        wall[searchwall].xrepeat = changechar(wall[searchwall].xrepeat, changedir, smooshyalign, 1);
                    message("Wall %d repeat: %d, %d", searchwall, wall[searchwall].xrepeat, wall[searchwall].yrepeat);
                }
                else
                {
                    int16_t w = SELECT_WALL();

                    if (mouseaction)
                    {
                        i=wall[w].cstat;
                        i=((i>>3)&1)+((i>>7)&2);

                        if (i==1||i==3)
                            changedir*=-1;

                        if (eitherCTRL)
                            updownunits *= 8;
                    }

                    while (updownunits--)
                        wall[w].xpanning = changechar(wall[w].xpanning, changedir, smooshyalign, 0);
                    message("Wall %d panning: %d, %d", w, wall[w].xpanning, wall[w].ypanning);
                }
                asksave = 1;
            }
            else if (AIMING_AT_CEILING_OR_FLOOR)
            {
                while (updownunits--)
                    AIMED_CEILINGFLOOR(xpanning) = changechar(AIMED_CEILINGFLOOR(xpanning), changedir, smooshyalign, 0);
                message("Sector %d %s panning: %d, %d", searchsector, typestr[searchstat],
                        AIMED_CEILINGFLOOR(xpanning), AIMED_CEILINGFLOOR(ypanning));
                asksave = 1;
            }
            else if (AIMING_AT_SPRITE)
            {
                static int32_t sumxvect=0, sumyvect=0;

                if (mouseaction==1)
                    mouseaction_movesprites(&sumxvect, &sumyvect, 1536, mousex);
                else
                {
                    sumxvect = sumyvect = 0;
                    if (mouseaction==2)
                        changedir *= -1;
                    while (updownunits--)
                        sprite[searchwall].xrepeat = changechar(sprite[searchwall].xrepeat, changedir, smooshyalign, 1);
                    if (sprite[searchwall].xrepeat < 4)
                        sprite[searchwall].xrepeat = 4;
                    message("Sprite %d repeat: %d, %d", searchwall, sprite[searchwall].xrepeat, sprite[searchwall].yrepeat);
                }
            }
            asksave = 1;
            repeatcountx = max(1,repeatcountx-2);
        }
        repeatcountx += synctics;
    }
    else
        repeatcountx = 0;


    ////////////////////
    updownunits=1;
    mouseaction=0;

    if ((bstatus&1) && !AIMING_AT_CEILING_OR_FLOOR)
    {
        if (eitherSHIFT)
        {
            mskip=1;
            if (mousey)
            {
                mouseaction = 1;
                updownunits = klabs(mousey);

                if (!AIMING_AT_SPRITE)
                    updownunits = klabs((int32_t)(mousey*128./tilesizy[wall[searchwall].picnum]));
            }
        }
        else if (eitherCTRL && !eitherALT)
        {
            mskip=1;
            if (mousey)
            {
                mouseaction = 2;
                repeatpanalign = 0;
                mouseay += mousey;
                updownunits = klabs(mouseay)/(32 - 28*AIMING_AT_SPRITE);
                if (updownunits)
                    mouseay=0;
            }
        }
    }

    if (!mouseb)
    {
        mouseax=0;
        mouseay=0;
    }

    if (keystatus[KEYSC_gUP] || keystatus[KEYSC_gDOWN] || mouseaction)  // 2 & 8 (keypad)
    {
        if (repeatcounty == 0 || repeatcounty > 32 || mouseaction)
        {
            changedir = 0;
            if (keystatus[KEYSC_gUP]   || mousey>0)
                changedir = -1;
            if (keystatus[KEYSC_gDOWN] || mousey<0)
                changedir = 1;

            if (AIMING_AT_WALL_OR_MASK)
            {
                if (repeatpanalign == 0)
                {
                    while (updownunits--)
                        wall[searchwall].yrepeat = changechar(wall[searchwall].yrepeat, changedir, smooshyalign, 1);
                    message("Wall %d repeat: %d, %d", searchwall, wall[searchwall].xrepeat, wall[searchwall].yrepeat);
                }
                else
                {
                    int16_t w = AIMING_AT_WALL?searchbottomwall:searchwall;
                    if (mouseaction && eitherCTRL)
                        updownunits *= 8;
                    while (updownunits--)
                        wall[w].ypanning = changechar(wall[w].ypanning, changedir, smooshyalign, 0);
                    message("Wall %d panning: %d, %d", w, wall[w].xpanning, wall[w].ypanning);
                }
            }
            else if (AIMING_AT_CEILING_OR_FLOOR)
            {
                while (updownunits--)
                    AIMED_CEILINGFLOOR(ypanning) = changechar(AIMED_CEILINGFLOOR(ypanning), changedir, smooshyalign, 0);
                message("Sector %d %s panning: %d, %d", searchsector, typestr[searchstat],
                        AIMED_CEILINGFLOOR(xpanning), AIMED_CEILINGFLOOR(ypanning));
                asksave = 1;
            }
            else if (AIMING_AT_SPRITE)
            {
                static int32_t sumxvect=0, sumyvect=0;

                if (mouseaction==1)
                    mouseaction_movesprites(&sumxvect, &sumyvect, 2048, mousey);
                else
                {
                    sumxvect = sumyvect = 0;
                    while (updownunits--)
                        sprite[searchwall].yrepeat = changechar(sprite[searchwall].yrepeat, changedir, smooshyalign, 1);
                    if (sprite[searchwall].yrepeat < 4)
                        sprite[searchwall].yrepeat = 4;
                    message("Sprite %d repeat: %d, %d", searchwall, sprite[searchwall].xrepeat, sprite[searchwall].yrepeat);
                }
            }
            asksave = 1;
            repeatcounty = max(1,repeatcounty-2);
        }
        repeatcounty += synctics;
    }
    else
        repeatcounty = 0;

    ////////////////////

    if (PRESSED_KEYSC(F11))  //F11 - brightness
    {
        static int16_t brightness = -1;

        if (brightness==-1)
            brightness = ((int16_t)((vid_gamma-1.0)*10.0))&15;

        brightness = brightness + (1-2*eitherSHIFT);
        brightness &= 15;

        vid_gamma = 1.0 + ((float)brightness / 10.0);
        setbrightness(brightness, palette, 0);
        message("Brightness: %d/16", brightness+1);
    }

    if (PRESSED_KEYSC(TAB))  //TAB
    {
        if (ASSERT_AIMING)
        {
            tempshade = AIMED_CF_SEL(shade);
            temppal = AIMED_CF_SEL(pal);
            templotag = AIMED_SEL(lotag);
            temphitag = AIMED_SEL(hitag);
            tempextra = AIMED_SEL(extra);

            if (AIMING_AT_WALL_OR_MASK)
            {
                temppicnum = AIMED_SELOVR_WALL(picnum);
                tempxrepeat = AIMED_SEL_WALL(xrepeat);
                tempyrepeat = AIMED_SEL_WALL(yrepeat);
                tempcstat = AIMED_SEL_WALL(cstat);
            }
            else if (AIMING_AT_CEILING_OR_FLOOR)
            {
                temppicnum = AIMED_CEILINGFLOOR(picnum);
                tempvis = sector[searchsector].visibility;
                tempxrepeat = AIMED_CEILINGFLOOR(xpanning);
                tempyrepeat = AIMED_CEILINGFLOOR(ypanning);
                tempcstat = AIMED_CEILINGFLOOR(stat);
            }
            else if (AIMING_AT_SPRITE)
            {
                temppicnum = sprite[searchwall].picnum;
                tempxrepeat = sprite[searchwall].xrepeat;
                tempyrepeat = sprite[searchwall].yrepeat;
                tempcstat = sprite[searchwall].cstat;
                tempxvel = sprite[searchwall].xvel;
                tempyvel = sprite[searchwall].yvel;
                tempzvel = sprite[searchwall].zvel;
            }

            somethingintab = searchstat;
        }
    }

    if (PRESSED_KEYSC(ENTER))  // ENTER -- paste clipboard contents
    {
        extern char pskysearch[MAXSECTORS];
        int16_t daang;
        int32_t dashade[2];

        if (eitherSHIFT)
        {
            if (AIMING_AT_WALL_OR_MASK && eitherCTRL)  //Ctrl-shift Enter (auto-shade)
            {
                dashade[0] = 127;
                dashade[1] = -128;

                i = searchwall;
                do
                {
                    if (dashade[0] > (int32_t)wall[i].shade)
                        dashade[0] = wall[i].shade;
                    if (dashade[1] < (int32_t)wall[i].shade)
                        dashade[1] = wall[i].shade;

                    i = wall[i].point2;
                }
                while (i != searchwall);

                daang = getangle(POINT2(searchwall).x-wall[searchwall].x, POINT2(searchwall).y-wall[searchwall].y);

                i = searchwall;
                do
                {
                    j = getangle(POINT2(i).x-wall[i].x,POINT2(i).y-wall[i].y);
                    k = (j+2048-daang)&2047;
                    if (k > 1024)
                        k = 2048-k;

                    wall[i].shade = dashade[0]+mulscale10(k, dashade[1]-dashade[0]);

                    i = wall[i].point2;
                }
                while (i != searchwall);

                message("Wall %d auto-shaded", searchwall);
            }
            else if (somethingintab < 255)
            {
                if (ASSERT_AIMING)
                {
                    AIMED_CF_SEL(shade) = tempshade;
                    AIMED_CF_SEL(pal) = temppal;

                    if (AIMING_AT_CEILING_OR_FLOOR)
                    {
                        if (somethingintab == SEARCH_CEILING || somethingintab == SEARCH_FLOOR)
                            sector[searchsector].visibility = tempvis;
                    }

                    message("Pasted shading+pal");
                    asksave = 1;
                }
            }
        }
        else if (AIMING_AT_WALL_OR_MASK && eitherCTRL && somethingintab < 255)  //Either ctrl key
        {
            i = searchwall;
            do
            {
                wall[i].picnum = temppicnum;
                wall[i].shade = tempshade;
                wall[i].pal = temppal;

                if (somethingintab == SEARCH_WALL || somethingintab == SEARCH_MASKWALL)
                {
                    wall[i].xrepeat = tempxrepeat;
                    wall[i].yrepeat = tempyrepeat;
                    wall[i].cstat = tempcstat;
                }

                fixrepeats((int16_t)i);
                i = wall[i].point2;
            }
            while (i != searchwall);

            message("Pasted picnum+shading+pal");
            asksave = 1;
        }
        else if (AIMING_AT_CEILING_OR_FLOOR && eitherCTRL && somethingintab < 255)  //Either ctrl key
        {
            Bmemset(pskysearch, 0, numsectors);

            i = searchsector;
            if (CEILINGFLOOR(i, stat)&1)
                pskysearch[i] = 1;

            while (pskysearch[i] == 1)
            {
                CEILINGFLOOR(i, picnum) = temppicnum;
                CEILINGFLOOR(i, shade) = tempshade;
                CEILINGFLOOR(i, pal) = temppal;

                if (somethingintab == SEARCH_CEILING || somethingintab == SEARCH_FLOOR)
                {
                    CEILINGFLOOR(i, xpanning) = tempxrepeat;
                    CEILINGFLOOR(i, ypanning) = tempyrepeat;
                    CEILINGFLOOR(i, stat) = tempcstat;
                }
                pskysearch[i] = 2;

                startwall = sector[i].wallptr;
                endwall = startwall + sector[i].wallnum - 1;

                for (j=startwall; j<=endwall; j++)
                {
                    k = wall[j].nextsector;
                    if (k >= 0)
                        if ((CEILINGFLOOR(k, stat)&1) > 0)
                            if (pskysearch[k] == 0)
                                pskysearch[k] = 1;
                }

                for (j=0; j<numsectors; j++)
                    if (pskysearch[j] == 1)
                        i = j;
            }

            message("Pasted picnum+shading+pal");
            asksave = 1;
        }
        else if (somethingintab < 255)
        {
            if (AIMING_AT_WALL)
            {
                wall[searchbottomwall].picnum = temppicnum;
                wall[searchbottomwall].shade = tempshade;
                wall[searchbottomwall].pal = temppal;

                if (somethingintab == SEARCH_WALL && searchwall==searchbottomwall)
                {
                    wall[searchwall].xrepeat = tempxrepeat;
                    wall[searchwall].yrepeat = tempyrepeat;
                    wall[searchwall].cstat = tempcstat;
                    wall[searchwall].lotag = templotag;
                    wall[searchwall].hitag = temphitag;
                    wall[searchwall].extra = tempextra;
                }
                fixrepeats(searchwall);
                asksave = 1;
            }
            else if (AIMING_AT_MASKWALL)
            {
                wall[searchwall].overpicnum = temppicnum;
                if (wall[searchwall].nextwall >= 0)
                    NEXTWALL(searchwall).overpicnum = temppicnum;

                wall[searchwall].shade = tempshade;
                wall[searchwall].pal = temppal;

                if (somethingintab == SEARCH_MASKWALL)
                {
                    wall[searchwall].xrepeat = tempxrepeat;
                    wall[searchwall].yrepeat = tempyrepeat;
                    wall[searchwall].cstat = tempcstat;
                    wall[searchwall].lotag = templotag;
                    wall[searchwall].hitag = temphitag;
                    wall[searchwall].extra = tempextra;
                }
                fixrepeats(searchwall);
                asksave = 1;
            }
            else if (AIMING_AT_CEILING_OR_FLOOR)
            {
                AIMED_CEILINGFLOOR(picnum) = temppicnum;
                AIMED_CEILINGFLOOR(shade) = tempshade;
                AIMED_CEILINGFLOOR(pal) = temppal;

                if (somethingintab == SEARCH_CEILING || somethingintab == SEARCH_FLOOR)
                {
                    AIMED_CEILINGFLOOR(xpanning) = tempxrepeat;
                    AIMED_CEILINGFLOOR(ypanning) = tempyrepeat;
                    AIMED_CEILINGFLOOR(stat) = tempcstat;

                    sector[searchsector].visibility = tempvis;
                    sector[searchsector].lotag = templotag;
                    sector[searchsector].hitag = temphitag;
                    sector[searchsector].extra = tempextra;
                }
                asksave = 1;
            }
            else if (AIMING_AT_SPRITE)
            {
                sprite[searchwall].picnum = temppicnum;

                if (tilesizx[temppicnum] <= 0 || tilesizy[temppicnum] <= 0)
                {
                    j = 0;
                    for (k=0; k<MAXTILES; k++)
                        if (tilesizx[k] > 0 && tilesizy[k] > 0)
                        {
                            j = k;
                            break;
                        }
                    sprite[searchwall].picnum = j;
                }

                sprite[searchwall].shade = tempshade;
                sprite[searchwall].pal = temppal;

                if (somethingintab == SEARCH_SPRITE)
                {
                    sprite[searchwall].xrepeat = max(tempxrepeat, 1);
                    sprite[searchwall].yrepeat = max(tempyrepeat, 1);
                    sprite[searchwall].cstat = tempcstat;
                    sprite[searchwall].lotag = templotag;
                    sprite[searchwall].hitag = temphitag;
                    sprite[searchwall].extra = tempextra;
                    sprite[searchwall].xvel = tempxvel;
                    sprite[searchwall].yvel = tempyvel;
                    sprite[searchwall].zvel = tempzvel;
                }
            }
            message("Pasted clipboard");
            asksave = 1;
        }
    }

    if (PRESSED_KEYSC(C))
    {
        if (eitherALT)  // Alt-C  picnum replacer
        {
            if (somethingintab < 255)
            {
                switch (searchstat)
                {
                case SEARCH_WALL:
                    j = wall[searchbottomwall].picnum;
                    for (i=0; i<numwalls; i++)
                        if (wall[i].picnum == j)
                            wall[i].picnum = temppicnum;
                    break;
                case SEARCH_CEILING:
                case SEARCH_FLOOR:
                    j = AIMED_CEILINGFLOOR(picnum);
                    for (i=0; i<numsectors; i++)
                        if (CEILINGFLOOR(i, picnum) == j)
                            CEILINGFLOOR(i, picnum) = temppicnum;
                    break;
                case SEARCH_SPRITE:
                    j = sprite[searchwall].picnum;
                    for (i=0; i<MAXSPRITES; i++)
                        if (sprite[i].statnum < MAXSTATUS)
                            if (sprite[i].picnum == j)
                                sprite[i].picnum = temppicnum;
                    break;
                case SEARCH_MASKWALL:
                    j = wall[searchwall].overpicnum;
                    for (i=0; i<numwalls; i++)
                        if (wall[i].overpicnum == j)
                            wall[i].overpicnum = temppicnum;
                    break;
                }
                message("Picnums replaced");
                asksave = 1;
            }
        }
        else	//C
        {
            if (AIMING_AT_SPRITE)
            {
                sprite[searchwall].cstat ^= 128;
                message("Sprite %d center bit %s",searchwall, ONOFF((sprite[searchwall].cstat&128)));
                asksave = 1;
            }
        }
    }

    if (PRESSED_KEYSC(SLASH))  // /?     Reset panning&repeat to 0
    {
        if (AIMING_AT_WALL_OR_MASK)
        {
            int16_t w = SELECT_WALL();
            wall[w].xpanning = 0;
            wall[w].ypanning = 0;
            wall[w].xrepeat = 8;
            wall[w].yrepeat = 8;
            wall[w].cstat = 0;
            fixrepeats((int16_t)searchwall);
        }
        else if (AIMING_AT_CEILING_OR_FLOOR)
        {
            AIMED_CEILINGFLOOR(xpanning) = 0;
            AIMED_CEILINGFLOOR(ypanning) = 0;
            AIMED_CEILINGFLOOR(stat) &= ~2;
            AIMED_CEILINGFLOOR(heinum) = 0;
        }
        else if (AIMING_AT_SPRITE)
        {
            if (eitherSHIFT)
                sprite[searchwall].xrepeat = sprite[searchwall].yrepeat;
            else
            {
                sprite[searchwall].xrepeat = 64;
                sprite[searchwall].yrepeat = 64;
            }
        }

        if (ASSERT_AIMING)
        {
            message("%s's size and panning reset", Typestr[searchstat]);
            asksave = 1;
        }
    }

    if (PRESSED_KEYSC(P))  // P (parallaxing sky)
    {
        if (eitherCTRL)
        {
            parallaxtype = (parallaxtype+1)%3;
            sector[searchsector].ceilingstat ^= 1;
            message("Parallax type %d", parallaxtype);
        }
        else if (eitherALT)
        {
            if (ASSERT_AIMING)
            {
                Bsprintf(tempbuf, "%s pal: ", Typestr[searchstat]);
                getnumberptr256(tempbuf, &AIMED_CF_SEL(pal) , sizeof(uint8_t), 255, 0, NULL);
                asksave = 1;
            }
        }
        else
        {
            if (AIMING_AT_WALL_OR_MASK || AIMING_AT_CEILING)
            {
                sector[searchsector].ceilingstat ^= 1;
                message("Sector %d ceiling parallax bit %s",searchsector, ONOFF(sector[searchsector].ceilingstat&1));
                asksave = 1;
            }
            else if (AIMING_AT_FLOOR)
            {
                sector[searchsector].floorstat ^= 1;
                message("Sector %d floor parallax bit %s",searchsector, ONOFF(sector[searchsector].floorstat&1));
                asksave = 1;
            }
        }
    }

    if (PRESSED_KEYSC(D))   //Alt-D  (adjust sprite[].clipdist)
    {
        if (eitherALT && AIMING_AT_SPRITE)
            sprite[searchwall].clipdist = getnumber256("Sprite clipdist: ", sprite[searchwall].clipdist, 256L, 0);
    }

    VM_OnEvent(EVENT_KEYS3D, -1);
}// end 3d

static void DoSpriteSearch(int32_t dir)  // <0: backwards, >=0: forwards
{
    char did_wrap = 0;
    int32_t i, j, k = 0;

    dir = 1-2*(dir<0);

    for (gs_cursprite += dir;; gs_cursprite += dir)
    {
        if (gs_cursprite < 0 || gs_cursprite >= MAXSPRITES)
        {
            if (did_wrap)
                break;

            did_wrap = 1;
            gs_cursprite &= (MAXSPRITES-1);
        }

        if (sprite[gs_cursprite].statnum == MAXSTATUS) continue;

        for (i=0; i<3; i++)
            for (j=0; i==1 ? j<6 : j<7; j++)
            {
                if (!gs_spritewhat[i][j]) continue;

                if (i==0)
                {
                    switch (j)
                    {
                    case 0: k = sprite[gs_cursprite].x; break;
                    case 1: k = sprite[gs_cursprite].y; break;
                    case 2: k = sprite[gs_cursprite].z; break;
                    case 3: k = sprite[gs_cursprite].sectnum; break;
                    case 4: k = sprite[gs_cursprite].statnum; break;
                    case 5: k = sprite[gs_cursprite].hitag; break;
                    case 6: k = sprite[gs_cursprite].lotag; break;
                    }
                }
                if (i==1)
                {
                    switch (j)
                    {
                    case 0:
                        k = sprite[gs_cursprite].cstat;
                        k &= gs_sprite[1][0];
                        break;
                    case 1: k = sprite[gs_cursprite].shade; break;
                    case 2: k = sprite[gs_cursprite].pal; break;
                    case 3:
                        k = gs_sprite[1][3];
                        if (k != sprite[gs_cursprite].xrepeat &&
                                k != sprite[gs_cursprite].yrepeat)
                            goto NEXTSPRITE;
                        break;
                    case 4:
                        k = gs_sprite[1][4];
                        if (k != sprite[gs_cursprite].xoffset &&
                                k != sprite[gs_cursprite].yoffset)
                            goto NEXTSPRITE;
                        break;
                    case 5: k = sprite[gs_cursprite].picnum; break;
                    }
                }
                if (i==2)
                {
                    switch (j)
                    {
                    case 0: k = sprite[gs_cursprite].ang; break;
                    case 1: k = sprite[gs_cursprite].xvel; break;
                    case 2: k = sprite[gs_cursprite].yvel; break;
                    case 3: k = sprite[gs_cursprite].zvel; break;
                    case 4: k = sprite[gs_cursprite].owner; break;
                    case 5: k = sprite[gs_cursprite].clipdist; break;
                    case 6: k = sprite[gs_cursprite].extra; break;
                    }
                }

                if (k != gs_sprite[i][j]) goto NEXTSPRITE;
            }

        // found matching sprite
        pos.x = sprite[gs_cursprite].x;
        pos.y = sprite[gs_cursprite].y;
        ang = sprite[gs_cursprite].ang;

        printmessage16("%s Sprite seach%s: found sprite %d", dir<0 ? "<" : ">",
                       did_wrap ? " (wrap)" : "", gs_cursprite);
        did_wrap = 0;
        return;

NEXTSPRITE:
        ;
    }
    printmessage16("%s Sprite search: none found", dir<0 ? "<" : ">");
}

extern int32_t graphicsmode;

static void Keys2d(void)
{
    int32_t i=0, j, k;
    int32_t smooshy, changedir;
    static int32_t repeatcnt[2] = {0,0};  // was repeatcountx, repeatcounty

//    for(i=0;i<0x50;i++) if(keystatus[i]==1) Bsprintf(tempbuf,"key %d",i); printmessage16(tempbuf);

    cursectornum = -1;
    for (i=0; i<numsectors; i++)
    {
        if (inside(mousxplc,mousyplc,i) == 1)
        {
            cursectornum = i;
            break;
        }
    }
    searchsector = cursectornum;

    if (eitherCTRL && PRESSED_KEYSC(Z)) // CTRL+Z
    {
        if (eitherSHIFT)
        {
            if (map_undoredo(1)) message("Nothing to redo!");
            else message("Restored revision %d",map_revision-1);
        }
        else
        {
            if (map_undoredo(0)) message("Nothing to undo!");
            else message("Revision %d undone",map_revision);
        }
    }

    if (keystatus[KEYSC_TAB])  //TAB
    {
        if (eitherSHIFT || eitherCTRL)
        {
            if (pointhighlight >= 16384)
            {
                drawgradient();
                showspritedata(pointhighlight&16383, 0);
            }
            else if (linehighlight >= 0 && ((bstatus&1) || sectorofwall(linehighlight)==cursectornum))
            {
                drawgradient();
                showwalldata(linehighlight, 0);
            }
        }
        else if (cursectornum >= 0)
        {
            drawgradient();
            showsectordata(cursectornum, 0);
        }
    }
    else if (!(keystatus[KEYSC_F5]|keystatus[KEYSC_F6]|keystatus[KEYSC_F7]|keystatus[KEYSC_F8]) && !eitherSHIFT)
    {
        static int32_t counter = 0;
        static int32_t omx = 0, omy = 0;
        /*
                static int32_t opointhighlight, olinehighlight, ocursectornum;

                if (pointhighlight == opointhighlight && linehighlight == olinehighlight && cursectornum == ocursectornum)
        */
        if (omx == mousxplc && omy == mousyplc)
        {
            if (counter < 6)
                counter++;
        }
        else if (counter > 0)
            counter--;

        omx = mousxplc;
        omy = mousyplc;

        /*
                opointhighlight = pointhighlight;
                olinehighlight = linehighlight;
                ocursectornum = cursectornum;
        */


        if (totalclock < lastpm16time + 120*2)
            _printmessage16(lastpm16buf);
        else if (counter >= 2 && totalclock >= 120*6)
        {
            if (pointhighlight >= 16384)
            {
                i = pointhighlight-16384;
                showspritedata(i, 1);

                if (sprite[i].picnum==SECTOREFFECTOR)
                    _printmessage16("^10%s", SectorEffectorText(i));
            }
            else if (linehighlight >= 0 && ((bstatus&1) || sectorofwall(linehighlight)==cursectornum))
                showwalldata(linehighlight, 1);
            else if (cursectornum >= 0)
                showsectordata(cursectornum, 1);
        }
    }

///__bigcomment__

    if (keystatus[KEYSC_QUOTE] && PRESSED_KEYSC(Z)) // ' z
    {
        editorzrange[0] = getnumber16("Upper Z range: ", editorzrange[0], INT32_MAX, 1);
        editorzrange[1] = getnumber16("Lower Z range: ", editorzrange[1], INT32_MAX, 1);
        // printmessage16("");
    }

    if (PRESSED_KEYSC(T))  // T (tag)
    {
        char buffer[80];

        if (eitherCTRL)  //Ctrl-T
        {
            extern int32_t showtags;

            showtags ^= 1;
            printmessage16("Show tags %s", ONOFF(showtags));
        }
        else if (eitherALT)  //ALT
        {
            if (pointhighlight >= 16384)
            {
                i = pointhighlight-16384;
                Bsprintf(buffer,"Sprite (%d) Lo-tag: ", i);
                sprite[i].lotag = _getnumber16(buffer, sprite[i].lotag, BTAG_MAX, 0, sprite[i].picnum==SECTOREFFECTOR ?
                                               (void *)SectorEffectorTagText : NULL);
            }
            else if (linehighlight >= 0)
            {
                i = linehighlight;
                Bsprintf(buffer,"Wall (%d) Lo-tag: ", i);
                wall[i].lotag = getnumber16(buffer, wall[i].lotag, BTAG_MAX, 0);
            }
        }
        else
        {
            for (i=0; i<numsectors; i++)
                if (inside(mousxplc,mousyplc,i) == 1)
                {
                    Bsprintf(buffer,"Sector (%d) Lo-tag: ",i);
                    j = qsetmode;
                    qsetmode = 200;
                    sector[i].lotag = _getnumber16(buffer, sector[i].lotag, BTAG_MAX, 0, (void *)ExtGetSectorType);
                    qsetmode = j;
//                    break;
                }
        }
    }

    if (PRESSED_KEYSC(F1) || (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_TILDE])) //F1 or ' ~
    {
        // PK_
        if (numhelppages>0)
            IntegratedHelp();
        else
            printmessage16("m32help.hlp invalid or not found!");
    }

    if (PRESSED_KEYSC(F2))
        if (g_numsounds > 0)
            SoundDisplay();

    getpoint(searchx,searchy, &mousxplc,&mousyplc);
    ppointhighlight = getpointhighlight(mousxplc,mousyplc, ppointhighlight);

    if ((ppointhighlight&0xc000) == 16384)
    {
        //   sprite[ppointhighlight&16383].cstat ^= 1;
        cursprite = ppointhighlight&16383;
    }

    if (keystatus[KEYSC_F9]) // F9 f1=3b
        Show2dText("sthelp.hlp");

    /* start Mapster32 */

    if (PRESSED_KEYSC(F4))
    {
        showfirstwall = !showfirstwall;
        message("Sector firstwall highlight %s", showfirstwall?"enabled":"disabled");
    }

    if (PRESSED_KEYSC(M))  // M (tag)
    {
        if (eitherALT)  //ALT
        {
            if (pointhighlight >= 16384)
            {
                i = pointhighlight-16384;
                Bsprintf(tempbuf, "Sprite %d Extra: ", i);
                sprite[i].extra = getnumber16(tempbuf, sprite[i].extra, BTAG_MAX, 1);
            }
            else if (linehighlight >= 0)
            {
                i = linehighlight;
                Bsprintf(tempbuf,"Wall %d Extra: ",i);
                wall[i].extra = getnumber16(tempbuf,wall[i].extra,BTAG_MAX,1);
            }
        }
        else
        {
            for (i=0; i<numsectors; i++)
                if (inside(mousxplc,mousyplc,i) == 1)
                {
                    Bsprintf(tempbuf,"Sector %d Extra: ",i);
                    sector[i].extra = getnumber16(tempbuf,sector[i].extra,BTAG_MAX,1);
//                    clearmidstatbar16();
//                    showsectordata(i, 0);
//                    break;
                }
        }
    }

    if (PRESSED_KEYSC(E))  // E (expand)
    {
        for (i=0; i<numsectors; i++)
            if (inside(mousxplc,mousyplc,i) == 1)
            {
                sector[i].floorstat ^= 8;
                message("Sector %d floor texture expansion bit %s", i, ONOFF(sector[i].floorstat&8));
                asksave = 1;
            }
    }

    if (PRESSED_KEYSC(SLASH))  // /     Reset panning&repeat to 0
    {
        if ((ppointhighlight&0xc000) == 16384)
        {
            if (eitherSHIFT)
                sprite[cursprite].xrepeat = sprite[cursprite].yrepeat;
            else
                sprite[cursprite].xrepeat = sprite[cursprite].yrepeat = 64;
            asksave = 1;
        }
        else if (graphicsmode != 0)
        {
            for (i=0; i<numsectors; i++)
                if (inside(mousxplc,mousyplc,i) == 1)
                {
                    sector[i].floorxpanning = sector[i].floorypanning = 0;
                    message("Sector %d floor panning reset", i);
                }
            asksave = 1;
        }
    }

    for (k=0; k<2; k++)    // panning/repeat
    {
        if (k==0)
            j = (keystatus[KEYSC_gLEFT]<<1)|keystatus[KEYSC_gRIGHT];  // 4 & 6 (keypad 2D)
        else
            j = (keystatus[KEYSC_gUP]<<1)|keystatus[KEYSC_gDOWN];  // 2 & 8 (keypad 2D)

        if (j)
        {
            smooshy = keystatus[KEYSC_gKP5];

            if (repeatcnt[k] == 0 || repeatcnt[k] > 32)
            {
                changedir = 1-(j&2);

                if ((ppointhighlight&0xc000) == 16384 && (sprite[cursprite].cstat & 48))
                {
                    uint8_t *repeat = (k==0) ? &sprite[cursprite].xrepeat : &sprite[cursprite].yrepeat;
                    *repeat = max(4, changechar(*repeat, changedir, smooshy, 1));
                    message("Sprite %d repeat: %d, %d", cursprite, sprite[cursprite].xrepeat, sprite[cursprite].yrepeat);
                }
                else
                {
                    for (i=0; i<numsectors; i++)
                        if (inside(mousxplc,mousyplc,i) == 1)
                        {
                            uint8_t *panning = (k==0) ? &sector[i].floorxpanning : &sector[i].floorypanning;
                            *panning = changechar(*panning, changedir, smooshy, 0);
                            message("Sector %d floor panning: %d, %d", searchsector, sector[i].floorxpanning, sector[i].floorypanning);
                        }
                }

                asksave = 1;
                repeatcnt[k] = max(1,repeatcnt[k]-2);
            }
            repeatcnt[k] += synctics;
        }
        else
            repeatcnt[k] = 0;
    }

    if (PRESSED_KEYSC(R))  // R (relative alignment, rotation)
    {
        if (pointhighlight >= 16384)
        {
            const char *aligntype[4] = { "view", "wall", "floor", "???" };

            i = sprite[cursprite].cstat;
            if ((i&48) < 32)
                i += 16;
            else
                i &= ~48;
            sprite[cursprite].cstat = i;

            message("Sprite %d now %s aligned", cursprite, aligntype[(i&48)/16]);
            asksave = 1;
        }
    }

    if (keystatus[KEYSC_QUOTE] && keystatus[KEYSC_S]) // ' S
    {
        if (pointhighlight >= 16384)
        {
            keystatus[KEYSC_S] = 0;
            Bsprintf(tempbuf, "Sprite %d xrepeat: ", cursprite);
            sprite[cursprite].xrepeat = getnumber16(tempbuf, sprite[cursprite].xrepeat, 255, 0);
            Bsprintf(tempbuf, "Sprite %d yrepeat: ", cursprite);
            sprite[cursprite].yrepeat = getnumber16(tempbuf, sprite[cursprite].yrepeat, 255, 0);
            Bsprintf(tempbuf, "Sprite %d updated", i);
            printmessage16(tempbuf);
        }
    }

    if (keystatus[KEYSC_QUOTE] && PRESSED_KEYSC(F)) // ' F
    {
        FuncMenu();
    }

    tsign = 0;
    if (PRESSED_KEYSC(LBRACK)) // [     search backward
        tsign = -1;
    if (PRESSED_KEYSC(RBRACK)) // ]     search forward
        tsign = +1;

    if (tsign)
    {
        if (wallsprite==0)
            SearchSectors(tsign);
        else if (wallsprite==1)
        {
            if ((tsign<0 && curwallnum>0) || (tsign>0 && curwallnum<numwalls))
                curwallnum += tsign;

            for (i=curwallnum; i>=0 && i<numwalls; i+=tsign)
            {
                if ((wall[i].picnum==wall[curwall].picnum)
                        && (search_lotag==0 || search_lotag==wall[i].lotag)
                        && (search_hitag==0 || search_hitag==wall[i].hitag))
                {
                    pos.x = wall[i].x - (wall[i].x-POINT2(i).x)/2;
                    pos.y = wall[i].y - (wall[i].y-POINT2(i).y)/2;
                    printmessage16("%s Wall search: found", tsign<0?"<":">");
                    return;
                }
                curwallnum--;
            }
            printmessage16("%s Wall search: none found", tsign<0?"<":">");
        }
        else if (wallsprite==2)
            DoSpriteSearch(tsign);

///__old_sprite_search_1__

///__old_sprite_search_2__

    }

    if (PRESSED_KEYSC(G))  // G (grid on/off)
    {
        if (autogrid)
        {
            grid = 0;
            if (eitherSHIFT)
                grid = 8;

            autogrid = 0;
        }
        else
        {
            grid += (1-2*eitherSHIFT);
            if (grid == -1 || grid == 9)
            {
                autogrid = 1;
                grid = 0;
            }
        }
        if (autogrid)
            Bsprintf(tempbuf,"Grid size: 9 (autosize)");
        else if (!grid)
            Bsprintf(tempbuf,"Grid off");
        else
            Bsprintf(tempbuf,"Grid size: %d (%d units)", grid, 2048>>grid);

        printmessage16(tempbuf);
    }
    if (autogrid)
    {
        grid = min(zoom+512, 65536);
        grid = scale(grid, 6, 6144);
        grid = clamp(grid, 0, 7);
    }


    if (keystatus[KEYSC_QUOTE] && PRESSED_KEYSC(L)) // ' L  (set sprite/wall coordinates)
    {
        if (pointhighlight >= 16384)
        {
            i = pointhighlight - 16384;
            Bsprintf(tempbuf, "Sprite %d x: ", i);
            sprite[i].x = getnumber16(tempbuf, sprite[i].x, editorgridextent-1, 1);
            Bsprintf(tempbuf, "Sprite %d y: ", i);
            sprite[i].y = getnumber16(tempbuf, sprite[i].y, editorgridextent-1, 1);
            Bsprintf(tempbuf, "Sprite %d z: ", i);
            sprite[i].z = getnumber16(tempbuf, sprite[i].z, 8388608, 1);
            Bsprintf(tempbuf, "Sprite %d angle: ", i);
            sprite[i].ang = getnumber16(tempbuf, sprite[i].ang, 2047, 0);
            Bsprintf(tempbuf, "Sprite %d updated", i);
            printmessage16(tempbuf);
        }

        else if (pointhighlight >= 0 /*<= 16383*/)
        {
            i = linehighlight;
            j = wall[i].x;
            k = wall[i].y;

            Bsprintf(tempbuf, "Wall %d x: ", i);
            j = getnumber16(tempbuf, j, editorgridextent, 1);
            Bsprintf(tempbuf, "Wall %d y: ", i);
            k = getnumber16(tempbuf, k, editorgridextent, 1);
            dragpoint(i, j, k);
            Bsprintf(tempbuf, "Wall %d updated", i);
            printmessage16(tempbuf);
        }
    }


    if (keystatus[KEYSC_QUOTE] && PRESSED_KEYSC(3)) // ' 3
    {
        onnames = (onnames+1)%8;
        Bsprintf(tempbuf, "Mode %d %s", onnames, SpriteMode[onnames]);
        printmessage16(tempbuf);
        //   clearmidstatbar16();
        //   for(i=0;i<MAXMODE32D;i++) {printext16(0*8,ydim16+32+(i*8),15,-1,SpriteMode[i],0);
    }
    //   Ver();

///__motorcycle___

    if (keystatus[KEYSC_QUOTE] && PRESSED_KEYSC(7)) // ' 7 : swap hilo
    {

        if (pointhighlight >= 16384)
        {
            swapshort(&sprite[cursprite].lotag, &sprite[cursprite].hitag);
            Bsprintf(tempbuf, "Sprite %d tags swapped", cursprite);
            printmessage16(tempbuf);
        }
        else if (linehighlight >= 0)
        {
            swapshort(&wall[linehighlight].lotag, &wall[linehighlight].hitag);
            Bsprintf(tempbuf, "Wall %d tags swapped", linehighlight);
            printmessage16(tempbuf);
        }
    }

    if (keystatus[KEYSC_QUOTE] && PRESSED_KEYSC(J)) // ' J
    {
        pos.x = getnumber16("X-coordinate:    ", pos.x, editorgridextent, 1);
        pos.y = getnumber16("Y-coordinate:    ", pos.y, editorgridextent, 1);
        Bsprintf(tempbuf, "Current pos now (%d, %d)", pos.x, pos.y);
        printmessage16(tempbuf);
    }
}// end key2d

static void InitCustomColors(void)
{
    /* blue */
    /*    vgapal16[9*4+0] = 63;
    vgapal16[9*4+1] = 31;
    vgapal16[9*4+2] = 7; */
    int32_t i;
    palette_t *edcol;
    /*

    char vgapal16[4*256] =
    {
    00,00,00,00, 42,00,00,00, 00,42,00,00, 42,42,00,00, 00,00,42,00,
    42,00,42,00, 00,21,42,00, 42,42,42,00, 21,21,21,00, 63,21,21,00,
    21,63,21,00, 63,63,21,00, 21,21,63,00, 63,21,63,00, 21,63,63,00,
    63,63,63,00
    };
    */
    /*    editorcolors[0] = getclosestcol(0,0,0);
    editorcolors[1] = getclosestcol(0,0,42);
    editorcolors[2] = getclosestcol(0,42,0);
    editorcolors[3] = getclosestcol(0,42,42);
    editorcolors[4] = getclosestcol(42,0,0);
    editorcolors[5] = getclosestcol(0,0,0);
    */

    vgapal16[9*4+0] = 63;
    vgapal16[9*4+1] = 31;
    vgapal16[9*4+2] = 7;

    /* orange */
    vgapal16[31*4+0] = 20; // blue
    vgapal16[31*4+1] = 45; // green
    vgapal16[31*4+2] = 60; // red

    vgapal16[39*4+0] = 36;
    vgapal16[39*4+1] = 53;
    vgapal16[39*4+2] = 63;


    /* light yellow */
    vgapal16[22*4+0] = 51;
    vgapal16[22*4+1] = 63;
    vgapal16[22*4+2] = 63;

    /* grey */
    vgapal16[23*4+0] = 45;
    vgapal16[23*4+1] = 45;
    vgapal16[23*4+2] = 45;

    /* blue */
    vgapal16[24*4+0] = 51;
    vgapal16[24*4+1] = 41;
    vgapal16[24*4+2] = 12;

    vgapal16[32*4+0] = 60;
    vgapal16[32*4+1] = 50;
    vgapal16[32*4+2] = 21;

    // grid color
    vgapal16[25*4+0] = 19;
    vgapal16[25*4+1] = 17;
    vgapal16[25*4+2] = 17;

    vgapal16[26*4+0] = 24;
    vgapal16[26*4+1] = 24;
    vgapal16[26*4+2] = 24;

    vgapal16[33*4+0] = 0;//15; // blue
    vgapal16[33*4+1] = 0;//30; // green
    vgapal16[33*4+2] = 48;//45; // red

    vgapal16[41*4+0] = 0;//24;
    vgapal16[41*4+1] = 0;//40;
    vgapal16[41*4+2] = 63;//48;

    for (i = 0; i<256; i++)
    {
        edcol = (palette_t *)&vgapal16[4*i];
        editorcolors[i] = getclosestcol(edcol->b,edcol->g,edcol->r);
    }
}

void ExtPreSaveMap(void)
{
    if (fixmapbeforesaving)
    {
        int32_t i, startwall, j, endwall;

        for (i=0; i<numsectors; i++)
        {
            startwall = sector[i].wallptr;
            for (j=startwall; j<numwalls; j++)
                if (wall[j].point2 < startwall)
                    startwall = wall[j].point2;
            sector[i].wallptr = startwall;
        }

        for (i=numsectors-2; i>=0; i--)
            sector[i].wallnum = sector[i+1].wallptr-sector[i].wallptr;
        sector[numsectors-1].wallnum = numwalls - sector[numsectors-1].wallptr;

        for (i=0; i<numwalls; i++)
        {
            wall[i].nextsector = -1;
            wall[i].nextwall = -1;
        }
        for (i=0; i<numsectors; i++)
        {
            startwall = sector[i].wallptr;
            endwall = startwall + sector[i].wallnum;
            for (j=startwall; j<endwall; j++)
                checksectorpointer((int16_t)j,(int16_t)i);
        }
    }
}

static void G_ShowParameterHelp(void)
{
    char *s = "Usage: mapster32 [OPTIONS] [FILE]\n\n"
              "-gFILE, -grp FILE\tUse extra group file FILE\n"
              "-hFILE\t\tUse definitions file FILE\n"
              "-jDIR, -game_dir DIR\n\t\tAdds DIR to the file path stack\n"
              "-check\t\tEnables map pointer checking when saving\n"
              "-nocheck\t\tDisables map pointer checking when saving (default)\n"  // kept for script compat
#if defined RENDERTYPEWIN || (defined RENDERTYPESDL && !defined __APPLE__ && defined HAVE_GTK2)
              "-setup\t\tDisplays the configuration dialog\n"
#endif
#if !defined(_WIN32)
              "-usecwd\t\tRead game data and configuration file from working directory\n"
#endif
              "\n-?, -help, --help\tDisplay this help message and exit"
              ;
    wm_msgbox("Mapster32"VERSION BUILDDATE,s);
}

static void AddGamePath(const char *buffer)
{
    struct strllist *s;
    s = (struct strllist *)Bcalloc(1,sizeof(struct strllist));
    s->str = Bstrdup(buffer);

    if (CommandPaths)
    {
        struct strllist *t;
        for (t = CommandPaths; t->next; t=t->next) ;
        t->next = s;
        return;
    }
    CommandPaths = s;
}

static void G_AddGroup(const char *buffer)
{
    struct strllist *s;
    s = (struct strllist *)Bcalloc(1,sizeof(struct strllist));
    s->str = Bstrdup(buffer);
    if (Bstrchr(s->str,'.') == 0)
        Bstrcat(s->str,".grp");

    if (CommandGrps)
    {
        struct strllist *t;
        for (t = CommandGrps; t->next; t=t->next) ;
        t->next = s;
        return;
    }
    CommandGrps = s;
}

#define COPYARG(i) \
    Bmemcpy(&testplay_addparam[j], argv[i], lengths[i]); \
    j += lengths[i]; \
    testplay_addparam[j++] = ' ';

#if defined(RENDERTYPEWIN) && defined(USE_OPENGL)
extern char forcegl;
#endif

static void G_CheckCommandLine(int32_t argc, const char **argv)
{
    int32_t i = 1, j, maxlen=0, *lengths;
    char *c, *k;

    mapster32_fullpath = (char *)argv[0];

    if (argc > 1)
    {
        lengths = Bmalloc(argc*sizeof(int32_t));
        for (j=1; j<argc; j++)
            maxlen += (lengths[j] = Bstrlen(argv[j]));

        testplay_addparam = Bmalloc(maxlen+argc);
        testplay_addparam[0] = 0;

        j = 0;

        while (i < argc)
        {
            c = (char *)argv[i];
            if ((*c == '-')
#ifdef _WIN32
                || (*c == '/')
#endif
)
            {
                if (!Bstrcasecmp(c+1,"?") || !Bstrcasecmp(c+1,"help") || !Bstrcasecmp(c+1,"-help"))
                {
                    G_ShowParameterHelp();
                    exit(0);
                }

                if (!Bstrcasecmp(c+1, "g") || !Bstrcasecmp(c+1,  "grp"))
                {
                    if (argc > i+1)
                    {
                        G_AddGroup(argv[i+1]);
                        COPYARG(i);
                        COPYARG(i+1);
                        i++;
                    }
                    i++;
                    continue;
                }

                if (!Bstrcasecmp(c+1,"game_dir"))
                {
                    if (argc > i+1)
                    {
#if defined(POLYMOST) && defined(USE_OPENGL)
                        extern char TEXCACHEFILE[BMAX_PATH];

                        Bsprintf(tempbuf,"%s/%s",argv[i+1],TEXCACHEFILE);
                        Bstrcpy(TEXCACHEFILE,tempbuf);
#endif
                        AddGamePath(argv[i+1]);

                        COPYARG(i);
                        COPYARG(i+1);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"cfg"))
                {
                    if (argc > i+1)
                    {
                        Bstrcpy(setupfilename,argv[i+1]);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"gamegrp"))
                {
                    if (argc > i+1)
                    {
                        Bstrcpy(defaultduke3dgrp,argv[i+1]);
                        COPYARG(i);
                        COPYARG(i+1);
                        i++;
                    }
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"nam"))
                {
                    Bstrcpy(g_grpNamePtr, "nam.grp");
                    COPYARG(i);
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"ww2gi"))
                {
                    Bstrcpy(g_grpNamePtr, "ww2gi.grp");
                    COPYARG(i);
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"check"))
                {
                    initprintf("Map pointer checking on saving enabled\n");
                    fixmapbeforesaving = 1;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"nocheck"))
                {
                    initprintf("Map pointer checking disabled\n");
                    fixmapbeforesaving = 0;
                    i++;
                    continue;
                }
                if (!Bstrcasecmp(c+1,"noautoload"))
                {
                    initprintf("Autoload disabled\n");
                    NoAutoLoad = 1;
                    COPYARG(i);
                    i++;
                    continue;
                }
#if !defined(_WIN32)
                if (!Bstrcasecmp(c+1,"usecwd"))
                {
                    usecwd = 1;
                    COPYARG(i);
                    i++;
                    continue;
                }
#endif
#if defined(RENDERTYPEWIN) && defined(USE_OPENGL)
                if (!Bstrcasecmp(c+1,"forcegl"))
                {
                    forcegl = 1;
                    i++;
                    continue;
                }
#endif
            }

            if ((*c == '-')
#ifdef _WIN32
                || (*c == '/')
#endif
)
            {
                c++;
                switch (*c)
                {
                case 'h':
                case 'H':
                    c++;
                    if (*c)
                    {
                        defsfilename = c;
                        COPYARG(i);
                        initprintf("Using DEF file: %s.\n",defsfilename);
                    }
                    break;
                case 'j':
                case 'J':
                    c++;
                    if (!*c) break;
                    AddGamePath(c);
                    COPYARG(i);
                    break;
                case 'g':
                case 'G':
                    c++;
                    if (!*c) break;
                    G_AddGroup(c);
                    COPYARG(i);
                    break;
                case 'x':
                case 'X':
                    c++;
                    if (!*c) break;
                    gamecon = c;
                    COPYARG(i);
                    break;
                }
            }
            else
            {
                k = Bstrrchr(c,'.');
                if (k)
                {
                    if (!Bstrcasecmp(k,".grp") || !Bstrcasecmp(k,".zip"))
                    {
                        COPYARG(i);
                        G_AddGroup(argv[i++]);
                        continue;
                    }
                    else if (!Bstrcasecmp(k,".def"))
                    {
                        COPYARG(i);
                        defsfilename = (char *)argv[i++];
                        initprintf("Using DEF file: %s.\n",defsfilename);
                        continue;
                    }
                    else if (!Bstrcasecmp(k,".con"))
                    {
                        COPYARG(i);
                        gamecon = (char *)argv[i++];
                        continue;
                    }
                }
            }
            i++;
        }

        Bfree(lengths);

        if (j > 0)
        {
            testplay_addparam[j-1] = 0;
            testplay_addparam = Brealloc(testplay_addparam, j*sizeof(char));
        }
        else
        {
            Bfree(testplay_addparam);
            testplay_addparam = NULL;
        }
    }
}
#undef COPYARG

int32_t ExtPreInit(int32_t argc,const char **argv)
{
    wm_setapptitle("Mapster32");

#ifdef _WIN32
    tempbuf[GetModuleFileName(NULL,tempbuf,BMAX_PATH)] = 0;
    Bcorrectfilename(tempbuf,1);
    //chdir(tempbuf);
#endif

    OSD_SetLogFile("mapster32.log");
    OSD_SetVersion("Mapster32"VERSION,0,2);
    initprintf("Mapster32"VERSION BUILDDATE"\n");
    //    initprintf("Copyright (c) 2008 EDuke32 team\n");

    G_CheckCommandLine(argc,argv);

    return 0;
}

static int32_t osdcmd_quit(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
    clearfilenames();
    ExtUnInit();
    uninitengine();

    exit(0);
}

static int32_t osdcmd_editorgridextent(const osdfuncparm_t *parm)
{
    int32_t i;
    extern int32_t editorgridextent;

    if (parm->numparms == 0)
    {
        OSD_Printf("\"editorgridextent\" is \"%d\"\n", editorgridextent);
        return OSDCMD_SHOWHELP;
    }
    else if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;

    i = Batol(parm->parms[0]);

    if (i >= 65536 && i <= 524288)
    {
        editorgridextent = i;
        OSD_Printf("editorgridextent %d\n", editorgridextent);
    }
    else
        OSD_Printf("editorgridextent: value out of range (65536 to 524288)\n");

    return OSDCMD_OK;
}

static int32_t osdcmd_addpath(const osdfuncparm_t *parm)
{
    char pathname[BMAX_PATH];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    Bstrcpy(pathname,parm->parms[0]);
    addsearchpath(pathname);
    return OSDCMD_OK;
}

static int32_t osdcmd_initgroupfile(const osdfuncparm_t *parm)
{
    char file[BMAX_PATH];

    if (parm->numparms != 1) return OSDCMD_SHOWHELP;

    Bstrcpy(file,parm->parms[0]);
    initgroupfile(file);
    return OSDCMD_OK;
}

static int32_t osdcmd_sensitivity(const osdfuncparm_t *parm)
{
    if (parm->numparms != 1)
    {
        OSD_Printf("\"sensitivity\" is \"%.2f\"\n",msens);
        return OSDCMD_SHOWHELP;
    }
    msens = atof(parm->parms[0]);
    OSD_Printf("sensitivity %.2f\n",msens);
    return OSDCMD_OK;
}

static int32_t osdcmd_noclip(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
    noclip = !noclip;
    OSD_Printf("Clipping %s\n", noclip?"disabled":"enabled");

    return OSDCMD_OK;
}

static int32_t osdcmd_testplay_addparam(const osdfuncparm_t *parm)
{
    int32_t slen;

    if (parm->numparms != 1)
    {
        OSD_Printf("additional parameters for test playing: %s%s%s\n",
                   testplay_addparam ? "\"" : "",
                   testplay_addparam ? testplay_addparam : "<empty>",
                   testplay_addparam ? "\"" : "");
        return OSDCMD_OK;
    }

    slen = Bstrlen(parm->parms[0]);

    if (slen > 0)
    {
        if (!testplay_addparam)
            testplay_addparam = Bmalloc(slen+1);
        else
            testplay_addparam = Brealloc(testplay_addparam, slen+1);

        Bmemcpy(testplay_addparam, parm->parms[0], slen);
        testplay_addparam[slen] = 0;
    }
    else
    {
        if (testplay_addparam)
        {
            Bfree(testplay_addparam);
            testplay_addparam = NULL;
        }
    }

    return OSDCMD_OK;
}


//PK vvv ------------
static int32_t osdcmd_vars_pk(const osdfuncparm_t *parm)
{
    int32_t showval = (parm->numparms < 1);

    // this is something of a misnomer, since it's actually accel+decel
    if (!Bstrcasecmp(parm->name, "pk_turnaccel"))
    {
        if (showval)
            OSD_Printf("Turning acceleration+declaration is %d\n", pk_turnaccel);
        else
        {
            pk_turnaccel = atoi(parm->parms[0]);
            pk_turnaccel = pk_turnaccel<=pk_turndecel ? (pk_turndecel+1):pk_turnaccel;
            pk_turnaccel = pk_turnaccel>256 ? 256:pk_turnaccel;
        }
    }
    else if (!Bstrcasecmp(parm->name, "pk_turndecel"))
    {
        if (showval)
            OSD_Printf("Turning deceleration is %d\n", pk_turndecel);
        else
        {
            pk_turndecel = atoi(parm->parms[0]);
            pk_turndecel = pk_turndecel<=0 ? 1:pk_turndecel;
            pk_turndecel = pk_turndecel>=pk_turnaccel ? (pk_turnaccel-1):pk_turndecel;
            pk_turndecel = pk_turndecel>128 ? 128:pk_turndecel;
        }
    }
    else if (!Bstrcasecmp(parm->name, "pk_quickmapcycling"))
    {
        OSD_Printf("Quick map cycling ((LShift-)Ctrl-X): %s\n",
                   (quickmapcycling = !quickmapcycling) ? "enabled":"disabled");
    }
    else if (!Bstrcasecmp(parm->name, "pk_uedaccel"))
    {
        if (parm->numparms==1)
        {
            pk_uedaccel = atoi(parm->parms[0]);
            pk_uedaccel = pk_uedaccel<0 ? 0:pk_uedaccel;
            pk_uedaccel = pk_uedaccel>5 ? 5:pk_uedaccel;
        }

        if (parm->numparms <= 1)
            OSD_Printf("UnrealEd mouse navigation acceleration is %d\n", pk_uedaccel);
        else
            return OSDCMD_SHOWHELP;
    }
    else if (!Bstrcasecmp(parm->name, "osd_tryscript"))
    {
        m32_osd_tryscript = !m32_osd_tryscript;
        OSD_Printf("Try M32 script execution on invalid OSD command: %s\n", m32_osd_tryscript?"on":"off");
    }
    else if (!Bstrcasecmp(parm->name, "script_expertmode"))
    {
        m32_script_expertmode = !m32_script_expertmode;
        if (m32_script_expertmode)
            OSD_Printf("M32 Script expert mode ENABLED.  Be sure to know what you are doing!\n");
        else
            OSD_Printf("M32 Script expert mode DISABLED.\n");
    }
    else if (!Bstrcasecmp(parm->name, "show_heightindicators"))
    {
        static const char *how[3] = {"none", "two-sided walls only", "all"};

        if (parm->numparms == 1)
            showheightindicators = clamp(atoi(parm->parms[0]), 0, 2);

        if (parm->numparms <= 1)
            OSD_Printf("height indicators: %s\n", how[showheightindicators]);
        else
            return OSDCMD_SHOWHELP;
    }
    else if (!Bstrcasecmp(parm->name, "show_ambiencesounds"))
    {
        static const char *how[3] = {"none", "current sector only", "all"};

        if (parm->numparms == 1)
            showambiencesounds = clamp(atoi(parm->parms[0]), 0, 2);

        if (parm->numparms <= 1)
            OSD_Printf("ambience sound circles: %s\n", how[showambiencesounds]);
        else
            return OSDCMD_SHOWHELP;
    }
    return OSDCMD_OK;
}

#ifdef POLYMOST
static int32_t osdcmd_tint(const osdfuncparm_t *parm)
{
    int32_t i;
    palette_t *p;

    if (parm->numparms==1)
    {
        i = atoi(parm->parms[0]);
        if (i>=0 && i<MAXPALOOKUPS-RESERVEDPALS)
        {
            p = &hictinting[i];
            OSD_Printf("pal %d: r=%d g=%d b=%d f=%d\n", i, p->r, p->g, p->b, p->f);
        }
    }
    else if (parm->numparms==0)
    {
        OSD_Printf("Hightile tintings:\n");
        for (i=0,p=&hictinting[0]; i<MAXPALOOKUPS-RESERVEDPALS; i++,p++)
            if (*(int32_t *)&hictinting[i] != B_LITTLE32(0x00ffffff))
                OSD_Printf("pal %d: rgb %3d %3d %3d  f %d\n", i, p->r, p->g, p->b, p->f);
    }
    else if (parm->numparms>=2)
    {
        i = atoi(parm->parms[0]);
        if (i<0 || i>=MAXPALOOKUPS-RESERVEDPALS)
            return OSDCMD_SHOWHELP;

        p = &hictinting[i];
        p->r = atoi(parm->parms[1]);
        p->g = (parm->numparms>=3) ? atoi(parm->parms[2]) : 255;
        p->b = (parm->numparms>=4) ? atoi(parm->parms[3]) : 255;
        p->f = (parm->numparms>=5) ? atoi(parm->parms[4])&HICEFFECTMASK : 0;
    }
    return OSDCMD_OK;
}
#endif

// M32 script vvv
static int32_t osdcmd_include(const osdfuncparm_t *parm)
{
    if (parm->numparms != 1)
        return OSDCMD_SHOWHELP;
    C_Compile(parm->parms[0], 1);
    return OSDCMD_OK;
}

static int32_t osdcmd_scriptinfo(const osdfuncparm_t *parm)
{
    UNREFERENCED_PARAMETER(parm);
    C_CompilationInfo();
    return OSDCMD_OK;
}

#ifdef DEBUGGINGAIDS
extern void X_Disasm(ofstype beg, int32_t size);

static int32_t osdcmd_disasm(const osdfuncparm_t *parm)
{
    int32_t i;

    if (parm->numparms != 2)
        return OSDCMD_SHOWHELP;

    if (!isdigit(parm->parms[1][0]))
        return OSDCMD_SHOWHELP;

    i=atoi(parm->parms[1]);

    if (parm->parms[0][0]=='s')
    {
        if (i>=0 && i<g_stateCount)
            X_Disasm(statesinfo[i].ofs, statesinfo[i].codesize);
    }
    else
    {
        if (i>=0 && i<MAXEVENTS && aEventOffsets[i]>=0)
            X_Disasm(aEventOffsets[i], aEventSizes[i]);
    }
    return OSDCMD_OK;
}
#endif

static int32_t osdcmd_do(const osdfuncparm_t *parm)
{
    intptr_t oscrofs;
    char *tp;
    int32_t i, j, slen, ofs;
    int32_t onumconstants=g_numSavedConstants;

    if (parm->numparms==0)
        return OSDCMD_SHOWHELP;

    oscrofs = (g_scriptPtr-script);

    ofs = 2*(parm->numparms>0);  // true if "do" command
    slen = Bstrlen(parm->raw+ofs);
    tp = Bmalloc(slen+2);
    if (!tp) goto OUTOFMEM;
    Bmemcpy(tp, parm->raw+ofs, slen);

    // needed so that subsequent commands won't execute old stuff.
    tp[slen] = '\n';
    tp[slen+1] = '\0';

    g_didDefineSomething = 0;

    C_Compile(tp, 0);

    if (parm->numparms>=0)
        Bfree(tp);

    if (g_numCompilerErrors)
    {
//        g_scriptPtr = script + oscrofs;  // handled in C_Compile()
        return OSDCMD_OK;
    }

    for (i=0,j=0; i<MAXEVENTS; i++)
        if (aEventOffsets[i]>=0)
            j++;

    if (g_didDefineSomething == 0)
    {
        g_numSavedConstants = onumconstants;

        *g_scriptPtr = CON_RETURN + (g_lineNumber<<12);
        g_scriptPtr = script + oscrofs;

        insptr = script + oscrofs;
        Bmemcpy(&vm, &vm_default, sizeof(vmstate_t));

        if (qsetmode==200 && AIMING_AT_SPRITE)
        {
            vm.g_i = searchwall;
            vm.g_sp = &sprite[vm.g_i];
        }

        VM_Execute(0);

        if (vm.updatehighlight)
        {
            update_highlight();
            vm.updatehighlight = 0;
        }

        if (!(vm.flags&VMFLAG_ERROR))
        {
            int32_t idx, dosave=1;

            for (i=1; i<=4; i++)
            {
                idx = (scripthistend-i)&(SCRIPTHISTSIZ-1);
                if (!scripthist[idx])
                    break;
                else if (!Bstrcmp(scripthist[idx], parm->raw))
                {
                    dosave = 0;
                    break;
                }
            }

            if (dosave)
            {
                if (scripthist[scripthistend])
                    Bfree((void *)scripthist[scripthistend]);
                scripthist[scripthistend] = Bstrdup(parm->raw);
                scripthistend++;
                scripthistend %= SCRIPTHISTSIZ;
            }
        }
//        asksave = 1; // handled in Access(Sprite|Sector|Wall)
    }

    return OSDCMD_OK;
OUTOFMEM:
    OSD_Printf("OUT OF MEMORY!\n");
    return OSDCMD_OK;
}

void M32RunScript(const char *s)
{
    osdfuncparm_t parm;

    parm.numparms = -1;
    parm.raw = s;

    osdcmd_do(&parm);
}

static int32_t osdcmd_endisableevent(const osdfuncparm_t *parm)
{
    int32_t i, j, enable;

    if (!label) return OSDCMD_OK;

    if (parm->numparms < 1)
    {
        OSD_Printf("--- Defined events:\n");
        for (i=0; i<MAXEVENTS; i++)
            if (aEventOffsets[i] >= 0)
                OSD_Printf("%s (%d): %s\n", label+(i*MAXLABELLEN), i, aEventEnabled[i]?"on":"off");
        return OSDCMD_OK;
    }

    enable = !Bstrcasecmp(parm->name, "enableevent");

    if (parm->numparms == 1)
    {
        if (!Bstrcasecmp(parm->parms[0], "all") || !Bstrcasecmp(parm->parms[0], "a"))
        {
            for (i=0; i<MAXEVENTS; i++)
                aEventEnabled[i] = enable?1:0;
            OSD_Printf("%sabled all events.\n", enable?"En":"Dis");
            return OSDCMD_OK;
        }
    }

    for (i=0; i<parm->numparms; i++)
    {
        char buf[64] = "EVENT_", buf2[64];

        if (isdigit(parm->parms[i][0]))
        {
            j = atoi(parm->parms[i]);
            Bsprintf(buf2, "event %d", j);
        }
        else if (!Bstrncmp(parm->parms[i], "EVENT_", 6))
        {
            j = hash_find(&h_labels, parm->parms[i]);
            Bstrncpy(buf2, parm->parms[i], sizeof(buf2));
            buf2[sizeof(buf2)-1] = '\0';
        }
        else
        {
            Bstrncat(buf, parm->parms[i], sizeof(buf)-6-1);
            j = hash_find(&h_labels, buf);
            Bmemcpy(buf2, buf, sizeof(buf2));
        }

        if (j>=0 && j<MAXEVENTS)
        {
            aEventEnabled[j] = enable?1:0;
            OSD_Printf("%sabled %s.\n", enable?"En":"Dis", buf2);
        }
        else
            OSD_Printf("Invalid event %s.\n", buf2);
    }
    return OSDCMD_OK;
}

static int32_t registerosdcommands(void)
{
    OSD_RegisterFunction("addpath","addpath <path>: adds path to game filesystem", osdcmd_addpath);

    OSD_RegisterFunction("editorgridextent","editorgridextent: sets the size of the 2D mode editing grid",osdcmd_editorgridextent);

    OSD_RegisterFunction("initgroupfile","initgroupfile <path>: adds a grp file into the game filesystem", osdcmd_initgroupfile);

    OSD_RegisterFunction("noclip","noclip: toggles clipping mode", osdcmd_noclip);

    OSD_RegisterFunction("quit","quit: exits the editor immediately", osdcmd_quit);
    OSD_RegisterFunction("exit","exit: exits the editor immediately", osdcmd_quit);

    OSD_RegisterFunction("sensitivity","sensitivity <value>: changes the mouse sensitivity", osdcmd_sensitivity);

    //PK
    OSD_RegisterFunction("pk_turnaccel", "pk_turnaccel <value>: sets turning acceleration+deceleration", osdcmd_vars_pk);
    OSD_RegisterFunction("pk_turndecel", "pk_turndecel <value>: sets turning deceleration", osdcmd_vars_pk);
    OSD_RegisterFunction("pk_uedaccel", "pk_uedaccel <value>: sets UnrealEd movement speed factor (0-5, exponentially)", osdcmd_vars_pk);
    OSD_RegisterFunction("pk_quickmapcycling", "pk_quickmapcycling: toggles quick cycling of maps with (Shift-)Ctrl-X", osdcmd_vars_pk);
    OSD_RegisterFunction("testplay_addparam", "testplay_addparam \"string\": sets additional parameters for test playing", osdcmd_testplay_addparam);
    OSD_RegisterFunction("show_heightindicators", "show_heightindicators <0, 1 or 2>: sets display of height indicators in 2D mode", osdcmd_vars_pk);
    OSD_RegisterFunction("show_ambiencesounds", "show_ambiencesounds <0, 1 or 2>: sets display of MUSICANDSFX circles in 2D mode", osdcmd_vars_pk);
#ifdef POLYMOST
    OSD_RegisterFunction("tint", "tint <pal> <r> <g> <b> <flags>: queries or sets hightile tinting", osdcmd_tint);
#endif

    // M32 script
    OSD_RegisterFunction("include", "include <filnames...>: compiles one or more M32 script files", osdcmd_include);
    OSD_RegisterFunction("do", "do (m32 script ...): executes M32 script statements", osdcmd_do);
    OSD_RegisterFunction("script_info", "script_info: shows information about compiled M32 script", osdcmd_scriptinfo);
    OSD_RegisterFunction("script_expertmode", "script_expertmode: toggles M32 script expert mode", osdcmd_vars_pk);
    OSD_RegisterFunction("enableevent", "enableevent <all|EVENT_...|(event number)>", osdcmd_endisableevent);
    OSD_RegisterFunction("disableevent", "disableevent <all|EVENT_...|(event number)>", osdcmd_endisableevent);
    OSD_RegisterFunction("osd_tryscript", "osd_tryscript: toggles execution of M32 script on invalid OSD command", osdcmd_vars_pk);
#ifdef DEBUGGINGAIDS
    OSD_RegisterFunction("disasm", "disasm [s|e] <state or event number>", osdcmd_disasm);
#endif
    return 0;
}

#define DUKEOSD
#ifdef DUKEOSD
void GAME_drawosdchar(int32_t x, int32_t y, char ch, int32_t shade, int32_t pal)
{
    int32_t ac;

    if (ch == 32) return;
    ac = ch-'!'+STARTALPHANUM;
    if (ac < STARTALPHANUM || ac > ENDALPHANUM) return;

    rotatesprite(((x<<3)+x)<<16, (y<<3)<<16, 65536l, 0, ac, shade, pal, 8|16, 0, 0, xdim-1, ydim-1);
}

void GAME_drawosdstr(int32_t x, int32_t y, char *ch, int32_t len, int32_t shade, int32_t pal)
{
    int32_t ac;

    for (x = (x<<3)+x; len>0; len--, ch++, x++)
    {
        if (*ch == 32)
        {
            x+=5;
            continue;
        }
        ac = *ch-'!'+STARTALPHANUM;
        if (ac < STARTALPHANUM || ac > ENDALPHANUM) return;

        rotatesprite(x<<16, (y<<3)<<16, 65536l, 0, ac, shade, pal, 8|16, 0, 0, xdim-1, ydim-1);
        if (*ch >= '0' && *ch <= '9') x+=8;
        else x += tilesizx[ac];
    }
}

static int32_t GetTime(void)
{
    return totalclock;
}

void GAME_drawosdcursor(int32_t x, int32_t y, int32_t type, int32_t lastkeypress)
{
    int32_t ac;

    if (type) ac = SMALLFNTCURSOR;
    else ac = '_'-'!'+STARTALPHANUM;

    if (!((GetTime()-lastkeypress) & 0x40l))
        rotatesprite(((x<<3)+x)<<16, ((y<<3)+(type?-1:2))<<16, 65536l, 0, ac, 0, 8, 8|16, 0, 0, xdim-1, ydim-1);
}

int32_t GAME_getcolumnwidth(int32_t w)
{
    return w/9;
}

int32_t GAME_getrowheight(int32_t w)
{
    return w>>3;
}

//#define BGTILE 311
//#define BGTILE 1156
#define BGTILE 1141	// BIGHOLE
#define BORDTILE 3250	// VIEWBORDER
#define BITSTH 1+32+8+16	// high translucency
#define BITSTL 1+8+16	// low translucency
#define BITS 8+16+64		// solid
#define SHADE 16
#define PALETTE 4
void GAME_clearbackground(int32_t numcols, int32_t numrows)
{
    UNREFERENCED_PARAMETER(numcols);

#if defined(POLYMOST) && defined(USE_OPENGL)
//    if (getrendermode() < 3) bits = BITS;
//    else 
    if (rendmode>=3 && qsetmode==200)
    {
        int32_t x, y, xsiz, ysiz, tx2, ty2;
        int32_t daydim, bits;

        bits = BITSTL;

        daydim = numrows<<3;

        xsiz = tilesizx[BGTILE];
        tx2 = xdim/xsiz;
        ysiz = tilesizy[BGTILE];
        ty2 = daydim/ysiz;

        setpolymost2dview();

        for (x=0; x<=tx2; x++)
            for (y=0; y<=ty2; y++)
                rotatesprite(x*xsiz<<16,y*ysiz<<16,65536L,0,BGTILE,SHADE,PALETTE,bits,0,0,xdim,daydim);

        xsiz = tilesizy[BORDTILE];
        tx2 = xdim/xsiz;
        ysiz = tilesizx[BORDTILE];

        for (x=0; x<=tx2; x++)
            rotatesprite(x*xsiz<<16,(daydim+ysiz+1)<<16,65536L,1536,BORDTILE,SHADE-12,PALETTE,BITS,0,0,xdim,daydim+ysiz+1);

        return;
    }
#endif

    CLEARLINES2D(0, min(ydim, numrows*8+8), editorcolors[16]);
}

static void m32_osdsetfunctions()
{
    OSD_SetFunctions(
        /*  	  GAME_drawosdchar,
        GAME_drawosdstr,
        GAME_drawosdcursor,
        GAME_getcolumnwidth,
        GAME_getrowheight,*/
        0,0,0,0,0,
        GAME_clearbackground,
        (int32_t( *)(void))GetTime,
        NULL
    );
}


#endif

enum
{
    T_EOF = -2,
    T_ERROR = -1,
    T_INCLUDE = 0,
    T_DEFINE = 1,
    T_LOADGRP,
    T_TILEGROUP,
    T_TILE,
    T_TILERANGE,
    T_HOTKEY,
    T_TILES,
    T_NOAUTOLOAD,
    T_COLORS,

    T_ALPHABET,
    T_MAP,
    T_MAPA,
    T_MAPRANGE,
    T_MAPRANGEA,
    T_OFFSET,
    T_OFFSETA,

    T_DEFINESOUND,
};

typedef struct
{
    char *text;
    int32_t tokenid;
}
tokenlist;

static int32_t getatoken(scriptfile *sf, tokenlist *tl, int32_t ntokens)
{
    char *tok;
    int32_t i;

    if (!sf) return T_ERROR;
    tok = scriptfile_gettoken(sf);
    if (!tok) return T_EOF;

    for (i=0; i<ntokens; i++)
    {
        if (!Bstrcasecmp(tok, tl[i].text))
            return tl[i].tokenid;
    }
    return T_ERROR;
}

static void DoAutoload(const char *fn)
{
    static const char *extensions[3] = { "*.grp", "*.zip", "*.pk3" };
    int32_t i;

    for (i=0; i<3; i++)
    {
        Bsprintf(tempbuf, "autoload/%s", fn);
        getfilenames(tempbuf, extensions[i]);

        while (findfiles)
        {
            Bsprintf(tempbuf, "autoload/%s/%s", fn, findfiles->name);
            initprintf("Using group file '%s'.\n", tempbuf);
            initgroupfile(tempbuf);
            findfiles = findfiles->next;
        }
    }
}

int32_t parsegroupfiles(scriptfile *script)
{
    int32_t tokn;
    char *cmdtokptr;

    tokenlist grptokens[] =
    {
        { "include",         T_INCLUDE },
        { "#include",        T_INCLUDE },
        { "loadgrp",         T_LOADGRP },
        { "noautoload",      T_NOAUTOLOAD }
    };

    while (1)
    {
        tokn = getatoken(script,grptokens,sizeof(grptokens)/sizeof(tokenlist));
        cmdtokptr = script->ltextptr;
        switch (tokn)
        {
        case T_LOADGRP:
        {
            char *fn;

            pathsearchmode = 1;
            if (!scriptfile_getstring(script,&fn))
            {
                int32_t j = initgroupfile(fn);

                if (j == -1)
                    initprintf("Could not find group file '%s'.\n",fn);
                else
                {
                    initprintf("Using group file '%s'.\n",fn);
                    if (!NoAutoLoad)
                        DoAutoload(fn);
                }

            }
            pathsearchmode = 0;
        }
        break;
        case T_INCLUDE:
        {
            char *fn;
            if (!scriptfile_getstring(script,&fn))
            {
                scriptfile *included;

                included = scriptfile_fromfile(fn);
                if (!included)
                {
                    initprintf("Warning: Failed including %s on line %s:%d\n",
                               fn, script->filename,scriptfile_getlinum(script,cmdtokptr));
                }
                else
                {
                    parsegroupfiles(included);
                    scriptfile_close(included);
                }
            }
            break;
        }
        break;
        case T_NOAUTOLOAD:
            NoAutoLoad = 1;
            break;
        case T_EOF:
            return(0);
        default:
            break;
        }
    }
    return 0;
}

int32_t loadgroupfiles(char *fn)
{
    scriptfile *script;

    script = scriptfile_fromfile(fn);
    if (!script) return -1;

    parsegroupfiles(script);

    scriptfile_close(script);
    scriptfile_clearsymbols();

    return 0;
}

int32_t parsetilegroups(scriptfile *script)
{
    int32_t tokn;
    char *cmdtokptr;

    tokenlist tgtokens[] =
    {
        { "include",         T_INCLUDE          },
        { "#include",        T_INCLUDE          },
        { "define",          T_DEFINE           },
        { "#define",         T_DEFINE           },
        { "tilegroup",       T_TILEGROUP        },
        { "spritehotkey",    T_HOTKEY           },
        { "alphabet",        T_ALPHABET         },
    };

    while (1)
    {
        tokn = getatoken(script,tgtokens,sizeof(tgtokens)/sizeof(tokenlist));
        cmdtokptr = script->ltextptr;
        switch (tokn)
        {
        case T_HOTKEY:
        {
            int32_t i, j;
            if (scriptfile_getsymbol(script,&i)) break;
            if (scriptfile_getsymbol(script,&j)) break;
            if (i < 0 || i > 9 || j < 0 || j >= MAXTILES) break;
            prefixtiles[i] = j;
            break;
        }
        case T_INCLUDE:
        {
            char *fn;
            if (!scriptfile_getstring(script,&fn))
            {
                scriptfile *included;

                included = scriptfile_fromfile(fn);
                if (!included)
                {
                    initprintf("Warning: Failed including %s on line %s:%d\n",
                               fn, script->filename,scriptfile_getlinum(script,cmdtokptr));
                }
                else
                {
                    parsetilegroups(included);
                    scriptfile_close(included);
                }
            }
            break;
        }
        case T_DEFINE:
        {
            char *name;
            int32_t number;

            if (scriptfile_getstring(script,&name)) break;
            if (scriptfile_getsymbol(script,&number)) break;
            if (scriptfile_addsymbolvalue(name,number) < 0)
                initprintf("Warning: Symbol %s was NOT redefined to %d on line %s:%d\n",
                           name,number,script->filename,scriptfile_getlinum(script,cmdtokptr));
            break;
        }
        case T_TILEGROUP:
        {
            char *end, *name;
            int32_t i;

            if (tile_groups >= MAX_TILE_GROUPS) break;
            if (scriptfile_getstring(script,&name)) break;
            if (scriptfile_getbraces(script,&end)) break;

            s_TileGroups[tile_groups].pIds = Bcalloc(MAX_TILE_GROUP_ENTRIES, sizeof(int32_t));
            s_TileGroups[tile_groups].szText = Bstrdup(name);

            while (script->textptr < end)
            {
                tokenlist tgtokens2[] =
                {
                    { "tilegroup",  T_TILEGROUP   },
                    { "tile",       T_TILE        },
                    { "tilerange",  T_TILERANGE   },
                    { "hotkey",     T_HOTKEY      },
                    { "tiles",      T_TILES       },
                    { "colors",     T_COLORS      },
                };

                int32_t token = getatoken(script,tgtokens2,sizeof(tgtokens2)/sizeof(tokenlist));
                switch (token)
                {
                case T_TILE:
                {
                    if (scriptfile_getsymbol(script,&i)) break;
                    if (i >= 0 && i < MAXTILES && s_TileGroups[tile_groups].nIds < MAX_TILE_GROUP_ENTRIES)
                        s_TileGroups[tile_groups].pIds[s_TileGroups[tile_groups].nIds++] = i;
                    //                    OSD_Printf("added tile %d to group %d\n",i,g);
                    break;
                }
                case T_TILERANGE:
                {
                    int32_t j;
                    if (scriptfile_getsymbol(script,&i)) break;
                    if (scriptfile_getsymbol(script,&j)) break;
                    if (i < 0 || i >= MAXTILES || j < 0 || j >= MAXTILES) break;
                    while (s_TileGroups[tile_groups].nIds < MAX_TILE_GROUP_ENTRIES && i <= j)
                    {
                        s_TileGroups[tile_groups].pIds[s_TileGroups[tile_groups].nIds++] = i++;
                        //                        OSD_Printf("added tile %d to group %d\n",i,g);
                    }
                    break;
                }
                case T_COLORS:
                {
                    int32_t j;
                    if (scriptfile_getsymbol(script, &i)) break;
                    if (scriptfile_getsymbol(script, &j)) break;
                    if (i < 0 || i >= 256 || j < 0 || j >= 256) break;
                    s_TileGroups[tile_groups].color1 = i;
                    s_TileGroups[tile_groups].color2 = j;
                    break;
                }
                case T_HOTKEY:
                {
                    char *c;
                    if (scriptfile_getstring(script,&c)) break;
                    s_TileGroups[tile_groups].key1 = Btoupper(c[0]);
                    s_TileGroups[tile_groups].key2 = Btolower(c[0]);
                    break;
                }
                case T_TILES:
                {
                    char *end2;
                    if (scriptfile_getbraces(script,&end2)) break;
                    while (script->textptr < end2-1)
                    {
                        if (!scriptfile_getsymbol(script,&i))
                        {
                            if (i >= 0 && i < MAXTILES && s_TileGroups[tile_groups].nIds < MAX_TILE_GROUP_ENTRIES)
                                s_TileGroups[tile_groups].pIds[s_TileGroups[tile_groups].nIds++] = i;
                            //                    OSD_Printf("added tile %d to group %d\n",i,g);
                        }
                    }
                    break;
                }
                }
            }

            s_TileGroups[tile_groups].pIds = Brealloc(s_TileGroups[tile_groups].pIds,
                                                      s_TileGroups[tile_groups].nIds*sizeof(int32_t));
            tile_groups++;
            break;
        }
        case T_ALPHABET:
        {
            char *end;
            int32_t i, j, k;

            if (numalphabets >= MAX_ALPHABETS)
            {
                OSD_Printf("Too many alphabet definitions (max: %d).\n", MAX_ALPHABETS);
                break;
            }

            if (scriptfile_getbraces(script,&end)) break;

            for (i=0; i<NUMPRINTABLES; i++)
            {
                alphabets[numalphabets].pic[i] = -1;
                alphabets[numalphabets].xofs[i] = 0;
                alphabets[numalphabets].yofs[i] = 0;
            }

            while (script->textptr < end)
            {
                tokenlist alphtokens2[] =
                {
                    { "map",        T_MAP         },
                    { "mapa",       T_MAPA        },
                    { "maprange",   T_MAPRANGE    },
                    { "maprangea",  T_MAPRANGEA   },
                    { "offset",     T_OFFSET      },
                    { "offseta",    T_OFFSETA     },
                };

                int32_t token = getatoken(script,alphtokens2,sizeof(alphtokens2)/sizeof(tokenlist));
                switch (token)
                {
                case T_MAP:  // map <ascii num> <start tilenum>, e.g. map 46 3002
                {
                    if (scriptfile_getnumber(script,&i)) break;
                    if (scriptfile_getsymbol(script,&j)) break;

                    if (i>=33 && i<=126 && j>= 0 && j<MAXTILES)
                        alphabets[numalphabets].pic[i-33] = j;

                    break;
                }
                case T_MAPA:  // mapa <ascii string> <start tilenum>, e.g. map ".,!?" 3002
                {
                    char *s;
                    if (scriptfile_getstring(script,&s)) break;
                    if (scriptfile_getsymbol(script,&i)) break;

                    for (; *s; s++, i++)
                    {
                        if (*s>=33 && *s<=126 && i>= 0 && i<MAXTILES)
                            alphabets[numalphabets].pic[(*s)-33] = i;
                    }
                    break;
                }
                // maprange <start ascii num> <end ascii num> <start tilenum>, e.g. map 33 126 STARTALPHANUM
                // maprangea <start char> <end char> <start tilenum>, e.g. map "!" "~" STARTALPHANUM
                case T_MAPRANGE:
                case T_MAPRANGEA:
                {
                    if (token==T_MAPRANGE)
                    {
                        if (scriptfile_getnumber(script,&i)) break;
                        if (scriptfile_getnumber(script,&j)) break;
                    }
                    else
                    {
                        char *c1, *c2;
                        if (scriptfile_getstring(script,&c1)) break;
                        if (scriptfile_getstring(script,&c2)) break;
                        i=*c1;
                        j=*c2;
                    }
                    if (scriptfile_getsymbol(script,&k)) break;

                    if (i>126 || j<33) break;
                    for (; i<=j && k<MAXTILES; i++, k++)
                    {
                        if (i>=33 && i<=126)
                            alphabets[numalphabets].pic[i-33] = k;
                    }
                    break;
                }
                case T_OFFSET:  // offset <ascii num> <xoffset> <yoffset>
                {
                    if (scriptfile_getnumber(script, &i)) break;
                    if (scriptfile_getnumber(script, &j)) break;
                    if (scriptfile_getnumber(script, &k)) break;

                    if (i >= 33 && i <= 126)
                    {
                        alphabets[numalphabets].xofs[i-33] = j;
                        alphabets[numalphabets].yofs[i-33] = k;
                    }
                    break;
                }
                case T_OFFSETA:  // offseta <ascii string> <xoffset> <yoffset>
                {
                    char *s;
                    if (scriptfile_getstring(script, &s)) break;
                    if (scriptfile_getnumber(script, &i)) break;
                    if (scriptfile_getnumber(script, &j)) break;

                    for (; *s; s++)
                        if (*s >= 33 && *s <= 126)
                        {
                            alphabets[numalphabets].xofs[(*s)-33] = i;
                            alphabets[numalphabets].yofs[(*s)-33] = j;
                        }
                    break;
                }
                }
            }
            numalphabets++;
            break;
        }
        case T_EOF:
            return(0);
        default:
            break;
        }
    }
    return 0;
}

static int32_t loadtilegroups(const char *fn)
{
    int32_t i, j;
    scriptfile *script;
    TileGroup blank = { NULL, 0, NULL, 0, 0, 0, 0};

    script = scriptfile_fromfile(fn);
    if (!script) return -1;

    for (i=0; i<tile_groups; i++)
    {
        if (s_TileGroups[i].pIds)
            Bfree(s_TileGroups[i].pIds);
        if (s_TileGroups[i].szText)
            Bfree(s_TileGroups[i].szText);
        Bmemcpy(&s_TileGroups[i], &blank, sizeof(blank));
    }
    tile_groups = 0;
#if 0
    // ---------- Init hardcoded tile group consisting of all named tiles
    s_TileGroups[0].szText = Bstrdup("All named");
    s_TileGroups[0].pIds = Bmalloc(MAXTILES * sizeof(s_TileGroups[0].pIds[0]));
    if (!s_TileGroups[0].pIds)
        return -1;
    j = 0;
    for (i=0; i<MAXTILES; i++)
        if (names[i][0])
            s_TileGroups[0].pIds[j++] = i;
    if (j)
    {
        s_TileGroups[0].nIds = j;
        s_TileGroups[0].key1 = 'Y';
        s_TileGroups[0].key2 = 'y';
        tile_groups++;
    }
    // --------------------
#endif
    parsetilegroups(script);

    scriptfile_close(script);
    scriptfile_clearsymbols();

    tilegroupItems = getTileGroup("Items");
    tilegroupActors = getTileGroup("Actors");

    // Apply 2d sprite colors as specified in tiles.cfg.
    for (i=0; i<tile_groups; i++)
    {
        // If the colors were specified...
        if (s_TileGroups[i].color1 && s_TileGroups[i].color2)
        {
            for (j = s_TileGroups[i].nIds-1; j >= 0 ; j--)
            {
                // Apply the colors to all tiles in the group.
                spritecol2d[s_TileGroups[i].pIds[j]][0] = s_TileGroups[i].color1;
                spritecol2d[s_TileGroups[i].pIds[j]][1] = s_TileGroups[i].color2;
            }
        }
    }

    return 0;
}

/// vvv Parse CON files partially to get sound definitions
static int32_t parseconsounds(scriptfile *script)
{
    int32_t tokn;
    char *cmdtokptr;
    int32_t num_invalidsounds=0;

    tokenlist cstokens[] =
    {
        { "include",         T_INCLUDE          },
        { "#include",        T_INCLUDE          },
        { "define",          T_DEFINE           },
        { "#define",         T_DEFINE           },
        { "definesound",     T_DEFINESOUND      },
    };

    while (1)
    {
        tokn = getatoken(script,cstokens,sizeof(cstokens)/sizeof(tokenlist));
        cmdtokptr = script->ltextptr;
        switch (tokn)
        {
        case T_INCLUDE:
        {
            char *fn;
            if (!scriptfile_getstring(script,&fn))
            {
                scriptfile *included;

                included = scriptfile_fromfile(fn);
                if (!included)
                {
                    initprintf("Warning: Failed including %s on line %s:%d\n",
                               fn, script->filename,scriptfile_getlinum(script,cmdtokptr));
                }
                else
                {
                    int32_t tmp = parseconsounds(included);
                    scriptfile_close(included);
                    if (tmp < 0) return tmp;
                }
            }
            break;
        }
        case T_DEFINE:
        {
            char *name;
            int32_t number;

            if (scriptfile_getstring(script,&name)) break;
            if (scriptfile_getsymbol(script,&number)) break;
            if (scriptfile_addsymbolvalue(name,number) < 0)
                initprintf("Warning: Symbol %s was NOT redefined to %d on line %s:%d\n",
                           name,number,script->filename,scriptfile_getlinum(script,cmdtokptr));
            break;
        }
        case T_DEFINESOUND:
        {
            char *definedname, *filename;
            int32_t sndnum, ps, pe, pr, m, vo;
            int32_t slen;

            if (scriptfile_getsymbol(script, &sndnum)) break;

            definedname = Bstrdup(script->ltextptr);
            if (!definedname) return -1;

            if (sndnum < 0 || sndnum >= MAXSOUNDS)
            {
                initprintf("Warning: invalid sound definition %s (sound number < 0 or >= MAXSOUNDS) on line %s:%d\n",
                           definedname, script->filename,scriptfile_getlinum(script,cmdtokptr));
                num_invalidsounds++;
                break;
            }

            if (scriptfile_getstring(script, &filename))
            {
                Bfree(definedname);
                num_invalidsounds++;
                break;
            }

            slen = Bstrlen(filename);
            if (slen >= BMAX_PATH)
            {
                initprintf("Warning: invalid sound definition %s (filename too long) on line %s:%d\n",
                           definedname, script->filename,scriptfile_getlinum(script,cmdtokptr));
                Bfree(definedname);
                num_invalidsounds++;
                break;
            }

            if (g_sounds[sndnum].filename == NULL)
                g_sounds[sndnum].filename = Bcalloc(slen+1,sizeof(uint8_t));
            // Hopefully noone does memcpy(..., g_sounds[].filename, BMAX_PATH)
            if (!g_sounds[sndnum].filename)
            {
                Bfree(definedname);
                return -1;
            }
            Bmemcpy(g_sounds[sndnum].filename, filename, slen+1);

            if (scriptfile_getnumber(script, &ps)) goto BAD;
            if (scriptfile_getnumber(script, &pe)) goto BAD;
            if (scriptfile_getnumber(script, &pr)) goto BAD;
            if (scriptfile_getnumber(script, &m)) goto BAD;
            if (ParentalLock && (m&8)) goto BAD;
            if (scriptfile_getnumber(script, &vo)) goto BAD;
            if (0)
            {
BAD:
                Bfree(definedname);
                Bfree(g_sounds[sndnum].filename);
                g_sounds[sndnum].filename = NULL;
                num_invalidsounds++;
                break;
            }

            g_sounds[sndnum].definedname = definedname;  // we want to keep it for display purposes
            g_sounds[sndnum].ps = ps;
            g_sounds[sndnum].pe = pe;
            g_sounds[sndnum].pr = pr;
            g_sounds[sndnum].m = m;
            g_sounds[sndnum].vo = vo;
            g_sndnum[g_numsounds] = g_definedsndnum[g_numsounds] = sndnum;
            g_numsounds++;
            if (g_numsounds == MAXSOUNDS)
                goto END;
            break;
        }
        case T_EOF:
            goto END;
        default:
            break;
        }
    }
END:
    return g_numsounds;
}

static int32_t loadconsounds(const char *fn)
{
    scriptfile *script;
    int32_t ret;

    initprintf("Loading sounds from '%s'\n",fn);

    script = scriptfile_fromfile(fn);
    if (!script)
    {
        initprintf("Error loading sounds: file '%s' not found.\n", fn);
        return -1;
    }
    ret = parseconsounds(script);
    if (ret < 0)
        initprintf("There was an error parsing '%s'.\n", fn);
    else if (ret == 0)
        initprintf("'%s' doesn't contain sound definitions. No sounds loaded.\n", fn);
    else
        initprintf("Loaded %d sound definitions.\n", ret);

    scriptfile_close(script);
    scriptfile_clearsymbols();
    return ret;
}

void ExtPreLoadMap(void)
{
}

/// ^^^

static void m32script_interrupt_handler(int signo)
{
    if (signo==SIGINT)
    {
        vm.flags |= VMFLAG_ERROR;
        OSD_Printf("M32 script execution interrupted.\n");
        Bmemset(aEventEnabled, 0, sizeof(aEventEnabled));
    }
}

int32_t ExtInit(void)
{
    int32_t rv = 0;
    int32_t i;
    char cwd[BMAX_PATH];

#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    addsearchpath("/usr/share/games/jfduke3d");
    addsearchpath("/usr/local/share/games/jfduke3d");
    addsearchpath("/usr/share/games/eduke32");
    addsearchpath("/usr/local/share/games/eduke32");
#elif defined(__APPLE__)
    addsearchpath("/Library/Application Support/JFDuke3D");
    addsearchpath("/Library/Application Support/EDuke32");
#endif

    if (getcwd(cwd,BMAX_PATH))
    {
#if defined(__APPLE__)
        /* Dirty hack on OS X to also look for gamedata inside the application bundle - rhoenie 08/08 */
        char seekinappcontainer[BMAX_PATH];
        Bsnprintf(seekinappcontainer,sizeof(seekinappcontainer),"%s/EDuke32.app/", cwd);
        addsearchpath(seekinappcontainer);
#endif
        addsearchpath(cwd);
        Bstrcpy(program_origcwd, cwd);
    }
    else
        program_origcwd[0] = '\0';

    if (CommandPaths)
    {
        struct strllist *s;
        while (CommandPaths)
        {
            s = CommandPaths->next;
            addsearchpath(CommandPaths->str);

            Bfree(CommandPaths->str);
            Bfree(CommandPaths);
            CommandPaths = s;
        }
    }

#if defined(_WIN32)
    if (!access("user_profiles_enabled", F_OK))
#else
    if (usecwd == 0 && access("user_profiles_disabled", F_OK))
#endif
    {
        char *homedir;
        int32_t asperr;

        if ((homedir = Bgethomedir()))
        {
            Bsnprintf(cwd,sizeof(cwd),"%s/"
#if defined(_WIN32)
                      "EDuke32 Settings"
#elif defined(__APPLE__)
                      "Library/Application Support/EDuke32"
#else
                      ".eduke32"
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
            Bfree(homedir);
        }
    }

    // JBF 20031220: Because it's annoying renaming GRP files whenever I want to test different game data
    if (getenv("DUKE3DGRP"))
    {
        g_grpNamePtr = getenv("DUKE3DGRP");
        initprintf("Using %s as main GRP file\n", g_grpNamePtr);
    }

    i = initgroupfile(g_grpNamePtr);

    if (!NoAutoLoad)
    {
        static const char *extensions[3] = { "*.grp", "*.zip", "*.pk3" };
        int32_t exti;

        for (exti=0; exti<3; exti++)
        {
            getfilenames("autoload", extensions[exti]);
            while (findfiles)
            {
                Bsprintf(tempbuf,"autoload/%s",findfiles->name);
                initprintf("Using group file '%s'.\n",tempbuf);
                initgroupfile(tempbuf);
                findfiles = findfiles->next;
            }
        }

        if (i != -1)
            DoAutoload(g_grpNamePtr);
    }

    if (getenv("DUKE3DDEF"))
    {
        defsfilename = getenv("DUKE3DDEF");
        initprintf("Using '%s' as definitions file\n", defsfilename);
    }
    loadgroupfiles(defsfilename);

    {
        struct strllist *s;
        int32_t j;

        pathsearchmode = 1;
        while (CommandGrps)
        {
            s = CommandGrps->next;
            j = initgroupfile(CommandGrps->str);
            if (j == -1) initprintf("Could not find group file '%s'.\n",CommandGrps->str);
            else
            {
                initprintf("Using group file '%s'.\n",CommandGrps->str);
                if (!NoAutoLoad)
                    DoAutoload(CommandGrps->str);
            }

            Bfree(CommandGrps->str);
            Bfree(CommandGrps);
            CommandGrps = s;
        }
        pathsearchmode = 0;
    }

    bpp = 32;

#if defined(POLYMOST) && defined(USE_OPENGL)
    glusetexcache = -1;

    if (Bstrcmp(setupfilename, "mapster32.cfg"))
        initprintf("Using config file '%s'.\n",setupfilename);

    if (loadsetup(setupfilename) < 0) initprintf("Configuration file not found, using defaults.\n"), rv = 1;

    if (glusetexcache == -1)
    {
        int32_t i;
#if 0
        i=wm_ynbox("Texture Cache",
                   "Would you like to enable the on-disk texture cache?\n\n"
                   "You generally want to say 'yes' here, especially if using the HRP.");
#else
        i = 1;
#endif
        if (i)
            glusetexcompr = 1, glusetexcache = 2;
        else glusetexcache = 0;
    }
#endif

    Bmemcpy((void *)buildkeys,(void *)keys,NUMBUILDKEYS);   //Trick to make build use setup.dat keys

    if (initengine())
    {
        initprintf("There was a problem initializing the engine.\n");
        return -1;
    }

    kensplayerheight = 40; //32
    zmode = 2;
    zlock = kensplayerheight<<8;
    defaultspritecstat = 0;

    ReadGamePalette();
    //  InitWater();

    InitCustomColors();

    getmessageleng = 0;
    getmessagetimeoff = 0;

    Bstrcpy(apptitle, "Mapster32"VERSION BUILDDATE);
    autosavetimer = totalclock+120*autosave;

#if defined(DUKEOSD)
    m32_osdsetfunctions();
#endif

    OSD_SetParameters(0,2, 0,0, 4,0);
    registerosdcommands();

    // backup pathsearchmode so that a later open
    // will hopefully be the same file
    pathsearchmode_oninit = pathsearchmode;
    loadtilegroups(default_tiles_cfg);

    ReadHelpFile("m32help.hlp");

    signal(SIGINT, m32script_interrupt_handler);

    return rv;
}

#ifdef RENDERTYPEWIN
void app_crashhandler(void)
{
    if (levelname[0])
    {
        char *f;
        fixspritesectors();   //Do this before saving!
        updatesector(startposx,startposy,&startsectnum);
        if (pathsearchmode)
            f = levelname;
        else
        {
            // virtual filesystem mode can't save to directories so drop the file into
            // the current directory
            f = Bstrrchr(levelname, '/');
            if (!f) f = levelname; else f++;
        }
        f=strstr(levelname,".map");
        if (f)
            Bstrcpy(f,"_crash.map");
        else Bstrcat(f,"_crash.map");
        ExtPreSaveMap();
        saveboard(levelname,&startposx,&startposy,&startposz,&startang,&startsectnum);
        ExtSaveMap(levelname);
    }
}
#endif

void ExtUnInit(void)
{
    int32_t i;
    // setvmode(0x03);
    S_SoundShutdown();
    uninitgroupfile();
    writesetup(setupfilename);

    for (i = MAX_TILE_GROUPS-1; i >= 0; i--)
    {
        if (s_TileGroups[i].pIds != NULL)
            Bfree(s_TileGroups[i].pIds);
        if (s_TileGroups[i].szText != NULL)
            Bfree(s_TileGroups[i].szText);
    }
    for (i = numhelppages-1; i >= 0; i--) Bfree(helppage[i]);
    if (helppage) Bfree(helppage);
}

void ExtPreCheckKeys(void) // just before drawrooms
{
    int32_t i = 0, ii;
    int32_t radius, xp1, yp1;
    int32_t col;
    int32_t picnum, frames;
    int32_t ang = 0, flags, shade;

    if (qsetmode == 200)    //In 3D mode
    {
        if (shadepreview)
        {
            int32_t i = 0;
            for (i=numsprites-1; i>=0; i--)
                if (sprite[i].picnum == SECTOREFFECTOR && (sprite[i].lotag == 12 || sprite[i].lotag == 3))
                {
                    int32_t w, isec=sprite[i].sectnum;
                    int32_t start_wall = sector[isec].wallptr;
                    int32_t end_wall = start_wall + sector[isec].wallnum;

                    if (isec<0)
                        continue;

                    for (w = start_wall; w < end_wall; w++)
                    {
                        if (!wallflag[w])
                        {
                            wallshades[w] = wall[w].shade;
                            wall[w].shade = sprite[i].shade;
                            wallpals[w] = wall[w].pal;
                            wall[w].pal = sprite[i].pal;
                            wallflag[w] = 1;
                        }
                        /*                        if (wall[w].nextwall >= 0)
                        {
                        if (!wallflag[wall[w].nextwall])
                        {
                        wallshades[wall[w].nextwall] = NEXTWALL(w).shade;
                        NEXTWALL(w).shade = sprite[i].shade;
                        wallpals[wall[w].nextwall] = NEXTWALL(w).pal;
                        NEXTWALL(w).pal = sprite[i].pal;
                        wallflag[wall[w].nextwall] = 1;
                        }
                        } */
                    }
                    sectorshades[isec][0] = sector[isec].floorshade;
                    sectorshades[isec][1] = sector[isec].ceilingshade;
                    sector[isec].floorshade = sprite[i].shade;
                    sector[isec].ceilingshade = sprite[i].shade;
                    sectorpals[isec][0] = sector[isec].floorpal;
                    sectorpals[isec][1] = sector[isec].ceilingpal;
                    sector[isec].floorpal = sprite[i].pal;
                    sector[isec].ceilingpal = sprite[i].pal;
                    w = headspritesect[isec];
                    while (w >= 0)
                    {
                        if (w == i) { w = nextspritesect[w]; continue; }
                        spriteshades[w] = sprite[w].shade;
                        sprite[w].shade = sprite[i].shade;
                        spritepals[w] = sprite[w].pal;
                        sprite[w].pal = sprite[i].pal;
                        w = nextspritesect[w];
                    }
                }
                else if (sprite[i].picnum == SECTOREFFECTOR && (sprite[i].lotag == 49 || sprite[i].lotag == 50))
                {
#ifdef POLYMER
                    if (sprite[i].lotag == 49)
                    {
                        if (getrendermode() == 4)
                        {
                            if (spritelightptr[i] == NULL)
                            {
#pragma pack(push,1)
                                _prlight mylight;
#pragma pack(pop)
                                mylight.sector = SECT;
                                Bmemcpy(&mylight, &sprite[i], sizeof(vec3_t));
                                mylight.range = SHT;
                                mylight.color[0] = sprite[i].xvel;
                                mylight.color[1] = sprite[i].yvel;
                                mylight.color[2] = sprite[i].zvel;
                                mylight.radius = 0;
                                mylight.angle = SA;
                                mylight.horiz = SH;
                                mylight.minshade = sprite[i].xoffset;
                                mylight.maxshade = sprite[i].yoffset;
                                mylight.tilenum = 0;

                                if (CS & 2)
                                {
                                    if (CS & 512)
                                        mylight.priority = PR_LIGHT_PRIO_LOW;
                                    else
                                        mylight.priority = PR_LIGHT_PRIO_HIGH;
                                }
                                else
                                    mylight.priority = PR_LIGHT_PRIO_MAX;

                                spritelightid[i] = polymer_addlight(&mylight);
                                if (spritelightid[i] >= 0)
                                    spritelightptr[i] = &prlights[spritelightid[i]];
                            }
                            else
                            {
                                if (Bmemcmp(&sprite[i], spritelightptr[i], sizeof(vec3_t)))
                                {
                                    Bmemcpy(spritelightptr[i], &sprite[i], sizeof(vec3_t));
                                    spritelightptr[i]->sector = sprite[i].sectnum;
                                    spritelightptr[i]->flags.invalidate = 1;
                                }
                                if (SHT != spritelightptr[i]->range)
                                {
                                    spritelightptr[i]->range = SHT;
                                    spritelightptr[i]->flags.invalidate = 1;
                                }
                                if ((sprite[i].xvel != spritelightptr[i]->color[0]) ||
                                        (sprite[i].yvel != spritelightptr[i]->color[1]) ||
                                        (sprite[i].zvel != spritelightptr[i]->color[2]))
                                {
                                    spritelightptr[i]->color[0] = sprite[i].xvel;
                                    spritelightptr[i]->color[1] = sprite[i].yvel;
                                    spritelightptr[i]->color[2] = sprite[i].zvel;
                                }
                            }
                        }
                    }
                    if (sprite[i].lotag == 50)
                    {
                        if (getrendermode() == 4)
                        {
                            if (spritelightptr[i] == NULL)
                            {
#pragma pack(push,1)
                                _prlight mylight;
#pragma pack(pop)

                                mylight.sector = SECT;
                                Bmemcpy(&mylight, &sprite[i], sizeof(vec3_t));
                                mylight.range = SHT;
                                mylight.color[0] = sprite[i].xvel;
                                mylight.color[1] = sprite[i].yvel;
                                mylight.color[2] = sprite[i].zvel;
                                mylight.radius = (256-(SS+128))<<1;
                                mylight.faderadius = (int16_t)(mylight.radius * 0.75f);
                                mylight.angle = SA;
                                mylight.horiz = SH;
                                mylight.minshade = sprite[i].xoffset;
                                mylight.maxshade = sprite[i].yoffset;
                                mylight.tilenum = OW;

                                if (CS & 2)
                                {
                                    if (CS & 512)
                                        mylight.priority = PR_LIGHT_PRIO_LOW;
                                    else
                                        mylight.priority = PR_LIGHT_PRIO_HIGH;
                                }
                                else
                                    mylight.priority = PR_LIGHT_PRIO_MAX;

                                spritelightid[i] = polymer_addlight(&mylight);
                                if (spritelightid[i] >= 0)
                                    spritelightptr[i] = &prlights[spritelightid[i]];
                            }
                            else
                            {
                                if (Bmemcmp(&sprite[i], spritelightptr[i], sizeof(vec3_t)))
                                {
                                    Bmemcpy(spritelightptr[i], &sprite[i], sizeof(vec3_t));
                                    spritelightptr[i]->sector = sprite[i].sectnum;
                                    spritelightptr[i]->flags.invalidate = 1;
                                }
                                if (SHT != spritelightptr[i]->range)
                                {
                                    spritelightptr[i]->range = SHT;
                                    spritelightptr[i]->flags.invalidate = 1;
                                }
                                if ((sprite[i].xvel != spritelightptr[i]->color[0]) ||
                                        (sprite[i].yvel != spritelightptr[i]->color[1]) ||
                                        (sprite[i].zvel != spritelightptr[i]->color[2]))
                                {
                                    spritelightptr[i]->color[0] = sprite[i].xvel;
                                    spritelightptr[i]->color[1] = sprite[i].yvel;
                                    spritelightptr[i]->color[2] = sprite[i].zvel;
                                }
                                if (((256-(SS+128))<<1) != spritelightptr[i]->radius)
                                {
                                    spritelightptr[i]->radius = (256-(SS+128))<<1;
                                    spritelightptr[i]->faderadius = (int16_t)(spritelightptr[i]->radius * 0.75f);
                                    spritelightptr[i]->flags.invalidate = 1;
                                }
                                if (SA != spritelightptr[i]->angle)
                                {
                                    spritelightptr[i]->angle = SA;
                                    spritelightptr[i]->flags.invalidate = 1;
                                }
                                if (SH != spritelightptr[i]->horiz)
                                {
                                    spritelightptr[i]->horiz = SH;
                                    spritelightptr[i]->flags.invalidate = 1;
                                }
                                spritelightptr[i]->tilenum = OW;
                            }
                        }
                    }
#endif // POLYMER
                }
        }

        if (floor_over_floor) SE40Code(pos.x,pos.y,pos.z,ang,horiz);
        if (purpleon) clearview(255);

        return;
    }
    begindrawing();

    //    if (cursectornum >= 0)
    //        fillsector(cursectornum, 31);

    if (graphicsmode && zoom >= 256)
    {
        for (i=ii=0; i<MAXSPRITES && ii < numsprites; i++)
        {
            if ((sprite[i].cstat & 48) != 0 || sprite[i].statnum == MAXSTATUS) continue;
            ii++;
            picnum = sprite[i].picnum;
            ang = flags = frames = shade = 0;

            switch (picnum)
            {
                // 5-frame walk
            case 1550 :             // Shark
                frames=5;
                // 2-frame walk
            case 1445 :             // duke kick
            case LIZTROOPDUCKING :
            case 2030 :            // pig shot
            case OCTABRAIN :
            case PIGCOPDIVE :
            case 2190 :            // liz capt shot
            case BOSS1SHOOT :
            case BOSS1LOB :
            case LIZTROOPSHOOT :
                if (frames==0) frames=2;

                // 4-frame walk
            case 1491 :             // duke crawl
            case LIZTROOP :
            case LIZTROOPRUNNING :
            case PIGCOP :
            case LIZMAN :
            case BOSS1 :
            case BOSS2 :
            case BOSS3 :
            case BOSS4 :
            case NEWBEAST:
                if (frames==0) frames=4;
            case LIZTROOPJETPACK :
            case DRONE :
            case COMMANDER :
            case TANK :
            case RECON :
                if (frames==0) frames = 10;
            case CAMERA1:
            case APLAYER :
                if (frames==0) frames=1;
            case GREENSLIME :
            case EGG :
            case PIGCOPSTAYPUT :
            case LIZMANSTAYPUT:
            case LIZTROOPSTAYPUT :
            case LIZMANSPITTING :
            case LIZMANFEEDING :
            case LIZMANJUMP :
            case NEWBEASTSTAYPUT :
            case BOSS1STAYPUT :
            {
                int32_t k;
                if (frames!=0)
                {
                    if (frames==10) frames=0;
                    k = 1536;//getangle(tspr->x-pos.x,tspr->y-pos.y);
                    k = (((sprite[i].ang+3072+128-k)&2047)>>8)&7;
                    //This guy has only 5 pictures for 8 angles (3 are x-flipped)
                    if (k <= 4)
                    {
                        picnum += k;
                        ang = 0;
                        flags &= ~4;
                    }
                    else
                    {
                        picnum += 8-k;
                        ang = 1024;
                        flags |= 4;
                    }
                }

                if (graphicsmode == 2)
                {
                    if (frames==2) picnum+=((((4-(totalclock>>5)))&1)*5);
                    if (frames==4) picnum+=((((4-(totalclock>>5)))&3)*5);
                    if (frames==5) picnum+=(((totalclock>>5)%5))*5;
                }

                if (tilesizx[picnum] == 0)
                    picnum -= 5;       //Hack, for actors
            }
            break;
            default:
                break;

            }

            xp1 = mulscale14(sprite[i].x-pos.x,zoom);
            yp1 = mulscale14(sprite[i].y-pos.y-(tilesizy[picnum]<<2),zoom);
            if (i+16384 != pointhighlight || !(totalclock&32))
            {
                shade = sprite[i].shade;
                if (shade < 6)
                    shade = 6;
            }

            xp1 += halfxdim16;
            yp1 += midydim16;

            ydim16 = ydim-STATUS2DSIZ2;

            if (xp1 < 4 || xp1 > xdim-6 || yp1 < 4 || yp1 > ydim16-6)
                continue;
            rotatesprite(xp1<<16,yp1<<16,zoom<<5,ang,picnum,
                         shade,sprite[i].pal,flags,0,0,xdim-1,ydim16-1);
        }
    }

    if (showambiencesounds)
        for (i=0; i<numsprites; i++)
            if (sprite[i].picnum == MUSICANDSFX /*&& zoom >= 256*/ && sprite[i].sectnum != MAXSECTORS)
            {
                if (showambiencesounds==1 && sprite[i].sectnum!=cursectnum)
                    continue;

                xp1 = mulscale14(sprite[i].x-pos.x,zoom);
                yp1 = mulscale14(sprite[i].y-pos.y,zoom);

                radius = mulscale14(sprite[i].hitag,zoom);
                col = 6;
                if (i+16384 == pointhighlight)
                    if (totalclock & 32) col += (2<<2);
                drawlinepat = 0xf0f0f0f0;
                drawcircle16(halfxdim16+xp1, midydim16+yp1, radius, editorcolors[(int32_t)col]);
                drawlinepat = 0xffffffff;
                //            radius = mulscale15(sprite[i].hitag,zoom);
                //          drawcircle16(halfxdim16+xp1, midydim16+yp1, radius, col);
            }

    enddrawing();
}

void ExtAnalyzeSprites(void)
{
    int32_t i, k;
    spritetype *tspr;
    int32_t frames=0, l;

    for (i=0,tspr=&tsprite[0]; i<spritesortcnt; i++,tspr++)
    {
        frames=0;

        if ((nosprites==1||nosprites==3)&&tspr->picnum<11) tspr->xrepeat=0;

        if (nosprites==1||nosprites==3)
            switch (tspr->picnum)
            {
            case SEENINE :
                tspr->xrepeat=0;
            }

        if (showinvisibility && (tspr->cstat&32768))
        {
            tspr->pal = 6;
            tspr->cstat &= ~32768;
            tspr->cstat |= 2+512;
        }

        if (shadepreview && !(tspr->cstat & 16))
        {
            if (tspr->sectnum<0)
                continue;

            if (sector[tspr->sectnum].ceilingstat&1)
            {
                l = sector[tspr->sectnum].ceilingshade;
                if (sector[tspr->sectnum].ceilingpal != 0 && sector[tspr->sectnum].ceilingpal < num_tables)
                    tspr->pal=sector[tspr->sectnum].ceilingpal;
            }
            else
            {
                l = sector[tspr->sectnum].floorshade;
                if (sector[tspr->sectnum].floorpal != 0 && sector[tspr->sectnum].floorpal < num_tables)
                    tspr->pal=sector[tspr->sectnum].floorpal;
            }
            if (l < -127) l = -127;
            if (l > 126) l =  127;

            tspr->shade = l;

        }

        switch (tspr->picnum)
        {
            // 5-frame walk
        case 1550 :             // Shark
            frames=5;
            // 2-frame walk
        case 1445 :             // duke kick
        case LIZTROOPDUCKING :
        case 2030 :            // pig shot
        case OCTABRAIN :
        case PIGCOPDIVE :
        case 2190 :            // liz capt shot
        case BOSS1SHOOT :
        case BOSS1LOB :
        case LIZTROOPSHOOT :
            if (frames==0) frames=2;

            // 4-frame walk
        case 1491 :             // duke crawl
        case LIZTROOP :
        case LIZTROOPRUNNING :
        case PIGCOP :
        case LIZMAN :
        case BOSS1 :
        case BOSS2 :
        case BOSS3 :
        case BOSS4 :
        case NEWBEAST:
            if (frames==0) frames=4;
        case LIZTROOPJETPACK :
        case DRONE :
        case COMMANDER :
        case TANK :
        case RECON :
            if (frames==0) frames = 10;
        case ROTATEGUN :
        case CAMERA1:
        case APLAYER :
            if (frames==0) frames=1;
        case GREENSLIME :
        case PIGCOPSTAYPUT :
        case LIZMANSTAYPUT:
        case LIZTROOPSTAYPUT :
        case LIZMANSPITTING :
        case LIZMANFEEDING :
        case LIZMANJUMP :
        case NEWBEASTSTAYPUT :
        case BOSS1STAYPUT :
            if (skill!=4)
            {
                if (tspr->lotag>skill+1)
                {
                    tspr->xrepeat=0;
                    tspr->cstat=32768;
                    break;
                }
            }
            if (nosprites==2||nosprites==3)
            {
                tspr->xrepeat=0;
                tspr->cstat=32768;
            }
            //                else tspr->cstat&=32767;

#if defined(USE_OPENGL) && defined(POLYMOST)
            if (!usemodels || md_tilehasmodel(tspr->picnum,tspr->pal) < 0)
#endif
            {
                if (frames!=0)
                {
                    if (frames==10) frames=0;
                    k = getangle(tspr->x-pos.x,tspr->y-pos.y);
                    k = (((tspr->ang+3072+128-k)&2047)>>8)&7;
                    //This guy has only 5 pictures for 8 angles (3 are x-flipped)
                    if (k <= 4)
                    {
                        tspr->picnum += k;
                        tspr->cstat &= ~4;   //clear x-flipping bit
                    }
                    else
                    {
                        tspr->picnum += 8-k;
                        tspr->cstat |= 4;    //set x-flipping bit
                    }
                }

                if (frames==2) tspr->picnum += (((4-(totalclock>>5)))&1)*5;
                if (frames==4) tspr->picnum += (((4-(totalclock>>5)))&3)*5;
                if (frames==5) tspr->picnum += ((totalclock>>5)%5)*5;

                if (tilesizx[tspr->picnum] == 0)
                    tspr->picnum -= 5;       //Hack, for actors
            }
            break;
        default:
            break;

        }
    }

    VM_OnEvent(EVENT_ANALYZESPRITES, -1);
}

#define MESSAGEX 3 // (xdimgame>>1)
#define MESSAGEY 3 // ((i/charsperline)<<3)+(ydimgame-(ydimgame>>3))-(((getmessageleng-1)/charsperline)<<3)

static void Keys2d3d(void)
{
    int32_t i;

    if (mapstate == NULL)
    {
        //        map_revision = 0;
        create_map_snapshot(); // initial map state
        //        Bfree(mapstate->next);
        //        mapstate = mapstate->prev;
    }

    if (keystatus[KEYSC_QUOTE] && PRESSED_KEYSC(A)) // 'A
    {
        if (qsetmode == 200)
            autosave = autosave?0:getnumber256("Autosave interval, in seconds: ",180,3600,0);
        else
            autosave = autosave?0:getnumber16("Autosave interval, in seconds: ",180,3600,0);

        if (autosave) message("Autosave enabled, interval: %d seconds",autosave);
        else message("Autosave disabled");
    }

    if (keystatus[KEYSC_QUOTE] && PRESSED_KEYSC(N)) // 'N
    {
        noclip = !noclip;
        message("Clipping %s", noclip?"disabled":"enabled");
    }

    if (eitherCTRL && PRESSED_KEYSC(N)) // CTRL+N
    {
        spnoclip = !spnoclip;
        message("Sprite clipping %s", spnoclip?"disabled":"enabled");
    }

    if (eitherCTRL)  //CTRL
    {
        char *f;
        if (pathsearchmode) f = levelname;
        else
        {
            // virtual filesystem mode can't save to directories so drop the file into
            // the current directory
            f = Bstrrchr(levelname, '/');
            if (!f) f = levelname; else f++;
        }

        if (PRESSED_KEYSC(P)) // Ctrl-P: Map playtesting
        {
            if (qsetmode != 200)
                test_map(eitherALT);
        }

        if (keystatus[KEYSC_S]) // S
        {
            if (levelname[0])
            {
                keystatus[KEYSC_S] = 0;
                fixspritesectors();   //Do this before saving!
                updatesector(startposx,startposy,&startsectnum);
                ExtPreSaveMap();
                saveboard(f,&startposx,&startposy,&startposz,&startang,&startsectnum);
                ExtSaveMap(f);
                message("Board saved");
                asksave = 0;
                lastsave=totalclock;
            }
        }
        if (keystatus[KEYSC_L]) // L
        {
            extern int32_t grponlymode;
            extern void loadmhk();

            if (totalclock < (lastsave + 120*10) || !AskIfSure("Are you sure you want to load the last saved map?"))
            {
                int32_t sposx=pos.x,sposy=pos.y,sposz=pos.z,sang=ang;

                lastsave=totalclock;
                highlightcnt = -1;
                //  			  sectorhighlightstat = -1;
                //  			  newnumwalls = -1;
                //  			  joinsector[0] = -1;
                //  			  circlewall = -1;
                //  			  circlepoints = 7;

                for (i=MAXSECTORS-1; i>=0; i--) sector[i].extra = -1;
                for (i=MAXWALLS-1; i>=0; i--) wall[i].extra = -1;
                for (i=MAXSPRITES-1; i>=0; i--) sprite[i].extra = -1;

                ExtPreLoadMap();
                i = loadboard(f,(!pathsearchmode&&grponlymode?2:0),&pos.x,&pos.y,&pos.z,&ang,&cursectnum);
                loadmhk();
                if (i == -2) i = loadoldboard(f,(!pathsearchmode&&grponlymode?2:0),&pos.x,&pos.y,&pos.z,&ang,&cursectnum);
                if (i < 0) printmessage16("Invalid map format.");
                else
                {
                    ExtLoadMap(f);
                    if (mapversion < 7) message("Map %s loaded successfully and autoconverted to V7!",f);
                    else message("Map %s loaded successfully",f);
                }
                updatenumsprites();
                startposx = pos.x;
                startposy = pos.y;
                startposz = pos.z;
                startang = ang;
                startsectnum = cursectnum;
                pos.x=sposx; pos.y=sposy; pos.z=sposz; ang=sang;
                keystatus[KEYSC_L]=0;
            }
        }
    }

    if (keystatus[buildkeys[BK_MODE2D_3D]])  // Enter
    {
        getmessageleng = 0;
        getmessagetimeoff = 0;
#if defined(DUKEOSD)
        m32_osdsetfunctions();
#endif
    }

    if (getmessageleng > 0)
    {
        if (qsetmode != 200)
            printmessage16(getmessage);
        if (totalclock > getmessagetimeoff)
            getmessageleng = 0;
    }

}
#undef EDUKE32_EXEC
#undef EDUKE32_LOCALEXEC

void ExtCheckKeys(void)
{
    static int32_t soundinit = 0;
    static int32_t lastbstatus = 0;

    if (!soundinit)
    {
        g_numsounds = 0;
        loadconsounds(gamecon);
        if (g_numsounds > 0)
        {
            if (S_SoundStartup() != 0)
                S_SoundShutdown();
        }
        soundinit = 1;
    }

    if (qsetmode == 200)
    {
        if (shadepreview)
        {
            int32_t i = 0;
            for (i=numsprites-1; i>=0; i--)
                if (sprite[i].picnum == SECTOREFFECTOR && (sprite[i].lotag == 12 || sprite[i].lotag == 3))
                {
                    int32_t w, isec=sprite[i].sectnum;
                    int32_t start_wall = sector[isec].wallptr;
                    int32_t end_wall = start_wall + sector[isec].wallnum;

                    if (isec<0)
                        continue;

                    for (w = start_wall; w < end_wall; w++)
                    {
                        if (wallflag[w])
                        {
                            wall[w].shade = wallshades[w];
                            wall[w].pal = wallpals[w];
                            wallflag[w] = 0;
                        }
                        /*                        if (wall[w].nextwall >= 0)
                        {
                        if (wallflag[wall[w].nextwall])
                        {
                        NEXTWALL(w).shade = wallshades[wall[w].nextwall];
                        NEXTWALL(w).pal = wallpals[wall[w].nextwall];
                        wallflag[wall[w].nextwall] = 0;
                        }
                        } */
                    }
                    sector[isec].floorshade = sectorshades[isec][0];
                    sector[isec].ceilingshade = sectorshades[isec][1];
                    sector[isec].floorpal = sectorpals[isec][0];
                    sector[isec].ceilingpal = sectorpals[isec][1];

                    w = headspritesect[isec];
                    while (w >= 0)
                    {
                        if (w == i) { w = nextspritesect[w]; continue; }
                        sprite[w].shade = spriteshades[w];
                        sprite[w].pal = spritepals[w];
                        w = nextspritesect[w];
                    }
                }
        }
    }
    lastbstatus = bstatus;
    readmousebstatus(&bstatus);

    Keys2d3d();

    if (qsetmode == 200)    //In 3D mode
    {
        Keys3d();
        editinput();
        if (infobox&2)
            m32_showmouse();
    }
    else Keys2d();

    if (asksave == 1 && (bstatus + lastbstatus) == 0 && mapstate)
    {
        //        message("Saved undo rev %d",map_revision);
        create_map_snapshot();
        asksave++;
    }
    else if (asksave == 2) asksave++;

    if (totalclock > autosavetimer && autosave)
    {
        if (asksave == 3)
        {
            fixspritesectors();   //Do this before saving!
            //             updatesector(startposx,startposy,&startsectnum);
            ExtPreSaveMap();
            saveboard("autosave.map",&startposx,&startposy,&startposz,&startang,&startsectnum);
            ExtSaveMap("autosave.map");
            message("Board autosaved to AUTOSAVE.MAP");
            asksave = 4;
        }
        autosavetimer = totalclock+120*autosave;
    }

    if (PRESSED_KEYSC(F12))   //F12
    {
        screencapture("captxxxx.tga", keystatus[KEYSC_LSHIFT]|keystatus[KEYSC_RSHIFT]);
        message("Saved screenshot %04d", capturecount-1);
    }
}

void faketimerhandler(void)
{
    sampletimer();
}

void SetBOSS1Palette(void)
{
    if (acurpalette==3) return;
    acurpalette=3;
    setbrightness(GAMMA_CALC,BOSS1palette,0);
}

void SetSLIMEPalette(void)
{
    if (acurpalette==2) return;
    acurpalette=2;
    setbrightness(GAMMA_CALC,SLIMEpalette,0);
}

void SetWATERPalette(void)
{
    if (acurpalette==1) return;
    acurpalette=1;
    setbrightness(GAMMA_CALC,WATERpalette,0);
}

void SetGAMEPalette(void)
{
    if (acurpalette==0) return;
    acurpalette=0;
    setbrightness(GAMMA_CALC,GAMEpalette,0);
}

static void SearchSectors(int32_t dir)  // <0: backwards, >=0: forwards
{
    int32_t ii=0;
    dir = 1-2*(dir<0);

    if (cursector_lotag!=0)
    {
        if ((dir>0 && cursectornum<MAXSECTORS) || (dir<0 && cursectornum>=0))
            cursectornum += dir;

        for (ii=cursectornum; ii>=0 && ii<numsectors; ii+=dir)
        {
            if (sector[ii].lotag==cursector_lotag)
            {
                pos.x = wall[sector[ii].wallptr].x;
                pos.y = wall[sector[ii].wallptr].y;
                printmessage16("%s Sector search: found", dir<0?"<":">");
                return;
            }
            cursectornum += dir;
        }
    }
    printmessage16("%s Sector search: none found", dir<0?"<":">");
}

// Build edit originally by Ed Coolidge <semicharm@earthlink.net>
static void EditSectorData(int16_t sectnum)
{
    char disptext[80];
    char edittext[80];
    int32_t col=1, row=0, rowmax = 6, dispwidth = 24, editval = 0, i = -1;
    int32_t xpos = 208, ypos = ydim-STATUS2DSIZ+48;

    drawgradient();

    disptext[dispwidth] = 0;

    showsectordata(sectnum, 0);

    begindrawing();
    while (keystatus[KEYSC_ESC] == 0)
    {
        idle_waitevent();
        if (handleevents())
            quitevent = 0;

        _printmessage16("Edit mode, press <Esc> to exit");
        if (PRESSED_KEYSC(DOWN))
        {
            if (row < rowmax)
            {
                printext16(xpos,ypos+row*8,editorcolors[11],editorcolors[0],disptext,0);
                row++;
            }
        }
        if (PRESSED_KEYSC(UP))
        {
            if (row > 0)
            {
                printext16(xpos,ypos+row*8,editorcolors[11],editorcolors[0],disptext,0);
                row--;
            }
        }
        if (PRESSED_KEYSC(LEFT))
        {
            if (col == 2)
            {
                printext16(xpos,ypos+row*8,editorcolors[11],editorcolors[0],disptext,0);
                col = 1;
                xpos = 208;
                rowmax = 6;
                dispwidth = 24;
                disptext[dispwidth] = 0;
                if (row > rowmax) row = rowmax;
            }
        }
        if (PRESSED_KEYSC(RIGHT))
        {
            if (col == 1)
            {
                printext16(xpos,ypos+row*8,editorcolors[11],editorcolors[0],disptext,0);
                col = 2;
                xpos = 408;
                rowmax = 6;
                dispwidth = 24;
                disptext[dispwidth] = 0;
                if (row > rowmax) row = rowmax;
            }
        }
        if (PRESSED_KEYSC(ENTER))
            editval = 1;

        if (col == 1)
        {
            switch (row)
            {
            case 0:
                for (i=Bsprintf(disptext,"Flags (hex): %x",sector[sectnum].ceilingstat); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sector %d Ceiling Flags: ",sectnum);
                if (editval)
                {
                    printmessage16(edittext);
                    sector[sectnum].ceilingstat = (int16_t)getnumber16(edittext,(int32_t)sector[sectnum].ceilingstat,65536L,0);
                }
                break;
            case 1:
                for (i=Bsprintf(disptext,"(X,Y)pan: %d, %d",sector[sectnum].ceilingxpanning,sector[sectnum].ceilingypanning); i < dispwidth; i++) disptext[i] = ' ';
                if (editval)
                {
                    Bsprintf(edittext,"Sector %d Ceiling X Pan: ",sectnum);
                    printmessage16(edittext);
                    sector[sectnum].ceilingxpanning = (char)getnumber16(edittext,(int32_t)sector[sectnum].ceilingxpanning,256L,0);
                    Bsprintf(edittext,"Sector %d Ceiling Y Pan: ",sectnum);
                    printmessage16(edittext);
                    sector[sectnum].ceilingypanning = (char)getnumber16(edittext,(int32_t)sector[sectnum].ceilingypanning,256L,0);
                }
                break;
            case 2:
                for (i=Bsprintf(disptext,"Shade byte: %d",sector[sectnum].ceilingshade); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sector %d Ceiling Shade: ",sectnum);
                if (editval)
                {
                    printmessage16(edittext);
                    sector[sectnum].ceilingshade = (char)getnumber16(edittext,(int32_t)sector[sectnum].ceilingshade,128L,1);
                }
                break;

            case 3:
                for (i=Bsprintf(disptext,"Z-coordinate: %d",sector[sectnum].ceilingz); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sector %d Ceiling Z-coordinate: ",sectnum);
                if (editval)
                {
                    printmessage16(edittext);
                    sector[sectnum].ceilingz = getnumber16(edittext,sector[sectnum].ceilingz,8388608,1); //2147483647L,-2147483648L
                }
                break;

            case 4:
                for (i=Bsprintf(disptext,"Tile number: %d",sector[sectnum].ceilingpicnum); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sector %d Ceiling Tile Number: ",sectnum);
                if (editval)
                {
                    printmessage16(edittext);
                    sector[sectnum].ceilingpicnum = (int16_t)getnumber16(edittext,(int32_t)sector[sectnum].ceilingpicnum,MAXTILES,0);
                }
                break;

            case 5:
                for (i=Bsprintf(disptext,"Ceiling heinum: %d",sector[sectnum].ceilingheinum); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sector %d Ceiling Heinum: ",sectnum);
                if (editval)
                {
                    printmessage16(edittext);
                    sector[sectnum].ceilingheinum = (int16_t)getnumber16(edittext,(int32_t)sector[sectnum].ceilingheinum,65536L,1);
                }
                break;

            case 6:
                for (i=Bsprintf(disptext,"Palookup number: %d",sector[sectnum].ceilingpal); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sector %d Ceiling Palookup Number: ",sectnum);
                if (editval)
                {
                    printmessage16(edittext);
                    sector[sectnum].ceilingpal = (char)getnumber16(edittext,(int32_t)sector[sectnum].ceilingpal,MAXPALOOKUPS,0);
                }
                break;
            }
        }
        if (col == 2)
        {
            switch (row)
            {
            case 0:
                for (i=Bsprintf(disptext,"Flags (hex): %x",sector[sectnum].floorstat); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sector %d Floor Flags: ",sectnum);
                if (editval)
                {
                    printmessage16(edittext);
                    sector[sectnum].floorstat = (int16_t)getnumber16(edittext,(int32_t)sector[sectnum].floorstat,65536L,0);
                }
                break;

            case 1:
                for (i=Bsprintf(disptext,"(X,Y)pan: %d, %d",sector[sectnum].floorxpanning,sector[sectnum].floorypanning); i < dispwidth; i++) disptext[i] = ' ';
                if (editval)
                {
                    Bsprintf(edittext,"Sector %d Floor X Pan: ",sectnum);
                    printmessage16(edittext);
                    sector[sectnum].floorxpanning = (char)getnumber16(edittext,(int32_t)sector[sectnum].floorxpanning,256L,0);
                    Bsprintf(edittext,"Sector %d Floor Y Pan: ",sectnum);
                    printmessage16(edittext);
                    sector[sectnum].floorypanning = (char)getnumber16(edittext,(int32_t)sector[sectnum].floorypanning,256L,0);
                }
                break;

            case 2:
                for (i=Bsprintf(disptext,"Shade byte: %d",sector[sectnum].floorshade); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sector %d Floor Shade: ",sectnum);
                if (editval)
                {
                    printmessage16(edittext);
                    sector[sectnum].floorshade = (char)getnumber16(edittext,(int32_t)sector[sectnum].floorshade,128L,1L);
                }
                break;

            case 3:
                for (i=Bsprintf(disptext,"Z-coordinate: %d",sector[sectnum].floorz); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sector %d Floor Z-coordinate: ",sectnum);
                if (editval)
                {
                    printmessage16(edittext);
                    sector[sectnum].floorz = getnumber16(edittext,sector[sectnum].floorz,8388608L,1); //2147483647L,-2147483648L
                }
                break;

            case 4:
                for (i=Bsprintf(disptext,"Tile number: %d",sector[sectnum].floorpicnum); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sector %d Floor Tile Number: ",sectnum);
                if (editval)
                {
                    printmessage16(edittext);
                    sector[sectnum].floorpicnum = (int16_t)getnumber16(edittext,(int32_t)sector[sectnum].floorpicnum,MAXTILES,0);
                }
                break;
            case 5:
                for (i=Bsprintf(disptext,"Floor heinum: %d",sector[sectnum].floorheinum); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sector %d Flooring Heinum: ",sectnum);
                if (editval)
                {
                    printmessage16(edittext);
                    sector[sectnum].floorheinum = (int16_t)getnumber16(edittext,(int32_t)sector[sectnum].floorheinum,65536L,1);
                }
                break;
            case 6:
                for (i=Bsprintf(disptext,"Palookup number: %d",sector[sectnum].floorpal); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sector %d Floor Palookup Number: ",sectnum);
                if (editval)
                {
                    printmessage16(edittext);
                    sector[sectnum].floorpal = (char)getnumber16(edittext,(int32_t)sector[sectnum].floorpal,MAXPALOOKUPS,0);
                }
                break;
            }
        }
        printext16(xpos,ypos+row*8,editorcolors[11],editorcolors[1],disptext,0);
        if (editval)
        {
            editval = 0;
        }
        showframe(1);
    }
    printext16(xpos,ypos+row*8,editorcolors[11],editorcolors[0],disptext,0);
    // printmessage16("");
    enddrawing();
    showframe(1);
    keystatus[KEYSC_ESC] = 0;
}

static void EditWallData(int16_t wallnum)
{
    char disptext[80];
    char edittext[80];
    int32_t row=0, dispwidth = 24, editval = 0, i = -1;
    int32_t xpos = 208, ypos = ydim-STATUS2DSIZ+48;

    drawgradient();

    disptext[dispwidth] = 0;

    showwalldata(wallnum, 0);
    begindrawing();
    while (keystatus[KEYSC_ESC] == 0)
    {
        idle_waitevent();
        if (handleevents())
            quitevent = 0;

        _printmessage16("Edit mode, press <Esc> to exit");
        if (PRESSED_KEYSC(DOWN))
        {
            if (row < 6)
            {
                printext16(xpos,ypos+row*8,editorcolors[11],editorcolors[0],disptext,0);
                row++;
            }
        }
        if (PRESSED_KEYSC(UP))
        {
            if (row > 0)
            {
                printext16(xpos,ypos+row*8,editorcolors[11],editorcolors[0],disptext,0);
                row--;
            }
        }
        if (PRESSED_KEYSC(ENTER))
            editval = 1;

        switch (row)
        {
        case 0:
            for (i=Bsprintf(disptext,"Flags (hex): %x",wall[wallnum].cstat); i < dispwidth; i++) disptext[i] = ' ';
            Bsprintf(edittext,"Wall %d Flags: ",wallnum);
            if (editval)
            {
                printmessage16(edittext);
                wall[wallnum].cstat = (int16_t)getnumber16(edittext,(int32_t)wall[wallnum].cstat,65536L,0);
            }
            break;
        case 1:
            for (i=Bsprintf(disptext,"Shade: %d",wall[wallnum].shade); i < dispwidth; i++) disptext[i] = ' ';
            Bsprintf(edittext,"Wall %d Shade: ",wallnum);
            if (editval)
            {
                printmessage16(edittext);
                wall[wallnum].shade = (char)getnumber16(edittext,(int32_t)wall[wallnum].shade,128,1);
            }
            break;
        case 2:
            for (i=Bsprintf(disptext,"Pal: %d",wall[wallnum].pal); i < dispwidth; i++) disptext[i] = ' ';
            Bsprintf(edittext,"Wall %d Pal: ",wallnum);
            if (editval)
            {
                printmessage16(edittext);
                wall[wallnum].pal = (char)getnumber16(edittext,(int32_t)wall[wallnum].pal,MAXPALOOKUPS,0);
            }
            break;
        case 3:
            for (i=Bsprintf(disptext,"(X,Y)repeat: %d, %d",wall[wallnum].xrepeat,wall[wallnum].yrepeat); i < dispwidth; i++) disptext[i] = ' ';
            if (editval)
            {
                Bsprintf(edittext,"Wall %d X Repeat: ",wallnum);
                printmessage16(edittext);
                wall[wallnum].xrepeat = (char)getnumber16(edittext,(int32_t)wall[wallnum].xrepeat,256L,0);
                Bsprintf(edittext,"Wall %d Y Repeat: ",wallnum);
                printmessage16(edittext);
                wall[wallnum].yrepeat = (char)getnumber16(edittext,(int32_t)wall[wallnum].yrepeat,256L,0);
            }
            break;
        case 4:
            for (i=Bsprintf(disptext,"(X,Y)pan: %d, %d",wall[wallnum].xpanning,wall[wallnum].ypanning); i < dispwidth; i++) disptext[i] = ' ';
            if (editval)
            {
                Bsprintf(edittext,"Wall %d X Pan: ",wallnum);
                printmessage16(edittext);
                wall[wallnum].xpanning = (char)getnumber16(edittext,(int32_t)wall[wallnum].xpanning,256L,0);
                Bsprintf(edittext,"Wall %d Y Pan: ",wallnum);
                printmessage16(edittext);
                wall[wallnum].ypanning = (char)getnumber16(edittext,(int32_t)wall[wallnum].ypanning,256L,0);
            }
            break;
        case 5:
            for (i=Bsprintf(disptext,"Tile number: %d",wall[wallnum].picnum); i < dispwidth; i++) disptext[i] = ' ';
            Bsprintf(edittext,"Wall %d Tile number: ",wallnum);
            if (editval)
            {
                printmessage16(edittext);
                wall[wallnum].picnum = (int16_t)getnumber16(edittext,(int32_t)wall[wallnum].picnum,MAXTILES,0);
            }
            break;

        case 6:
            for (i=Bsprintf(disptext,"OverTile number: %d",wall[wallnum].overpicnum); i < dispwidth; i++) disptext[i] = ' ';
            Bsprintf(edittext,"Wall %d OverTile number: ",wallnum);
            if (editval)
            {
                printmessage16(edittext);
                wall[wallnum].overpicnum = (int16_t)getnumber16(edittext,(int32_t)wall[wallnum].overpicnum,MAXTILES,0);
            }
            break;
        }
        printext16(xpos,ypos+row*8,editorcolors[11],editorcolors[1],disptext,0);
        if (editval)
        {
            editval = 0;
            //showwalldata(wallnum, 0);
            //// printmessage16("");
        }
        //enddrawing();
        showframe(1);
    }
    //begindrawing();
    printext16(xpos,ypos+row*8,editorcolors[11],editorcolors[0],disptext,0);
    // printmessage16("");
    enddrawing();
    showframe(1);
    keystatus[KEYSC_ESC] = 0;
}

static void EditSpriteData(int16_t spritenum)
{
    char disptext[80];
    char edittext[80];
    int32_t col=0, row=0, rowmax=4, dispwidth = 24, editval = 0, i = -1;
    int32_t xpos = 8, ypos = ydim-STATUS2DSIZ+48;

    drawgradient();

    disptext[dispwidth] = 0;
    //    clearmidstatbar16();

    showspritedata(spritenum, 0);

    while (keystatus[KEYSC_ESC] == 0)
    {
        idle_waitevent();
        begindrawing();
        if (handleevents())
        {
            if (quitevent) quitevent = 0;
        }

        _printmessage16("Edit mode, press <Esc> to exit");
        if (PRESSED_KEYSC(DOWN))
        {
            if (row < rowmax)
            {
                printext16(xpos,ypos+row*8,editorcolors[11],editorcolors[0],disptext,0);
                row++;
            }
        }
        if (PRESSED_KEYSC(UP))
        {
            if (row > 0)
            {
                printext16(xpos,ypos+row*8,editorcolors[11],editorcolors[0],disptext,0);
                row--;
            }
        }
        if (PRESSED_KEYSC(LEFT))
        {
            switch (col)
            {
            case 1:
            {
                printext16(xpos,ypos+row*8,editorcolors[11],editorcolors[0],disptext,0);
                col = 0;
                xpos = 8;
                rowmax = 4;
                dispwidth = 23;
                disptext[dispwidth] = 0;
                if (row > rowmax) row = rowmax;
            }
            break;
            case 2:
            {
                printext16(xpos,ypos+row*8,editorcolors[11],editorcolors[0],disptext,0);
                col = 1;
                xpos = 208;
                rowmax = 5;
                dispwidth = 24;
                disptext[dispwidth] = 0;
                if (row > rowmax) row = rowmax;
            }
            break;
            }
        }
        if (PRESSED_KEYSC(RIGHT))
        {
            switch (col)
            {
            case 0:
            {
                printext16(xpos,ypos+row*8,editorcolors[11],editorcolors[0],disptext,0);
                col = 1;
                xpos = 208;
                rowmax = 5;
                dispwidth = 24;
                disptext[dispwidth] = 0;
                if (row > rowmax) row = rowmax;
            }
            break;
            case 1:
            {
                printext16(xpos,ypos+row*8,editorcolors[11],editorcolors[0],disptext,0);
                col = 2;
                xpos = 408;
                rowmax = 6;
                dispwidth = 26;
                disptext[dispwidth] = 0;
                if (row > rowmax) row = rowmax;
            }
            break;
            }
        }
        if (PRESSED_KEYSC(ENTER))
            editval = 1;

        switch (col)
        {
        case 0:
        {
            switch (row)
            {
            case 0:
            {
                for (i=Bsprintf(disptext,"X-coordinate: %d",sprite[spritenum].x); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d X-coordinate: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].x = getnumber16(edittext,sprite[spritenum].x,131072,1);
                }
            }
            break;
            case 1:
            {
                for (i=Bsprintf(disptext,"Y-coordinate: %d",sprite[spritenum].y); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Y-coordinate: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].y = getnumber16(edittext,sprite[spritenum].y,131072,1);
                }
            }
            break;
            case 2:
            {
                for (i=Bsprintf(disptext,"Z-coordinate: %d",sprite[spritenum].z); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Z-coordinate: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].z = getnumber16(edittext,sprite[spritenum].z,8388608,1); //2147483647L,-2147483648L
                }
            }
            break;
            case 3:
            {
                for (i=Bsprintf(disptext,"Sectnum: %d",sprite[spritenum].sectnum); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Sectnum: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    i = getnumber16(edittext,sprite[spritenum].sectnum,numsectors-1,0);
                    if (i != sprite[spritenum].sectnum)
                        changespritesect(spritenum,i);
                }
            }
            break;
            case 4:
            {
                for (i=Bsprintf(disptext,"Statnum: %d",sprite[spritenum].statnum); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Statnum: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    i = getnumber16(edittext,sprite[spritenum].statnum,MAXSTATUS-1,0);
                    if (i != sprite[spritenum].statnum)
                        changespritestat(spritenum,i);
                }
            }
            break;
            }
        }
        break;
        case 1:
        {
            switch (row)
            {
            case 0:
            {
                for (i=Bsprintf(disptext,"Flags (hex): %x",sprite[spritenum].cstat); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Flags: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].cstat = (int16_t)getnumber16(edittext,(int32_t)sprite[spritenum].cstat,65536L,0);
                }
            }
            break;
            case 1:
            {
                for (i=Bsprintf(disptext,"Shade: %d",sprite[spritenum].shade); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Shade: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].shade = (char)getnumber16(edittext,(int32_t)sprite[spritenum].shade,128,1);
                }
            }
            break;
            case 2:
            {
                for (i=Bsprintf(disptext,"Pal: %d",sprite[spritenum].pal); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Pal: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].pal = (char)getnumber16(edittext,(int32_t)sprite[spritenum].pal,MAXPALOOKUPS,0);
                }
            }
            break;
            case 3:
            {
                for (i=Bsprintf(disptext,"(X,Y)repeat: %d, %d",sprite[spritenum].xrepeat,sprite[spritenum].yrepeat); i < dispwidth; i++) disptext[i] = ' ';
                if (editval)
                {
                    Bsprintf(edittext,"Sprite %d X Repeat: ",spritenum);
                    printmessage16(edittext);
                    sprite[spritenum].xrepeat = (char)getnumber16(edittext,(int32_t)sprite[spritenum].xrepeat,256L,0);
                    Bsprintf(edittext,"Sprite %d Y Repeat: ",spritenum);
                    printmessage16(edittext);
                    sprite[spritenum].yrepeat = (char)getnumber16(edittext,(int32_t)sprite[spritenum].yrepeat,256L,0);
                }
            }
            break;
            case 4:
            {
                for (i=Bsprintf(disptext,"(X,Y)offset: %d, %d",sprite[spritenum].xoffset,sprite[spritenum].yoffset); i < dispwidth; i++) disptext[i] = ' ';
                if (editval)
                {
                    Bsprintf(edittext,"Sprite %d X Offset: ",spritenum);
                    printmessage16(edittext);
                    sprite[spritenum].xoffset = (char)getnumber16(edittext,(int32_t)sprite[spritenum].xoffset,128L,1);
                    Bsprintf(edittext,"Sprite %d Y Offset: ",spritenum);
                    printmessage16(edittext);
                    sprite[spritenum].yoffset = (char)getnumber16(edittext,(int32_t)sprite[spritenum].yoffset,128L,1);
                }
            }
            break;
            case 5:
            {
                for (i=Bsprintf(disptext,"Tile number: %d",sprite[spritenum].picnum); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Tile number: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].picnum = (int16_t)getnumber16(edittext,(int32_t)sprite[spritenum].picnum,MAXTILES,0);
                }
            }
            break;
            }
        }
        break;
        case 2:
        {
            switch (row)
            {
            case 0:
            {
                for (i=Bsprintf(disptext,"Angle (2048 degrees): %d",sprite[spritenum].ang); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Angle: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].ang = (int16_t)getnumber16(edittext,(int32_t)sprite[spritenum].ang,2048L,0);
                }
            }
            break;
            case 1:
            {
                for (i=Bsprintf(disptext,"X-Velocity: %d",sprite[spritenum].xvel); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d X-Velocity: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].xvel = getnumber16(edittext,(int32_t)sprite[spritenum].xvel,65536,1);
                }
            }
            break;
            case 2:
            {
                for (i=Bsprintf(disptext,"Y-Velocity: %d",sprite[spritenum].yvel); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Y-Velocity: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].yvel = getnumber16(edittext,(int32_t)sprite[spritenum].yvel,65536,1);
                }
            }
            break;
            case 3:
            {
                for (i=Bsprintf(disptext,"Z-Velocity: %d",sprite[spritenum].zvel); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Z-Velocity: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].zvel = getnumber16(edittext,(int32_t)sprite[spritenum].zvel,65536,1);
                }
            }
            break;
            case 4:
            {
                for (i=Bsprintf(disptext,"Owner: %d",sprite[spritenum].owner); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Owner: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].owner = getnumber16(edittext,(int32_t)sprite[spritenum].owner,MAXSPRITES,1);
                }
            }
            break;
            case 5:
            {
                for (i=Bsprintf(disptext,"Clipdist: %d",sprite[spritenum].clipdist); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Clipdist: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].clipdist = (char)getnumber16(edittext,(int32_t)sprite[spritenum].clipdist,256,0);
                }
            }
            break;
            case 6:
            {
                for (i=Bsprintf(disptext,"Extra: %d",sprite[spritenum].extra); i < dispwidth; i++) disptext[i] = ' ';
                Bsprintf(edittext,"Sprite %d Extra: ",spritenum);
                if (editval)
                {
                    printmessage16(edittext);
                    sprite[spritenum].extra = getnumber16(edittext,(int32_t)sprite[spritenum].extra,BTAG_MAX,1);
                }
            }
            break;
            }
        }
        break;
        }

        printext16(xpos,ypos+row*8,editorcolors[11],editorcolors[1],disptext,0);
        if (editval)
        {
            editval = 0;
        }
        enddrawing();
        showframe(1);
    }
    begindrawing();
    printext16(xpos,ypos+row*8,editorcolors[11],editorcolors[0],disptext,0);
    // printmessage16("");
    enddrawing();
    showframe(1);
    keystatus[KEYSC_ESC] = 0;
}

static void GenericSpriteSearch()
{
    char disptext[80];
    char edittext[80];
    static int32_t col=0, row=0;
    int32_t i, j, k;
    int32_t rowmax[3]= {6,5,6}, dispwidth[3] = {24,24,28};
    int32_t xpos[3] = {8,200,400}, ypos = ydim-STATUS2DSIZ+48;

    static char *labels[7][3] =
    {
        {"X-coordinate", "Flags (hex)", "Angle (2048 degrees)"},
        {"Y-coordinate", "Shade",       "X-Velocity"},
        {"Z-coordinate", "Pal",         "Y-Velocity"},
        {"Sectnum",      "(X/Y)repeat", "Z-Velocity"},
        {"Statnum",      "(X/Y)offset", "Owner"},
        {"Hitag",        "Tile number", "Clipdist"},
        {"Lotag",        "",            "Extra"}
    };

    static int32_t maxval[7][3] =
    {
        { 524288      , 65536       , 2048 },
        { 524288      , 128         , 65536 },
        { 8388608     , MAXPALOOKUPS, 65536 },
        { MAXSECTORS-1, 128         , 65536 },
        { MAXSTATUS-1 , 128         , MAXSPRITES-1 },
        { BTAG_MAX    , MAXTILES    , 256 },
        { BTAG_MAX    , 0           , BTAG_MAX }
    };

    static char sign[7][3] =
    {
        {1, 0, 1},
        {1, 1, 1},
        {1, 0, 1},
        {0, 0, 1},
        {0, 1, 0},
        {0, 0, 0},
        {0, 0, 1}
    };

    static char firstrun=1;

    if (firstrun)
    {
        firstrun = 0;
        Bmemset(&gs_spritewhat, 0, sizeof(gs_spritewhat));
    }

    clearmidstatbar16();

    drawgradient();

    printext16(xpos[0], ypos-2*8, editorcolors[10], editorcolors[0], "Sprite search", 0);

    for (i=0; i<3; i++)
        for (j=0; j<=rowmax[i]; j++)
        {
            if (gs_spritewhat[i][j])
                k=Bsprintf(disptext, "%s: %d", labels[j][i], gs_sprite[i][j]);
            else
                k=Bsprintf(disptext, "%s: ^7any", labels[j][i]);
            for (; k<dispwidth[i]; k++) disptext[k] = 0;

            printext16(xpos[i], ypos+j*8, editorcolors[11], editorcolors[0], disptext, 0);
        }
    for (k=0; k<80; k++) disptext[k] = 0;

    //    disptext[dispwidth[col]] = 0;
    //    showspritedata(spritenum, 0);
    wallsprite = 2;

    while (keystatus[KEYSC_ESC] == 0)
    {
        idle_waitevent();
        if (handleevents())
        {
            if (quitevent) quitevent = 0;
        }

        begindrawing();
        printmessage16("Sprite search, press <Esc> to exit");

        if (PRESSED_KEYSC(DOWN))
        {
            if (row < rowmax[col])
            {
                printext16(xpos[col],ypos+row*8,editorcolors[11],editorcolors[0],disptext,0);
                row++;
            }
        }
        if (PRESSED_KEYSC(UP))
        {
            if (row > 0)
            {
                printext16(xpos[col],ypos+row*8,editorcolors[11],editorcolors[0],disptext,0);
                row--;
            }
        }
        if (PRESSED_KEYSC(LEFT))
        {
            if (col > 0)
            {
                printext16(xpos[col],ypos+row*8,editorcolors[11],editorcolors[0],disptext,0);
                col--;
                disptext[dispwidth[col]] = 0;
                if (row > rowmax[col]) row = rowmax[col];
            }
        }
        if (PRESSED_KEYSC(RIGHT))
        {
            if (col < 2)
            {
                printext16(xpos[col],ypos+row*8,editorcolors[11],editorcolors[0],disptext,0);
                col++;
                disptext[dispwidth[col]] = 0;
                if (row > rowmax[col]) row = rowmax[col];
            }
        }
        if (PRESSED_KEYSC(ENTER))
        {
            Bsprintf(edittext, "%s: ", labels[row][col]);
            enddrawing();
            printmessage16(edittext);
            i = getnumber16(edittext, gs_spritewhat[col][row] ? gs_sprite[col][row] : 0,
                            maxval[row][col], sign[row][col]);
            if (col == 2 && row == 0) i = (i+2048)&2047;  // angle
            gs_sprite[col][row] = i;
            begindrawing();
            gs_spritewhat[col][row] = 1;

            if (col == 1 && row == 5)  // picnum
                printext16(xpos[1], ypos-2*8, editorcolors[14], editorcolors[0], names[i], 0);
        }
        if (PRESSED_KEYSC(BS) || PRESSED_KEYSC(DELETE))
        {
            gs_spritewhat[col][row] = 0;

            if (col == 1 && row == 5)  // picnum
                printext16(xpos[1], ypos-2*8, editorcolors[14], editorcolors[0], "                         ", 0);
        }

        if (gs_spritewhat[col][row])
        {
            if (col == 1 && row == 0)  // flags
                k = Bsprintf(disptext, "%s: %x", labels[row][col], gs_sprite[col][row]);
            else
                k = Bsprintf(disptext, "%s: %d", labels[row][col], gs_sprite[col][row]);
        }
        else
            k = Bsprintf(disptext, "%s: ^7any", labels[row][col]);
        for (; k<dispwidth[col]; k++) disptext[k] = ' ';
        disptext[k] = 0;

        printext16(xpos[col],ypos+row*8,editorcolors[11],editorcolors[1],disptext,0);

        enddrawing();
        showframe(1);
    }
    //    begindrawing();
    printext16(xpos[col],ypos+row*8,editorcolors[11],editorcolors[0],disptext,0);
    printmessage16("Search sprite");
    //    enddrawing();
    showframe(1);
    keystatus[KEYSC_ESC] = 0;
}

// Build edit

static const char *FuncMenuStrings[] =
{
    "Replace invalid tiles",
    "Delete all spr of tile #",
    "Set map sky shade",
    "Set map sky height",
    "Global Z coord shift",
    "Resize selection",
    "Global shade divide",
    "Global visibility divide"
};

#define MENU_Y_SPACING 8
#define MENU_BASE_Y ydim-STATUS2DSIZ+32


static void FuncMenuOpts(void)
{
    int32_t x = 8;
    int32_t y = MENU_BASE_Y+16;
    int32_t i = 0;
    //  int32_t x2 = 0;
    //    static int32_t x2_max = 0;

    int32_t numopts = (sizeof(FuncMenuStrings)/sizeof(FuncMenuStrings[0]));

    do
    {
        //        x2 =
        printext16(x,y,editorcolors[11],editorcolors[0],FuncMenuStrings[i],0);
        //    if (x2 > x2_max) x2_max = x2;
        y += MENU_Y_SPACING;
    }
    while (++i < numopts);
    //    drawline16(x-1,y,x2_max+1,y,1);
    //  drawline16(x-1,MENU_BASE_Y-4,x-1,y,1);

    //    x2 =
    printext16(x,MENU_BASE_Y,editorcolors[11],-1,"Special functions",0);
    //    drawline16(x-1,MENU_BASE_Y-4,x2+1,MENU_BASE_Y-4,1);
    //  drawline16(x2_max+1,MENU_BASE_Y+16-4,x2_max+1,y-1,1);
    //drawline16(x2+1,MENU_BASE_Y+16-1,x2_max+1,MENU_BASE_Y+16-1,1);
}

static void FuncMenu(void)
{
    char disptext[80];
    int32_t col=0, row=0, rowmax=7, dispwidth = 24, editval = 0, i = -1, j;
    int32_t xpos = 8, ypos = MENU_BASE_Y+16;

    drawgradient();

    disptext[dispwidth] = 0;
    //    clearmidstatbar16();

    FuncMenuOpts();

    while (!editval && keystatus[KEYSC_ESC] == 0)
    {
        idle_waitevent();
        if (handleevents())
            quitevent = 0;

        begindrawing();
        _printmessage16("Select an option, press <Esc> to exit");
        if (PRESSED_KEYSC(DOWN))
        {
            if (row < rowmax)
            {
                printext16(xpos,ypos+row*MENU_Y_SPACING,editorcolors[11],editorcolors[0],disptext,0);
                row++;
            }
        }
        if (PRESSED_KEYSC(UP))
        {
            if (row > 0)
            {
                printext16(xpos,ypos+row*MENU_Y_SPACING,editorcolors[11],editorcolors[0],disptext,0);
                row--;
            }
        }
#if 0
        if (PRESSED_KEYSC(LEFT))
        {
            /*            if (col == 2)
            {
            printext16(xpos,ypos+row*8,editorcolors[11],0,disptext,0);
            col = 1;
            xpos = 200;
            rowmax = 6;
            dispwidth = 24;
            disptext[dispwidth] = 0;
            if (row > rowmax) row = rowmax;
            }
            else */
            if (col == 1)
            {
                printext16(xpos,ypos+row*8,editorcolors[11],0,disptext,0);
                col = 0;
                xpos = 8;
                rowmax = 7;
                dispwidth = 24;
                disptext[dispwidth] = 0;
                if (row > rowmax) row = rowmax;
            }
        }

        if (PRESSED_KEYSC(RIGHT))
        {
            if (col == 0)
            {
                printext16(xpos,ypos+row*8,editorcolors[11],0,disptext,0);
                col = 1;
                xpos = 200;
                rowmax = 0;
                dispwidth = 24;
                disptext[dispwidth] = 0;
                if (row > rowmax) row = rowmax;
            }
            /*            else if (col == 1)
            {
            printext16(xpos,ypos+row*8,editorcolors[11],0,disptext,0);
            col = 2;
            xpos = 400;
            rowmax = 6;
            dispwidth = 26;
            disptext[dispwidth] = 0;
            if (row > rowmax) row = rowmax;
            } */
        }
#endif
        if (PRESSED_KEYSC(ENTER))
            editval = 1;

        switch (col)
        {
        case 0:
            switch (row)
            {
            case 0:
            {
                for (i=Bsprintf(disptext,"%s",FuncMenuStrings[row]); i < dispwidth; i++) disptext[i] = ' ';
                if (editval)
                {
                    j = 0;
                    for (i=0; i<MAXSECTORS; i++)
                    {
                        if (tilesizx[sector[i].ceilingpicnum] <= 0)
                            sector[i].ceilingpicnum = 0,j++;
                        if (tilesizx[sector[i].floorpicnum] <= 0)
                            sector[i].floorpicnum = 0,j++;
                    }
                    for (i=0; i<MAXWALLS; i++)
                    {
                        if (tilesizx[wall[i].picnum] <= 0)
                            wall[i].picnum = 0,j++;
                        if (tilesizx[wall[i].overpicnum] <= 0)
                            wall[i].overpicnum = 0,j++;
                    }
                    for (i=0; i<MAXSPRITES; i++)
                    {
                        if (tilesizx[sprite[i].picnum] <= 0)
                            sprite[i].picnum = 0,j++;
                    }
                    Bsprintf(tempbuf,"Replaced %d invalid tiles",j);
                    printmessage16(tempbuf);
                }
            }
            break;
            case 1:
            {
                for (i=Bsprintf(disptext,"%s",FuncMenuStrings[row]); i < dispwidth; i++) disptext[i] = ' ';
                if (editval)
                {
                    Bsprintf(tempbuf,"Delete all sprites of tile #: ");
                    i = getnumber16(tempbuf,-1,MAXSPRITES-1,1);
                    if (i >= 0)
                    {
                        int32_t k = 0;
                        for (j=0; j<MAXSPRITES-1; j++)
                            if (sprite[j].picnum == i)
                                deletesprite(j), k++;
                        Bsprintf(tempbuf,"%d sprite(s) deleted",k);
                        printmessage16(tempbuf);
                    }
                    else printmessage16("Aborted");
                }
            }
            break;
            case 2:
            {
                for (i=Bsprintf(disptext,"%s",FuncMenuStrings[row]); i < dispwidth; i++) disptext[i] = ' ';
                if (editval)
                {
                    j=getnumber16("Set map sky shade:    ",0,128,1);

                    for (i=0; i<numsectors; i++)
                    {
                        if (sector[i].ceilingstat&1)
                            sector[i].ceilingshade = j;
                    }
                    printmessage16("All parallax skies adjusted");
                }
            }
            break;
            case 3:
            {
                for (i=Bsprintf(disptext,"%s",FuncMenuStrings[row]); i < dispwidth; i++) disptext[i] = ' ';
                if (editval)
                {
                    j=getnumber16("Set map sky height:    ",0,16777216,1);
                    if (j != 0)
                    {
                        for (i=0; i<numsectors; i++)
                        {
                            if (sector[i].ceilingstat&1)
                                sector[i].ceilingz = j;
                        }
                        printmessage16("All parallax skies adjusted");
                    }
                    else printmessage16("Aborted");
                }
            }
            break;
            case 4:
            {
                for (i=Bsprintf(disptext,"%s",FuncMenuStrings[row]); i < dispwidth; i++) disptext[i] = ' ';
                if (editval)
                {
                    j=getnumber16("Z offset:    ",0,16777216,1);
                    if (j!=0)
                    {
                        for (i=0; i<numsectors; i++)
                        {
                            sector[i].ceilingz += j;
                            sector[i].floorz += j;
                        }
                        for (i=0; i<MAXSPRITES; i++)
                            sprite[i].z += j;
                        printmessage16("Map adjusted");
                    }
                    else printmessage16("Aborted");
                }
            }
            break;
            case 5:
            {
                for (i=Bsprintf(disptext,"%s",FuncMenuStrings[row]); i < dispwidth; i++) disptext[i] = ' ';
                if (editval)
                {
                    j=getnumber16("Percentage of original:    ",100,1000,0);
                    if (j!=100)
                    {
                        int32_t w, currsector, start_wall, end_wall;
                        double size = (j/100.f);
                        for (i = 0; i < highlightsectorcnt; i++)
                        {
                            currsector = highlightsector[i];
                            sector[currsector].ceilingz = (int32_t)(sector[currsector].ceilingz*size);
                            sector[currsector].floorz = (int32_t)(sector[currsector].floorz*size);
                            // Do all the walls in the sector
                            start_wall = sector[currsector].wallptr;
                            end_wall = start_wall + sector[currsector].wallnum;
                            for (w = start_wall; w < end_wall; w++)
                            {
                                wall[w].x = (int32_t)(wall[w].x*size);
                                wall[w].y = (int32_t)(wall[w].y*size);
                                wall[w].yrepeat = min((int32_t)(wall[w].yrepeat/size),255);
                            }
                            w = headspritesect[highlightsector[i]];
                            while (w >= 0)
                            {
                                sprite[w].x = (int32_t)(sprite[w].x*size);
                                sprite[w].y = (int32_t)(sprite[w].y*size);
                                sprite[w].z = (int32_t)(sprite[w].z*size);
                                sprite[w].xrepeat = min(max((int32_t)(sprite[w].xrepeat*size),1),255);
                                sprite[w].yrepeat = min(max((int32_t)(sprite[w].yrepeat*size),1),255);
                                w = nextspritesect[w];
                            }
                        }
                        printmessage16("Map scaled");
                    }
                    else printmessage16("Aborted");
                }
            }
            break;
            case 6:
            {
                for (i=Bsprintf(disptext,"%s",FuncMenuStrings[row]); i < dispwidth; i++) disptext[i] = ' ';
                if (editval)
                {
                    j=getnumber16("Shade divisor:    ",1,128,1);
                    if (j!=1)
                    {
                        for (i=0; i<numsectors; i++)
                        {
                            sector[i].ceilingshade /= j;
                            sector[i].floorshade /= j;
                        }
                        for (i=0; i<numwalls; i++)
                            wall[i].shade /= j;
                        for (i=0; i<MAXSPRITES; i++)
                            sprite[i].shade /= j;
                        printmessage16("Shades adjusted");
                    }
                    else printmessage16("Aborted");
                }
            }
            break;
            case 7:
            {
                for (i=Bsprintf(disptext,"%s",FuncMenuStrings[row]); i < dispwidth; i++) disptext[i] = ' ';
                if (editval)
                {
                    j=getnumber16("Visibility divisor:    ",1,128,0);
                    if (j!=1)
                    {
                        for (i=0; i<numsectors; i++)
                        {
                            if (sector[i].visibility < 240)
                                sector[i].visibility /= j;
                            else sector[i].visibility = 240 + (sector[i].visibility>>4)/j;
                        }
                        printmessage16("Visibility adjusted");
                    }
                    else printmessage16("Aborted");
                }
            }
            break;
            }
            break;
        }
        printext16(xpos,ypos+row*MENU_Y_SPACING,editorcolors[11],editorcolors[1],disptext,0);
        enddrawing();
        showframe(1);
    }
    begindrawing();
    printext16(xpos,ypos+row*MENU_Y_SPACING,editorcolors[11],editorcolors[0],disptext,0);
    enddrawing();
    /*clearmidstatbar16();*/
    showframe(1);
    keystatus[KEYSC_ESC] = 0;
}
